/*
 * sb_base_mpipe.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_BASE_MPIPE_H_
#define SB_BASE_MPIPE_H_

// Align "p" mod "align", assuming "p" is a "void*".
#define ALIGN(p, align) do { (p) += -(long)(p) & ((align) - 1); } while(0)

//mpipe stack buffer最大页数
#define MAX_MPIPE_STACK_PAGE_NUM 	15


typedef struct net_mpipe_channel_tag
{
	gxio_mpipe_link_t link;
	sb_s8 link_name[MAX_LINK_NAME];			//链接名gbe1 gbe2 最多支持10
	sb_s32 channel_id;

}net_mpipe_channel_t;

typedef struct net_mpipe_stack_tag
{
	sb_s32 stack_id;
	sb_s32 buffer_num;
	sb_s32 buffer_size;				//each block size same as buffer_kind
	gxio_mpipe_buffer_size_enum_t buffer_kind;
}net_mpipe_stack_t;

typedef struct i_mpipe_tag
{
	gxio_mpipe_iqueue_t* iqueue;
	sb_s32 iqueue_mem_size;
	void *iqueue_mem;
}i_mpipe_t;

typedef struct e_mpipe_tag
{
	gxio_mpipe_equeue_t* equeue;
	sb_s32 equeue_mem_size;
	void *equeue_mem;
}e_mpipe_t;

typedef struct net_mpipe_tag
{
	gxio_mpipe_context_t context;		//mpipe_context
	sb_s32 instance;					//instance
	sb_s32 channel_num;					//channel_num

	net_mpipe_channel_t* ichannels;		//输入端口
	net_mpipe_stack_t small_stack;		//small
	net_mpipe_stack_t large_stack;		//large
	sb_s32 iqueue_num;					//iqueue数量
	sb_s32 iqueue_len;					//每个输入队列的最大缓冲包数
	sb_s32 iring_id;					//第一个iqueue的ring_id
	i_mpipe_t* iqueues;					//输入描述符队列数组

	net_mpipe_channel_t* echannels_dpi;		//DPI输出端口
	sb_s32 equeue_num_dpi;					//DPI数据输出queue数量
	e_mpipe_t* equeues_dpi;					//DPI输出描述符队列数组

	net_mpipe_channel_t* echannels_ql;		//全量输出端口
	sb_s32 equeue_num_ql;					//全量数据输出queue数量
	e_mpipe_t* equeues_ql;					//全量输出描述符队列数组

	net_mpipe_channel_t* echannels_psql;	//PS全量输出端口
	sb_s32 equeue_num_psql;					//PS全量数据输出queue数量
	e_mpipe_t* equeues_psql;				//PS全量输出描述符队列数组

	sb_s32 equeue_num;					//equeue数量
	sb_s32 equeue_len;					//每个输出队列的最大缓冲包数
	sb_s32 ering_id;					//第一个equeue的ring_id

	void *mem;							//分配的buffer stack内存区
	void *iqueue_mems;					//分配的iqueue_mem内存

	gxio_mpipe_rules_t rules;				//规则
}net_mpipe_t;


sb_s32 init_mpipe_net(tmc_alloc_t *alloc_ptr);			//初始化mpipe函数

sb_s32 set_mpipe_rules_and_open(void);		//设置规则开始mpipe

sb_s32 close_mpipe_net(void);		//结束mpipe关闭链接!

extern net_mpipe_t* g_mpipe_info ;		//mpipe共享指针

#endif /* SB_BASE_MPIPE_H_ */
