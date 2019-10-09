/*
 * Human Monitor Interface
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#ifndef HMP_H
#define HMP_H

#include "qemu-common.h"
#include "qemu/readline.h"
#include "qapi-types.h"
#include "qapi/qmp/qdict.h"

void hmp_info_name(Monitor *mon, const QDict *qdict);
void hmp_info_version(Monitor *mon, const QDict *qdict);
void hmp_info_kvm(Monitor *mon, const QDict *qdict);
void hmp_info_status(Monitor *mon, const QDict *qdict);
void hmp_info_uuid(Monitor *mon, const QDict *qdict);
void hmp_info_chardev(Monitor *mon, const QDict *qdict);
void hmp_info_mice(Monitor *mon, const QDict *qdict);
void hmp_info_migrate(Monitor *mon, const QDict *qdict);
void hmp_info_migrate_capabilities(Monitor *mon, const QDict *qdict);
void hmp_info_migrate_parameters(Monitor *mon, const QDict *qdict);
void hmp_info_migrate_cache_size(Monitor *mon, const QDict *qdict);
void hmp_info_cpus(Monitor *mon, const QDict *qdict);
void hmp_info_block(Monitor *mon, const QDict *qdict);
void hmp_info_blockstats(Monitor *mon, const QDict *qdict);
void hmp_info_vnc(Monitor *mon, const QDict *qdict);
void hmp_info_spice(Monitor *mon, const QDict *qdict);
void hmp_info_balloon(Monitor *mon, const QDict *qdict);
void hmp_info_irq(Monitor *mon, const QDict *qdict);
void hmp_info_pic(Monitor *mon, const QDict *qdict);
void hmp_info_pci(Monitor *mon, const QDict *qdict);
void hmp_info_block_jobs(Monitor *mon, const QDict *qdict);
void hmp_info_tpm(Monitor *mon, const QDict *qdict);
void hmp_info_iothreads(Monitor *mon, const QDict *qdict);
void hmp_quit(Monitor *mon, const QDict *qdict);
void hmp_stop(Monitor *mon, const QDict *qdict);
void hmp_system_reset(Monitor *mon, const QDict *qdict);
void hmp_system_powerdown(Monitor *mon, const QDict *qdict);
void hmp_cpu(Monitor *mon, const QDict *qdict);
void hmp_memsave(Monitor *mon, const QDict *qdict);
void hmp_pmemsave(Monitor *mon, const QDict *qdict);
void hmp_ringbuf_write(Monitor *mon, const QDict *qdict);
void hmp_ringbuf_read(Monitor *mon, const QDict *qdict);
void hmp_cont(Monitor *mon, const QDict *qdict);
void hmp_system_wakeup(Monitor *mon, const QDict *qdict);
void hmp_nmi(Monitor *mon, const QDict *qdict);
void hmp_set_link(Monitor *mon, const QDict *qdict);
void hmp_block_passwd(Monitor *mon, const QDict *qdict);
void hmp_balloon(Monitor *mon, const QDict *qdict);
void hmp_block_resize(Monitor *mon, const QDict *qdict);
void hmp_snapshot_blkdev(Monitor *mon, const QDict *qdict);
void hmp_snapshot_blkdev_internal(Monitor *mon, const QDict *qdict);
void hmp_snapshot_delete_blkdev_internal(Monitor *mon, const QDict *qdict);
void hmp_drive_mirror(Monitor *mon, const QDict *qdict);
void hmp_drive_backup(Monitor *mon, const QDict *qdict);
void hmp_loadvm(Monitor *mon, const QDict *qdict);
void hmp_savevm(Monitor *mon, const QDict *qdict);
void hmp_delvm(Monitor *mon, const QDict *qdict);
void hmp_info_snapshots(Monitor *mon, const QDict *qdict);
void hmp_migrate_cancel(Monitor *mon, const QDict *qdict);
void hmp_migrate_incoming(Monitor *mon, const QDict *qdict);
void hmp_migrate_set_downtime(Monitor *mon, const QDict *qdict);
void hmp_migrate_set_speed(Monitor *mon, const QDict *qdict);
void hmp_migrate_set_capability(Monitor *mon, const QDict *qdict);
void hmp_migrate_set_parameter(Monitor *mon, const QDict *qdict);
void hmp_migrate_set_cache_size(Monitor *mon, const QDict *qdict);
void hmp_client_migrate_info(Monitor *mon, const QDict *qdict);
void hmp_migrate_start_postcopy(Monitor *mon, const QDict *qdict);
void hmp_x_colo_lost_heartbeat(Monitor *mon, const QDict *qdict);
void hmp_set_password(Monitor *mon, const QDict *qdict);
void hmp_expire_password(Monitor *mon, const QDict *qdict);
void hmp_eject(Monitor *mon, const QDict *qdict);
void hmp_change(Monitor *mon, const QDict *qdict);
void hmp_block_set_io_throttle(Monitor *mon, const QDict *qdict);
void hmp_block_stream(Monitor *mon, const QDict *qdict);
void hmp_block_job_set_speed(Monitor *mon, const QDict *qdict);
void hmp_block_job_cancel(Monitor *mon, const QDict *qdict);
void hmp_block_job_pause(Monitor *mon, const QDict *qdict);
void hmp_block_job_resume(Monitor *mon, const QDict *qdict);
void hmp_block_job_complete(Monitor *mon, const QDict *qdict);
void hmp_migrate(Monitor *mon, const QDict *qdict);
void hmp_device_add(Monitor *mon, const QDict *qdict);
void hmp_device_del(Monitor *mon, const QDict *qdict);
void hmp_dump_guest_memory(Monitor *mon, const QDict *qdict);
void hmp_netdev_add(Monitor *mon, const QDict *qdict);
void hmp_netdev_del(Monitor *mon, const QDict *qdict);
void hmp_getfd(Monitor *mon, const QDict *qdict);
void hmp_closefd(Monitor *mon, const QDict *qdict);
void hmp_sendkey(Monitor *mon, const QDict *qdict);
void hmp_screendump(Monitor *mon, const QDict *qdict);
void hmp_nbd_server_start(Monitor *mon, const QDict *qdict);
void hmp_nbd_server_add(Monitor *mon, const QDict *qdict);
void hmp_nbd_server_stop(Monitor *mon, const QDict *qdict);
void hmp_chardev_add(Monitor *mon, const QDict *qdict);
void hmp_chardev_change(Monitor *mon, const QDict *qdict);
void hmp_chardev_remove(Monitor *mon, const QDict *qdict);
void hmp_chardev_send_break(Monitor *mon, const QDict *qdict);
void hmp_qemu_io(Monitor *mon, const QDict *qdict);
void hmp_cpu_add(Monitor *mon, const QDict *qdict);
void hmp_quantum_pause(Monitor *mon, const QDict *qdict);
void hmp_quantum_set(Monitor *mon, const QDict *qdict);
void hmp_quantum_get(Monitor *mon, const QDict *qdict);
void hmp_cpu_dbg(Monitor *mon,  const QDict *qdict);
void hmp_cpu_zero_all(Monitor *mon,  const QDict *qdict);
#ifdef CONFIG_FLEXUS
void hmp_flexus_setDebug(Monitor *mon, const QDict *qdict);
void hmp_flexus_setStatInterval(Monitor *mon, const QDict *qdict);
void hmp_flexus_setProfileInterval(Monitor *mon, const QDict *qdict);
void hmp_flexus_setRegionInterval(Monitor *mon, const QDict *qdict);
void hmp_flexus_setTimestampInterval(Monitor *mon, const QDict *qdict);
void hmp_flexus_printCycleCount(Monitor *mon, const QDict *qdict);
void hmp_flexus_setStopCycle(Monitor *mon, const QDict *qdict);
void hmp_flexus_setBreakCPU(Monitor *mon, const QDict *qdict);
void hmp_flexus_setBreakInsn(Monitor *mon, const QDict *qdict);
void hmp_flexus_printProfile(Monitor *mon, const QDict *qdict);
void hmp_flexus_resetProfile(Monitor *mon, const QDict *qdict);
void hmp_flexus_writeProfile(Monitor *mon, const QDict *qdict);
void hmp_flexus_printConfiguration(Monitor *mon, const QDict *qdict);
void hmp_flexus_writeConfiguration(Monitor *mon, const QDict *qdict);
void hmp_flexus_parseConfiguration(Monitor *mon, const QDict *qdict);
void hmp_flexus_setConfiguration(Monitor *mon, const QDict *qdict);
void hmp_flexus_printMeasurement(Monitor *mon, const QDict *qdict);
void hmp_flexus_listMeasurements(Monitor *mon, const QDict *qdict);
void hmp_flexus_writeMeasurement(Monitor *mon, const QDict *qdict);
void hmp_flexus_enterFastMode(Monitor *mon, const QDict *qdict);
void hmp_flexus_leaveFastMode(Monitor *mon, const QDict *qdict);
void hmp_flexus_backupStats(Monitor *mon, const QDict *qdict);
void hmp_flexus_saveStats(Monitor *mon, const QDict *qdict);
void hmp_flexus_reloadDebugCfg(Monitor *mon, const QDict *qdict);
void hmp_flexus_addDebugCfg(Monitor *mon, const QDict *qdict);
void hmp_flexus_enableCategory(Monitor *mon, const QDict *qdict);
void hmp_flexus_disableCategory(Monitor *mon, const QDict *qdict);
void hmp_flexus_listCategories(Monitor *mon, const QDict *qdict);
void hmp_flexus_enableComponent(Monitor *mon, const QDict *qdict);
void hmp_flexus_disableComponent(Monitor *mon, const QDict *qdict);
void hmp_flexus_listComponents(Monitor *mon, const QDict *qdict);
void hmp_flexus_printDebugConfiguration(Monitor *mon, const QDict *qdict);
void hmp_flexus_writeDebugConfiguration(Monitor *mon, const QDict *qdict);
void hmp_flexus_log(Monitor *mon, const QDict *qdict);
void hmp_flexus_printMMU(Monitor *mon, const QDict *qdict);
#endif


void hmp_object_add(Monitor *mon, const QDict *qdict);
void hmp_object_del(Monitor *mon, const QDict *qdict);
void hmp_info_memdev(Monitor *mon, const QDict *qdict);
void hmp_info_memory_devices(Monitor *mon, const QDict *qdict);
void hmp_qom_list(Monitor *mon, const QDict *qdict);
void hmp_qom_set(Monitor *mon, const QDict *qdict);
void object_add_completion(ReadLineState *rs, int nb_args, const char *str);
void object_del_completion(ReadLineState *rs, int nb_args, const char *str);
void device_add_completion(ReadLineState *rs, int nb_args, const char *str);
void device_del_completion(ReadLineState *rs, int nb_args, const char *str);
void sendkey_completion(ReadLineState *rs, int nb_args, const char *str);
void chardev_remove_completion(ReadLineState *rs, int nb_args, const char *str);
void chardev_add_completion(ReadLineState *rs, int nb_args, const char *str);
void set_link_completion(ReadLineState *rs, int nb_args, const char *str);
void netdev_add_completion(ReadLineState *rs, int nb_args, const char *str);
void netdev_del_completion(ReadLineState *rs, int nb_args, const char *str);
void ringbuf_write_completion(ReadLineState *rs, int nb_args, const char *str);
void info_trace_events_completion(ReadLineState *rs, int nb_args, const char *str);
void trace_event_completion(ReadLineState *rs, int nb_args, const char *str);
void watchdog_action_completion(ReadLineState *rs, int nb_args,
                                const char *str);
void migrate_set_capability_completion(ReadLineState *rs, int nb_args,
                                       const char *str);
void migrate_set_parameter_completion(ReadLineState *rs, int nb_args,
                                      const char *str);
void host_net_add_completion(ReadLineState *rs, int nb_args, const char *str);
void host_net_remove_completion(ReadLineState *rs, int nb_args,
                                const char *str);
void delvm_completion(ReadLineState *rs, int nb_args, const char *str);
void loadvm_completion(ReadLineState *rs, int nb_args, const char *str);
void hmp_rocker(Monitor *mon, const QDict *qdict);
void hmp_rocker_ports(Monitor *mon, const QDict *qdict);
void hmp_rocker_of_dpa_flows(Monitor *mon, const QDict *qdict);
void hmp_rocker_of_dpa_groups(Monitor *mon, const QDict *qdict);
void hmp_info_dump(Monitor *mon, const QDict *qdict);
void hmp_info_ramblock(Monitor *mon, const QDict *qdict);
void hmp_hotpluggable_cpus(Monitor *mon, const QDict *qdict);
void hmp_info_vm_generation_id(Monitor *mon, const QDict *qdict);
void hmp_info_memory_size_summary(Monitor *mon, const QDict *qdict);

#if defined(CONFIG_FLEXUS)
void hmp_profile_start(Monitor *mon, const QDict *qdict);
void hmp_profile_stop(Monitor *mon, const QDict *qdict);
#endif

#endif
