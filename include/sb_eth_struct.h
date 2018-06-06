/*
 * sb_eth_struct.h
 *
 *  Created on: 2014年5月28日
 *      Author: Figoo
 */

#ifndef SB_ETH_STRUCT_H_
#define SB_ETH_STRUCT_H_

#include "sb_type.h"
#include "sb_defines.h"

/*! \name 数据包包头结构
*/
//@{
//! 以太网头
typedef struct ether_head_tag
{
	sb_u8 ether_dsthost[ETHER_LEN];	//!< 目的MAC
	sb_u8 ether_srchost[ETHER_LEN];	//!< 源MAC
	sb_u16 ether_type;			//!< 以太网协议类型
} ether_head_t;

typedef struct _vlan_head
{
	sb_u16	priority;
	sb_u16	ethtype;
}
vlan_head_t;

//! 定义IP头
typedef struct ip_head_tag
{
#ifdef WORDS_BIGENDIAN
	sb_u8 ip_v:4, ip_hl:4;
#else
	sb_u8 ip_hl:4, ip_v:4;
#endif
	sb_u8 ip_tos;
	sb_u16 ip_len;
	sb_u16 ip_id;
	sb_u16 ip_off;
	sb_u8 ip_ttl;
	sb_u8 ip_p;
	sb_u16 ip_sum;
	sb_in_addr ip_src;
	sb_in_addr ip_dst;
} ip_head_t;

//! tcp包头
typedef struct tcp_head_tag
{
	sb_u16 th_sport;
	sb_u16 th_dport;
	sb_u32 th_seq;
	sb_u32 th_ack;
#ifdef WORDS_BIGENDIAN
	sb_u8 th_off:4, th_x2:4;
#else
	sb_u8 th_x2:4, th_off:4;
#endif
	sb_u8 th_flags;
	sb_u16 th_win;
	sb_u16 th_sum;
	sb_u16 th_urp;
} tcp_head_t;

//! udp包头
typedef struct
{
	sb_u16 uh_sport;			/* source port */
	sb_u16 uh_dport;			/* destination port */
	sb_u16 uh_ulen;				/* udp length */
	sb_u16 uh_sum;				/* udp checksum */
} udp_head_t;

//! 流5元组结构
typedef struct _flow
{
	sb_u32 src_ip;				//!< 源IP，Source IP address
	sb_u32 dst_ip;				//!< 目的IP，Destination IP address
	sb_u16 src_port;			//!< 源端口，Source port number
	sb_u16 dst_port;			//!< 目的端口，Destination port number
	sb_u8  l4_protocol;			//!< 四层协议类型,tcp/udp
} sb_flow_t;


#endif /* SB_ETH_STRUCT_H_ */
