/*
 * base_lib_mpipe.c
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//tilera 相关头文件
#include <tmc/cmem.h>
#include <tmc/mem.h>
#include <tmc/sync.h>
#include <tmc/cpus.h>
#include <tmc/spin.h>
#include <tmc/task.h>
#include <gxio/mpipe.h>
#include <gxcr/gxcr.h>
#include <gxio/mica.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_error_code.h"
#include "sb_base_util.h"
#include "sb_base_mica.h"
#include "sb_base_mem.h"
#include "sb_base_mpipe.h"
#include "sb_public_var.h"
#include "sb_log.h"

net_mpipe_t* g_mpipe_info = NULL ;		//mpipe

static sb_s32 init_mpipe_channels(tmc_alloc_t *alloc_ptr)
{
	sb_s32 ret = RET_SUCC;
	sb_s32 i,j;

	g_mpipe_info->ichannels = (net_mpipe_channel_t *)
							tmc_alloc_map(alloc_ptr, sizeof(net_mpipe_channel_t)*g_device_config.cap_ports.interface_num);
	if(g_mpipe_info->ichannels == NULL)
	{
		fprintf(stderr,"cmem alloc g_mpipe_info->ichannels fail!");
		return RET_FAIL;
	}

	for(i=0;i< g_device_config.cap_ports.interface_num;i++)
	{

		strncpy(g_mpipe_info->ichannels[i].link_name,g_device_config.cap_ports.interface_name[i],MAX_LINK_NAME-1);
		if(i==0)
		{
			g_mpipe_info->instance = gxio_mpipe_link_instance(g_mpipe_info->ichannels[i].link_name);
			ret = gxio_mpipe_init(&(g_mpipe_info->context),g_mpipe_info->instance);
			if(ret < 0)
			{
				fprintf(stderr,"init mpipe context error!");
				return RET_FAIL;
			}
			fprintf(stdout,"mpipe instance is %d, linkname %s!\n",g_mpipe_info->instance, g_mpipe_info->ichannels[i].link_name);

		}
		else{


			if(g_mpipe_info->instance != gxio_mpipe_link_instance(g_mpipe_info->ichannels[i].link_name))
			{
				fprintf(stderr,"init link instance of %s error!",g_mpipe_info->ichannels[i].link_name);
				return RET_FAIL;
			}
			fprintf(stdout,"mpipe instance is %d, linkname %s!\n",g_mpipe_info->instance, g_mpipe_info->ichannels[i].link_name);

		}
		ret = gxio_mpipe_link_open(&(g_mpipe_info->ichannels[i].link),&g_mpipe_info->context,g_mpipe_info->ichannels[i].link_name,0);
		if(ret < 0)
		{
			fprintf(stderr,"open link %s failed !",g_mpipe_info->ichannels[i].link_name);
			return RET_FAIL;
		}


		g_mpipe_info->ichannels[i].channel_id = gxio_mpipe_link_channel(&(g_mpipe_info->ichannels[i].link));

	}
	g_mpipe_info->channel_num += g_device_config.cap_ports.interface_num;

	//初始化DPI发送口变量
	if(g_device_config.dpi_send_port_num > 0)
	{
		g_mpipe_info->echannels_dpi = (net_mpipe_channel_t *)
								tmc_alloc_map(alloc_ptr, sizeof(net_mpipe_channel_t)*g_device_config.dpi_send_port_num);
		if(g_mpipe_info->echannels_dpi == NULL)
		{
			fprintf(stderr,"cmem alloc g_mpipe_info->echannels_dpi fail!");
			return RET_FAIL;
		}

		for(i = 0; i < g_device_config.dpi_send_port_num; i++)
		{
			sb_s8 finded = 0;

			strncpy(g_mpipe_info->echannels_dpi[i].link_name,g_device_config.dpi_interface_name[i],MAX_LINK_NAME-1);

			for(j = 0; j < g_device_config.cap_ports.interface_num; j++)
			{
				if (strcasecmp(g_mpipe_info->ichannels[j].link_name, g_mpipe_info->echannels_dpi[i].link_name) == 0)
				{
					g_mpipe_info->echannels_dpi[i].link = g_mpipe_info->ichannels[j].link;
					g_mpipe_info->echannels_dpi[i].channel_id = g_mpipe_info->ichannels[j].channel_id;
					finded = 1;
					break;
				}
			}
			if(finded == 1)continue;

			if(g_mpipe_info->instance != gxio_mpipe_link_instance(g_device_config.dpi_interface_name[i]))
			{
				fprintf(stderr,"init dpi link instance of %s error!",g_device_config.dpi_interface_name[i]);
				return RET_FAIL;
			}
			fprintf(stdout,"mpipe instance is %d, dpi linkname %s!\n",g_mpipe_info->instance, g_device_config.dpi_interface_name[i]);

			ret = gxio_mpipe_link_open(&(g_mpipe_info->echannels_dpi[i].link),&g_mpipe_info->context,g_device_config.dpi_interface_name[i],0);
			if(ret < 0)
			{
				fprintf(stderr,"open dpi link %s failed !",g_mpipe_info->echannels_dpi[i].link_name);
				return RET_FAIL;
			}

			g_mpipe_info->echannels_dpi[i].channel_id = gxio_mpipe_link_channel(&(g_mpipe_info->echannels_dpi[i].link));

			//			g_mpipe_info->channel_num ++;
		}

	}

	//初始化全量发送口变量
	if(g_device_config.ql_send_port_num > 0)
	{
		g_mpipe_info->echannels_ql = (net_mpipe_channel_t *)
								tmc_alloc_map(alloc_ptr, sizeof(net_mpipe_channel_t)*g_device_config.ql_send_port_num);
		if(g_mpipe_info->echannels_ql == NULL)
		{
			fprintf(stderr,"cmem alloc g_mpipe_info->echannels_dpi fail!");
			return RET_FAIL;
		}

		for(i = 0; i < g_device_config.ql_send_port_num; i++)
		{
			sb_s8 finded = 0;

			strncpy(g_mpipe_info->echannels_ql[i].link_name,g_device_config.ql_interface_name[i],MAX_LINK_NAME-1);

			for(j = 0; j < g_device_config.cap_ports.interface_num; j++)
			{
				if (strcasecmp(g_mpipe_info->ichannels[j].link_name, g_mpipe_info->echannels_ql[i].link_name) == 0)
				{
					g_mpipe_info->echannels_ql[i].link = g_mpipe_info->ichannels[j].link;
					g_mpipe_info->echannels_ql[i].channel_id = g_mpipe_info->ichannels[j].channel_id;
					finded = 1;
					break;
				}
			}
			if(finded == 1)continue;

			for(j = 0; j < g_device_config.dpi_send_port_num; j++)
			{
				if (strcasecmp(g_mpipe_info->echannels_dpi[j].link_name, g_mpipe_info->echannels_ql[i].link_name) == 0)
				{
					g_mpipe_info->echannels_ql[i].link = g_mpipe_info->echannels_dpi[j].link;
					g_mpipe_info->echannels_ql[i].channel_id = g_mpipe_info->echannels_dpi[j].channel_id;
					finded = 1;
					break;
				}
			}
			if(finded == 1)continue;

			if(g_mpipe_info->instance != gxio_mpipe_link_instance(g_device_config.ql_interface_name[i]))
			{
				fprintf(stderr,"init ql link instance of %s error!",g_device_config.ql_interface_name[i]);
				return RET_FAIL;
			}
			fprintf(stdout,"mpipe instance is %d, ql linkname %s!\n",g_mpipe_info->instance, g_device_config.ql_interface_name[i]);

			ret = gxio_mpipe_link_open(&(g_mpipe_info->echannels_ql[i].link),&g_mpipe_info->context,g_device_config.ql_interface_name[i],0);
			if(ret < 0)
			{
				fprintf(stderr,"open ql link %s failed !",g_mpipe_info->echannels_ql[i].link_name);
				return RET_FAIL;
			}

			g_mpipe_info->echannels_ql[i].channel_id = gxio_mpipe_link_channel(&(g_mpipe_info->echannels_ql[i].link));

			//			g_mpipe_info->channel_num ++;
		}

	}

	//初始化PS全量发送口变量
	if(g_device_config.psql_send_port_num > 0)
	{
		g_mpipe_info->echannels_psql = (net_mpipe_channel_t *)
								tmc_alloc_map(alloc_ptr, sizeof(net_mpipe_channel_t)*g_device_config.psql_send_port_num);
		if(g_mpipe_info->echannels_psql == NULL)
		{
			fprintf(stderr,"cmem alloc g_mpipe_info->echannels_dpi fail!");
			return RET_FAIL;
		}

		for(i = 0; i < g_device_config.psql_send_port_num; i++)
		{
			sb_s8 finded = 0;

			strncpy(g_mpipe_info->echannels_psql[i].link_name,g_device_config.psql_interface_name[i],MAX_LINK_NAME-1);

			for(j = 0; j < g_device_config.cap_ports.interface_num; j++)
			{
				if (strcasecmp(g_mpipe_info->ichannels[j].link_name, g_mpipe_info->echannels_psql[i].link_name) == 0)
				{
					g_mpipe_info->echannels_psql[i].link = g_mpipe_info->ichannels[j].link;
					g_mpipe_info->echannels_psql[i].channel_id = g_mpipe_info->ichannels[j].channel_id;
					finded = 1;
					break;
				}
			}
			if(finded == 1)continue;

			for(j = 0; j < g_device_config.dpi_send_port_num; j++)
			{
				if (strcasecmp(g_mpipe_info->echannels_dpi[j].link_name, g_mpipe_info->echannels_psql[i].link_name) == 0)
				{
					g_mpipe_info->echannels_psql[i].link = g_mpipe_info->echannels_dpi[j].link;
					g_mpipe_info->echannels_psql[i].channel_id = g_mpipe_info->echannels_dpi[j].channel_id;
					finded = 1;
					break;
				}
			}
			if(finded == 1)continue;

			for(j = 0; j < g_device_config.ql_send_port_num; j++)
			{
				if (strcasecmp(g_mpipe_info->echannels_ql[j].link_name, g_mpipe_info->echannels_psql[i].link_name) == 0)
				{
					g_mpipe_info->echannels_psql[i].link = g_mpipe_info->echannels_ql[j].link;
					g_mpipe_info->echannels_psql[i].channel_id = g_mpipe_info->echannels_ql[j].channel_id;
					finded = 1;
					break;
				}
			}
			if(finded == 1)continue;

			if(g_mpipe_info->instance != gxio_mpipe_link_instance(g_device_config.psql_interface_name[i]))
			{
				fprintf(stderr,"init psql link instance of %s error!",g_device_config.psql_interface_name[i]);
				return RET_FAIL;
			}
			fprintf(stdout,"mpipe instance is %d, psql linkname %s!\n",g_mpipe_info->instance, g_device_config.psql_interface_name[i]);

			ret = gxio_mpipe_link_open(&(g_mpipe_info->echannels_psql[i].link),&g_mpipe_info->context,g_device_config.psql_interface_name[i],0);
			if(ret < 0)
			{
				fprintf(stderr,"open psql link %s failed !",g_mpipe_info->echannels_psql[i].link_name);
				return RET_FAIL;
			}

			g_mpipe_info->echannels_psql[i].channel_id = gxio_mpipe_link_channel(&(g_mpipe_info->echannels_psql[i].link));

			//			g_mpipe_info->channel_num ++;
		}

	}

	return ret;
}

static sb_s32
create_stack(gxio_mpipe_context_t* context, int stack_idx,
		gxio_mpipe_buffer_size_enum_t buf_size, int num_buffers)
{
	int result;
	size_t page_size = 64*1024*1024;//64M
	sb_s32 page_num;
	size_t mem_size;

	size_t size = gxio_mpipe_buffer_size_enum_to_buffer_size(buf_size);
	size_t stack_bytes = gxio_mpipe_calc_buffer_stack_bytes(num_buffers);
	mem_size = stack_bytes + num_buffers * size + 3*0x10000;

	//  page_size = tmc_alloc_get_huge_pagesize(); //16MB
	page_num = mem_size/page_size +1;
	if(page_num > MAX_MPIPE_STACK_PAGE_NUM)
	{
		fprintf(stderr, "mpipe stack buffer page num > %d",MAX_MPIPE_STACK_PAGE_NUM);

		return RET_FAIL;
	}
	ADD_LOG(LOG_LEVEL_DEBUG )
	(0,0,"alloc pages %d for buffer stack ",page_num);
	mem_size = page_num *page_size;	//16MB

	tmc_alloc_t alloc = TMC_ALLOC_INIT;
	tmc_alloc_set_huge(&alloc);
	tmc_alloc_set_shared(&alloc);
	tmc_alloc_set_pagesize(&alloc, page_size);//For 64M,Added by ffwen
	void* mem = tmc_alloc_map(&alloc, mem_size);
	if (mem == NULL)
	{
		fprintf(stderr, "cmem_malloc mpipe stack_buffer fail\n");
		return RET_FAIL;
	}
	//  printf("1 mem=%p, end=%p\n", mem, mem+page_size);
	ALIGN(mem, 0X10000);
	//  printf("2 mem=%p\n", mem);
	result = gxio_mpipe_init_buffer_stack(context, stack_idx, buf_size,
			mem, stack_bytes, 0);
	if(result<0)
	{
		ADD_LOG(LOG_LEVEL_DEBUG)
						(0,0, "gxio_mpipe_init large_stack id: %d kind :%d num: %d size:%llu ",g_mpipe_info->large_stack.stack_id, g_mpipe_info->large_stack.buffer_kind,g_mpipe_info->large_stack.buffer_num,g_mpipe_info->large_stack.buffer_size);


		fprintf(stderr, " init_large_buffer_stack error! \n");
		return RET_FAIL;
	}
	//  mem += stack_bytes;
	//  printf("3 mem=%p\n", mem);
	ALIGN(mem, 0x10000);  //align 64k
	//  printf("4 mem=%p\n", mem);
	for (int i = 0; i < page_num; i++)
	{
		result = gxio_mpipe_register_page(context, stack_idx,
				mem + i * page_size, page_size, 0);
		if(result < 0)
		{
			fprintf(stderr, "gxio_mpipe_register_page error=%s\n",gxio_strerror(result));
			fprintf(stderr, "stack_idx=%d, size=%lu, page_size=%lu\n",stack_idx, size, page_size);
			fprintf(stderr, "page_num=%d, i=%d, addr=%p\n",page_num, i, mem + i * page_size);
			return RET_FAIL;
		}
	}
	mem += stack_bytes;
	for (int i = 0; i < num_buffers; i++)
	{
		gxio_mpipe_push_buffer(context, stack_idx, mem);
		mem += size;
	}

	return RET_SUCC;

}



/* buffer stack */
static sb_s32 init_mpipe_buffer_stacks(tmc_alloc_t *alloc_ptr)
{
	sb_s32 ret = RET_SUCC;
	size_t page_size;
	size_t mem_size = 0;
	sb_s32 page_num;
	sb_s32 stack_id;
	sb_s32 i;
	sb_s32 iqueue_mem_total = 0;
	sb_s32 equeue_mem_total = 0;

	g_mpipe_info->iqueues = (i_mpipe_t*)tmc_alloc_map(alloc_ptr, sizeof(i_mpipe_t) * g_mpipe_info->iqueue_num);
	memset(g_mpipe_info->iqueues,0,sizeof(i_mpipe_t) * g_mpipe_info->iqueue_num);

	if(NULL == g_mpipe_info->iqueues)
	{
		printf("g_mpipe_info->iqueues 分配内存失败! line %d\n",__LINE__);
		return RET_FAIL;
	}

	for(i=0; i<g_mpipe_info->iqueue_num; i++)
	{
		g_mpipe_info->iqueues[i].iqueue = (gxio_mpipe_iqueue_t*)tmc_alloc_map(alloc_ptr, sizeof(gxio_mpipe_iqueue_t));
		if(NULL == g_mpipe_info->iqueues[i].iqueue)
		{
			fprintf(stderr,"alloc g_mpipe_info->iqueues[%d].iqueue failed\n", i);
			return RET_FAIL;
		}
		g_mpipe_info->iqueues[i].iqueue_mem_size = g_mpipe_info->iqueue_len * sizeof(gxio_mpipe_idesc_t);
		fprintf(stderr,"[%d]:iqueue mem size = %d\n", i, g_mpipe_info->iqueues[i].iqueue_mem_size);
		iqueue_mem_total += g_mpipe_info->iqueues[i].iqueue_mem_size;
	}

	//分配输出队列
	if(g_mpipe_info->equeue_num_dpi > 0)
	{
		g_mpipe_info->equeues_dpi = (e_mpipe_t*)tmc_alloc_map(alloc_ptr, sizeof(e_mpipe_t) * g_mpipe_info->equeue_num_dpi);
		memset(g_mpipe_info->equeues_dpi,0,sizeof(e_mpipe_t) * g_mpipe_info->equeue_num_dpi);

		if(NULL == g_mpipe_info->equeues_dpi)
		{
			printf("g_mpipe_info->equeues_dpi 分配内存失败! line %d\n",__LINE__);
			return RET_FAIL;
		}

		for(i=0; i<g_mpipe_info->equeue_num_dpi; i++)
		{
			g_mpipe_info->equeues_dpi[i].equeue = (gxio_mpipe_equeue_t*)tmc_alloc_map(alloc_ptr, sizeof(gxio_mpipe_equeue_t));
			if(NULL == g_mpipe_info->equeues_dpi[i].equeue)
			{
				fprintf(stderr,"alloc g_mpipe_info->equeues_dpi[%d].equeue failed\n", i);
				return RET_FAIL;
			}
			g_mpipe_info->equeues_dpi[i].equeue_mem_size = g_mpipe_info->equeue_len * sizeof(gxio_mpipe_edesc_t);
			fprintf(stderr,"[%d]:equeue mem size = %d\n", i, g_mpipe_info->equeues_dpi[i].equeue_mem_size);
			equeue_mem_total += g_mpipe_info->equeues_dpi[i].equeue_mem_size;
		}

	}

	if(g_mpipe_info->equeue_num_ql > 0)
	{
		g_mpipe_info->equeues_ql = (e_mpipe_t*)tmc_alloc_map(alloc_ptr, sizeof(e_mpipe_t) * g_mpipe_info->equeue_num_ql);
		memset(g_mpipe_info->equeues_ql,0,sizeof(e_mpipe_t) * g_mpipe_info->equeue_num_ql);

		if(NULL == g_mpipe_info->equeues_ql)
		{
			printf("g_mpipe_info->equeues_ql 分配内存失败! line %d\n",__LINE__);
			return RET_FAIL;
		}

		for(i=0; i<g_mpipe_info->equeue_num_ql; i++)
		{
			g_mpipe_info->equeues_ql[i].equeue = (gxio_mpipe_equeue_t*)tmc_alloc_map(alloc_ptr, sizeof(gxio_mpipe_equeue_t));
			if(NULL == g_mpipe_info->equeues_ql[i].equeue)
			{
				fprintf(stderr,"alloc g_mpipe_info->equeues_ql[%d].equeue failed\n", i);
				return RET_FAIL;
			}
			g_mpipe_info->equeues_ql[i].equeue_mem_size = g_mpipe_info->equeue_len * sizeof(gxio_mpipe_edesc_t);
			fprintf(stderr,"[%d]:equeue mem size = %d\n", i, g_mpipe_info->equeues_ql[i].equeue_mem_size);
			equeue_mem_total += g_mpipe_info->equeues_ql[i].equeue_mem_size;
		}

	}

	if(g_mpipe_info->equeue_num_psql > 0)
	{
		g_mpipe_info->equeues_psql = (e_mpipe_t*)tmc_alloc_map(alloc_ptr, sizeof(e_mpipe_t) * g_mpipe_info->equeue_num_psql);
		memset(g_mpipe_info->equeues_psql,0,sizeof(e_mpipe_t) * g_mpipe_info->equeue_num_psql);

		if(NULL == g_mpipe_info->equeues_psql)
		{
			printf("g_mpipe_info->equeues_psql 分配内存失败! line %d\n",__LINE__);
			return RET_FAIL;
		}

		for(i=0; i<g_mpipe_info->equeue_num_psql; i++)
		{
			g_mpipe_info->equeues_psql[i].equeue = (gxio_mpipe_equeue_t*)tmc_alloc_map(alloc_ptr, sizeof(gxio_mpipe_equeue_t));
			if(NULL == g_mpipe_info->equeues_psql[i].equeue)
			{
				fprintf(stderr,"alloc g_mpipe_info->equeues_psql[%d].equeue failed\n", i);
				return RET_FAIL;
			}
			g_mpipe_info->equeues_psql[i].equeue_mem_size = g_mpipe_info->equeue_len * sizeof(gxio_mpipe_edesc_t);
			fprintf(stderr,"[%d]:equeue mem size = %d\n", i, g_mpipe_info->equeues_psql[i].equeue_mem_size);
			equeue_mem_total += g_mpipe_info->equeues_psql[i].equeue_mem_size;
		}

	}

	page_size = tmc_alloc_get_huge_pagesize(); //16MB
	mem_size =  iqueue_mem_total + equeue_mem_total + 3*0x10000;
	page_num = mem_size/page_size +1;
	if(page_num > MAX_MPIPE_STACK_PAGE_NUM)
	{
		fprintf(stderr, "mpipe iqueue buffer page num > %d",MAX_MPIPE_STACK_PAGE_NUM);

		return RET_FAIL;
	}
	ADD_LOG(LOG_LEVEL_DEBUG )(0,0,"alloc pages %d for iqueue ",page_num);
	mem_size = page_num *page_size;	//16MB

	tmc_alloc_t alloc = TMC_ALLOC_INIT;
	tmc_alloc_set_huge(&alloc);
	tmc_alloc_set_shared(&alloc);
	//	tmc_alloc_set_home(&alloc, TMC_ALLOC_HOME_HASH);

	g_mpipe_info->iqueue_mems = tmc_alloc_map(&alloc, mem_size);

	if(g_mpipe_info->iqueue_mems == NULL)
	{
		fprintf(stderr, "cmem_malloc mpipe iqueue fail, size is %lu ",mem_size);
		return RET_FAIL;
	}

	void *mem_pos = g_mpipe_info->iqueue_mems;
	//	void *mem_end = mem_pos + mem_size;
	// iqueue
	for(i = 0; i < g_mpipe_info->iqueue_num; i++)
	{
		sb_s32 ring_id = g_mpipe_info->iring_id + i;

		g_mpipe_info->iqueues[i].iqueue_mem = mem_pos;
		fprintf(stderr,"[%d] ring_id = %d, iqueue_mem = %p\n", i, ring_id, mem_pos);
		mem_pos += g_mpipe_info->iqueues[i].iqueue_mem_size;
		ret = gxio_mpipe_iqueue_init(g_mpipe_info->iqueues[i].iqueue,
				&g_mpipe_info->context,
				ring_id,
				g_mpipe_info->iqueues[i].iqueue_mem,
				g_mpipe_info->iqueues[i].iqueue_mem_size,0);
		if(ret<0)
		{
			fprintf(stderr,"gxio_mpipe_iqueue_init array[%d] error\n",i);
			ADD_LOG(LOG_LEVEL_FAILURE )
			(0,0, "gxio_mpipe_iqueue_init array[%d] ring_id is %d,iqueue_len %d  ,iqueue_size %d, ret is %d error\n",
					i,ring_id,g_mpipe_info->iqueue_len,g_mpipe_info->iqueues[i].iqueue_mem_size,ret);

			return RET_FAIL;
		}
		fprintf(stdout,"gxio_mpipe_iqueue_init ring_id %d,iqueue_len %d\n ",ring_id,g_mpipe_info->iqueue_len);
		ADD_LOG(LOG_LEVEL_DEBUG )
		(0,0, "gxio_mpipe_iqueue_init ring_id %d,iqueue_len %d  ,iqueue_size %d, sizeof(idesc) %d \n",
				ring_id,g_mpipe_info->iqueue_len,g_mpipe_info->iqueues[i].iqueue_mem_size,sizeof(gxio_mpipe_idesc_t));

	}

	//分配输出描述符队列
	if(g_mpipe_info->equeue_num_dpi > 0)
	{
		for(i = 0; i < g_mpipe_info->equeue_num_dpi; i++)
		{
			sb_s32 ring_id = g_mpipe_info->ering_id + i;

			g_mpipe_info->equeues_dpi[i].equeue_mem = mem_pos;
			fprintf(stderr,"[%d] ring_id = %d, equeue_mem = %p\n", i, ring_id, mem_pos);
			mem_pos += g_mpipe_info->equeues_dpi[i].equeue_mem_size;
			ret = gxio_mpipe_equeue_init(g_mpipe_info->equeues_dpi[i].equeue, &g_mpipe_info->context, ring_id, g_mpipe_info->echannels_dpi[i].channel_id,
							g_mpipe_info->equeues_dpi[i].equeue_mem, g_mpipe_info->equeues_dpi[i].equeue_mem_size, 0);

			if(ret<0)
			{
				fprintf(stderr,"gxio_mpipe_equeue_init array[%d] error\n",i);
				ADD_LOG(LOG_LEVEL_FAILURE )
				(0,0, "gxio_mpipe_equeue_init array[%d] ring_id is %d,equeue_len %d  ,equeue_size %d, ret is %d error\n",
						i,ring_id,g_mpipe_info->equeue_len,g_mpipe_info->equeues_dpi[i].equeue_mem_size,ret);

				return RET_FAIL;
			}
			fprintf(stdout,"gxio_mpipe_equeue_init ring_id %d,equeue_len %d\n ",ring_id,g_mpipe_info->equeue_len);
			ADD_LOG(LOG_LEVEL_DEBUG )
			(0,0, "gxio_mpipe_equeue_init ring_id %d,equeue_len %d  ,equeue_size %d, sizeof(edesc) %d \n",
					ring_id,g_mpipe_info->equeue_len,g_mpipe_info->equeues_dpi[i].equeue_mem_size,sizeof(gxio_mpipe_edesc_t));

		}

	}

	if(g_mpipe_info->equeue_num_ql > 0)
	{
		for(i = 0; i < g_mpipe_info->equeue_num_ql; i++)
		{
			sb_s32 ring_id = g_mpipe_info->ering_id + g_mpipe_info->equeue_num_dpi + i;

			g_mpipe_info->equeues_ql[i].equeue_mem = mem_pos;
			fprintf(stderr,"[%d] ring_id = %d, equeue_mem = %p\n", i, ring_id, mem_pos);
			mem_pos += g_mpipe_info->equeues_ql[i].equeue_mem_size;
			ret = gxio_mpipe_equeue_init(g_mpipe_info->equeues_ql[i].equeue, &g_mpipe_info->context, ring_id, g_mpipe_info->echannels_ql[i].channel_id,
							g_mpipe_info->equeues_ql[i].equeue_mem, g_mpipe_info->equeues_ql[i].equeue_mem_size, 0);

			if(ret<0)
			{
				fprintf(stderr,"gxio_mpipe_equeue_init array[%d] error\n",i);
				ADD_LOG(LOG_LEVEL_FAILURE )
				(0,0, "gxio_mpipe_equeue_init array[%d] ring_id is %d,equeue_len %d  ,equeue_size %d, ret is %d error\n",
						i,ring_id,g_mpipe_info->equeue_len,g_mpipe_info->equeues_ql[i].equeue_mem_size,ret);

				return RET_FAIL;
			}
			fprintf(stdout,"gxio_mpipe_equeue_init ring_id %d,equeue_len %d\n ",ring_id,g_mpipe_info->equeue_len);
			ADD_LOG(LOG_LEVEL_DEBUG )
			(0,0, "gxio_mpipe_equeue_init ring_id %d,equeue_len %d  ,equeue_size %d, sizeof(edesc) %d \n",
					ring_id,g_mpipe_info->equeue_len,g_mpipe_info->equeues_ql[i].equeue_mem_size,sizeof(gxio_mpipe_edesc_t));

		}

	}

	if(g_mpipe_info->equeue_num_psql > 0)
	{
		for(i = 0; i < g_mpipe_info->equeue_num_psql; i++)
		{
			sb_s32 ring_id = g_mpipe_info->ering_id + g_mpipe_info->equeue_num_dpi + g_mpipe_info->equeue_num_ql + i;

			g_mpipe_info->equeues_psql[i].equeue_mem = mem_pos;
			fprintf(stderr,"[%d] ring_id = %d, equeue_mem = %p\n", i, ring_id, mem_pos);
			mem_pos += g_mpipe_info->equeues_psql[i].equeue_mem_size;
			ret = gxio_mpipe_equeue_init(g_mpipe_info->equeues_psql[i].equeue, &g_mpipe_info->context, ring_id, g_mpipe_info->echannels_psql[i].channel_id,
							g_mpipe_info->equeues_psql[i].equeue_mem, g_mpipe_info->equeues_psql[i].equeue_mem_size, 0);

			if(ret<0)
			{
				fprintf(stderr,"gxio_mpipe_equeue_init array[%d] error\n",i);
				ADD_LOG(LOG_LEVEL_FAILURE )
				(0,0, "gxio_mpipe_equeue_init array[%d] ring_id is %d,equeue_len %d  ,equeue_size %d, ret is %d error\n",
						i,ring_id,g_mpipe_info->equeue_len,g_mpipe_info->equeues_psql[i].equeue_mem_size,ret);

				return RET_FAIL;
			}
			fprintf(stdout,"gxio_mpipe_equeue_init ring_id %d,equeue_len %d\n ",ring_id,g_mpipe_info->equeue_len);
			ADD_LOG(LOG_LEVEL_DEBUG )
			(0,0, "gxio_mpipe_equeue_init ring_id %d,equeue_len %d  ,equeue_size %d, sizeof(edesc) %d \n",
					ring_id,g_mpipe_info->equeue_len,g_mpipe_info->equeues_psql[i].equeue_mem_size,sizeof(gxio_mpipe_edesc_t));

		}

	}

	g_mpipe_info->large_stack.buffer_kind = GXIO_MPIPE_BUFFER_SIZE_1664;
	g_mpipe_info->large_stack.buffer_size = 1664;
	g_mpipe_info->large_stack.buffer_num = g_adv_config.mpipe_queue_buffer_size * g_adv_config.sig_parser_cpus;

	g_mpipe_info->small_stack.buffer_kind = GXIO_MPIPE_BUFFER_SIZE_256;
	g_mpipe_info->small_stack.buffer_size = 256;
	g_mpipe_info->small_stack.buffer_num = g_adv_config.mpipe_queue_buffer_size * g_adv_config.sig_parser_cpus;

	stack_id = gxio_mpipe_alloc_buffer_stacks(&g_mpipe_info->context,2, 0 ,0 );
	if(stack_id <0 )
	{
		fprintf(stderr," alloc buffer_stacks fail!");
		return RET_FAIL;
	}
	g_mpipe_info->small_stack.stack_id = stack_id;
	g_mpipe_info->large_stack.stack_id = stack_id + 1;
	printf("stack_id=%d\n", stack_id);
	ret = create_stack(&g_mpipe_info->context, g_mpipe_info->small_stack.stack_id,
			GXIO_MPIPE_BUFFER_SIZE_256, g_mpipe_info->small_stack.buffer_num);
	if(ret < 0)
	{
		fprintf(stderr," create_stack small fail!\n");
		return RET_FAIL;
	}
	ret = create_stack(&g_mpipe_info->context, g_mpipe_info->large_stack.stack_id,
			GXIO_MPIPE_BUFFER_SIZE_1664, g_mpipe_info->large_stack.buffer_num);
	if(ret < 0)
	{
		fprintf(stderr," create_stack large fail!\n");
		return RET_FAIL;
	}

	return RET_SUCC;
}

/**/
static sb_s32 init_mpipe_queues()
{
	sb_s32 ring_id;
	sb_s32 ret = RET_SUCC;

	g_mpipe_info->iqueue_num = g_adv_config.sig_parser_cpus;
	if(g_mpipe_info->iqueue_num<=0)
	{
		fprintf(stderr,"iqueue_num <=0 ,error!");
		return RET_FAIL;
	}
	g_mpipe_info->iqueue_len = g_adv_config.mpipe_iqueue_size;
	if(g_mpipe_info->iqueue_len <=0)
	{
		fprintf(stderr,"iqueue_len <=0 ,error!");
		return RET_FAIL;
	}

	ring_id = gxio_mpipe_alloc_notif_rings(&g_mpipe_info->context,g_mpipe_info->iqueue_num,0,0);
	if(ring_id <0)
	{
		fprintf(stderr,"gxio_mpipe_alloc_notif_rings error !");
		return RET_FAIL;
	}
	g_mpipe_info->iring_id = ring_id;
	ADD_LOG(LOG_LEVEL_DEBUG )
	(0,0, "gxio_mpipe_alloc_notif_rings ring_id %d,iqueue_num is %d ",ring_id,g_mpipe_info->iqueue_num);

	g_mpipe_info->equeue_num_dpi = g_device_config.dpi_send_port_num;
	g_mpipe_info->equeue_num_ql = g_device_config.ql_send_port_num;
	g_mpipe_info->equeue_num_psql = g_device_config.psql_send_port_num;
	g_mpipe_info->equeue_num = g_mpipe_info->equeue_num_dpi + g_mpipe_info->equeue_num_ql + g_mpipe_info->equeue_num_psql;
	g_mpipe_info->equeue_len = g_adv_config.mpipe_equeue_size;
	if(g_mpipe_info->equeue_num > 0)
	{

		ring_id = gxio_mpipe_alloc_edma_rings(&g_mpipe_info->context,
				g_mpipe_info->equeue_num,0,0);
		if(ring_id <0)
		{
			fprintf(stderr,"gxio_mpipe_alloc_edma_rings error !");
			return RET_FAIL;
		}
		g_mpipe_info->ering_id = ring_id;
		ADD_LOG(LOG_LEVEL_DEBUG )
		(0,0, "gxio_mpipe_alloc_edma_rings ring_id %d,equeue_num is %d ",ring_id, g_mpipe_info->equeue_num);

	}

	return ret;
}

/* mpipe*/
static sb_s32 set_mpipe_rules()
{
	sb_s32 ret = RET_SUCC;
	sb_s32 i = 0;
	sb_s32 group;
	sb_s32 bucket;
	gxio_mpipe_bucket_mode_t mode;

	ret = gxio_mpipe_alloc_notif_groups(&g_mpipe_info->context, 1,0,0);

	if(ret <0 )
	{
		fprintf(stderr,"gxio_mpipe_alloc_notif_groups error !\n");
		return RET_FAIL;
	}
	group = ret;

	ret = gxio_mpipe_alloc_buckets(&g_mpipe_info->context, g_mpipe_info->iqueue_num,0,0);
	ADD_LOG(LOG_LEVEL_DEBUG )
	(0,0, "alloc buckets num :%d",g_mpipe_info->iqueue_num);
	if(ret<0)
	{
		fprintf(stderr,"gxio_mpipe_alloc_buckets error!\n");
		return RET_FAIL;
	}
	bucket = ret;

	mode = GXIO_MPIPE_BUCKET_ROUND_ROBIN ;
	ret = gxio_mpipe_init_notif_group_and_buckets(&g_mpipe_info->context,group,g_mpipe_info->iring_id,g_mpipe_info->iqueue_num,	bucket,g_mpipe_info->iqueue_num,mode);
	ADD_LOG(LOG_LEVEL_DEBUG )
	(0,0,"add rule group:%d , first ring :%d, ring_num :%d , first bucket :%d ,bucket+num :%d", group, g_mpipe_info->iring_id,g_mpipe_info->iqueue_num,bucket,g_mpipe_info->iqueue_num);


	gxio_mpipe_rules_init(&g_mpipe_info->rules,&g_mpipe_info->context);
	gxio_mpipe_rules_begin(&g_mpipe_info->rules,bucket, g_mpipe_info->iqueue_num,NULL);

	for(i=0;i<g_mpipe_info->channel_num;i++)
	{
		ADD_LOG(LOG_LEVEL_DEBUG )
							(0,0,"add channel:%d ", g_mpipe_info->ichannels[i].channel_id);



		ret = gxio_mpipe_rules_add_channel(&g_mpipe_info->rules, g_mpipe_info->ichannels[i].channel_id);
		if(ret<0)
		{
			ADD_LOG(LOG_LEVEL_FAILURE)
								(0,0," gxio_mpipe_rules_add_channel %d failed !  ",g_mpipe_info->ichannels[i].channel_id);

		}

	}
	ret = gxio_mpipe_rules_commit(&g_mpipe_info->rules);
	if(ret <0 )
	{
		fprintf(stderr,"gxio_mpipe_rules_commit() error! %d \n",ret );
		ADD_LOG(LOG_LEVEL_FAILURE)
		(0,0,"gxio_mpipe_rules_commit() failed! %d ",ret);
		return RET_FAIL;
	}

	return RET_SUCC;
}

sb_s32 init_mpipe_net(tmc_alloc_t *alloc_ptr)
{
	sb_s32 ret = RET_SUCC;
	if(0 >= g_adv_config.sig_parser_cpus)
	{
		ADD_LOG(LOG_LEVEL_FAILURE)
							(0,0, "sig_parser cpus <= 0!");
		return RET_FAIL;
	}


	g_mpipe_info = (net_mpipe_t *)tmc_alloc_map(alloc_ptr, sizeof(net_mpipe_t));
	if(g_mpipe_info == NULL)
	{
		fprintf(stderr,"%s[%d]: cmem alloc g_mpipe_info fail!\n",g_process_name,g_process_order);
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0, "cmem alloc g_mpipe_info fail!");

		return RET_FAIL;
	}
	memset(g_mpipe_info,0,sizeof(net_mpipe_t));

	//
	ret = init_mpipe_channels(alloc_ptr);
	if(ret  == RET_FAIL)
	{
		fprintf(stderr,"%s[%d]: init_mpipe_channel fail!\n",g_process_name,g_process_order);
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0, "init_mpipe_channel fail!");

		return RET_FAIL;
	}

	//
	ret = init_mpipe_queues();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"%s[%d]: init_mpipe_iqueues fail!\n",g_process_name,g_process_order);
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0, "init_mpipe_iqueues fail!");

		return RET_FAIL;
	}


	ret = init_mpipe_buffer_stacks(alloc_ptr);
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"%s[%d]: init_mpipe_buffer_stacks fail!\n",g_process_name,g_process_order);
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0, "init_mpipe_buffer_stacks fail!");

		return RET_FAIL;
	}

	ret = set_mpipe_rules();
	if(ret == RET_FAIL)
	{
		fprintf(stderr,"%s[%d]: init_mpipe_rules fail!\n",g_process_name,g_process_order);
		ADD_LOG (LOG_LEVEL_FAILURE)
		(0, 0, "init_mpipe_rules fail!");

		return RET_FAIL;
	}

	struct timespec ts;
	ts.tv_sec = g_current_time;
	ts.tv_nsec = 0;
	gxio_mpipe_set_timestamp(&g_mpipe_info->context, &ts);

	return ret;

}

sb_s32 set_mpipe_rules_and_open()
{
	sb_s32 ret = RET_SUCC;

	return ret;
}

//mpipe

sb_s32 close_mpipe_net(void)
{
	sb_s32 i= 0;
	//disable all links
	//	sim_disable_mpipe_links( g_mpipe_info->instance,-1);
	if (NULL == g_mpipe_info )
		return RET_FAIL;
	gxio_mpipe_rules_init(&g_mpipe_info->rules,&g_mpipe_info->context);
	gxio_mpipe_rules_commit(&g_mpipe_info->rules);

	if(NULL != g_mpipe_info->ichannels)
		for( i= 0; i< g_mpipe_info->channel_num ;i++)
		{
			gxio_mpipe_link_close(&g_mpipe_info->ichannels[i].link);
			fprintf(stdout,"close mpipe link %d\n",i);
		}

	if(g_device_config.dpi_send_port_num > 0)
	{
		if(NULL != g_mpipe_info->echannels_dpi)
			for(i = 0; i < g_device_config.dpi_send_port_num; i++)
			{
				gxio_mpipe_link_close(&g_mpipe_info->echannels_dpi[i].link);
			}
	}

	if(g_device_config.ql_send_port_num > 0)
	{
		if(NULL != g_mpipe_info->echannels_ql)
			for(i = 0; i < g_device_config.ql_send_port_num; i++)
			{
				gxio_mpipe_link_close(&g_mpipe_info->echannels_ql[i].link);
			}
	}

	if(g_device_config.psql_send_port_num > 0)
	{
		if(NULL != g_mpipe_info->echannels_psql)
			for(i = 0; i < g_device_config.psql_send_port_num; i++)
			{
				gxio_mpipe_link_close(&g_mpipe_info->echannels_psql[i].link);
			}
	}

	close(g_mpipe_info->context.fd);

	return RET_SUCC;
}



