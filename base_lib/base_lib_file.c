/*
 * base_lib_file.c
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>

//tilera 相关头文件
#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_error_code.h"
#include "sb_base_mem.h"
#include "sb_public_var.h"

//! 错误码和和错误内容的对照表，通过定义的宏进行索引查找相应的内容
sb_u8 g_code_content_map[SB_BUF_LEN_SMALL * 2][SB_BUF_LEN_TINY] = { {0}
,
};

/**********************************************************************
函数说明：创建输出目录
输入参数：无

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 create_output_dir(void)
        \brief 创建输出目录
        \param[in] 无
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
 */
sb_s32 create_output_dir (void)
{
	/*!
<b>流程:</b>
	 */
	mode_t mode = S_IRWXU | S_IRGRP |  S_IXGRP | S_IROTH | S_IXOTH ;
	mkdir(PROCESS_WORKING_DIR PROCESS_LOG_DIR, mode);
	mkdir(PROCESS_WORKING_DIR PROCESS_STATUS_DIR , mode);
	mkdir(PROCESS_WORKING_DIR PROCESS_CONFIG_DIR , mode);

	return RET_SUCC;
}

/*!
        \fn void in_free_log_thread(void)
        \brief 释放线程日志
        \param[in] 无
        \param[out] 无
        \return void
 */
void in_free_log_thread (void)
{

	/*!
<b>全局变量:</b> \ref g_thread_key
	 */

	/*!
<b>流程:</b>
	 */
	sb_u8 *tmp_thread_name = NULL;

	//!     - 取出\ref g_thread_key对应线程私有变量
	tmp_thread_name = (sb_u8*)  pthread_getspecific(g_thread_key);

	if (NULL == tmp_thread_name )
	{
		return;
	}

	//!     - 释放该私有变量的内存
	check_free ((void **)tmp_thread_name);
	tmp_thread_name = NULL;
}

//线程退出时执行的线程线程

/*!
        \fn void in_thread_log_exit(void *thread_data)
        \brief 线程退出时执行的线程清场函数
        \param[in] thread_data 清场函数的调用参数
        \param[out] 无
        \return void
 */
void in_thread_log_exit (void *thread_data)
{

	/*!
<b>全局变量:</b>
	 */

	/*!
<b>流程:</b>
	 */
	//!     - 释放线程日志\ref in_free_log_thread
	in_free_log_thread ();
	return;
}

/**********************************************************************
函数说明：初始化日志功能的全局东东
输入参数：sb_u32 log_level，日志级别

输出参数：无

全局变量：g_thread_key、g_device_config


函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 init_log(sb_u32 log_level)
        \brief 初始化日志功能的全局东东
        \param[in] log_level 日志级别
        \param[out] 无
        \return sb_s32
		\retval RET_FAIL 失败
		\retval RET_SUCC 成功
 */
sb_s32 init_log (sb_u32 log_level)
{

	/*!
<b>全局变量:</b> \ref g_thread_key, \ref g_device_config
	 */

	/*!
<b>流程:</b>
	 */

	sb_s32 ret;
	//!     - 创建线程私有变量
	ret = pthread_key_create (&g_thread_key, in_thread_log_exit);

	if (RET_SUCC != ret)
	{
		return RET_FAIL;
	}

	//! - 初始化日志相关全局变量\ref g_current_time，g_device_config.log_level
	g_adv_config.log_level = log_level;
	g_current_time = time (NULL);

	return RET_SUCC;
}

/**********************************************************************
函数说明：初始化线程日志
输入参数：sb_u8* thread_name：线程名称

输出参数：无

全局变量：g_thread_key

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 init_log_thread(sb_u8* thread_name)
        \brief 初始化线程日志
        \param[in] thread_name 线程名称
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
 */
sb_s32 init_log_thread (sb_s8 * thread_name )
{

	/*!
<b>全局变量:</b> \ref g_thread_key
	 */

	/*!
<b>流程:</b
	 */
	pthread_t thread_id;
	sb_u8 *tmp_thread_name = NULL;
	sb_s32 ret;
	//!     - 判断参数有效
	if (NULL == thread_name)
	{
		return RET_ERR_ARG;
	}

	//!     - 获取线程id
	thread_id = pthread_self();
	//!     - 为变量tmp_thread_name分配内存，存放thread_name
	tmp_thread_name = (sb_u8 *) check_malloc ("init_log_thread", 1, strlen ((char *)thread_name) + 1);
	if (NULL == tmp_thread_name)
	{
		return RET_FAIL;

	}

	strncpy ((char *)tmp_thread_name, (char *)thread_name, strlen ((char *)thread_name));

	//!     - 设置 \ref g_thread_key 对应线程私有变量的
	ret = pthread_setspecific(g_thread_key, (void *)tmp_thread_name);

	if (RET_SUCC != ret)
	{
		check_free ((void **)&tmp_thread_name);
		tmp_thread_name = NULL;
		return RET_FAIL;
	}

	return RET_SUCC;
}

/**********************************************************************
函数说明：根据进程名称从进程信息数组中获取数组索引
输入参数：error_num_public,	公共错误码
          error_num_private,	私有错误码
          fmt,	：格式化串
          ...		：va_list 结构


输出参数：无

函数返回：无

修改记录：无
备    注：
 **********************************************************************/

/*!
        \fn void write_log(sb_u32 error_num_public, sb_u32 error_num_private, sb_u8*fmt, ...)
        \brief 根据进程名称从进程信息数组中获取数组索引
        \param[in] error_num_public 公共错误码
        \param[in] error_num_private 私有错误码
        \param[in] fmt 格式化串
        \param[in] ... va_list 结构
        \param[out] 无
        \return void
 */
void write_log (sb_u32 error_num_public,
		sb_u32 error_num_private, sb_s8 * fmt,...)
{

	/*!
<b>全局变量:</b> \ref g_thread_key, \ref g_current_time, \ref g_process_name, \ref g_code_content_map
	 */

	/*!
<b>流程:</b>
	 */

	sb_u8 *tmp_thread_name = NULL;
	sb_u8 log_file_name[SB_BUF_LEN_NORMAL] = { 0 };

	sb_u8 log_date[SB_BUF_LEN_TINY] = { 0 };
	sb_u8 log_time[SB_BUF_LEN_TINY] = { 0 };
	struct tm *p_tm = NULL;
	size_t time_size;
	time_t tmp_cur_time;

	va_list tmp_list;
	sb_u8 buf_fmt[SB_BUF_LEN_BIG] = { 0 };
	sb_u8 buf[SB_BUF_LEN_BIG] = { 0 };

	const sb_u8 *public_err = NULL;
	const sb_u8 *private_err = NULL;

	FILE *fp = NULL;
	static sb_u32 fd;

	struct stat file_stat;
	sb_s32 ret;

	//!     - 判断参数有效性
	if (NULL == fmt)
	{
		return;
	}


	tmp_thread_name= pthread_getspecific( g_thread_key);

	if ( NULL == tmp_thread_name)
	{
		return;
	}
	//! - 获取系统全局时间
	tmp_cur_time = (time_t) (g_current_time & 0xFFFFFFFF);
	p_tm = localtime (&tmp_cur_time);
	if (NULL == p_tm)
	{
		return;
	}

	time_size = strftime ((char *)log_date, sizeof (log_date) - 1, "%Y%m%d", p_tm);
	if (0 == time_size)
	{
		return;
	}

	time_size =
			strftime ((char *)log_time, sizeof (log_time) - 1, "%Y/%m/%d %H:%M:%S", p_tm);
	if (0 == time_size)
	{
		return;
	}

	//!     - 生成日志文件名：路径+进程名.线程名.日期.log
	snprintf ((char *)log_file_name, sizeof (log_file_name) - 1,
			"%s%s.%s.%s.log", PROCESS_WORKING_DIR PROCESS_LOG_DIR,
			g_process_name, tmp_thread_name, log_date);
	//printf("log_file:%s \n");

	//!     - 从可变参数生成日志串写入缓冲buf_fmt
	va_start (tmp_list, fmt);
	vsnprintf ((char *)buf_fmt, sizeof (buf_fmt) - 1, (char *)fmt, tmp_list);
	va_end (tmp_list);

	if (error_num_public >= SB_BUF_LEN_SMALL * 2)
	{
		return;
	}
	public_err = g_code_content_map[error_num_public];
	if (error_num_private >= SB_BUF_LEN_SMALL * 2)
	{
		return;
	}
	private_err = g_code_content_map[error_num_private];
	snprintf ((char *)buf, sizeof (buf) - 1, "%s %s %s %s\n", log_time,
			public_err, private_err, buf_fmt);

	//!     - 打开日志文件

	fp = fopen ((char *)log_file_name, "a");
	if (NULL == fp)
	{
		return;
	}

	//! - 获取日志文件大小，如果小于限制\ref MAX_LOG_FILE_SIZE ，写入前面生成的日志串
	fd = (sb_u32) fileno (fp);
	ret = fstat (fd, &file_stat);
	if (0 == ret)
	{
		if (file_stat.st_size < MAX_LOG_FILE_SIZE)
		{
			fputs ((char *)buf, fp);
		}
	}

	//! - 关闭日志文件
	fclose (fp);

}

//释放日志全局东东

/*!
        \fn void free_log(void)
        \brief 释放日志全局东东
        \param[in] 无
        \param[out] 无
        \return void
 */
void free_log (void)
{

	/*!
<b>全局变量:</b> \ref g_thread_key
	 */

	/*!
<b>流程:</b>
	 */

	//!     删除线程key \ref g_thread_key
	pthread_key_delete (g_thread_key);
}

