/*
 * base_lib_util.c
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

/* 引用标准库的头文件*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <math.h>

//tilera 相关头文件
#include <tmc/cmem.h>
#include <tmc/mem.h>
#include <tmc/cpus.h>
#include <gxio/mpipe.h>
#include <gxcr/gxcr.h>
#include <gxio/mica.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_error_code.h"
#include "sb_base_util.h"
#include "sb_base_mem.h"
#include "sb_public_var.h"

sb_u32 hash_value(sb_u32 value)
{
	sb_u32 initval = DEFAULT_HASH_KEY;
	return __insn_crc32_32(value, initval);
}

sb_u32 hash_value_dual(sb_u32 value1, sb_u32 value2)
{
	return __insn_crc32_32(value1, value2);
}

//BCD转十进制
sb_u64 bcd_to_int(sb_u8 *bcd, sb_u32 len, sb_u8 odd)
{
	sb_u32 i;
	sb_u64 ret = 0;

	if(len > 10) return 0;

	if(odd == 1)
	{
		ret = ((bcd[0]&0xF0) >> 4) * pow(10, len*2 - 2);
		for(i = 1; i < len; i++)
		{
			ret += (bcd[i]&0x0F) * pow(10, (len-i)*2 - 1) + ((bcd[i]&0xF0) >> 4) * pow(10, (len-i)*2 - 2);
		}
	}
	else
	{
		ret = ((bcd[0]&0xF0) >> 4) * pow(10, (len - 1)*2 - 1);
		for(i = 1; i < len - 1; i++)
		{
			ret += (bcd[i]&0x0F) * pow(10, (len-i - 1)*2) + ((bcd[i]&0xF0) >> 4) * pow(10, (len-i - 1)*2 - 1);
		}
		ret += (bcd[len - 1]&0x0F);
	}

      return ret;
}


/**********************************************************************
函数说明：去除首部空格及tab符及回车换行
输入参数：sb_u8 *str: 需要去除首部空格的源字符串

输出参数：sb_u8 *str：指针指定地址的字符串被处理，但是指针变量本身不作返回参数

全局变量：无
函数返回：无

修改记录：无
备   注：
**********************************************************************/

/*!
        \fn void chars_left_trim(sb_u8 *str)
        \brief 去除首部空格及tab符及回车换行
        \param[in] str 需要去除首部空格的源字符串
        \param[out] str 指针指定地址的字符串被处理，但是指针变量本身不作返回参数
        \return void
*/
void chars_left_trim (sb_s8 * str)
{

/*!
<b>全局变量:</b>
*/

/*!
<b>流程:</b>
*/
	sb_s8 *tmp;
	sb_s8 *p;
	sb_u32 len;

	//!     - 判断参数有效
	if (str == NULL)
	{
		return;
	}

	//! - 获取源字符串str的长度
	len = strlen ((char *)str);
	if (len == 0)
	{
		return;
	}

	//!     - 按取得长度加1为字符串tmp分配内存给p，清
	tmp = (sb_s8 *) (check_malloc ("chars_left_trim", 1, len + 1));
	if (tmp == NULL)
	{
		return;
	}
	memset (tmp, 0, len + 1);

	//!     - 拷贝str到tmp，str清零
	strcpy ((char *)tmp, (char *)str);
	memset (str, 0, len);

	//!     - 字符指针p从tmp头循环判断字符，指针跳过前导空格及tab
	p = tmp;
	while ((*p != '\0') && ((*p == ' ') || (*p == '\t') ||
							(toascii (*p) == 10) || (toascii (*p) == 13)))
	{
		p++;
	}

	//!     - 拷贝p到str
	strcpy ((char *)str, (char *)p);

	//!     - 释放p分配内存
	check_free ((void **)&tmp);
	tmp = NULL;
}

/**********************************************************************
函数说明：去除尾部空格及tab符及回车换行
输入参数：sb_u8 *str: 需要去除尾部空格的源字符串

输出参数：sb_u8 *str：指针指定地址的字符串被处理，但是指针变量本身不作返回参数

全局变量：无
函数返回：无

修改记录：无
备         注：
**********************************************************************/

/*!
        \fn void chars_right_trim(sb_u8 *str)
        \brief 去除尾部空格及tab符及回车换行
        \param[in] str 需要去除尾部空格的源字符串
        \param[out] str 指针指定地址的字符串被处理，但是指针变量本身不作返回参数
        \return void
*/
void chars_right_trim (sb_s8 * str)
{

/*!
<b>全局变量:</b>
*/

/*!
<b>流程:</b>
*/
	sb_s8 *p;
	sb_u32 len;
	sb_s32 i;

	//!     - 判断参数有效
	if (str == NULL)
	{
		return;
	}

	//! - 获取源字符串str的长
	len = strlen ((char *)str);
	if (len == 0)
	{
		return;
	}

	//!     - 按取得长度加1为字符串tmp分配内存给p，清
	p = (sb_s8 *) (check_malloc ("chars_right_trim", 1, len + 1));
	if (p == NULL)
	{
		return;
	}
	memset (p, 0, len + 1);

	//!     - 拷贝str到tmp，str清零
	strcpy ((char *)p, (char *)str);
	memset (str, 0, len);

	//!     - 字符指针p从tmp尾循环判断字符，尾部空格及tab
	for (i = len - 1; i >= 0; i--)
	{
		if ((*(p + i) == '\0') || ((*(p + i) != ' ') && (*(p + i) != '\t') &&
								   (toascii (*(p + i)) != 10)
								   && (toascii (*(p + i)) != 13)))
		{
			break;
		}

		*(p + i) = '\0';
	}

	//!     - 拷贝p到str
	strcpy ((char *)str, (char *)p);

	//!     - 释放p分配内存
	check_free ((void **)&p);
	p = NULL;
}

/**********************************************************************
函数说明：去除首尾空格及tab
输入参数：sb_u8 *str: 需要去除首尾空格的源字符串

输出参数：sb_u8 *str：指针指定地址的字符串被处理，但是指针变量本身不作返回参数

全局变量：无
函数返回：无

修改记录：无
备          注：
**********************************************************************/

/*!
        \fn void chars_all_trim(sb_u8 *str)
        \brief 去除首尾空格及tab
        \param[in] str 需要去除首尾空格的源字符串
        \param[out] str 指针指定地址的字符串被处理，但是指针变量本身不作返回参数
        \return void
*/
void chars_all_trim (sb_s8 * str)
{

/*!
<b>全局变量:</b>
*/

/*!
<b>流程:</b>
*/
	//!     - 执行\ref chars_left_trim 去除str首部空格及tab
	chars_left_trim (str);
	//!     - 执行\ref chars_left_trim 去除str尾部空格及tab
	chars_right_trim (str);
}

/**********************************************************************
函数说明：将分离出来的IP字符串转换成整数后写入指定指针所指内容

输入参数：sb_u8 *ip_str：ip字符串
          sb_s32 *ip：存储IP的整型指针

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
**********************************************************************/

/*!
        \fn sb_s32 in_write_ip_int(sb_u8 *ip_str, sb_u32 *ip)
        \brief 将分离出来的IP字符串转换成整数后写入指定指针所指内容
        \param[in] ip_str ip字符串
        \param[out] ip 存储IP的整型指针
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
*/
sb_s32 in_write_ip_int (sb_s8 * ip_str, sb_u32 * ip)
{

/*!
<b>全局变量:</b>
*/

/*!
<b>流程:</b>
*/
	sb_s32 ret;
	sb_u32 ip_addr = 0;

	//!     - 判断参数有效性
	if ((NULL == ip_str) || (ip == NULL))
	{
		return RET_FAIL;
	}
	if (('\0' == ip_str[0]))
	{
		return RET_FAIL;
	}

	//!     - 将ip_str字符串的点分四段IP地址转为整数，并完成网络字节序到主机字节序的转换
	ret = inet_pton (AF_INET, (char *)ip_str, &ip_addr);
	if (1 != ret)
	{
		return RET_FAIL;
	}
	ip_addr = ntohl (ip_addr);

	//!     -前面得到的值赋值给*ip
	*ip = ip_addr;

	return RET_SUCC;
}

/**********************************************************************
函数说明：将分离出来的字符串写入到指定字符串指针指向地址

输入参数：sb_u8 *src_str：源字符串
          sb_u8 *dest_str：目的字符串
          size_t size：目的串大小

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备        注：
**********************************************************************/

/*!
        \fn sb_s32 in_write_string(sb_u8 *src_str, sb_u8 *dest_str, size_t size)
        \brief 将分离出来的字符串写入到指定字符串指针指向地址
        \param[in] src_str 源字符串
        \param[in] size 目的串大小
        \param[out] dest_str 目的字符串
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
*/
sb_s32 in_write_string (sb_s8 * src_str, sb_s8 * dest_str, size_t size)
{

/*!
<b>全局变量:</b>
*/
	if ((NULL == src_str) || (NULL == dest_str))
	{
		return RET_FAIL;
	}
	if (('\0' == src_str[0]))
	{
		return RET_FAIL;
	}

	strncpy ((char *)dest_str, (char *)src_str, size);

	return RET_SUCC;
}

/**********************************************************************
函数说明：将分离出来的字符串写入到指定整型（sb_u32）指针指向地址

输入参数：sb_u8 *int_str：整型字符串
          sb_u32 *p_value：指向目的地址的整型指针

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
**********************************************************************/

/*!
        \fn sb_s32 in_write_integer(sb_u8 *int_str, sb_u32 *p_value)
        \brief 将分离出来的字符串写入到指定无符号整型（sb_u32）指针指向地址
        \param[in] int_str 整型字符串
        \param[out] p_value 指向目的地址的无符号整型指针
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
*/
sb_s32 in_write_integer (sb_s8 * int_str, sb_u32 * p_value)
{

/*!
<b>全局变量:</b>
*/
	sb_u32 value;

	if ((NULL == int_str) || (NULL == p_value))
	{
		return RET_FAIL;
	}
	if (('\0' == int_str[0]))
	{
		return RET_FAIL;
	}

	value = atoi ((char *)int_str);

	*p_value = value;

	return RET_SUCC;
}

/**********************************************************************
函数说明：将分离出来的字符串写入到指定整型（sb_s32）指针指向地址

输入参数：sb_u8 *int_str：整型字符串
          sb_s32 *p_value：指向目的地址的有符号整型指针

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
**********************************************************************/

/*!
        \fn sb_s32 in_write_signed_integer(sb_u8 *int_str, sb_s32 *p_value)
        \brief 将分离出来的字符串写入到指定整型（sb_s32）指针指向地址
        \param[in] int_str 整型字符串
        \param[out] p_value 指向目的地址的有符号整型指针
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
*/
sb_s32 in_write_signed_integer (sb_s8 * int_str, sb_s32 * p_value)
{

/*!
<b>全局变量:</b>
*/
	sb_s32 value;

	if ((NULL == int_str) || (NULL == p_value))
	{
		return RET_FAIL;
	}
	if (('\0' == int_str[0]))
	{
		return RET_FAIL;
	}

	value = atoi ((char *)int_str);

	*p_value = value;

	return RET_SUCC;
}

sb_s32 in_write_unsigned_integer (sb_s8 * int_str, sb_u32 * p_value)
{

/*!
<b>全局变量:</b>
*/
	sb_s32 value;

	if ((NULL == int_str) || (NULL == p_value))
	{
		return RET_FAIL;
	}
	if (('\0' == int_str[0]))
	{
		return RET_FAIL;
	}

	value = atoi ((char *)int_str);

	*p_value = value;

	return RET_SUCC;
}

sb_s32 in_write_unsigned_short (sb_s8 * int_str, sb_u16 * p_value)
{

/*!
<b>全局变量:</b>
*/
	sb_s32 value;

	if ((NULL == int_str) || (NULL == p_value))
	{
		return RET_FAIL;
	}
	if (('\0' == int_str[0]))
	{
		return RET_FAIL;
	}

	value = atoi ((char *)int_str);

	*p_value = value;

	return RET_SUCC;
}

/**********************************************************************
函数说明：将分离出来的10进制字符串写入到指定整型（sb_u64）指针指向地址

输入参数：sb_u8 *int_str：整型字符串
          sb_u64 *p_value：指向目的地址的有符号整型指针

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
**********************************************************************/

/*!
        \fn sb_s32 in_write_unsigned_long(sb_u8 *int_str, sb_u64 *p_value)
        \brief 将分离出来的10进制字符串写入到指定整型（sb_u64）指针指向地址
        \param[in] int_str 整型字符串
        \param[out] p_value 指向目的地址的有符号整型指针
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
*/
sb_s32 in_write_unsigned_long (sb_s8 * int_str, sb_u64 * p_value)
{

/*!
<b>全局变量:</b>
*/
	sb_u64 value;

	if ((NULL == int_str) || (NULL == p_value))
	{
		return RET_FAIL;
	}
	if (('\0' == int_str[0]))
	{
		return RET_FAIL;
	}

	value = strtoul((char *)int_str,NULL,10);

	*p_value = value;

	return RET_SUCC;
}

/**********************************************************************
函数说明：将分离出来的16进制字符串写入到指定整型（sb_u32）指针指向地址

输入参数：sb_u8 *int_str：整型字符串
          sb_u32 *p_value：指向目的地址的有符号整型指针

输出参数：无

全局变量：无

函数返回：RET_SUCC：成功
          RET_FAIL：失败

修改记录：无
备         注：
**********************************************************************/

/*!
        \fn sb_s32 in_write_hex_unsigned_integer(sb_u8 *int_str, sb_u32 *p_value)
        \brief 将分离出来的16进制字符串写入到指定整型（sb_u32）指针指向地址
        \param[in] int_str 整型字符串
        \param[out] p_value 指向目的地址的有符号整型指针
        \return sb_s32
		\retval RET_SUCC 成功
		\retval RET_FAIL 失败
*/
sb_s32 in_write_hex_unsigned_integer (sb_s8 * int_str, sb_u32 * p_value)
{

/*!
<b>全局变量:</b>
*/
	sb_u32 value;

	if ((NULL == int_str) || (NULL == p_value))
	{
		return RET_FAIL;
	}
	if (('\0' == int_str[0]))
	{
		return RET_FAIL;
	}

	value = strtoul((char *)int_str,NULL,16);

	*p_value = value;

	return RET_SUCC;
}

// host long 64 to network

sb_u64  hl64ton(sb_u64   host)
{
	sb_u64   ret = 0;
	sb_u32   high,low;

	low   =   host & 0xFFFFFFFF;
	high   =  (host >> 32) & 0xFFFFFFFF;
	low   =   htonl(low);
	high   =   htonl(high);

	ret   =   low;
	ret   <<= 32;
	ret   |=   high;

	return   ret;
}

//network to host long 64

sb_u64  ntohl64(sb_u64   host)
{
	sb_u64   ret = 0;
	sb_u32   high,low;

	low   =   host & 0xFFFFFFFF;
	high   =  (host >> 32) & 0xFFFFFFFF;
	low   =   ntohl(low);
	high   =   ntohl(high);

	ret   =   low;
	ret   <<= 32;
	ret   |=   high;

	return   ret;
}

//Compute Time delta(ms)
sb_u32 time_minus(union sb_time_t new_time, union sb_time_t old_time)
{
  sb_u32 delta = 0;

  if(new_time.capture_time_detail.time_stamp_sec > old_time.capture_time_detail.time_stamp_sec)
    {
            delta = (new_time.capture_time_detail.time_stamp_sec - old_time.capture_time_detail.time_stamp_sec - 1)*1000;
            delta += (1000 - old_time.capture_time_detail.time_stamp_ns/1000000) + new_time.capture_time_detail.time_stamp_ns/1000000;

    }
    else if(new_time.capture_time_detail.time_stamp_sec == old_time.capture_time_detail.time_stamp_sec)
    {
            delta = (new_time.capture_time_detail.time_stamp_ns - old_time.capture_time_detail.time_stamp_ns)/1000000;
    }


  return delta;
}

//Compute Time delta(ms)
sb_u32 time_minus_split(union sb_time_t new_time, sb_u32 old_time_s, sb_u32 old_time_ns)
{
  sb_u32 delta = 0;

  if(new_time.capture_time_detail.time_stamp_sec > old_time_s)
    {
            delta = (new_time.capture_time_detail.time_stamp_sec - old_time_s - 1)*1000;
            delta += (1000 - old_time_ns/1000000) + new_time.capture_time_detail.time_stamp_ns/1000000;

    }
    else if(new_time.capture_time_detail.time_stamp_sec == old_time_s)
    {
            delta = (new_time.capture_time_detail.time_stamp_ns - old_time_ns)/1000000;
    }


  return delta;
}

