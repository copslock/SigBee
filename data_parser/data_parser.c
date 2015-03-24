/*
 * data_parser/data_parser.c
 *
 */

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>

#include <tmc/cpus.h>
#include <tmc/task.h>
#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_xdr_struct.h"
#include "sb_base_util.h"
#include "sb_public_var.h"
#include "sb_base_mem.h"

#include "dp_define.h"
#include "dp_struct.h"
#include "dp_sndcp.h"
#include "data_parser.h"
#include "dp_tup.h"

/* queues we need to handle */
static sp_packet_queue_t *dp_sp_packet_queue;
static sp_user_info_queue_t *dp_sp_user_queue;
static dp_xdr_queue_t *dp_xdr_queue;
static dp_user_info_queue_t *dp_user_queue;

// hash table
dp_hash_packet_t *dp_hash_packet[HASH_SIZE];
dp_hash_user_t *dp_hash_user[HASH_SIZE];

// data parser statistics
dp_statistic_t *dp_statistic;

// function declaration
static void exit_data_parser(void);
static void process_queues(void);
static void parse_gb_packet(sp_packet_info_t *sp_packet);
static void hash_timeout(void);
static dp_packet_t *get_hash_dp_packet(sb_u64 user_id);
static dp_user_info_t *get_hash_dp_user(sb_u64 user_id);
static void put_into_xdr_queue(dp_packet_t *dp_packet, xdr_type_t type);
static void flush_statistics();

#define is_queue_head_invalid(queue_head, workers) \
	(!(queue_head) || (queue_head)->num < (g_process_order + 1) * workers)

void data_parser_main(void)
{
	sb_s32 i;
	sb_s8 ver_info[64];
	sb_s8 ver_time[32];

	if (0 > tmc_cmem_init(0)) {
		log_error("%s can not join cmem\n", g_process_name);
		return;
	}

	ver_info[0] = 0;
#ifdef	DEBUG_VER
	strcpy(ver_info, "Debug ");
#endif

#ifdef	BUILD_DATE
	sprintf(ver_time, "Build %u.%u ", BUILD_DATE, BUILD_TIME);
	strcat(ver_info, ver_time);
#endif
	dp_info("%s version: %s %s\n", g_process_name, EXE_BUILD, ver_info);

	atexit(exit_data_parser);

	if (is_queue_head_invalid(g_sp_packet_queue_head, NR_SP_WORKERS)) {
		log_error("bad g_sp_packet_queue_head\n");
		return;
	}

	if (is_queue_head_invalid(g_sp_user_queue_head, NR_SP_WORKERS)) {
		log_error("bad g_sp_user_queue_head\n");
		return;
	}

	if (is_queue_head_invalid(g_dp_xdr_queue_head, NR_XDR_WORKERS)) {
		log_error("bad g_dp_xdr_queue_head\n");
		return;
	}

	if (is_queue_head_invalid(g_dp_user_queue_head, NR_DPI_WORKERS)) {
		log_error("g_dp_user_queue_head\n");
		return;
	}

	dp_sp_packet_queue = INIT_DP_SP_PACKET_QUEUE;
	dp_sp_user_queue = INIT_DP_SP_USER_QUEUE;
	dp_xdr_queue = INIT_DP_XDR_QUEUE;
	dp_user_queue = INIT_DP_USER_QUEUE;

	// is this ok?
	// how to check g_dp_statistics?
	dp_statistic = g_dp_statistics + g_process_order;
	dp_statistic->cpu_id = g_cpu_id;

	// initialize hash table
	for (i = 0; i < HASH_SIZE; i++) {
		dp_hash_packet[i] = NULL;
		dp_hash_user[i] = NULL;
	}

	while (1)
		sleep(10);
	//process_queues();
}

static void free_dp_packet(dp_packet_t *dp_packet)
{
	dp_tcp_session_t *ts;
	dp_tcp_session_t *ts_next;
	dp_udp_session_t *us;
	dp_udp_session_t *us_next;

	if (!dp_packet)
		return;

	ts = dp_packet->tcp_session_list;
	while (ts) {
		ts_next = ts->next;
		check_free((void **)&ts);
		ts = ts_next;
		if (ts)
			ts_next = ts->next;
	}

	us = dp_packet->udp_session_list;
	while (us) {
		us_next = us->next;
		check_free((void **)&us);
		us = us_next;
		if (us)
			us_next = us->next;
	}
}

static void exit_data_parser(void)
{
	sb_s32 i;
	dp_hash_packet_t *hash_packet;
	dp_hash_packet_t *hash_packet_next;
	dp_hash_user_t *hash_user;
	dp_hash_user_t *hash_user_next;

	dp_info("goodbye data parser\n");

	for (i = 0; i < HASH_SIZE; i++) {

		hash_packet = dp_hash_packet[i];
		while (hash_packet) {
			hash_packet_next = hash_packet->next;
			free_dp_packet(&hash_packet->dp_packet);
			check_free((void **)&hash_packet);
			hash_packet = hash_packet_next;
			if (hash_packet)
				hash_packet_next = hash_packet->next;
		}

		hash_user = dp_hash_user[i];
		while (hash_user) {
			hash_user_next = hash_user->next;
			check_free((void **)&hash_user);
			hash_user = hash_user_next;
			if (hash_user)
				hash_user_next = hash_user->next;
		}
	}
}

/* macros for manipulating queues */
#define inc_queue_head(queue) \
	(queue)->head = ((queue)->head + 1) % (queue)->size
#define inc_queue_tail(queue) \
	(queue)->tail = ((queue)->tail + 1) % (queue)->size
#define inc_queue_push_idx(queue) \
	(queue)->push_index++
#define inc_queue_pop_idx(queue) \
	(queue)->pop_index++
#define is_queue_full(queue)	\
	((queue)->push_index - (queue)->pop_index >= (queue)->size)
#define is_queue_empty(queue)	\
	((queue)->pop_index >= (queue)->push_index)

static void process_packet_queue()
{
	sp_packet_queue_t *sp_packet_queue;
	sp_packet_info_t  *sp_packet;
	static sb_u8 queue_idx = 0;

	sp_packet_queue = dp_sp_packet_queue + (queue_idx % NR_SP_WORKERS);

	if (is_queue_empty(sp_packet_queue)) {
		log_notice("sp_packet_queue is empty: %lu - %lu\n",
				sp_packet_queue->push_index,
				sp_packet_queue->pop_index);
		cycle_relax();
		queue_idx++;
		return;
	} else {
		sp_packet = &sp_packet_queue->sp_packet[sp_packet_queue->head];

		dp_statistic->packet_count++;
		dp_statistic->packet_size +=
			gxio_mpipe_idesc_get_l2_length(&sp_packet->idesc);

		switch (sp_packet->interface) {
		case GN_INT:
			break;
		case IUPS_INT:
			break;
		case GB_INT:
			parse_gb_packet(sp_packet);
			break;
		case S1MME_INT:
			break;
		case S6A_INT:
			break;
		case S11_INT:
			break;
		case S10_INT:
			break;
		case S1U_INT:
			break;
		case S5_INT:
			break;
		default:
			log_warning("unknown interface %hhu\n",
					sp_packet->interface);
			break;
		}
	}
	gxio_mpipe_iqueue_drop(sp_packet->iqueue, &sp_packet->idesc);
	queue_idx++;
	inc_queue_head(sp_packet_queue);
	inc_queue_pop_idx(sp_packet_queue);
}

/* process user info queue */
static void process_user_queue()
{
	sp_user_info_queue_t *sp_user_queue;
	sp_user_info_t *sp_user;
	dp_user_info_t *dp_user;
	static sb_u8 queue_idx = 0;

	sp_user_queue = dp_sp_user_queue + (queue_idx % NR_SP_WORKERS);

	if (is_queue_empty(sp_user_queue)) {
		log_notice("sp_user_queue is empty: %lu - %lu)\n",
			   sp_user_queue->push_index,
			   sp_user_queue->pop_index);
		cycle_relax();
		queue_idx++;
		return;
	} else {
		sp_user = &sp_user_queue->user_infos[sp_user_queue->head];

		dp_user = get_hash_dp_user(sp_user->user_id);
		if (!dp_user) {
			log_error("unable to get dp hash user\n");
			return;
		}

		memcpy(dp_user, &sp_user->interface, sizeof(dp_user_info_t));

		// put into dp_user_info_queue
		sb_s32 i;
		dp_user_info_queue_t *user_queue;

		for (i = 0; i < NR_DPI_WORKERS; i++) {
			user_queue = dp_user_queue + i;
			if (is_queue_full(user_queue)) {
				log_warning("dp_user_info_queue[%d] is full\n", i);
				continue;
			}

			memcpy(&user_queue->user_infos[user_queue->tail],
			       dp_user, sizeof(dp_user_info_t));
			inc_queue_tail(user_queue);
			inc_queue_push_idx(user_queue);
			break;
		}
	}

	queue_idx++;
	inc_queue_head(sp_user_queue);
	inc_queue_pop_idx(sp_user_queue);
}

#define is_need_flush(time) \
	(u64_to_s(g_current_time) - u64_to_s(time) >= 1800)

static void process_queues(void)
{
	sb_u32 start_cycle_count;
	sb_u32 end_cycle_count;
	sb_u32 process_cycle_count;
	sb_u64 last_statistics_flush_time;

	last_statistics_flush_time = g_current_time;

	while (0 == *g_process_exit) {
		start_cycle_count = get_cycle_count();

		// put before process_packet_queue
		// we need user info.
		process_user_queue();
		process_packet_queue();

		end_cycle_count = get_cycle_count();
		process_cycle_count = end_cycle_count - start_cycle_count;

		dp_statistic->total_process_time += process_cycle_count;
		if (dp_statistic->max_process_time < process_cycle_count)
			dp_statistic->max_process_time = process_cycle_count;
		else if (dp_statistic->min_process_time > process_cycle_count)
			dp_statistic->min_process_time = process_cycle_count;

		// 3 minutes
		if (is_need_flush(last_statistics_flush_time)) {
			flush_statistics();
			last_statistics_flush_time = g_current_time;
		}
	}
}

static void parse_gb_packet(sp_packet_info_t *sp_packet)
{
	dp_packet_t *dp_packet;
	sb_u8  packet[NR_SNDCP_SEG * MAX_SNDCP_SEG_SIZE];
	sb_s32 ret;

	if (!sp_packet) {
		log_warning("received a NULL pointer!\n");
		return;
	}

	log_debug("parsing gb packet...\n");

	dp_packet = get_hash_dp_packet(sp_packet->user_id);
	if (!dp_packet) {
		log_error("unable to get hash dp packet\n");
		return;
	}

	dp_packet->data = (sb_u8 *)gxio_mpipe_idesc_get_va(&sp_packet->idesc);
	if (!dp_packet->data) {
		log_error("unable to get mpipe data\n");
		return;
	}

	// set packet timestamp
	dp_packet->ts.capture_time_detail.time_stamp_sec =
		sp_packet->idesc.time_stamp_sec;
	dp_packet->ts.capture_time_detail.time_stamp_ns  =
		sp_packet->idesc.time_stamp_ns;

	dp_packet->data += sp_packet->offset;
	dp_packet->len  = sp_packet->size; /* make sure size is good one */
	dp_packet->packet_direct = sp_packet->packet_direct;

	ret = parse_sndcp_head(dp_packet);
	if(ret) {
		log_debug("unable to parse sndcp head\n");
		return;
	}

	if (1 != dp_packet->sndcp.pdu_type || dp_packet->sndcp.is_compressed) {
		log_debug("confirmed or compressed sndcp\n");
		return;
	}

	// need reassemble?
	if (dp_packet->sndcp.seg_no || dp_packet->sndcp.is_more) {
		sb_s32 i;
		sb_u8  *src;
		sb_u8  seg_no = dp_packet->sndcp.seg_no;
		sb_u16 old_n_pdu = dp_packet->sndcp_packet.n_pdu;
		sb_u16 new_n_pdu = dp_packet->sndcp.n_pdu;
		dp_sndcp_packet_t *sndcp_packet = &dp_packet->sndcp_packet;

		log_debug("sndcp packet need reassemble\n");

		// this should be enough(3 * 600 = 1800)
		if (seg_no >= NR_SNDCP_SEG
				|| dp_packet->len > MAX_SNDCP_SEG_SIZE) {
			log_debug("sndcp segment too big: %hhu: %u\n", seg_no,
				dp_packet->len);
			memset(sndcp_packet, 0, sizeof(dp_sndcp_packet_t));
			return;
		}

		if (sndcp_packet->valid) {
			if (new_n_pdu != old_n_pdu) {
				log_debug("new sndcp but old ones NOT "
						"reassembled\n");
				memset(sndcp_packet, 0, sizeof(*sndcp_packet));
				sndcp_packet->valid = 1;
				sndcp_packet->n_pdu = new_n_pdu;
			}
		} else {
			sndcp_packet->valid = 1;
			sndcp_packet->n_pdu = new_n_pdu;
		}

		// we can get total length of the sndcp packet from
		// first sndcp frgament
		if (dp_packet->sndcp.is_first) {
			ret = get_sndcp_packet_size(dp_packet);
			if (-1 == ret) {
				log_debug("unable to get sndcp packet size\n");
				memset(sndcp_packet, 0, sizeof(*sndcp_packet));
				return;
			}
			sndcp_packet->remain_len += ret;
		}

		sndcp_packet->remain_len -= dp_packet->len;
		sndcp_packet->frag[seg_no].seg_len = dp_packet->len;
		memcpy(sndcp_packet->frag[seg_no].seg,
				dp_packet->data, dp_packet->len);

		// finished?
		if (sndcp_packet->remain_len)
			return;

		// make packet
		dp_packet->data = packet;
		dp_packet->len = 0;
		for (i = 0, src = packet; i < NR_SNDCP_SEG; i++) {
			if (0 == sndcp_packet->frag[i].seg_len)
				break;
			memcpy(src, sndcp_packet->frag[i].seg,
			       sndcp_packet->frag[i].seg_len);
			src += sndcp_packet->frag[i].seg_len;
			dp_packet->len += sndcp_packet->frag[i].seg_len;

			log_debug("sndcp segment:\n%d: %hu\n", i,
				 sndcp_packet->frag[i].seg_len);
		}

		memset(sndcp_packet, 0, sizeof(dp_sndcp_packet_t));
	}

	// get user info.
	dp_packet->user = get_hash_dp_user(sp_packet->user_id);
	if (!dp_packet->user) {
		log_debug("unable to get dp hash user\n");
		return;
	}

	if (g_device_config.dpi_flag)
		if0_1_send_packet(dp_packet);

	// parse tcp/udp head
	ret = parse_tup_head(dp_packet);
	if (-1 == ret)
		return;

	if (IPPROTO_TCP == dp_packet->flow.l4_protocol) {
		dp_tcp_session_t *ts = dp_packet->tcp_session;

		switch (ts->tcp_state) {
		case TCP_STATE_RUNNING:
			parse_tcp_session(dp_packet);

			break;
		case TCP_STATE_ESTABLISHED:
			ts->tcp_est_time.capture_time_total = g_current_time;
			ts->tcp.tcp_create_status = 0;

			break;
		case TCP_STATE_SYN:
			ts->tcp_syn_time.capture_time_total = g_current_time;
			ts->tcp.tcp_create_try_times++;

			break;
		case TCP_STATE_SYN_ACK:
			ts->tcp_syn_ack_time.capture_time_total = g_current_time;

			break;
		case TCP_STATE_CLOSED:
			ts->tcp_closed_time.capture_time_total = g_current_time;
			put_into_xdr_queue(dp_packet, XDR_TYPE_TCP);

			break;
		default:
			log_warning("unknown tcp state\n");
			return;
			break;
		}
	} else {
		parse_udp_session(dp_packet);
	}
	hash_timeout();
}

#define USER_TIMEOUT	g_adv_config.user_timeout
#define is_user_timeout(visit_time) \
	(u64_to_s(g_current_time) - union_to_s(visit_time) >= USER_TIMEOUT)

#define TUP_TIMEOUT	g_adv_config.tcp_udp_timeout
#define is_tup_timeout(visit_time) \
	(u64_to_s(g_current_time) - union_to_s(visit_time) >= TUP_TIMEOUT)

static void hash_tcp_timeout(dp_packet_t *dp_packet)
{
	dp_tcp_session_t *ts;
	dp_tcp_session_t *ts_prev;

	if (!dp_packet || dp_packet->tcp_session_list) {
		log_warning("received a NULL pointer\n");
		return;
	}

	ts = dp_packet->tcp_session_list;
	ts_prev = dp_packet->tcp_session_list;

	while (ts) {
		if (is_tup_timeout(ts->visit_time)) {
			log_debug("tcp timeout\n");
			if (ts != dp_packet->tcp_session_list) {
				ts_prev->next = ts->next;

				dp_packet->tcp_session = ts;
				put_into_xdr_queue(dp_packet, XDR_TYPE_TCP);

				check_free((void **)&ts);
				dp_packet->tcp_session = NULL;
				ts = ts_prev->next;
			} else {
				dp_packet->tcp_session_list = ts->next;
				ts_prev = ts->next;

				dp_packet->tcp_session = ts;
				put_into_xdr_queue(dp_packet, XDR_TYPE_TCP);

				check_free((void **)&ts);
				dp_packet->tcp_session = NULL;
				ts = ts_prev;
			}
			continue;
		}

		ts_prev = ts;
		ts = ts->next;
	}

}

static void hash_udp_timeout(dp_packet_t *dp_packet)
{
	dp_udp_session_t *us;
	dp_udp_session_t *us_prev;

	if (!dp_packet || dp_packet->udp_session_list) {
		log_warning("received a NULL pointer\n");
		return;
	}

	us = dp_packet->udp_session_list;
	us_prev = dp_packet->udp_session_list;

	while (us) {
		if (is_tup_timeout(us->visit_time)) {
			log_debug("udp timeout\n");
			if (us != dp_packet->udp_session_list) {
				us_prev->next = us->next;

				dp_packet->udp_session = us;
				put_into_xdr_queue(dp_packet, XDR_TYPE_UDP);

				check_free((void **)&us);
				dp_packet->udp_session = NULL;
				us = us_prev->next;
			} else {
				dp_packet->udp_session_list = us->next;
				us_prev = us->next;

				dp_packet->udp_session = us;
				put_into_xdr_queue(dp_packet, XDR_TYPE_UDP);

				check_free((void **)&us);
				dp_packet->udp_session = NULL;
				us = us_prev;
			}
			continue;
		}

		us_prev = us;
		us = us->next;
	}
}

#define TCP_SESSION_LIST  (hash_packet->dp_packet.tcp_session_list)
#define UDP_SESSION_LIST  (hash_packet->dp_packet.udp_session_list)

/* don't put hash_timeout rigth after get_hash_dp_xxx */
static void hash_timeout(void)
{
	sb_s32 i;
	dp_hash_packet_t *hash_packet;
	dp_hash_packet_t *hash_packet_prev;
	dp_hash_user_t *hash_user;
	dp_hash_user_t *hash_user_prev;

	for (i = 0; i < HASH_SIZE; i++) {

		hash_packet = dp_hash_packet[i];
		hash_packet_prev = dp_hash_packet[i];
		while (hash_packet) {

			hash_tcp_timeout(&hash_packet->dp_packet);
			hash_udp_timeout(&hash_packet->dp_packet);

			if (!TCP_SESSION_LIST && !UDP_SESSION_LIST) {
				if (hash_packet != dp_hash_packet[i]) {
					hash_packet_prev->next =
						hash_packet->next;
					check_free((void **)&hash_packet);
					hash_packet = hash_packet_prev->next;
				} else {
					dp_hash_packet[i] = hash_packet->next;
					hash_packet_prev = hash_packet->next;
					check_free((void **)&hash_packet);
					hash_packet = hash_packet_prev;
				}
				continue;
			}
			hash_packet_prev = hash_packet;
			hash_packet = hash_packet->next;
		}

		hash_user = dp_hash_user[i];
		hash_user_prev = dp_hash_user[i];
		while (hash_user) {

			if (is_user_timeout(hash_user->visit_time)) {
				if (hash_user != dp_hash_user[i]) {
					hash_user_prev->next = hash_user->next;
					check_free((void **)&hash_user);
					hash_user = hash_user_prev->next;
				} else {
					dp_hash_user[i] = hash_user->next;
					hash_user_prev = hash_user->next;
					check_free((void **)&hash_user);
					hash_user = hash_user_prev;
				}
				continue;
			}
			hash_user_prev = hash_user;
			hash_user = hash_user->next;
		}

	}

}

/*
 * dp_hash_packet_t has no visit_time member,
 * `cause if tcp_session_list == NULL && udp_session_list == NULL,
 * it should be deleted.
 */
static dp_packet_t *get_hash_dp_packet(sb_u64 user_id)
{
	dp_hash_packet_t *hash_packet;
	sb_u32 hash_idx = hash_value(user_id) % HASH_SIZE;

	hash_packet = dp_hash_packet[hash_idx];

	while (hash_packet) {
		if (user_id == hash_packet->user_id) {
			log_debug("hit hash dp packet\n");
			return &hash_packet->dp_packet;
		}
		hash_packet = hash_packet->next;
	}

	// here, hash_packet is NULL at first or at end of hash list（NULL too!)
	log_debug("create a new dp hash packet\n");
	hash_packet = check_malloc("get_hash_dp_packet", 1,
				   sizeof(dp_hash_packet_t));
	if (!hash_packet) {
		log_warning("unable to create a new dp hash packet\n");
		return NULL;
	}

	// initialize new dp_hash_pakcet
	hash_packet->user_id = user_id;

	// there is no need to initialize other member

	hash_packet->next = dp_hash_packet[hash_idx];
	dp_hash_packet[hash_idx] = hash_packet;

	return &hash_packet->dp_packet;
}

static dp_user_info_t *get_hash_dp_user(sb_u64 user_id)
{
	dp_hash_user_t *hash_user;
	sb_u32 hash_idx = hash_value(user_id) % HASH_SIZE;

	hash_user = dp_hash_user[hash_idx];

	while (hash_user) {
		if (user_id == hash_user->user_id) {
			log_debug("hit hash user\n");
			hash_user->visit_time.capture_time_total =
				g_current_time;
			return &hash_user->user;
		}
		hash_user = hash_user->next;
	}

	// here, hash_user is NULL at first or at end of hash list（NULL too!)
	log_debug("create a new hash user\n");
	hash_user = check_malloc("get_hash_dp_user", 1, sizeof(dp_hash_user_t));
	if (!hash_user) {
		log_warning("unable to create a new dp hash user\n");
		return NULL;
	}

	// initialize new dp_hash_user
	hash_user->user_id = user_id;

	// there is no need to initialize other member

	hash_user->next = dp_hash_user[hash_idx];
	dp_hash_user[hash_idx] = hash_user;

	return &hash_user->user;
}

//
static void put_into_xdr_queue(dp_packet_t *dp_packet, xdr_type_t type)
{
	sb_s32 i;
	dp_xdr_queue_t *xdr_queue = NULL;
	dp_tcp_session_t *ts;
	dp_tcp_session_t *ts_prev;
	dp_udp_session_t *us;
	dp_udp_session_t *us_prev;

	if (!dp_packet) {
		log_warning("called unexpectively\n");
		return;
	}

	for (i = 0; i < NR_XDR_WORKERS; i++) {
		xdr_queue = dp_xdr_queue + i;
		if (is_queue_empty(xdr_queue))
			break;
	}

	switch (type) {
	case XDR_TYPE_TCP:
		log_debug("put tcp session into queue\n");
		if (!dp_packet->tcp_session) {
			log_warning("called unexpectively\n");
			return;
		}

		ts = dp_packet->tcp_session;

		ts->tcp.tcp_create_res_delay =
			time_minus(ts->tcp_syn_ack_time, ts->tcp_syn_time);
		ts->tcp.tcp_create_ack_delay =
			time_minus(ts->tcp_est_time, ts->tcp_syn_time);
		ts->tcp.tcp_first_req_delay  =
			time_minus(ts->tcp_first_req_time, ts->tcp_est_time);
		ts->tcp.tcp_first_res_delay =
			time_minus(ts->tcp_first_resp_time,
				   ts->tcp_first_req_time);

		if (TCP_STATE_CLOSED == ts->tcp_state) {
			ts->tcp.user_pub_header.end_time =
				union_to_ms(ts->tcp_closed_time);
			ts->tcp.session_end_status = 1;
		} else {
			ts->tcp.user_pub_header.end_time =
				u64_to_ms(g_current_time);
			ts->tcp.session_end_status = 3;
		}

		// put into queue if not full
		if (NR_XDR_WORKERS != i) {
			copy_xdr_tup(&xdr_queue->lb_xdrs[xdr_queue->tail],
				     &ts->tcp);
			inc_queue_tail(xdr_queue);
			inc_queue_push_idx(xdr_queue);
		}

		// free tcp_session
		if (ts == dp_packet->tcp_session_list) {
			dp_packet->tcp_session_list = ts->next;
		} else {
			ts = dp_packet->tcp_session_list->next;
			ts_prev = dp_packet->tcp_session_list;
			while (ts) {
				if (ts == dp_packet->tcp_session) {
					ts_prev->next = ts->next;
					break;
				}
				ts_prev = ts;
				ts = ts->next;
			}
		}

		// tcp_session will be set to NULL
		check_free((void **)&dp_packet->tcp_session);

		break;
	case XDR_TYPE_UDP:
		log_debug("put udp session into queue\n");
		if (!dp_packet->udp_session) {
			log_warning("called unexpectively\n");
			return;
		}

		us = dp_packet->udp_session;
		us->udp.user_pub_header.start_time = u64_to_ms(g_current_time);

		// put into queue if not full
		if (NR_XDR_WORKERS != i) {
			copy_xdr_tup(&xdr_queue->lb_xdrs[xdr_queue->tail],
				     &us->udp);
			inc_queue_tail(xdr_queue);
			inc_queue_push_idx(xdr_queue);
		}

		// free udp_session
		if (us == dp_packet->udp_session_list) {
			dp_packet->udp_session_list = us->next;
		} else {
			us = dp_packet->udp_session_list->next;
			us_prev = dp_packet->udp_session_list;
			while (us) {
				if (us == dp_packet->udp_session) {
					us_prev->next = us->next;
					break;
				}
				us_prev = us;
				us = us->next;
			}
		}

		// udp_session will be set to NULL
		check_free((void **)&dp_packet->udp_session);

		break;
	default:
		log_warning("unknown xdr type\n");
		break;
	}
}

static void flush_statistics()
{
	sb_s8 time_buf[30];
	struct tm *tm;
	time_t clock = (time_t)(u64_to_s(g_current_time));

	tm = localtime(&clock);
	sprintf(time_buf, "%d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	log_failure("Data parser statistics(%s):\n"
		    "  Total packets:        %lu\n"
		    "  Total packets size:   %lu\n"
		    "  TCP packets:          %lu\n"
		    "  TCP packets size:     %lu\n"
		    "  UDP packets:          %lu\n"
		    "  UDP packets size:     %lu\n"
		    "  Non-IP packets:       %lu\n"
		    "  Non-TUP packets:      %lu\n"
		    "  Total process time:   %lu\n"
		    "  Maximum process time: %u\n"
		    "  Minmal process time:  %u\n",
		    time_buf,
		    dp_statistic->packet_count,
		    dp_statistic->packet_size,
		    dp_statistic->tcp_packet_count,
		    dp_statistic->tcp_packet_size,
		    dp_statistic->udp_packet_count,
		    dp_statistic->udp_packet_size,
		    dp_statistic->nip_packet_count,
		    dp_statistic->ntcpudp_packet_count,
		    dp_statistic->total_process_time,
		    dp_statistic->max_process_time,
		    dp_statistic->min_process_time);

	memset(dp_statistic, 0, sizeof(dp_statistic_t));
	dp_statistic->cpu_id = g_cpu_id;
}

