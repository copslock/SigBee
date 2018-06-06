/*
 * data_parser/dp_sndcp.h
 *
 */

#ifndef DP_SNDCP_H_
#define DP_SNDCP_H_

extern dp_statistic_t *dp_statistic;

extern sb_s32 parse_sndcp_head(dp_packet_t *dp_packet);
extern sb_s32 get_sndcp_packet_size(dp_packet_t *dp_packet);

#endif /* DP_SNDCP_H_ */

