/*
 * sig_gb_llc.h
 *
 *  Created on: 2014年6月18日
 *      Author: Figoo
 */

#ifndef SIG_GB_LLC_H_
#define SIG_GB_LLC_H_

#define LLC_HEAD_LEN 3		//LLC头长度

sb_s32 sp_get_llc_head(sp_packet_t *p_packet_info, sp_gb_t *gb_info);

#endif /* SIG_GB_LLC_H_ */
