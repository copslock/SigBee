/*
 * test/ui_receiver/ui_receiver.c
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

#include <sb_type.h>

#pragma pack(1)
typedef struct _dp_user_info_tag
{
	sb_u8 interface;
	sb_u8 imsi[15];
	sb_u8 imei[16];
	sb_u32 ptmsi;
	sb_u32 mcc;
	sb_u32 mnc;
	sb_u32 lac;
	sb_u32 rac;
	sb_u32 cid;
	sb_u32 old_mcc;
	sb_u32 old_mnc;
	sb_u32 old_lac;
	sb_u32 old_rac;
	sb_u32 old_cid;
	sb_u32 sgsn_sig_ip;
	sb_u32 bsc_sig_ip;
	sb_u8 rat;
	sb_u8 apn[32];
} dp_user_info_t;

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

ssize_t send_pkt(const void *buf, size_t size);
ssize_t recv_pkt(void *buf, size_t size);

void exit_ui_receiver()
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
	exit_ui_receiver();
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
		exit_ui_receiver();
	}

	ret = listen(sockfd, 10);
	if (-1 == ret) {
		fprintf(stderr, "failed to bind\n");
		exit_ui_receiver();
	}

	socklen_t addrlen = sizeof(struct sockaddr_in);
	cfd = accept(sockfd, (struct sockaddr *)&cin, &addrlen);
	if (-1 == cfd) {
		fprintf(stderr, "failed to accept\n");
		exit_ui_receiver();
	}
	printf("ok, accept one client\n");
	/* Set to Nonblock mode */
	flags = fcntl(cfd, F_GETFL, 0);
	ret = fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

	fin = fopen("file.dat", "w");
	if (NULL == fin) {
		fprintf(stderr, "failed to open file.dat");
		exit_ui_receiver();
	}

	buf = malloc(1024 * 1024);
	if (!buf) {
		fprintf(stderr, "out of memory\n");
		exit_ui_receiver();
	}

	while (1) {
		ssize_t ret;
		sb_u8 imsi[16];
		sb_u8 imei[17];
		sb_u8 apn[33];
		sdtp_header_t sdtp_header;
		dp_user_info_t *user = (dp_user_info_t *)buf;

		ret = recv_pkt(&sdtp_header, sizeof(sdtp_header_t));
		if (ret != sizeof(sdtp_header_t)) {
			fprintf(stderr, "unable to receive data\n");
			continue;
		}

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

		memcpy(imsi, user->imsi, 15);
		imsi[15] = 0;
		memcpy(imei, user->imei, 16);
		imei[16] = 0;
		memcpy(apn, user->apn, 32);
		apn[32] = 0;

		fprintf(fin, "%hhu,%s,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
				"%u,%hhu,%s\n",
				user->interface, imsi, imei,
				ntohl(user->ptmsi),
				ntohl(user->mcc), ntohl(user->mnc),
				ntohl(user->lac), ntohl(user->rac),
				ntohl(user->cid), 
				ntohl(user->old_mcc),
				ntohl(user->old_mnc),
				ntohl(user->old_lac), 
				ntohl(user->old_rac),
				ntohl(user->old_cid), 
				ntohl(user->sgsn_sig_ip),
				ntohl(user->bsc_sig_ip),
				user->rat, apn);
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
		ret = send(sockfd, (sb_s8 *)buf + sent, size - sent, 0);
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

		ret = recv(sockfd, (sb_s8 *)buf + received, size - received, 0);
		if (ret < 0 && EINTR == errno)
			continue;
		if (ret <= 0) 
			return -1;
		//printf("receive %zd bytes\n", ret);

		received += ret;
	}

	return received;
}

