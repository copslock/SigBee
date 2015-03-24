/*
 * sb_error_code.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_ERROR_CODE_H_
#define SB_ERROR_CODE_H_

#define	RET_SUCC	0			/*      成功    继续            */
#define	RET_FAIL	-1			/*      失败    继续            */
#define	RET_TRUE	0			/*      真      继续            */
#define	RET_FALSE	-2			/*      假      继续            */
#define	RET_ERR_SHM	-3			/*      共享内存不存在  等待            */
#define	RET_ERR_SEM	-4			/*      信号量不存在    等待            */
#define	RET_ERR_MEM	-5			/*      内存分配失败    退出            */
#define RET_ERR_GLOBAL  -6                      /*      全局配置无法读取        退出    影响进程运行的配置      */
#define RET_ERR_ARG     -7                      /*      参数错误        继续            */
#define RET_ERR_LOCAL   -8                      /*      本地配置有错误  待定    不影响整体功能的配置    */
#define RET_ERR_OTHER   -9                      /*      自定义  待定    特殊分支使用    */
#define RET_ERR_NOINIT  -10                     /*      函数未初始化    退出    影响进程运行的配置      */
#define RET_ERR_WRITE   -11                     /*      写文件失败      待定            */
#define RET_OTHER       1                       /*      自定义                  */


#endif /* SB_ERROR_CODE_H_ */
