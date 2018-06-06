/*
 * sb_logd.h
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

#ifndef SB_LOGD_H_
#define SB_LOGD_H_

#include <stdint.h>
#include <time.h>

#include <tmc/task.h>
#include <tmc/cpus.h>
#include <tmc/spin.h>
#include <tmc/queue.h>

#include "sb_qlog.h"

extern void sb_write_log();
extern void sb_open_and_sizecheck(log_msg_t *object, const sb_s8 *oldname, const sb_s8 *newname);

#endif /* SB_LOGD_H_ */
