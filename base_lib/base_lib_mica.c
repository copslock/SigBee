/*
 * base_lib_mica.c
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//tilera 相关头文件
#include <tmc/cmem.h>
#include <tmc/mem.h>
#include <tmc/cpus.h>
#include <gxio/mpipe.h>
#include <gxcr/gxcr.h>
#include <gxio/mica.h>
#include <tmc/sync.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_error_code.h"
#include "sb_base_mica.h"
#include "sb_base_mem.h"
#include "sb_public_var.h"
#include "sb_log.h"

mica_info_t g_mica_info ;
mica_md5_t g_mica_md5;

/* 处理时所有 memcpy没有改成fast_memcpy 基本小于 108 */
//mica内存分配 init
/* */
sb_s32 init_mica(mica_t* mica_t_p ,sb_s32 mica_type)
{
		  sb_s32 ret;
		  if(NULL == mica_t_p)
		  {
					 return RET_FAIL;
		  }

		  if( MICA_TYPE_CRYPTO != mica_type && MICA_TYPE_COMP != mica_type)
		  {
					 return RET_FAIL;
		  }

		  if(MICA_TYPE_CRYPTO == mica_type)
		  {
					 ret = gxio_mica_init( &mica_t_p->cb,  GXIO_MICA_ACCEL_CRYPTO , 0);
					 if(ret < 0)
					 {
//								fprintf(stderr, "gxio_mica_init  cpypto error :%d GXIO_ERR_BUSY  !\n",ret,GXIO_ERR_BUSY );
								switch(ret)
								{
										  case GXIO_ERR_BUSY:
													 fprintf(stderr, "init_mica CRYPTO failed, GXIP_ERR_BUSY\n");
													 break;
										  case GXIO_ERR_NO_DEVICE:
													 fprintf(stderr, "init_mica CRYPTO failed, GXIO_ERR_NO_DEVICE\n");
													 break;
										  default:
													 break;

								}

								return RET_FAIL;
					 }
					 mica_t_p->mica_type = mica_type;
					 tmc_alloc_init(&mica_t_p->alloc);
					 tmc_alloc_set_home(&mica_t_p->alloc, tmc_cpus_get_my_current_cpu());
					 mica_t_p->mem_size =  getpagesize();
//					 mica_t_p->mem_size = tmc_alloc_get_huge_pagesize();
//					 tmc_alloc_set_huge(&mica_t_p->alloc);
					 mica_t_p->mem = tmc_alloc_map(&mica_t_p->alloc, mica_t_p->mem_size);
					 ret =  gxio_mica_register_page(&mica_t_p->cb, mica_t_p->mem,mica_t_p->mem_size, 0);
					 if(ret <0)
					 {
							fprintf(stderr," gxio_mica_register_page MICA_TYPE_CRYPTO failed !\n");
					 }
					 mica_t_p->used = 1;
					 return RET_SUCC;
		  }

		  if(MICA_TYPE_COMP == mica_type)
		  {
					 ret = gxio_mica_init( &mica_t_p->cb,  GXIO_MICA_ACCEL_COMP , 0);
					 if(ret < 0)
					 {
//								fprintf(stderr, "gxio_mica_init  cpypto error :%d GXIO_ERR_BUSY  !\n",ret,GXIO_ERR_BUSY );
								switch(ret)
								{
										  case GXIO_ERR_BUSY:
													 fprintf(stderr, "init_mica COMP failed, GXIP_ERR_BUSY\n");
													 break;
										  case GXIO_ERR_NO_DEVICE:
													 fprintf(stderr, "init_mica COMP failed, GXIO_ERR_NO_DEVICE\n");
													 break;
										  default:
													 break;

								}

								return RET_FAIL;
					 }
					 mica_t_p->mica_type = mica_type;
					 tmc_alloc_init(&mica_t_p->alloc);
					 tmc_alloc_set_home(&mica_t_p->alloc, tmc_cpus_get_my_current_cpu());
					 mica_t_p->mem_size = tmc_alloc_get_huge_pagesize();
					 tmc_alloc_set_huge(&mica_t_p->alloc);
					 mica_t_p->mem = tmc_alloc_map(&mica_t_p->alloc, mica_t_p->mem_size);
					 ret =  gxio_mica_register_page(&mica_t_p->cb, mica_t_p->mem,mica_t_p->mem_size, 0);
					 if(ret <0)
					 {
							fprintf(stderr," gxio_mica_register_page MICA_TYPE_COMP failed !\n");
					 }

					 mica_t_p->used = 1;
					 return RET_SUCC;

			}
			return RET_FAIL;

}


sb_s32 init_mica_info(sb_s32 flag)
{
	if((flag & MICA_TYPE_CRYPTO) == MICA_TYPE_CRYPTO)
	{
			sb_s32 ret = init_mica(&g_mica_info.mica_crypto, MICA_TYPE_CRYPTO);
			if(ret < 0)
			{
					fprintf(stderr," init_mica g_mica_info,mica_crypto  failed !\n");
					return RET_FAIL;
			}
	}
	if((flag & MICA_TYPE_COMP) == MICA_TYPE_COMP)
	{
			sb_s32 ret = init_mica(&g_mica_info.mica_comp, MICA_TYPE_COMP);
			if(ret < 0)
			{
					fprintf(stderr," init_mica g_mica_info,mica_comp  failed !\n");
					return RET_FAIL;
			}
	}

	return RET_SUCC;

}

//md5编码,直接计算，cpu等待计算结束
sb_s32 do_mica_md5(void *srcdata, sb_s32 len,void *dstdata)
{
	if(1!=g_mica_info.mica_crypto.used)
	{
		return RET_FAIL;
	}
	sb_s32 ret = RET_SUCC;


	if(NULL == srcdata || NULL == dstdata)
		return RET_FAIL;
	if(gxio_mica_is_busy(&g_mica_info.mica_crypto.cb))
	{
		gxcr_wait_for_completion(&g_mica_info.mica_crypto.cb);
	}

	fast_memcpy(g_mica_md5.src_addr,srcdata,len);

	ret = gxcr_do_op( &g_mica_info.mica_crypto.cb,&g_mica_md5.encrypt_context, g_mica_md5.src_addr, g_mica_md5.dst_addr ,len);

	if(ret<0)
		return RET_FAIL;
	void * enc_digest = g_mica_md5.dst_addr+len;
//	ADD_LOG(LOG_LEVEL_DEBUG)(0, 0, "after do md5 ! ");

	fast_memcpy(dstdata, enc_digest, GXCR_MD5_DIGEST_SIZE);
	g_mica_md5.src_length = 0;
	return RET_SUCC;
}



/* md5全局结构体的初始化 */
sb_s32 init_mica_md5(void)
{

	g_mica_md5.src_addr = g_mica_info.mica_crypto.mem;
	if(NULL == g_mica_md5.src_addr)
	{
		return RET_FAIL;
	}
	g_mica_md5.max_src_length = g_mica_info.mica_crypto.mem_size/4;
	g_mica_md5.enc_metadata_length = g_mica_md5.max_src_length;
	g_mica_md5.dst_addr = g_mica_md5.src_addr+ g_mica_md5.max_src_length;
	g_mica_md5.enc_metadata = g_mica_md5.dst_addr + g_mica_md5.max_src_length;

	sb_s32 ret = gxcr_init_context(&g_mica_md5.encrypt_context, g_mica_md5.enc_metadata,g_mica_md5.enc_metadata_length,GXCR_CIPHER_NONE, GXCR_DIGEST_MD5, NULL,NULL,1,1,0);
	if(ret<0)
		return RET_FAIL;
	return RET_SUCC;
}
/*异步计算md5码，开始计算 */
sb_s32 start_mica_md5(void *srcdata, sb_s32 len)
{
	if(1!=g_mica_info.mica_crypto.used)
	{
		return RET_FAIL;
	}
	sb_s32 ret = RET_SUCC;


	fast_memcpy(g_mica_md5.src_addr,srcdata,len);
	if(NULL == srcdata )
		return RET_FAIL;
	if(gxio_mica_is_busy(&g_mica_info.mica_crypto.cb))									//如果忙，等待
	{
		gxcr_wait_for_completion(&g_mica_info.mica_crypto.cb);
	}

	ret = gxcr_start_op( &g_mica_info.mica_crypto.cb,&g_mica_md5.encrypt_context, g_mica_md5.src_addr, g_mica_md5.dst_addr ,len);

	if(ret<0)
	{
		g_mica_md5.src_length = 0;
    	ADD_LOG(LOG_LEVEL_FAILURE)(0, 0, "start_op failed ! ");

		return RET_FAIL;
	}
	else
	{
		g_mica_md5.src_length = len;
		return RET_SUCC;
	}

}
/* 异步等待md5码计算，如果返回FAIL 则md5码已经由md5库计算完成 */
sb_s32 wait_for_mica_md5(void *dstdata)
{
	if(0 == g_mica_md5.src_length)						//长度为0不需要等待，直接返回FAIL
	{
	ADD_LOG(LOG_LEVEL_FAILURE)(0, 0, "g_mica_md5.src_length is 0 ! ");

		return RET_FAIL;
	}
	if(gxio_mica_is_busy(&g_mica_info.mica_crypto.cb))
	{
		gxcr_wait_for_completion(&g_mica_info.mica_crypto.cb);

	}
	void * enc_digest = g_mica_md5.dst_addr+g_mica_md5.src_length;
	fast_memcpy(dstdata, enc_digest, g_mica_md5.src_length);
	g_mica_md5.src_length = 0;
	return RET_SUCC;
}


