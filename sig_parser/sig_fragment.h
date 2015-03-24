/*
 * sig_fragment.h
 *
 *  Created on: 2014年6月15日
 *      Author: Figoo
 */

#ifndef SIG_FRAGMENT_H_
#define SIG_FRAGMENT_H_


#define MAX_IP_FRAG_NODE_NUM	256

#define FRAG_TIMEOUT	10	//超时时间 10s

typedef enum
{
	FRAG_FLAG_NO = 0,
	FRAG_FLAG_YES,
	FRAG_FLAG_PROCESS
}frag_flag_e;


extern sb_u32 g_sp_frag_timeout_s;//IP分片老化区间,s——>e
extern sb_u32 g_sp_frag_timeout_e;

sb_u32 dual_ip_id_hash(sb_u32 s_ip, sb_u32 d_ip, sb_u16 id);
ip_frag_t *get_from_frag_pool(ip_frag_mem_ctl_t *ip_frag_nodelist);
sb_s32 put_to_frag_pool(ip_frag_t *ip_fragnode, ip_frag_mem_ctl_t *ip_frag_nodelist);
ip_frag_t *find_node_from_frag_pool(ip_frag_mem_ctl_t *ip_frag_nodelist, sb_u32 sip, sb_u32 dip, sb_u16 id);
void timeout_node_from_frag_pool();

#endif /* SIG_FRAGMENT_H_ */
