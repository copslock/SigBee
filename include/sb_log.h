/*
 * sb_log.h
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

#ifndef SB_LOG_H_
#define SB_LOG_H_

#include <tmc/spin.h>
#include <tmc/queue.h>

#include <pthread.h>


extern pthread_key_t g_thread_key;
sb_s32 init_log (sb_u32 log_level);
sb_s32 init_log_thread (sb_s8 * thread_name );
void write_log (sb_u32 error_num_public, sb_u32 error_num_private, sb_s8 * fmt, ...);
#define ADD_LOG(loglevel) if (g_adv_config.log_level >= loglevel) write_log

#endif /* SB_LOG_H_ */
