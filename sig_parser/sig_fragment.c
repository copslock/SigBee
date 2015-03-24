/*
 * sig_fragment.c
 *
 *  Created on: 2014年6月15日
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
#include "sb_error_code.h"
#include "sb_qlog.h"
#include "sb_sp.h"
#include "sig_fragment.h"

ip_frag_mem_ctl_t *sp_ipfrag_s[MAX_IP_FRAG_HASH_NUM];

 sb_u32 dual_ip_id_hash(sb_u32 s_ip, sb_u32 d_ip, sb_u16 id)
{
	sb_u32 ret = 0;

	ret = __insn_crc32_32(s_ip, d_ip);
	ret = __insn_crc32_32(ret, id) % MAX_IP_FRAG_HASH_NUM;

	return ret;
}

//分配MAX_IP_FRAG_HASH_NUM * MAX_IP_FRAG_NODE_NUM个分片包结构
sb_s32 sp_init_frage_mem()
{
	sb_s32 ret = RET_SUCC;
	sb_s32 i = 0;
	ip_frag_mem_ctl_t *ip_frag_nodelist = NULL;

	for(i = 0; i < MAX_IP_FRAG_HASH_NUM; i++)
	{
		sb_s32 j = 0;

		ip_frag_nodelist = tmc_cmem_malloc(sizeof(ip_frag_mem_ctl_t));
		if(unlikely(NULL == ip_frag_nodelist))
		{
			ADD_LOG(LOG_LEVEL_FAILURE)(0, 0, "Failure to allocate ip_frag_nodelist struct");
			return RET_FAIL;
		}
		ip_frag_nodelist->size = MAX_IP_FRAG_NODE_NUM;
		tmc_sync_mutex_init(&(ip_frag_nodelist->frag_sync_lock));

		ip_frag_t *frag_mem =
				(ip_frag_t *)tmc_cmem_malloc(ip_frag_nodelist->size * sizeof(ip_frag_t));
		if(NULL == frag_mem)
		{
			ADD_LOG(LOG_LEVEL_FAILURE)(0, 0, "FAilure to allocate ip_frag_nodelist's  frag memory");
			return RET_FAIL;
		}
		memset(frag_mem, 0, ip_frag_nodelist->size * sizeof(ip_frag_t));
		for(int j=0; j<(ip_frag_nodelist->size-1); j++)
		{
			frag_mem[j].next = &(frag_mem[j+1]);
		}
		for(int j=1; j<ip_frag_nodelist->size; j++)
		{
			frag_mem[j].pre = &(frag_mem[j-1]);
		}
//		frag_mem[(ip_frag_nodelist->size - 1)].next = NULL;
//		frag_mem[0].pre = NULL;

		ip_frag_nodelist->prim_head = frag_mem;
		ip_frag_nodelist->free_head = frag_mem;
		ip_frag_nodelist->free_tail = &(frag_mem[(ip_frag_nodelist->size-1)]);
		ip_frag_nodelist->free_nr = ip_frag_nodelist->size;
		ip_frag_nodelist->used_head = NULL;
		ip_frag_nodelist->used_tail = NULL;
		ip_frag_nodelist->used_nr = 0;

		sp_ipfrag_s[i] = ip_frag_nodelist;
	}

	ADD_LOG(LOG_LEVEL_DEBUG)(0, 0, "%s: succed", __FUNCTION__);

	return ret;
}

void sp_destroy_frag_mem()
{
	sb_s32 i = 0;
	ip_frag_mem_ctl_t *ip_frag_nodelist = NULL;

	for(i = 0; i < MAX_IP_FRAG_HASH_NUM; i++)
	{

		ip_frag_nodelist = sp_ipfrag_s[i];
		if(ip_frag_nodelist)
		{
			sb_s32 j = 0;
			ip_frag_t *frag_mem = ip_frag_nodelist->prim_head;
			memset(frag_mem, 0, ip_frag_nodelist->size * sizeof(ip_frag_t));
			tmc_cmem_free(frag_mem);
			memset(ip_frag_nodelist, 0, sizeof(ip_frag_mem_ctl_t));
			tmc_cmem_free(ip_frag_nodelist);
		}

		sp_ipfrag_s[i] = NULL;
	}

	return ;
}

//从缓存池中取出一个空节点
ip_frag_t *get_from_frag_pool(ip_frag_mem_ctl_t *ip_frag_nodelist)
{
	ip_frag_t *get_frag_p = NULL;

	if(unlikely(NULL == ip_frag_nodelist))
		return NULL;


	tmc_sync_mutex_lock(&(ip_frag_nodelist->frag_sync_lock));
	//保留最后一个不分配出去，性能原因
	if(ip_frag_nodelist->free_nr > 1 && ip_frag_nodelist->free_head != ip_frag_nodelist->free_tail)
	{
		get_frag_p = ip_frag_nodelist->free_head;
		ip_frag_nodelist->free_head = ip_frag_nodelist->free_head->next;
		ip_frag_nodelist->free_head->pre = NULL;
		ip_frag_nodelist->free_nr --;

		get_frag_p->next = ip_frag_nodelist->used_head;
		if(ip_frag_nodelist->used_head)ip_frag_nodelist->used_head->pre = get_frag_p;
		ip_frag_nodelist->used_head = get_frag_p;
		if(NULL == ip_frag_nodelist->used_tail)ip_frag_nodelist->used_tail = get_frag_p;
		ip_frag_nodelist->used_nr ++;
		__insn_mf();

		tmc_sync_mutex_unlock(&(ip_frag_nodelist->frag_sync_lock));
		return get_frag_p;
	}
	tmc_sync_mutex_unlock(&(ip_frag_nodelist->frag_sync_lock));

	return NULL;
}

//将节点放回缓存池
sb_s32 put_to_frag_pool(ip_frag_t *ip_fragnode, ip_frag_mem_ctl_t *ip_frag_nodelist)
{
	sb_s32 ret = RET_SUCC;

	if(unlikely(NULL == ip_frag_nodelist || NULL == ip_fragnode))
		return RET_FAIL;


	tmc_sync_mutex_lock(&(ip_frag_nodelist->frag_sync_lock));

	if(ip_frag_nodelist->used_head == ip_fragnode)ip_frag_nodelist->used_head = ip_fragnode->next;
	else ip_fragnode->pre->next = ip_fragnode->next;
	if(ip_frag_nodelist->used_tail == ip_fragnode)ip_frag_nodelist->used_tail = ip_fragnode->pre;
	else ip_fragnode->next->pre = ip_fragnode->pre;
	if(ip_frag_nodelist->used_nr > 0)ip_frag_nodelist->used_nr --;

	memset(ip_fragnode, 0, sizeof(ip_frag_t));
	ip_fragnode->next = ip_frag_nodelist->free_head;
	ip_frag_nodelist->free_head->pre = ip_fragnode;
	ip_frag_nodelist->free_head = ip_fragnode;
	ip_frag_nodelist->free_nr ++;
	__insn_mf();

	tmc_sync_mutex_unlock(&(ip_frag_nodelist->frag_sync_lock));

	return ret;
}

//查找节点
ip_frag_t *find_node_from_frag_pool(ip_frag_mem_ctl_t *ip_frag_nodelist, sb_u32 sip, sb_u32 dip, sb_u16 id)
{
	ip_frag_t *ip_fragnode = NULL;

	if(unlikely(NULL == ip_frag_nodelist))
		return NULL;


	tmc_sync_mutex_lock(&(ip_frag_nodelist->frag_sync_lock));

	ip_fragnode = ip_frag_nodelist->used_head;
	while(ip_fragnode)
	{
		if(ip_fragnode->sip == sip && ip_fragnode->dip == dip && ip_fragnode->identification == id)
		{
			tmc_sync_mutex_unlock(&(ip_frag_nodelist->frag_sync_lock));
			return ip_fragnode;
		}
	}

	tmc_sync_mutex_unlock(&(ip_frag_nodelist->frag_sync_lock));

	return NULL;
}

#define FRAG_TIMEOUT_LINK_NUM	100		//!< 每次老化100个链
static sb_u32 c_frag_timout_s = g_sp_frag_timeout_s;
//节点老化
void timeout_node_from_frag_pool()
{
	ip_frag_mem_ctl_t *ip_frag_nodelist = NULL;
	ip_frag_t *ip_fragnode = NULL;
	ip_frag_t *ip_fragnode_temp = NULL;
	sb_u32 current_time_s = g_current_time & 0xFFFFFFFF;
	sb_u32 end = c_frag_timout_s + FRAG_TIMEOUT_LINK_NUM;

	for(; c_frag_timout_s < end; c_frag_timout_s++)
	{
		if(c_frag_timout_s >= g_sp_frag_timeout_e)
		{
			c_frag_timout_s = g_sp_frag_timeout_s;
			break;
		}
		ip_frag_nodelist = sp_ipfrag_s[c_frag_timout_s];

		if(ip_frag_nodelist)
		{
			tmc_sync_mutex_lock(&(ip_frag_nodelist->frag_sync_lock));
			ip_fragnode = ip_frag_nodelist->used_head;
			while(ip_fragnode)
			{
				ip_fragnode_temp = ip_fragnode->next;
				if(current_time_s > (ip_fragnode->idesc.time_stamp_sec + FRAG_TIMEOUT))
				{
					gxio_mpipe_iqueue_drop(ip_fragnode->iqueue,ip_fragnode->idesc);

					if(ip_frag_nodelist->used_head == ip_fragnode)ip_frag_nodelist->used_head = ip_fragnode->next;
					else ip_fragnode->pre->next = ip_fragnode->next;
					if(ip_frag_nodelist->used_tail == ip_fragnode)ip_frag_nodelist->used_tail = ip_fragnode->pre;
					else ip_fragnode->next->pre = ip_fragnode->pre;
					if(ip_frag_nodelist->used_nr > 0)ip_frag_nodelist->used_nr --;

					memset(ip_fragnode, 0, sizeof(ip_frag_t));
					ip_fragnode->next = ip_frag_nodelist->free_head;
					ip_frag_nodelist->free_head->pre = ip_fragnode;
					ip_frag_nodelist->free_head = ip_fragnode;
					ip_frag_nodelist->free_nr ++;

				}
				ip_fragnode = ip_fragnode_temp;
			}
			__insn_mf();
			tmc_sync_mutex_unlock(&(ip_frag_nodelist->frag_sync_lock));
		}

	}

	return;
}
