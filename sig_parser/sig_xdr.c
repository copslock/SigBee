/*
 * sig_xdr.c
 *
 *  Created on: 2014年6月29日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <arpa/inet.h>

//tilera 相关头文件
#include <tmc/cmem.h>
#include <tmc/mem.h>
#include <tmc/sync.h>
#include <tmc/cpus.h>
#include <tmc/spin.h>
#include <tmc/task.h>
#include <gxio/mpipe.h>
#include <gxio/mica.h>
#include <gxcr/gxcr.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_xdr_struct.h"
#include "sb_error_code.h"
#include "sb_base_file.h"
#include "sb_base_util.h"
#include "sb_shared_queue.h"
#include "sb_base_mica.h"
#include "sb_base_mpipe.h"
#include "sb_public_var.h"
#include "sb_base_mem.h"
#include "sb_base_process.h"
#include "sb_base_pack.h"
#include "sb_qlog.h"
#include "sb_sp.h"

sb_u32 xdr_send_queue_no = 0;

//分配XDR ID，每个核一个分配范围，0~0xFFFFFFFFFFFF（每个核6字节大小的空间），最高字节是设备ID，第二个字节是核ID
sb_u64 sp_malloc_xdrid()
{
	sb_u64 xdrid = 0;

	xdrid = g_sp_current_xdrid + g_sp_xdrid_offset;
	g_sp_current_xdrid++;
	g_sp_current_xdrid %= 0x1000000000000;

	return xdrid;
}

/*!
  \fn sb_s32 add_to_xdr_queue( sp_xdr_t * xdr  )
  \brief 将XDR加入相应的发送队列中
  \param[in] xdr 待发送的XDR
  \param[out]
  \return sb_s32
  \retval RET_SUCC 成功
  \retval RET_FAIL 失败
  */

/**********************************************************************
  函数说明：将XDR加入相应的发送队列中

  输入参数：xdr 待发送的XDR

 全局变量：

  函数返回：RET_SUCC：成功，RET_FAIL：失败

  修改记录：无

 **********************************************************************/
sb_s32 add_to_xdr_queue (sp_xdr_t * xdr)
{
  sb_u64 push_index = 0, pop_index = 0;
  sb_u64 start_cycle = 0;
  sb_u64 end_cycle = 0;
  sb_u64 now_cycle;

	if(NULL == g_sp_xdr_queue_info_p)
	{
		SB_ADD_LOG(LOG_LEVEL_DEBUG)
				(SB_LOG_ALERT,0, "g_sp_xdr_queue_info_p is NULL!");

		return RET_FAIL;
	}
	sb_u16 queue_index = xdr_send_queue_no % g_sp_xdr_queue_info_p->num;
	xdr_send_queue_no++;
	sp_xdr_queue_t* xdr_queue_p = g_sp_xdr_queue_info_p->queue_array[queue_index];

	push_index = xdr_queue_p->push_index;
	pop_index = xdr_queue_p->pop_index;

	if(push_index-pop_index >= ((sb_u64)xdr_queue_p->size))			//队列满了
	{
		SB_ADD_LOG(LOG_LEVEL_DEBUG)
				(SB_LOG_ALERT,0, "  queue[%d] is full, push_index is %llu pop_index is %llu queue size is %u !",
						queue_index, xdr_queue_p->push_index, xdr_queue_p->pop_index, xdr_queue_p->size);
		g_sp_statistic->xdr_fwd_succ_count++;

		return RET_FAIL;
	}
	start_cycle = get_cycle_count();
	g_sp_statistic->xdr_fwd_succ_count++;

	sp_xdr_t *xdr_p = &xdr_queue_p->lb_xdrs[xdr_queue_p->tail];		//获得需要传入的指针
	memcpy(xdr_p, xdr, sizeof(sp_xdr_t));
	xdr_queue_p->tail = (xdr_queue_p->tail+1)% xdr_queue_p->size;

	__insn_mf();

	xdr_queue_p->push_index++;

	end_cycle = get_cycle_count();
	now_cycle = end_cycle - start_cycle;

	return RET_SUCC;
}
