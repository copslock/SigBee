/*
 * sb_shared_queue.h
 *
 *  Created on: 2014年5月28日
 *      Author: Figoo
 */

#ifndef SB_SHARED_QUEUE_H_
#define SB_SHARED_QUEUE_H_

#include "sb_type.h"
#include "sb_defines.h"

//sig_parser到data_parser的用户信息队列结构
#pragma pack(1)
typedef struct _sp_user_info_tag
{
	sb_u64 user_id;				//!< 用户ID
	sb_u8 interface;			//!< 接口类型，0x01：Gn，0x03:IuPS，0x04：Gb，0x11：S1-MME，0x12：S6a，0x13：S11， 0x14：S10，0x15：S1-U
	sb_u8 imsi[15];
	sb_u8 imei[16];
	sb_u32 ptmsi;
	sb_u32 mcc;
	sb_u32 mnc;
	sb_u32 lac;
	sb_u32 rac;
	sb_u32 cid;
	sb_u32 old_mcc;
	sb_u32 old_mnc;
	sb_u32 old_lac;
	sb_u32 old_rac;
	sb_u32 old_cid;
	sb_u32 sgsn_sig_ip;
	sb_u32 bsc_sig_ip;
	sb_u8 rat;				//!< 无线网络类型， 1：UTRAN，2：GERAN，3：WLAN，4：GAN，5：HSPA，6：EUTRAN
	sb_u8 apn[32];
}sp_user_info_t;
#pragma pack()

typedef struct _sp_user_info_queue_tag
{
	sp_user_info_t *user_infos;	//!< 存放用户信息的数组，创建队列时指定大小
	sb_u32 head;				//!< 队首下标
	sb_u32 tail;				//!< 队尾下标
	sb_u32 count;				//!< 队列长度
	sb_u32 size;				//!< 队列大小
	sb_u64 push_index;			//!< 放入的总个数
	sb_u64 pop_index;			//!< 取出的总个数 push_index-pop_index < size 即为可写入  push_index!=pop_index即为可读

} sp_user_info_queue_t;


typedef struct sp_user_queue_head_tag
{
	sb_s32 num;			//队列数量
	sp_user_info_queue_t *addr;	//队列数组
} sb_sp_user_queue_head_t;

//sig_parser到data_parser的数据包信息队列结构
typedef struct _sp_packet_info_tag{
	gxio_mpipe_iqueue_t *iqueue;
	gxio_mpipe_idesc_t idesc;

	sb_u64 user_id;				//!< 用户ID,0:未知用户
	sb_u32 offset;				//!< SNDCP层偏移量
	sb_u32 size;				//!< SNDCP及以上层数据长度
	sb_u8 interface;			//!< 接口类型，0x01：Gn，0x03:IuPS，0x04：Gb，0x11：S1-MME，0x12：S6a，0x13：S11， 0x14：S10，0x15：S1-U
	sb_u8 packet_direct;		//!< 数据方向，0:上行，1：下行，0xFF:方向不确定
	sb_u8 psql_send;			//!< 是否要作为PS全量数据发送，0：不确定，需要在用户面数据处理中对用户IP进行匹配后决定，1：需要
} sp_packet_info_t;

typedef struct _sp_packet_queue_tag
{
	sp_packet_info_t *sp_packet;	//!< 存放数据包信息的数组，创建队列时指定大小
	sb_u32 head;				//!< 队首下标
	sb_u32 tail;				//!< 队尾下标
	sb_u32 count;				//!< 队列长度
	sb_u32 size;				//!< 队列大小
	sb_u64 push_index;			//!< 放入的总个数
	sb_u64 pop_index;			//!< 取出的总个数 push_index-pop_index < size 即为可写入  push_index!=pop_index即为可读
} sp_packet_queue_t;

typedef struct sp_packet_queue_head_tag
{
	sb_s32 num;			//队列数量
	sp_packet_queue_t *addr;	//队列数组
} sb_sp_packet_queue_head_t;

//data_parser到xdr_sender的队列结构
typedef struct _sp_xdr{
	sb_u16 length;			//!< XDR内容的长度

	sb_u8 data[MAX_SIG_XDR_LEN];		//!< 控制信令XDR内容

} sp_xdr_t;

typedef struct _sp_xdr_queue_tag
{
	sp_xdr_t *lb_xdrs;	//!< 存放XDR的数组，创建队列时指定大小
	sb_u32 head;				//!< 队首下标
	sb_u32 tail;				//!< 队尾下标
	sb_u32 count;				//!< 队列长度
	sb_u32 size;				//!< 队列大小
	sb_u64 push_index;			//!< 放入的总个数
	sb_u64 pop_index;			//!< 取出的总个数 push_index-pop_index < size 即为可写入  push_index!=pop_index即为可读

} sp_xdr_queue_t;

typedef struct sp_xdr_queue_head_tag
{
	sb_s32 num;			//队列数量
	sp_xdr_queue_t *addr;	//队列数组
} sb_sp_xdr_queue_head_t;

//data_parser到xdr_sender的队列结构
typedef struct _dp_xdr{
	sb_u16 length;			//!< XDR内容的长度

	sb_u8 data[MAX_DATA_XDR_LEN];		//!< 数据面XDR内容

} dp_xdr_t;

typedef struct _dp_xdr_queue_tag
{
	dp_xdr_t *lb_xdrs;	//!< 存放XDR的数组，创建队列时指定大小
	sb_u32 head;				//!< 队首下标
	sb_u32 tail;				//!< 队尾下标
	sb_u32 count;				//!< 队列长度
	sb_u32 size;				//!< 队列大小
	sb_u64 push_index;			//!< 放入的总个数
	sb_u64 pop_index;			//!< 取出的总个数 push_index-pop_index < size 即为可写入  push_index!=pop_index即为可读

} dp_xdr_queue_t;

typedef struct dp_xdr_queue_head_tag
{
	sb_s32 num;			//队列数量
	dp_xdr_queue_t *addr;	//队列数组
} sb_dp_xdr_queue_head_t;

//data_parser到ui_sender的用户信息队列结构
#pragma pack(1)
typedef struct _dp_user_info_tag
{
	sb_u8 interface;			//!< 接口类型，0x01：Gn，0x03:IuPS，0x04：Gb，0x11：S1-MME，0x12：S6a，0x13：S11， 0x14：S10，0x15：S1-U
	sb_u8 imsi[15];
	sb_u8 imei[16];
	sb_u32 ptmsi;
	sb_u32 mcc;
	sb_u32 mnc;
	sb_u32 lac;
	sb_u32 rac;
	sb_u32 cid;
	sb_u32 old_mcc;
	sb_u32 old_mnc;
	sb_u32 old_lac;
	sb_u32 old_rac;
	sb_u32 old_cid;
	sb_u32 sgsn_sig_ip;
	sb_u32 bsc_sig_ip;
	sb_u8 rat;				//!< 无线网络类型， 1：UTRAN，2：GERAN，3：WLAN，4：GAN，5：HSPA，6：EUTRAN
	sb_u8 apn[32];
}dp_user_info_t;
#pragma pack()

typedef struct _dp_user_info_queue_tag
{
	dp_user_info_t *user_infos;	//!< 存放用户信息的数组，创建队列时指定大小
	sb_u32 head;				//!< 队首下标
	sb_u32 tail;				//!< 队尾下标
	sb_u32 count;				//!< 队列长度
	sb_u32 size;				//!< 队列大小
	sb_u64 push_index;			//!< 放入的总个数
	sb_u64 pop_index;			//!< 取出的总个数 push_index-pop_index < size 即为可写入  push_index!=pop_index即为可读

} dp_user_info_queue_t;


typedef struct sp_dp_user_queue_head_tag
{
	sb_s32 num;			//队列数量
	dp_user_info_queue_t *addr;	//队列数组
} sb_dp_user_queue_head_t;

sb_s32 init_sp_user_share_queues(void);
sb_s32 destroy_sp_user_share_queues(void);
sb_s32 init_sp_packet_share_queues(void);
sb_s32 destroy_sp_packet_share_queues(void);
sb_s32 init_sp_xdr_share_queues(void);
sb_s32 destroy_sp_xdr_share_queues(void);
sb_s32 init_dp_xdr_share_queues(void);
sb_s32 destroy_dp_xdr_share_queues(void);
sb_s32 init_dp_user_share_queues(void);
sb_s32 destroy_dp_user_share_queues(void);

#endif /* SB_SHARED_QUEUE_H_ */
