/*
 * sb_qlog.h
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

#ifndef SB_QLOG_H_
#define SB_QLOG_H_

#include "sb_type.h"
#include "sb_log.h"
#include "sb_defines.h"

#include <time.h>
#include <stdio.h>


/* Log message type. */
typedef enum {
    SB_LOG_INFO,
    SB_LOG_ALERT,
    SB_LOG_ERR
}log_msg_type_t;

typedef struct {
    log_msg_type_t msg_type;
    sb_s8 pname[MAX_PROCESS_NAME_SIZE];
    sb_u32 cpuid;
    time_t time;
    sb_s8 content[MAX_LOG_CONENT_LEN];
} log_msg_t;

#define LG2_CAPACITY (10)

#if 0
TMC_QUEUE(log_queue,log_msg_t, LG2_CAPACITY, \
          (TMC_QUEUE_SINGLE_SENDER | TMC_QUEUE_SINGLE_RECEIVER));
#endif

TMC_QUEUE(log_queue,log_msg_t, LG2_CAPACITY, \
          (TMC_QUEUE_SINGLE_RECEIVER));

extern log_queue_t *g_log_queue;

void write_qlog (sb_u32 error_num_public, sb_u32 error_num_private, sb_s8 * fmt, ...);
#define ADD_LOG_QUEUE(loglevel) if (g_adv_config.log_level >= loglevel) write_qlog
#ifndef SB_QLOG
#define SB_ADD_LOG  ADD_LOG
#else
#define SB_ADD_LOG ADD_LOG_QUEUE
#endif

sb_s32 nfprintf (FILE *stream, const char *format, ...);
#ifndef SB_NPRINT
#define SB_FPRINT fprintf
#else
#define SB_FPRINT nfprintf
#endif

#endif /* SB_QLOG_H_ */
