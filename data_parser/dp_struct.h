/*
 * data_parser/dp_struct.h
 */

#ifndef DP_STRUCT_H_
#define DP_STRUCT_H_

#include "sb_type.h"
#include "sb_eth_struct.h"


// xdr type
typedef enum {
	XDR_TYPE_TCP,
	XDR_TYPE_UDP,
	XDR_TYPE_HTTP,
	XDR_TYPE_MMS
} xdr_type_t;

// tcp state
typedef enum {
	TCP_STATE_INIT,
	TCP_STATE_SYN,
	TCP_STATE_SYN_ACK,
	TCP_STATE_ESTABLISHED,
	TCP_STATE_RUNNING,
	TCP_STATE_CLOSED
} tcp_state_t;

// TCP segment uplink
typedef struct __dp_tcp_ul_seg {
	sb_u32 valid;
	sb_u32 trans_id;
	sb_u32 seq;
	sb_u32 ack;
	sb_u32 len;
} dp_tcp_ul_seg_t;

// TCP segment dwonlink
typedef struct __dp_tcp_dl_seg {
	sb_u32 valid;
	sb_u32 trans_id;
	sb_u32 seq;
	sb_u32 ack;
	sb_u32 len;
	sb_u8  data[MAX_PACKET_LEN];	// we need a copy of data
} dp_tcp_dl_seg_t;

// TCP session struct
typedef struct _dp_tcp_session {
	sb_u32 seq;
	sb_u32 ack;
	sb_flow_t flow;
	tcp_state_t tcp_state;			// TCP state
	sb_u32 nr_trans;			// number of transaction request
	union sb_time_t visit_time;		// last visit time
	union sb_time_t tcp_syn_time;		// tcp first handshaking
	union sb_time_t tcp_syn_ack_time;	// tcp second handshaking
	union sb_time_t tcp_est_time;		// tcp established
	union sb_time_t tcp_first_req_time;	// first transaction request
	union sb_time_t tcp_first_resp_time;	// first transcation response
	union sb_time_t tcp_closed_time;	// tcp closed
	dp_tcp_ul_seg_t ul_seg[NR_TCP_SEG];	// uplink tcp segment info.
	dp_tcp_dl_seg_t dl_seg[NR_TCP_SEG];	// downlink tcp segment info.
	xdr_tup_t tcp;				// xdr general data transaction

	struct _dp_tcp_session *next;
} dp_tcp_session_t;

// UDP session struct
typedef struct _dp_udp_session {
	sb_flow_t flow;
	union sb_time_t visit_time;		// last visit time
	xdr_tup_t udp;				// xdr general data transaction

	struct _dp_udp_session *next;
} dp_udp_session_t;


typedef struct _dp_sndcp_t {
	sb_u8  pdu_type;
	sb_u8  nsapi;		// NSAPI
	sb_u8  is_first;	//
	sb_u8  is_more;		//
	sb_u8  is_compressed;	//
	sb_u32 n_pdu;		// N-PDU no
	sb_u8  seg_no;
} dp_sndcp_t;

/*
 * sndcp segment info.
 * if seg_len == 0, this info is not uesed.
 */
typedef struct _dp_sndcp_frag {
	sb_u16 seg_len;			// segment length
	sb_u8  seg[MAX_SNDCP_SEG_SIZE];	// segment contents
} dp_sndcp_frag_t;

/*
 * sndcp packet struct
 *
 * if remain_size == 0, all sndcp segments are reassembled.
 */
typedef struct _dp_sndcp_packet {
	sb_u16 valid;
	sb_u16 n_pdu;
	sb_s16 remain_len;			// sndcp reamined length
	dp_sndcp_frag_t frag[NR_SNDCP_SEG];	// sndcp segment
} dp_sndcp_packet_t;

// IP reassemble
typedef struct _dp_ip_packet {
	sb_u16 valid;
	sb_u16 id;
	sb_u16 nr;			// numbers of ip fragment
	sb_u32 reas_len;		// reassembled length
	sb_u32 len;			// total length
	sb_u8  data[MAX_REAS_L4_SIZE];
} dp_ip_packet_t;

typedef struct _dp_packet {
	union sb_time_t ts;			// packet timestamp
	dp_sndcp_t sndcp;			//
	dp_sndcp_packet_t sndcp_packet;		// used for sndcp reassemble
	dp_ip_packet_t ip_packet;		// used for ip reassemble

	dp_user_info_t *user;			// user info.

	sb_u8 packet_direct;			// 0 - up, 1 - down
	sb_flow_t flow;
	dp_tcp_session_t *tcp_session;		// current tcp session
	dp_tcp_session_t *tcp_session_list;
	dp_udp_session_t *udp_session;		// current udp sesion
	dp_udp_session_t *udp_session_list;

	sb_u8  *data;
	sb_u32 len;
	struct _dp_packet *next;
} dp_packet_t;

typedef struct _dp_hash_packet {
	sb_u64	    user_id;
	dp_packet_t dp_packet;

	struct _dp_hash_packet *next;
} dp_hash_packet_t;

typedef struct _dp_hash_user {
	sb_u64 user_id;
	dp_user_info_t user;
	union sb_time_t visit_time;

	struct _dp_hash_user *next;
} dp_hash_user_t;

#endif /* DP_STRUCT_H_ */

