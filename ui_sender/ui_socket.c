/*
 * ui_sender/ui_socket.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <gxio/mpipe.h>

#include "sb_type.h"
#include "sb_defines.h"
#include "sb_struct.h"
#include "sb_base_util.h"
#include "sb_shared_queue.h"
#include "sb_public_var.h"

static struct pollfd pfd;
static sb_s32 sockfd = -1;

sb_s32 init_ui_socket()
{
	sb_s32 ret;
	sb_s32 flag;
	struct sockaddr_in sin;

	if (0 == g_device_config.dpi_flag)	// 不发送
		return -2;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		printf("unable to create socket: %s\n", strerror(errno));
		return -1;
	}

	pfd.fd = sockfd;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(g_device_config.dpi_dev[0].dev_ip);
	sin.sin_port = htons(9002);

	ret = connect(sockfd, (struct sockaddr *)&sin, sizeof(sin));
	//printf("%s\n", strerror(errno));
	if (-1 == ret) {
		printf("unable to connect to %s: %s\n",
				inet_ntoa(sin.sin_addr), strerror(errno));
		close(sockfd);
		return -1;
	}

	flag = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);

	return 0;
}

void exit_ui_socket()
{
	if (0 > sockfd)
		return;
	close(sockfd);
	sockfd = -1;
}

ssize_t send_ui_pkt(const void *buf, size_t size)
{
	size_t sent = 0;
	ssize_t ret;

	if (0 == g_device_config.dpi_flag)	// 不发送
		return size;

	pfd.events = POLLOUT;

	while (sent != size) {
		if (0 == poll(&pfd, 1, 5000))
			break;

		if (!(pfd.revents & POLLOUT))
			continue;
		ret = send(sockfd, (char *)buf + sent, size - sent, 0);
		if (ret < 0 && EINTR == errno)
			continue;

		if (ret < 0) {
			exit_ui_socket();
			while (-1 == init_ui_socket())
				sleep(1);
			sent = 0;
			continue;
		}

		//printf("sent %zd\n", ret);

		sent += ret;
	}

	return sent;
}

ssize_t recv_ui_pkt(void *buf, size_t size)
{
	size_t received = 0;
	ssize_t ret;

	if (0 == g_device_config.dpi_flag)	// 不发送
		return size;

	pfd.events = POLLIN;

	while (received != size) {
		if (0 == poll(&pfd, 1, 5000))
			break;

		if (!(pfd.revents & POLLIN))
			continue;

		ret = recv(sockfd, (char *)buf + received, size - received, 0);
		if (ret < 0 && EINTR == errno)
			continue;
		if (ret <= 0) {
			exit_ui_socket();
			while (-1 == init_ui_socket())
				sleep(1);
			received = 0;
			continue;
		}
		//printf("receive %zd bytes\n", ret);

		received += ret;
	}

	return received;
}

