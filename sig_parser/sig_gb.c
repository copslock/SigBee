/*
 * sig_gb.c
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

sp_gb_t g_gb_info;

//解析NS层信息
sb_s32 parse_gb_ns(sp_packet_t *p_packet_info)
{
	sb_s32 ret = RET_FAIL;

	if(p_packet_info->data_len < GPRS_NS_HD_LEN)
	{
		g_sp_statistic->packet_err_count++;
		g_sp_statistic->packet_err_size += p_packet_info->s_len;
		return ret;
	}

	if( NS_UNITDATA == p_packet_info->p_data[0])
	{
		sb_ns_t *ns = (sb_ns_t*)p_packet_info->p_data;

		g_gb_info.ns.pdu_type = 0;
		g_gb_info.ns.bvci = ntohs(ns->bvci);

		p_packet_info->p_data =
				(sb_u8 *) ((sb_u8 *) p_packet_info->p_data + GPRS_NS_HD_LEN);
		p_packet_info->data_len = p_packet_info->data_len - GPRS_NS_HD_LEN;

		ret = RET_SUCC;
	}
	else
	{
		return ret;
	}

	return ret;
}

sb_s32 sp_process_gb(sp_packet_t *p_packet_info, gxio_mpipe_idesc_t *pdesc)
{
	sb_s32 ret = RET_FAIL;

	memset((sb_s8*)&g_gb_info, 0, sizeof(sp_gb_t));

	ret = parse_gb_ns(p_packet_info);
	if(RET_FAIL == ret)
	{
		return ret;
	}

	ret = sp_get_bssgp_head(p_packet_info, &g_gb_info);
	if(RET_FAIL == ret)
	{
		return ret;
	}

	ret = sp_get_llc_head(p_packet_info, &g_gb_info);
	if(RET_FAIL == ret)
	{
		return ret;
	}

	//GMM数据
	if (g_gb_info->llc.sapi == 1)
	{
		ret = sp_get_gmm_head(p_packet_info, &g_gb_info);
		if(RET_FAIL == ret)
		{
			return ret;
		}
		add_to_xdr_queue(&g_xdr);
		add_to_ui_queue (&g_ui);

		ret = RET_SUCC;
	}
	else
	{
		add_to_pack_queue (&g_pack);
		ret = RET_SUCC;
	}

	return ret;
}
