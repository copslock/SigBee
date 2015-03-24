/*
 * read_config.c
 *
 *  Created on: 2014年6月8日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

//tilera 相关头文件
#include <tmc/cmem.h>
#include <tmc/mem.h>
#include <tmc/sync.h>
#include <tmc/cpus.h>
#include <tmc/spin.h>
#include <tmc/task.h>
#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_xdr_struct.h"
#include "sb_error_code.h"
#include "init.h"
#include "sb_base_util.h"
#include "sb_shared_queue.h"
#include "sb_base_mpipe.h"
#include "sb_base_process.h"
#include "sb_log.h"
#include "sb_logd.h"
#include "sb_qlog.h"
#include "sb_public_var.h"

#define DEVICE_INI_ITEM_COUNT 30	//!< DEVICE_INI_FILENAME全局配置文件条目数
#define CONFIG_STR_SEPARATOR "\t "
#define SERVER_STR_SEPARATOR ","
#define CI_STR_SEPARATOR "-"

/* 局部函数的实现*/

/**********************************************************************
函数说明：从字符串读出配置项放入配置结构

输入参数：sb_u8 *line：配置行
          adv_config_t *device_config：配置结

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 in_read_config_from_string(sb_u8 *line, adv_config_t *device_config)
        \brief 从字符串读出配置项放入配置结构中
        \param[in] line 配置
        \param[out] device_config 写入配置信息的配置结
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
 */
sb_s32 in_read_config_from_string (sb_s8 * line, adv_config_t * device_config)
{

	/*!
<b>全局变量:</b>
	 */

	/*!
<b>流程:</b>
	 */
	sb_s32 ret = 0;

	sb_s8 tmp_buf[SB_BUF_LEN_LARGE + 100] = { 0 };

	/*分割后字符串指针 */
	sb_s8 *str_key = NULL;
	sb_s8 *str_1 = NULL;

	sb_s8 *ptrptr = NULL;

	//!     - 判断参数有效
	if ((line == NULL) || (device_config == NULL))
	{
		return RET_FAIL;
	}

	//!     - 空行及注释行返回RET_FAIL
	if (('\0' == *line) || ('#' == *line))
	{
		return RET_FAIL;
	}

	//! - 将line拷贝到tmp_buf
	strncpy ((sb_s8 *)tmp_buf, (sb_s8 *)line, sizeof (tmp_buf) - 1);

	//!     - 按\ref CONFIG_STR_SEPARATOR 分隔tmp_buf，前部分赋给str_key
	str_key =
			(sb_s8 *) strtok_r ((sb_s8 *)tmp_buf, CONFIG_STR_SEPARATOR, (sb_s8 **)&ptrptr);

	//!     - 如果str_key为NULL或空字符串，返回RET_FAIL
	if (NULL == str_key)
	{
		return RET_FAIL;
	}
	if (str_key[0] == '\0')
	{
		return RET_FAIL;
	}

	str_1 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
	if (NULL != str_1)
	{
		chars_all_trim (str_1);
	}


	if (strcasecmp ((sb_s8 *) str_key, "dataplane_cpu_start") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->dataplane_cpu_start));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "sig_parser_cpus") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->sig_parser_cpus));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "data_parser_cpus") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->data_parser_cpus));
		return ret;
	}

	if (strcasecmp((sb_s8 *) str_key, "xdr_sender_cpus") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->xdr_sender_cpus));
		return ret;
	}

	/*log_level 0 */
	/*!     - &nbsp;&nbsp;&nbsp;
	   log_level: 调用\ref in_write_signed_integer
	 */
	if (strcasecmp ((sb_s8 *)str_key, "log_level") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->log_level));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "mpipe_iqueue_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->mpipe_iqueue_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "mpipe_equeue_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->mpipe_equeue_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "mpipe_queue_buffer_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->mpipe_queue_buffer_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "sig_xdr_queue_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->sig_xdr_queue_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "data_xdr_queue_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->data_xdr_queue_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "sig_user_queue_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->sig_user_queue_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "sig_data_queue_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->sig_data_queue_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "dpi_user_queue_size") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->dpi_user_queue_size));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "user_timeout") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->user_timeout));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "xdr_timeout") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->xdr_timeout));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "xdr_event_timeslice") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->xdr_event_timeslice));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "tcp_udp_timeout") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->tcp_udp_timeout));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "tcp_udp_inactive_gap") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->tcp_udp_inactive_gap));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "attach_request_timeout") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->attach_request_timeout));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "rau_request_timeout") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->rau_request_timeout));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "pdp_request_timeout") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->pdp_request_timeout));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "detach_request_timeout") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->detach_request_timeout));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "wtp_transaction_inactive_gap") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->wtp_transaction_inactive_gap));
		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "wtp_inactive_gap") == 0)
	{
		ret = in_write_signed_integer (str_1, &(device_config->wtp_inactive_gap));
		return ret;
	}

	return RET_FAIL;
}

/* 局部函数的实现*/

sb_s32 in_parse_if(sb_s8 *line, cap_interface_t *cap)
{
	sb_s32 ret = 0;
	sb_s8 *str_2 = NULL;
	sb_s32 i = 0;

	sb_s8 *ptrptr = NULL;

	str_2 = (sb_s8 *) strtok_r (line, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
	while (NULL != str_2)
	{
		sb_u8 repeat = 0;

		chars_all_trim (str_2);
		//需要检查是否重复
		if(cap->interface_num > 0)
		{
			for(i = 0; i < cap->interface_num; i++)
			{
				if(strcasecmp(str_2, cap->interface_name[i]) == 0)
				{
					repeat = 1;
					break;
				}
			}
		}
		if(0 == repeat)
		{
			ret = in_write_string (str_2,
					cap->interface_name[cap->interface_num],
					sizeof(cap->interface_name[cap->interface_num]) - 1);
			cap->interface_num++;
			if(cap->interface_num >= CAPTURE_INTERFACE_SIZE)break;
		}
		str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
	}

	return ret;
}
/**********************************************************************
函数说明：从字符串读出配置项放入配置结构

输入参数：sb_u8 *line：配置行
          device_config_t *device_config：配置结

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 in_read_dev_config_from_string(sb_u8 *line, device_config_t *device_config)
        \brief 从字符串读出配置项放入配置结构中
        \param[in] line 配置
        \param[out] device_config 写入配置信息的配置结
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
 */
sb_s32 in_read_dev_config_from_string (sb_s8 * line, device_config_t * device_config)
{

	/*!
<b>全局变量:</b>
	 */

	/*!
<b>流程:</b>
	 */
	sb_s32 ret = 0;

	sb_s8 tmp_buf[SB_BUF_LEN_LARGE + 100] = { 0 };

	/*分割后字符串指针 */
	sb_s8 *str_key = NULL;
	sb_s8 *str_1 = NULL;
	sb_s8 *str_2 = NULL;
	sb_s32 i = 0;

	sb_s8 *ptrptr = NULL;

	//!     - 判断参数有效
	if ((line == NULL) || (device_config == NULL))
	{
		return RET_FAIL;
	}

	//!     - 空行及注释行返回RET_FAIL
	if (('\0' == *line) || ('#' == *line))
	{
		return RET_FAIL;
	}

	//! - 将line拷贝到tmp_buf
	strncpy ((sb_s8 *)tmp_buf, (sb_s8 *)line, sizeof (tmp_buf) - 1);

	//!     - 按\ref CONFIG_STR_SEPARATOR 分隔tmp_buf，前部分赋给str_key
	str_key =
			(sb_s8 *) strtok_r ((sb_s8 *)tmp_buf, CONFIG_STR_SEPARATOR, (sb_s8 **)&ptrptr);

	//!     - 如果str_key为NULL或空字符串，返回RET_FAIL
	if (NULL == str_key)
	{
		return RET_FAIL;
	}
	if (str_key[0] == '\0')
	{
		return RET_FAIL;
	}

	str_1 = (sb_s8 *) strtok_r (NULL, CONFIG_STR_SEPARATOR, (sb_s8 **)&ptrptr);
	if (NULL != str_1)
	{
		chars_all_trim (str_1);
	}


	if (strcasecmp ((sb_s8 *) str_key, "device_id") == 0)
	{
		ret = in_write_unsigned_integer (str_1, &(device_config->device_identity));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "lb_exist") == 0)
	{
		ret = in_write_unsigned_integer (str_1, &(device_config->is_lb_exist));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "cap_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->cap_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "gb_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->gb_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "iups_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->iups_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "gn_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->gn_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "s1mme_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->s1mme_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "s1u_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->s1u_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "s6a_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->s6a_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "s10_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->s10_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "s11_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->s11_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "s5_interface") == 0)
	{
		ret = in_parse_if(str_1, (cap_interface_t *)&device_config->s5_ports);

		return ret;
	}

	if (strcasecmp ((sb_s8 *)str_key, "xdr_server_ip") == 0)
	{
		ret = in_write_ip_int (str_1, &(device_config->xdr_dev_ip));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "xdr_server_port") == 0)
	{
		ret = in_write_unsigned_short (str_1, &(device_config->xdr_dev_port));
		return ret;
	}

	//dpi device configure
	if (strcasecmp ((sb_s8 *) str_key, "dpi_flag") == 0)
	{
		ret = in_write_unsigned_integer (str_1, &(device_config->dpi_flag));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "dpi_devip") == 0)
	{
		str_2 = (sb_s8 *) strtok_r (str_1, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		while (NULL != str_2)
		{
			chars_all_trim (str_2);
			//不需要检查是否重复，IP地址可以相同

			ret = in_write_ip_int (str_2,&device_config->dpi_dev[device_config->dpi_dev_num].dev_ip);
			device_config->dpi_dev_num++;
			if(device_config->dpi_dev_num >= MAX_DEV_NUM)break;

			str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		}
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "dpi_devid") == 0)
	{
		i = 0;

		str_2 = (sb_s8 *) strtok_r (str_1, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		while (NULL != str_2)
		{
			chars_all_trim (str_2);

			ret = in_write_unsigned_integer (str_2,&device_config->dpi_dev[i].dev_id);
			i++;
			if(i >= device_config->dpi_dev_num)break;

			str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		}
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "dpi_data_interface") == 0)
	{
		str_2 = (sb_s8 *) strtok_r (line, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		while (NULL != str_2)
		{
			sb_u8 repeat = 0;

			chars_all_trim (str_2);
			//需要检查是否重复
			if(device_config->dpi_send_port_num > 0)
			{
				for(i = 0; i < device_config->dpi_send_port_num; i++)
				{
					if(strcasecmp(str_2, device_config->dpi_interface_name[i]) == 0)
					{
						repeat = 1;
						break;
					}
				}
			}
			if(0 == repeat)
			{
				ret = in_write_string (str_2,
						device_config->dpi_interface_name[device_config->dpi_send_port_num],
						sizeof(device_config->dpi_interface_name[device_config->dpi_send_port_num]) - 1);
				device_config->dpi_send_port_num++;
				if(device_config->dpi_send_port_num >= MAX_DEV_NUM)break;
			}
			str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		}
	}

	//全量数据发送设备 configure
	if (strcasecmp ((sb_s8 *) str_key, "ql_flag") == 0)
	{
		ret = in_write_unsigned_integer (str_1, &(device_config->ql_flag));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "ql_devid") == 0)
	{
		str_2 = (sb_s8 *) strtok_r (str_1, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		while (NULL != str_2)
		{
			chars_all_trim (str_2);

			ret = in_write_unsigned_integer (str_2,&device_config->ql_dev_id[device_config->ql_dev_num]);
			device_config->ql_dev_num++;
			if(device_config->ql_dev_num >= MAX_DEV_NUM)break;

			str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		}
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "ql_data_interface") == 0)
	{
		str_2 = (sb_s8 *) strtok_r (line, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		while (NULL != str_2)
		{
			sb_u8 repeat = 0;
			chars_all_trim (str_2);
			//需要检查是否重复
			if(device_config->ql_send_port_num > 0)
			{
				for(i = 0; i < device_config->ql_send_port_num; i++)
				{
					if(strcasecmp(str_2, device_config->ql_interface_name[i]) == 0)
					{
						repeat = 1;
						break;
					}
				}
			}
			if(0 == repeat)
			{
				ret = in_write_string (str_2,
						device_config->ql_interface_name[device_config->ql_send_port_num],
						sizeof(device_config->ql_interface_name[device_config->ql_send_port_num]) - 1);
				device_config->ql_send_port_num++;
				if(device_config->ql_send_port_num >= MAX_DEV_NUM)break;
			}
			str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		}
	}

	//PS全量数据发送设备 configure
	if (strcasecmp ((sb_s8 *) str_key, "psql_flag") == 0)
	{
		ret = in_write_unsigned_integer (str_1, &(device_config->psql_flag));
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "psql_devid") == 0)
	{
		str_2 = (sb_s8 *) strtok_r (str_1, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		while (NULL != str_2)
		{
			chars_all_trim (str_2);

			ret = in_write_unsigned_integer (str_2,&device_config->psql_dev_id[device_config->psql_dev_num]);
			device_config->psql_dev_num++;
			if(device_config->psql_dev_num >= MAX_DEV_NUM)break;

			str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		}
		return ret;
	}

	if (strcasecmp ((sb_s8 *) str_key, "psql_data_interface") == 0)
	{
		str_2 = (sb_s8 *) strtok_r (line, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		while (NULL != str_2)
		{
			sb_u8 repeat = 0;

			chars_all_trim (str_2);
			//需要检查是否重复
			if(device_config->psql_send_port_num > 0)
			{
				for(i = 0; i < device_config->psql_send_port_num; i++)
				{
					if(strcasecmp(str_2, device_config->psql_interface_name[i]) == 0)
					{
						repeat = 1;
						break;
					}
				}
			}
			if(0 == repeat)
			{
				ret = in_write_string (str_2,
						device_config->psql_interface_name[device_config->psql_send_port_num],
						sizeof(device_config->psql_interface_name[device_config->psql_send_port_num]) - 1);
				device_config->psql_send_port_num++;
				if(device_config->psql_send_port_num >= MAX_DEV_NUM)break;
			}
			str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		}
	}

	return ret;
}

/**********************************************************************
函数说明：将device_ini_filename中的配置内容读取到结构变量g_device_config
输入参数：sb_u8 * device_ini_filename：系统配置文件名

输出参数：无

全局变量：device_config_t g_device_config

函数返回：RET_SUCC：成功
          RET_FAIL：失败
		  RET_OTHER：不完全正确

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 read_dev_config(sb_u8 * device_ini_filename)
        \brief 将device_ini_filename中的配置内容读取到结构变量g_device_config
        \param[in] device_ini_filename 全局配置文件名称
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
		\retval RET_OTHER 不完全正确
 */
sb_s32 read_dev_config (sb_s8 * device_ini_filename)
{

	/*!
<b>全局变量:</b> \ref g_device_config
	 */

	/*!
<b>流程:</b>
	 */

	FILE *fp = NULL;
	sb_s8 line_buf[SB_BUF_LEN_LARGE + 200] = { 0 };
	sb_s8 *line_buf_head = NULL;
	sb_s32 ret;

	memset (&g_device_config, 0, sizeof (device_config_t));


	//!     - 判断参数有效性
	if (device_ini_filename == NULL)
	{
		return RET_FAIL;
	}
	if (device_ini_filename[0] == '\0')
	{
		return RET_FAIL;
	}

	//! - 打开配置文件device_ini_filename
	fp = fopen ((char *)device_ini_filename, "r");
	if (fp != NULL)
	{
		while (1)
		{
			if (feof (fp))
			{
				break;
			}
			memset (line_buf, 0, sizeof (line_buf));
			if (fgets ((char *)line_buf, sizeof (line_buf) - 1, fp) == NULL)
			{
				break;
			}

			/*跳过最前面的空格、tab符号 */
			line_buf_head = line_buf;
			while ((*line_buf_head == ' ') || (*line_buf_head == '\t'))
			{
				if ((*line_buf_head == '\0') || (toascii (*line_buf_head) == 10)
						|| (toascii (*line_buf_head) == 13))
				{
					line_buf_head = NULL;	/*读到结束符、回车换行符认为是空字符串，无效字符*/
					break;
				}
				line_buf_head++;
			}
			if (line_buf_head == NULL)
			{
				continue;
			}

			chars_all_trim (line_buf_head);

			if (('\0' == *line_buf_head) || ('#' == *line_buf_head))
			{
				continue;
			}

			/*根据"全局配置文件device.ini"device配置文件读入后的结构"g_device_config
			   的对应结构进行赋值，实现字符串的大小写无关处理*/
			ret = in_read_dev_config_from_string (line_buf_head, &g_device_config);
			if (RET_SUCC != ret)
			{
				fprintf (stdout, "读取配置项%s 失败\n", line_buf_head);
				continue;
			}
		}

		fclose (fp);
	}
	else
	{
		fprintf(stderr,"Open file %s失败\n",device_ini_filename);
		return RET_FAIL;
	}

	if (0 == g_device_config.cap_ports.interface_num)
	{
		fprintf(stderr,"No any capture interface has been set!\n");
		return RET_FAIL;
	}

	if (0 == g_device_config.xdr_dev_ip || 0 == g_device_config.xdr_dev_port)
	{
		fprintf(stderr,"No any xdr server has been set!\n");
		return RET_FAIL;
	}

	if(g_device_config.dpi_flag > 0 && (g_device_config.dpi_dev_num == 0 || g_device_config.dpi_send_port_num == 0 ))
	{
		fprintf(stderr,"No any dpi server has been set!\n");
		return RET_FAIL;
	}

	if(g_device_config.ql_flag > 0 && (g_device_config.ql_dev_num == 0 || g_device_config.ql_send_port_num == 0 ))
	{
		fprintf(stderr,"No any quanliang server has been set!\n");
		return RET_FAIL;
	}

	if(g_device_config.psql_flag > 0 && (g_device_config.psql_dev_num == 0 || g_device_config.psql_send_port_num == 0 ))
	{
		fprintf(stderr,"No any ps quanliang server has been set!\n");
		return RET_FAIL;
	}

	return RET_SUCC;
}


/**********************************************************************
函数说明：将ini_filename中的配置内容读取到结构变量g_adv_config
输入参数：sb_u8 * ini_filename：系统配置文件名

输出参数：无

全局变量：adv_config_t g_adv_config

函数返回：RET_SUCC：成功
          RET_FAIL：失败
		  RET_OTHER：不完全正确

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 read_adv_config(sb_u8 * device_ini_filename)
        \brief 将ini_filename中的配置内容读取到结构变量g_adv_config
        \param[in] ini_filename 全局配置文件名称
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
		\retval RET_OTHER 不完全正确
 */
sb_s32 read_adv_config (sb_s8 * ini_filename)
{

	/*!
<b>全局变量:</b> \ref g_adv_config
	 */

	/*!
<b>流程:</b>
	 */

	FILE *fp = NULL;
	sb_s8 line_buf[SB_BUF_LEN_LARGE + 200] = { 0 };
	sb_s8 *line_buf_head = NULL;
	sb_s32 ret;
	cpu_set_t cpus;
	sb_s32 dataplane_cpus_count = 0;

	memset (&g_adv_config, 0, sizeof (adv_config_t));

	//Default configure value
	g_adv_config.sig_parser_cpus = 1;
	g_adv_config.data_parser_cpus = 15;
	g_adv_config.xdr_sender_cpus = 1;

	g_adv_config.mpipe_iqueue_size = 65536;
	g_adv_config.mpipe_equeue_size = 65536;
	g_adv_config.mpipe_queue_buffer_size = 131072;

	g_adv_config.sig_xdr_queue_size = 3000;
	g_adv_config.data_xdr_queue_size = 3000;

	g_adv_config.sig_user_queue_size = 100;
	g_adv_config.sig_data_queue_size = 1000;
	g_adv_config.dpi_user_queue_size = 100;

	g_adv_config.user_timeout = 24*60*60;//1 day
	g_adv_config.xdr_timeout = 60;
	g_adv_config.xdr_event_timeslice = 600;
	g_adv_config.tcp_udp_timeout = 120;
	g_adv_config.tcp_udp_inactive_gap = 60;

	g_adv_config.attach_request_timeout = 15;
	g_adv_config.rau_request_timeout = 8;
	g_adv_config.pdp_request_timeout = 8;
	g_adv_config.detach_request_timeout = 8;
	g_adv_config.wtp_transaction_inactive_gap = 60;
	g_adv_config.wtp_inactive_gap = 10;
	//!     - 判断参数有效性
	if (ini_filename == NULL)
	{
		return RET_FAIL;
	}
	if (ini_filename[0] == '\0')
	{
		return RET_FAIL;
	}

	//! - 打开配置文件ini_filename
	fp = fopen ((char *)ini_filename, "r");
	if (fp != NULL)
	{
		while (1)
		{
			if (feof (fp))
			{
				break;
			}
			memset (line_buf, 0, sizeof (line_buf));
			if (fgets ((char *)line_buf, sizeof (line_buf) - 1, fp) == NULL)
			{
				break;
			}

			/*跳过最前面的空格、tab符号 */
			line_buf_head = line_buf;
			while ((*line_buf_head == ' ') || (*line_buf_head == '\t'))
			{
				if ((*line_buf_head == '\0') || (toascii (*line_buf_head) == 10)
						|| (toascii (*line_buf_head) == 13))
				{
					line_buf_head = NULL;	/*读到结束符、回车换行符认为是空字符串，无效字符*/
					break;
				}
				line_buf_head++;
			}
			if (line_buf_head == NULL)
			{
				continue;
			}

			chars_all_trim (line_buf_head);

			if (('\0' == *line_buf_head) || ('#' == *line_buf_head))
			{
				continue;
			}

			/*根据"全局配置文件adv.ini"配置文件读入后的结构"g_adv_config
			   的对应结构进行赋值，实现字符串的大小写无关处理*/
			ret = in_read_config_from_string (line_buf_head, &g_adv_config);
			if (RET_SUCC != ret)
			{
				fprintf (stdout, "读取配置项%s 失败\n", line_buf_head);
				continue;
			}
		}

		fclose (fp);
	}
	else
	{
		fprintf(stderr,"Open file %s失败\n",ini_filename);
		return RET_FAIL;
	}

	if (0 == g_adv_config.sig_parser_cpus)
	{
		fprintf(stderr,"No any sig_parser cpu has been set!\n");
		return RET_FAIL;
	}

	if (0 == g_adv_config.data_parser_cpus)
	{
		fprintf(stderr,"No any data_parser cpu has been set!\n");
		return RET_FAIL;
	}

	if (0 == g_adv_config.xdr_sender_cpus)
	{
		fprintf(stderr,"No any xdr_sender cpu has been set!\n");
		return RET_FAIL;
	}

	dataplane_cpus_count = tmc_cpus_count(&cpus) - g_adv_config.dataplane_cpu_start;

	if(dataplane_cpus_count < g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus + g_adv_config.xdr_sender_cpus)
	{
		fprintf(stderr,"Required cpus exceed available dataplane cpus(%d < %d)!\n",
				dataplane_cpus_count, g_adv_config.sig_parser_cpus + g_adv_config.data_parser_cpus + g_adv_config.xdr_sender_cpus);
		return RET_FAIL;
	}

	return RET_SUCC;
}

/*!
        \fn sb_s32 in_read_ip(void)
        \brief 将ini_filename中的配置内容读取到结构变量g_proto_ips
        \param[in] ini_filename 全局配置文件名称
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
		\retval RET_OTHER 不完全正确
 */
sb_s32 in_read_ip (sb_s8 * ini_filename, sb_u32 protocol_type)
{
	FILE *fp = NULL;
	sb_s8 line_buf[SB_BUF_LEN_LARGE + 200] = { 0 };
	sb_s8 *line_buf_head = NULL;
	sb_s32 ret = 0;
	sb_u32 ip = 0;
	sb_u32 link_num = 0;

//	if(g_proto_ips_num >= MAX_PRO_IP_NUM)return RET_FAIL;

	//!     - 判断参数有效性
	if (ini_filename == NULL)
	{
		return RET_FAIL;
	}
	if (ini_filename[0] == '\0')
	{
		return RET_FAIL;
	}

	//! - 打开配置文件ini_filename
	fp = fopen ((char *)ini_filename, "r");
	if (fp != NULL)
	{
		while (1)
		{
			if (feof (fp))
			{
				break;
			}
			memset (line_buf, 0, sizeof (line_buf));
			if (fgets ((char *)line_buf, sizeof (line_buf) - 1, fp) == NULL)
			{
				break;
			}

			/*跳过最前面的空格、tab符号 */
			line_buf_head = line_buf;
			while ((*line_buf_head == ' ') || (*line_buf_head == '\t'))
			{
				if ((*line_buf_head == '\0') || (toascii (*line_buf_head) == 10)
						|| (toascii (*line_buf_head) == 13))
				{
					line_buf_head = NULL;	/*读到结束符、回车换行符认为是空字符串，无效字符*/
					break;
				}
				line_buf_head++;
			}
			if (line_buf_head == NULL)
			{
				continue;
			}

			chars_all_trim (line_buf_head);

			if (('\0' == *line_buf_head) || ('#' == *line_buf_head))
			{
				continue;
			}


			ret = in_write_ip_int (line_buf_head, &ip);

			if (RET_SUCC != ret)
			{
				fprintf (stdout, "读取配置项%s 失败\n", line_buf_head);
				continue;
			}
			link_num = ip % MAX_PRO_IP_LINK_NUM;
			if(g_proto_ips_num[link_num] >= MAX_PRO_IP_NODE_NUM)continue;
			g_proto_ips[link_num][g_proto_ips_num[link_num]].ip = ip;
			g_proto_ips[link_num][g_proto_ips_num[link_num]].proto_id = protocol_type;
			g_proto_ips_num[link_num]++;
		}

		fclose (fp);
	}
	else
	{
		fprintf(stderr,"Open file %s失败\n",ini_filename);
		return RET_FAIL;
	}


	return ret;
}

/*!
        \fn sb_s32 read_ips_config(void)
        \brief 将ini_filename中的配置内容读取到结构变量g_proto_ips
        \param[in] ini_filename 全局配置文件名称
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
		\retval RET_OTHER 不完全正确
 */
sb_s32 read_ips_config (void)
{
	sb_s32 ret = 0;

	memset(&g_proto_ips_num, 0, sizeof(sb_s32) * MAX_PRO_IP_LINK_NUM);
	memset(&g_proto_ips, 0, sizeof(ip_proto_t) * MAX_PRO_IP_LINK_NUM * MAX_PRO_IP_NODE_NUM);

	in_read_ip(GB_INI_FILENAME, GB_INT);
	in_read_ip(IUPS_INI_FILENAME, IUPS_INT);
	in_read_ip(GN_INI_FILENAME, GN_INT);
	in_read_ip(S1MME_INI_FILENAME, S1MME_INT);
	in_read_ip(S1SGW_INI_FILENAME, S1U_INT);
	in_read_ip(S6A_INI_FILENAME, S6A_INT);
	in_read_ip(S10_INI_FILENAME, S10_INT);
	in_read_ip(S11_INI_FILENAME, S11_INT);
	in_read_ip(S5_INI_FILENAME, S5_INT);

	return ret;
}

sb_s32 in_read_rule(sb_s8 *line, ps_filter_rule_t *rule)
{
	sb_s32 ret = 0;
	sb_s8 *str_1 = NULL;
	sb_s8 *str_2 = NULL;
	sb_s32 i = 0, j = 0;

	sb_s8 *ptrptr = NULL;

	str_2 = (sb_s8 *) strtok_r (line, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
	while (NULL != str_2)
	{
		chars_all_trim (str_2);

		switch (i)
		{
		case 0:
			ret = in_write_unsigned_integer (str_2,&rule->id);

			break;
		case 1:
			ret = in_write_ip_int (str_2,&rule->user_ip);

			break;
		case 2:
			ret = in_write_unsigned_long (str_2,&rule->imsi);

			break;
		case 3:
			ret = in_write_unsigned_long (str_2,&rule->imei);

			break;
		case 4:
			ret = in_write_unsigned_long (str_2,&rule->msisdn);

			break;
		case 5:
			ret = in_write_ip_int (str_2,&rule->rn_ip1);

			break;
		case 6:
			ret = in_write_ip_int (str_2,&rule->rn_ip2);

			break;
		case 7:
			ret = in_write_unsigned_integer (str_2,&rule->cid_type);

			break;
		case 8:
			str_1 = (sb_s8 *) strtok_r (str_2, CI_STR_SEPARATOR, (sb_s8 **)&ptrptr);
			while (NULL != str_1)
			{
				chars_all_trim (str_1);

				switch(j)
				{
				case 0:
					ret = in_write_unsigned_short (str_1,&rule->mcc);
					break;
				case 1:
					ret = in_write_unsigned_short (str_1,&rule->mnc);
					break;
				case 2:
					if(rule->cid_type == 2)
						ret = in_write_unsigned_integer (str_1,&rule->ci);
					else
						ret = in_write_unsigned_short (str_1,&rule->lac);
					break;
				case 3:
					if(rule->cid_type == 1)
						ret = in_write_unsigned_integer (str_1,&rule->ci);
					else if(rule->cid_type == 0)
						ret = in_write_unsigned_short (str_1,&rule->rac);

					break;
				case 4:
					if(rule->cid_type == 0)
						ret = in_write_unsigned_integer (str_1,&rule->ci);
					break;
				default:
					break;
				}
				str_1 = (sb_s8 *) strtok_r (NULL, CI_STR_SEPARATOR, (sb_s8 **)&ptrptr);
				j++;
				if(j >= 5)break;
			}

			break;
		default:
			break;
		}

		str_2 = (sb_s8 *) strtok_r (NULL, SERVER_STR_SEPARATOR, (sb_s8 **)&ptrptr);
		i++;
		if(i >= 9)break;
	}

	if(i < 9) return RET_FAIL;

	return ret;
}
/**********************************************************************
函数说明：将ini_filename中的配置内容读取到结构变量g_ps_filter
输入参数：sb_u8 * ini_filename：系统配置文件名

输出参数：无

全局变量：ps_filter_rule_t g_ps_filter

函数返回：RET_SUCC：成功
          RET_FAIL：失败
		  RET_OTHER：不完全正确

修改记录：无
备         注：
 **********************************************************************/

/*!
        \fn sb_s32 read_filter_config(sb_u8 * device_ini_filename)
        \brief 将ini_filename中的配置内容读取到结构变量g_ps_filter
        \param[in] ini_filename 全局配置文件名称
        \param[out] 无
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
		\retval RET_OTHER 不完全正确
 */
sb_s32 read_filter_config (sb_s8 * ini_filename)
{
	FILE *fp = NULL;
	sb_s8 line_buf[SB_BUF_LEN_LARGE + 200] = { 0 };
	sb_s8 *line_buf_head = NULL;
	sb_s32 ret;

	memset (&g_ps_filter, 0, sizeof (ps_filter_rule_t));

	//!     - 判断参数有效性
	if (ini_filename == NULL)
	{
		return RET_FAIL;
	}
	if (ini_filename[0] == '\0')
	{
		return RET_FAIL;
	}

	//! - 打开配置文件ini_filename
	fp = fopen ((char *)ini_filename, "r");
	if (fp != NULL)
	{
		while (1)
		{
			if (feof (fp))
			{
				break;
			}
			memset (line_buf, 0, sizeof (line_buf));
			if (fgets ((char *)line_buf, sizeof (line_buf) - 1, fp) == NULL)
			{
				break;
			}

			/*跳过最前面的空格、tab符号 */
			line_buf_head = line_buf;
			while ((*line_buf_head == ' ') || (*line_buf_head == '\t'))
			{
				if ((*line_buf_head == '\0') || (toascii (*line_buf_head) == 10)
						|| (toascii (*line_buf_head) == 13))
				{
					line_buf_head = NULL;	/*读到结束符、回车换行符认为是空字符串，无效字符*/
					break;
				}
				line_buf_head++;
			}
			if (line_buf_head == NULL)
			{
				continue;
			}

			chars_all_trim (line_buf_head);

			if (('\0' == *line_buf_head) || ('#' == *line_buf_head))
			{
				continue;
			}

			/*根据"全局配置文件ps_filter.ini"配置文件读入后的结构"g_ps_filter
			   的对应结构进行赋值，实现字符串的大小写无关处理*/
			ret = in_read_rule (line_buf_head, &g_ps_filter[g_ps_filter_num]);
			if (RET_SUCC != ret)
			{
				fprintf (stdout, "读取配置项%s 失败\n", line_buf_head);
				continue;
			}
			g_ps_filter_num ++;
			if(g_ps_filter_num >= MAX_PS_RULE_NUM)break;
		}

		fclose (fp);
	}
	else
	{
		fprintf(stderr,"Open file %s失败\n",ini_filename);
		return RET_FAIL;
	}


	return RET_SUCC;
}
