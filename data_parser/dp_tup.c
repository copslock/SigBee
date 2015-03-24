/*
 * data_parser/dp_tup.c
 */
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <endian.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_base_util.h"
#include "sb_base_mem.h"
#include "sb_public_var.h"
#include "sb_xdr_struct.h"

#include "dp_define.h"
#include "dp_struct.h"
#include "dp_tup.h"

static dp_tcp_session_t *get_tcp_session(dp_packet_t *dp_packet);
static dp_udp_session_t *get_udp_session(dp_packet_t *dp_packet);

sb_s32 parse_tup_head(dp_packet_t *dp_packet)
{
	sb_u8  *data;
	sb_u16 ip_off;
	sb_u32 l4_size;
	sb_u32 ip_hdr_size;
	sb_u32 tcp_hdr_size;
	ip_head_t  *ip_hdr;
	tcp_head_t *tcp_hdr;
	udp_head_t *udp_hdr;
	dp_tcp_session_t *ts;
	dp_udp_session_t *us;

	if (!dp_packet) {
		log_warning("received a NULL pointer\n");
		return -1;
	}

	data = dp_packet->data;
	ip_hdr = (ip_head_t *)data;

	if (!data || dp_packet->len < MIN_IP_HEADER_LEN) {
		log_debug("broken dp packet\n");
		return -1;
	}

	if (4 != ip_hdr->ip_v) {
		log_debug("NOT a ipv4 packet\n");
		dp_statistic->nip_packet_count++;
		return -1;
	}

	dp_packet->flow.l4_protocol = ip_hdr->ip_p;
	if (UPLINK == dp_packet->packet_direct) {
		dp_packet->flow.dst_ip = ntohl(ip_hdr->ip_dst);
		dp_packet->flow.src_ip = ntohl(ip_hdr->ip_src);
	} else {
		dp_packet->flow.dst_ip = ntohl(ip_hdr->ip_src);
		dp_packet->flow.src_ip = ntohl(ip_hdr->ip_dst);
	}

	ip_off = ntohs(ip_hdr->ip_off);

	ip_hdr_size = ((sb_u32)ip_hdr->ip_hl) << 2;
	l4_size = ntohs(ip_hdr->ip_len) - ip_hdr_size;
	data += ip_hdr_size;
	dp_packet->data = data;

	if (l4_size > (dp_packet->len - ip_hdr_size)) {
		log_debug("broken packet\n");
		return -1;
	}

	dp_packet->len = l4_size;

	// are we fragmented？
	if((ip_off & IP_MF) || (ip_off & IP_OFFMASK) > 0) {
		dp_ip_packet_t *ip_packet = &dp_packet->ip_packet;
		sb_u32 offset = ((sb_u32)(ip_off & IP_OFFMASK)) << 3;

		log_debug("ip fragment\n");
		if (0 == ip_packet->valid) {
			ip_packet->valid = 1;
			ip_packet->id = ntohs(ip_hdr->ip_id);
			ip_packet->nr = 0;
			ip_packet->reas_len = 0;
			ip_packet->len = 0;
		} else {
			if (ip_hdr->ip_id != ip_packet->id) {
				log_debug("new ip fragment but old ones NOT "
					"reassembled\n");
				ip_packet->id = ip_hdr->ip_id;
				ip_packet->nr = 0;
				ip_packet->reas_len = 0;
				ip_packet->len = 0;
			}
		}
		if (0 == (ip_off & IP_MF)) // last fragment
			ip_packet->len = offset + l4_size;
		ip_packet->reas_len += l4_size;
		if (MAX_REAS_L4_SIZE < offset + l4_size) {
			log_debug("ip reassemble failed, packet too big!\n");
			ip_packet->valid = 0;
			return -1;
		}
		memcpy(ip_packet->data + offset, dp_packet->data, l4_size);
		ip_packet->nr++;

		if (ip_packet->len != ip_packet->reas_len)
			return -1;
		// ip packet reassembled
		dp_packet->data = ip_packet->data;
		dp_packet->len  = ip_packet->len;
	}

	switch (ip_hdr->ip_p) {
	case IPPROTO_TCP:
		ts = get_tcp_session(dp_packet);
		if (!ts) {
			log_debug("unable to get tcp session\n");
			return -1;
		}

		tcp_hdr = (tcp_head_t *)data;
		tcp_hdr_size = ((sb_u32)tcp_hdr->th_off) << 2;
		if (UPLINK == dp_packet->packet_direct) {
			dp_packet->flow.dst_port = ntohs(tcp_hdr->th_dport);
			dp_packet->flow.src_port = ntohs(tcp_hdr->th_sport);
			ts->tcp.user_pub_header.ul_packets++;
			if (dp_packet->ip_packet.valid) {
				ts->tcp.user_pub_header.ul_ip_frag_packets +=
					dp_packet->ip_packet.nr;
				dp_packet->ip_packet.valid = 0;
			}

		} else {
			dp_packet->flow.dst_port = ntohs(tcp_hdr->th_sport);
			dp_packet->flow.src_port = ntohs(tcp_hdr->th_dport);
			ts->tcp.user_pub_header.dl_packets++;
			if (dp_packet->ip_packet.valid) {
				ts->tcp.user_pub_header.dl_ip_frag_packets +=
					dp_packet->ip_packet.nr;
				dp_packet->ip_packet.valid = 0;
			}

		}

		ts->seq = ntohl(tcp_hdr->th_seq);
		ts->ack = ntohl(tcp_hdr->th_ack);

		// set tcp state
		if (TCP_STATE_SYN_ACK == ts->tcp_state)
			ts->tcp_state = TCP_STATE_ESTABLISHED;

		if (tcp_hdr->th_flags & TH_SYN) {	// syn
			if (tcp_hdr->th_flags & TH_ACK)	// ack
				ts->tcp_state = TCP_STATE_SYN_ACK;
			else
				ts->tcp_state = TCP_STATE_SYN;
		}

		// fin || rst
		if ((tcp_hdr->th_flags & TH_FIN)
				|| (tcp_hdr->th_flags & TH_RST))
			ts->tcp_state = TCP_STATE_CLOSED;

		// tcp window size, may be recalculated in tcp options
		ts->tcp.windows_size = ntohs(tcp_hdr->th_win);

		// TCP options
		if (TCP_STATE_SYN_ACK == ts->tcp_state && tcp_hdr_size > 20) {
			sb_u8  *opt = data + 20;
			sb_u16 *value;
			sb_s32 i = 0;
			sb_s32 opt_len = tcp_hdr_size - 20;

			while (opt_len > 0 && 2 != i) {
				switch (*opt) {
				case TCPOPT_MAXSEG: // mss
					value = (sb_u16 *)(opt + 2);
					ts->tcp.mss_size = ntohs(*value);
					opt += TCPOLEN_MAXSEG;
					opt_len -= TCPOLEN_MAXSEG;
					i++;
					break;
				case TCPOPT_WINDOW: // window scale
					ts->tcp.windows_size <<= opt[2];
					opt += TCPOLEN_WINDOW;
					opt_len -= TCPOLEN_WINDOW;
					i++;
					break;
				case TCPOPT_EOL: // eof
					i = 2;
					break;
				case TCPOPT_NOP: // nop
					opt += 1;
					opt_len -= 1;
					break;
				default:
					opt_len -= opt[1];
					opt += opt[1];
					break;
				}
			}
		}

		dp_packet->data = data + tcp_hdr_size;
		dp_packet->len -= tcp_hdr_size;

		dp_statistic->tcp_packet_count++;
		dp_statistic->tcp_packet_size += dp_packet->len;

		break;
	case IPPROTO_UDP:
		us = get_udp_session(dp_packet);
		if (!us) {
			log_debug("unable to get a udp session\n");
			return -1;
		}

		udp_hdr = (udp_head_t *)data;
		if (0 == dp_packet->packet_direct) {	// uplink
			dp_packet->flow.dst_port = ntohs(udp_hdr->uh_dport);
			dp_packet->flow.src_port = ntohs(udp_hdr->uh_sport);
			us->udp.user_pub_header.ul_packets++;
			if (dp_packet->ip_packet.valid) {
				us->udp.user_pub_header.ul_ip_frag_packets +=
					dp_packet->ip_packet.nr;
				dp_packet->ip_packet.valid = 0;
			}

		} else {
			dp_packet->flow.dst_port = ntohs(udp_hdr->uh_sport);
			dp_packet->flow.src_port = ntohs(udp_hdr->uh_dport);
			us->udp.user_pub_header.dl_packets++;
			if (dp_packet->ip_packet.valid) {
				us->udp.user_pub_header.dl_ip_frag_packets +=
					dp_packet->ip_packet.nr;
				dp_packet->ip_packet.valid = 0;
			}

		}

		dp_packet->data = data + 8;
		dp_packet->len -= 8;

		dp_statistic->udp_packet_count++;
		dp_statistic->udp_packet_size += dp_packet->len;

		break;
	default:
		log_debug("NOT tcp or udp\n");
		dp_statistic->ntcpudp_packet_count++;
		return -1;
		break;
	}

	return 0;
}

static dp_tcp_session_t *get_tcp_session(dp_packet_t *dp_packet)
{
	struct in_addr in;
	dp_tcp_session_t *ts;

	if (!dp_packet || !dp_packet->tcp_session_list) {
		log_warning("called unexpectively\n");
		return NULL;
	}

	ts = dp_packet->tcp_session_list;

	while (ts) {
		if (UPLINK == dp_packet->packet_direct
				&& dp_packet->flow.dst_ip == ts->flow.dst_ip
				&& dp_packet->flow.src_ip == ts->flow.src_ip
				&& dp_packet->flow.dst_port == ts->flow.dst_port
				&& dp_packet->flow.src_port == ts->flow.src_port) {
			log_debug("hit tcp session\n");
			ts->visit_time.capture_time_total = g_current_time;
			dp_packet->tcp_session = ts;

			return ts;
		} else if (DOWNLINK == dp_packet->packet_direct
				&& dp_packet->flow.dst_ip == ts->flow.src_ip
				&& dp_packet->flow.src_ip == ts->flow.dst_ip
				&& dp_packet->flow.dst_port == ts->flow.src_port
				&& dp_packet->flow.src_port == ts->flow.dst_port) {
			log_debug("hit tcp session\n");
			ts->visit_time.capture_time_total = g_current_time;
			dp_packet->tcp_session = ts;

			return ts;
		}

		ts = ts->next;
	}

	// here, ts is NULL at first or at end of tcp session list（NULL too!)
	log_debug("create a new tcp session\n");
	ts = check_malloc("get_tcp_session", 1, sizeof(dp_tcp_session_t));
	if (!ts) {
		log_error("unable to create a new tcp session\n");
		return NULL;
	}

	// initialize tcp session struct, most of members have been zeroed
	// by check_malloc.
	ts->flow = dp_packet->flow;
	ts->tcp_state = TCP_STATE_INIT;
	ts->visit_time.capture_time_total = g_current_time;
	ts->tcp_syn_time.capture_time_total = g_current_time;
	ts->tcp_syn_ack_time.capture_time_total = g_current_time;
	ts->tcp_est_time.capture_time_total = g_current_time;
	ts->tcp_first_req_time.capture_time_total = g_current_time;
	ts->tcp_first_resp_time.capture_time_total = g_current_time;
	ts->tcp_closed_time.capture_time_total = g_current_time;

	// initialize xdr_tup struct
	// header
	ts->tcp.header.len = sizeof(xdr_tup_t);
	ts->tcp.header.type = 3;	// CDR/TDR(Gb/IuPS)
	ts->tcp.header.version = 0x10;
	ts->tcp.header.var_ie_num = 1;
	ts->tcp.header.proto_type = 0x1001;
	ts->tcp.header.id = CDR_ID;
	// pub_header
	memcpy(&ts->tcp.user_pub_header.pub_header, dp_packet->user,
	       sizeof(xdr_pub_header_t));
	// user_pub_header
	ts->tcp.user_pub_header.procedure_type = 100;
	ts->tcp.user_pub_header.procedure_id = PROC_ID;
	ts->tcp.user_pub_header.start_time = u64_to_ms(g_current_time);
	ts->tcp.user_pub_header.l4_protocol = 0;	// tcp
	in.s_addr = ts->flow.src_ip;
	strcpy((sb_s8 *)ts->tcp.user_pub_header.user_ip, inet_ntoa(in));
	ts->tcp.user_pub_header.user_port = ts->flow.src_port;
	in.s_addr = ts->flow.dst_ip;
	strcpy((sb_s8 *)ts->tcp.user_pub_header.server_ip, inet_ntoa(in));
	ts->tcp.user_pub_header.server_port = ts->flow.dst_port;
	in.s_addr = dp_packet->user->sgsn_sig_ip;
	strcpy((sb_s8 *)ts->tcp.user_pub_header.sgsn_user_ip, inet_ntoa(in));
	in.s_addr = dp_packet->user->bsc_sig_ip;
	strcpy((sb_s8 *)ts->tcp.user_pub_header.bsc_user_ip, inet_ntoa(in));

	ts->next = dp_packet->tcp_session_list;
	dp_packet->tcp_session_list = ts;
	dp_packet->tcp_session = ts;

	return ts;
}

static dp_udp_session_t *get_udp_session(dp_packet_t *dp_packet)
{
	struct in_addr in;
	dp_udp_session_t *us;

	if (!dp_packet || !dp_packet->udp_session_list) {
		log_warning("called unexpectively\n");
		return NULL;
	}

	us = dp_packet->udp_session_list;

	while (us) {
		if (0 == dp_packet->packet_direct
				&& dp_packet->flow.dst_ip == us->flow.dst_ip
				&& dp_packet->flow.src_ip == us->flow.src_ip
				&& dp_packet->flow.dst_port == us->flow.dst_port
				&& dp_packet->flow.src_port == us->flow.src_port) {
			log_debug("hit udp session\n");
			us->visit_time.capture_time_total = g_current_time;
			dp_packet->udp_session = us;

			return us;
		} else if (1 == dp_packet->packet_direct
				&& dp_packet->flow.dst_ip == us->flow.src_ip
				&& dp_packet->flow.src_ip == us->flow.dst_ip
				&& dp_packet->flow.dst_port == us->flow.src_port
				&& dp_packet->flow.src_port == us->flow.dst_port) {
			log_debug("hit udp session\n");
			us->visit_time.capture_time_total = g_current_time;
			dp_packet->udp_session = us;

			return us;
		}

		us = us->next;
	}

	// here, us is NULL at first or at end of udp session list（NULL too!)
	log_debug("create a new udp session\n");
	us = check_malloc("get_udp_session", 1, sizeof(dp_udp_session_t));
	if (!us) {
		log_error("unable to create a new udp session\n");
		return NULL;
	}

	// initialize udp session struct, most of members have been zeroed
	// by check_malloc.
	us->flow = dp_packet->flow;
	us->visit_time.capture_time_total = g_current_time;

	// initialize xdr_tup struct
	// header
	us->udp.header.len = sizeof(xdr_tup_t);
	us->udp.header.type = 3;	// CDR/TDR(Gb/IuPS)
	us->udp.header.version = 0x10;
	us->udp.header.var_ie_num = 1;
	us->udp.header.proto_type = 0x1001;
	us->udp.header.id = CDR_ID;
	// pub_header
	memcpy(&us->udp.user_pub_header.pub_header, dp_packet->user,
	       sizeof(xdr_pub_header_t));
	// user_pub_header
	us->udp.user_pub_header.procedure_type = 100;
	us->udp.user_pub_header.procedure_id = PROC_ID;
	us->udp.user_pub_header.start_time = u64_to_ms(g_current_time);
	us->udp.user_pub_header.l4_protocol = 1;	// udp
	in.s_addr = us->flow.src_ip;
	strcpy((sb_s8 *)us->udp.user_pub_header.user_ip, inet_ntoa(in));
	us->udp.user_pub_header.user_port = us->flow.src_port;
	in.s_addr = us->flow.dst_ip;
	strcpy((sb_s8 *)us->udp.user_pub_header.server_ip, inet_ntoa(in));
	us->udp.user_pub_header.server_port = us->flow.dst_port;
	in.s_addr = dp_packet->user->sgsn_sig_ip;
	strcpy((sb_s8 *)us->udp.user_pub_header.sgsn_user_ip, inet_ntoa(in));
	in.s_addr = dp_packet->user->bsc_sig_ip;
	strcpy((sb_s8 *)us->udp.user_pub_header.bsc_user_ip, inet_ntoa(in));

	us->next = dp_packet->udp_session_list;
	dp_packet->udp_session_list = us;
	dp_packet->udp_session = us;

	return us;
}

static void parse_tcp_session_ul(dp_packet_t *dp_packet)
{
	sb_s32 i;
	sb_s32 seg_idx = -1;
	dp_tcp_session_t *ts;

	if (!dp_packet || !dp_packet->tcp_session) {
		log_warning("called unexpectively\n");
		return;
	}

	ts = dp_packet->tcp_session;

	// is it retransmission or out-of-order?
	for (i = 0; i < NR_TCP_SEG; i++) {
		if (ts->ul_seg[i].valid) {
			if (ts->ul_seg[i].seq == ts->seq
					&& ts->ul_seg[i].ack == ts->ack) {
				// Yes!
				log_debug("uplink tcp retransmission\n");
				ts->tcp.user_pub_header.ul_retrans_packets++;
				return;
			} else if (ts->ul_seg[i].seq > ts->seq) {
				// out-of-order
				log_debug("uplink tcp out of order\n");
				ts->tcp.user_pub_header.ul_reorder_packets++;
			}
			continue;
		}
		seg_idx = i;
	}

	//
	if (dp_packet->len) {
		ts->nr_trans++;
		ts->tcp.user_pub_header.ul_bytes += dp_packet->len;

		if (-1 == seg_idx) {
			log_warning("uplink tcp segment exceeded\n");
			return;
		}

		// first tcp transaction request
		if (1 == ts->nr_trans) {
			ts->tcp_first_req_time.capture_time_total =
				g_current_time;
		}
		ts->ul_seg[seg_idx].valid = 1;
		ts->ul_seg[seg_idx].trans_id = ts->nr_trans;
		ts->ul_seg[seg_idx].seq = ts->seq;
		ts->ul_seg[seg_idx].ack = ts->ack;
		ts->ul_seg[seg_idx].len = dp_packet->len;
	}

	// free downlink segment info.
	for (i = 0; i < NR_TCP_SEG; i++) {
		if (0 == ts->dl_seg[i].valid)
			continue;

		if ((ts->dl_seg[i].seq + ts->dl_seg[i].len) <= ts->ack) {
			if (1 == ts->dl_seg[i].trans_id) {
				ts->tcp_first_resp_time.capture_time_total =
					g_current_time;
			}
			ts->dl_seg[i].valid = 0;
		}
	}

}

static void parse_tcp_session_dl(dp_packet_t *dp_packet)
{
	sb_s32 i;
	sb_s32 seg_idx = -1;
	dp_tcp_session_t *ts;

	if (!dp_packet || !dp_packet->tcp_session) {
		log_warning("called unexpectively\n");
		return;
	}

	ts = dp_packet->tcp_session;

	// is it retransmission or out-of-order?
	for (i = 0; i < NR_TCP_SEG; i++) {
		if (ts->dl_seg[i].valid) {
			if (ts->dl_seg[i].seq == ts->seq
					&& ts->dl_seg[i].ack == ts->ack) {
				// Yes!
				log_debug("downlink tcp retransmission\n");
				ts->tcp.user_pub_header.dl_retrans_packets++;
				return;
			} else if (ts->dl_seg[i].seq > ts->seq) {
				// out-of-order
				log_debug("downlink tcp out of order\n");
				ts->tcp.user_pub_header.dl_reorder_packets++;
			}
			continue;
		}
		seg_idx = i;
	}

	//
	if (dp_packet->len) {
		ts->nr_trans++;
		ts->tcp.user_pub_header.dl_bytes += dp_packet->len;

		if (-1 == seg_idx) {
			log_warning("downlink tcp segment exceeded\n");
			return;
		}

		if (1 == ts->nr_trans) {
			ts->tcp_first_req_time.capture_time_total =
				g_current_time;
		}

		ts->dl_seg[seg_idx].valid = 1;
		ts->dl_seg[seg_idx].trans_id = ts->nr_trans;
		ts->dl_seg[seg_idx].seq = ts->seq;
		ts->dl_seg[seg_idx].ack = ts->ack;
		ts->dl_seg[seg_idx].len = dp_packet->len;
		// we need a copy of data for tcp reassemble
		memcpy(ts->dl_seg[seg_idx].data, dp_packet->data,
				dp_packet->len);
	}

	// free uplink segment info.
	for (i = 0; i < NR_TCP_SEG; i++) {
		if (ts->ul_seg[i].valid)
			continue;

		if ((ts->ul_seg[i].seq + ts->ul_seg[i].len) <= ts->ack) {
			if (1 == ts->ul_seg[i].trans_id) {
				ts->tcp_first_resp_time.capture_time_total =
					g_current_time;
			}
			ts->ul_seg[i].valid = 0;
		}
	}

}

void parse_tcp_session(dp_packet_t *dp_packet)
{
	if (!dp_packet) {
		log_debug("received a NULL pointer\n");
		return;
	}

	if (UPLINK == dp_packet->packet_direct)
		parse_tcp_session_ul(dp_packet);
	else
		parse_tcp_session_dl(dp_packet);
}

static void parse_udp_session_ul(dp_packet_t *dp_packet)
{
	dp_udp_session_t *us;

	if (!dp_packet || !dp_packet->udp_session) {
		log_warning("called unexpectively\n");
		return;
	}

	us = dp_packet->udp_session;

	us->udp.user_pub_header.dl_bytes += dp_packet->len;
}

static void parse_udp_session_dl(dp_packet_t *dp_packet)
{
	dp_udp_session_t *us;

	if (!dp_packet || !dp_packet->udp_session) {
		log_warning("called unexpectively\n");
		return;
	}

	us = dp_packet->udp_session;

	us->udp.user_pub_header.dl_bytes += dp_packet->len;
}

void parse_udp_session(dp_packet_t *dp_packet)
{
	if (!dp_packet) {
		log_debug("received a NULL pointer\n");
		return;
	}

	if (0 == dp_packet->packet_direct)
		parse_udp_session_ul(dp_packet);
	else
		parse_udp_session_dl(dp_packet);
}

void copy_xdr_tup(dp_xdr_t *dp_xdr, xdr_tup_t *src)
{
	xdr_tup_t *dst;

	if (!dp_xdr || !src) {
		log_debug("received a NULL pointer\n");
		return;
	}

	dp_xdr->length = sizeof(xdr_tup_t);
	dst = (xdr_tup_t *)dp_xdr->data;

	*dst = *src;

	// header
	dst->header.len = htons(src->header.len);
	dst->header.var_ie_num = htons(src->header.var_ie_num);
	dst->header.proto_type = htons(src->header.proto_type);
	dst->header.id = htobe64(src->header.id);

	// pub header
	dst->user_pub_header.pub_header.ptmsi =
		htonl(src->user_pub_header.pub_header.ptmsi);
	dst->user_pub_header.pub_header.mcc =
		htonl(src->user_pub_header.pub_header.mcc);
	dst->user_pub_header.pub_header.mnc =
		htonl(src->user_pub_header.pub_header.mnc);
	dst->user_pub_header.pub_header.lac =
		htonl(src->user_pub_header.pub_header.lac);
	dst->user_pub_header.pub_header.rac =
		htonl(src->user_pub_header.pub_header.rac);
	dst->user_pub_header.pub_header.cid =
		htonl(src->user_pub_header.pub_header.cid);
	dst->user_pub_header.pub_header.old_mcc =
		htonl(src->user_pub_header.pub_header.old_mcc);
	dst->user_pub_header.pub_header.old_mnc =
		htonl(src->user_pub_header.pub_header.old_mnc);
	dst->user_pub_header.pub_header.old_lac =
		htonl(src->user_pub_header.pub_header.old_lac);
	dst->user_pub_header.pub_header.old_rac =
		htonl(src->user_pub_header.pub_header.old_rac);
	dst->user_pub_header.pub_header.old_cid =
		htonl(src->user_pub_header.pub_header.old_cid);
	dst->user_pub_header.pub_header.sgsn_sig_ip =
		htonl(src->user_pub_header.pub_header.sgsn_sig_ip);
	dst->user_pub_header.pub_header.bsc_sig_ip =
		htonl(src->user_pub_header.pub_header.bsc_sig_ip);

	// user_pub_header
	dst->user_pub_header.procedure_id =
		htobe64(dst->user_pub_header.procedure_id);
	dst->user_pub_header.start_time =
		htobe64(dst->user_pub_header.start_time);
	dst->user_pub_header.end_time =
		htobe64(dst->user_pub_header.end_time);
	dst->user_pub_header.app_type =
		htons(src->user_pub_header.app_type);
	dst->user_pub_header.app_subtype =
		htons(src->user_pub_header.app_subtype);
	dst->user_pub_header.user_port =
		htons(src->user_pub_header.user_port);
	dst->user_pub_header.server_port =
		htons(src->user_pub_header.server_port);
	dst->user_pub_header.ul_bytes =
		htonl(src->user_pub_header.ul_bytes);
	dst->user_pub_header.dl_bytes =
		htonl(src->user_pub_header.dl_bytes);
	dst->user_pub_header.ul_packets =
		htonl(src->user_pub_header.ul_packets);
	dst->user_pub_header.dl_packets =
		htonl(src->user_pub_header.dl_packets);
	dst->user_pub_header.ul_reorder_packets =
		htonl(src->user_pub_header.ul_reorder_packets);
	dst->user_pub_header.dl_reorder_packets =
		htonl(src->user_pub_header.dl_reorder_packets);
	dst->user_pub_header.ul_retrans_packets =
		htonl(src->user_pub_header.ul_retrans_packets);
	dst->user_pub_header.dl_retrans_packets =
		htonl(src->user_pub_header.dl_retrans_packets);
	dst->user_pub_header.ul_ip_frag_packets =
		htonl(src->user_pub_header.ul_ip_frag_packets);
	dst->user_pub_header.dl_ip_frag_packets =
		htonl(src->user_pub_header.dl_ip_frag_packets);

	// the rest
	dst->tcp_create_res_delay = htonl(src->tcp_create_res_delay);
	dst->tcp_create_ack_delay = htonl(src->tcp_create_ack_delay);
	dst->tcp_first_req_delay = htonl(src->tcp_first_req_delay);
	dst->tcp_first_res_delay = htonl(src->tcp_first_res_delay);
	dst->windows_size = htonl(src->windows_size);
	dst->mss_size = htonl(src->mss_size);
}

