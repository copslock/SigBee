/*
 * data_parser/dp_define.h
 */

#ifndef DP_DEFINE_H_
#define DP_DEFINE_H_

#include "sb_log.h"

#define u64_to_s(time)	(time & 0xffffffff)
#define u64_to_ns(time)	(time >> 32)

#define u64_to_ms(time) (u64_to_s(time) * 1000 + u64_to_ns(time) / 1000000)

#define union_to_s(time)  (time.capture_time_detail.time_stamp_sec)
#define union_to_ns(time) (time.capture_time_detail.time_stamp_ns)

#define union_to_ms(time) \
	(union_to_s(time) * 1000 + union_to_ns(time) / 1000000)

#define HASH_SIZE		500000
#define SNDCP_HEAD_LEN		4
#define MAX_SNDCP_SEG_SIZE	600
#define NR_SNDCP_SEG		3	/* maximum sndcp segment */
#define MAX_REAS_L4_SIZE	4000	/* maximum reassembled l4 length */
#define NR_TCP_SEG		6	/* maximum tcp segment */
#define MAX_PACKET_LEN		1600	/* maximum packet length */

#define IP_RF		0x8000	/* reserved fragment flag */
#define IP_DF		0x4000	/* dont fragment flag */
#define IP_MF		0x2000	/* more fragments flag */
#define IP_OFFMASK	0x1fff	/* mask for fragmenting bits */

#define TH_FIN        	0x01
#define TH_SYN        	0x02
#define TH_RST        	0x04
#define TH_PUSH       	0x08
#define TH_ACK        	0x10
#define TH_URG        	0x20

#define TCPOPT_EOL		0
#define TCPOPT_NOP		1
#define TCPOPT_MAXSEG		2
#define TCPOLEN_MAXSEG		4
#define TCPOPT_WINDOW		3
#define TCPOLEN_WINDOW		3
#define TCPOPT_SACK_PERMITTED	4	/* Experimental */
#define TCPOLEN_SACK_PERMITTED	2
#define TCPOPT_SACK		5	/* Experimental */
#define TCPOPT_TIMESTAMP	8
#define TCPOLEN_TIMESTAMP	10

#define MIN_IP_HEADER_LEN	20
#define MIN_TCP_HEADER_LEN	20
#define MIN_UDP_HEADER_LEN	8

#define UPLINK		0
#define DOWNLINK	1

// log
#define log_failure(msg, ...)						\
	do {								\
		if (g_adv_config.log_level >= LOG_LEVEL_FAILURE)	\
			write_log(0, 0, msg, ##__VA_ARGS__);		\
	} while (0)

#define log_overload(msg, ...)						\
	do {								\
		if (g_adv_config.log_level >= LOG_LEVEL_OVERLOAD)	\
			write_log(0, 0, msg, ##__VA_ARGS__);		\
	} while (0)

#define log_error(msg, ...)						\
	do {								\
		if (g_adv_config.log_level >= LOG_LEVEL_ERROR)		\
			write_log(0, 0, msg, ##__VA_ARGS__);		\
	} while (0)

#define log_warning(msg, ...)						\
	do {								\
		if (g_adv_config.log_level >= LOG_LEVEL_WARNING)	\
			write_log(0, 0, msg, ##__VA_ARGS__);		\
	} while (0)

#define log_notice(msg, ...)						\
	do {								\
		if (g_adv_config.log_level >= LOG_LEVEL_NOTICE)		\
			write_log(0, 0, msg, ##__VA_ARGS__);		\
	} while (0)

#define log_debug(msg, ...)						\
	do {								\
		if (g_adv_config.log_level >= LOG_LEVEL_DEBUG)		\
			write_log(0, 0, msg, ##__VA_ARGS__);		\
	} while (0)

#define dp_info(msg, ...)	fprintf(stdout, msg, ##__VA_ARGS__)
#define dp_warning(msg, ...)	fprintf(stderr, msg, ##__VA_ARGS__)
#define dp_error(msg, ...)	fprintf(stderr, msg, ##__VA_ARGS__)

#endif /* DP_DEFINE_H_ */

