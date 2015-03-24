/*
 * sig_gb_bssgp.h
 *
 *  Created on: 2014年6月18日
 *      Author: Figoo
 */

#ifndef SIG_GB_BSSGP_H_
#define SIG_GB_BSSGP_H_

#define BSSGP_CELL_IDENTIFIER	0x08
#define BSSGP_MSRAC				0x13
#define BSSGP_IMSI				0x0D
#define BSSGP_LLC_PDU			0x0E
#define BSSGP_TLLI                         0x1F

sb_s32 sp_get_bssgp_head(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_get_msc(sb_u8 *data, gprs_msc_t *msc);


#endif /* SIG_GB_BSSGP_H_ */
