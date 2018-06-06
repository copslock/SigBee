/*
 * data_parser/dp_sndcp.c
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

#include "dp_define.h"
#include "dp_struct.h"
#include "dp_sndcp.h"

sb_s32 parse_sndcp_head(dp_packet_t *dp_packet)
{

	sb_u8  *data = NULL;
	sb_u16 *no = NULL;

	if (!dp_packet) {
		log_warning("received a NULL pointer!\n");
		return -1;
	}

	data = dp_packet->data;

	if (!data || dp_packet->len < SNDCP_HEAD_LEN) {
		log_error("broken sndcp packet(%u < %u)\n", dp_packet->len,
			 SNDCP_HEAD_LEN);
		return -1;
	}

	dp_packet->sndcp.pdu_type = (data[0] & 0x20) >> 5;
	dp_packet->sndcp.nsapi = data[0] & 0x0F;
	dp_packet->sndcp.is_first = (data[0] & 0x40) >> 6;
	dp_packet->sndcp.is_more = (data[0] & 0x10) >> 4;
	if (1 == dp_packet->sndcp.is_first) {
		dp_packet->sndcp.is_compressed = data[1];
		dp_packet->sndcp.seg_no = (data[2] & 0xF0) >> 4;
		no = (sb_u16 *)(data + 2);
		dp_packet->sndcp.n_pdu = htons(*no) & 0x0FFF;

		dp_packet->data += 4;
		dp_packet->len  -= 4;
	} else {
		dp_packet->sndcp.is_compressed = 0;
		dp_packet->sndcp.seg_no = (data[1] & 0xF0) >> 4;
		no = (sb_u16 *)(data + 1);
		dp_packet->sndcp.n_pdu = htons(*no) & 0x0FFF;

		dp_packet->data += 3;
		dp_packet->len  -= 3;
	}

	return 0;
}

// 第一个段里有IP头，可以从中得到SNDCP包的大小
sb_s32 get_sndcp_packet_size(dp_packet_t *dp_packet)
{
	ip_head_t *ip_hdr;

	if (!dp_packet) {
		log_warning("received a NULL pointer\n");
		return -1;
	}

	ip_hdr = (ip_head_t *)dp_packet->data;
	// 要判断数据大小吗？
	if (4 != ip_hdr->ip_v) {
		log_debug("not ipv4\n");
		dp_statistic->nip_packet_count++;
		return -1;
	}

	return ntohs(ip_hdr->ip_len);
}

