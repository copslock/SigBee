/*
 * sig_gb_gmm.h
 *
 *  Created on: 2014年6月18日
 *      Author: Figoo
 */

#ifndef SIG_GB_GMM_H_
#define SIG_GB_GMM_H_

#define MIN_GMM_HEAD_LEN 2		//最小的GMM头长度

#define GMM_ATTACH_REQUEST 0x01
#define GMM_AUTH_CIPHER_RESPONSE 0x13
#define GMM_ATTACH_ACCEPT 0x02
#define GMM_ATTACH_COMPLETE 0x03
#define GMM_ATTACH_REJECT 0x04
#define GMM_DETACH_REQUEST 0x05
#define GMM_DETACH_ACCEPT 0x06

#define GMM_ROUTING_AREA_UPDATE_REQUEST 0x08
#define GMM_ROUTING_AREA_UPDATE_ACCEPT 0x09
#define GMM_ROUTING_AREA_UPDATE_COMPLETE 0x0A
#define GMM_ROUTING_AREA_UPDATE_REJECT 0x0B

#define GMM_ACT_PDP_CONTEXT_REQUEST 0x41
#define GMM_ACT_PDP_CONTEXT_ACCEPT 0x42
#define GMM_ACT_PDP_CONTEXT_REJECT 0x43
#define GMM_DEACT_PDP_CONTEXT_REQUEST 0x46
#define GMM_DEACT_PDP_CONTEXT_ACCEPT 0x47

//CDR TYPE
#define CDR_ATTACH 1
#define CDR_DETACH 2
#define CDR_ROUTING_AREA_UPDATE 3
#define CDR_PERIODIC_ROUTING_AREA_UPDATE 4
#define CDR_ACTIVATE_PDP_CONTEXT 5
#define CDR_DEACTIVATE_PDP_CONTEXT 6

//cdr result
#define CDR_INCOMPLETE 0
#define CDR_SUCCESS 1
#define CDR_FAILURE 2

//cdr detail result
#define CDR_REQUST_D 0x01
#define CDR_AUTH_RES_D 0x20
#define CDR_ACCEPT_D 0x02
#define CDR_COMPLETE_D 0x04
#define CDR_REJECT_D 0x08

sb_s32 sp_parse_attach_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_auth_cipher_response(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_attach_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_attach_complete(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_attach_reject(sp_packet_t *p_packet_info, sp_gb_t *gb_info);

sb_s32 sp_parse_detach_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_detach_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info);

sb_s32 sp_parse_routing_area_update_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_routing_area_update_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_routing_area_update_complete(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_routing_area_update_reject(sp_packet_t *p_packet_info, sp_gb_t *gb_info);

sb_s32 sp_parse_activate_pdp_context_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_activate_pdp_context_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_activate_pdp_context_reject(sp_packet_t *p_packet_info, sp_gb_t *gb_info);

sb_s32 sp_parse_deactivate_pdp_context_request(sp_packet_t *p_packet_info, sp_gb_t *gb_info);
sb_s32 sp_parse_deactivate_pdp_context_accept(sp_packet_t *p_packet_info, sp_gb_t *gb_info);

sb_s32 sp_get_gmm_head(sp_packet_t *p_packet_info, sp_gb_t *gb_info);


#endif /* SIG_GB_GMM_H_ */
