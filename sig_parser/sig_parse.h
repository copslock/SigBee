/*
 * sig_parse.h
 *
 *  Created on: 2014年6月10日
 *      Author: Figoo
 */

#ifndef SIG_PARSE_H_
#define SIG_PARSE_H_

typedef struct sp_per_statistics_tag
{

	sb_per_statistic_t add_to_user_info_queue_f;       //!< add_to_user_info_queue函数调用
	sb_per_statistic_t update_tlli_core_link_f;
	sb_per_statistic_t get_tlli_f;
//	sb_per_statistic_t update_tlli_core_link1_f;
//	sb_per_statistic_t update_tlli_core_link2_f;
	sb_per_statistic_t timeout_tlli_core_link_f;

} sp_per_statistics_t;

extern sp_per_statistics_t sp_per_static;//性能测试统计计数器

typedef struct sp_user_queue_info_tag
{
	sb_s32 num;			//个数
	sp_user_info_queue_t** queue_array;	//数组
}sp_user_queue_info_t;

typedef struct sp_packet_queue_info_tag
{
	sb_s32 num;			//个数
	sp_packet_queue_t** queue_array;	//数组
}sp_packet_queue_info_t;

typedef struct sp_xdr_queue_info_tag
{
	sb_s32 num;			//个数
	sp_xdr_queue_t** queue_array;	//数组
}sp_xdr_queue_info_t;

void exit_sig_parse (void);
sb_s32 alloc_all_queues(void);
void sp_packets_loop (void);
void sp_log_pack_info();

#endif /* SIG_PARSE_H_ */
