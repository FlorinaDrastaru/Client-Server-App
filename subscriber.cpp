


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "helpers.h"
#include "structures.h"

void usage(char *file) {	
	fprintf(stderr, "Usage: %s <id_client> <server_address> <server_port>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, fdmax, flag = 1;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	fd_set read_fds;
	fd_set tmp_fds;
	// verific numarul de parametri din comanda de rulare
	if (argc < 4) {
		usage(argv[0]);
	}

	if (strlen(argv[1]) > 10) {
		fprintf(stderr, "Client ID is maximum 10");
		exit(0);
	}

	// golesc multimea de descriptori
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// initializez socketul
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	// completez cu datele despre socket, pt conexiunea la server
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	// setez file descriptorii
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

    // trimit id-ul la server
    n = send(sockfd, argv[1], strlen(argv[1]), 0);
    DIE(n < 0, "send");

	// dezactivez algoritmul lui Neagle
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

	msg_for_server msg_for_server;
	memset(&msg_for_server, 0, sizeof(msg_for_server));

	while (1) {
  		// se citeste de la tastatura
		tmp_fds = read_fds;
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			// clientul se deconecteaza cand primeste exit
			if (strncmp(buffer, "exit", 4) == 0) {
			    break;
			}
			char *remaining_string;
			remaining_string = strtok(buffer, " ");
			// verific daca primul token din buffer este comanda subscribe
			if (strncmp(remaining_string, "subscribe", 9) == 0) { 
				// salvez comanda			
				msg_for_server.type = 1;			
				//salvez topicu
				remaining_string = strtok(NULL," ");							
				strncpy(msg_for_server.topic, remaining_string, strlen(remaining_string));
				if (remaining_string == NULL) {
					fprintf(stderr, "Missing topic\n");
					exit(0);
				}
				
				remaining_string = strtok(NULL, " ");
				if (remaining_string == NULL) {
					fprintf(stderr, "Missing sf\n");
					exit(0);
				}				
				int sf = atoi(remaining_string);
				// salvez sf
				msg_for_server.sf = sf;				
				if (sf != 0 && sf != 1) {
					fprintf(stderr, "Incorrect value for SF\n");
					exit(0);
				}
			} else if (strncmp(remaining_string, "unsubscribe", 11) == 0) {
				// salvez comanda
				msg_for_server.type = 0;
				// salvez topic
				remaining_string = strtok(NULL, " ");				
				strncpy(msg_for_server.topic, remaining_string, strlen(remaining_string));
				if (remaining_string == NULL) {
					fprintf(stderr, "Missing topic\n");
					exit(0);
				}				
			} else {
				fprintf(stderr, "wrong command");
				exit(0);
			}
			// trimit mesaj la server
            n = send(sockfd, (char*) &msg_for_server, sizeof(msg_for_server), 0);
			DIE(n < 0, "send");
			if (msg_for_server.type == 1) {
				printf("subscribed %s\n", msg_for_server.topic);
			} else if (msg_for_server.type ==0) {
				printf("unsubscribed %s\n", msg_for_server.topic);
			}
		}
		// cazul in care s-a primit mesaj de la server
		if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			// primesc mesajul
			n = recv(sockfd, buffer, sizeof(msg_for_tcp), 0);
			if (n > 0) {
				msg_for_tcp msg;
				memset(&msg, 0, sizeof(msg_for_tcp));
				memcpy(&msg, buffer, sizeof(msg));			
				char str_type[20];
				memset(str_type, 0, sizeof(str_type));
				if (msg.data_type == 0) {
					strcpy(str_type, "INT");
				} else if (msg.data_type == 1) {
					strcpy(str_type, "SHORT_REAL");
				} else if (msg.data_type == 2) {
					strcpy(str_type, "FLOAT");
				} else if (msg.data_type == 3) {
					strcpy(str_type, "STRING");
				}
				// afisez mesajul de la server
				printf("%s:%hu - %s - %s - %s\n", msg.ip, msg.portno, msg.topic, str_type, msg.data);
			} else {
				break;
			}
		}
		
	}
	close(sockfd);
	return 0;
}



