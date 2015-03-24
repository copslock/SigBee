/*
 * ui_sender/ui_sender.c
 */
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_base_util.h"
#include "sb_base_mem.h"
#include "sb_shared_queue.h"
#include "sb_public_var.h"

#define NR_DATA_WORKER	g_adv_config.data_parser_cpus

#define sb_warn printf
#define sb_debug printf

#pragma pack(1)
typedef struct _sdtp_packet {
	sb_u16	sync;
	sb_u16	len;
	sb_u16	type;
	sb_u32	id;
	sb_u32	ts;
	sb_u8	nr;
	dp_user_info_t user;
} sdtp_packet_t;
#pragma pack()

static sb_u32 sdtp_id = 0;
static sdtp_packet_t *sdtp_packet;
dp_user_info_queue_t *dp_user_info_queue;

static void exit_ui_sender(void);
static void process_user_queue();
extern sb_s32 init_ui_socket();
extern void exit_ui_socket();
extern ssize_t send_ui_pkt(const void *buf, size_t size);

void ui_sender_main(void)
{
	sb_s8 ver_info[64], ver_time[32];

	if (tmc_cmem_init(0) < 0) {
		sb_warn("%s can not join cmem\n", g_process_name);
		return;
	}

	memset(ver_info, 0, sizeof(ver_info));
#ifdef DEBUG_VER
	strcpy(ver_info, "Debug ");
#endif

#ifdef BUILD_DATE
	sprintf(ver_time, "Build %u.%u ", BUILD_DATE, BUILD_TIME);
	strcat(ver_info, ver_time);
#endif
	printf("%s version: %s %s\n", g_process_name, EXE_BUILD, ver_info);

	atexit(exit_ui_sender);

	while (0 != init_ui_socket())
		sleep(1);

	if (!g_dp_user_queue_head
			|| g_dp_user_queue_head->num < g_process_order) {
		sb_warn("Bad dp_user_queue\n");
		return;
	}

	dp_user_info_queue = g_dp_user_queue_head->addr
				+ g_process_order * NR_DATA_WORKER;

	sdtp_packet = check_malloc("xdr_sender_main", 1,
			(sizeof(sdtp_packet_t) + 3) & ~3);

	sdtp_packet->sync = htons(0x7e5a);
	sdtp_packet->type = htons(0x9001);
	sdtp_packet->nr = 1;

	while (1)
		sleep(10);
	//process_user_queue();
}

static void exit_ui_sender(void)
{
	printf("goodbye ui sender\n");
	exit_ui_socket();
	check_free((void **)&sdtp_packet);
}

static void process_user_queue()
{
	dp_user_info_queue_t *user_queue;
	dp_user_info_t *user;
	sb_u8 queue_idx = 0;
	sb_u64 push_idx;
	sb_u64 pop_idx;
	sb_u32 head;

	while (0 == *g_process_exit) {
		user_queue = dp_user_info_queue + (queue_idx % NR_DATA_WORKER);
		push_idx = user_queue->push_index;
		pop_idx = user_queue->pop_index;

		if (pop_idx >= push_idx) {
			sb_debug("sp_user_queue is empty(push index: %lu, "
					"pop index:  %lu)\n",
					push_idx, pop_idx);
			cycle_relax();
			queue_idx++;
			continue;
		} else {
			head = user_queue->head;
			user = user_queue->user_infos + head;

			sdtp_packet->len =
				htons((sb_u16)(sizeof(sdtp_packet_t) + 3) & ~3);
			sdtp_packet->id = htonl(sdtp_id++);
			sdtp_packet->ts = ntohl((sb_u32)time(NULL));

			memcpy(&sdtp_packet->user, user, sizeof(*user));

			send_ui_pkt(sdtp_packet, sdtp_packet->len);

		}

		queue_idx++;
		user_queue->head = (head + 1) % user_queue->size;
		user_queue->pop_index++;
	}
}

