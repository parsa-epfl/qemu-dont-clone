#include "qflex/fa-qflex-api.h"

bool fa_qflex_user_mode = false;
bool fa_qflex_running = false;
bool qflex_inst_done = false;
void qflex_api_values_init(void) {
    fa_qflex_user_mode = false;
    fa_qflex_running = false;
    qflex_inst_done = false;
}
