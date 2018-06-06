/*
 * sig_parse.c
 *
 *  Created on: 2014年6月10日
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
#include "sig_packet.h"
#include "sig_fragment.h"
#include "sig_user.h"
#include "sig_xdr.h"

/*********************************************************/
/*
 * 修改记录:
 * V0.1:	1、初始版本；
			2、2014/6/10，温富发

 */
/*********************************************************/
/* 宏定义 */
#define SIG_PARSER_VERSION "0.1"

gxio_mpipe_iqueue_t *gg_iqueue_p = NULL;
gxio_mpipe_iqueue_t *g_iqueue_p = NULL;

sp_user_queue_info_t *g_sp_user_queue_info_p = NULL;
sp_packet_queue_info_t *g_sp_packet_queue_info_p = NULL;
sp_xdr_queue_info_t *g_sp_xdr_queue_info_p = NULL;

sp_statistic_t* g_sp_statistic = NULL;	//!< sig_parser统计信息结构
sp_per_statistics_t sp_per_static;//性能测试统计计数器

sb_u32 g_sp_frag_timeout_s = 0;//IP分片老化区间,s——>e
sb_u32 g_sp_frag_timeout_e = MAX_IP_FRAG_HASH_NUM;

sb_u64	g_sp_current_uid = 0;	//!< 当前分配的用户ID，需要加上下面的偏移量
sb_u64 g_sp_uid_offset = 0;

sb_u64	g_sp_current_xdrid = 0;	//!< 当前分配的XDR ID，需要加上下面的偏移量
sb_u64 g_sp_xdrid_offset = 0;

/*!
  \fn void sig_parser_main(void)
  \brief 进程主流程
  \par sig_parser_main函数流程图:
  \n
 */

/**********************************************************************
  函数说明：sig_parser进程主运行函数

  输入参数：无
  argv：参数串

  输出参数：无

  全局变量：无
  函数返回：RET_SUCC：成功
  RET_FAIL：失败

  修改记录：无
  备    注：
 **********************************************************************/

void sig_parser_main (void)
{

	sb_s32 ret = 0;
	sb_u32 i;

	if(tmc_cmem_init(0) <0 )
	{
		SB_FPRINT(stderr,"join cmem failed\n");
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "join cmem failed");
		return;
	}

	sb_s8 ver_info[64], ver_time[32];

	//! \par main函数主要流程:
	//! \n
	memset (ver_info, 0, 64);
#ifdef DEBUG_VER
	strcpy (ver_info, "Debug ");
#endif

#ifdef BUILD_DATE
	sprintf (ver_time, "Build %u.%u ", BUILD_DATE, BUILD_TIME);
	strcat (ver_info, ver_time);
#endif

	SB_FPRINT (stdout, "sig_parser version: %s-%s %s\n", EXE_BUILD, SIG_PARSER_VERSION, ver_info);
	SB_ADD_LOG (LOG_LEVEL_FAILURE)
	(SB_LOG_INFO, 0, "sig_parser version: %s-%s %s", EXE_BUILD, SIG_PARSER_VERSION, ver_info);

	atexit (exit_sig_parse);

	//这里需要知道本sig_parser所在的mpipe队列，然后循环收包即可

	if(g_mpipe_info == NULL)
	{
		SB_FPRINT(stderr,"%s[%d]:g_mpipe_info is NULL\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "g_mpipe_info is NULL ");
		return ;
	}


	if(g_mpipe_info->iqueue_num < g_process_order+1)
	{
		SB_FPRINT(stderr,"%s[%d]:mpipe iqueue_num < process_order!\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR ,0 ,"mpipe iqueue_num< process_order!");
		return;
	}

	if(g_sp_user_queue_head == NULL)
	{
		SB_FPRINT(stderr,"%s[%d]:g_sp_user_queue_head is NULL\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "g_sp_user_queue_head is NULL!");
		return ;

	}

	if(g_sp_packet_queue_head == NULL)
	{
		SB_FPRINT(stderr,"%s[%d]:g_sp_packet_queue_head is NULL\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "g_sp_packet_queue_head is NULL!");
		return ;

	}

	if(g_sp_xdr_queue_head == NULL)
	{
		SB_FPRINT(stderr,"%s[%d]:g_sp_xdr_queue_head is NULL\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "g_sp_xdr_queue_head is NULL!");
		return ;

	}

	if(g_process_exit == NULL)
	{
		SB_FPRINT(stderr,"%s[%d]: g_process_exit is NULL\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "g_process_exit  is NULL!");

		return;
	}

	gg_iqueue_p = g_mpipe_info->iqueues[g_process_order].iqueue ;

	if(gg_iqueue_p == NULL)
	{

		SB_FPRINT(stderr,"%s[%d]: g_iqueue_p is NULL!\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "g_iqueue_p is NULL!");
		return;
	}

	g_sp_frag_timeout_s = g_process_order * (MAX_IP_FRAG_HASH_NUM % g_adv_config.sig_parser_cpus);
	if(g_process_order < (g_adv_config.sig_parser_cpus - 1))g_sp_frag_timeout_e = g_sp_frag_timeout_s + MAX_IP_FRAG_HASH_NUM % g_adv_config.sig_parser_cpus;
	g_sp_uid_offset = g_process_order * 0x100000000000000;
	g_sp_xdrid_offset = ((g_device_config.device_identity << 8) + tmc_cpus_get_my_cpu()) * 0x1000000000000;

	//get iqueue into cache
	tmc_mem_prefetch((void *)gg_iqueue_p,sizeof(gxio_mpipe_iqueue_t));


	g_sp_statistic = &g_sp_statistics[g_process_order];

	if( g_sp_statistic == NULL)
	{
		SB_FPRINT(stderr,"%s[%d]: g_sp_statistic is NULL!\n",g_process_name,g_process_order);
		SB_ADD_LOG(LOG_LEVEL_FAILURE)
		(SB_LOG_ERR, 0, "g_sp_statistic is NULL");
	}
	SB_FPRINT(stdout,"%s[%d]: g_sp_statistic  %p \n",g_process_name,g_process_order,g_sp_statistic);


	//更新cpu_id
	g_sp_statistic->cpu_id = tmc_cpus_get_my_cpu();

	//计算所对应的queue数
	ret = alloc_lp_queues();

	if(RET_FAIL == ret)
	{

		SB_FPRINT(stderr,"%s[%d]分配数组失败!\n",g_process_name,g_process_order);
		SB_ADD_LOG (LOG_LEVEL_FAILURE)
		(SB_LOG_ERR,0,"分配数组失败!");

		return;
	}


/*
	for(i = 0; i < MAX_HASH; i++)
	{
		lb_tlli_core_s[i] = NULL;
	}
*/

	memset(&sp_per_static, 0, sizeof(sp_per_statistics_t));
	SB_ADD_LOG (LOG_LEVEL_FAILURE)
	(SB_LOG_INFO,0," packet_loop_inetrace");
	SB_FPRINT(stdout,"before loop\n");

	sp_packets_loop();

	return;// RET_SUCC;
	//! </ul>
}

/*
   退出程序处理 :

 */
void exit_sig_parse (void)
{
//	release_tlli_core_link();
	if(g_sp_user_queue_info_p)
	{
		if(g_sp_user_queue_info_p->queue_array)tmc_alloc_unmap(g_sp_user_queue_info_p->queue_array, sizeof(sp_user_info_queue_t*)*g_sp_user_queue_info_p->num);
		tmc_alloc_unmap(g_sp_user_queue_info_p, sizeof(sp_user_queue_info_t));
		g_sp_user_queue_info_p = NULL;
	}

	if(g_sp_packet_queue_info_p)
	{
		if(g_sp_packet_queue_info_p->queue_array)tmc_alloc_unmap(g_sp_packet_queue_info_p->queue_array, sizeof(sp_packet_queue_t*)*g_sp_packet_queue_info_p->num);
		tmc_alloc_unmap(g_sp_packet_queue_info_p, sizeof(sp_packet_queue_info_t));
		g_sp_packet_queue_info_p = NULL;
	}

	if(g_sp_xdr_queue_info_p)
	{
		if(g_sp_xdr_queue_info_p->queue_array)tmc_alloc_unmap(g_sp_xdr_queue_info_p->queue_array, sizeof(sp_xdr_queue_t*)*g_sp_xdr_queue_info_p->num);
		tmc_alloc_unmap(g_sp_xdr_queue_info_p, sizeof(sp_xdr_queue_info_t));
		g_sp_xdr_queue_info_p = NULL;
	}

	SB_FPRINT (stderr, "%s[%d:%d]退出\n", g_process_name, g_process_order, g_cpu_id);
	SB_ADD_LOG (LOG_LEVEL_FAILURE)
	(SB_LOG_INFO, 0, "exit sig_parser process");
	return;
}

//计算分配队列
sb_s32 alloc_all_queues(void)
{
	sb_s32 i;

	if(g_sp_user_queue_head == NULL)
	{
		SB_FPRINT (stdout, "g_sp_user_queue_head is NULL!\n");
		return RET_FAIL;
	}

	if(g_sp_packet_queue_head == NULL)
	{
		SB_FPRINT (stdout, "g_sp_packet_queue_head is NULL!\n");
		return RET_FAIL;
	}

	if(g_sp_xdr_queue_head == NULL)
	{
		SB_FPRINT (stdout, "g_sp_xdr_queue_head is NULL!\n");
		return RET_FAIL;
	}

	SB_ADD_LOG(LOG_LEVEL_DEBUG )
	(SB_LOG_INFO, 0, " try to alloc  g_sp_user_queue_info_p queue size is num is %d", g_sp_user_queue_head->num );


	// 使用一个数组计算所有sig_parser所对应的data_parser user数组

	tmc_alloc_t alloc = TMC_ALLOC_INIT;

	g_sp_user_queue_info_p = tmc_alloc_map(&alloc,sizeof(sp_user_queue_info_t));
	if(NULL == g_sp_user_queue_info_p )
	{
		SB_FPRINT(stderr,"alloc g_sp_user_queue_info_p error!\n");
		return RET_FAIL;
	}

	g_sp_user_queue_info_p->num = g_adv_config.data_parser_cpus;


	g_sp_user_queue_info_p->queue_array = tmc_alloc_map(&alloc,sizeof(sp_user_info_queue_t*)*g_sp_user_queue_info_p->num);
	if(NULL == g_sp_user_queue_info_p->queue_array )
	{
		SB_FPRINT(stderr,"alloc g_sp_user_queue_info_p->query_array error! size is %ld",sizeof(sp_user_info_queue_t*)*g_sp_user_queue_info_p->num);
		return RET_FAIL;
	}
	SB_ADD_LOG(LOG_LEVEL_DEBUG )
	(SB_LOG_INFO, 0, "alloc g_sp_user_queue_info_p queue num is %d", g_sp_user_queue_info_p->num );

	for(i = 0 ; i< g_sp_user_queue_info_p->num; i++)
	{
		g_sp_user_queue_info_p->queue_array[i] = &g_sp_user_queue_head->addr[i * g_adv_config.sig_parser_cpus + g_process_order];
		SB_ADD_LOG(LOG_LEVEL_DEBUG) (SB_LOG_INFO,0,"get g_sp_user_queue_info_p queue num %d ", i * g_adv_config.sig_parser_cpus + g_process_order);
	}

	SB_ADD_LOG(LOG_LEVEL_DEBUG )
	(SB_LOG_INFO, 0, " try to alloc  g_sp_packet_queue_info_p queue size is num is %d", g_sp_packet_queue_head->num );


	// 使用一个数组计算所有sig_parser所对应的data_parser user数组

	tmc_alloc_t alloc = TMC_ALLOC_INIT;

	g_sp_packet_queue_info_p = tmc_alloc_map(&alloc,sizeof(sp_packet_queue_info_t));
	if(NULL == g_sp_packet_queue_info_p )
	{
		SB_FPRINT(stderr,"alloc g_sp_packet_queue_info_p error!\n");
		return RET_FAIL;
	}

	g_sp_packet_queue_info_p->num = g_adv_config.data_parser_cpus;


	g_sp_packet_queue_info_p->queue_array = tmc_alloc_map(&alloc,sizeof(sp_packet_queue_t*)*g_sp_packet_queue_info_p->num);
	if(NULL == g_sp_packet_queue_info_p->queue_array )
	{
		SB_FPRINT(stderr,"alloc g_sp_packet_queue_info_p->query_array error! size is %ld",sizeof(sp_packet_queue_t*)*g_sp_packet_queue_info_p->num);
		return RET_FAIL;
	}
	SB_ADD_LOG(LOG_LEVEL_DEBUG )
	(SB_LOG_INFO, 0, "alloc g_sp_packet_queue_info_p queue num is %d", g_sp_packet_queue_info_p->num );

	for(i = 0 ; i< g_sp_packet_queue_info_p->num; i++)
	{
		g_sp_packet_queue_info_p->queue_array[i] = &g_sp_packet_queue_head->addr[i * g_adv_config.sig_parser_cpus + g_process_order];
		SB_ADD_LOG(LOG_LEVEL_DEBUG) (SB_LOG_INFO,0,"get g_sp_packet_queue_info_p queue num %d ", i * g_adv_config.sig_parser_cpus + g_process_order);
	}

	SB_ADD_LOG(LOG_LEVEL_DEBUG )
	(SB_LOG_INFO, 0, " try to alloc  g_sp_xdr_queue_info_p queue size is num is %d", g_sp_xdr_queue_head->num );


	// 使用一个数组计算所有sig_parser所对应的data_parser user数组

	tmc_alloc_t alloc = TMC_ALLOC_INIT;

	g_sp_xdr_queue_info_p = tmc_alloc_map(&alloc,sizeof(sp_xdr_queue_info_t));
	if(NULL == g_sp_xdr_queue_info_p )
	{
		SB_FPRINT(stderr,"alloc g_sp_xdr_queue_info_p error!\n");
		return RET_FAIL;
	}

	g_sp_xdr_queue_info_p->num = g_adv_config.xdr_sender_cpus;


	g_sp_xdr_queue_info_p->queue_array = tmc_alloc_map(&alloc,sizeof(sp_packet_queue_t*)*g_sp_xdr_queue_info_p->num);
	if(NULL == g_sp_xdr_queue_info_p->queue_array )
	{
		SB_FPRINT(stderr,"alloc g_sp_xdr_queue_info_p->query_array error! size is %ld",sizeof(sp_packet_queue_t*)*g_sp_xdr_queue_info_p->num);
		return RET_FAIL;
	}
	SB_ADD_LOG(LOG_LEVEL_DEBUG )
	(SB_LOG_INFO, 0, "alloc g_sp_xdr_queue_info_p queue num is %d", g_sp_xdr_queue_info_p->num );

	for(i = 0 ; i< g_sp_xdr_queue_info_p->num; i++)
	{
		g_sp_xdr_queue_info_p->queue_array[i] = &g_sp_xdr_queue_head->addr[i * g_adv_config.sig_parser_cpus + g_process_order];
		SB_ADD_LOG(LOG_LEVEL_DEBUG) (SB_LOG_INFO,0,"get g_sp_xdr_queue_info_p queue num %d ", i * g_adv_config.sig_parser_cpus + g_process_order);
	}

	return RET_SUCC;
}


/*!
  \fn void sp_packets_loop( void )
  \brief 数据包循环处理过程，循环获取数据包，并启动数据包处理的回调函数。
  \return void
  \par sp_packets_loop函数流程图:
  \n
 */

/**********************************************************************
  函数说明：数据包循环处理过程，循环获取数据包，并启动数据包处理的回调函数。
  输入参数：无
  输出参数：无
  全局变量：g_device_config

  g_process_exit
  函数返回：无
  修改记录：无
  备    注：
 **********************************************************************/
void sp_packets_loop (void)
{
	//! \par sp_packets_loop函数主要流程:
	//! \n
	//! <ul>
	//现在只有1种读包的方式，不用考虑其他
	sb_s32 ret = 0;
	gxio_mpipe_idesc_t *idescs = NULL ;

	//	sb_s32 count = 0;

	sb_u64 packet_start_cycle = 0;
	sb_u64 packet_end_cycle = 0;
	sb_u64 packet_now_cycle;
	sb_u32 i, k = 0;
	sb_u64 check_time_last = g_current_time;
	sb_u32 log_time_last = g_current_time & 0xFFFFFFFF;
	sb_u32 frag_time_last = g_current_time & 0xFFFFFFFF;

	g_sp_statistic->max_process_time = 0;
	g_sp_statistic->min_process_time = 0;
	g_sp_statistic->total_process_time = 0;

//	fill_lb_tlli_core_s();
//	update_tlli_core_link(0xa1234cd4, 0x14abcdef);
	g_iqueue_p = gg_iqueue_p;

	while(1!= *g_process_exit )
	{
		ret = gxio_mpipe_iqueue_try_peek(g_iqueue_p,&idescs);

		if(ret <0 && ret != GXIO_MPIPE_ERR_IQUEUE_EMPTY)
		{
//		    SB_FPRINT (stdout, "ret = %d\n", ret);
			break;
		}
		//接收到数据包
		if(ret>0 && NULL!=idescs )
		{
			if(((g_current_time & 0xFFFFFFFF) - (check_time_last & 0xFFFFFFFF)) >= 2)//2s检查一次超时
			{
//				timeout_tlli_core_link(lb_tlli_core_s, 3600);//用户表3600秒超时

				check_time_last = g_current_time;
			}
			if(((g_current_time & 0xFFFFFFFF) - log_time_last) > 600)//600s记录一次日志
			{
				sp_log_pack_info();
				log_time_last = g_current_time  & 0xFFFFFFFF;
			}
			if(((g_current_time & 0xFFFFFFFF) - frag_time_last) > 1800)//1800s IP分片队列超时检查
			{
				timeout_node_from_frag_pool();
				frag_time_last = g_current_time  & 0xFFFFFFFF;
			}

			for (int i = 0; i < ret; i++)
			{
				gxio_mpipe_idesc_t* pdesc = &idescs[i];


				if(gxio_mpipe_idesc_has_error(pdesc) == 1)// has error
				{

					g_sp_statistic->packet_err_count++;
					g_sp_statistic->packet_err_size += pdesc->l2_size;
					  if(pdesc->be)g_sp_statistic->be_err_count++;
					  if(pdesc->me)g_sp_statistic->me_err_count++;
					  if(pdesc->tr)g_sp_statistic->tr_err_count++;
					  if(pdesc->ce)g_sp_statistic->ce_err_count++;

					gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
					gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);


				}
				else
				{
					  if((gxio_mpipe_idesc_get_status(pdesc) & 0x80) != 0)g_sp_statistic->cus_err_count++;
					  if(pdesc->cs && (pdesc->csum_seed_val != 0xFFFF))g_sp_statistic->cs_err_count++;

					  g_sp_statistic->packet_count++;
					  g_sp_statistic->packet_size += pdesc->l2_size;

					packet_start_cycle = get_cycle_count();

					sp_process_packet_mpipe(pdesc);
					g_iqueue_p = gg_iqueue_p;
					//				gxio_mpipe_iqueue_drop(g_iqueue_p,pdesc);
					//				gxio_mpipe_iqueue_consume(g_iqueue_p,pdesc);

					packet_end_cycle = get_cycle_count();
					packet_now_cycle = packet_end_cycle - packet_start_cycle;
					if( 0 == g_sp_statistic->min_process_time)
					{
						g_sp_statistic->min_process_time = packet_now_cycle;
					}
					if((sb_u32)packet_now_cycle < g_sp_statistic->min_process_time)
						g_sp_statistic->min_process_time = packet_now_cycle;

					if((sb_u32)packet_now_cycle > g_sp_statistic->max_process_time)
						g_sp_statistic->max_process_time = packet_now_cycle;

					g_sp_statistic->total_process_time += packet_now_cycle;
				}
				//SB_FPRINT (stdout, "process packet ok\n");
			}
		}
		//没有数据包
		else
		  {


		    for(i = 0; i < 150; i++)//sleep 1us
		      {
		        cycle_relax();//6 cycles
		      }

		    k++;

		    if((k % 30000000) == 0)//30s
		      {
		        g_current_time += 30;
		        g_current_time &= 0xFFFFFFFF;

//		        timeout_tlli_core_link(lb_tlli_core_s, 3600);//用户表3600秒超时
				check_time_last = g_current_time;

				if(((g_current_time & 0xFFFFFFFF) - (log_time_last & 0xFFFFFFFF)) > 600)//600s记录一次日志
				{
					sp_log_pack_info();
					log_time_last = g_current_time;
				}

		        k = 0;
		      }
		  }
	}

}

//记录与数据包有关的信息
void sp_log_pack_info()
{
	SB_ADD_LOG (LOG_LEVEL_FAILURE)
	(SB_LOG_INFO, 0, "Rec(%lu, %lu, %lu, %lu, %lu, %u); "
			"Fwd(%lu, %lu); Error(%lu, %lu, %lu, %lu, %lu, %lu)",
			g_sp_statistic->packet_count, g_sp_statistic->packet_size,
			g_sp_statistic->packet_err_count,  g_sp_statistic->packet_err_size,
			g_sp_statistic->total_process_time, g_sp_statistic->max_process_time,
			g_sp_statistic->packet_fwd_count, g_sp_statistic->packet_fwd_fail_count,
			g_sp_statistic->be_err_count, g_sp_statistic->me_err_count,
			g_sp_statistic->tr_err_count,  g_sp_statistic->ce_err_count,
			g_sp_statistic->cs_err_count, g_sp_statistic->cus_err_count);


	memset((sb_s8*)g_sp_statistic, 0, sizeof(sp_statistic_t));

	SB_ADD_LOG (LOG_LEVEL_FAILURE)
			(SB_LOG_INFO, 0,
					"add_to_lb_lp_queue_f: (%lu, %lu, %lu) , update_tlli_core_link_f: (%lu, %lu, %lu), "
					"timeout_tlli_core_link_f: (%lu, %lu, %lu), get_tlli_f: (%lu, %lu, %lu)",
					sp_per_static.add_to_user_info_queue_f.run_count, sp_per_static.add_to_user_info_queue_f.run_total_cycle,
					sp_per_static.add_to_user_info_queue_f.run_max_cycle,  sp_per_static.update_tlli_core_link_f.run_count,
					sp_per_static.update_tlli_core_link_f.run_total_cycle, sp_per_static.update_tlli_core_link_f.run_max_cycle,
					sp_per_static.timeout_tlli_core_link_f.run_count, sp_per_static.timeout_tlli_core_link_f.run_total_cycle,
					sp_per_static.timeout_tlli_core_link_f.run_max_cycle,  sp_per_static.get_tlli_f.run_count,
					sp_per_static.get_tlli_f.run_total_cycle, sp_per_static.get_tlli_f.run_max_cycle);
//	max_lb_tlli_node_num = 0;
	memset((sb_s8*)&sp_per_static, 0, sizeof(sp_per_statistics_t));

	return ;
}

