/*
 * test/xdr_receiver/xdr_receiver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <signal.h>
#include <endian.h>
#include <time.h>

#include <sb_xdr_struct.h>

#define HEADER_SIZE	(sizeof(xdr_pub_header_t) + sizeof(xdr_header_t))

#pragma pack(1)
typedef struct _sdtp_header {
	sb_u16	sync;
	sb_u16	len;
	sb_u16	type;
	sb_u32	id;
	sb_u32	ts;
	sb_u8 	nr;
} sdtp_header_t;
#pragma pack()

static struct pollfd pfd;
static sb_s32 sockfd = -1;
static sb_s32 cfd = -1;
sb_u8 *buf = NULL;
FILE *fin = NULL;
sb_u64 bytes = 0;
static struct sockaddr_in sin;
static struct sockaddr_in cin;
static xdr_header_t *xdr_header;
static xdr_pub_header_t *pub_header;

ssize_t send_pkt(const void *buf, size_t size);
ssize_t recv_pkt(void *buf, size_t size);

void print_attach();
void print_identity();
void print_auth();
void print_detach();
void print_pdp_act();
void print_pdp_deact();
void print_pdp_mod();
void print_rab();
void print_rau();
void print_bssgp();
void print_ranap();
void print_reloc();
void print_sr();
void print_paging();
void print_tup();
void print_other();


void exit_xdr_receiver()
{
	if (-1 != cfd)
                close(cfd);
        if (-1 != sockfd)
                close(sockfd);

        if (NULL != fin)
                fclose(fin);
        if (NULL != buf)
                free(buf);
        printf("\nreceived %lu bytes\n", bytes);
        exit(0);
}

void sig(sb_s32 signo)
{
	printf("received signal\n");
	exit_xdr_receiver();
}

sb_s32 main(sb_s32 argc, sb_s8 **argv)
{
	sb_s32 ret;
	sb_s32 flags;

	if (2 > argc) {
		fprintf(stderr, "need a port\n");
		return -1;
	}

	signal(SIGINT, sig);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		fprintf(stderr, "failed to create socket\n");
		exit(-1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((sb_u16)atoi(argv[1]));
	sin.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *)&sin, sizeof(sin));
	//printf("%s\n", strerror(errno));
	if (-1 == ret) {
		fprintf(stderr, "failed to bind\n");
		exit_xdr_receiver();
	}

	ret = listen(sockfd, 10);
	if (-1 == ret) {
		fprintf(stderr, "failed to bind\n");
		exit_xdr_receiver();
	}

	socklen_t addrlen = sizeof(struct sockaddr_in);
	cfd = accept(sockfd, (struct sockaddr *)&cin, &addrlen);
	if (-1 == cfd) {
		fprintf(stderr, "failed to accept\n");
		exit_xdr_receiver();
	}
	printf("ok, accept one client\n");

	pfd.fd = cfd;
	/* Set to Nonblock mode */
	flags = fcntl(cfd, F_GETFL, 0);
	ret = fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

	fin = fopen("file.dat", "w");
	if (NULL == fin) {
		fprintf(stderr, "failed to open file.dat");
		exit_xdr_receiver();
	}

	buf = malloc(1024 * 1024);
	if (!buf) {
		fprintf(stderr, "out of memory\n");
		exit_xdr_receiver();
	}

	while (1) {
		ssize_t ret;
		sb_u8 imsi[16];
		sb_u8 imei[17];
		sb_u8 apn[33];
		sdtp_header_t sdtp_header;

		xdr_header = (xdr_header_t *)buf;
		pub_header =
			(xdr_pub_header_t *)(buf + sizeof(xdr_header_t));
		sb_u8 *proc_type = buf + HEADER_SIZE;

		ret = recv_pkt(&sdtp_header, sizeof(sdtp_header_t));
		if (ret != sizeof(sdtp_header_t)) {
			fprintf(stderr, "unable to receive data: %zd\n", ret);
			sleep(5);
			continue;
		}

		bytes += ret;

		sdtp_header.sync = ntohs(sdtp_header.sync);
		// 减去头部
		sdtp_header.len = ntohs(sdtp_header.len) - 15;

		if (0x7e5a != sdtp_header.sync) {
			fprintf(stderr, "NOT a SDTP packet\n");
			continue;
		}

		ret = recv_pkt(buf, sdtp_header.len);
		if (ret != sdtp_header.len) {
			fprintf(stderr, "unable to received the rest of data\n");
			continue;
		}

		bytes += ret;

		memcpy(imsi, pub_header->imsi, 15);
		imsi[15] = 0;
		memcpy(imei, pub_header->imei, 16);
		imei[16] = 0;
		memcpy(apn, pub_header->apn, 32);
		apn[32] = 0;

		fprintf(fin, "%hu,%hhu,%hhu,%hu,%hu,%lu,"	/* xdr_header */
				"%hhu,%s,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
				"%u,%u,%hhu,%s,%hhu,",	/* xdr_pub_header */
				ntohs(xdr_header->len),
				xdr_header->type,
				xdr_header->version,
				ntohs(xdr_header->var_ie_num),
				ntohs(xdr_header->proto_type),
				be64toh(xdr_header->id),
				pub_header->interface, imsi, imei,
				ntohl(pub_header->ptmsi),
				ntohl(pub_header->mcc), ntohl(pub_header->mnc),
				ntohl(pub_header->lac), ntohl(pub_header->rac),
				ntohl(pub_header->cid),
				ntohl(pub_header->old_mcc),
				ntohl(pub_header->old_mnc),
				ntohl(pub_header->old_lac),
				ntohl(pub_header->old_rac),
				ntohl(pub_header->old_cid),
				ntohl(pub_header->sgsn_sig_ip),
				ntohl(pub_header->bsc_sig_ip),
				pub_header->rat, apn, *proc_type);


		switch (*proc_type) {
		case 0:	/* attach */
			printf("attach\n");
			print_attach();
			break;
		case 1:	/* identity */
			printf("identify\n");
			print_identity();
			break;
		case 2:	/* authentication */
			printf("authentication\n");
			print_auth();
			break;
		case 3:	/* detach */
			printf("deattach\n");
			print_detach();
			break;
		case 4:	/* pdp activate */
			printf("pdp activate\n");
			print_pdp_act();
			break;
		case 5: /* pdp deactivate */
			printf("pdp deactivate\n");
			print_pdp_deact();
			break;
		case 6:	/* pdp modification */
			print_pdp_mod();
			break;
		case 7:	/* rab */
			print_rab();
			break;
		case 8:	/* rau */
			print_rau();
			break;
		case 9:	/* bssgp */
			print_bssgp();
			break;
		case 10:	/* ranap */
			print_ranap();
			break;
		case 11:	/* relocation */
			print_reloc();
			break;
		case 12:	/* sr */
			print_sr();
			break;
		case 13:	/* paging */
			print_paging();
			break;
		case 100:	/* tup */
			print_tup();
			break;
		case 254:	/* other */
			print_other();
			break;
		default:
			printf("unknown type\n");
			break;
		}

	}

	return 0;
}

ssize_t send_pkt(const void *buf, size_t size)
{
	size_t sent = 0;
	ssize_t ret;

	pfd.events = POLLOUT;

	while (sent != size) {
		if (0 == poll(&pfd, 1, 5000))
			break;

		if (!(pfd.revents & POLLOUT))
			continue;
		ret = send(cfd, (sb_s8 *)buf + sent, size - sent, 0);
		if (ret < 0 && EINTR == errno)
			continue;

		if (ret < 0)
			return -1;

		//printf("sent %zd\n", ret);

		sent += ret;
	}

	return sent;
}

ssize_t recv_pkt(void *buf, size_t size)
{
	size_t received = 0;
	ssize_t ret;

	pfd.events = POLLIN;

	while (received != size) {
		if (0 == poll(&pfd, 1, 5000))
			break;

		if (!(pfd.revents & POLLIN))
			continue;

		ret = recv(cfd, (sb_s8 *)buf + received, size - received, 0);
		if (ret < 0 && EINTR == errno)
			continue;
		if (ret <= 0)
			return -1;
		//printf("receive %zd bytes\n", ret);

		received += ret;
	}

	return received;
}

sb_s8 start_time[30];
sb_s8 end_time[30];

void fmttime(sb_u64 t, sb_s8 *buf, size_t maxsize)
{
	struct tm *tm;
	time_t clock = (time_t)(t & 0xffffffff);

	tm = localtime(&clock);

	snprintf(buf, maxsize, "%d-%02d-%02d %02d:%02d:%02d.%u",
			tm->tm_year + 1900, tm->tm_mon + 1,
			tm->tm_mday, tm->tm_hour, tm->tm_min,
			tm->tm_sec, (sb_u32)(t >> 32) / 1000000);
}


void print_attach()
{
	xdr_gb_iups_attach_t * att = (xdr_gb_iups_attach_t *)buf;

	fmttime(be64toh(att->start_time), start_time, 30);
	fmttime(be64toh(att->end_time), end_time, 30);

	fprintf(fin, "%lu,%hu,%s,%s,%hu,%hhu,%hhu,%u\n",
			be64toh(att->procedure_id),
			ntohs(att->sub_procedure_id),
			start_time, end_time,
			ntohs(att->result_code),
			att->result_type, att->attach_type,
			ntohl(att->old_ptmsi));
}

void print_identity()
{
	xdr_gb_iups_identity_t * id = (xdr_gb_iups_identity_t *)buf;

	fmttime(be64toh(id->start_time), start_time, 30);
	fmttime(be64toh(id->end_time), end_time, 30);

	fprintf(fin, "%lu,%hu,%s,%s,%hu,%hhu\n",
			be64toh(id->procedure_id),
			ntohs(id->sub_procedure_id),
			start_time, end_time,
			ntohs(id->result_code),
			id->result_type);
}

void print_auth()
{
	xdr_gb_iups_auth_t * auth = (xdr_gb_iups_auth_t *)buf;

	fmttime(be64toh(auth->start_time), start_time, 30);
	fmttime(be64toh(auth->end_time), end_time, 30);

	fprintf(fin, "%lu,%hu,%s,%s,%hu,%hhu\n",
			be64toh(auth->procedure_id),
			ntohs(auth->sub_procedure_id),
			start_time, end_time,
			ntohs(auth->result_code),
			auth->result_type);
}

void print_detach()
{
	xdr_gb_iups_detach_t * detach = (xdr_gb_iups_detach_t *)buf;

	fmttime(be64toh(detach->start_time), start_time, 30);
	fmttime(be64toh(detach->end_time), end_time, 30);

	fprintf(fin, "%lu,%hu,%s,%s,%hu,%hhu,%hhu\n",
			be64toh(detach->procedure_id),
			ntohs(detach->sub_procedure_id),
			start_time, end_time,
			ntohs(detach->result_code),
			detach->result_type, detach->detach_type);
}

void print_pdp_act()
{
	sb_s8 net_request_time[30];
	sb_s8 net_accept_time[30];

	xdr_gb_iups_pdpact_t *act = (xdr_gb_iups_pdpact_t *)buf;

	fmttime(be64toh(act->start_time), start_time, 30);
	fmttime(be64toh(act->accept_time), end_time, 30);
	fmttime(be64toh(act->net_request_time), net_request_time, 30);
	fmttime(be64toh(act->net_accept_time), net_accept_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hhu,%hhu,%s,%s,%hu,%hhu,%s,%hhu,%hu,"
			"%hu,%hu,%hu\n",
			be64toh(act->procedure_id),
			act->net_sub_procedure_id, net_request_time,
			net_accept_time, act->net_result_type,
			act->sub_procedure_id, start_time, end_time,
			ntohs(act->result_code), act->result_type,
			act->pdp_addr, act->connect_type,
			ntohs(act->up_rate), ntohs(act->down_rate),
			ntohs(act->gua_up_rate), ntohs(act->gua_down_rate));
}

void print_pdp_deact()
{
	xdr_gb_iups_pdpdeact_t *deact = (xdr_gb_iups_pdpdeact_t *)buf;

	fmttime(be64toh(deact->start_time), start_time, 30);
	fmttime(be64toh(deact->end_time), end_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hu,%hhu\n",
			be64toh(deact->procedure_id),
			deact->sub_procedure_id,
			start_time, end_time,
			ntohs(deact->result_code),
			deact->result_type);
}

void print_pdp_mod()
{
	sb_s8 net_request_time[30];
	sb_s8 net_accept_time[30];

	xdr_gb_iups_pdpmod_t *mod = (xdr_gb_iups_pdpmod_t *)buf;

	fmttime(be64toh(mod->start_time), start_time, 30);
	fmttime(be64toh(mod->end_time), end_time, 30);
	fmttime(be64toh(mod->net_request_time), net_request_time, 30);
	fmttime(be64toh(mod->net_accept_time), net_accept_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hhu,%hhu,%s,%s,%hu,%hhu,%hhu,%hu,"
			"%hu,%hu,%hu\n",
			be64toh(mod->procedure_id),
			mod->net_sub_procedure_id, net_request_time,
			net_accept_time, mod->net_result_type,
			mod->sub_procedure_id, start_time, end_time,
			ntohs(mod->end_time), mod->result_type,
			mod->connect_type,
			ntohs(mod->up_rate), ntohs(mod->down_rate),
			ntohs(mod->gua_up_rate), ntohs(mod->gua_down_rate));

}

void print_rab()
{
	xdr_gb_iups_rab_t * rab = (xdr_gb_iups_rab_t *)buf;

	fmttime(be64toh(rab->start_time), start_time, 30);
	fmttime(be64toh(rab->end_time), end_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hu,%hhu\n",
			be64toh(rab->procedure_id),
			rab->sub_procedure_id,
			start_time, end_time,
			ntohs(rab->result_code),
			rab->result_type);
}

void print_rau()
{
	xdr_gb_iups_rau_t * rau = (xdr_gb_iups_rau_t *)buf;

	fmttime(be64toh(rau->start_time), start_time, 30);
	fmttime(be64toh(rau->end_time), end_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hu,%hhu,%hhu,%u\n",
			be64toh(rau->procedure_id),
			rau->sub_procedure_id,
			start_time, end_time,
			ntohs(rau->result_code),
			rau->result_type, rau->rau_type,
			ntohl(rau->old_ptmsi));
}

void print_bssgp()
{
	xdr_gb_iups_bssgp_t * bssgp = (xdr_gb_iups_bssgp_t *)buf;

	fmttime(be64toh(bssgp->start_time), start_time, 30);
	fmttime(be64toh(bssgp->end_time), end_time, 30);

	fprintf(fin, "%lu,%hu,%s,%s,%hu,%hhu\n",
			be64toh(bssgp->procedure_id),
			bssgp->sub_procedure_id,
			start_time, end_time,
			ntohs(bssgp->result_code),
			bssgp->result_type);
}

void print_ranap()
{
	xdr_gb_iups_ranap_t * ranap = (xdr_gb_iups_ranap_t *)buf;

	fmttime(be64toh(ranap->start_time), start_time, 30);
	fmttime(be64toh(ranap->end_time), end_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hu,%hhu\n",
			be64toh(ranap->procedure_id),
			ranap->sub_procedure_id,
			start_time, end_time,
			ntohs(ranap->result_code),
			ranap->result_type);
}

void print_reloc()
{
	xdr_gb_iups_reloc_t * reloc = (xdr_gb_iups_reloc_t *)buf;

	fmttime(be64toh(reloc->start_time), start_time, 30);
	fmttime(be64toh(reloc->end_time), end_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hu,%hhu,%hu,%hu,%hu\n",
			be64toh(reloc->procedure_id),
			ntohs(reloc->sub_procedure_id),
			start_time, end_time,
			ntohs(reloc->result_code),
			reloc->result_type,
			ntohs(reloc->src_rnc_id),
			ntohs(reloc->dst_rnc_id),
			ntohs(reloc->relocation_cause));
}

void print_sr()
{
	sb_s8 net_request_time[30];
	sb_s8 net_accept_time[30];

	xdr_gb_iups_sr_t *sr = (xdr_gb_iups_sr_t *)buf;

	fmttime(be64toh(sr->start_time), start_time, 30);
	fmttime(be64toh(sr->end_time), end_time, 30);
	fmttime(be64toh(sr->net_request_time), net_request_time, 30);
	fmttime(be64toh(sr->net_accept_time), net_accept_time, 30);

	fprintf(fin, "%lu,%hhu,%s,%s,%hhu,%hhu,%hhu,%hhu,%s,%s,%hu,%hhu,%hhu\n",
			be64toh(sr->procedure_id),
			sr->net_sub_procedure_id, net_request_time,
			net_accept_time, sr->net_result_type,
			sr->subsesstype, sr->paging_num,
			sr->sub_procedure_id, start_time, end_time,
			ntohs(sr->result_code), sr->result_type,
			sr->sr_type);
}

void print_paging()
{
	xdr_gb_iups_paging_t * paging = (xdr_gb_iups_paging_t *)buf;

	fmttime(be64toh(paging->start_time), start_time, 30);
	fmttime(be64toh(paging->end_time), end_time, 30);

	fprintf(fin, "%lu,%hu,%s,%s,%hu,%hhu,%hhu,%hhu\n",
			be64toh(paging->procedure_id),
			paging->sub_procedure_id,
			start_time, end_time,
			ntohs(paging->result_code),
			paging->result_type, paging->subsesstype,
			paging->paging_num);
}

void print_tup()
{
	xdr_tup_t *tup = (xdr_tup_t *)buf;

	fmttime(be64toh(tup->user_pub_header.start_time), start_time, 30);
	fmttime(be64toh(tup->user_pub_header.end_time), end_time, 30);

	fprintf(fin, "%lu,%s,%s,%hu,%hu,%hhu,%s,%hu,%s,%hu,%s,%s,%u,%u,"
			"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
			"%hhu,%hhu,%hhu\n",
			be64toh(tup->user_pub_header.procedure_id),
			start_time, end_time,
			ntohs(tup->user_pub_header.app_type),
			ntohs(tup->user_pub_header.app_subtype),
			tup->user_pub_header.l4_protocol,
			tup->user_pub_header.user_ip,
			ntohs(tup->user_pub_header.user_port),
			tup->user_pub_header.server_ip,
			ntohs(tup->user_pub_header.server_port),
			tup->user_pub_header.sgsn_user_ip,
			tup->user_pub_header.bsc_user_ip,
			ntohl(tup->user_pub_header.ul_bytes),
			ntohl(tup->user_pub_header.dl_bytes),
			ntohl(tup->user_pub_header.ul_packets),
			ntohl(tup->user_pub_header.dl_packets),
			ntohl(tup->user_pub_header.ul_reorder_packets),
			ntohl(tup->user_pub_header.dl_reorder_packets),
			ntohl(tup->user_pub_header.ul_retrans_packets),
			ntohl(tup->user_pub_header.dl_retrans_packets),
			ntohl(tup->user_pub_header.ul_ip_frag_packets),
			ntohl(tup->user_pub_header.dl_ip_frag_packets),
			ntohl(tup->tcp_create_res_delay),
			ntohl(tup->tcp_create_ack_delay),
			ntohl(tup->tcp_first_req_delay),
			ntohl(tup->tcp_first_res_delay),
			ntohl(tup->windows_size),
			ntohl(tup-> mss_size),
			tup->tcp_create_try_times,
			tup->tcp_create_status,
			tup->session_end_status);

}

void print_other()
{
	xdr_gb_iups_other_t * other = (xdr_gb_iups_other_t *)buf;

	fmttime(be64toh(other->start_time), start_time, 30);
	fmttime(be64toh(other->end_time), end_time, 30);

	fprintf(fin, "%lu,%hu,%s,%s,%hhu\n",
			be64toh(other->procedure_id),
			other->sub_procedure_id,
			start_time, end_time,
			other->result_type);
}

