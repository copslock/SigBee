/*
 * sb_qlog.c
 *
 *  Created on: 2014年5月28日
 *      Author: Figoo
 */

#include <stdarg.h>

#include "sb_type.h"
#include "sb_qlog.h"
#include "sb_public_var.h"

log_queue_t *g_log_queue;


 /**********************************************************************
 函数说明：空打印函数
 输入参数：stream,	文件流
           format,	要打印的格式化字符串
 输出参数：无

 函数返回：0

 修改记录：无
 备    注：
 **********************************************************************/

 /*!
         \fn sb_s32 nfprintf (FILE *stream, const char *format, ...)
         \brief 根据进程名称从进程信息数组中获取数组索引
         \param[in] stream	文件流
         \param[in] format	要打印的格式化字符串
         \param[out] 无
         \return 0
 */
 sb_s32 nfprintf (FILE *stream, const char *format, ...)
 {
	 return 0;
 }

 /**********************************************************************
 函数说明：将日志写入日志队列
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
         \fn void write_qlog(sb_u32 error_num_public, sb_u32 error_num_private, sb_u8*fmt, ...)
         \brief 将日志写入日志队列
         \param[in] error_num_public 公共错误码
         \param[in] error_num_private 私有错误码
         \param[in] fmt 格式化串
         \param[in] ... va_list 结构
         \param[out] 无
         \return void
 */
 void write_qlog (sb_u32 error_num_public,
 							 sb_u32 error_num_private, sb_s8 * fmt,...)
 {

 /*!
 <b>全局变量:</b> \ref g_log_queue, \ref g_current_time, \ref g_process_name, \ref g_process_order, \ref g_cpu_id
 */

 /*!
 <b>流程:</b>
 */
	 log_msg_t log_msg;
	 va_list tmp_list;
	 sb_s32 ret;

	 memset((sb_s8*)&log_msg, 0, sizeof(log_msg));
	 log_msg.msg_type = error_num_public;
	 memcpy(log_msg.pname, g_process_name, strlen(g_process_name));
	 log_msg.cpuid = g_cpu_id;
	 log_msg.time = (time_t) (g_current_time & 0xFFFFFFFF);

	 //!     - 从可变参数生成日志串写入缓冲
	 va_start (tmp_list, fmt);
	 vsnprintf ((char *)log_msg.content, sizeof (log_msg.content) - 1, (char *)fmt, tmp_list);
	 va_end (tmp_list);
//	 printf("log content is :%s\n", log_msg.content);
	 ret=log_queue_enqueue(g_log_queue,log_msg);

	 return;

 }

