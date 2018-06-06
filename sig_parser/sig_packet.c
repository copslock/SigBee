/*
 * sig_packet.c
 *
 *  Created on: 2014年6月17日
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

#include "sig_parse.h"
#include "sig_fragment.h"
#include "sig_packet.h"

sp_packet_t g_sp_packet;	//!< 数据包信息
sp_lb_t *g_sp_lb_tag;	//!< 负载均衡设备标签
sp_xdr_t g_xdr;
sp_user_info_t g_ui;
sp_packet_info_t g_pack;

// IP header checksum calculation
static inline sb_u16 sb_ip_cal_xsum(sb_u16 *buff, int len, sb_u32 xsum)
{
	// calculate IP checksum for a buffer of bytes
	// len is number of 16-bit values
	while (len--)
		xsum += * buff++;  // accumulate 16-bit sum

	while (xsum >> 16)   // propagate carries for 1's complement
		xsum = (xsum & 0xFFFF) + (xsum >> 16);

	return (sb_u16) xsum;
}

// ???Result is networking byte order.
sb_u16 sb_ip_cal_csum(void *buff, sb_s32 len)
{
	return ~sb_ip_cal_xsum((sb_u16 *)buff, len * 2, 0);
}

// 返回L3层的协议类型
static inline sb_u16
process_l2_layer(sp_packet_t* p_packet_info)
{
	sb_u16 eth_type = PROCESS_PROTO_ERROR;
	sb_u32 offset = 0;

	if (p_packet_info->s_len < sizeof (ether_head_t))
	{
		g_sp_statistic->packet_err_count++;
		g_sp_statistic->packet_err_size += p_packet_info->s_len;

		SB_FPRINT (stdout, "pdesc->l2_size < sizeof (ether_head_t)\n");
		return eth_type;
	}

	p_packet_info->data_len = p_packet_info->s_len;
	p_packet_info->eth = (ether_head_t *)p_packet_info->s_data;

	eth_type = ntohs (p_packet_info->eth->ether_type);

	offset = sizeof (ether_head_t);

	if(eth_type == ETHERTYPE_VLAN)
	{
		if((p_packet_info->data_len - offset) < sizeof(vlan_head_t))
		{
			GC_FPRINT (stdout, "caplen < sizeof (vlan_head_t)\n");
			return eth_type;
		}


		vlan_head_t *vlan_h = (vlan_head_t *)(p_packet_info->s_data + offset);
		eth_type = ntohs(vlan_h->ethtype);

		offset += sizeof(vlan_head_t);
	}

	p_packet_info->data_len -= offset;
	p_packet_info->p_data = p_packet_info->s_data + offset;

	return eth_type;
}

// 返回0为发生了错误
static inline sb_u8
process_ip_layer(sp_packet_t *p_packet_info, gxio_mpipe_idesc_t *pdesc)
{
	sb_u16 ip_flag_offset;
	sb_u16 ip_flag;
	sb_u16 ip_offset;
	ip_head_t *ip_head;
	sb_u16 ip_len;

	if(unlikely(NULL == p_packet_info ))
	{
		return PROCESS_PROTO_ERROR;
	}
	//! <li> 分析剩余的长度
	if (unlikely( p_packet_info->data_len < sizeof (ip_head_t) ))
	{
		SB_FPRINT(stdout,"data_len < sizeof (ip_head_t)\n");
		return PROCESS_PROTO_ERROR;
	}

	p_packet_info->iph = (sb_u8 *)(p_packet_info->p_data );

	ip_head = (ip_head_t *)p_packet_info->iph;
	ip_len = ntohs(ip_head->ip_len);

	if( unlikely( ((sb_u32)ip_head->ip_hl << 2) > p_packet_info->data_len ||
			ip_len > p_packet_info->data_len ||
			((sb_u32)ip_head->ip_hl << 2) > ip_len ) )
	{
		SB_FPRINT(stdout,"(iph->ip_hl <<2) > caplen\n");
		return PROCESS_PROTO_ERROR;
	}

	p_packet_info->flow.src_ip = ntohl (ip_head->ip_src);
	p_packet_info->flow.dst_ip = ntohl (ip_head->ip_dst);

	//IP 分片处理
	ip_flag_offset = ntohs(ip_head->ip_off);
	ip_flag = ip_flag_offset & 0xE000;

	ip_offset = (ip_flag_offset & 0x1FFF);

	if(ip_flag == 0x2000)//第一个分片
	{
		sb_u16 ip_identification = ntohs(ip_head->ip_id);
		ip_frag_t *frag = find_node_from_frag_pool(
				sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)],
				p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification);
		g_sp_statistic->ip_seg1_packet_count++;
		g_sp_statistic->ip_seg1_size += p_packet_info->s_len;
		// 第一个分片,找到第二个分片，重组
		if(frag )
		{
			sb_u8 *s_data = NULL;
			sb_u16 p_data_len = 0;
			ip_head_t *ip_head_f = NULL;

			if( (s_data = (sb_u8 *)gxio_mpipe_idesc_get_va(&(frag->idesc))) == NULL)
			{
				//释放分片缓存资源和mpipe包缓存资源
				put_to_frag_pool(frag,
						sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)]);
				gxio_mpipe_iqueue_drop(frag->iqueue,&(frag->idesc));

				return PROCESS_PROTO_ERROR;
			}
			//分片合并，将第二个分片的数据拷贝到第一个分片数据末尾，没有考虑合并后长度超过buffer长度的情况
			ip_head_f = (ip_head_t *)(s_data + frag->ip_head_offset);
			p_data_len = ntohs(ip_head_f->ip_len) - (ip_head_f->ip_hl << 2);
			memcpy((sb_u8*)p_packet_info->s_data+p_packet_info->s_len,
					s_data + frag->ip_head_offset + (ip_head_f->ip_hl << 2),
					p_data_len);

			//修正IP长度字段
			ip_len += p_data_len;
			ip_head->ip_len = htons(ip_len);
			p_packet_info->s_len += p_data_len;
			p_packet_info->data_len += p_data_len;
			pdesc->l2_size += p_data_len;
			ip_flag_offset &= 0xDFFF;//去除分片标记
			ip_head->ip_off = htons(ip_flag_offset);
			//重新计算ip 校验码
			ip_head->ip_sum = 0;
			ip_head->ip_sum = sb_ip_cal_csum((void *)ip_head, ip_head->ip_hl);

			//释放分片缓存资源和mpipe包缓存资源
			put_to_frag_pool(frag,
					sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)]);
			gxio_mpipe_iqueue_drop(frag->iqueue,&(frag->idesc));
		}
		else//没有找到，放入缓存
		{
			ip_frag_t *frag = get_from_frag_pool(
					sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)]);
			if(NULL == frag)
			{
				return PROCESS_PROTO_ERROR;
			}
			frag->sip = p_packet_info->flow.src_ip;
			frag->dip = p_packet_info->flow.dst_ip;
			frag->identification = ip_identification;
			frag->ip_head_offset = p_packet_info->p_data - p_packet_info->s_data;
			frag->idesc = *pdesc;
			frag->iqueue = g_iqueue_p;
			p_packet_info->frag_flag = FRAG_FLAG_YES;

			return ip_head->ip_p;
		}
	}
	else if(ip_offset != 0)//第二个分片，目前只处理两个分片的情况
	{
		sb_u16 ip_identification = ntohs(ip_head->ip_id);
		ip_frag_t *frag = find_node_from_frag_pool(
				sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)],
				p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification);
		g_sp_statistic->ip_seg2_packet_count++;
		g_sp_statistic->ip_seg2_size += p_packet_info->s_len;
		// 第二个分片,找到第一个分片，重组
		if(frag )
		{
			sb_u8 *s_data = NULL;
			sb_u16 p_data_len = 0;
			ip_head_t *ip_head_f = NULL;

			if( (s_data = (sb_u8 *)gxio_mpipe_idesc_get_va(&(frag->idesc))) == NULL)
			{
				//释放分片缓存资源和mpipe包缓存资源
				put_to_frag_pool(frag,
						sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)]);
				gxio_mpipe_iqueue_drop(frag->iqueue,&(frag->idesc));

				return PROCESS_PROTO_ERROR;
			}
			//分片合并，将第二个分片的数据拷贝到第一个分片数据末尾，没有考虑合并后长度超过buffer长度的情况
			ip_head_f = (ip_head_t *)(s_data + frag->ip_head_offset);
			p_data_len = ip_len - (ip_head->ip_hl << 2);
			memcpy(s_data + frag->idesc.l2_size,
					p_packet_info->p_data + (ip_head->ip_hl << 2),
					p_data_len);


			//修正IP长度字段
			ip_len = ntohs(ip_head_f->ip_len) + p_data_len;
			ip_head_f->ip_len = htons(ip_len);
			frag->idesc->l2_size += p_data_len;
			//重新赋值数据包信息结构的内容，使指向第一个分片
			p_packet_info->iqueue = frag->iqueue;
			p_packet_info->idesc = frag->idesc;
			p_packet_info->s_data = s_data;
			p_packet_info->s_len = frag->idesc.l2_size;
			p_packet_info->eth = (ether_head_t*)s_data;
			p_packet_info->iph = ip_head_f;
			p_packet_info->p_data = (sb_u8*)ip_head_f;
			p_packet_info->data_len = frag->idesc.l2_size - frag->ip_head_offset;
			ip_flag_offset = ntohs(ip_head_f->ip_off);
			ip_flag_offset &= 0xDFFF;//去除分片标记
			ip_head_f->ip_off = htons(ip_flag_offset);

			//重新计算ip 校验码
			ip_head_f->ip_sum = 0;
			ip_head_f->ip_sum = sb_ip_cal_csum((void *)ip_head_f, ip_head_f->ip_hl);

			ip_head = ip_head_f;

			p_packet_info->frag_flag = FRAG_FLAG_PROCESS;

			//释放分片缓存资源
			put_to_frag_pool(frag,
					sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)]);

		}
		else//没有找到，放入缓存
		{
			ip_frag_t *frag = get_from_frag_pool(
					sp_ipfrag_s[dual_ip_id_hash(p_packet_info->flow.src_ip, p_packet_info->flow.dst_ip, ip_identification)]);
			if(NULL == frag)
			{
				return PROCESS_PROTO_ERROR;
			}
			frag->sip = p_packet_info->flow.src_ip;
			frag->dip = p_packet_info->flow.dst_ip;
			frag->identification = ip_identification;
			frag->ip_head_offset = p_packet_info->p_data - p_packet_info->s_data;
			frag->idesc = *pdesc;
			frag->iqueue = g_iqueue_p;
			p_packet_info->frag_flag = FRAG_FLAG_YES;

			return ip_head->ip_p;

		}
	}

	p_packet_info->p_data += (ip_head->ip_hl << 2);
	p_packet_info->data_len -= (ip_head->ip_hl << 2);

	return ip_head->ip_p;
}

// UDP 包
static inline sb_s32
process_udp_layer(sp_packet_t *p_packet_info)
{
	udp_head_t *udp_head ;

	if(unlikely( NULL == p_packet_info))
	{
		return RET_FAIL;
	}

	if( unlikely(p_packet_info->data_len < sizeof(udp_head_t)) )
	{
		SB_FPRINT(stdout,"data_len < sizeof (udp_head_t)\n");
		return RET_FAIL;
	}

	p_packet_info->l4_headp.udph =	(udp_head_t*)(p_packet_info->p_data);

	udp_head = (udp_head_t *)(p_packet_info->l4_headp.udph);

	p_packet_info->flow.src_port = ntohs (udp_head->uh_sport);
	p_packet_info->flow.dst_port = ntohs (udp_head->uh_dport);

	p_packet_info->p_data += sizeof (udp_head_t);
	p_packet_info->data_len -= sizeof (udp_head_t);

	return RET_SUCC;
}

// SCTP 包
static inline sb_s32
process_sctp_layer(sp_packet_t *p_packet_info)
{
	sctp_head_t *sctp_head ;

	if(unlikely( NULL == p_packet_info ))
	{
		return RET_FAIL;
	}

	if( unlikely(p_packet_info->data_len < sizeof(sctp_head_t)) )
	{
		SB_FPRINT(stdout,"data_len < sizeof (sctp_head_t)\n");
		return RET_FAIL;
	}

	p_packet_info->l4_headp.sctph =	(udp_head_t*)(p_packet_info->p_data);

	sctp_head = (udp_head_t *)(p_packet_info->l4_headp.sctph);

	p_packet_info->flow.src_port = ntohs (sctp_head->sh_sport);
	p_packet_info->flow.dst_port = ntohs (sctp_head->sh_dport);

	p_packet_info->p_data += sizeof (sctp_head_t);
	p_packet_info->data_len -= sizeof (sctp_head_t);

	return RET_SUCC;
}

//mpipe包处理函数
void sp_process_packet_mpipe (gxio_mpipe_idesc_t* pdesc)
{
	//! \par sp_process_packet_mpipe函数主要流程:
	//! \n
	//! <ul>

	//总包数统计

	tmc_mem_prefetch(gxio_mpipe_idesc_get_va(pdesc),pdesc->l2_size);

	sp_packet_t* ptr_sp_packet = &g_sp_packet;
	sb_s32 caplen = 0;
	sb_u16 eth_type = PROCESS_PROTO_ERROR;
	sb_u8 trans_type = PROCESS_PROTO_ERROR;
	sb_u32 l5_proto = PROCESS_PROTO_ERROR;
	sb_u32 offset = 0;
	sb_u64 packet_start_cycle = 0;
	sb_u64 packet_end_cycle = 0;
	sb_u64 packet_now_cycle;
	sb_s32 ret = 0;

	g_current_time = pdesc->time_stamp_ns;
	g_current_time <<= 32;
	g_current_time += pdesc->time_stamp_sec;
	memset(ptr_sp_packet, 0, sizeof(sp_packet_t));

	if( (ptr_sp_packet->s_data = (sb_u8 *)gxio_mpipe_idesc_get_va(pdesc)) == NULL)
	{
		g_sp_statistic->packet_err_count++;
		g_sp_statistic->packet_err_size += pdesc->l2_size;
		gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
		gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

		SB_FPRINT (stdout, "s_data is NULL\n");
		return;

	}

	ptr_sp_packet->s_len = pdesc->l2_size;
	/**********************************
		进入分析L2层
	 *********************************/
	eth_type =	process_l2_layer(ptr_sp_packet);


	//! <li> 处理以太网包
	switch (eth_type)
	{
	//! <ul>
	case ETHERTYPE_IP:
	{
		/**********************************
			进入分析L3层
		 *********************************/
		trans_type = process_ip_layer( ptr_sp_packet,  pdesc);

		//ip分片包
		if(ptr_sp_packet->frag_flag == FRAG_FLAG_YES)
		{
			gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

			return;
		}
		else if(ptr_sp_packet->frag_flag == FRAG_FLAG_PROCESS)//找到了第一个分片
		{
			gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
			gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);
			pdesc = &ptr_sp_packet->idesc;
			g_iqueue_p = ptr_sp_packet->iqueue;
		}
		switch (trans_type)
		{
		//! <ul>
		//! <li> TCP包协议分析过程，直接丢弃
		//

		case IPPROTO_TCP:

			g_sp_statistic->tcp_packet_count++;
			g_sp_statistic->tcp_packet_size += pdesc->l2_size;
			gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
			if(ptr_sp_packet->frag_flag != FRAG_FLAG_PROCESS)
				gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

			return;
			//! <li> UDP包协议分析过程
		case IPPROTO_UDP:

			ptr_sp_packet->flow.l4_protocol = IPPROTO_UDP;
			g_sp_statistic->udp_packet_count++;
			g_sp_statistic->udp_packet_size += pdesc->l2_size;

			ret = process_udp_layer(ptr_sp_packet);

			if(RET_SUCC != ret)
			{
				gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
				if(ptr_sp_packet->frag_flag != FRAG_FLAG_PROCESS)
					gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

				return;
			}

			break;
			//! <li> SCTP包协议分析过程
		case IPPROTO_SCTP:

			ptr_sp_packet->flow.l4_protocol = IPPROTO_SCTP;
			g_sp_statistic->sctp_packet_count++;
			g_sp_statistic->sctp_packet_size += pdesc->l2_size;

			ret = process_sctp_layer(ptr_sp_packet);

			//			if(RET_SUCC != ret)
			{
				gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
				if(ptr_sp_packet->frag_flag != FRAG_FLAG_PROCESS)
					gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

				return;
			}

			break;
		default:
			g_sp_statistic->ntcpudp_packet_count++;

			gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
			if(ptr_sp_packet->frag_flag != FRAG_FLAG_PROCESS)
				gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

			return;
			//! </ul>
		}
	}
	break;
	default:
		g_sp_statistic->nip_packet_count++;

		gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
		gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

		return;
		//! </ul>
	}

	//获取接口类型和协议类型
	ret = sp_get_l5_proto(ptr_sp_packet, pdesc);
	if(RET_SUCC != ret)
	{
		gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
		if(ptr_sp_packet->frag_flag != FRAG_FLAG_PROCESS)
			gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

		return;
	}

	switch(ptr_sp_packet->interface)
	{
	case GB_INT:
	{
		ret = sp_process_gb(ptr_sp_packet, pdesc);
		if(RET_SUCC != ret)
		{
			gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
			if(ptr_sp_packet->frag_flag != FRAG_FLAG_PROCESS)
				gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

			return;
		}
		break;
	}
	case IUPS_INT:
	{
//		break;
	}
	case S1MME_INT:
	{
//		break;
	}
	case S1U_INT:
	{
		break;
	}
	case S6A_INT:
	{
//		break;
	}
	case S10_INT:
	{
//		break;
	}
	case S11_INT:
	{
//		break;
	}
	case GN_INT:
	{
//		break;
	}
	case S5_INT:
	{
//		break;
	}
	default:
	{
		gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
		if(ptr_sp_packet->frag_flag != FRAG_FLAG_PROCESS)
			gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

		break;
	}
	}

	return;

}

sb_s32 sp_get_l5_proto(sp_packet_t *p_packet_info, gxio_mpipe_idesc_t *pdesc)
{
	sb_s32 ret = RET_FAIL;

	sb_u8 interface = INTERFACE_ERROR;
	sb_u16 proto_type = PROCESS_PROTO_ERROR;
	sb_u16 i;
	sb_u32 link_num = 0;

	//前端有负载均衡设备
	if(g_device_config.is_lb_exist)
	{
		sb_u16 tag_offset = 0;

		//获取标签偏移量
		memcpy((sb_u8*)&tag_offset, (sb_u8*)p_packet_info->s_data + 10, 2);
		tag_offset = ntohs(tag_offset);
		g_sp_lb_tag = p_packet_info->s_data + tag_offset;
		//从标签中获取接口类型和协议类型
		p_packet_info->interface = g_sp_lb_tag->interface;
		p_packet_info->proto_type = ntohs(g_sp_lb_tag->proto_type);

		return RET_SUCC;
	}

	//前端没有负载均衡设备
	switch(p_packet_info->flow.l4_protocol)
	{
	case IPPROTO_UDP:
	{
		if(p_packet_info->flow.dst_port == 2152 || p_packet_info->flow.src_port == 2152)
		{
			p_packet_info->interface = IUPS_INT;//IUPS/S1-U/GN口/S5
			p_packet_info->proto_type = PROTO_GB_GTP_U;

			//通过输入口来判断接口
			if(g_device_config.iups_ports.interface_num > 0)
				for(i = 0; i < g_device_config.iups_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.iups_ports.channel_no)
					{
						p_packet_info->interface = IUPS_INT;
						p_packet_info->proto_type = PROTO_GB_GTP_U;//IUPS

						return RET_SUCC;
					}
				}

			if(g_device_config.s1u_ports.interface_num > 0)
				for(i = 0; i < g_device_config.s1u_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.s1u_ports.channel_no)
					{
						p_packet_info->interface = S1U_INT;//S1U
						p_packet_info->proto_type = PROTO_LTE_S1U_COMMON;

						return RET_SUCC;
					}
				}

			if(g_device_config.gn_ports.interface_num > 0)
				for(i = 0; i < g_device_config.gn_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.gn_ports.channel_no)
					{
						p_packet_info->interface = GN_INT;//GN
						p_packet_info->proto_type = PROTO_GB_GTP_U;

						return RET_SUCC;
					}
				}

			if(g_device_config.s5_ports.interface_num > 0)
				for(i = 0; i < g_device_config.s5_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.s5_ports.channel_no)
					{
						p_packet_info->interface = S5_INT;//S5
						p_packet_info->proto_type = PROTO_LTE_S1U_COMMON;

						return RET_SUCC;
					}
				}

			//通过IP判断接口
			link_num = p_packet_info->flow.dst_ip % MAX_PRO_IP_LINK_NUM;

			if(g_proto_ips_num[link_num] > 0)
				for(i = 0; i < g_proto_ips_num[link_num]; i++)
				{
					if(p_packet_info->flow.dst_ip == g_proto_ips[link_num][i].ip)
					{
						p_packet_info->interface = g_proto_ips[link_num][i].proto_id;

						if(p_packet_info->interface == IUPS_INT || p_packet_info->interface == GN_INT)
						{
							p_packet_info->proto_type = PROTO_GB_GTP_U;
						}
						else if(p_packet_info->interface == S1U_INT || p_packet_info->interface == S5_INT)
						{
							p_packet_info->proto_type = PROTO_LTE_S1U_COMMON;
						}

						return RET_SUCC;
					}
				}

			link_num = p_packet_info->flow.src_ip % MAX_PRO_IP_LINK_NUM;

			if(g_proto_ips_num[link_num] > 0)
				for(i = 0; i < g_proto_ips_num[link_num]; i++)
				{
					if(p_packet_info->flow.src_ip == g_proto_ips[link_num][i].ip)
					{
						p_packet_info->interface = g_proto_ips[link_num][i].proto_id;

						if(p_packet_info->interface == IUPS_INT || p_packet_info->interface == GN_INT)
						{
							p_packet_info->proto_type = PROTO_GB_GTP_U;
						}
						else if(p_packet_info->interface == S1U_INT || p_packet_info->interface == S5_INT)
						{
							p_packet_info->proto_type = PROTO_LTE_S1U_COMMON;
						}

						return RET_SUCC;
					}
				}
			return RET_SUCC;
		}
		else if(p_packet_info->flow.dst_port == 2123 || p_packet_info->flow.src_port == 2123)
		{
			p_packet_info->interface = GN_INT;//S10/S11/S5/S8/GN口
			p_packet_info->proto_type = PROTO_GB_GTP_C;
			//通过输入口来判断接口
			if(g_device_config.s10_ports.interface_num > 0)
				for(i = 0; i < g_device_config.s10_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.s10_ports.channel_no)
					{
						p_packet_info->interface = S10_INT;//S10
						p_packet_info->proto_type = PROTO_LTE_GTPV2;

						return RET_SUCC;
					}
				}

			if(g_device_config.s11_ports.interface_num > 0)
				for(i = 0; i < g_device_config.s11_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.s11_ports.channel_no)
					{
						p_packet_info->interface = S11_INT;//S11
						p_packet_info->proto_type = PROTO_LTE_GTPV2;

						return RET_SUCC;
					}
				}

			if(g_device_config.s5_ports.interface_num > 0)
				for(i = 0; i < g_device_config.s5_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.s5_ports.channel_no)
					{
						p_packet_info->interface = S5_INT;//S5
						p_packet_info->proto_type = PROTO_LTE_GTPV2;

						return RET_SUCC;
					}
				}

			if(g_device_config.gn_ports.interface_num > 0)
				for(i = 0; i < g_device_config.gn_ports.interface_num; i++)
				{
					if(pdesc->channel == g_device_config.gn_ports.channel_no)
					{
						p_packet_info->interface = GN_INT;//GN
						p_packet_info->proto_type = PROTO_GB_GTP_C;

						return RET_SUCC;
					}
				}

			//通过IP判断接口
			link_num = p_packet_info->flow.dst_ip % MAX_PRO_IP_LINK_NUM;

			if(g_proto_ips_num[link_num] > 0)
				for(i = 0; i < g_proto_ips_num[link_num]; i++)
				{
					if(p_packet_info->flow.dst_ip == g_proto_ips[link_num][i].ip)
					{
						p_packet_info->interface = g_proto_ips[link_num][i].proto_id;

						if(p_packet_info->interface == GN_INT || p_packet_info->interface == GN_INT)
						{
							p_packet_info->proto_type = PROTO_GB_GTP_C;
						}
						else if(p_packet_info->interface == S10_INT ||
								p_packet_info->interface == S11_INT || p_packet_info->interface == S5_INT)
						{
							p_packet_info->proto_type = PROTO_LTE_GTPV2;
						}

						return RET_SUCC;
					}
				}

			link_num = p_packet_info->flow.src_ip % MAX_PRO_IP_LINK_NUM;

			if(g_proto_ips_num[link_num] > 0)
				for(i = 0; i < g_proto_ips_num[link_num]; i++)
				{
					if(p_packet_info->flow.src_ip == g_proto_ips[link_num][i].ip)
					{
						p_packet_info->interface = g_proto_ips[link_num][i].proto_id;

						if(p_packet_info->interface == GN_INT || p_packet_info->interface == GN_INT)
						{
							p_packet_info->proto_type = PROTO_GB_GTP_C;
						}
						else if(p_packet_info->interface == S10_INT ||
								p_packet_info->interface == S11_INT || p_packet_info->interface == S5_INT)
						{
							p_packet_info->proto_type = PROTO_LTE_GTPV2;
						}

						return RET_SUCC;
					}
				}
			return RET_SUCC;
		}
		else
		{
			p_packet_info->interface = GB_INT;//GB口
			p_packet_info->proto_type = PROTO_GB_BSSGP;

			return RET_SUCC;
		}
		break;
	}
	case IPPROTO_SCTP:
	{
		sctp_chunk_t *chunk_h;

		chunk_h = (sctp_chunk_t*)p_packet_info->p_data;
		if(chunk_h->sch_type == SCTP_DATA && chunk_h->sch_flags == SCTP_TYPE_FLAG_FLSEG)
		{
			if(chunk_h->protocol == SCTP_PROTOCOL_M3UA)
			{
				p_packet_info->interface = IUPS_INT;//IUPS口
				p_packet_info->proto_type = PROTO_GB_RANAP;

				return RET_SUCC;
			}
			else if(chunk_h->protocol == SCTP_PROTOCOL_S1AP)
			{
				p_packet_info->interface = S1MME_INT;//S1-MME口
				p_packet_info->proto_type = PROTO_LTE_S1AP_NAS;

				return RET_SUCC;
			}
			else if((p_packet_info->flow.dst_port == 3868 || p_packet_info->flow.src_port == 3868)
					&& (chunk_h->protocol == SCTP_PROTOCOL_UN || chunk_h->protocol == SCTP_PROTOCOL_DIAMETER
							|| chunk_h->protocol == SCTP_PROTOCOL_TALI))
			{
				p_packet_info->interface = S6A_INT;//S6a口
				p_packet_info->proto_type = PROTO_LTE_DIAMETER;

				return RET_SUCC;
			}
		}
		break;
	}
	default:
		break;
	}

	return ret;
}

/*!
  \fn sb_s32 add_to_pack_queue( sp_user_info_t * ui  )
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
sb_s32 add_to_pack_queue (sp_packet_info_t * pack)
{
  sb_u64 push_index = 0, pop_index = 0;
  sb_u64 start_cycle = 0;
  sb_u64 end_cycle = 0;
  sb_u64 now_cycle;

	if(NULL == g_sp_packet_queue_info_p)
	{
		SB_ADD_LOG(LOG_LEVEL_DEBUG)
				(SB_LOG_ALERT,0, "g_sp_user_queue_info_p is NULL!");

		return RET_FAIL;
	}
	sb_u16 queue_index = pack->user_id % g_sp_packet_queue_info_p->num;
	sp_packet_queue_t* pack_queue_p = g_sp_packet_queue_info_p->queue_array[queue_index];

	push_index = pack_queue_p->push_index;
	pop_index = pack_queue_p->pop_index;

	if(push_index-pop_index >= ((sb_u64)pack_queue_p->size))			//队列满了
	{
		SB_ADD_LOG(LOG_LEVEL_DEBUG)
				(SB_LOG_ALERT,0, "  queue[%d] is full, push_index is %llu pop_index is %llu queue size is %u !",
						queue_index, pack_queue_p->push_index, pack_queue_p->pop_index, pack_queue_p->size);
		g_sp_statistic->packet_fwd_fail_count++;

		return RET_FAIL;
	}
	start_cycle = get_cycle_count();
	g_sp_statistic->packet_fwd_count++;

	sp_packet_info_t *pack_p = &pack_queue_p->sp_packet[pack_queue_p->tail];		//获得需要传入的指针
	memcpy(pack_p, pack, sizeof(sp_packet_info_t));
	pack_queue_p->tail = (pack_queue_p->tail+1)% pack_queue_p->size;

	__insn_mf();

	pack_queue_p->push_index++;

	end_cycle = get_cycle_count();
	now_cycle = end_cycle - start_cycle;

	return RET_SUCC;
}
