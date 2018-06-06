/*
 * main.c
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

//tilera 相关头文件
#include <tmc/cmem.h>
#include <tmc/mem.h>
#include <tmc/sync.h>
#include <tmc/cpus.h>
#include <tmc/spin.h>
#include <tmc/task.h>
#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_xdr_struct.h"
#include "sb_error_code.h"
#include "init.h"
#include "sb_base_util.h"
#include "sb_shared_queue.h"
#include "sb_base_mpipe.h"
#include "sb_base_process.h"
#include "sb_log.h"
#include "sb_logd.h"
#include "sb_qlog.h"
#include "sb_statistic.h"
#include "sb_sp.h"

/*全局变量*/
sb_u32* g_process_exit = NULL;  //!< 进程退出全局变量
sb_u64 g_current_time;  //!< 全局时间
//当前进程的模块名
sb_s8 g_process_name[MAX_PROCESS_NAME_SIZE] = {0,};
//当前进程在模块号
sb_s32 g_process_id = 0;
//当前进程模块序号
sb_s32 g_process_order = 0;
//当前进程的cpu号 由于可能绑定cpu序号可能为0，所以默认为-1为不绑定
sb_s32 g_cpu_id = -1;

//是否发送数据包，0：不发送，其他值：发送
sb_s32 g_is_send_packet = 0;

pthread_key_t g_thread_key;

device_config_t g_device_config;        //!< device配置文件读入后的结构
adv_config_t g_adv_config;	//!< adv.ini配置文件读入后的结构
sb_s32 g_proto_ips_num[MAX_PRO_IP_LINK_NUM];	//!< 无线协议识别IP个数
ip_proto_t g_proto_ips[MAX_PRO_IP_LINK_NUM][MAX_PRO_IP_NODE_NUM];	//!< 无线协议识别IP列表
sb_s32 g_ps_filter_num = 0;		//!< PS全量过滤条件个数
ps_filter_rule_t g_ps_filter[MAX_PS_RULE_NUM];	//!< PS全量过滤条件

sp_statistic_t *g_sp_statistics = NULL; //!< 计数器入口指针,是一个数组，其大小为sig_parser进程数量
dp_statistic_t *g_dp_statistics = NULL; //!< 计数器入口指针,是一个数组，其大小为data_parser进程数量

#include "sb_public_var.h"

/*
void sig_parser_main (void)
{
	return;
}

void data_parser_main (void)
{
	return;
}

void xdr_sender_main(sb_s32 argc, sb_s8 ** argv)
{
	return;
}

void ui_sender_main(void)
{
	return;
}
*/


/*******************************************************************
 *
 * 1.0.0:
 * 		1、初始版本；
 * 		2、2014/6/6，温富发
 ******************************************************************/

/*!
  \fn sb_s32 main( sb_s32 argc , sb_s8 ** argv )
  \brief 进程主流程
  \param[in] argc 命令行参数个数
  \param[in] argv 命令行参数指针数组
  \return sb_s32
  \retval RET_SUCC 成功
  \retval RET_FAIL 失败
  \par main函数流程图:
  \n
 */

/**********************************************************************
  函数说明：进程主流程

  输入参数：argc:参数个数
  argv：参数串

  输出参数：无

  全局变量：无
  函数返回：RET_SUCC：成功
  RET_FAIL：失败

  修改记录：无
  备    注：
 **********************************************************************/
sb_s32 main (sb_s32 argc, sb_s8 ** argv)
{
	sb_s32 ret = 0;
	sb_s8 ver_info[64], ver_time[32];

	g_process_id = PROCESS_ID_SB;					//进程类型
	g_process_order = 0;						//进程序号
	strncpy(g_process_name,PROCESS_NAME_SB,MAX_PROCESS_NAME_SIZE-1); //进程名

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

	parse_cmd_line(argc, argv);
	fprintf (stdout, "SigBee version: %s %s\n", EXE_BUILD, ver_info);

	if(g_cmd_options.show_version == TRUE)
	{
		return 0;
	}

	if(g_cmd_options.show_usage == TRUE)
	{
		fprintf(stdout, "Usage: sigbee -v -h\n");
		fprintf(stdout, "       -v Show version\n");
		fprintf(stdout, "       -h Show usage\n");
		return 0;
	}

	// 进程实例初始化函数
	//! <ul><li> 检查运行实例，提取运行参数，daemon形式运行处理  //  之后可以使用log
	ret = init_instance (argc, argv);
	if (RET_FAIL == ret)
	{
		fprintf (stderr, "init_instance error！\n");
		return RET_FAIL;
	}
	// 全局初始化函数 (init只需要初始化共享的数据结构即可， 各种缓冲队列 及 共享策略区  ,但主配置文件需要读取 并且一些数据结构是共享的

	//! <li> 读取配置文件，共享内存检查，

	ret = init_initialize (argc, argv);   //之后可以使用log
	if (RET_FAIL == ret)
	{
		fprintf (stderr, "initialize error! \n");
		return RET_FAIL;
	}


	ret = create_process(argc,argv);

	if (RET_FAIL == ret)
	{
		fprintf(stderr, "创建子进程失败! \n");
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0, "[%s]:创建子进程失败!",__FUNCTION__);

		return RET_FAIL;
	}
	/*	ADD_LOG (LOG_LEVEL_DEBUG)
			(0, 0, "创建子进程成功!");*/


	switch(g_process_id)
	{
	case PROCESS_ID_LOGD:
	{
		fprintf(stdout, "%s[%d]: 进入logd main函数!\n",g_process_name,g_process_order);

		sb_write_log();

		*g_process_exit = 1;

		ADD_LOG (LOG_LEVEL_DEBUG)
		(0,0,"退出logd函数 !");

		break;
	}

	case PROCESS_ID_SIG_PARSER:
	{
		SB_FPRINT(stdout, "%s[%d]: 进入sig_parser_main函数!\n",g_process_name,g_process_order);

		sig_parser_main();

		*g_process_exit = 1;

		SB_ADD_LOG (LOG_LEVEL_DEBUG)
		(SB_LOG_INFO,0,"退出sig_parser_main函数 !");

		break;
	}
	case PROCESS_ID_DATA_PARSER:
	{
		fprintf(stdout, "%s[%d]: 进入data_parser_main函数!\n",g_process_name,g_process_order);
		data_parser_main();

		*g_process_exit = 1;

		ADD_LOG (LOG_LEVEL_DEBUG)
		(0,0,"退出data_parser_main函数 !");

		break;
	}
	case PROCESS_ID_XDR_SENDER:
	{
		fprintf(stdout, "%s[%d]: 进入xdr_sender_main函数!\n",g_process_name,g_process_order);

		xdr_sender_main(argc,argv);

		*g_process_exit = 1;

		ADD_LOG (LOG_LEVEL_DEBUG)
		(0,0,"退出xdr_sender_main函数 !");

		break;

	}
	case PROCESS_ID_UI_SENDER:
	{
		fprintf(stdout, "%s[%d]: 进入ui_serder_main函数!\n",g_process_name,g_process_order);

		ui_sender_main();

		*g_process_exit = 1;

		ADD_LOG (LOG_LEVEL_DEBUG)
		(0,0,"退出ui_serder_main函数 !");

		break;
	}

	case PROCESS_ID_SB:
	{
		atexit (delete_name_mutex);
		/*退出函数处理  */

		atexit (exit_sb);

		sb_s32 num = g_process_info_p->num -1 ;
		sb_u32 time_num = 0;
		while(1)
		{
			//每小时同步1次时间
			if((time_num % 3600) == 0)
			{
				struct timespec ts;

				g_current_time = time (NULL);
				gxio_mpipe_get_timestamp(&g_mpipe_info->context, &ts);

				ts.tv_sec = g_current_time;
				gxio_mpipe_set_timestamp(&g_mpipe_info->context, &ts);
			}

			if(1 == *g_process_exit)
			{
				fprintf(stdout, "%s[%d]: g_process_exit 设为退出 ，等待所有子进程退出 !\n",g_process_name,g_process_order);
				ADD_LOG (LOG_LEVEL_NOTICE)
				(0, 0, "%s[%d]: g_process_exit 设为退出 ，等待所有子进程退出 !",g_process_name,g_process_order);
				kill(0, SIGTERM);
				//wait所有子进程然后退出
				for(sb_s32 i = 0; i< num-1; i++)
				{
					wait(NULL);
				}
				fprintf(stdout, "%s[%d]: 进程退出 !\n",g_process_name,g_process_order);
				ADD_LOG (LOG_LEVEL_NOTICE)
				(0, 0, "%s[%d]: 进程退出 !",g_process_name,g_process_order);

				break;
			}

			sleep(5);
			time_num += 5;
		}
		break;
	}
	}

	return RET_SUCC;

}


/*!
  \fn sb_s32 init_instance( sb_s32 argc , sb_s8 ** argv )
  \brief 进程实例初始化，检查进程是否重复启动，根据命令行参数选择启动模式
  \param[in] argc 命令行参数个数
  \param[in] argv 命令行参数指针数组
  \return sb_s32
  \retval RET_SUCC 成功
  \retval RET_FAIL 失败
  \par init_instance函数流程图:
  \n
 */

/**********************************************************************
  函数说明：进程实例初始化，检查进程是否重复启动，根据命令行参数选择启动模式

  输入参数：sb_u16 argc  命令行参数个数
		sb_u8** argv  命令行参数串

  输出参数：无

  全局变量：无
  函数返回：sb_s32  实例初始化，RET_SUCC：成功;RET_FAIL：失败

  修改记录：无
  备    注：
 **********************************************************************/
sb_s32 init_instance (sb_s32 argc, sb_s8 ** argv)
{
	sb_s32 ret = 0;

	//! \par init_instance函数主要流程:
	//! \n
	//! <ul>

	//! <li> 运行实例检查

	ret = check_process (PROCESS_NAME_SB, argc, (sb_s8 **) argv);
	if (RET_TRUE == ret)
	{
		fprintf (stderr, "process gc is already running!\n");
		return RET_FAIL;
	}
	//! <li> 启动daemon模式
	ret = start_daemon (PROCESS_NAME_SB, argc, (sb_s8 **) argv);
	//! <li> 判断启动函数返回值
	//! <ol>
	switch (ret)
	{
	//! <li> 异常，返回失败信息
	case RET_FAIL:
	{
		fprintf (stderr, "以daemon方式启动失败！\n");
		return RET_FAIL;
	}
	break;
	//! <li> daemon方式启动，打印提示信息
	default:
	{
		fprintf (stdout, "SigBee 进程以后台方式运行!\n");
	}
	break;
	}
	//! </ol>

	// 初始化成功
	return RET_SUCC;
	//! </ul>
}

/*!
  \fn sb_s32 initialize( sb_s32 argc , sb_s8 ** argv )
  \brief 内部结构初始化
  \param[in] argc 命令行参数个数
  \param[in] argv 命令行参数指针数组
  \return sb_s32
  \retval RET_SUCC 成功
  \retval RET_FAIL 失败
  \par initialize函数流程图:
  \n
 */

/**********************************************************************
  函数说明：全局结构初始化

  输入参数：无

  输出参数：无

  全局变量：无
  函数返回：RET_SUCC：成功;RET_FAIL：失败

  修改记录：无
  备    注：
 **********************************************************************/
sb_s32 init_initialize (sb_s32 argc, sb_s8 ** argv)
{
	//! \par initialize函数主要流程:
	//! \n
	//! <ul>

	//初始化共享内存
	if(tmc_cmem_init(MAX_SHM_SIZE)<0)
	{
		fprintf(stderr, "初始化共享内存失败! \n");
		return RET_FAIL;
	}

	tmc_cmem_persist_after_exec(1);

	sb_s32 ret = 0;

	g_log_queue=(log_queue_t*)tmc_cmem_malloc(sizeof(*g_log_queue));
	if(g_log_queue==NULL)
	{
		tmc_task_die("tmc malloc error!\n");
	}
	log_queue_init(g_log_queue);

	//! <li> 全局结构初始化，读取全局配置文件，数据包读取初始化(mpipe)，日志初始化，共享内存存在判定；
	//之后可以使用ADD_LOG!
	ret = init_config (PROCESS_NAME_SB, argc, (sb_s8 **) argv);

	if (RET_FAIL == ret)
	{
		fprintf (stderr, "初始化公共接口失败！\n");
		return RET_FAIL;
	}

	//初始化mpipe收包
	tmc_alloc_t alloc_mpipe = TMC_ALLOC_INIT;
	tmc_alloc_set_shared(&alloc_mpipe);
	ret = init_mpipe_net(&alloc_mpipe);
	if (ret == RET_FAIL) {
		fprintf(stderr, "%s[%d]: 初始化mpipe失败!\n", g_process_name,
				g_process_order);
		ADD_LOG(LOG_LEVEL_FAILURE)(0, 0, "初始化mpipe失败! \n");

		return RET_FAIL;
	}

	ret = init_share_process_info();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"初始化进程信息队列失败!\n");
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0 ,"[%s]:初始化进程信息队列失败!",__FUNCTION__);
		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0, "初始化进程信息队列成功!");

	ret = init_sp_user_share_queues();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"初始化sig_parser到data_parser的用户队列失败!\n");
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0 ,"[%s]:初始化sig_parser到data_parser的用户队列失败!",__FUNCTION__);

		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0, "初始化sig_parser到data_parser的用户队列成功!");

	ret = init_sp_packet_share_queues();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"初始化sig_parser到data_parser的数据包信息队列失败!\n");
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0 ,"[%s]:初始化sig_parser到data_parser的数据包信息队列失败!",__FUNCTION__);

		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0, "初始化sig_parser到data_parser的数据包信息队列成功!");

	ret = init_sp_xdr_share_queues();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"初始化sig_parser到xdr_sender的队列失败!\n");
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0 ,"[%s]:初始化sig_parser到xdr_sender的队列失败!",__FUNCTION__);

		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0, "初始化sig_parser到xdr_sender的队列成功!");

	ret = init_dp_xdr_share_queues();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"初始化data_parser到xdr_sender的队列失败!\n");
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0 ,"[%s]:初始化data_parser到xdr_sender的队列失败!",__FUNCTION__);

		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0, "初始化data_parser到xdr_sender的队列成功!");

	ret = init_dp_user_share_queues();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"初始化data_parser到ui_sender的用户队列失败!\n");
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0 ,"[%s]:初始化data_parser到ui_sender的用户队列失败!",__FUNCTION__);

		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0, "初始化data_parser到ui_sender的用户队列成功!");

	if(RET_SUCC != sb_statistic_init())
	{

		ADD_LOG (LOG_LEVEL_FAILURE)
									(0, 0 ,"[%s]:初始化统计内存失败!",__FUNCTION__);

		fprintf(stderr,"初始化统计内存失败!\n");
		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0 ,"初始化统计内存成功!");

	if(RET_SUCC != sp_init_frage_mem())
	{

		ADD_LOG (LOG_LEVEL_FAILURE)
									(0, 0 ,"[%s]:初始化ip碎片内存失败!",__FUNCTION__);

		fprintf(stderr,"初始化ip碎片内存失败!\n");
		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0 ,"初始化ip碎片内存成功!");

	if(RET_SUCC != sp_init_user_mem())
	{

		ADD_LOG (LOG_LEVEL_FAILURE)
									(0, 0 ,"[%s]:初始化sp用户表内存失败!",__FUNCTION__);

		fprintf(stderr,"初始化sp用户表内存失败!\n");
		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0 ,"初始化sp用户表内存成功!");

	if(RET_SUCC != sp_init_tlli_mem())
	{

		ADD_LOG (LOG_LEVEL_FAILURE)
									(0, 0 ,"[%s]:初始化sp tlli用户表内存失败!",__FUNCTION__);

		fprintf(stderr,"初始化sp tlli用户表内存失败!\n");
		return RET_FAIL;
	}
	ADD_LOG (LOG_LEVEL_DEBUG)
	(0, 0 ,"初始化sp tlli用户表内存成功!");

	return RET_SUCC;
}

/*
   退出程序处理 :
   1.断开新接口机连接
   2.退出所有已登记的线程
 */
void exit_sb (void)
{
	release_resource();
	close_mpipe_net();
	tmc_cmem_close();

}


/* 全局函数的实现体*/

/**********************************************************************
函数说明：初始化系统
输入参数：sb_u8 *process_name：进程名
          sb_u16 argc：参数个数
          sb_u8 **argv：参数列表

输出参数：无

全局变量：g_process_name
          g_cfg_shm_info
          g_device_config
          g_pfxx_packet
          g_cmd_options


函数返回：RET_SUCC：初始化成功
          RET_FAIL：初始化失败

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 init_config(sb_u8 *process_name, sb_u16 argc, sb_u8 **argv)
        \brief 初始化系统
        \param[in] process_name 进程名
        \param[in] argc 参数个数
        \param[in] argv 参数列表
        \return sb_s32
		\retval RET_FAIL 初始化失败
		\retval RET_SUCC 初始化成功
 */
sb_s32 init_config (sb_s8 * process_name, sb_u16 argc, sb_s8 ** argv)
{

	/*!
<b>全局变量:</b> \ref g_process_name, \ref g_device_config \ref g_adv_config, \ref g_proto_ips_num, \ref g_proto_ips \ref g_ps_filter_num, \ref g_ps_filter
	 */

	sb_s32 ret = 0;

	//!     - 判断参数有效性
	if (NULL == process_name)
	{
		return RET_FAIL;
	}
	if ('\0' == process_name[0])
	{
		return RET_FAIL;
	}

	//! - 初始化全局变量\ref g_process_name
	memset (g_process_name, 0, sizeof (g_process_name));

	strncpy ((char *)g_process_name, (char *)process_name, sizeof (g_process_name) - 1);


	//!     - atexit注册\ref release_resource
	/*
	if (atexit (release_resource) != 0)
	{
		release_resource ();
		return RET_FAIL;
	}
	 */

	//初始化共享全局变量
	if(init_global_vars() < 0)
	{
		return RET_FAIL;
	}

	printf("Read dev config file %s, init_config\n", DEVICE_INI_FILENAME);
	//! - 读取设备配置文件结构
	ret = read_dev_config (DEVICE_INI_FILENAME);

	if (RET_SUCC!= ret)
	{
		fprintf (stderr, "read_dev_config失败，返回%d\n", ret);
		return RET_FAIL;
	}


	printf("Read adv config file %s, init_config\n", ADV_INI_FILENAME);
	//! - 读取高级配置文件结构
	ret = read_adv_config (ADV_INI_FILENAME);

	if (RET_SUCC!= ret)
	{
		fprintf (stderr, "read_adv_config失败，返回%d\n", ret);
		return RET_FAIL;
	}

	printf("Read ip-protocol config file, init_config\n");
	//! - 读取ip文件
	ret = read_ips_config ();

	if (RET_SUCC!= ret)
	{
		fprintf (stderr, "read_ips_config失败，返回%d\n", ret);
		return RET_FAIL;
	}

	printf("Read filter config file %s, init_config\n", FILTER_INI_FILENAME);
	//! - 读取过滤规则配置文件结构
	ret = read_filter_config (FILTER_INI_FILENAME);

	if (RET_SUCC!= ret)
	{
		fprintf (stderr, "read_filter_config失败，返回%d\n", ret);
		return RET_FAIL;
	}


	//! - 初始化日志功能
	ret = init_log (g_adv_config.log_level);
	if (RET_SUCC != ret)
	{
		fprintf (stderr, "init_log失败，返回%d\n", ret);
		return RET_FAIL;
	}


	sb_s8 tmp_name[MAX_PROCESS_NAME_SIZE];
	sprintf(tmp_name,"%s.%d",g_process_name, g_process_order);

	ret = init_log_thread (tmp_name);
	if (RET_SUCC != ret)
	{
		fprintf (stderr, "init_log_thread %s失败，返回%d\n", g_process_name, ret);
		return RET_FAIL;
	}

	return RET_SUCC;
}

/*初始化全局变量函数 */
sb_s32 init_global_vars(void)
{
	g_process_exit = tmc_cmem_malloc(sizeof(sb_s32));
	if(g_process_exit == NULL)
		return -1;
	*g_process_exit = 0;
	tmc_mem_fence();

	return 0;
}

/**********************************************************************
函数说明：进程退出时，释放公共资源
输入参数：无

输出参数：无

全局变量：

函数返回：无

修改记录：无
备    注：无
 **********************************************************************/

/*!
        \fn void release_resource(void)
        \brief 进程退出时，释放公共资源
        \param[in] 无
        \param[out] 无
        \return void
 */
void release_resource (void)
{
	printf("SigBee Release resource\n");

	if(g_sp_statistics)tmc_cmem_free(g_sp_statistics);
	if(g_dp_statistics)tmc_cmem_free(g_dp_statistics);
	destroy_sp_user_share_queues();
	destroy_sp_packet_share_queues();
	destroy_sp_xdr_share_queues();
	destroy_dp_xdr_share_queues();
	destroy_dp_user_share_queues();

	sp_destroy_frag_mem();
	sp_destroy_user_mem();
	sp_destroy_tlli_mem();

	return;
}

/* 初始化statistic */
sb_s32 sb_statistic_init(void)
{

	if(g_adv_config.sig_parser_cpus <=0 || g_adv_config.data_parser_cpus <=0)
	{
		return RET_FAIL;
	}

	//malloc sig_parser statistic
	g_sp_statistics = tmc_cmem_malloc(sizeof(sp_statistic_t) * g_adv_config.sig_parser_cpus);

	if(NULL == g_sp_statistics)
	{
		fprintf(stderr,"malloc g_sp_statistics failed!\n");
		return RET_FAIL;
	}
	memset(g_sp_statistics, 0, sizeof(sp_statistic_t) * g_adv_config.sig_parser_cpus);

	//malloc data parser statistic
	g_dp_statistics = tmc_cmem_malloc(sizeof(dp_statistic_t) * g_adv_config.data_parser_cpus);

	if(NULL == g_dp_statistics)
	{
		fprintf(stderr,"malloc g_dp_statistics failed!\n");
		return RET_FAIL;
	}
	memset(g_dp_statistics, 0, sizeof(dp_statistic_t) * g_adv_config.data_parser_cpus);

	return RET_SUCC;;
}
