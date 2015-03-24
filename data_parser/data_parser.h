/*
 * data_parser/data_parser.h
 *
 */

#ifndef DATA_PARSER_H_
#define DATA_PARSER_H_

#define NR_SP_WORKERS	g_adv_config.sig_parser_cpus
#define NR_DATA_WORKERS	g_adv_config.data_parser_cpus
#define NR_XDR_WORKERS	g_adv_config.xdr_sender_cpus
#define NR_DPI_WORKERS	g_device_config.dpi_dev_num

#define INIT_DP_SP_PACKET_QUEUE \
	(g_sp_packet_queue_head->addr + g_process_order * NR_SP_WORKERS)
#define INIT_DP_SP_USER_QUEUE \
	(g_sp_user_queue_head->addr + g_process_order * NR_DATA_WORKERS)
#define INIT_DP_XDR_QUEUE \
	(g_dp_xdr_queue_head->addr + g_process_order * NR_XDR_WORKERS)
#define INIT_DP_USER_QUEUE \
	(g_dp_user_queue_head->addr + g_process_order * NR_DPI_WORKERS)

extern void if0_1_send_packet(dp_packet_t *dp_packet);

#endif /* DATA_PARSER_H_ */

