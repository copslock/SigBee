/*
 * sb_base_util.h
 *
 *  Created on: 2014年6月3日
 *      Author: Figoo
 */

#ifndef SB_BASE_UTIL_H_
#define SB_BASE_UTIL_H_

sb_u64 bcd_to_int(sb_u8 *bcd, sb_u32 len, sb_u8 odd);

void chars_left_trim (sb_s8 * str);
void chars_right_trim (sb_s8 * str);
void chars_all_trim (sb_s8 * str);

sb_s32 in_write_ip_int (sb_s8 * ip_str, sb_u32 * ip);
sb_s32 in_write_string (sb_s8 * src_str, sb_s8 * dest_str, size_t size);
sb_s32 in_write_integer (sb_s8 * int_str, sb_u32 * p_value);
sb_s32 in_write_signed_integer (sb_s8 * int_str, sb_s32 * p_value);
sb_s32 in_write_unsigned_integer (sb_s8 * int_str, sb_u32 * p_value);
sb_s32 in_write_unsigned_short (sb_s8 * int_str, sb_u16 * p_value);
sb_s32 in_write_unsigned_long (sb_s8 * int_str, sb_u64 * p_value);
sb_s32 in_write_hex_unsigned_integer (sb_s8 * int_str, sb_u32 * p_value);
sb_u64  ntohl64(sb_u64   host);
sb_u64  hl64ton(sb_u64   host);
sb_u32 time_minus(union sb_time_t new_time, union sb_time_t old_time);
sb_u32 time_minus_split(union sb_time_t new_time, sb_u32 old_time_s, sb_u32 old_time_ns);

sb_u32 hash_value(sb_u32 value);
sb_u32 hash_value_dual(sb_u32 value1, sb_u32 value2);

#endif /* SB_BASE_UTIL_H_ */
