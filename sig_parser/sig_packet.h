/*
 * sig_packet.h
 *
 *  Created on: 2014年6月17日
 *      Author: Figoo
 */

#ifndef SIG_PACKET_H_
#define SIG_PACKET_H_

#define PROCESS_PROTO_ERROR	0
#define INTERFACE_ERROR 0

/* chunk types */
#define SCTP_DATA               0x00
#define SCTP_INIT               0x01
#define SCTP_INIT_ACK           0x02
#define SCTP_SACK               0x03
#define SCTP_HEARTBEAT          0x04
#define SCTP_HEARTBEAT_ACK      0x05
#define SCTP_ABORT              0x06
#define SCTP_SHUTDOWN           0x07
#define SCTP_SHUTDOWN_ACK       0x08
#define SCTP_ERROR              0x09
#define SCTP_COOKIE_ECHO        0x0a
#define SCTP_COOKIE_ACK         0x0b
#define SCTP_ECNE               0x0c
#define SCTP_CWR                0x0d
#define SCTP_SHUTDOWN_COMPLETE  0x0e
#define SCTP_AUTH               0x0f
#define SCTP_ASCONF_ACK         0x80
#define SCTP_PKTDROP            0x81
#define SCTP_PAD                0x84
#define SCTP_FORWARD_TSN        0xc0
#define SCTP_ASCONF             0xc1

/* chunk types bitmask flags */
#define SCTP_TYPEFLAG_REPORT    1
#define SCTP_TYPEFLAG_SKIP      2

#define SCTP_TYPE_FLAG_FLSEG		3

#define SCTP_PROTOCOL_M3UA			3
#define SCTP_PROTOCOL_S1AP			18
#define SCTP_PROTOCOL_UN			0
#define SCTP_PROTOCOL_DIAMETER		46
#define SCTP_PROTOCOL_TALI			9

#define MAX_SCTP_SIGS				15

//sctp头
typedef struct sctp_hdr {
	sb_u16 		sh_sport;       /* source port */
	sb_u16		sh_dport;       /* destination port */
	sb_u32		sh_vtag;        /* sctp verification tag */
	sb_u32		sh_sum;         /* sctp checksum */
} sctp_head_t;

typedef struct sctp_chunkhdr {
	sb_u8		sch_type;       /* chunk type */
	sb_u8		sch_flags;      /* chunk flags */
	sb_u16 		sch_length;     /* chunk length */

	// optional, 这里根据flag为3的定义
	sb_u32	sch_tsn;
	sb_u16	sch_si;
	sb_u16	sch_ssn;
	sb_u32	protocol;
} sctp_chunk_t;


//! sig_parser接收到的数据包结构，剥离以太网头和tcp、udp、sctp头
typedef struct sp_packet_tag
{
	gxio_mpipe_iqueue_t *iqueue;
	gxio_mpipe_idesc_t idesc;
	sb_u8* s_data;				//!< 原始数据报的头
	sb_u32 s_len;				//!< 原始数据报的长度
	sb_u64 hash_code;			//!< 四元组hash值   由inetrace md5计算或者由 mica计算得到 (classifier这个不能使用)

	ether_head_t *eth;			//!< 以太头指针
	ip_head_t *iph;				//!< ip头指针
	frag_flag_e	 frag_flag;	//!< ip分片标记
	union{
		tcp_head_t *tcph;			//!< tcp头指针
		udp_head_t *udph;			//!< udp头指针
		sctp_head_t *sctph;			//!< sctp头指针
	}l4_headp;
	sb_flow_t flow;				//!< 根据原始数据包进行分析获得的四原组
	sb_s32 data_len;			//!< tcp或者udp、sctp数据长度
	sb_u8 *p_data;				//!< tcp或者udp、sctp数据段指针，报文内容

	sb_u8 interface;			//!< 接口类型
	sb_u16 proto_type;			//!< 标识协议类型

} sp_packet_t;

//! 负载均衡设备标签
typedef struct sp_lb_tag
{
	sb_u64 user_id;				//!< 用户ID
	sb_u8 interface;			//!< 接口类型，0x01：Gn，0x03:IuPS，0x04：Gb，0x11：S1-MME，0x12：S6a，0x13：S11， 0x14：S10，0x15：S1-U
	sb_u8 flow_direct;			//!< 数据方向，0：上行，1：下行，0xFF：未知
	sb_u16 proto_type;			//!< 标识协议类型
	sb_u32 time_s;				//!< 时间戳，秒
	sb_u32 time_ns;				//!< 时间戳，纳秒
	sb_u8 sctp_sig_num;			//!< SCTP包含的信令个数
	sb_u8 sctp_sig_no[MAX_SCTP_SIGS];			//!< SCTP信令序号

} sp_lb_t;

extern sp_packet_t g_sp_packet;
extern sp_lb_t *g_sp_lb_tag;

extern gxio_mpipe_iqueue_t *gg_iqueue_p;
extern gxio_mpipe_iqueue_t *g_iqueue_p;

extern sp_user_queue_info_t *g_sp_user_queue_info_p;
extern sp_packet_queue_info_t *g_sp_packet_queue_info_p;
extern sp_xdr_queue_info_t *g_sp_xdr_queue_info_p;

extern sp_statistic_t* g_sp_statistic;	//!< sig_parser统计信息结构

extern sp_xdr_t g_xdr;
extern sp_user_info_t g_ui;
extern sp_packet_info_t g_pack;

sb_u16 sb_ip_cal_csum(void *buff, sb_s32 len);
sb_s32 sp_get_l5_proto(sp_packet_t *p_packet_info, gxio_mpipe_idesc_t *pdesc);
void sp_process_packet_mpipe (gxio_mpipe_idesc_t* pdesc);
sb_s32 add_to_pack_queue (sp_packet_info_t * pack);

#endif /* SIG_PACKET_H_ */
