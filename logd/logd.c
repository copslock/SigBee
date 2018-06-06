/*
 * logd.c
 *
 *  Created on: 2014年5月27日
 *      Author: Figoo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>

#include "sb_type.h"
#include "sb_logd.h"
#include "sb_defines.h"

#define MAXLOGSIZE 5000000

sb_s8 logfilename1[] = PROCESS_WORKING_DIR PROCESS_LOG_DIR"sys1.log";
sb_s8 logfilename2[] = PROCESS_WORKING_DIR PROCESS_LOG_DIR"sys2.log";
sb_s8 logfilename3[] = PROCESS_WORKING_DIR PROCESS_LOG_DIR"alert1.log";
sb_s8 logfilename4[] = PROCESS_WORKING_DIR PROCESS_LOG_DIR"alert2.log";
sb_s8 logfilename5[] = PROCESS_WORKING_DIR PROCESS_LOG_DIR"error1.log";
sb_s8 logfilename6[] = PROCESS_WORKING_DIR PROCESS_LOG_DIR"error2.log";

void sb_open_and_sizecheck(log_msg_t *object, const sb_s8 *oldname, const sb_s8 *newname)
{

	sb_s8 datestr[16];
	sb_s8 timestr[16];
	struct tm *now;
	object->time = time(NULL);
	now = localtime(&object->time);
	sprintf(datestr,"%04d-%02d-%02d",now->tm_year+1900,now->tm_mon+1,now->tm_mday);
	sprintf(timestr,"%02d:%02d:%02d",now->tm_hour,now->tm_min,now->tm_sec);

	FILE *flog1;
	flog1 = fopen(oldname,"a+");
	if(NULL != flog1)
	{
		fprintf(flog1,"%s %s processname:%s cpuid:%02d content:%s\n",datestr,timestr,object->pname,object->cpuid,object->content);
		if(ftell(flog1)>MAXLOGSIZE)
		{
			fclose(flog1);
			rename(oldname,newname);
		}
		else{
			fclose(flog1);
		}

	}
	else{
		printf("open file error!\n");
	}
}

void sb_write_log()
{
	sb_s32 result;
	log_msg_t object;

	while(1)
	{
		if(-1 != (result=log_queue_dequeue(g_log_queue,&object)))
		{
			if(object.msg_type == SB_LOG_INFO)
			{
				sb_open_and_sizecheck(&object, logfilename1, logfilename2);
			}else
			{
				if(object.msg_type == SB_LOG_ALERT)
				{
					sb_open_and_sizecheck(&object, logfilename3, logfilename4);
				}
				else
				{
					if(object.msg_type == SB_LOG_ERR)
					{
						sb_open_and_sizecheck(&object, logfilename5, logfilename6);
					}
				}
			}
		}
		else
		{
			//             printf("log_queue_dequeue is %d!\n",result);
			usleep(1000);
		}
	}
}


