/* client.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>

#define BUFSIZE 10000000 - 16

struct header {

	uint16_t op;
	uint16_t checksum;
	uint32_t keyword;
	uint64_t length;

};

/* Check validity of header by computing checksum 
   If header is valid, return 0. Else, return -1 */
int check_validity(struct header* hdr, char data[]) {

	char keyword[4];
	uint16_t datasum;

	/* check operation if it is 0 or 1 */
	if (hdr->op != 0 && hdr->op != 1) {
		/* invalid operation */
		fprintf(stderr, "invalid operation");
		return -1;
	}

	/* check keyword validity */
	memcpy(keyword, &hdr->keyword, 4);

	int i = 0;

	for (i = 0; i < 4; i++) {
		if (keyword[i] < 65 && 90 < keyword[i] && keyword[i] < 97 && 122 < keyword[i]) {
			/* i-th char of keyword is non-alphabet */
			fprintf(stderr, "invalid keyword");
			return -1;
		}
	}

	i = 0;

	/* check checksum */
	while (data[i] != '\0') { 
		datasum += (uint16_t)data[i];   /* sum of all char in data */
		i++;
	}

	if (hdr->checksum != (datasum + (uint16_t)hdr->op + (uint16_t)hdr->keyword + (uint16_t)hdr->length)) {
		/* checksum error */
		fprintf(stderr, "checksum error");
		return -1;
	}

	return 0;

}


int main(int argc, char* argv[]) {

	int status;

	struct addrinfo hints, *servinfo, *p;
	int sockfd, n;

	struct header* hdr;

	char data[BUFSIZE];
	uint16_t datasum;

	int flag_h = 0, flag_p = 0, flag_o = 0, flag_k = 0;
	int c;

	char* host = NULL;
	char* port = NULL;
	char* op = NULL;
	char* key = NULL;
	
	while ((c = getopt(argc, argv, "h:p:o:k:")) != -1) {
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

	// #1

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

	hdr->op = atoi(op);
	memcpy(&hdr->keyword, key, 4);

	fgets(data, BUFSIZE, stdin);  /* get string from stdin */

	hdr->length = 16 + strlen(data);
	hdr->checksum = (uint16_t)hdr->op + (uint16_t)hdr->keyword + (uint16_t)hdr->length;

	datasum = 0;
	int i = 0;

	while (data[i] != '\0') { 
		datasum += (uint16_t)data[i];   /* sum of all char in data */
		i++;
	}

	hdr->checksum += datasum;

	n = send(sockfd, hdr, sizeof(struct header), 0);

	if (n < 0) {
		fprintf(stderr, "error at writing to socket\n");
		//break;
	}

	n = send(sockfd, data, strlen(data) + 1, 0);

	if (n < 0) {
		fprintf(stderr, "error at writing to socket\n");
		//break;
	}

	// #1 ends

	// #2

	memset(hdr, 0, sizeof(struct header));
	memset(data, 0, BUFSIZE);

	n = recv(sockfd, hdr, sizeof(struct header), 0);

	if (n < 0) {
		fprintf(stderr, "fail to read\n");
		//break;
	}

	n = recv(sockfd, data, BUFSIZE, 0);

	if (n < 0) {
		fprintf(stderr, "fail to read\n");
		//break;
	}

	if (check_validity(hdr, data) < 0) {
		fprintf(stderr, "violate protocol rules");
		close(sockfd);
		exit(0);
	}

	// #2 ends

	// #3

	printf("%s\n", data);

	close(sockfd);

	return 0;
}

