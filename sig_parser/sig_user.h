/*
 * sig_user.h
 *
 *  Created on: 2014年6月29日
 *      Author: Figoo
 */

#ifndef SIG_USER_H_
#define SIG_USER_H_

extern sb_u64	g_sp_current_uid;	//!< 当前分配的用户ID，需要加上下面的偏移量
extern sb_u64 g_sp_uid_offset;

sb_u64 sp_malloc_uid();
sb_s32 add_to_ui_queue (sp_user_info_t * ui);

#endif /* SIG_USER_H_ */
