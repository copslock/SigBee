/*
 * sig_user.c
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

sp_user_info_link_t *sp_users[MAX_SP_USER_INFO_HASH_NUM];
sp_tlli_link_t *sp_tlli_users[MAX_SP_TLLI_HASH_NUM];

//分配MAX_SP_USER_INFO_HASH_NUM个用户链结构
sb_s32 sp_init_user_mem()
{
	sb_s32 ret = RET_SUCC;
	sb_s32 i = 0;
	sp_user_info_link_t *user_nodelist = NULL;

	for(i = 0; i < MAX_SP_USER_INFO_HASH_NUM; i++)
	{

		user_nodelist = tmc_cmem_malloc(sizeof(sp_user_info_link_t));
		if(unlikely(NULL == user_nodelist))
		{
			ADD_LOG(LOG_LEVEL_FAILURE)(0, 0, "Failure to allocate user_nodelist struct");
			return RET_FAIL;
		}
		user_nodelist->size = 0;
		user_nodelist->user_info_head = NULL;
		tmc_sync_mutex_init(&(user_nodelist->user_link_sync_lock));
		sp_users[i] = user_nodelist;
	}

	ADD_LOG(LOG_LEVEL_DEBUG)(0, 0, "%s: succed", __FUNCTION__);

	return ret;
}

void sp_destroy_user_mem()
{
	sb_s32 i = 0, j = 0;
	sp_user_info_link_t *user_nodelist = NULL;

	for(i = 0; i < MAX_SP_USER_INFO_HASH_NUM; i++)
	{

		user_nodelist = sp_users[i];
		if(user_nodelist)
		{
			sb_s32 j = 0;
			sp_user_info_node_t *user_mem = user_nodelist->user_info_head;
			sp_user_info_node_t *user_mem_next = NULL;
			while(user_mem)
			{
				user_mem_next = user_mem->next;
				for(j = 0; j < MAX_XDR_CUR_NUM; j++)
				{
					if(user_mem->xdr[j])tmc_cmem_free(user_mem->xdr[j]);
					user_mem->xdr[j] = NULL;
				}
				tmc_cmem_free(user_mem);
				user_mem = user_mem_next;
			}
			memset(user_nodelist, 0, sizeof(sp_user_info_link_t));
			tmc_cmem_free(user_nodelist);
		}

		sp_users[i] = NULL;
	}

	return ;
}

//分配MAX_SP_TLLI_HASH_NUM个用户链结构
sb_s32 sp_init_tlli_mem()
{
	sb_s32 ret = RET_SUCC;
	sb_s32 i = 0;
	sp_tlli_link_t *tlli_nodelist = NULL;

	for(i = 0; i < MAX_SP_TLLI_HASH_NUM; i++)
	{

		tlli_nodelist = tmc_cmem_malloc(sizeof(sp_tlli_link_t));
		if(unlikely(NULL == tlli_nodelist))
		{
			ADD_LOG(LOG_LEVEL_FAILURE)(0, 0, "Failure to allocate tlli_nodelist struct");
			return RET_FAIL;
		}
		tlli_nodelist->size = 0;
		tlli_nodelist->user_info_head = NULL;
		tmc_sync_mutex_init(&(tlli_nodelist->tlli_link_sync_lock));
		sp_tlli_users[i] = tlli_nodelist;
	}

	ADD_LOG(LOG_LEVEL_DEBUG)(0, 0, "%s: succed", __FUNCTION__);

	return ret;
}

void sp_destroy_tlli_mem()
{
	sb_s32 i = 0;
	sp_tlli_link_t *tlli_nodelist = NULL;

	for(i = 0; i < MAX_SP_TLLI_HASH_NUM; i++)
	{

		tlli_nodelist = sp_tlli_users[i];
		if(tlli_nodelist)
		{
			sb_s32 j = 0;
			sp_tlli_node_t *user_mem = tlli_nodelist->user_info_head;
			sp_tlli_node_t *user_mem_next = NULL;
			while(user_mem)
			{
				user_mem_next = user_mem->next;
				tmc_cmem_free(user_mem);
				user_mem = user_mem_next;
			}
			memset(tlli_nodelist, 0, sizeof(sp_tlli_link_t));
			tmc_cmem_free(tlli_nodelist);
		}

		sp_tlli_users[i] = NULL;
	}

	return ;
}

//分配用户ID，每个核一个分配范围，0~0xFFFFFFFFFFFFFF（每个核7个字节大小的空间）
sb_u64 sp_malloc_uid()
{
	sb_u64 uid = 0;

	uid = g_sp_current_uid + g_sp_uid_offset;
	g_sp_current_uid++;
	g_sp_current_uid %= 0x100000000000000;

	return uid;
}

/*!
  \fn sb_s32 add_to_ui_queue( sp_user_info_t * ui  )
  \brief 将用户信息加入相应的队列中
  \param[in] ui 待发送的用户信息
  \param[out]
  \return sb_s32
  \retval RET_SUCC 成功
  \retval RET_FAIL 失败
  */

/**********************************************************************
  函数说明：将用户信息加入相应的队列中

  输入参数：ui 待发送的用户信息

 全局变量：

  函数返回：RET_SUCC：成功，RET_FAIL：失败

  修改记录：无

 **********************************************************************/
sb_s32 add_to_ui_queue (sp_user_info_t * ui)
{
  sb_u64 push_index = 0, pop_index = 0;
  sb_u64 start_cycle = 0;
  sb_u64 end_cycle = 0;
  sb_u64 now_cycle;

	if(NULL == g_sp_user_queue_info_p)
	{
		SB_ADD_LOG(LOG_LEVEL_DEBUG)
				(SB_LOG_ALERT,0, "g_sp_user_queue_info_p is NULL!");

		return RET_FAIL;
	}
	sb_u16 queue_index = ui->user_id % g_sp_user_queue_info_p->num;
	sp_user_info_queue_t* ui_queue_p = g_sp_user_queue_info_p->queue_array[queue_index];

	push_index = ui_queue_p->push_index;
	pop_index = ui_queue_p->pop_index;

	if(push_index-pop_index >= ((sb_u64)ui_queue_p->size))			//队列满了
	{
		SB_ADD_LOG(LOG_LEVEL_DEBUG)
				(SB_LOG_ALERT,0, "  queue[%d] is full, push_index is %llu pop_index is %llu queue size is %u !",
						queue_index, ui_queue_p->push_index, ui_queue_p->pop_index, ui_queue_p->size);
		g_sp_statistic->ui_fwd_fail_count++;

		return RET_FAIL;
	}
	start_cycle = get_cycle_count();
	g_sp_statistic->ui_fwd_succ_count++;

	sp_user_info_t *ui_p = &ui_queue_p->user_infos[ui_queue_p->tail];		//获得需要传入的指针
	memcpy(ui_p, ui, sizeof(sp_user_info_t));
	ui_queue_p->tail = (ui_queue_p->tail+1)% ui_queue_p->size;

	__insn_mf();

	ui_queue_p->push_index++;

	end_cycle = get_cycle_count();
	now_cycle = end_cycle - start_cycle;

	return RET_SUCC;
}
