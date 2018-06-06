/*
 * data_parser/dp_if0_1.c
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_error_code.h"
#include "sb_base_util.h"
#include "sb_base_mem.h"
#include "sb_public_var.h"
#include "sb_xdr_struct.h"
#include "sb_base_mpipe.h"

#include "dp_define.h"
#include "dp_struct.h"

static void set_ip_checksum(ip_head_t *ip)
{
	sb_u8 *s = (sb_u8 *)ip;
	sb_u32 sum = 0;
	sb_s32 i;

	ip->ip_sum = 0;

	for (i = 0; i < (ip->ip_hl << 2); i += 2)
		sum += *(sb_u16 *)(s + i);
	ip->ip_sum = (sb_u16)~((sum & 0xffff) + (sum >> 16));
}

void if0_1_send_packet(dp_packet_t *dp_packet)
{
	gxio_mpipe_edesc_t edesc;
	sb_u8 *frame;
	sb_u8 *ip;
	sb_s32 ip_hdr_len;
	sb_u32 size = 14;
	sb_s32 ret;
	ether_head_t *eh;
	ip_head_t *ip_hdr;
	sb_s8 imsi_buf[16];
	sb_u64 *imsi;
	sb_u32 *sgsn_ip;
	sb_u32 *bsc_ip;
	sb_u64 *ts;
	sb_u8  *mac;


	if (!dp_packet) {
		log_debug("Received a NULL pointer\n");
		return;
	}

	log_debug("if0-1: sending packet...");

	frame = gxio_mpipe_pop_buffer(&g_mpipe_info->context,
			g_mpipe_info->large_stack.stack_id);
	if (NULL == frame) {
		log_debug("stack empty\n");
		return;
	}

	ip = frame + 14;
	eh = (ether_head_t *)frame;
	imsi = (sb_u64 *)(frame + 14 + 20);
	sgsn_ip = (sb_u32 *)(frame + 14 + 20 + 8);
	bsc_ip = (sb_u32 *)(frame + 14 + 20 + 8 + 4);
	ts = (sb_u64 *)(frame + 14 + 20 + 8 + 4 + 4);

	// set ethernet head
	// dst mac
	mac = frame;
	mac[0] = 0x66;
	mac[1] = 0x55;
	mac[2] = 0x66;
	mac[3] = 0x55;
	*(sb_u16 *)(mac + 4) = 1;

	// src mac
	*(sb_u16 *)(mac + 6) = dp_packet->packet_direct ? 0x1000 : 0x0000;
	*(sb_u16 *)(mac + 8) = 0;
	mac[10] = (sb_u8)g_device_config.device_identity;
	mac[11] = 0;

	//
	ip_hdr = (ip_head_t *)dp_packet->data;
	ip_hdr_len = (sb_s32)ip_hdr->ip_hl << 2;
	memcpy(ip, dp_packet->data, 20);
	memcpy(ip + 44, dp_packet->data + ip_hdr_len,
			dp_packet->len - ip_hdr_len);

	size += 44 + dp_packet->len - ip_hdr_len;

	//
	ip_hdr = (ip_head_t *)ip;
	ip_hdr->ip_hl = 0xb;
	ip_hdr->ip_len = htons(ntohs(ip_hdr->ip_len) - ip_hdr_len + 44);

	memcpy(imsi_buf, dp_packet->user->imsi, 15);
	imsi_buf[15] = 0;
	sscanf(imsi_buf, "%lu", imsi);
	*sgsn_ip = dp_packet->user->sgsn_sig_ip;
	*bsc_ip  = dp_packet->user->bsc_sig_ip;
	*ts = htobe64(dp_packet->ts.capture_time_total);

	// ip checksum
	set_ip_checksum(ip_hdr);

	// send packet
	edesc.xfer_size = size;
	edesc.va = (long)frame;
	edesc.bound = 1;
	edesc.inst = g_mpipe_info->instance;
	edesc.stack_idx = g_mpipe_info->large_stack.stack_id;
	edesc.size = g_mpipe_info->large_stack.buffer_kind;
	edesc.hwb = 1;


	ret = gxio_mpipe_equeue_put(g_mpipe_info->equeues_dpi->equeue, edesc);
	if (ret) {
		log_debug("failed\n");
		return;
	}
	log_debug("ok\n");
}

