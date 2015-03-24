/*
 * sb_base_process.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_BASE_PROCESS_H_
#define SB_BASE_PROCESS_H_

/* 进程信息结构体 */
typedef struct process_info_tag
{
	sb_s32 cpu_id;							//所绑定的cpu_id,如果为-1表示不绑定
	sb_u32 pid;								//进程在内核中的编号
	sb_s8  process_name[SB_BUF_LEN_TINY]; 	//进程名
	sb_u32 process_id;						//进程模块编号
	sb_u32 process_order;					//进程所属模块的编号!
}process_info_t;


/* pf模块信息结构体 */
typedef struct sb_module_info_tag
{
	sb_u32	module_id;							//模块编号
	sb_u8	module_name[SB_BUF_LEN_TINY];		//模块名
}sb_module_info_t;

/* 进程信息全局结构体 */
typedef struct global_process_info_tag
{
	sb_s32 num;						//多少个进程
	sb_s32 run_num;					//多少个进程在运行
	process_info_t *process_array;  //进程信息数组

}global_process_info_t;


global_process_info_t *g_process_info_p;

typedef struct cmd_options_tag
{
	sb_s32 show_version;
	sb_s32 show_usage;
	sb_s32 cap_signal_mode;//抓取信令包
} cmd_options_t;

extern cmd_options_t g_cmd_options;

sb_s32 parse_cmd_line (sb_s32 argc, sb_s8 ** argv);

sb_s32 init_share_process_info(void);
sb_s32 in_register_signal_func (void *sig_func);
void in_signal_func (sb_s32 sig_no);


#endif /* SB_BASE_PROCESS_H_ */
