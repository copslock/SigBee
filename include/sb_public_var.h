/*
 * sb_public_var.h
 *
 *  Created on: 2014年5月28日
 *      Author: Figoo
 */

#ifndef SB_PUBLIC_VAR_H_
#define SB_PUBLIC_VAR_H_

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

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
#include "sb_statistic.h"
#include "sb_shared_queue.h"

/*全局变量*/
extern sb_u32* g_process_exit;	//!< 进程退出全局变量
extern sb_u64 g_current_time;	//!< 全局时间
//当前进程的模块名
extern sb_s8 g_process_name[MAX_PROCESS_NAME_SIZE];
//当前进程在模块号
extern sb_s32 g_process_id;
//当前进程模块序号
extern sb_s32 g_process_order;
//当前进程的cpu号 由于可能绑定cpu序号可能为0，所以默认为-1为不绑定
extern sb_s32 g_cpu_id;

extern pthread_key_t g_thread_key;

extern device_config_t g_device_config;	//!< device.ini配置文件读入后的结构
extern adv_config_t g_adv_config;	//!< adv.ini配置文件读入后的结构
extern sb_s32 g_proto_ips_num[MAX_PRO_IP_LINK_NUM];	//!< 无线协议识别IP个数
extern ip_proto_t g_proto_ips[MAX_PRO_IP_LINK_NUM][MAX_PRO_IP_NODE_NUM];	//!< 无线协议识别IP列表
extern sb_s32 g_ps_filter_num;		//!< PS全量过滤条件个数
extern ps_filter_rule_t g_ps_filter[MAX_PS_RULE_NUM];	//!< PS全量过滤条件

extern sp_statistic_t* g_sp_statistics; //计数器入口指针,是一个数组，其大小为sig_parser进程数量
extern dp_statistic_t* g_dp_statistics; //计数器入口指针,是一个数组，其大小为data_parser进程数量

extern sb_sp_user_queue_head_t *g_sp_user_queue_head; //!< sig_parser到data_parser的用户队列全局指针
extern sb_sp_packet_queue_head_t *g_sp_packet_queue_head; //!< sig_parser到data_parser的数据包信息队列全局指针
extern sb_sp_xdr_queue_head_t *g_sp_xdr_queue_head; //!< sig_parser到xdr_sender的队列全局指针
extern sb_dp_xdr_queue_head_t *g_dp_xdr_queue_head; //!< data_parser到xdr_sender的队列全局指针
extern sb_dp_user_queue_head_t *g_dp_user_queue_head; //!< data_parser到ui_sender的用户队列全局指针

#endif /* SB_PUBLIC_VAR_H_ */
