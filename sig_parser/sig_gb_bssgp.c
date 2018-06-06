/*
 * sig_gb_bssgp.c
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

//offset:偏移量从字节最高位开始算
sb_u8 get_umts(sb_u8 *data, sb_u8 offset)
{
	sb_u8 umts = 0;

	if(NULL == data) return umts;
	if(offset >= 8) return umts;

	//printf("data[0] = 0x%08X, data[1] = 0x%08X, offset = %d\n", data[0], data[1], offset);
	if(data[0] & (0x80 >> offset))//DTM GPRS Multi Slot Class,4bits or 6bits
	{
		if(data[(offset + 4) / 8] & (0x80 >> ((offset + 4) % 8)))//DTM EGPRS Multi Slot Class,2bits
		{
			if(data[(offset + 7) / 8] & (0x80 >> ((offset + 7) % 8)))//8PSK Power Capabilit,2bits
			{
				if(data[(offset + 12) / 8] & (0x80 >> ((offset + 12) % 8))) umts |= 0x01;//UMTS FDD Radio Access Technology Capability
				if(data[(offset + 13) / 8] & (0x80 >> ((offset + 13) % 8))) umts |= 0x02;//UMTS 3.84 Mcps TDD Radio Access Technology Capability
				if(data[(offset + 14) / 8] & (0x80 >> ((offset + 14) % 8))) umts |= 0x04;//CDMA 2000 Radio Access Technology Capability
				if(data[(offset + 15) / 8] & (0x80 >> ((offset + 15) % 8))) umts |= 0x08;//UMTS 1.28 Mcps TDD Radio Access Technology Capability

			}
			else
			{
				if(data[(offset + 10) / 8] & (0x80 >> ((offset + 10) % 8))) umts |= 0x01;//UMTS FDD Radio Access Technology Capability
				if(data[(offset + 11) / 8] & (0x80 >> ((offset + 11) % 8))) umts |= 0x02;//UMTS 3.84 Mcps TDD Radio Access Technology Capability
				if(data[(offset + 12) / 8] & (0x80 >> ((offset + 12) % 8))) umts |= 0x04;//CDMA 2000 Radio Access Technology Capability
				if(data[(offset + 13) / 8] & (0x80 >> ((offset + 13) % 8))) umts |= 0x08;//UMTS 1.28 Mcps TDD Radio Access Technology Capability
			}
		}
		else
		{
			if(data[(offset + 5) / 8] & (0x80 >> ((offset + 5) % 8)))//8PSK Power Capabilit,2bits
			{
				if(data[(offset + 10) / 8] & (0x80 >> ((offset + 10) % 8))) umts |= 0x01;//UMTS FDD Radio Access Technology Capability
				if(data[(offset + 11) / 8] & (0x80 >> ((offset + 11) % 8))) umts |= 0x02;//UMTS 3.84 Mcps TDD Radio Access Technology Capability
				if(data[(offset + 12) / 8] & (0x80 >> ((offset + 12) % 8))) umts |= 0x04;//CDMA 2000 Radio Access Technology Capability
				if(data[(offset + 13) / 8] & (0x80 >> ((offset + 13) % 8))) umts |= 0x08;//UMTS 1.28 Mcps TDD Radio Access Technology Capability

			}
			else
			{
				if(data[(offset + 8) / 8] & (0x80 >> ((offset + 8) % 8))) umts |= 0x01;//UMTS FDD Radio Access Technology Capability
				if(data[(offset + 9) / 8] & (0x80 >> ((offset + 9) % 8))) umts |= 0x02;//UMTS 3.84 Mcps TDD Radio Access Technology Capability
				if(data[(offset + 10) / 8] & (0x80 >> ((offset + 10) % 8))) umts |= 0x04;//CDMA 2000 Radio Access Technology Capability
				if(data[(offset + 11) / 8] & (0x80 >> ((offset + 11) % 8))) umts |= 0x08;//UMTS 1.28 Mcps TDD Radio Access Technology Capability
			}
		}
	}
	else
	{
		if(data[(offset + 1) / 8] & (0x80 >> ((offset + 1) % 8)))//8PSK Power Capabilit,2bits
		{
			if(data[(offset + 6) / 8] & (0x80 >> ((offset + 6) % 8))) umts |= 0x01;//UMTS FDD Radio Access Technology Capability
			if(data[(offset + 7) / 8] & (0x80 >> ((offset + 7) % 8))) umts |= 0x02;//UMTS 3.84 Mcps TDD Radio Access Technology Capability
			if(data[(offset + 8) / 8] & (0x80 >> ((offset + 8) % 8))) umts |= 0x04;//CDMA 2000 Radio Access Technology Capability
			if(data[(offset + 9) / 8] & (0x80 >> ((offset + 9) % 8))) umts |= 0x08;//UMTS 1.28 Mcps TDD Radio Access Technology Capability

		}
		else
		{
			if(data[(offset + 4) / 8] & (0x80 >> ((offset + 4) % 8))) umts |= 0x01;//UMTS FDD Radio Access Technology Capability
			if(data[(offset + 5) / 8] & (0x80 >> ((offset + 5) % 8))) umts |= 0x02;//UMTS 3.84 Mcps TDD Radio Access Technology Capability
			if(data[(offset + 6) / 8] & (0x80 >> ((offset + 6) % 8))) umts |= 0x04;//CDMA 2000 Radio Access Technology Capability
			if(data[(offset + 7) / 8] & (0x80 >> ((offset + 7) % 8))) umts |= 0x08;//UMTS 1.28 Mcps TDD Radio Access Technology Capability
		}
	}

	return umts;
}

sb_s32 get_iei(sb_u8 *data, sb_u32 len, sb_iei_t *iei) {
	sb_s32 ret = 0;

	if (NULL == data || len <= 2) {
		return RET_FAIL;
	}

	iei->type = data[0];
	if ((data[1] & 0x80) == 0x80) {
		iei->head_len = 2;
		iei->length = (data[1] & 0x7F);
	} else {
		iei->head_len = 3;
		iei->length = ntohs(*(sb_u16*) &data[1]);
	}
	if (len < (iei->head_len + iei->length)) {
		return RET_FAIL;
	}

	iei->value = data + iei->head_len;

	return ret;
}

sb_s32 sp_get_msc(sb_u8 *data, gprs_msc_t *msc)
{
	sb_s32 ret = 0;

	if(NULL == data || NULL == msc)
	{
		return RET_FAIL;
	}
	//return ret;
	if((data[1] & 0x02) > 0)//A5 bits 0x19 13 42 33 57 2B
	{
		if((data[3] & 0x20) > 0)//Multislot capability
		{
			if((data[3] & 0x10) > 0)//HSCSD multislot class,5bits
			{
				if((data[4] & 0x40) > 0)//GPRS multislot class,6bits
				{
					msc->gprs_msc = ((data[4] & 0x3E) >> 1); //GPRS MultiSlot Class
					if((data[5] & 0x80) > 0)//SMS_VALUE and SM_VALUE,8bits
					{
						if((data[6] & 0x40) > 0)//ECSD multislot class,5bits
						{
							if((data[6] & 0x01) > 0)// EGPRS multislot class,6bits
							{
								msc->egprs_msc = ((data[7] & 0xF8) >> 3); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[7], 6);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[7], 0);
							}
						}
						else
						{
							if((data[6] & 0x20) > 0)// EGPRS multislot class
							{
								msc->egprs_msc = (data[6] & 0x1F); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[7], 1);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 3);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if((data[5] & 0x40) > 0)//ECSD multislot class
						{
							if((data[5] & 0x01) > 0)// EGPRS multislot class
							{
								msc->egprs_msc = ((data[6] & 0xF8) >> 3); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 6);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 0);
							}
						}
						else
						{
							if((data[5] & 0x20) > 0)// EGPRS multislot class
							{
								msc->egprs_msc = (data[5] & 0x1F); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 1);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 3);
							}
						}
					}
				}
				else//no GPRS multislot class
				{
					msc->gprs_msc = 0; //GPRS MultiSlot Class
					if(data[4] & 0x20)//SMS_VALUE and SM_VALUE
					{
						if(data[5] & 0x10)//ECSD multislot class
						{
							if(data[6] & 0x40)// EGPRS multislot class
							{
								msc->egprs_msc = ((data[6] & 0x3E) >> 1); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[7], 0);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 2);
							}
						}
						else
						{
							if(data[5] & 0x08)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[5] & 0x07) << 2)| ((data[6] & 0xC0) >> 6)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 3);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 5);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if(data[4] & 0x10)//ECSD multislot class
						{
							if(data[5] & 0x40)// EGPRS multislot class
							{
								msc->egprs_msc = ((data[5] & 0x3E) >> 1); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 0);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 2);
							}
						}
						else
						{
							if(data[4] & 0x08)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x07) << 2) | ((data[5] & 0xC0) >> 6)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 3);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 5);
							}
						}
					}
				}
			}
			else//no HSCSD multislot class
			{
				if(data[3] & 0x08)//GPRS multislot class
				{
					msc->gprs_msc = (((data[3] & 0x07) << 2) | ((data[4] & 0xC0) >> 6)); //GPRS MultiSlot Class
					if(data[4] & 0x10)//SMS_VALUE and SM_VALUE
					{
						if(data[5] & 0x08)//ECSD multislot class
						{
							if(data[6] & 0x20)// EGPRS multislot class
							{
								msc->egprs_msc = (data[6] & 0x1F); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[7], 1);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 3);
							}
						}
						else
						{
							if(data[5] & 0x04)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[5] & 0x03) << 3) | ((data[6] & 0xE0) >> 5)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 4);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 6);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if(data[4] & 0x08)//ECSD multislot class
						{
							if(data[5] & 0x20)// EGPRS multislot class
							{
								msc->egprs_msc = (data[5] & 0x1F); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 1);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 3);
							}
						}
						else
						{
							if(data[4] & 0x04)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x03) << 3) | ((data[5] & 0xE0) >> 5)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 4);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 6);
							}
						}
					}
				}
				else//no GPRS multislot class
				{
					msc->gprs_msc = 0; //GPRS MultiSlot Class
					if(data[3] & 0x08)//SMS_VALUE and SM_VALUE
					{
						if(data[4] & 0x04)//ECSD multislot class
						{
							if(data[5] & 0x10)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[5] & 0x07) << 2) | ((data[6] & 0xC0) >> 6)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 2);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 4);
							}
						}
						else
						{
							if(data[4] & 0x02)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x01) << 4) | ((data[5] & 0xF0) >> 4)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 5);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 7);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if(data[3] & 0x04)//ECSD multislot class
						{
							if(data[4] & 0x10)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x07) << 2) | ((data[5] & 0xC0) >> 6)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 2);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 4);
							}
						}
						else
						{
							if(data[3] & 0x02)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[3] & 0x01) << 4) | ((data[4] & 0xF0) >> 4)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 5);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[3], 7);
							}
						}
					}
				}
			}
		}
		else//no Multislot capability
		{
			msc->gprs_msc = 0; //GPRS MultiSlot Class
			msc->egprs_msc = 0; //EGPRS MultiSlot Class
			msc->umts = get_umts(&data[3], 3);
		}
	}
	else//no A5 bits,7bits
	{
		if(data[2] & 0x10)//Multislot capability
		{
			if(data[2] & 0x08)//HSCSD multislot class
			{
				if(data[3] & 0x20)//GPRS multislot class
				{
					msc->gprs_msc = (data[3] & 0x1F); //GPRS MultiSlot Class
					if(data[4] & 0x40)//SMS_VALUE and SM_VALUE
					{
						if(data[5] & 0x20)//ECSD multislot class
						{
							if(data[6] & 0x80)// EGPRS multislot class
							{
								msc->egprs_msc = ((data[6] & 0x7C) >> 2); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 7);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 1);
							}
						}
						else
						{
							if(data[5] & 0x10)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[5] & 0xF) << 1) | ((data[6] & 0x80) >> 7)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 2);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 4);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if(data[4] & 0x20)//ECSD multislot class
						{
							if(data[5] & 0x80)// EGPRS multislot class
							{
								msc->egprs_msc = ((data[5] & 0x7C) >> 2); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 7);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 1);
							}
						}
						else
						{
							if(data[4] & 0x10)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0xF) << 1) | ((data[5] & 0x80) >> 7)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 2);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 4);
							}
						}
					}
				}
				else//no GPRS multislot class
				{
					msc->gprs_msc = 0; //GPRS MultiSlot Class
					if(data[3] & 0x10)//SMS_VALUE and SM_VALUE
					{
						if(data[4] & 0x08)//ECSD multislot class
						{
							if(data[5] & 0x20)// EGPRS multislot class
							{
								msc->egprs_msc = (data[5] & 0x1F); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 1);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 3);
							}
						}
						else
						{
							if(data[4] & 0x04)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x03) << 3)| ((data[5] & 0xE0) >> 5)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 4);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 6);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if(data[3] & 0x08)//ECSD multislot class
						{
							if(data[4] & 0x20)// EGPRS multislot class
							{
								msc->egprs_msc = (data[4] & 0x1F); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 1);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 3);
							}
						}
						else
						{
							if(data[3] & 0x04)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[3] & 0x03) << 3) | ((data[4] & 0xE0) >> 5)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 4);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[3], 6);
							}
						}
					}
				}
			}
			else//no HSCSD multislot class
			{
				if(data[2] & 0x04)//GPRS multislot class
				{
					msc->gprs_msc = (((data[2] & 0x03) << 3) | ((data[3] & 0xE0) >> 5)); //GPRS MultiSlot Class
					if(data[3] & 0x08)//SMS_VALUE and SM_VALUE
					{
						if(data[4] & 0x04)//ECSD multislot class
						{
							if(data[5] & 0x10)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[5] & 0x0F) << 1) | ((data[6] & 0x80) >> 7)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[6], 2);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 4);
							}
						}
						else
						{
							if(data[4] & 0x02)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x01) << 4) | ((data[5] & 0xF0) >> 4)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 5);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 7);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if(data[3] & 0x04)//ECSD multislot class
						{
							if(data[4] & 0x10)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x0F) << 1) | ((data[5] & 0x80) >> 7)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 2);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 4);
							}
						}
						else
						{
							if(data[3] & 0x02)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[3] & 0x01) << 4) | ((data[4] & 0xF0) >> 4)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 5);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[3], 7);
							}
						}
					}
				}
				else//no GPRS multislot class
				{
					msc->gprs_msc = 0; //GPRS MultiSlot Class
					if(data[2] & 0x04)//SMS_VALUE and SM_VALUE
					{
						if(data[3] & 0x02)//ECSD multislot class
						{
							if(data[4] & 0x08)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[4] & 0x03) << 3) | ((data[5] & 0xE0) >> 5)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[5], 3);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 5);
							}
						}
						else
						{
							if(data[3] & 0x01)// EGPRS multislot class
							{
								msc->egprs_msc = ((data[4] & 0xF8) >> 3); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 6);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 0);
							}
						}
					}
					else//no SMS_VALUE and SM_VALUE
					{
						if(data[2] & 0x02)//ECSD multislot class
						{
							if(data[3] & 0x08)// EGPRS multislot class
							{
								msc->egprs_msc = (((data[3] & 0x03) << 3) | ((data[4] & 0xE0) >> 5)); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[4], 3);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[3], 5);
							}
						}
						else
						{
							if(data[2] & 0x01)// EGPRS multislot class
							{
								msc->egprs_msc = ((data[4] & 0xF8) >> 3); //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[3], 6);
							}
							else
							{
								msc->egprs_msc = 0; //EGPRS MultiSlot Class
								msc->umts = get_umts(&data[3], 0);
							}
						}
					}
				}
			}
		}
		else
		{
			msc->gprs_msc = 0; //GPRS MultiSlot Class
			msc->egprs_msc = 0; //EGPRS MultiSlot Class
			msc->umts = get_umts(&data[2], 4);
		}
	}

	return ret;
}

sb_s32 sp_get_bssgp_head(sp_packet_t *p_packet_info, sp_gb_t *gb_info) {
	sb_s32 ret = 0;
	sb_u8 *data = NULL;
	sb_u32 len = 0;
	sb_iei_t iei;
	gprs_msc_t gprs_msc;

	if (NULL == p_packet_info || NULL == gb_info) {
		return RET_FAIL;
	}

	data = p_packet_info->p_data;
	len = p_packet_info->data_len;

	if (NULL == data || len < MIN_BSSGP_LEN) {
		return RET_FAIL;
	}

	if (data[0] > 0x01) {
		return RET_FAIL;
	} else {
		gb_info->bssgp.pdu_type = data[0];
	}

	if (gb_info->bssgp.pdu_type == 0x00) //down link
	{
		data += 8; //go to option iei
		len -= 8;

		ret = get_iei(data, len, &iei);

		while (RET_SUCC == ret) {
			switch (iei.type) {
			case BSSGP_MSRAC: {
				if(iei.length < 8)break;
				sp_get_msc(iei.value, (gprs_msc_t*)&gprs_msc);

				gb_info->bssgp.gprs_msc = gprs_msc.gprs_msc;
				gb_info->bssgp.egprs_msc = gprs_msc.egprs_msc;
				gb_info->bssgp.umts = gprs_msc.umts;
				//printf("bssgp: gprs_msc = 0x%02X, egprs_msc = 0x%02X\n", lp_packet->bssgp.gprs_msc,lp_packet->bssgp.egprs_msc);

				break;
			}
			case BSSGP_IMSI: {
				sb_u8 *vdata = iei.value;

				if(iei.length < 2)break;

				if ((vdata[0] & 0x08) == 0x08) {
					gb_info->bssgp.imsi = bcd_to_int(vdata, iei.length, 1);
				} else {
					gb_info->bssgp.imsi = bcd_to_int(vdata,
							iei.length, 0);
				}

				break;
			}
			case BSSGP_TLLI: {
				sb_u32 *tlli = (sb_u32 *)((sb_u8*)iei.value);

				gb_info->bssgp.old_tlli = ntohl(*tlli);

				break;
			}
			case BSSGP_LLC_PDU: {
				gb_info->bssgp.llc_len = iei.length;

				break;
			}
			default:
				break;
			}


			if (iei.type == BSSGP_LLC_PDU)
			{
				data = data + iei.head_len;
				len = len - iei.head_len;
				break;
			}
			data = data + iei.head_len + iei.length;
			len = len - iei.head_len - iei.length;
			ret = get_iei(data, len, &iei);
		}
		p_packet_info->p_data = data;
		p_packet_info->data_len = len;
	} else //up link
	{
		if (len < MIN_UP_BSSGP_LEN) {
			return RET_FAIL;
		}
		data += 8; //go to ci
		len -= 8;

		ret = get_iei(data, len, &iei);

		while (RET_SUCC == ret) {
			switch (iei.type) {
			case BSSGP_CELL_IDENTIFIER: {
				sb_u8 *vdata = iei.value;

				if(iei.length < 7)break;

				gb_info->bssgp.mcc = (vdata[0] & 0x0F) * 100
						+ ((vdata[0] >> 4) & 0x0F) * 10 + (vdata[1] & 0x0F);
				if ((vdata[1] & 0xF0) == 0xF0) //两位数
				{
					gb_info->bssgp.mnc = (vdata[2] & 0x0F) * 10
							+ ((vdata[2] >> 4) & 0x0F);
				} else //三位数
				{
					gb_info->bssgp.mnc = (vdata[2] & 0x0F) * 100
							+ ((vdata[2] >> 4) & 0x0F) * 10
							+ ((vdata[1] >> 4) & 0x0F);
				}

				gb_info->bssgp.lac = ntohs(*(sb_u16*) &vdata[3]);
				gb_info->bssgp.rac = vdata[5];

				gb_info->bssgp.ci = ntohs(*(sb_u16*) &vdata[6]);
				break;
			}
			case BSSGP_IMSI: {
				sb_u8 *vdata = iei.value;

				if(iei.length < 2)break;

				if ((vdata[0] & 0x08) == 0x08) {
					gb_info->bssgp.imsi = bcd_to_int(vdata, iei.length, 1);
				} else {
					gb_info->bssgp.imsi = bcd_to_int(vdata,
							iei.length, 0);
				}

				break;
			}
			case BSSGP_TLLI: {
				sb_u32 *tlli = (sb_u32 *)((sb_u8*)iei.value);

				gb_info->bssgp.old_tlli = ntohl(*tlli);

				break;
			}
			case BSSGP_LLC_PDU: {
				gb_info->bssgp.llc_len = iei.length;

				break;
			}
			default:
				break;
			}


			if (iei.type == BSSGP_LLC_PDU)
			{
				data = data + iei.head_len;
				len = len - iei.head_len;
				break;
			}

			data = data + iei.head_len + iei.length;
			len = len - iei.head_len - iei.length;

			ret = get_iei(data, len, &iei);
		}
		p_packet_info->p_data = data;
		p_packet_info->data_len = len;

	}

	return ret;
}
