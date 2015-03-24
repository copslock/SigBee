/*
 * sb_sp.h
 *
 *  Created on: 2014年6月15日
 *      Author: Figoo
 */

#ifndef SB_SP_H_
#define SB_SP_H_

#include "sb_type.h"
#include "sb_defines.h"

#define MAX_IP_FRAG_HASH_NUM	1024

// ip fragment struct
// hash table and linked by link list
// 只处理两个分片的情况，多余两个分片不处理，直接丢弃该包的所有分片包
// 先到的分片放入缓存中，等另外一个分片到达后一起重组

typedef struct ip_frag_tag
{
	sb_u32		sip;
	sb_u32		dip;
	sb_u16		identification;
	sb_u16		ip_head_offset;
	gxio_mpipe_iqueue_t *iqueue;
	gxio_mpipe_idesc_t idesc;
	struct ip_frag_tag *next;
	struct ip_frag_tag *pre;
}ip_frag_t;

typedef struct ip_frag_mem_ctl_tag
{
	sb_s32  size;
	ip_frag_t *prim_head;
	sb_s32  used_nr;
	ip_frag_t  *used_head;
	ip_frag_t  *used_tail;
	sb_s32 free_nr;
	ip_frag_t  *free_head;
	ip_frag_t  *free_tail;

	tmc_sync_mutex_t frag_sync_lock;
}ip_frag_mem_ctl_t;

//用户表
#define MAX_SP_USER_INFO_HASH_NUM	500000
#define MAX_SP_USER_INFO_NODE_NUM	256
#define MAX_XDR_CUR_NUM	4
typedef struct sp_user_info_node_tag
{
	sp_user_info_t user_info;
	sb_u8 xdr_type[MAX_XDR_CUR_NUM];
	void *xdr[MAX_XDR_CUR_NUM];
	struct sp_user_info_node_tag *next;
	struct sp_user_info_node_tag *pre;

	sb_u32 time_s;
	sb_u32 time_ns;

	tmc_sync_mutex_t user_node_sync_lock;
}sp_user_info_node_t;

typedef struct sp_user_info_link_tag
{
	sb_s32  size;
	sp_user_info_node_t *user_info_head;

	tmc_sync_mutex_t user_link_sync_lock;
}sp_user_info_link_t;

//tlli----uid对应表
#define MAX_SP_TLLI_HASH_NUM	500000
#define MAX_SP_TLLI_NODE_NUM	256
typedef struct sp_tlli_node_tag
{
	sb_u32 tlli;
	sp_user_info_node_t *user_info;
	struct sp_tlli_node_tag *next;
	struct sp_tlli_node_tag *pre;

	sb_u32 time_s;
	sb_u32 time_ns;
}sp_tlli_node_t;

typedef struct sp_tlli_link_tag
{
	sb_s32  size;
	sp_tlli_node_t *user_info_head;

	tmc_sync_mutex_t tlli_link_sync_lock;
}sp_tlli_link_t;

extern ip_frag_mem_ctl_t *sp_ipfrag_s[MAX_IP_FRAG_HASH_NUM];
extern sp_user_info_link_t *sp_users[MAX_SP_USER_INFO_HASH_NUM];
extern sp_tlli_link_t *sp_tlli_users[MAX_SP_TLLI_HASH_NUM];

void sp_destroy_frag_mem();
sb_s32 sp_init_frage_mem();

void sp_destroy_user_mem();
sb_s32 sp_init_user_mem();

void sp_destroy_tlli_mem();
sb_s32 sp_init_tlli_mem();

#endif /* SB_SP_H_ */
