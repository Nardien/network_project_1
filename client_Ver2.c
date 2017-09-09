/* client_Ver2.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFSIZE 100

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

	int datalen = hdr->length - 16;

	/* printf data and hdr value */
	printf("\nop : %d\nkeyword : %u\nlength : %u\nchecksum : %u\n",
		(int)hdr->op, (unsigned int)hdr->keyword, (unsigned int)hdr->length, (unsigned int)hdr->checksum);
	
	char key_check[5];
	key_check[4] = '\0';
	memcpy(key_check, &hdr->keyword, 4);
	printf("%s\n\n", key_check);

	char data_check[datalen + 1];
	data_check[datalen] = '\0';
	memcpy(data_check, data, datalen);
	printf("data : %s\n", data_check);


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
	for (i = 0; i < datalen; i++) { 
		datasum += (uint16_t)data[i];   /* sum of all char in data */
	}


	if (hdr->checksum != (datasum + (uint16_t)hdr->op + (uint16_t)hdr->keyword + (uint16_t)hdr->length)) {
		/* checksum error */
		fprintf(stderr, "checksum error");
		return -1;
	}

	return 0;

}

int main(int argc, char* argv[]) {

	/* For getting argument */

	int flag_h = 0;
	int flag_p = 0;
	int flag_o = 0;
	int flag_k = 0;
	int c;
	char* host = NULL;
	char* port = NULL;
	char* op = NULL;
	char* key = NULL;

	/* For socket implementation */

	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int status;
	int sockfd, n;


	if (argc == 1) {
		fprintf(stderr, "options are not given\n");
		exit(1);
	}

	while ((c = getopt(argc, argv, "h:p:o:k:")) != -1) {
		switch(c) {
			case 'h' :
				flag_h = 1;
				host = malloc(strlen(optarg));
				strcpy(host, optarg);
				break;
			case 'p' :
				flag_p = 1;
				port = malloc(strlen(optarg));
				strcpy(port, optarg);
				break;
			case 'o' :
				flag_o = 1;
				op = malloc(strlen(optarg));
				strcpy(op, optarg);
				break;
			case 'k' :
				flag_k = 1;
				key = malloc(strlen(optarg));
				strcpy(key, optarg);
				break;
			case '?' :
				fprintf(stderr, "unknown option is given\n");
				exit(1);
		}
	}

	if (!(flag_h && flag_p && flag_o && flag_k)) {
		fprintf(stderr, "some options are not given\n");
		exit(1);
	}

	/* Test for argument getting */
	printf("host : %s\nport : %s\noption : %s\nkey : %s\n",
		host, port, op, key);

	/* get address information of host */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

	/* get socket fd then connect to that socket */

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			fprintf(stderr, "socket error\n");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			fprintf(stderr, "connect error\n");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "failed to connect\n");
		exit(1);
	}

	printf("client: connecting to server\n");

	/* get data from stdin */

	int size = BUFSIZE;
	char data[BUFSIZE];
	char* sending_data;
	int len = 0;

	fgets(data, BUFSIZE, stdin);
	len = strlen(data);
	sending_data = malloc(BUFSIZE);
	strcpy(sending_data, data);

	/* Not completed! Implement case of given string exceeds BUFSIE */

	while (size <= 10000000 - 16 && len == size) {
		fgets(data, BUFSIZE, stdin);
		len = strlen(data);
		size += len;

		sending_data = realloc(sending_data, size);
		strcat(sending_data, data);
	} 

	if (sending_data[strlen(sending_data) - 1] == '\n') {
		sending_data[strlen(sending_data) - 1] = '\0';
	}

	printf("data : %s\n", sending_data);

	/* Set header */

	struct header* hdr;

	hdr = malloc(sizeof(struct header));

	hdr->op = atoi(op);
	memcpy(&hdr->keyword, key, 4);

	hdr->length = 16 + strlen(sending_data); // \n in included
	hdr->checksum = (uint16_t)hdr->op + (uint16_t)hdr->keyword + (uint16_t)hdr->length;

	uint16_t datasum = 0;
	int i = 0;

	while (sending_data[i] != '\0') {
		datasum += (uint16_t)sending_data[i];
		i++;
	}

	hdr->checksum += datasum;

	/* Check header value */
	printf("\nop : %d\nkeyword : %u\nlength : %u\nchecksum : %u\n",
		(int)hdr->op, (unsigned int)hdr->keyword, (unsigned int)hdr->length, (unsigned int)hdr->checksum);
	char key_check[5];

	key_check[4] = '\0';
	memcpy(key_check, &hdr->keyword, 4);
	printf("%s\n\n", key_check);

	/* send header */

	n = send(sockfd, hdr, sizeof(struct header), 0);

	if (n < 0) {
		fprintf(stderr, "error at writing to socket\n");
		exit(1);
	}

	/* send data */

	n = send(sockfd, sending_data, strlen(sending_data), 0);


	if (n < 0) {
		fprintf(stderr, "error at writing to socket\n");
		exit(1);
	}

	/* #1 ends, #2 */
	/* receive resended data */

	int datalen = strlen(sending_data);
	memset(hdr, 0, sizeof(struct header));
	char data_resend[datalen];

	n = recv(sockfd, hdr, sizeof(struct header), 0);

	if (n < 0) {
		fprintf(stderr, "fail to read\n");
		exit(1);
	}

	n = recv(sockfd, data_resend, datalen, 0);

	if (n < 0) {
		fprintf(stderr, "fail to read\n");
		exit(1);
	}

	printf("\nop : %d\nkeyword : %u\nlength : %u\nchecksum : %u\n",
		(int)hdr->op, (unsigned int)hdr->keyword, (unsigned int)hdr->length, (unsigned int)hdr->checksum);

	if (check_validity(hdr, data_resend) < 0) {
		fprintf(stderr, "violate protocol rules");
		close(sockfd);
		exit(0);
	}

	char data_print[strlen(sending_data) + 1];
	strcpy(data_print, data_resend);
	data_print[strlen(sending_data)] = '\0';

	printf("%s\n", data_print);

	freeaddrinfo(servinfo);

	free(host);
	free(port);
	free(op);
	free(key);

	return 0;
}