/*
 * sig_xdr.h
 *
 *  Created on: 2014年6月29日
 *      Author: Figoo
 */

#ifndef SIG_XDR_H_
#define SIG_XDR_H_


extern sb_u64	g_sp_current_xdrid;	//!< 当前分配的XDR ID，需要加上下面的偏移量
extern sb_u64 g_sp_xdrid_offset;

sb_u64 sp_malloc_xdrid();
sb_s32 add_to_xdr_queue (sp_xdr_t * xdr);

#endif /* SIG_XDR_H_ */
