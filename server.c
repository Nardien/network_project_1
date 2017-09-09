/* server.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

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
	strcpy(keyword, hdr->keyword);

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

	if (hdr->checksum != (datasum + (uint16_t)hdr->option + (uint16_t)hdr->keyword + (uint16_t)hdr->length)) {
		/* checksum error */
		fprintf(stderr, "checksum error");
		return -1;
	}

	return 0;

}

/* Encrypt given string via Vigenere Cipher */
int data_encrypt(uint32_t keyword, char data[]) {

	char key[4];
	int i = 0; /* for traverse data */
	int j = 0; /* for traverse key */

	memcpy(key, &keyword);

	/* convert key for using as key */
	for (j = 0; j < 4; j++) {
		key[j] = (char) tolower(key[j]);
		key[j] -= 97;
	}

	/* convert all char to lower case */
	while (data[i] != '\0') {
		data[i] = (char) tolower(data[i]);
		i++;
	}

	/* encrypt data */
	i = 0;
	while (data[i] != '\0') {
		/* encrypt only alphabet */
		if ((97 <= data[i]) && (data[i] <= 122)) {
			data[i] += key[j];

			if (122 < data[i]) {
				data[i] -= 26;
			}

			j++;
			if (j == 4) j = 0;
		}

		i++;
	}

	printf("Encrypted data : %s\n", data);
	
}

/* Decrypt given string via Vigenere Cipher */
int data_decrypt(uint32_t keyword, char data[]) {

	char key[4];
	int i = 0; /* for traverse data */
	int j = 0; /* for traverse key */

	memcpy(key, &keyword);

	/* convert key for using as key */
	for (j = 0; j < 4; j++) {
		key[j] = (char) tolower(key[j]);
		key[j] -= 97;
	}

	/* convert all char to lower case */
	while (data[i] != '\0') {
		data[i] = (char) tolower(data[i]);
		i++;
	}

	/* decrypt data */
	i = 0;
	while (data[i] != '\0') {
		/* encrypt only alphabet */
		if ((97 <= data[i]) && (data[i] <= 122)) {
			data[i] -= key[j];

			if (data[i] < 97) {
				data[i] += 26;
			}

			j++;
			if (j == 4) j = 0;
		}

		i++;
	}

	printf("Decrypted data : %s\n", data);
	
}

int main(int argc, char* argv[]) {

	int flag_p = 0;
	char* port;
	int c; 

	int sockfd, newsockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	int status;
	int n;
	struct header* hdr;
	char data[BUFSIZE];

	while ((c = getopt(argc, argv, "p:")) != -1) {
		switch (c) {
			case 'p' :
				flag_p = 1;
				port = optarg;
				break;
			case '?' :
				exit(1);
		}
	}

	if (!flag_p) {
		fprintf(stderr, "server: port number is not given\n");
		exit(1);
	}

	fprintf(stdout, "port no. : %s\n", port);

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

	for (p = servinfo; p != NULL; 	p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: fail to make socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("server: setsockopt error");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: fail to bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: fail to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	if (listen(sockfd, 10) == -1) { // 10 connections allowed
		perror("server: fail to listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("server: sigaction error");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {
		sin_size = sizeof(their_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (newsockfd == -1) {
			perror("server: fail to accept");
			continue;
		}

		printf("server: get connection from client\n");

		if (!fork()) {

			hdr = malloc(sizeof(struct header));

			memset(hdr, 0, sizeof(struct header));
			memset(data, 0, BUFSIZE);

			n = recv(newsockfd, hdr, sizeof(struct header), 0);

			if (n < 0) {
				fprintf(stderr, "server: fail to read");
				close(newsockfd);
				exit(0);
			}

			n = recv(newsockfd, data, BUFSIZE, 0);

			if (n < 0) {
				fprintf(stderr, "server: fail to read");
				close(newsockfd);
				exit(0);
			}

			if (check_validity(hdr, data) < 0) {
				fprintf(stderr, "violate protocol rules");
				close(newsockfd);
				exit(0);
			}

			if (hdr->op == 0)
				data_encrypt(hdr->keyword, data);
			else if (hdr->op == 1)
				data_decrypt(hdr->keyword, data);

			/* re-packing */

			uint16_t datasum;
			int i = 0;

			while (data[i] != '\0') { 
				datasum += (uint16_t)data[i];   /* sum of all char in data */
				i++;
			}

			/* length is unchanged... */

			hdr->checksum = (uint16_t)hdr->option + (uint16_t)hdr->keyword + (uint16_t)hdr->length + datasum;

			/* send encrypted data to client */

			n = send(newsockfd, hdr, sizeof(struct header), 0);

			if (n < 0) {
				fprintf(stderr, "error at writing to socket\n");
				break;
			}

			n = send(newsockfd, data, strlen(data) + 1, 0);

			if (n < 0) {
				fprintf(stderr, "error at writing to socket\n");
				break;
			}


			/* close connection between server and given client */
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}

	return 0;
}