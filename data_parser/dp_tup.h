/*
 * data_parser/dp_tup.h
 */
#ifndef DP_TUP_H_
#define DP_TUP_H_

#define CDR_ID	0xc000000000000000 
#define PROC_ID	(((sb_u64)g_device_config.device_identity << 32) | g_cpu_id)

extern dp_statistic_t *dp_statistic;

extern sb_s32 parse_tup_head(dp_packet_t *dp_packet);
extern void parse_tcp_session(dp_packet_t *dp_packet);
extern void parse_udp_session(dp_packet_t *dp_packet);
extern void copy_xdr_tup(dp_xdr_t *dp_xdr, xdr_tup_t *src);

#endif /* DP_TUP_H_ */

