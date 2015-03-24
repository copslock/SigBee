/*
 * base_lib_queue.c
 *
 *  Created on: 2014年6月4日
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
#include "sb_xdr_struct.h"
#include "sb_error_code.h"
#include "sb_log.h"
#include "sb_base_util.h"
#include "sb_shared_queue.h"
#include "sb_base_mica.h"
#include "sb_base_mem.h"
#include "sb_base_process.h"

sb_sp_user_queue_head_t *g_sp_user_queue_head = NULL; //!< sp与dp之间的用户信息队列全局指针
sb_sp_packet_queue_head_t *g_sp_packet_queue_head = NULL; //!< sig_parser到data_parser的数据包信息队列全局指针
sb_sp_xdr_queue_head_t *g_sp_xdr_queue_head = NULL; //!< sig_parser到xdr_sender的队列全局指针
sb_dp_xdr_queue_head_t *g_dp_xdr_queue_head = NULL; //!< data_parser到xdr_sender的队列全局指针
sb_dp_user_queue_head_t *g_dp_user_queue_head = NULL; //!< data_parser到ui_sender的用户队列全局指针

#include "sb_public_var.h"

/**********************************************************************
  函数说明：初始化sig_parser到data_parser模块的用户信息队列

  输入参数：无
  输出参数：无
  全局变量：无
  函数返回：RET_SUCC：成功;RET_FAIL：失败
  修改记录：无
  备    注：注意是无锁结构，如果可以，封装一下存包读包
 **********************************************************************/

sb_s32 init_sp_user_share_queues(void)
{
	sb_s32 i = 0;

	g_sp_user_queue_head = (sb_sp_user_queue_head_t*)tmc_cmem_malloc(sizeof(sb_sp_user_queue_head_t));
	memset(g_sp_user_queue_head,0,sizeof(sb_sp_user_queue_head_t));

	if(NULL == g_sp_user_queue_head)
	{
		printf("g_sp_user_queue_head 分配内存失败! line %d\n",__LINE__);
		return RET_FAIL;
	}

	g_sp_user_queue_head->num = g_adv_config.sig_parser_cpus * g_adv_config.data_parser_cpus;//default 4*15


	g_sp_user_queue_head->addr = tmc_cmem_malloc(sizeof(sp_user_info_queue_t)*g_sp_user_queue_head->num);
	if(g_sp_user_queue_head->addr == NULL)
	{
		fprintf(stderr,"g_sb_packet_queue_head->addr 分配内存失败!\n");
		return RET_FAIL;
	}
	memset(g_sp_user_queue_head->addr,0, sizeof(sp_user_info_queue_t)*g_sp_user_queue_head->num);


	for(i = 0; i< g_sp_user_queue_head->num; i++)
	{
		sp_user_info_queue_t *p = &g_sp_user_queue_head->addr[i];

		p->size = g_adv_config.sig_user_queue_size;
		tmc_alloc_t alloc = TMC_ALLOC_INIT;
//		tmc_alloc_set_huge(&alloc);
		tmc_alloc_set_shared(&alloc);
		p->user_infos = tmc_alloc_map(&alloc, sizeof(sp_user_info_t)*p->size);
		if(p->user_infos == NULL)
		{
			fprintf(stderr,"g_sp_user_queue_head->user_infos 分配内存失败!\n");
			return RET_FAIL;
		}
		memset(p->user_infos,0,sizeof(sp_user_info_t)*p->size);
	}

	return RET_SUCC;
}

sb_s32 destroy_sp_user_share_queues(void)
{
	sb_s32 i = 0;
	if(NULL == g_sp_user_queue_head)
	{
		fprintf(stderr," 销毁队列时全局指针g_sp_user_queue_head为NULL\n");
		return RET_FAIL;
	}
	for(i = 0; i<g_sp_user_queue_head->num;i++)
	{
		if(NULL != g_sp_user_queue_head->addr[i].user_infos)tmc_alloc_unmap(g_sp_user_queue_head->addr[i].user_infos, sizeof(sp_user_info_t)*g_sp_user_queue_head->addr[i].size);   //释放每个队列
		g_sp_user_queue_head->addr[i].user_infos = NULL;


	}
	if(NULL != g_sp_user_queue_head->addr)tmc_cmem_free(g_sp_user_queue_head->addr);
	g_sp_user_queue_head->addr = NULL;
	if(NULL != g_sp_user_queue_head)tmc_cmem_free(g_sp_user_queue_head);

	return RET_SUCC;
}

/**********************************************************************
  函数说明：初始化sig_parser到data_parser模块的数据包信息队列

  输入参数：无
  输出参数：无
  全局变量：无
  函数返回：RET_SUCC：成功;RET_FAIL：失败
  修改记录：无
  备    注：注意是无锁结构，如果可以，封装一下存包读包
 **********************************************************************/

sb_s32 init_sp_packet_share_queues(void)
{
	sb_s32 i = 0;

	g_sp_packet_queue_head = (sb_sp_packet_queue_head_t*)tmc_cmem_malloc(sizeof(sb_sp_packet_queue_head_t));
	memset(g_sp_packet_queue_head,0,sizeof(sb_sp_packet_queue_head_t));

	if(NULL == g_sp_packet_queue_head)
	{
		printf("g_sp_packet_queue_head 分配内存失败! line %d\n",__LINE__);
		return RET_FAIL;
	}

	g_sp_packet_queue_head->num = g_adv_config.sig_parser_cpus * g_adv_config.data_parser_cpus;//default 4*15


	g_sp_packet_queue_head->addr = tmc_cmem_malloc(sizeof(sp_packet_queue_t)*g_sp_packet_queue_head->num);
	if(g_sp_packet_queue_head->addr == NULL)
	{
		fprintf(stderr,"g_sp_packet_queue_head->addr 分配内存失败!\n");
		return RET_FAIL;
	}
	memset(g_sp_packet_queue_head->addr,0, sizeof(sp_packet_queue_t)*g_sp_packet_queue_head->num);


	for(i = 0; i< g_sp_packet_queue_head->num; i++)
	{
		sp_packet_queue_t *p = &g_sp_packet_queue_head->addr[i];

		p->size = g_adv_config.sig_data_queue_size;
		tmc_alloc_t alloc = TMC_ALLOC_INIT;
//		tmc_alloc_set_huge(&alloc);
		tmc_alloc_set_shared(&alloc);
		p->sp_packet = tmc_alloc_map(&alloc, sizeof(sp_packet_info_t)*p->size);
		if(p->sp_packet == NULL)
		{
			fprintf(stderr,"g_sp_packet_queue_head->sp_packet 分配内存失败!\n");
			return RET_FAIL;
		}
		memset(p->sp_packet,0,sizeof(sp_packet_info_t)*p->size);
	}

	return RET_SUCC;
}

sb_s32 destroy_sp_packet_share_queues(void)
{
	sb_s32 i = 0;
	if(NULL == g_sp_packet_queue_head)
	{
		fprintf(stderr," 销毁队列时全局指针g_sp_packet_queue_head为NULL\n");
		return RET_FAIL;
	}
	for(i = 0; i<g_sp_packet_queue_head->num;i++)
	{
		if(NULL != g_sp_packet_queue_head->addr[i].sp_packet)tmc_alloc_unmap(g_sp_packet_queue_head->addr[i].sp_packet, sizeof(sp_packet_info_t)*g_sp_packet_queue_head->addr[i].size);   //释放每个队列
		g_sp_packet_queue_head->addr[i].sp_packet = NULL;


	}
	if(NULL != g_sp_packet_queue_head->addr)tmc_cmem_free(g_sp_packet_queue_head->addr);
	g_sp_packet_queue_head->addr = NULL;
	if(NULL != g_sp_packet_queue_head)tmc_cmem_free(g_sp_packet_queue_head);

	return RET_SUCC;
}

/**********************************************************************
  函数说明：初始化sig_parser到xdr_sender模块的数据包信息队列

  输入参数：无
  输出参数：无
  全局变量：无
  函数返回：RET_SUCC：成功;RET_FAIL：失败
  修改记录：无
  备    注：注意是无锁结构，如果可以，封装一下存包读包
 **********************************************************************/

sb_s32 init_sp_xdr_share_queues(void)
{
	sb_s32 i = 0;

	g_sp_xdr_queue_head = (sb_sp_xdr_queue_head_t*)tmc_cmem_malloc(sizeof(sb_sp_xdr_queue_head_t));
	memset(g_sp_xdr_queue_head,0,sizeof(sb_sp_xdr_queue_head_t));

	if(NULL == g_sp_xdr_queue_head)
	{
		printf("g_sp_xdr_queue_head 分配内存失败! line %d\n",__LINE__);
		return RET_FAIL;
	}

	g_sp_xdr_queue_head->num = g_adv_config.sig_parser_cpus * g_adv_config.xdr_sender_cpus;//default 4*15


	g_sp_xdr_queue_head->addr = tmc_cmem_malloc(sizeof(sp_xdr_queue_t)*g_sp_xdr_queue_head->num);
	if(g_sp_xdr_queue_head->addr == NULL)
	{
		fprintf(stderr,"g_sp_xdr_queue_head->addr 分配内存失败!\n");
		return RET_FAIL;
	}
	memset(g_sp_xdr_queue_head->addr,0, sizeof(sp_xdr_queue_t)*g_sp_xdr_queue_head->num);


	for(i = 0; i< g_sp_xdr_queue_head->num; i++)
	{
		sp_xdr_queue_t *p = &g_sp_xdr_queue_head->addr[i];

		p->size = g_adv_config.sig_xdr_queue_size;
		tmc_alloc_t alloc = TMC_ALLOC_INIT;
//		tmc_alloc_set_huge(&alloc);
		tmc_alloc_set_shared(&alloc);
		p->lb_xdrs = tmc_alloc_map(&alloc, sizeof(sp_xdr_t)*p->size);
		if(p->lb_xdrs == NULL)
		{
			fprintf(stderr,"g_sp_xdr_queue_head->lb_xdrs 分配内存失败!\n");
			return RET_FAIL;
		}
		memset(p->lb_xdrs,0,sizeof(sp_xdr_t)*p->size);
	}

	return RET_SUCC;
}

sb_s32 destroy_sp_xdr_share_queues(void)
{
	sb_s32 i = 0;
	if(NULL == g_sp_xdr_queue_head)
	{
		fprintf(stderr," 销毁队列时全局指针g_sp_xdr_queue_head为NULL\n");
		return RET_FAIL;
	}
	for(i = 0; i<g_sp_xdr_queue_head->num;i++)
	{
		if(NULL != g_sp_xdr_queue_head->addr[i].lb_xdrs)tmc_alloc_unmap(g_sp_xdr_queue_head->addr[i].lb_xdrs, sizeof(sp_xdr_t)*g_sp_xdr_queue_head->addr[i].size);   //释放每个队列
		g_sp_xdr_queue_head->addr[i].lb_xdrs = NULL;


	}
	if(NULL != g_sp_xdr_queue_head->addr)tmc_cmem_free(g_sp_xdr_queue_head->addr);
	g_sp_xdr_queue_head->addr = NULL;
	if(NULL != g_sp_xdr_queue_head)tmc_cmem_free(g_sp_xdr_queue_head);

	return RET_SUCC;
}

/**********************************************************************
  函数说明：初始化data_parser到xdr_sender模块的数据包信息队列

  输入参数：无
  输出参数：无
  全局变量：无
  函数返回：RET_SUCC：成功;RET_FAIL：失败
  修改记录：无
  备    注：注意是无锁结构，如果可以，封装一下存包读包
 **********************************************************************/

sb_s32 init_dp_xdr_share_queues(void)
{
	sb_s32 i = 0;

	g_dp_xdr_queue_head = (sb_dp_xdr_queue_head_t*)tmc_cmem_malloc(sizeof(sb_dp_xdr_queue_head_t));
	memset(g_dp_xdr_queue_head,0,sizeof(sb_dp_xdr_queue_head_t));

	if(NULL == g_dp_xdr_queue_head)
	{
		printf("g_dp_xdr_queue_head 分配内存失败! line %d\n",__LINE__);
		return RET_FAIL;
	}

	g_dp_xdr_queue_head->num = g_adv_config.data_parser_cpus * g_adv_config.xdr_sender_cpus;//default 4*15


	g_dp_xdr_queue_head->addr = tmc_cmem_malloc(sizeof(dp_xdr_queue_t)*g_dp_xdr_queue_head->num);
	if(g_dp_xdr_queue_head->addr == NULL)
	{
		fprintf(stderr,"g_dp_xdr_queue_head->addr 分配内存失败!\n");
		return RET_FAIL;
	}
	memset(g_dp_xdr_queue_head->addr,0, sizeof(dp_xdr_queue_t)*g_dp_xdr_queue_head->num);


	for(i = 0; i< g_dp_xdr_queue_head->num; i++)
	{
		dp_xdr_queue_t *p = &g_dp_xdr_queue_head->addr[i];

		p->size = g_adv_config.data_xdr_queue_size;
		tmc_alloc_t alloc = TMC_ALLOC_INIT;
//		tmc_alloc_set_huge(&alloc);
		tmc_alloc_set_shared(&alloc);
		p->lb_xdrs = tmc_alloc_map(&alloc, sizeof(dp_xdr_t)*p->size);
		if(p->lb_xdrs == NULL)
		{
			fprintf(stderr,"g_dp_xdr_queue_head->lb_xdrs 分配内存失败!\n");
			return RET_FAIL;
		}
		memset(p->lb_xdrs,0,sizeof(dp_xdr_t)*p->size);
	}

	return RET_SUCC;
}

sb_s32 destroy_dp_xdr_share_queues(void)
{
	sb_s32 i = 0;
	if(NULL == g_dp_xdr_queue_head)
	{
		fprintf(stderr," 销毁队列时全局指针g_dp_xdr_queue_head为NULL\n");
		return RET_FAIL;
	}
	for(i = 0; i<g_dp_xdr_queue_head->num;i++)
	{
		if(NULL != g_dp_xdr_queue_head->addr[i].lb_xdrs)tmc_alloc_unmap(g_dp_xdr_queue_head->addr[i].lb_xdrs, sizeof(dp_xdr_t)*g_dp_xdr_queue_head->addr[i].size);   //释放每个队列
		g_dp_xdr_queue_head->addr[i].lb_xdrs = NULL;


	}
	if(NULL != g_dp_xdr_queue_head->addr)tmc_cmem_free(g_dp_xdr_queue_head->addr);
	g_dp_xdr_queue_head->addr = NULL;
	if(NULL != g_dp_xdr_queue_head)tmc_cmem_free(g_dp_xdr_queue_head);

	return RET_SUCC;
}

/**********************************************************************
  函数说明：初始化data_parser到ui_sender模块的用户信息队列

  输入参数：无
  输出参数：无
  全局变量：无
  函数返回：RET_SUCC：成功;RET_FAIL：失败
  修改记录：无
  备    注：注意是无锁结构，如果可以，封装一下存包读包
 **********************************************************************/

sb_s32 init_dp_user_share_queues(void)
{
	sb_s32 i = 0;

	g_dp_user_queue_head = (sb_dp_user_queue_head_t*)tmc_cmem_malloc(sizeof(sb_dp_user_queue_head_t));
	memset(g_dp_user_queue_head,0,sizeof(sb_dp_user_queue_head_t));

	if(NULL == g_dp_user_queue_head)
	{
		printf("g_dp_user_queue_head 分配内存失败! line %d\n",__LINE__);
		return RET_FAIL;
	}

	g_dp_user_queue_head->num = g_adv_config.data_parser_cpus * g_device_config.dpi_dev_num;//default 4*15


	g_dp_user_queue_head->addr = tmc_cmem_malloc(sizeof(dp_user_info_queue_t)*g_dp_user_queue_head->num);
	if(g_dp_user_queue_head->addr == NULL)
	{
		fprintf(stderr,"g_sb_packet_queue_head->addr 分配内存失败!\n");
		return RET_FAIL;
	}
	memset(g_dp_user_queue_head->addr,0, sizeof(dp_user_info_queue_t)*g_dp_user_queue_head->num);


	for(i = 0; i< g_dp_user_queue_head->num; i++)
	{
		dp_user_info_queue_t *p = &g_dp_user_queue_head->addr[i];

		p->size = g_adv_config.dpi_user_queue_size;
		tmc_alloc_t alloc = TMC_ALLOC_INIT;
//		tmc_alloc_set_huge(&alloc);
		tmc_alloc_set_shared(&alloc);
		p->user_infos = tmc_alloc_map(&alloc, sizeof(dp_user_info_t)*p->size);
		if(p->user_infos == NULL)
		{
			fprintf(stderr,"g_dp_user_queue_head->user_infos 分配内存失败!\n");
			return RET_FAIL;
		}
		memset(p->user_infos,0,sizeof(dp_user_info_t)*p->size);
	}

	return RET_SUCC;
}

sb_s32 destroy_dp_user_share_queues(void)
{
	sb_s32 i = 0;
	if(NULL == g_dp_user_queue_head)
	{
		fprintf(stderr," 销毁队列时全局指针g_dp_user_queue_head为NULL\n");
		return RET_FAIL;
	}
	for(i = 0; i<g_dp_user_queue_head->num;i++)
	{
		if(NULL != g_dp_user_queue_head->addr[i].user_infos)tmc_alloc_unmap(g_dp_user_queue_head->addr[i].user_infos, sizeof(dp_user_info_t)*g_dp_user_queue_head->addr[i].size);   //释放每个队列
		g_dp_user_queue_head->addr[i].user_infos = NULL;


	}
	if(NULL != g_dp_user_queue_head->addr)tmc_cmem_free(g_dp_user_queue_head->addr);
	g_dp_user_queue_head->addr = NULL;
	if(NULL != g_dp_user_queue_head)tmc_cmem_free(g_dp_user_queue_head);

	return RET_SUCC;
}
