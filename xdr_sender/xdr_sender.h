/*
 * xdr_sender/xdr_sender.h
 */
#ifndef XDR_SENDER_H_
#define XDR_SENDER_H_

#pragma pack(1)
typedef struct _sdtp_packet {
	sb_u16	sync;
	sb_u16	len;
	sb_u16	type;
	sb_u32	id;
	sb_u32	ts;
	sb_u8 	nr;
	sb_u8	data[MAX_DATA_XDR_LEN];
} sdtp_packet_t;
#pragma pack()

#define ALIGN(v, n)		((v + n - 1) & ~(n - 1))
#define SDTP_PACKET_SIZE	(ALIGN(sizeof(sdtp_packet_t), 4))

#define NR_SP_WORKERS	g_adv_config.sig_parser_cpus
#define NR_DATA_WORKERS	g_adv_config.data_parser_cpus

#define INIT_XDR_SP_PACKET_QUEUE \
	(g_sp_xdr_queue_head->addr + g_process_order * NR_SP_WORKERS)
#define INIT_XDR_DP_USER_QUEUE \
	(g_dp_xdr_queue_head->addr + g_process_order * NR_DATA_WORKERS)

#define xs_info(msg, ...)	fprintf(stdout, msg, ##__VA_ARGS__)
#define xs_warning(msg, ...)	fprintf(stderr, msg, ##__VA_ARGS__)
#define xs_error(msg, ...)	fprintf(stderr, msg, ##__VA_ARGS__)

/* macros for manipulating queues */
#define inc_queue_head(queue) \
	(queue)->head = ((queue)->head + 1) % (queue)->size
#define inc_queue_tail(queue) \
	(queue)->tail = ((queue)->tail + 1) % (queue)->size
#define inc_queue_push_idx(queue) \
	(queue)->push_index++
#define inc_queue_pop_idx(queue) \
	(queue)->pop_index++
#define is_queue_full(queue)	\
	((queue)->push_index - (queue)->pop_index >= (queue)->size)
#define is_queue_empty(queue)	\
	((queue)->pop_index >= (queue)->push_index)

#endif /* XDR_SENDER_H_ */

