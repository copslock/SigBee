/*
 * sig_gb_llc.c
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
#include "sig_gb_llc.h"


sb_s32 sp_get_llc_head(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}
	data = p_packet_info->p_data;

	if (NULL == data || p_packet_info->data_len < LLC_HEAD_LEN + 3) {
		return RET_FAIL;
	}

	if ((data[0] & 0x80) == 0x80) {
		return RET_FAIL;
	}

	if ((data[1] & 0xC0) != 0xC0) {
		return RET_FAIL;
	}

	gb_info->llc.sapi = (data[0] & 0x0F);

	p_packet_info->p_data = data + 3;
	p_packet_info->data_len -= 3;
	p_packet_info->data_len -= 3;//去除FCS的长度

	return ret;
}

