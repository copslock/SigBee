/*
 * sb_base_pack.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_BASE_PACK_H_
#define SB_BASE_PACK_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <tmc/alloc.h>

#include <arch/atomic.h>
#include <arch/sim.h>

#include <gxio/mpipe.h>

#include <tmc/cpus.h>
#include <tmc/mem.h>
#include <tmc/sync.h>
#include <tmc/task.h>

#include "sb_type.h"

// error number define
#define DATA_SEND_OK 0
#define ARGS_ERROR -1
#define DATA_START_ERROR -2
#define DATA_LENGTH_ERROR -3
#define DATA_SEND_ERROR -4

// the dmac value that we want to set
// it will pass to dmac array so data count must 6 bytes
#define DMAC_FOR_SET {0x66,0x66,0x66,0x66,0x66,0x66}

/* * * * *
 *      函数名：sb_send_packet
 *
 *      参数  ：1.gxio_mpipe_equeue_t*  要将edesc放入的输出队列
 *                        2.gxio_mpipe_iqueue_t*  idesc资源所在的输入队列
 *                        3.gxio_mpipe_idesc_t*   从输入队列中得到的存放数据的描述符
 *
 *      返回值：0  :表示成功
 *                        <0 :表示错误
 *                                      -1.传入参数错误
 *                                      -2.要发送数据指针为空
 *                                      -3.要发送数据长度为0
 *                                      -4.数据放入输出队列时出错
 *
 *      功能  ：
 *                       把要发送数据包的目标MAC改为指定的MAC地址，然后发送出去
 *                       最后释放数据包使用的缓冲区
 *
 * */
sb_s32 sb_send_packet(gxio_mpipe_equeue_t*, gxio_mpipe_idesc_t*);



#endif /* SB_BASE_PACK_H_ */
