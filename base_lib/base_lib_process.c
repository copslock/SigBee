/*
 * base_lib_process.c
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

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
#include "sb_base_file.h"
#include "sb_base_util.h"
#include "sb_base_mica.h"
#include "sb_base_mem.h"
#include "sb_base_process.h"
#include "sb_public_var.h"

cmd_options_t g_cmd_options = {0,};

sb_s32 parse_cmd_line (sb_s32 argc, sb_s8 ** argv)
{
	sb_s32 i;

	memset (&g_cmd_options, 0, sizeof (cmd_options_t));

	for (i = 1; i < argc; i++)
	{
		if (strcmp ((char *)argv[i], "-v") == 0)
		{
			g_cmd_options.show_version = TRUE;
		}
		if (strcmp ((char *)argv[i], "-h") == 0)
		{
			g_cmd_options.show_usage = TRUE;
		}

	}

	return (0);
}

/**********************************************************************
  函数说明：初始化进程数组，每个元素

  输入参数：无
  输出参数：无
  全局变量：g_process_info_p
  函数返回：RET_SUCC：成功;RET_FAIL：失败
  修改记录：无
  备    注：
 **********************************************************************/
sb_s32 init_share_process_info(void)
{
	sb_s32 load_balance_order = 0;
	sb_s32 link_parser_order = 0;
	sb_s32 app_parser_order = 0;
	sb_s32 i = 0;

	g_process_info_p = tmc_cmem_malloc(sizeof(global_process_info_t));

	if(g_process_info_p == NULL)
	{
		fprintf(stderr,"g_process_info_p 分配内存失败!\n");
		return RET_FAIL;
	}
	memset(g_process_info_p, 0 , sizeof(global_process_info_t));
	//init+logd+sig_parser+data_parser+xdr_sender+ui_sender
	g_process_info_p->num = 1 + 1 + g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus + g_adv_config.xdr_sender_cpus + 1;
	g_process_info_p->run_num = 1;  //自己
	g_process_info_p->process_array = tmc_cmem_malloc(sizeof(process_info_t)* g_process_info_p->num);
	if(g_process_info_p->process_array == NULL)
	{
		fprintf(stderr,"g_process_info_p->process_array 分配内存失败!\n");
		return RET_FAIL;
	}
	memset(g_process_info_p->process_array, 0 , sizeof(process_info_t)*g_process_info_p->num);

	for(i = 0;i<g_process_info_p->num;i++)
	{

		process_info_t *p = &g_process_info_p->process_array[i];
		p->cpu_id = -1;
		if(i == 0){											//SB
			p->pid = (sb_u32)getpid();
			strncpy(p->process_name, PROCESS_NAME_SB,SB_BUF_LEN_TINY-1);
			p->process_id = PROCESS_ID_SB;
		}
		else if(i == 1){	//LOGD

			strncpy(p->process_name, PROCESS_NAME_LOGD, SB_BUF_LEN_TINY-1);
			p->process_id = PROCESS_ID_LOGD;
		}
		else if(1 < i && i <= g_adv_config.sig_parser_cpus + 1){ 								//sig_parser

			strncpy(p->process_name, PROCESS_NAME_SIG_PARSER, SB_BUF_LEN_TINY-1);
			p->process_id = PROCESS_ID_SIG_PARSER;
			p->process_order = load_balance_order;
			load_balance_order++;

		}
		else if(g_adv_config.sig_parser_cpus + 1 < i && i <= g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus + 1){	//data_parser								//shm_rule

			strncpy(p->process_name, PROCESS_NAME_DATA_PARSER,SB_BUF_LEN_TINY-1);
			p->process_id = PROCESS_ID_DATA_PARSER;
			p->process_order = link_parser_order;
			link_parser_order++;

		}
		else if(g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus + 1 < i
				&& i <= g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus + g_adv_config.xdr_sender_cpus + 1){	//xdr_sender
			strncpy(p->process_name, PROCESS_NAME_XDR_SENDER,SB_BUF_LEN_TINY-1);
			p->process_id = PROCESS_ID_XDR_SENDER;
			p->process_order = app_parser_order;
			app_parser_order++;
		}
		else if(i == g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus + g_adv_config.xdr_sender_cpus + 1 + 1){	//ui_sender

			strncpy(p->process_name, PROCESS_NAME_UI_SENDER, SB_BUF_LEN_TINY-1);
			p->process_id = PROCESS_ID_UI_SENDER;
		}

		else if(i>= g_process_info_p->num){
			fprintf(stderr,"unknow process!\n");
			return RET_FAIL;
		}

	}
	return RET_SUCC;


}

/*!
        \fn void in_signal_func(sb_s32 sig_no)
        \brief 信号处理函数
        \param[in] sig_no 信号
		\param[out] 无
        \return void
*/
void in_signal_func (sb_s32 sig_no)
{

/*!
<b>全局变量:</b> \ref g_process_exit
*/

/*!
<b>流程:</b>
*/
	//!     - sig_no为SIGKILL、SIGQUIT、SIGINT、SIGTERM、SIGABRT:
	if ((SIGKILL == sig_no) || (SIGQUIT == sig_no) || (SIGINT == sig_no) ||
		(SIGTERM == sig_no) || (SIGABRT == sig_no))
	{
		/*!     - &nbsp;&nbsp;&nbsp;
		   发出线程停止信号（全局变量 \ref g_process_exit
		 */
		if(g_process_id == PROCESS_ID_SB){
			*g_process_exit = 1;
			fprintf(stderr, "接收到退出信号，等待所有子进程退出\n");
//			kill(0, SIGTERM);
			sb_s32 num = g_process_info_p->num - 1 - 1;//等待loadbalance和link_parser进程退出
			while(num > 0){
				wait(NULL);
				num--;
			}
			sleep(1);
			kill(0, SIGTERM);
			fprintf(stderr, "所有进程退出，本身退出\n");
			exit(RET_SUCC);
		}else{
			exit(RET_SUCC);
		}
		/*!     - &nbsp;&nbsp;&nbsp;
		   强制退出（删除互斥信号量即释放资源均atexit了）
		 */

//		exit (0);
	}

	//!     - 否则需要再次设置信号处理函数
	in_register_signal_func (in_signal_func);

	return;
}

/*!
        \fn sb_s32 in_register_signal_func(void *sig_func)
        \brief 注册信号处理函数
        \param[in] sig_func 函数句柄
                \param[out]
        \return pf_s32
                \retval RET_SUCC 成功
                \retval RET_FAIL 失败
                \retval RET_ERR_ARG 参数无效
*/
sb_s32 in_register_signal_func (void *sig_func)
{

/*!
<b>全局变量:</b>
*/

/*!
<b>流程:</b>
*/
        //!     - 判断参数有效
        if (NULL == sig_func)
        {
                return RET_ERR_ARG;
        }

        //!     - 注册信号SIGKILL、SIGQUIT、SIGINT、SIGTERM、SIGABRT的信号处理函数为sig_func
        /*需要注册的信号包括SIGKILL,SIGQUIT,SIGSEGV,SIGABRT */

        signal (SIGKILL, sig_func);
        signal (SIGQUIT, sig_func);
        signal (SIGINT, sig_func);
        signal (SIGTERM, sig_func);
        signal (SIGABRT, sig_func);

        signal (SIGPIPE, SIG_IGN);
        return RET_SUCC;
}

