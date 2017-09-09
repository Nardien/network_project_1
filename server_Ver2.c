/* server_Ver2.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

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

/* Encrypt given string via Vigenere Cipher */
void data_encrypt(struct header* hdr, char data[], char* data_resend) {

	char key[4];
	int i = 0; /* for traverse data */
	int j = 0; /* for traverse key */

	uint32_t keyword = hdr->keyword;
	int datalen = hdr->length - 16;

	memcpy(key, &keyword, 4);

	printf("key : ");
	/* convert key for using as key */
	for (j = 0; j < 4; j++) {
		key[j] = (char) tolower(key[j]);
		key[j] -= 97;
		printf("%d ", key[j]);
	}
	printf("\n");



	/* convert all char to lower case */
	while (i < datalen) {
		data[i] = (char) tolower(data[i]);
		i++;
	}

	/* encrypt data */

	char encrypted_data[datalen];
	i = 0;
	j = 0;

	char change;
	while (i < datalen) {
		/* encrypt only alphabet */
		printf("Let's encrypt %c\n", data[i]);
		if ((97 <= data[i]) && (data[i] <= 122)) {
			printf("encrypt %c", data[i]);

			change = data[i] + key[j];

			if (122 < change) {
				change -= 26;
			}

			encrypted_data[i] = change;

			printf(" to %c!\n", encrypted_data[i]);
			j++;
			if (j == 4) j = 0;
		}

		i++;
	}

	char data_check[datalen + 1];
	data_check[datalen] = '\0';
	memcpy(data_check, encrypted_data, datalen);
	printf("Encrypted data : %s\n", data_check);

	memcpy(data_resend, encrypted_data, datalen);
	printf("Successfully move!\n");
	
}

/* Decrypt given string via Vigenere Cipher */
void data_decrypt(struct header* hdr, char data[], char* data_resend) {

	char key[4];
	int i = 0; /* for traverse data */
	int j = 0; /* for traverse key */

	uint32_t keyword = hdr->keyword;
	int datalen = hdr->length - 16;

	memcpy(key, &keyword, 4);

	printf("key : ");
	/* convert key for using as key */
	for (j = 0; j < 4; j++) {
		key[j] = (char) tolower(key[j]);
		key[j] -= 97;
		printf("%d ", key[j]);
	}
	printf("\n");


	/* convert all char to lower case */
	while (i < datalen) {
		data[i] = (char) tolower(data[i]);
		i++;
	}

	/* decrypt data */

	char decrypted_data[datalen];
	i = 0;
	j = 0;
	char change;
	while (i < datalen) {
		/* encrypt only alphabet */
		printf("Let's decrypt %c\n", data[i]);
		if ((97 <= data[i]) && (data[i] <= 122)) {
			printf("decrypt %c", data[i]);

			change = data[i] - key[j];

			if (97 > change) {
				change += 26;
			}

			decrypted_data[i] = change;

			printf(" to %c!\n", decrypted_data[i]);
			j++;
			if (j == 4) j = 0;
		}

		i++;
	}

	char data_check[datalen + 1];
	data_check[datalen] = '\0';
	memcpy(data_check, decrypted_data, datalen);
	printf("Decrypted data : %s\n", data_check);

	memcpy(data_resend, decrypted_data, datalen);
	
}

int main(int argc, char *argv[]) {

	/* For getting argument */

	int flag_p = 0;
	char* port;
	int c;

	/* For socket implementation */

	int sockfd, newsockfd;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	int yes = 1;
	int status, n;


	if (argc == 1) {
		fprintf(stderr, "options are not given\n");
		exit(1);
	}

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

	printf("port no. : %s\n", port);

	/* get address information */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(1);
	}

	/* bind to socket */

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			fprintf(stderr, "fail to make socket\n");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			fprintf(stderr, "setsockopt error\n");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			fprintf(stderr, "fail to bind\n");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: fail to bind\n");
		exit(1);
	}

	freeaddrinfo(servinfo);

	/* 10 connections allowed */
	if (listen(sockfd, 10) == -1) {
		fprintf(stderr, "fail to listen\n");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	/* accept connections */
	while (1) {
		sin_size = sizeof(their_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (newsockfd == -1) {
			fprintf(stderr, "fail to accept\n");
			continue;
		}

		printf("server: get connection from client\n");

		/* fork for multiple client connection works seperately */
		if (!fork()) {

			/* Get header from client */

			struct header* hdr;
			hdr = malloc(sizeof(struct header));

			memset(hdr, 0, sizeof(struct header));

			n = recv(newsockfd, hdr, sizeof(struct header), 0);

			if (n < 0) {
				fprintf(stderr, "fail to read");
				close(newsockfd);
				exit(0);
			}

			/* Get data from client */

			char data[hdr->length - 16];
			int datalen = hdr->length - 16;

			n = recv(newsockfd, data, datalen, 0);

			if (n < 0) {
				fprintf(stderr, "fail to read");
				close(newsockfd);
				exit(0);
			}

			if (check_validity(hdr, data) < 0) {
				fprintf(stderr, "violate protocol rules");
				close(newsockfd);
				exit(0);
			}

			printf("data is served successfully!\n");

			char* data_resend = malloc(datalen);

			/* Do operation */
			if (hdr->op == 0)
				data_encrypt(hdr, data, data_resend);
			else if (hdr->op == 1)
				data_decrypt(hdr, data, data_resend);

			/* resend data */

			data_resend[hdr->length - 16 + 1] = '\0';
			printf("%s\n", data_resend);

			int i;
			uint16_t datasum;

			while (i < hdr->length - 16) {
				datasum += (uint16_t)data_resend[i];
				i++;
			}

			/* Only datasum is changed */
			hdr->checksum = (uint16_t)hdr->op + (uint16_t)hdr->keyword + (uint16_t)hdr->length + datasum;

			printf("\nop : %d\nkeyword : %u\nlength : %u\nchecksum : %u\n",
				(int)hdr->op, (unsigned int)hdr->keyword, (unsigned int)hdr->length, (unsigned int)hdr->checksum);

			/* send result to client */

			n = send(newsockfd, hdr, sizeof(struct header), 0);

			if (n < 0) {
				fprintf(stderr, "error at writing to socket\n");
				break;
			}

			n = send(newsockfd, data_resend, hdr->length - 16, 0);

			if (n < 0) {
				fprintf(stderr, "error at writing to socket\n");
				break;
			}

			free(data_resend);

			/* close connection between server and given client */
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}

	return 0;
}