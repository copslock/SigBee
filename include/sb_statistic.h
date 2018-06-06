/*
 * sb_statistic.h
 *
 *  Created on: 2014年5月28日
 *      Author: Figoo
 */

#ifndef SB_STATISTIC_H_
#define SB_STATISTIC_H_

//! 性能测试用计数器结构
typedef struct sb_per_statistic_tag
{
	sb_u64 run_count;            //!< 运行次数
	sb_u64 run_total_cycle;       //!< 总处理时间
	sb_u64 run_max_cycle;         //!< 最大处理时间

} sb_per_statistic_t;

//! sig_parser statistics
typedef struct sp_statistic_tag
{
	sb_u32 cpu_id;				//!< cpu id

	sb_u64 packet_count;		//!< 收到的数据包总数
	sb_u64 packet_size;			//!< 收到的包总字节数
	sb_u64 packet_fwd_count;	//!< 转发的数据包总数
	sb_u64 packet_fwd_size;		//!< 转发的包总字节数
	sb_u64 packet_err_count;	//!< 错误数据包总数
	sb_u64 packet_err_size;		//!< 错误数据包总字节数
	sb_u64 packet_fwd_fail_count;	//!< 转发失败的数据包总数
	sb_u64 packet_fwd_fail_size;	//!< 转发失败的包总字节数

	sb_u64 xdr_fwd_succ_count;	//!< 转发XDR成功的总数
	sb_u64 xdr_fwd_fail_count;	//!< 转发XDR失败的总数
	sb_u64 ui_fwd_succ_count;	//!< 转发用户信息成功的总数
	sb_u64 ui_fwd_fail_count;	//!< 转发用户信息失败的总数

	sb_u64 be_err_count;
	sb_u64 me_err_count;
	sb_u64 tr_err_count;
	sb_u64 ce_err_count;
	sb_u64 cs_err_count;
	sb_u64 cus_err_count;

	sb_u64 tcp_packet_count;	//!< 收到的TCP数据包总数
	sb_u64 tcp_packet_size;		//!< 收到的TCP包总字节数
	sb_u64 udp_packet_count;	//!< 收到的UDP数据包总数
	sb_u64 udp_packet_size;		//!< 收到的UDP包总字节数
	sb_u64 sctp_packet_count;	//!< 收到的SCTP数据包总数
	sb_u64 sctp_packet_size;		//!< 收到的SCTP包总字节数
	sb_u64 nip_packet_count;        //!< 收到的非IP数据包总数
	sb_u64 ntcpudp_packet_count;        //!< 收到的非TCP/UDP数据包总数
	sb_u64 ip_seg1_packet_count;		//!< 第一个IP分片包总数
	sb_u64 ip_seg1_size;			//!< 第一个IP分片包总字节数
	sb_u64 ip_seg2_packet_count;		//!< 第二个IP分片包总数
	sb_u64 ip_seg2_size;			//!< 第二个IP分片包总字节数

	sb_u64 gn_packet_count;			//!< GN口数据包总数
	sb_u64 gn_packet_size;			//!< GN口总字节数
	sb_u64 gn_sig_packet_count;		//!< GN口信令包总数
	sb_u64 gn_sig_packet_size;		//!< GN口信令总字节数
	sb_u64 gn_user_packet_count;	//!< GN口用户面数据包
	sb_u64 gn_user_packet_size;		//!< GN口用户面总字节数
	sb_u64 iups_packet_count;
	sb_u64 iups_packet_size;
	sb_u64 iups_sig_packet_count;
	sb_u64 iups_sig_packet_size;
	sb_u64 iups_user_packet_count;
	sb_u64 iups_user_packet_size;
	sb_u64 gb_packet_count;
	sb_u64 gb_packet_size;
	sb_u64 gb_sig_packet_count;
	sb_u64 gb_sig_packet_size;
	sb_u64 gb_user_packet_count;
	sb_u64 gb_user_packet_size;
	sb_u64 s1mme_packet_count;
	sb_u64 s1mme_packet_size;
	sb_u64 s6a_packet_count;
	sb_u64 s6a_packet_size;
	sb_u64 s11_packet_count;
	sb_u64 s11_packet_size;
	sb_u64 s10_packet_count;
	sb_u64 s10_packet_size;
	sb_u64 s1u_packet_count;
	sb_u64 s1u_packet_size;
	sb_u64 s5_packet_count;
	sb_u64 s5_packet_size;
	sb_u64 s5_sig_packet_count;
	sb_u64 s5_sig_packet_size;
	sb_u64 s5_user_packet_count;
	sb_u64 s5_user_packet_size;

	sb_u64 total_process_time;	//!< 所有数据包处理总机器周期数
	sb_u32 max_process_time;	//!< 数据包处理最大的机器周期数
	sb_u32 min_process_time;	//!< 数据包处理最小的机器周期数
}__attribute((aligned(64))) sp_statistic_t;

//! data_parser statistics
typedef struct dp_statistic_tag
{
	sb_u32 cpu_id;				//!< cpu id

	sb_u64 packet_count;		//!< 收到的数据包总数
	sb_u64 packet_size;			//!< 收到的包总字节数
	sb_u64 packet_fwd_count;	//!< 转发的数据包总数
	sb_u64 packet_fwd_size;		//!< 转发的包总字节数
	sb_u64 packet_fwd_fail_count;	//!< 转发失败的数据包总数
	sb_u64 packet_fwd_fail_size;	//!< 转发失败的包总字节数

	sb_u64 tcp_packet_count;	//!< 收到的TCP数据包总数
	sb_u64 tcp_packet_size;		//!< 收到的TCP包总字节数
	sb_u64 udp_packet_count;	//!< 收到的UDP数据包总数
	sb_u64 udp_packet_size;		//!< 收到的UDP包总字节数
	sb_u64 nip_packet_count;        //!< 收到的非IP数据包总数
	sb_u64 ntcpudp_packet_count;        //!< 收到的非TCP/UDP数据包总数

	sb_u64 total_process_time;	//!< 所有数据包处理总机器周期数
	sb_u32 max_process_time;	//!< 数据包处理最大的机器周期数
	sb_u32 min_process_time;	//!< 数据包处理最小的机器周期数
}__attribute((aligned(64))) dp_statistic_t;

#endif /* SB_STATISTIC_H_ */
