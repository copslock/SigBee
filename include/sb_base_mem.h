/*
 * sb_base_mem.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_BASE_MEM_H_
#define SB_BASE_MEM_H_


#include <tmc/mspace.h>

#define LOG_MAX_SIZE	200000000	//文件最大字节数

void * check_malloc (sb_s8 *func, size_t num, size_t size);
void check_free (void **buffer);
void *fast_memcpy(void *dest,const void *src,size_t n);
void string_cpy(sb_u8 *dst, sb_u8 *src, sb_u16 len);
void * check_malloc_msp (tmc_mspace msp, sb_s8 *func, size_t num, size_t size);
void check_free_msp (void **buffer);

#endif /* SB_BASE_MEM_H_ */
