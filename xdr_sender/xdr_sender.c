/*
 * xdr_sender/xdr_sender.c
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_base_util.h"
#include "sb_base_mem.h"
#include "sb_shared_queue.h"
#include "sb_public_var.h"

#include "xdr_sender.h"

static sp_xdr_queue_t *sp_xdr_queue;
static dp_xdr_queue_t *dp_xdr_queue;

static sb_u32 sdtp_id = 0;
static sdtp_packet_t *sdtp_packet;

static void exit_xdr_sender(void);
void xdr_send_queue(void);
extern sb_s32 init_xdr_socket();
extern void exit_xdr_socket();
extern ssize_t send_xdr_pkt(const void *buf, size_t size);

#define is_queue_head_invalid(queue_head, workers) \
	(!(queue_head) || (queue_head)->num < (g_process_order + 1) * workers)

void xdr_sender_main(sb_s32 argc, sb_s8 **argv)
{
	sb_s8 ver_info[64], ver_time[32];

	if (tmc_cmem_init(0) < 0) {
		xs_warning("%s can not join cmem\n", g_process_name);
		return;
	}

	ver_info[0] = 0;
#ifdef	DEBUG_VER
	strcpy(ver_info, "Debug ");
#endif

#ifdef 	BUILD_DATE
	sprintf(ver_time, "Build %u.%u ", BUILD_DATE, BUILD_TIME);
	strcat(ver_info, ver_time);
#endif
	xs_info("%s version: %s %s\n", g_process_name, EXE_BUILD, ver_info);

	atexit(exit_xdr_sender);

	while (-1 == init_xdr_socket())
		sleep(1);

	printf("ok!\n");

	if (is_queue_head_invalid(g_sp_xdr_queue_head, NR_SP_WORKERS)) {
		xs_warning("Bad g_sp_xdr_queue_head\n");
		return;
	}

	if (is_queue_head_invalid(g_dp_xdr_queue_head, NR_DATA_WORKERS)) {
		xs_warning("Bad g_dp_xdr_queue_head\n");
		return;
	}

	sp_xdr_queue = INIT_XDR_SP_PACKET_QUEUE;

	dp_xdr_queue = INIT_XDR_DP_USER_QUEUE;

	sdtp_packet = check_malloc("xdr_sender_main", 1, SDTP_PACKET_SIZE);

	sdtp_packet->sync = htons(0x7e5a);
	sdtp_packet->type = htons(0x8006);
	sdtp_packet->nr = 1;


	xdr_send_queue();
}

static void exit_xdr_sender(void)
{
	xs_info("goodbye xdr sender\n");
	exit_xdr_socket();
	check_free((void **)&sdtp_packet);
}

static void xdr_send_sig_queue(void)
{
	sp_xdr_queue_t *xdr_queue;
	sp_xdr_t *xdr;
	static sb_u8 queue_idx = 0;

	xdr_queue = sp_xdr_queue + (queue_idx % NR_SP_WORKERS);

	if (is_queue_empty(xdr_queue)) {
		xs_warning("sp_xdr_queue is empty(push index: %lu, "
				"pop index:  %lu)\n",
				xdr_queue->push_index,
				xdr_queue->pop_index);
		cycle_relax();
		sleep(1);
		queue_idx++;
		return;
	} else {
		ssize_t ret;
		printf("xdr_sender sending sig!\n");
		xdr = xdr_queue->lb_xdrs + xdr_queue->head;

		sdtp_packet->len = htons(ALIGN(15 + xdr->length, 4));
		sdtp_packet->id = htonl(sdtp_id++);
		sdtp_packet->ts = ntohl((sb_u32)time(NULL));

		memcpy(sdtp_packet->data, xdr->data, xdr->length);

		printf("sending %hu\n", ntohs(sdtp_packet->len));

		ret = send_xdr_pkt(sdtp_packet, ntohs(sdtp_packet->len));

		printf("%zd ?= %hu\n", ret, ntohs(sdtp_packet->len));
	}

	queue_idx++;
	inc_queue_head(xdr_queue);
	inc_queue_pop_idx(xdr_queue);
}

static void xdr_send_data_queue(void)
{
	dp_xdr_queue_t *xdr_queue;
	dp_xdr_t *xdr;
	static sb_u8 queue_idx = 0;

	xdr_queue = dp_xdr_queue + (queue_idx % NR_DATA_WORKERS);

	if (is_queue_empty(xdr_queue)) {
		xs_warning("dp_xdr_queue is empty(push index: %lu, "
				"pop index:  %lu)\n",
				xdr_queue->push_index,
				xdr_queue->pop_index);
		cycle_relax();
		queue_idx++;
		return;
	} else {
		xdr = xdr_queue->lb_xdrs + xdr_queue->head;

		sdtp_packet->len = htons(ALIGN(15 + xdr->length, 4));
		sdtp_packet->id = htonl(sdtp_id++);
		sdtp_packet->ts = ntohl((sb_u32)time(NULL));

		memcpy(sdtp_packet->data, xdr->data, xdr->length);

		send_xdr_pkt(sdtp_packet, ntohs(sdtp_packet->len));

	}

	queue_idx++;
	inc_queue_head(xdr_queue);
	inc_queue_pop_idx(xdr_queue);
}

void xdr_send_queue(void)
{
	while (0 == *g_process_exit) {
		xdr_send_sig_queue();
		//xdr_send_data_queue();
	}
}

