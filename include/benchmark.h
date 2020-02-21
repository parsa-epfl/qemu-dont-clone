#ifndef __BENCH__
#define __BENCH__
#include <time.h>
#include "io/channel-command.h"

extern QIOChannel *input_channel;
extern double time_in_channel;

static inline double get_result(struct timespec begin, struct timespec end) {
        long nsec_time = (end.tv_nsec - begin.tv_nsec);
        long sec_time = (end.tv_sec - begin.tv_sec);

        return (double) ((double) sec_time + ((double) nsec_time / 1000000000)) ;
}

#endif
