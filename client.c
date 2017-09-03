#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE 10000000 - 16

struct header {

	uint16_t op;
	uint16_t checksum;
	uint32_t keyword;
	uint64_t length;

};

int main(int argc, char* argv[]) {

	int flag_h, flag_p, flag_o, flag_k;
	int c;
	char* host, port, op, key;

	struct addrinfo hints, *servinfo, *p;
	int sockfd, n;

	struct header* hdr;

	char data[BUFSIZE];
	uint16_t datasum;

	while ((c = getopt(argv, argv, "h:p:o:k:")) != -1) {
		switch(c) {
			case 'h' :
				flag_h = 1;
				host = optarg;
				break;
			case 'p' :
				flag_p = 1;
				port = optarg;
				break;
			case 'o' :
				flag_o = 1;
				op = optarg;
				break;
			case 'k' :
				flag_k = 1;
				key = optarg;
				break;
			case '?' :
				fprintf(stderr, "unknown option\n");
				break;
		}
	}

	if (!flag_p) {
		fprintf(stderr, "port number is not given\n");
		exit(1);
	}

	if (!flag_h) {
		fprintf(stderr, "host name is not given\n");
		exit(1);
	}

	if (!flag_o) {
		fprintf(stderr, "option is not given\n");
		exit(1);
	}

	if (!flag_k) {
		fprintf(stderr, "key is not given\n");
		exit(1);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket error");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("connect error");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	printf("client: connecting to server\n"); 

	freeaddrinfo(servinfo);

	/* set header */

	hdr = malloc(sizeof(struct header));

	hdr->option = op;
	hdr->keyword = key;

	fgets(data, BUFSIZE, stdin);  /* get string from stdin */

	hdr->length = 16 + strlen(data);
	hdr->checksum = (uint16_t)hdr->option + (uint16_t)hdr->keyword + (uint16_t)hdr->length;

	datasum = 0;
	int i = 0;

	while (data[i] != '\n') { 
		datasum += (uint16_t)data[i];   /* sum of all char in data */
	}

	hdr->checksum += datasum;

	n = send(sockfd, hdr, sizeof(struct header), 0);

	if (n < 0) {
		fprintf(stderr, "error at writing to socket\n");
		break;
	}

	n = send(sockfd, data, strlen(data) + 1, 0);

	if (n < 0) {
		fprintf(stderr, "error at writing to socket\n");
		break;
	}

	close(sockfd);

	return 0;
}

