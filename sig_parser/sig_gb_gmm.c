/*
 * sig_gb_gmm.c
 *
 *  Created on: 2014年6月18日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <arpa/inet.h>

//tilera 相关头文件
#include <tmc/cmem.h>
#include <tmc/mem.h>
#include <tmc/sync.h>
#include <tmc/cpus.h>
#include <tmc/spin.h>
#include <tmc/task.h>
#include <gxio/mpipe.h>
#include <gxio/mica.h>
#include <gxcr/gxcr.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_xdr_struct.h"
#include "sb_error_code.h"
#include "sb_base_file.h"
#include "sb_base_util.h"
#include "sb_shared_queue.h"
#include "sb_base_mica.h"
#include "sb_base_mpipe.h"
#include "sb_public_var.h"
#include "sb_base_mem.h"
#include "sb_base_process.h"
#include "sb_base_pack.h"
#include "sb_qlog.h"
#include "sb_sp.h"

#include "sig_parse.h"
#include "sig_fragment.h"
#include "sig_packet.h"
#include "sig_gb.h"
#include "sig_gb_bssgp.h"
#include "sig_gb_gmm.h"


sb_s32 sp_get_gmm_head(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len = 0;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;

	if (NULL == data || len < MIN_GMM_HEAD_LEN) {
		//printf("sp_get_gmm_head: len = %d\n", len);
		return RET_FAIL;
	}

	gb_info->gmm.gmm_cause = 0;
	gb_info->gmm.sm_cause = 0x48;

	if ((data[0] & 0x0F) == 0x08) //GPRS mobility management messages
	{
		gb_info->gmm.cdr_subtype = data[1];
		switch (data[1]) {
		case GMM_ATTACH_REQUEST: {
			ret = sp_parse_attach_request(p_packet_info, gb_info);

			break;
		}
		case GMM_AUTH_CIPHER_RESPONSE: {
			ret = sp_parse_auth_cipher_response(p_packet_info, gb_info);

			break;
		}
		case GMM_ATTACH_ACCEPT: {
			ret = sp_parse_attach_accept(p_packet_info, gb_info);

			break;
		}
		case GMM_ATTACH_COMPLETE: {
			ret = sp_parse_attach_complete(p_packet_info, gb_info);

			break;
		}
		case GMM_ATTACH_REJECT: {
			ret = sp_parse_attach_reject(p_packet_info, gb_info);

			break;
		}
		case GMM_DETACH_REQUEST: {
			ret = sp_parse_detach_request(p_packet_info, gb_info);

			break;
		}
		case GMM_DETACH_ACCEPT: {
			ret = sp_parse_detach_accept(p_packet_info, gb_info);

			break;
		}
		case GMM_ROUTING_AREA_UPDATE_REQUEST: {
			ret = sp_parse_routing_area_update_request(p_packet_info, gb_info);

			break;
		}
		case GMM_ROUTING_AREA_UPDATE_ACCEPT: {
			ret = sp_parse_routing_area_update_accept(p_packet_info, gb_info);

			break;
		}
		case GMM_ROUTING_AREA_UPDATE_COMPLETE: {
			ret = sp_parse_routing_area_update_complete(p_packet_info, gb_info);

			break;
		}
		case GMM_ROUTING_AREA_UPDATE_REJECT: {
			ret = sp_parse_routing_area_update_reject(p_packet_info, gb_info);

			break;
		}
		default: {
			//printf("mobility cmd error\n");
			return RET_FAIL;
		}
		}
	}
	else if ((data[0] & 0x0F) == 0x0A) //GPRS session management messages
	{
		sb_u8 vdata = data[1];
		if((data[0] & 0x70) == 0x70 && (data[1]&0x80) == 0x80)
		{
			vdata = data[2];
			p_packet_info->p_data++;
			p_packet_info->data_len --;
		}
		gb_info->gmm.cdr_subtype = vdata;
		switch (vdata) {
		case GMM_ACT_PDP_CONTEXT_REQUEST: {
			ret = sp_parse_activate_pdp_context_request(p_packet_info, gb_info);

			break;
		}
		case GMM_ACT_PDP_CONTEXT_ACCEPT: {
			ret = sp_parse_activate_pdp_context_accept(p_packet_info, gb_info);

			break;
		}
		case GMM_ACT_PDP_CONTEXT_REJECT: {
			ret = sp_parse_activate_pdp_context_reject(p_packet_info, gb_info);

			break;
		}
		case GMM_DEACT_PDP_CONTEXT_REQUEST: {
			ret = sp_parse_deactivate_pdp_context_request(p_packet_info, gb_info);

			break;
		}
		case GMM_DEACT_PDP_CONTEXT_ACCEPT: {
			ret = sp_parse_deactivate_pdp_context_accept(p_packet_info, gb_info);

			break;
		}
		default: {
			//printf("session cmd error\n");
			return RET_FAIL;
		}
		}
	}
	else {
		//printf("message type error!\n");
		return RET_FAIL;
	}

	return ret;
}

sb_s32 sp_parse_attach_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;
	sb_iei_t iei;
	gprs_msc_t gprs_msc;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;

	if (len < 23)
	{
		printf("ATTACH(0x%08X) len = %d\n", gb_info->bssgp.tlli, len);
		return RET_FAIL;
	}

	gb_info->gmm.cdr_type = CDR_ATTACH;
	gb_info->gmm.cdr_result = CDR_INCOMPLETE;
	gb_info->gmm.cdr_d_result = CDR_REQUST_D;

	//go to Mobile identity
	data += (3 + data[2] + 3);
	len -= (3 + data[2] + 3);
	iei.length = data[0];
	iei.value = &data[1];
	if (iei.length >= 5 && iei.length <= 9) {
		switch (iei.value[0] & 0x07) {
		case 0x01: //IMSI,BCD CODE
		{
			if ((iei.value[0] & 0x08) == 0x08) //odd
			{
				gb_info->gmm.imsi = bcd_to_int(iei.value, iei.length, 1);
			} else //even
			{
				gb_info->gmm.imsi = bcd_to_int(iei.value,
						iei.length, 0);
			}
			break;
		}
		case 0x02: //IMEI,BCD CODE
		{
			if ((iei.value[0] & 0x08) == 0x08) //odd
			{
				gb_info->gmm.imei = bcd_to_int(iei.value, iei.length, 1);
			} else //even
			{
				gb_info->gmm.imei = bcd_to_int(iei.value,
						iei.length, 0);
			}
			break;
		}
		case 0x04: //TMSI
		{
			gb_info->gmm.tmsi = ntohl(*(sb_u32*) &iei.value[1]);
			break;
		}
		default: {
			break;
		}
		}
	} else
	{
		printf("imsi len = %d\n", iei.length);
		return RET_FAIL;
	}

	//go to Old routing area identification
	data += iei.length + 1;
	len -= iei.length + 1;

	gb_info->gmm.mcc = (data[0] & 0x0F) * 100 + ((data[0] >> 4) & 0x0F) * 10
			+ (data[1] & 0x0F);
	if ((data[1] & 0xF0) == 0xF0) //两位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 10 + ((data[2] >> 4) & 0x0F);
	} else //三位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 100
				+ ((data[2] >> 4) & 0x0F) * 10 + ((data[1] >> 4) & 0x0F);
	}

	gb_info->gmm.lac = ntohs(*(sb_u16*) &data[3]);
	gb_info->gmm.rac = data[5];

	//go to MS Radio Access capability
	data += 6;
	len -= 6;
	iei.length = data[0];
	iei.value = &data[1];

	get_msc(iei.value, (gprs_msc_t*)&gprs_msc);

	gb_info->gmm.gprs_msc = gprs_msc.gprs_msc;
	gb_info->gmm.egprs_msc = gprs_msc.egprs_msc;
	gb_info->gmm.umts = gprs_msc.umts;

	return ret;
}


sb_s32 sp_parse_auth_cipher_response(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;
	sb_iei_t iei;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;

	if (len < 14)
	{
		return RET_FAIL;
	}

	//printf("parse auth\n");
	gb_info->gmm.cdr_type = CDR_ATTACH;
	gb_info->gmm.cdr_result = CDR_INCOMPLETE;
	gb_info->gmm.cdr_d_result = CDR_AUTH_RES_D;

	//go to Mobile identity
	data += 3;
	len -= 3;
	while (len > 0) {
		iei.type = data[0];
		switch (iei.type) {
		case 0x22: //Authentication parameter Response
		{
			data += 5;
			len -= 5;

			break;
		}
		case 0x23: //IMEISV
		{
			iei.length = data[1];
			iei.value = &data[2];
			if (iei.length >= 5 && iei.length <= 9) {
				switch (iei.value[0] & 0x07) {
				case 0x01: //IMSI,BCD CODE
				{
					if ((iei.value[0] & 0x08) == 0x08) //odd
					{
						gb_info->gmm.imsi = bcd_to_int(iei.value, iei.length, 1);
					} else //even
					{
						gb_info->gmm.imsi = bcd_to_int(iei.value,
								iei.length, 0);
					}
					break;
				}
				case 0x02: //IMEI,BCD CODE
				{
					if ((iei.value[0] & 0x08) == 0x08) //odd
					{
						gb_info->gmm.imei = bcd_to_int(iei.value, iei.length, 1);
					} else //even
					{
						gb_info->gmm.imei = bcd_to_int(iei.value,
								iei.length, 0);
					}
					break;
				}
				case 0x03: //IMEISV,BCD CODE
				{
					if ((iei.value[0] & 0x08) == 0x08) //odd
					{
						gb_info->gmm.imei = bcd_to_int(iei.value, iei.length, 1);
					} else //even
					{
						gb_info->gmm.imei = bcd_to_int(iei.value,
								iei.length, 0);
					}
					break;
				}
				case 0x04: //TMSI
				{
					gb_info->gmm.tmsi = ntohl(*(sb_u32*) &iei.value[1]);
					break;
				}
				default: {
					break;
				}

				}
				return ret;
			} else
			{
				printf("imsi len = %d\n", iei.length);
				return RET_FAIL;
			}
			break;
		}

		default: {
			return ret;
		}
		}
	}

	return ret;
}

sb_s32 sp_parse_attach_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;
	sb_iei_t iei;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;

	if (len < 8)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ATTACH;
	gb_info->gmm.cdr_result = CDR_SUCCESS;
	gb_info->gmm.cdr_d_result = CDR_ACCEPT_D;

	//go to Routing area identification
	data += 5;
	len -= 5;

	gb_info->gmm.mcc = (data[0] & 0x0F) * 100 + ((data[0] >> 4) & 0x0F) * 10
			+ (data[1] & 0x0F);
	if ((data[1] & 0xF0) == 0xF0) //两位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 10 + ((data[2] >> 4) & 0x0F);
	} else //三位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 100
				+ ((data[2] >> 4) & 0x0F) * 10 + ((data[1] >> 4) & 0x0F);
	}

	gb_info->gmm.lac = ntohs(*(sb_u16*) &data[3]);
	gb_info->gmm.rac = data[5];

	//go to MS Radio Access capability
	data += 6;
	len -= 6;
	while (len > 0) {
		iei.type = data[0];
		switch (iei.type) {
		case 0x19: //P-TMSI signature
		{
			data += 4;
			len -= 4;

			break;
		}
		case 0x17: //Negotiated READY timer
		{
			data += 2;
			len -= 2;

			break;
		}
		case 0x18: //Allocated P-TMSI
		{
			gb_info->gmm.tmsi = ntohl(*(sb_u32*) &data[3]);
			gb_info->gmm.cdr_result = CDR_INCOMPLETE;
			data += 7;
			len -= 7;

			break;
		}
		case 0x23: //MS identity
		{
			data += 2 + data[1];
			len -= 2 + data[1];

			break;
		}
		case 0x25: //GMM cause
		{
			gb_info->gmm.gmm_cause = data[1];
			data += 2;
			len -= 2;

			return ret;

			break;
		}
		default: {
			return ret;
		}
		}
	}

	return ret;
}

sb_s32 sp_parse_attach_complete(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	len = p_packet_info->data_len;
	if (len < 1)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ATTACH;
	gb_info->gmm.cdr_result = CDR_SUCCESS;
	gb_info->gmm.cdr_d_result = CDR_COMPLETE_D;

	return ret;
}


sb_s32 sp_parse_attach_reject(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;
	if (len < 3)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ATTACH;
	gb_info->gmm.cdr_result = CDR_FAILURE;
	gb_info->gmm.cdr_d_result = CDR_REJECT_D;

	gb_info->gmm.gmm_cause = data[2];

	return ret;
}


sb_s32 sp_parse_detach_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;
	if (len < 3)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_DETACH;
	gb_info->gmm.cdr_result = CDR_INCOMPLETE;
	gb_info->gmm.cdr_d_result = CDR_REQUST_D;

	if(len >= 5 && data[3] == 0x25)gb_info->gmm.gmm_cause = data[4];

	return ret;
}

sb_s32 sp_parse_detach_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	len = p_packet_info->data_len;
	if (len < 1)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_DETACH;
	gb_info->gmm.cdr_result = CDR_SUCCESS;
	gb_info->gmm.cdr_d_result = CDR_ACCEPT_D;

	return ret;
}

sb_s32 sp_parse_routing_area_update_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;
	sb_iei_t iei;
	gprs_msc_t gprs_msc;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;
	if (len < 12)
		return RET_FAIL;

	if ((data[2] & 0x07) == 0x00)
		gb_info->gmm.cdr_type = CDR_ROUTING_AREA_UPDATE;
	else if ((data[2] & 0x07) == 0x03)
		gb_info->gmm.cdr_type = CDR_PERIODIC_ROUTING_AREA_UPDATE;
	else
		return RET_FAIL;

	gb_info->gmm.cdr_result = CDR_INCOMPLETE;
	gb_info->gmm.cdr_d_result = CDR_REQUST_D;

	//go to Old routing area identification
	data += 3;
	len -= 3;
	gb_info->gmm.mcc = (data[0] & 0x0F) * 100 + ((data[0] >> 4) & 0x0F) * 10
			+ (data[1] & 0x0F);
	if ((data[1] & 0xF0) == 0xF0) //两位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 10 + ((data[2] >> 4) & 0x0F);
	} else //三位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 100
				+ ((data[2] >> 4) & 0x0F) * 10 + ((data[1] >> 4) & 0x0F);
	}

	gb_info->gmm.lac = ntohs(*(sb_u16*) &data[3]);
	gb_info->gmm.rac = data[5];

	//go to MS Radio Access capability
	data += 6;
	len -= 6;

	iei.length = data[0];
	iei.value = &data[1];

	get_msc(iei.value, (gprs_msc_t*)&gprs_msc);

	gb_info->gmm.gprs_msc = gprs_msc.gprs_msc;
	gb_info->gmm.egprs_msc = gprs_msc.egprs_msc;
	gb_info->gmm.umts = gprs_msc.umts;

	data += iei.length + 1;
	len -= iei.length + 1;

	while (len > 0) {
		iei.type = data[0];
		switch (iei.type) {
		case 0x19: //Old P-TMSI signature
		{
			data += 4;
			len -= 4;

			break;
		}
		case 0x17: //Requested READY timer value
		{
			data += 2;
			len -= 2;

			break;
		}
		case 0x27: //DRX parameter
		{
			data += 3;
			len -= 3;

			break;
		}
		case 0x09: //TMSI status
		{
			data += 1;
			len -= 1;

			break;
		}
		case 0x18: //P-TMSI
		{
			gb_info->gmm.tmsi = ntohl(*(sb_u32*) &data[3]);
			return ret;

			break;
		}
		default: {
			return ret;
		}
		}
	}
	return ret;
}

sb_s32 sp_parse_routing_area_update_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;
	sb_iei_t iei;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;
	if (len < 7)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ROUTING_AREA_UPDATE;
	gb_info->gmm.cdr_result = CDR_SUCCESS;
	gb_info->gmm.cdr_d_result = CDR_ACCEPT_D;

	//go to Old routing area identification
	data += 4;
	len -= 4;
	gb_info->gmm.mcc = (data[0] & 0x0F) * 100 + ((data[0] >> 4) & 0x0F) * 10
			+ (data[1] & 0x0F);
	if ((data[1] & 0xF0) == 0xF0) //两位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 10 + ((data[2] >> 4) & 0x0F);
	} else //三位数
	{
		gb_info->gmm.mnc = (data[2] & 0x0F) * 100
				+ ((data[2] >> 4) & 0x0F) * 10 + ((data[1] >> 4) & 0x0F);
	}

	gb_info->gmm.lac = ntohs(*(sb_u16*) &data[3]);
	gb_info->gmm.rac = data[5];

	//go to MS Radio Access capability
	data += 6;
	len -= 6;
	while (len > 0) {
		iei.type = data[0];
		switch (iei.type) {
		case 0x19: //Old P-TMSI signature
		{
			data += 4;
			len -= 4;

			break;
		}
		case 0x18: //Allocated P-TMSI
		{
			gb_info->gmm.tmsi = ntohl(*(sb_u32*) &data[3]);
			gb_info->gmm.cdr_result = CDR_INCOMPLETE;

			data += 7;
			len -= 7;

			break;
		}
		case 0x23: //MS identity
		{
			data += 2 + data[1];
			len -= 2 + data[1];
			break;
		}
		case 0x26: //List of Receive N PDU Numbers
		{
			gb_info->gmm.cdr_result = CDR_INCOMPLETE;
			data += 2 + data[1];
			len -= 2 + data[1];

			break;
		}
		case 0x17: //Negotiated READY timer value
		{
			data += 2;
			len -= 2;

			break;
		}
		case 0x25: //GMM cause
		{
			gb_info->gmm.gmm_cause = data[1];
			data += 2;
			len -= 2;

			return ret;

			break;
		}
		default: {

			return ret;
		}
		}
	}

	return ret;
}

sb_s32 sp_parse_routing_area_update_complete(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	len = p_packet_info->data_len;
	if (len < 1)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ROUTING_AREA_UPDATE;
	gb_info->gmm.cdr_result = CDR_SUCCESS;
	gb_info->gmm.cdr_d_result = CDR_COMPLETE_D;

	return ret;
}

sb_s32 sp_parse_routing_area_update_reject(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;
	if (len < 4)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ROUTING_AREA_UPDATE;
	gb_info->gmm.cdr_result = CDR_FAILURE;
	gb_info->gmm.cdr_d_result = CDR_REJECT_D;

	gb_info->gmm.gmm_cause = data[2];

	return ret;
}

sb_s32 sp_parse_activate_pdp_context_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;
	sb_iei_t iei;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;
	if (len < 18)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ACTIVATE_PDP_CONTEXT;

	gb_info->gmm.cdr_result = CDR_INCOMPLETE;
	gb_info->gmm.cdr_d_result = CDR_REQUST_D;


	{
		gb_info->gmm.nsapi = data[2];
		data += 4;
		len -= 4;
	}

	iei.length = data[0];
	iei.value = &data[1];
	if (len <= iei.length)
	{
		printf("1: tlli 0x%08X, len(%d) < iei.length(%d)\n", gb_info->bssgp.tlli, len , iei.length);
		return RET_FAIL;
	}

	gb_info->gmm.qos_r_delay = ((iei.value[0] & 0x38) >> 3); //Delay Class
	gb_info->gmm.qos_r_reliability = (iei.value[0] & 0x07); //Reliability Class
	gb_info->gmm.qos_r_precedence = (iei.value[1] & 0x07); //Precedence Class
	gb_info->gmm.qos_r_peak = ((iei.value[1] & 0xF0) >> 4); //Peak throughput
	gb_info->gmm.qos_r_mean = (iei.value[2] & 0x1F); //Mean throughput

	data += iei.length + 1;
	len -= iei.length + 1;

	iei.length = data[0];
	iei.value = &data[1];
	if (len <= iei.length)
	{
		printf("2: tlli 0x%08X, len(%d) < iei.length(%d)\n", gb_info->bssgp.tlli, len , iei.length);
		return RET_FAIL;
	}

	len -= iei.length + 1;
	if (len <= 1)
		return RET_SUCC;
	data += iei.length + 1;

	iei.type = data[0];
	iei.length = data[1];
	iei.value = &data[2];
	if (len <= iei.length)
	{
		printf("3: tlli 0x%08X, len(%d) < iei.length(%d)\n", gb_info->bssgp.tlli, len , iei.length);
		return RET_FAIL;
	}

	if (iei.type == 0x28) //Access point name
	{
		sb_u8 apn_len = 0;
		iei.length = iei.length > 40?40:iei.length;

		for(int i = 0; i < iei.length - 1; i++)
		{
			if((apn_len + iei.value[i] + 1) < 40)
			{
				if(apn_len > 0)
				{
					gb_info->gmm.apn[apn_len] = '.';
					memcpy((sb_s8 *)&gb_info->gmm.apn[apn_len + 1], (sb_s8 *)&iei.value[i + 1], iei.value[i]);
					apn_len += iei.value[i];
					apn_len ++;
				}
				else
				{
					memcpy((sb_s8 *)&gb_info->gmm.apn[apn_len], (sb_s8 *)&iei.value[i + 1], iei.value[i]);
					apn_len += iei.value[i];
				}


				i += iei.value[i];
			}
			else
				break;
		}
		gb_info->gmm.apn[apn_len] = 0;
	}

	return ret;
}

sb_s32 sp_parse_activate_pdp_context_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len;
	sb_iei_t iei;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;
	if (len < 15)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ACTIVATE_PDP_CONTEXT;

	gb_info->gmm.cdr_result = CDR_SUCCESS;
	gb_info->gmm.cdr_d_result = CDR_ACCEPT_D;


	{
		data += 3;
		len -= 3;
	}

	iei.length = data[0];
	iei.value = &data[1];
	if (len <= iei.length)
		return RET_FAIL;

	gb_info->gmm.qos_n_delay = ((iei.value[0] & 0x38) >> 3); //Delay Class
	gb_info->gmm.qos_n_reliability = (iei.value[0] & 0x07); //Reliability Class
	gb_info->gmm.qos_n_precedence = (iei.value[1] & 0x07); //Precedence Class
	gb_info->gmm.qos_n_peak = ((iei.value[1] & 0xF0) >> 4); //Peak throughput
	gb_info->gmm.qos_n_mean = (iei.value[2] & 0x1F); //Mean throughput

	len -= iei.length + 2;
	if (len <= 1)
		return RET_SUCC;
	data += iei.length + 2;

	while (len > 0) {
		iei.type = data[0];
		switch (iei.type) {
		case 0x2B: //PDP address
		{
			iei.length = data[1];
			iei.value = &data[2];
			gb_info->gmm.user_ip = ntohl(*(sb_u32*) &iei.value[2]);
			data += 2 + iei.length;
			len -= 2 + iei.length;

			break;
		}
		case 0x27: //Protocol configuration options
		{
			iei.length = data[1];
			data += 2 + iei.length;
			len -= 2 + iei.length;

			break;
		}
		case 0x34: //Packet Flow Identifier
		{
			data += 3;
			len -= 3;

			break;
		}
		case 0x39: //SM cause
		{
			gb_info->gmm.sm_cause = data[2];
			data += 3;
			len -= 3;

			return ret;

			break;
		}
		default: {
			return ret;
		}
		}
	}

	return ret;
}

sb_s32 sp_parse_activate_pdp_context_reject(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	len = p_packet_info->data_len;
	if (len < 3)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_ACTIVATE_PDP_CONTEXT;
	gb_info->gmm.cdr_result = CDR_FAILURE;
	gb_info->gmm.cdr_d_result = CDR_REJECT_D;

	{
		gb_info->gmm.sm_cause = p_packet_info->p_data[2];
	}

	return ret;
}

sb_s32 sp_parse_deactivate_pdp_context_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	len = p_packet_info->data_len;
	if (len < 3)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_DEACTIVATE_PDP_CONTEXT;
	gb_info->gmm.cdr_result = CDR_INCOMPLETE;
	gb_info->gmm.cdr_d_result = CDR_REQUST_D;


	{
		gb_info->gmm.sm_cause = p_packet_info->p_data[2];
	}

	return ret;
}

sb_s32 sp_parse_deactivate_pdp_context_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u32 len;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	len = p_packet_info->data_len;
	if (len < 1)
		return RET_FAIL;

	gb_info->gmm.cdr_type = CDR_DEACTIVATE_PDP_CONTEXT;
	gb_info->gmm.cdr_result = CDR_SUCCESS;
	gb_info->gmm.cdr_d_result = CDR_ACCEPT_D;

	return ret;
}

