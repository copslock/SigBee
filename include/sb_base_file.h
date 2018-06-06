/*
 * sb_base_file.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_BASE_FILE_H_
#define SB_BASE_FILE_H_

//为了去除icc编译过程中得warning
#define strlen_for_icc(str)                     strlen((sb_s8*)(str))
#define strdup_for_icc(str)                     (sb_s8 *)strdup((sb_s8*)(str))
#define strstr_for_icc(str, sub_str)            (sb_s8 *)strstr((sb_s8*)(str), (sb_s8*)(sub_str))
#define strchr_for_icc(str, sub_str)            (sb_s8 *)strchr((sb_s8*)(str), (sub_str))
#define strrchr_for_icc(str, sub_str)	(sb_s8 *)strrchr((sb_s8*)(str), (sub_str))

#define strncmp_for_icc(str_1, sub_2, len)      strncmp((sb_s8*)(str_1), (sb_s8*)(sub_2), (len))
#define strncasecmp_for_icc(str_1, sub_2, len)      strncasecmp((sb_s8*)(str_1), (sb_s8*)(sub_2), (len))
#define strncpy_for_icc(str_1, sub_2, len)      strncpy((sb_s8*)(str_1), (sb_s8*)(sub_2), (len))
#define strcpy_for_icc(str_1, sub_2)            strcpy((sb_s8*)(str_1), (sb_s8*)(sub_2))
#define strcmp_for_icc(str_1, sub_2)            strcmp((sb_s8*)(str_1), (sb_s8*)(sub_2))
#define strcat_for_icc(str_1, sub_2)            strcat((sb_s8*)(str_1), (sb_s8*)(sub_2))
#define strcasecmp_for_icc(str_1, sub_2)            strcasecmp((sb_s8*)(str_1), (sb_s8*)(sub_2))
#define get_line_for_icc(str_1, str1_len, p_buf, p_buf_len)            (sb_s8*)get_line((sb_s8*)(str_1), (str1_len), (sb_s8*)(p_buf), (sb_s32*)(p_buf_len))
#define fgets_for_icc(buf, buf_len, fp)            fgets((sb_s8*)buf, buf_len, fp)
#define strtok_for_icc(buf, sub)	(sb_s8*)strtok((sb_s8 *)buf,(sb_s8 *)sub)

sb_s32 create_output_dir (void);

#endif /* SB_BASE_FILE_H_ */
