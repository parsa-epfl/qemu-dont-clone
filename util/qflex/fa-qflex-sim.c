#include "qflex/qflex.h"
#include "qflex/fa-qflex.h"
#include "qflex/fa-qflex-sim.h"
#include "qflex/json.h"

FA_QFlexSimConfig_t sim_cfg = {
    "SIM_STATE", "SIM_LOCK", "SIM_CMD",
    "QEMU_STATE", "QEMU_LOCK", "QEMU_CMD",
    "PROGRAM_PAGE", 0,
    FA_QFLEX_ROOT_DIR,
    SIMULATOR_ROOTDIR
};

const FA_QFlexCmd_t cmds[FA_QFLEXCMDS_NR] = {
    {DATA_LOAD,  0, "DATA_LOAD" },
    {DATA_STORE, 0, "DATA_STORE"},
    {INST_FETCH, 0, "INST_FETCH"},
    {INST_UNDEF, 0, "INST_UNDEF"},
    {INST_EXCP,  0, "INST_EXCP" },
    {SIM_START,  0, "SIM_START" },
    {SIM_STOP,   0, "SIM_STOP"  },
    {LOCK_WAIT,  0, "LOCK_WAIT" },
    {CHECK_N_STEP, 0, "CHECK_N_STEP"}
};

void* fa_qflex_start_sim(void *arg) {
    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        exit(-1);
    } else if (pid == 0) {
        //* child code
        size_t max_size = 500;
        FA_QFlexSimConfig_t *cfg = (FA_QFlexSimConfig_t *)arg;
        char page_size[65];
        char buffer[max_size];
        snprintf(page_size, 64+1, "%0lu", cfg->page_size);
        assert(chdir(cfg->simPath) == 0);
        snprintf(buffer, max_size,
                 "/usr/bin/sbt 'test:runMain protoflex.SimulatorMain %s %s %s %s %s %s %s %s %s'",
                 cfg->sim_state, cfg->sim_lock, cfg->sim_cmd,
                 cfg->qemu_state, cfg->qemu_lock, cfg->qemu_cmd,
                 cfg->program_page, page_size,
                 cfg->rootPath);
        execl("/usr/bin/xterm", "xterm", "-e", buffer, (char *) NULL);
        // */
        return NULL;
    } else {
        return NULL;  // parent code
    }
}


FA_QFlexCmd_t* fa_qflex_loadfile_json2cmd(const char* filename) {
    char *json;
    size_t size;
    FA_QFlexCmd_t* cmd = malloc(sizeof(FA_QFlexCmd_t));
    json = fa_qflex_read_file(filename, &size);
    json_value_s* root = json_parse(json, size);
    json_object_s* objects = root->payload;
    json_object_element_s* curr = objects->start;
    do {
        if(!strcmp(curr->name->string, "addr")) {
            assert(curr->value->type == json_type_string);
            json_string_s* number = curr->value->payload;
            cmd->addr = strtol(number->string, NULL, 16);
        } else if(!strcmp(curr->name->string, "cmd")) {
            assert(curr->value->type == json_type_string);
            json_number_s* number = curr->value->payload;
            cmd->cmd = strtol(number->number, NULL, 10);
        }
        curr = curr->next;
    } while(curr);
    free(json);
    cmd->str = cmds[cmd->cmd].str;
    return cmd;
}

void fa_qflex_writefile_cmd2json(const char* filename, FA_QFlexCmd_t in_cmd) {
    qflex_log_mask(FA_QFLEX_LOG_CMDS, "QEMU: CMD OUT %s in %s\n", in_cmd.str, filename);
    size_t size;
    char *json;
    json_value_s root;
    json_object_s objects;
    objects.length = 0;
    root.type = json_type_object;
    root.payload = &objects;
    json_object_element_s* head;


    //* cmd packing
    json_string_s cmd;

    char cmd_num[3];
    snprintf(cmd_num, 3,"%0"XSTR(2)"d", in_cmd.cmd);
    cmd.string = cmd_num;
    cmd.string_size = 2;

    json_value_s cmd_val = {.payload = &cmd, .type = json_type_string};
    json_string_s cmd_name = {.string = "cmd", .string_size = strlen("cmd")};
    json_object_element_s cmd_obj = {.value = &cmd_val, .name = &cmd_name, .next = NULL};
    objects.start = &cmd_obj;
    head = &cmd_obj;
    objects.length++;
    // */

    //* addr flags packing
    json_string_s addr;

    char addr_num[ULONG_HEX_MAX + 1];
    snprintf(addr_num, ULONG_HEX_MAX+1, "%0"XSTR(ULONG_HEX_MAX)"lx", in_cmd.addr);
    addr.string = addr_num;
    addr.string_size = ULONG_HEX_MAX;

    json_value_s addr_val = {.payload = &addr, .type = json_type_string};
    json_string_s addr_name = {.string = "addr", .string_size = strlen("addr")};
    json_object_element_s addr_obj = {.value = &addr_val, .name = &addr_name, .next = NULL};
    head->next = &addr_obj;
    head = &addr_obj;
    objects.length++;
    // */

    json = json_write_minified(&root, &size);
    fa_qflex_write_file(filename, json, size-1);
    free(json);
}

void fa_qflex_fileread_json2cpu(CPUState *cpu, const char* filename) {
    size_t size;
    char* json = fa_qflex_read_file(filename, &size);
    fa_qflex_json2cpu(cpu, json, size);
    free(json);
}

void fa_qflex_filewrite_cpu2json(CPUState *cpu, const char* filename) {
    size_t size;
    char *buffer = fa_qflex_cpu2json(cpu, &size);
    fa_qflex_write_file(filename, buffer, size - 1); // Get rid of last char:'\0'
    free(buffer);
}

FA_QFlexCmd_t* fa_qflex_cmd2json_lock_wait(const char *filename) {    
    qflex_log_mask(FA_QFLEX_LOG_CMDS, "QEMU: CMD OUT %s in %s\n", cmds[LOCK_WAIT].str, filename);
    FA_QFlexCmd_t* cmd;
    do {
        sleep(3);
        cmd = fa_qflex_loadfile_json2cmd(filename);
    } while (cmd->cmd == LOCK_WAIT);
    qflex_log_mask(FA_QFLEX_LOG_CMDS, "QEMU: CMD IN %s\n", cmd->str);
    fa_qflex_writefile_cmd2json(filename, cmds[LOCK_WAIT]);
    return cmd;
}
