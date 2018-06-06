/*
 * sb_process.c
 *
 *  Created on: 2014年6月8日
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
#include <errno.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>

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
#include "sb_error_code.h"
#include "sb_public_var.h"
#include "sb_base_process.h"
#include "sb_public_var.h"
#include "sb_qlog.h"
#include "sb_base_file.h"

/**********************************************************************
函数说明：运行实例检查，判定process_name指定的进程是否已经运行
输入参数：sb_u8 *process_name: 程序名称
          sb_u16 argc: 参数个数
          sb_u8 **argv: 参数列表

输出参数：无

全局变量：g_process_name  g_process_exit

函数返回：RET_TRUE：已经启动
          RET_FALSE：未启动
          RET_FAIL：失败，直接退出

修改记录：无
备    注：调用本函数通常在程序初始化时，此时还没有启动报警线程，故输出错误
          仅仅是简单的向标准出错输出
**********************************************************************/

/*!
        \fn sb_s32 check_process(sb_u8 *process_name, sb_u16 argc, sb_u8 **argv)
        \brief 运行实例检查，判定process_name指定的进程是否已经运行
        \param[in] process_name 程序名称
        \param[in] argc 参数个数
        \param[in] argv 参数列表
        \param[out] 无
        \return sb_s32
		\retval RET_TRUE 已经启动
		\retval RET_FALSE 未启动
		\retval RET_FAIL 失败，直接退出
*/
sb_s32 check_process (sb_s8 * process_name, sb_u16 argc, sb_s8 ** argv)
{

/*!
<b>全局变量:</b> \ref g_process_name, \ref g_process_exit
*/

/*!
<b>流程:</b>
*/
	sb_s8 named_mutex[SB_BUF_LEN_SMALL * 2] = { 0 };	//用于有名互斥量名称组装
	sb_s32 ret;

	//!     - 判断参数有效性
	if (NULL == process_name)
	{
		fprintf (stderr, "check_process 错误，参数process_name无效\n");
		exit (RET_FAIL);
	}
	if ('\0' == process_name[0])
	{
		fprintf (stderr, "check_process 错误，参数process_name无效\n");
		exit (RET_FAIL);
	}

	strncpy ((sb_s8 *)g_process_name, (sb_s8 *)process_name, sizeof (g_process_name) - 1);

	snprintf ((sb_s8 *)named_mutex, sizeof (named_mutex), "%s%s",  process_name, NAME_MUTEX_POS);

	if ('\0' == named_mutex[0])
	{
		fprintf (stderr, "check_process 错误，named_mutex为空\n");
		exit (RET_FAIL);
	}

	//!     - 判断程序是否已在运行
	sem_t* sem_ret = sem_open(named_mutex , O_CREAT|O_EXCL,0644,1 );

	if (SEM_FAILED == sem_ret)//程序在运行 直接返回TRUE
	{
		fprintf(stdout,"named_mutex:%s 存在! errno:%d \n",named_mutex,errno);
		return RET_TRUE;
	}
	else
	{

		sem_close(sem_ret);
	}

	ret = create_output_dir ();
	if (RET_SUCC != ret)
	{
		fprintf (stderr, "create_output_dir失败，返回%d\n", ret);
		return RET_FAIL;
	}

	return RET_FALSE;
}

/**********************************************************************
函数说明：启动成为daemon。根据输入参数决定进程以前台模式启动或者daemon模式启动。
输入参数：sb_u8 *process_name: 程序名称
          sb_u16 argc: 参数个数
          sb_u8 **argv: 参数列表

输出参数：无

全局变量拢何?
函数返回：RET_SUCC：成功
          RET_FAIL：失败
          RET_OTHER：前台模式运行

修改记录：无
备    注：无
**********************************************************************/

/*!
        \fn sb_s32 start_daemon(sb_u8 *process_name,sb_u16 argc,sb_u8 **argv)
        \brief 启动成为daemon。根据输入参数决定进程以前台模式启动或者daemon模式启动。
        \param[in] process_name 程序名称
        \param[in] argc 参数个数
        \param[in] argv 参数列表
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
		\retval RET_OTHER 前台模式运行
*/
sb_s32 start_daemon (sb_s8 * process_name, sb_u16 argc, sb_s8 ** argv)
{

/*!
<b>全局变量:</b> \ref g_process_name, \ref g_cmd_options, \ref g_main_pool
*/

/*!
<b>流程:</b>
*/
	sb_s32 ret = RET_SUCC;

	sb_s8 pid_name[SB_BUF_LEN_SMALL] = { 0, };
	FILE *pf = NULL;

	pid_t pid;

	if (NULL == process_name)
	{
		fprintf (stdout, "进程名为空\n");

		return RET_FAIL;
	}
	if ('\0' == process_name[0])
	{
		fprintf (stdout, "进程名为空\n");

		return RET_FAIL;
	}

	strncpy ((sb_s8 *)g_process_name, (sb_s8 *)process_name, sizeof (g_process_name) - 1);

	//进入运行路径：PROCESS_WORKING_DIR
	ret = chdir(PROCESS_WORKING_DIR);

	if (RET_FAIL == ret)
	{
		return RET_FAIL;
	}

	//创建一个子进程
	pid = fork();
	if (pid < 0)				//失败
	{
		fprintf (stderr, "启动daemon时fork失败，返回%d\n", pid);
		exit (RET_FAIL);
	}
	else
	{
		if (pid > 0)			/*父进程操作 */
		{
			fprintf (stdout, "启动daemon成功\n");
			exit (RET_SUCC);
		}

	}
	/*子进程操作 */
	/*将子进程获取的pid号已文本方式存储入pid文件 */
	pid = getpid ();
	snprintf ((sb_s8 *)pid_name, sizeof (pid_name),  PROCESS_WORKING_DIR PROCESS_STATUS_DIR "%s.%u", process_name,  (sb_u32) pid);

	pf = fopen ((sb_s8 *)pid_name, "w+");
	if (NULL != pf)				/*打开文件成功写入pid */
	{
		fputs ((sb_s8 *)pid_name, pf);
		fclose (pf);
	}

	/*将子线程设置成为新进程组中的孤进程 */
	pid = setsid ();


	/*设置进程权限为"rw-r-r" */
	umask (022);

	{
		sb_s8 named_mutex[SB_BUF_LEN_SMALL * 2] = { 0 };

		snprintf ((sb_s8 *)named_mutex, sizeof (named_mutex), "%s%s",   process_name, NAME_MUTEX_POS);

		if ('\0' == named_mutex[0])
		{
			fprintf (stderr, "startdaemon  创建有名互斥量错误，named_mutex为空\n");
			return RET_FAIL;
		}
		sem_t *sem_ret = SEM_FAILED;
		sem_ret = sem_open(named_mutex, O_CREAT,0644,1);

		if( SEM_FAILED == sem_ret)
		{

			fprintf (stderr, "startdaemon 创建有名互斥量：%s 错误，返回值：%d\n", named_mutex, errno);
			return RET_FAIL;
		}
		sem_wait(sem_ret);
		//程序退出时注意删除信号量，atexit注册
		//创建成功
		fprintf (stdout, "startdaemon 创建有名互斥量：%s成功\n", named_mutex);

		ret =	sem_close(sem_ret);

		if (RET_SUCC != ret)
		{
			fprintf (stderr, "startdaemon 关闭有名互斥量：%s错误返回值：%d\n", named_mutex, errno);
			sem_unlink(named_mutex);
			return RET_FAIL;
		}

	}
	return ret;
}

/*!
        \fn void delete_name_mutex(void)
        \brief 删除命名进程互斥量
        \param[in] 无
        \param[out] 无
        \return void
*/
void delete_name_mutex (void)
{

/*!
<b>全局变量:</b> \ref g_main_pool
*/

	sb_s8 named_mutex[SB_BUF_LEN_SMALL * 2] = { 0 };	//用于有名互斥量名称组装
	snprintf ((sb_s8 *)named_mutex, sizeof (named_mutex), "%s%s", g_process_name, NAME_MUTEX_POS);

	if ('\0' == named_mutex[0])
	{
		return;
	}

	if(0==sem_unlink(named_mutex))
	{
		fprintf (stdout, "删除进程互斥信号量：%s\n", named_mutex);

	}
	else
	{
		fprintf (stdout, "删除互斥信号量: %s 失败\n", named_mutex);
	}

	return;
}

//创建所有进程
sb_s32 create_process( sb_s32 argc , sb_s8 ** argv )

{
	cpu_set_t cpus;
	sb_s32 dataplane_start = 0;
	sb_s32 num_dataplane_cpus;
	sb_s32 num_dataplane_need_cpus;
	sb_s32 ret;
	sb_s32 sig_parser_order = 0;
	sb_s32 data_parser_order = 0;
	sb_s32 xdr_sender_order = 0;
	sb_s32 ui_sender_order = 0;
	sb_s32 dataplane_order = 0;
	sb_s32 logd_order = 0;
	sb_s32 i;
	//! - 获取可使用的dataplane cpu集

	ret = tmc_cpus_get_dataplane_cpus(&cpus);
	if (RET_SUCC!= ret)
	{
		fprintf (stderr, "获取dataplane cpu 失败!\n");
		ADD_LOG (LOG_LEVEL_FAILURE)
			(0, 0 ,"[%s]:获取dataplane cpu 失败!",__FUNCTION__);

		return RET_FAIL;
	}

	dataplane_start  = g_adv_config.dataplane_cpu_start;
	num_dataplane_cpus = tmc_cpus_count(&cpus) - dataplane_start ;
	num_dataplane_need_cpus = g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus
								+ g_adv_config.xdr_sender_cpus;


	if(num_dataplane_cpus < num_dataplane_need_cpus)
	{
		fprintf (stderr, "工作在dataplane模式的cpu数量不足，需要%d个，实际有%d个可供使用!", num_dataplane_need_cpus, num_dataplane_cpus);
		ADD_LOG (LOG_LEVEL_FAILURE)
			(0, 0 ,"[%s]:工作在dataplane模式的cpu数量不足，需要%d个，实际有%d个可供使用!",__FUNCTION__,num_dataplane_need_cpus, num_dataplane_cpus);

		return RET_FAIL;
	}




	for(i = 1; i< g_process_info_p->num; i++)
	{

		process_info_t* p = &g_process_info_p->process_array[i];

		fprintf(stdout,"创建进程 %s[%d] \n",p->process_name,p->process_order );

		ADD_LOG (LOG_LEVEL_NOTICE)
			(0, 0 , "创建进程 %s[%d]!",p->process_name,p->process_order );

		switch(p->process_id)
		{
			case PROCESS_ID_SIG_PARSER:
			{
				p->process_order = sig_parser_order;
				break;
			}
			case PROCESS_ID_DATA_PARSER:
			{
				p->process_order = data_parser_order;
				break;
			}
			case PROCESS_ID_XDR_SENDER:
			{
				p->process_order = xdr_sender_order;
				break;
			}
			case PROCESS_ID_UI_SENDER:
			{
				p->process_order = ui_sender_order;
				break;
			}
			case PROCESS_ID_LOGD:
			{
				p->process_order = logd_order;
				break;
			}
			default:
				break;
		}

		in_register_signal_func (in_signal_func);

		pid_t pid = fork();

		if(pid<0){
			printf(" 创建进程 %s[%d] id=%d  失败\n",p->process_name, p->process_order,i );
			ADD_LOG (LOG_LEVEL_FAILURE)
				(0, 0 , "创建进程 %s[%d] id=%d 失败!",p->process_name,p->process_order,i );

			return RET_FAIL;

		}
		else if(pid == 0){				//child
				  tmc_cmem_init(0);
				  g_process_id = p->process_id;
				  strncpy(g_process_name,p->process_name,MAX_PROCESS_NAME_SIZE-1);

				  g_process_order = p->process_order;

				  //为每个进程初始化log
				  sb_s8 tmp_name[MAX_PROCESS_NAME_SIZE];
				  sprintf(tmp_name,"%s.%d",g_process_name, g_process_order);

				  ret = init_log_thread (tmp_name);

				  if(p->process_id == PROCESS_ID_SIG_PARSER || p->process_id == PROCESS_ID_DATA_PARSER
						  || p->process_id == PROCESS_ID_XDR_SENDER)
				  {
				  	  sb_s32 cpu_id = tmc_cpus_find_nth_cpu(&cpus,dataplane_order+dataplane_start); //dataplane_start for test

					  sb_s32 ret = tmc_cpus_set_my_cpu(cpu_id);  //设置dataplane模式
					  if(ret<0)
					  {
						  //暂时只有sig_parser模块式采用logd方式记录日志，其它的进程都采用以前的方式记录
						  if(p->process_id == PROCESS_ID_SIG_PARSER)
						  {
							  SB_FPRINT(stderr, "%s[%d]绑定dataplane dataplane_id[%d] cpu_id %d 失败!\n ",g_process_name,g_process_order,dataplane_order,cpu_id);
							  SB_ADD_LOG (LOG_LEVEL_FAILURE)
							  (SB_LOG_ERR, 0 , "%s[%d] 绑定dataplane dataplane_id[%d] cpu_id %d失败!",g_process_name,g_process_order,dataplane_order,cpu_id);
						  }
						  else
						  {
							  fprintf(stderr, "%s[%d]绑定dataplane dataplane_id[%d] cpu_id %d 失败!\n ",g_process_name,g_process_order,dataplane_order,cpu_id);
							  ADD_LOG (LOG_LEVEL_FAILURE)
							  (0, 0 , "%s[%d] 绑定dataplane dataplane_id[%d] cpu_id %d失败!",g_process_name,g_process_order,dataplane_order,cpu_id);

						  }

					  }
					  else{
						  if(p->process_id == PROCESS_ID_SIG_PARSER)
						  {
							  SB_FPRINT(stderr, "%s[%d]绑定dataplane dataplane_id[%d] cpu_id %d 成功!\n ",g_process_name,g_process_order,dataplane_order,cpu_id);
							  SB_ADD_LOG (LOG_LEVEL_NOTICE)
								  (SB_LOG_INFO, 0 , "%s[%d] 绑定dataplane dataplane_id[%d] cpu_id %d 成功!",g_process_name,g_process_order,dataplane_order,cpu_id);
						  }
						  else
						  {
							  fprintf(stderr, "%s[%d]绑定dataplane dataplane_id[%d] cpu_id %d 成功!\n ",g_process_name,g_process_order,dataplane_order,cpu_id);
							  ADD_LOG (LOG_LEVEL_NOTICE)
							  (0, 0 , "%s[%d] 绑定dataplane dataplane_id[%d] cpu_id %d 成功!",g_process_name,g_process_order,dataplane_order,cpu_id);
						  }
						  p->cpu_id = tmc_cpus_get_my_cpu();
						  g_cpu_id = p->cpu_id;
						  tmc_mem_fence();					//改动达到2级cache
					  }
				  }
				  prctl(PR_SET_NAME, (unsigned long)p->process_name,NULL,NULL,NULL);		//修改进程名，未必有用

				  //修改进程名 arg[0]注意，如果进程名长度小于原进程名，会有问题
				  sb_s8 arg[100];
				  sb_s8* q = (sb_s8*)arg;
				  sb_s32 length = 0;
				  memset(arg,0,100);



				  for(i=0;i< argc;i++)
				  {
							 if(i==0)
							 {
										strcpy(q,g_process_name);

										length = length + strlen(q)+1;

										q= arg+length;
							 }
							 else{
										strcpy(q,argv[i]);
										length = length + strlen(q)+1;
										q= arg+length;

							 }
				  }
//				fprintf(stdout,"修改进程名前!\n");
				memcpy(argv[0],arg,length);
//				fprintf(stdout,"修改进程名后!\n");
				for(i =1;i<argc;i++)
				{
					argv[i] = argv[i-1]+strlen(argv[i-1])+1;

				}
				for(i=0;i<argc;i++)
				{
					argv[i][strlen(argv[i])]=' ';
				}
				argv[argc-1][strlen(argv[argc-1])] = '\0';
//				fprintf(stdout, "修改进程名为 %s !\n",p->process_name);
//				ADD_LOG (LOG_LEVEL_DEBUG)
//				(0, 0 , "修改进程名为 %s !\n",p->process_name);


				break;					//必须跳出循环!
		}

		else{						//father 记录procee_info信息
			p->pid = (sb_u32)pid;
			switch(p->process_id)
			{
				case PROCESS_ID_SIG_PARSER:
				{
					sig_parser_order++;
					dataplane_order++;
//				  	sb_s32 cpu_id = tmc_cpus_find_nth_cpu(&cpus,dataplane_order+dataplane_start);

					break;
				}
				case PROCESS_ID_DATA_PARSER:
				{
					data_parser_order++;
					dataplane_order++;
//				  	sb_s32 cpu_id = tmc_cpus_find_nth_cpu(&cpus,dataplane_order+dataplane_start);

					break;
				}
				case PROCESS_ID_XDR_SENDER:
				{
					xdr_sender_order++;
					dataplane_order++;
//					sb_s32 cpu_id = tmc_cpus_find_nth_cpu(&cpus,dataplane_order+dataplane_start);

					break;
				}
				default:
					break;
			}
		}


	}
	return RET_SUCC;
}
