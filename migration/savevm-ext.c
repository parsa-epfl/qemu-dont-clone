/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 * Copyright (c) 2009-2015 Red Hat Inc
 *
 * Authors:
 *  Klim Kireev <klim.s.kireev@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qapi/qmp/qstring.h"
#include "qapi/qmp/qerror.h"
#include "block/block_int.h"
#include "io/channel-command.h"
#include "qemu-common.h"

#include "hw/boards.h"
#include "hw/hw.h"
#include "hw/qdev.h"
#include "hw/xen/xen.h"
#include "net/net.h"
#include "monitor/monitor.h"
#include "sysemu/sysemu.h"
#include "qemu/timer.h"
#include "audio/audio.h"
#include "migration/migration.h"
#include "migration/postcopy-ram.h"
#include "migration/global_state.h"
#include "qemu/error-report.h"
#include "qemu/sockets.h"
#include "qemu/queue.h"
#include "sysemu/cpus.h"
#include "exec/memory.h"
#include "qmp-commands.h"
#include "trace.h"
#include "savevm.h"
#include "qemu/bitops.h"
#include "qemu/iov.h"
#include "block/snapshot.h"
#include "block/qapi.h"
#include "qemu/cutils.h"
#include "io/channel-buffer.h"
#include "io/channel-file.h"
#include "qemu-file-channel.h"
#include "qemu-file.h"

const char *input_command = "gunzip -c";
const char *output_command = "gzip -c";

static int qemu_savevm_state(QEMUFile *f, Error **errp)
{
    int ret;
    MigrationState *ms = migrate_init();
    MigrationStatus status;
    ms->to_dst_file = f;

    if (migration_is_blocked(errp)) {
        ret = -EINVAL;
        goto done;
    }

    if (migrate_use_block()) {
        error_setg(errp, "Saving vm and internal snapshots are incompatible");
        ret = -EINVAL;
        goto done;
    }

    qemu_mutex_unlock_iothread();
    qemu_savevm_state_header(f);
    qemu_savevm_state_setup(f);
    qemu_mutex_lock_iothread();

    while (qemu_file_get_error(f) == 0) {
        if (qemu_savevm_state_iterate(f, false) > 0) {
            break;
        }
    }

    ret = qemu_file_get_error(f);
    if (ret == 0) {
        qemu_savevm_state_complete_precopy(f, false, false);
        ret = qemu_file_get_error(f);
    }
    qemu_savevm_state_cleanup();
    if (ret != 0) {
        error_setg_errno(errp, -ret, "Error while writing VM state");
    }

done:
    if (ret != 0) {
        status = MIGRATION_STATUS_FAILED;
    } else {
        status = MIGRATION_STATUS_COMPLETED;
    }
    migrate_set_state(&ms->state, MIGRATION_STATUS_SETUP, status);

    /* f is outer parameter, it should not stay in global migration state after
     * this function finished */
    ms->to_dst_file = NULL;

    return ret;
}

static void qlist_push(QList *qlist, QObject *value)
{
    QListEntry *entry;

    entry = g_malloc(sizeof(*entry));
    entry->value = value;

    QTAILQ_INSERT_HEAD(&qlist->head, entry, next);
}

static bool isNumber(const char* name)
{
    while (*name) {
        if (!isdigit(*name)) {
                return false;
        }
        name++;
    }
    return true;
}

static BlockDriverState* find_active(void)
{
    BdrvNextIterator bi;
    BlockDriverState *bs = bdrv_first(&bi);
    while (bs && bs->read_only) {
        bs = bdrv_next(&bi);
    }
    return bs;
}

static BlockDriverState* get_base(BlockDriverState *bs)
{
    BlockDriverState* base_bs = bs;
    while (base_bs && base_bs->backing) {
        base_bs = base_bs->backing->bs;
    }
    return base_bs;
}

static char *get_file_name_from_file_path(const char *file_path) {
    char *word;
    word = strrchr(file_path, '/');
    if (word == NULL) {
        return NULL;
    }
    word++; // Get rid of '/'
    return word;
}

static QString *get_dir_path_from_file_path(const char *file_path) {
    char *end = NULL;
    size_t size = 0;
    end = strrchr(file_path, '/');
    if (end == NULL) {
        return NULL;
    }
    size = end - file_path;
    return qstring_from_substr(file_path, 0, size - 1);
}

static char *generate_name(char* filename, int inc) {
    const char* vmname = qemu_get_vm_name();
    char *tmp_name = g_strdup("");
    srand(time(NULL));
    long unsigned int rand = random() + inc;
    if (vmname != NULL) {
        tmp_name = g_strdup_printf("%s-%08lX-%s-tmp", filename, rand, vmname);
    } else {
        tmp_name = g_strdup_printf("%s-%08lX-tmp", filename, rand);
    }
    return tmp_name;
}

static QList *get_snap_chain(BlockDriverState *bs) {
    QList* lst = qlist_new();

    bs = bs->backing->bs;

    while (bs->backing != NULL) {
        QString *snap = get_dir_path_from_file_path(bs->filename);
        qlist_push(lst, QOBJECT(snap));
        bs = bs->backing->bs;
    }

    return lst;
}

int delete_tmp_overlay(void) {
    BlockDriverState *bs = find_active();
    if (bs == NULL) {
        return -EINVAL;
    }
    return unlink(bs->filename);
}

int create_tmp_overlay(void) {
    Error *local_err = NULL;
    BlockDriverState *bs = find_active();
    BlockDriverState *base_bs = get_base(bs);
    char *tmp_name;
    const char *dev_name = bdrv_get_device_name(bs);
    int i = 0;

    do {
        char *filename = base_bs->filename;
        tmp_name = generate_name(filename, i);
        i++;
    } while (!access(tmp_name, F_OK));



    qmp_blockdev_snapshot_sync(true, dev_name, false, NULL,
                                     tmp_name,
                                     false,
                                     NULL,
                                     true, "qcow2",
                                     true, 1, &local_err);

    g_free(tmp_name);

    if (local_err != NULL) {
        error_report_err(local_err);
        return -EINVAL;
    }

    return 0;
}

static int mkdir_p(const char *dir, const mode_t mode) {
    char tmp[PATH_MAX];
    char *p = NULL;
    struct stat sb;
    size_t len;

    /* copy path */
    strncpy(tmp, dir, sizeof(tmp));
    len = strlen(dir);
    if (len >= sizeof(tmp)) {
        return -1;
    }

    /* remove trailing slash */
    if(tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    /* recursive mkdir */
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            /* test path */
            if (stat(tmp, &sb) != 0) {
                /* path does not exist - create directory */
                if (mkdir(tmp, mode) < 0) {
                    return -1;
                }
            } else if (!S_ISDIR(sb.st_mode)) {
                /* not a directory */
                return -1;
            }
            *p = '/';
        }
    }
    /* test path */
    if (stat(tmp, &sb) != 0) {
        /* path does not exist - create directory */
        if (mkdir(tmp, mode) < 0) {
            return -1;
        }
    } else if (!S_ISDIR(sb.st_mode)) {
        /* not a directory */
        return -1;
    }
    return 0;
}

static char *gen_snap_path(const char *snap_name, char *file_path) {
    char *snap_path = g_strdup("");
    QString *dir_path = get_dir_path_from_file_path(file_path);
    char *base_name = get_file_name_from_file_path(file_path);
    snap_path = g_strdup_printf("%s/%s/%s-sn", qstring_get_str(dir_path), snap_name, base_name);
    QDECREF(dir_path);
    return snap_path;
}

int save_vmstate_ext(Monitor *mon, const char *snap_name)
{
    int ret = -EINVAL;
    int saved_vm_running = 0;
    Error *local_err = NULL;
    BlockDriverState *bs = NULL;
    BlockDriverState *base_bs = NULL;
    QEMUFile *f = NULL;
    QString *snap_dir = NULL;
    char *snap_filepath = NULL;

    if(isNumber(snap_name)){
        monitor_printf(mon, "Error: Please don't save snapshot with numeric name\n"); // Why?
        ret = -EINVAL;
        goto end;
    }

    bs = find_active();
    base_bs = get_base(bs);
    if (bs == NULL) {
        monitor_printf(mon, "There are not block devices on current VM\n");
        ret = -ENODEV;
        goto end;
    }


    snap_filepath = gen_snap_path(snap_name, base_bs->filename);
    if (snap_filepath == NULL) {
        monitor_printf(mon, "Cannot get snapshot '%s' full path \n", snap_name);
        goto end;
    }

    snap_dir = get_dir_path_from_file_path(snap_filepath);
    if(snap_dir == NULL) {
        monitor_printf(mon, "Cannot get snap directory from '%s'\n", snap_name);
        goto end;
    }

    ret = mkdir_p(qstring_get_str(snap_dir), 0777);
    if (ret < 0) {
        monitor_printf(mon, "Cannot create directory for snapshot '%s'\n", snap_name);
        goto end;
    }

    if( access(snap_filepath, F_OK ) != -1 ) {
        monitor_printf(mon, "overwriting snapshot directory %s\n", snap_name);
        remove(snap_filepath);
    }

    ret = link(bs->filename, snap_filepath);
    if (ret < 0) {
        monitor_printf(mon, "Cannot save snapshot '%s'\n", snap_name);
        goto end;
    }
    ret = unlink(bs->filename);
    if (ret < 0) {
        monitor_printf(mon, "Cannot save snapshot '%s'\n", snap_name);
        goto end;
    }

    memset(bs->filename, 0, PATH_MAX);
    memset(bs->exact_filename, 0, PATH_MAX);
    strcpy(bs->filename, snap_filepath);
    strcpy(bs->exact_filename, snap_filepath);

    ret = create_tmp_overlay();
    if (ret < 0) {
        monitor_printf(mon, "Cannot create temporary overlay %s\n", snap_name);
        goto end;
    }

    saved_vm_running = runstate_is_running();

    ret = global_state_store();
    if (ret) {
        monitor_printf(mon, "Error saving global state\n");
        goto end;
    }
    vm_stop(RUN_STATE_SAVE_VM);

    char command[PATH_MAX] = {};
    snprintf(command, PATH_MAX, "%s > %s/mem", output_command, snap_dir->string);
    const char *argv[] = { "/bin/sh", "-c", command, NULL };

#ifdef CONFIG_FLEXUS
    flexus_doSave(snap_dir->string, &local_err);
    if (local_err)
        error_report_err(local_err);
#endif

    QIOChannel *ioc;
    ioc = QIO_CHANNEL(qio_channel_command_new_spawn(argv,
                                                    O_WRONLY,
                                                    &local_err));
    qio_channel_set_name(ioc, "migration-exec-outgoing");

    if (!ioc) {
        monitor_printf(mon, "Could not open VM state file's channel\n");
        goto end;
    }

    f = qemu_fopen_channel_output(ioc);
    if (!f) {
        monitor_printf(mon, "Could not open VM state file\n");
        goto end;
    }

    ret = qemu_savevm_state(f, &local_err);
    qemu_fclose(f);
    if (ret < 0) {
        error_report_err(local_err);
        goto end;
    }


    ret = 0;
end:
    QDECREF(snap_dir);
    g_free(snap_filepath);

    if (saved_vm_running) {
        vm_start();
    }
    return ret;
}

static int goto_snap(const char* snap) {
    int ret = -EINVAL;
    char image_path[PATH_MAX] = {};
    Error *local_err = NULL;
    BlockDriverState *bs = NULL;
    BlockDriverState *base_bs = NULL;
    BlockDriverState *new_back = NULL;
    QString *dir_path = NULL;
    char *base_name = NULL;

    bs = find_active();
    base_bs = get_base(bs);
    if (bs == NULL) {
        ret = -EINVAL;
        goto end;
    }

    ret = bs->drv->bdrv_make_empty(bs);
    dir_path = get_dir_path_from_file_path(base_bs->filename);
    base_name = get_file_name_from_file_path(base_bs->filename);

    snprintf(image_path, PATH_MAX, "%s/%s/%s-sn", qstring_get_str(dir_path),
                                       snap, base_name);

    ret = bdrv_change_backing_file(bs, image_path, "qcow2");

    if (ret < 0) {
        goto end;
    }


    new_back = bdrv_open(bs->backing_file, NULL, NULL, 0, &local_err);
    if (new_back == NULL) {
        error_report_err(local_err);
        goto end;
    }

    bdrv_set_backing_hd(bs, NULL, &local_err);
    if (local_err != NULL) {
        error_report_err(local_err);
        goto end;
    }
    bdrv_set_backing_hd(bs, new_back, &local_err);
    if (local_err != NULL) {
        error_report_err(local_err);
        goto end;
    }

    ret = 0;
end:
    bdrv_unref(new_back);
    QDECREF(dir_path);
    return ret;
}

static int load_state_ext(QString *dir_path)
{
    int ret = -EINVAL;
    char command[NAME_MAX] = {};
    Error *local_err = NULL;
    QEMUFile *f = NULL;
    MigrationIncomingState *mis = migration_incoming_get_current();

    sprintf(command, "%s %s/mem", input_command, dir_path->string);
    const char *argv[] = { "/bin/sh", "-c", command, NULL };

    QIOChannel *ioc;
    ioc = QIO_CHANNEL(qio_channel_command_new_spawn(argv,
                                                    O_RDONLY,
                                                    &local_err));
    qio_channel_set_name(ioc, "loadvm-exec-incoing");
    if (!ioc) {
        error_report("Could not open VM state file's channel\n");
        goto end;
    }

    f = qemu_fopen_channel_input(ioc);
    if (!f) {
        error_report("Cannot run a command %s\n", command);
        goto end;
    }


    mis->from_src_file = f;

    ret = qemu_loadvm_state(f);

    migration_incoming_state_destroy();
    if (ret < 0) {
        error_report("Error %d while loading vm state", ret);
        goto end;
    }

    ret = 0;
end:
    return ret;
}

int incremental_load_vmstate_ext (const char *name, Monitor *mon) {
    int saved_vm_running  = runstate_is_running();
    int ret = -EINVAL;
    BlockDriverState *bs = NULL;
    BlockDriverState *base_bs = NULL;
    QList *snap_chain = NULL;
    QString *dir_path = NULL;
    QString *path = NULL;

    if (saved_vm_running) {
        vm_stop(RUN_STATE_RESTORE_VM);
    }
    memory_global_dirty_log_start();

    bs = find_active();
    base_bs = get_base(bs);
    if (bs == NULL) {
        monitor_printf(mon, "There are not block devices on current VM\n");
        ret = -ENODEV;
        goto end;
    }

    dir_path = get_dir_path_from_file_path(base_bs->filename);
    if (dir_path == NULL) {
        monitor_printf(mon, "There are not block devices on current VM\n");
        goto end;
    }

    ret = goto_snap(name);
    if (ret < 0) {
        monitor_printf(mon, "Cannot load snapshot %s\n", name);
        goto end;
    }



    // Incremental snapshots
    // Make QList of QStrings with backtracking of snapshot directories
    snap_chain = get_snap_chain(bs);
    if (snap_chain == NULL) {
        monitor_printf(mon, "Cannot build snapshot chain on current VM\n");
        goto end;
    }

    bdrv_drain_all();
    if (qlist_empty(snap_chain)) {
        monitor_printf(mon, "The snapshot chain is empty\n");
        goto end;
    }

    // Load incrementally snapshots
    while ((path = qobject_to_qstring(qlist_pop(snap_chain))) != NULL) {
        vm_start();
        vm_stop(RUN_STATE_RESTORE_VM);

        ret = load_state_ext(path);
        if (ret < 0) {
            monitor_printf(mon, "Cannot load memory for snapshot located in %s\n", path->string);
            goto end;
        }

#ifdef CONFIG_FLEXUS
        set_flexus_load_dir(path->string);
#endif

        QDECREF(path);
    }

    ret = 0;
end:
    QDECREF(snap_chain);
    QDECREF(dir_path);
    QDECREF(path);

    if (saved_vm_running) {
        vm_start();
    }
    return ret;
}
