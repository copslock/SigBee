/*
 * base_lib_mem.c
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <iconv.h>

#include <tmc/mspace.h>
#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_base_file.h"
#include "sb_base_mem.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_log.h"

#define ALIGN(p, align) do { (p) += -(unsigned long long)(p) & ((align) - 1); } while(0)
#define MIN_FAST_LEN 64

sb_s32 g_sys_abnormal;
pthread_mutex_t log_mutex;

sb_s8 *mygettime (void)
{
  time_t t;
  struct tm *newtime;
  static sb_s8 tm0[48] = { 0 };

  time (&t);
  newtime = localtime (&t);

  sprintf (tm0, "[%04d-%02d-%02d %02u:%02d:%02d] ", newtime->tm_year + 1900,
      newtime->tm_mon, newtime->tm_mday, newtime->tm_hour,
      newtime->tm_min, newtime->tm_sec);

  return tm0;
}

void mem_log (sb_s8 *fmt, ...)
{
  FILE *logfp = NULL;
  va_list ap;
  sb_s8 logfile[1024] = { 0, };
  sb_s32 lFileLen = 0;

  sprintf (logfile, "log/memwatch.log");

  /* open logfile */
  pthread_mutex_lock (&log_mutex);
  logfp = fopen (logfile, "a+t");
  if (logfp == NULL)
    {
      pthread_mutex_unlock (&log_mutex);
      return;
    }
  //处理文件大小上限
  lFileLen = ftell (logfp);
  if (lFileLen == -1)
    {
      fclose (logfp);
      pthread_mutex_unlock (&log_mutex);
      return;
    }
  else
    {
      if (lFileLen > LOG_MAX_SIZE)
        {
          fclose (logfp);
          logfp = NULL;
          unlink (logfile);
          logfp = fopen (logfile, "a+t");
          if (logfp == NULL)
            {
              pthread_mutex_unlock (&log_mutex);
              return;
            }
        }
    }

  va_start (ap, fmt);
  fprintf (logfp, "%s ", mygettime ());
  vfprintf (logfp, fmt, ap);
  fprintf (logfp, "\n");
  va_end (ap);
  fclose (logfp);
  pthread_mutex_unlock (&log_mutex);

  return;
}

// safe malloc based on input length
void * check_malloc (sb_s8 *func, size_t num, size_t size)
{
  void *ptr = NULL;

  // 如果内存分配失败，则退出系统
  if (num * size <= 0 || (ptr = calloc (num, size)) == NULL)
    {
      mem_log ("Can't malloc in %s, num: %u, size: %u", func, num, size);
      // 加此判断是为了避免在write_log中调用check_malloc出错而陷入递归循环中
      if (strcmp (func, "in_send_alert") != 0 && strcmp (func, "write_log") != 0)
        {
          write_log (0, 0, (sb_s8 *) "Can't malloc in %s, num: %u, size: %u",
              func, num, size);
        }
      g_sys_abnormal = TRUE;
      pthread_exit (NULL);
    }
  memset (ptr, 0, num * size);

  return ptr;
}

void check_free (void **buffer)
{
  if(NULL != *buffer)
	free (*buffer);

	*buffer = NULL;

	return;
}

// safe malloc based on input length
void * check_malloc_msp (tmc_mspace msp, sb_s8 *func, size_t num, size_t size)
{
  void *ptr = NULL;

  // 如果内存分配失败，则退出系统
  if (num * size <= 0 || (ptr = tmc_mspace_calloc (msp, num, size)) == NULL)
    {
      mem_log ("Can't malloc in %s, num: %u, size: %u", func, num, size);
      // 加此判断是为了避免在write_log中调用check_malloc出错而陷入递归循环中
      if (strcmp (func, "in_send_alert") != 0 && strcmp (func, "write_log") != 0)
        {
          write_log (0, 0, (sb_s8 *) "Can't malloc in %s, num: %u, size: %u",
              func, num, size);
        }
      g_sys_abnormal = TRUE;
      pthread_exit (NULL);
    }
//  g_lp_ma_stat.malloc_count++;
  memset (ptr, 0, num * size);

  return ptr;
}

void check_free_msp (void **buffer)
{
  if(NULL != *buffer)
  {
    tmc_mspace_free (*buffer);
//    g_lp_ma_stat.free_count++;
  }

        *buffer = NULL;

        return;
}

/* 注意dest 和 src 指针必须与sizeof(void *)对齐或者与 sizeof(void *)的对齐 偏移量相同，不然没有任何优化，直接返回memcpy */
void *fast_memcpy(void *dest,const void *src,size_t n)
{
  if(( (unsigned long long)dest &  (sizeof(void *)-1)) != ((unsigned long long)src &  (sizeof(void *)- 1)) ) //偏移不同，没有任何意义
    {
      return memcpy(dest,src,n);
    }
  if(n<= MIN_FAST_LEN) //sizeof(void *))
    {
      return memcpy(dest,src,n);
    }
  else{
      /* 先地址对齐 */
      void * ret = dest;
      void * tmp_src = (void *)src;
      ALIGN(dest,sizeof(void *));               //8字节对齐
      if(ret != dest)
        {
          /* 这里memcpy 性能仍然很低 ，尝试自己实现 */
          switch( (unsigned long long )dest - (unsigned long long)ret)
          {
          case 7:
            {
              *((unsigned char *)ret) = *((unsigned char *)src);
              *((unsigned short *)(ret+1)) = *((unsigned short *)(src+1));
              *((unsigned int *)(ret+3)) = *((unsigned int *)(src+3));
            }
            break;
          case 6:
            {
              *((unsigned short *)ret) = *((unsigned short *)src);
              *((unsigned int *)(ret+2)) = *((unsigned int *)(src+2));

            }
            break;
          case 5:
            {
              *((unsigned char *)ret) = *((unsigned char *)src);
              *((unsigned int *)(ret+1)) = *((unsigned int *)(src+1));

            }
            break;
          case 4:
            {
              *((unsigned int *)ret) = *((unsigned int *)src);

            }
            break;
          case 3:
            {
              *((unsigned char *)ret) = *((unsigned char *)src);
              *((unsigned short *)(ret+1)) = *((unsigned short *)(src+1));


            }
            break;
          case 2:
            {
              *((unsigned short *)ret) = *((unsigned short *)src);


            }
            break;
          case 1:
            {
              *((unsigned char *)ret) = *((unsigned char *)src);


            }
            break;
          default:
            {

            }
            break;

          }
          ALIGN(tmp_src, sizeof(void *));
        }

      while( n >= (unsigned long long)dest - (unsigned long long)ret + sizeof(void *))
        {
          *((unsigned long long *)dest) =  *((unsigned long long *)tmp_src);
          dest+= sizeof(void *);
          tmp_src+= sizeof(void *);
        }

      if(n > ((unsigned long long)dest - (unsigned long long)ret) )
        {

          switch(n-( (unsigned long long )dest - (unsigned long long)ret))
          {
          case 7:
            {
              *((unsigned int *)dest) = *((unsigned int *)tmp_src);
              *((unsigned short *)(dest+4)) = *((unsigned short *)(tmp_src+4));
              *((unsigned char *)(dest+6)) = *((unsigned char *)(tmp_src+6));
            }
            break;
          case 6:
            {
              *((unsigned int *)dest) = *((unsigned int *)tmp_src);
              *((unsigned short *)(dest+4)) = *((unsigned short *)(tmp_src+4));

            }
            break;
          case 5:
            {

              //	start = get_cycle_count();
              *((unsigned int *)dest) = *((unsigned int  *)tmp_src);
              *((unsigned char *)(dest+4)) = *((unsigned char *)(tmp_src+4));
              //	end = get_cycle_count();
              //	printf("tail_time %lu aglin %lu  \n",end-start,n-( (unsigned long long )dest - (unsigned long long)ret));


            }
            break;
          case 4:
            {
              *((unsigned int *)dest) = *((unsigned int *)tmp_src);

            }
            break;
          case 3:
            {
              *((unsigned short *)dest) = *((unsigned short *)tmp_src);
              *((unsigned char *)(dest+2)) = *((unsigned char *)(tmp_src+2));


            }
            break;
          case 2:
            {
              *((unsigned short *)dest) = *((unsigned short *)tmp_src);


            }
            break;
          case 1:
            {
              *((unsigned char *)dest) = *((unsigned char *)tmp_src);


            }

            break;
          default:
            break;
          }
        }
      return ret;
  }
}

void string_cpy_y(sb_u8 *dst, sb_u8 *src, sb_u16 len)
{
	sb_u8 is_special = 0;
	sb_u16 dst_len = 0;

  if((dst == NULL) || (src == NULL))
  {
    printf("ERROR in string_cpy\n");
    return;
  }

  while(*src != '\0')
  {
    if((*src != ',') && (*src != '\"') && (*src != ';') && (*src != '\n') && (*src != '\r'))     // "\r\n"
    {
      *dst=*src;
      if(is_special == 0 && (*dst & 0x80) == 0x80)is_special = 1;//不可打印字符
      dst++;
      dst_len++;
    }
    src++;
  }

  if(is_special == 1)
    {
      if(dst_len < len - 1 )
        {
          *dst = 0x20;//加空格
          *++dst = '\0';
        }
      else
        {
          *--dst = 0x20;
          *++dst = '\0';
        }
    }
  else
    {
      *dst = '\0';
    }

  return;
}

void string_cpy(sb_u8 *dst, sb_u8 *src, sb_u16 len)
{
//        sb_u8 is_special = 0;
        sb_u16 dst_len = 0;

  if((dst == NULL) || (src == NULL))
  {
    printf("ERROR in string_cpy\n");
    return;
  }

  while(*src != '\0')
  {
    if(*src < 0x21 || *src > 0x7E)
      {
        *dst = '.';
      }
    else if((*src == ',') || (*src == '\"') || (*src == ';') || (*src == '\n') ||(*src == '\r'))     // "\r\n"
    {
        *dst = '.';
    }
    else
      {
        *dst=*src;
      }
    dst++;
    dst_len++;
    src++;
    if(dst_len >= (len - 1))break;
  }

        *dst = '\0';


  return;
}

