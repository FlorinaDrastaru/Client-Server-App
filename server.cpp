#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <netinet/tcp.h>
#include "helpers.h"
#include "structures.h"



void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}


int main(int argc, char *argv[])
{	
	int newsockfd, portno, flag = 1, sock_udp, sock_tcp, new_sock, n, i, ret;
	char buffer[BUFLEN];
	struct sockaddr_in udp_addr, tcp_addr, new_addr;
	socklen_t socklen = sizeof(sockaddr);
	socklen_t sockudplen = sizeof(udp_addr);
	struct client clients[MAX_CLIENTS];
	int clients_num = 0;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0); // se asteapta conexiuni
	DIE(sock_tcp < 0, "socket");
	sock_udp = socket(PF_INET, SOCK_DGRAM, 0); // se asteapta conexiuni
  	DIE(sock_udp < 0, "udp");
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &tcp_addr, 0, sizeof(tcp_addr));
	memset((char *) &udp_addr, 0, sizeof(udp_addr));
	memset((char *) &new_addr, 0, sizeof(new_addr));
	// completez datele pt conexiuni
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(portno);
	tcp_addr.sin_addr.s_addr = INADDR_ANY;
	udp_addr.sin_family = AF_INET;
  	udp_addr.sin_addr.s_addr = INADDR_ANY;
  	udp_addr.sin_port = htons(portno);
	ret = bind(sock_tcp, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	ret = bind(sock_udp, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
  	DIE(ret < 0, "bind");
	ret = listen(sock_tcp, MAX_CLIENTS);
	DIE(ret < 0, "unable to listen");
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
	
	fdmax = std::max(sock_tcp, sock_udp);

    // declar si initializez structurile de mesaj pe care le folosesc 
	// pentru a realiza transferul de date intre server si clienti
	msg_for_server recv_msg;
	msg_for_tcp msg_to_sent;
	msg_from_udp recv_msg_udp;
	memset(&recv_msg_udp, 0, sizeof(msg_from_udp));
	memset(&recv_msg, 0, sizeof(msg_for_server));
	memset(&msg_to_sent, 0, sizeof(msg_for_tcp));

	int exit_now = 0;

	while (!exit_now) {
		tmp_fds = read_fds; 		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				// daca primesc comanda de la tastatura
				if (i == STDIN_FILENO) {
					fgets(buffer, BUFLEN - 1, stdin);
					if (strcmp(buffer, "exit\n") == 0) {
                        exit_now = 1;
                        break;
                    } else {
                        printf("Unaccepted command\n");
                    }
				} else if (i == sock_udp) {
					memset(buffer, 0, BUFLEN);
					n = recvfrom(sock_udp, buffer, BUFLEN, 0, (sockaddr*) &udp_addr, &sockudplen);
					DIE(n < 0, "recv");
					// imi salvez datele primite intr-o structura 
					memcpy(&recv_msg_udp, buffer, sizeof(recv_msg_udp));
					// completez portul si ip-ul de pe care a venit mesajul
					msg_to_sent.portno = ntohs(udp_addr.sin_port);
					strcpy(msg_to_sent.ip, inet_ntoa(udp_addr.sin_addr));

					// in functie de tipul de date primit, convertesc datele din format binar
					if (recv_msg_udp.data_type == 0) {
						long long payload;						
						payload = ntohl(*(uint32_t*)(recv_msg_udp.data + 1));
						if (recv_msg_udp.data[0] == 1) {
                			payload = payload * (-1);
              			} 								
						msg_to_sent.data_type = 0;
						sprintf(msg_to_sent.data, "%lld", payload);
					} else if (recv_msg_udp.data_type == 1) { 
						double payload;
						payload = ntohs(*(uint16_t*)(recv_msg_udp.data));
            			payload = payload / 100;
            			sprintf(msg_to_sent.data, "%.2f", payload);
            			msg_to_sent.data_type = 1;
					} else if (recv_msg_udp.data_type == 2) { 
						double payload;
						payload = ntohl(*(uint32_t*)(recv_msg_udp.data + 1));
            			payload /= pow(10, recv_msg_udp.data[5]);
           				if (recv_msg_udp.data[0]) {
                			payload = payload * (-1);
            			}
						sprintf(msg_to_sent.data, "%lf", payload);
						msg_to_sent.data_type = 2;
					} else if (recv_msg_udp.data_type == 3) {
						msg_to_sent.data_type = 3;
						strcpy(msg_to_sent.data, recv_msg_udp.data);
					} 		
					msg_to_sent.topic[51] = 0;
					std::vector<subscription>::iterator it;
					// trimit mesaje clientilor tcp in functie de topicurile la care sunt abonati
					for(int t = 0; t < clients_num; t++) {
						for (it = clients[t].subscriptions.begin(); it != clients[t].subscriptions.end(); it++) {
							if (strncmp((*it).topic, recv_msg_udp.topic, strlen(recv_msg_udp.topic)) == 0) {
								strcpy(msg_to_sent.topic, recv_msg_udp.topic);
								// verific daca clientul e conectat
								if (clients[t].connected) {
									n = send(clients[t].socket, (char*) &msg_to_sent, sizeof(msg_for_tcp), 0);
									DIE(n < 0, "couldn't send message");
								// daca clientul e offline dar are sf activat, salvez mesajele intr-o coada
								} else if (clients[t].connected == false && (*it).sf == 1) {
									clients[t].offlineMsg.push(msg_to_sent);
								}
								break;
							}
						}
					
					}
				} else if (i == sock_tcp) {	// cerere noua de conexiune de pe socketul tcp								
					newsockfd = accept(sock_tcp, (struct sockaddr *) &new_addr, &socklen);
					DIE(newsockfd < 0, "accept");
					setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					bool found_id = false;
					int pos = -1;
					memset(buffer, 0, BUFLEN);
					// primesc id-ul clientului tcp
					n = recv(newsockfd, buffer, BUFLEN, 0);
					DIE(n < 0, "no id received");
					// verific daca id-ul exista in vectorul de clienti
					for (int t = 0; t < clients_num; t++) {
						if (strcmp(clients[t].id, buffer) == 0) {
							found_id = true;
							pos = t;
							if (clients[t].connected == true) {
								printf("The id is in use\n");
								close(newsockfd);
							}
						}
					}
					// daca clientul nu este nou, si doar se reconecteaza
					if (found_id == true) {
						clients[pos].connected = true;
						clients[pos].socket = newsockfd;
						if (!clients[pos].offlineMsg.empty()) {
							while (!clients[pos].offlineMsg.empty()) {
								msg_for_tcp offline_msg = clients[pos].offlineMsg.front();
								clients[pos].offlineMsg.pop();
								n = send(clients[pos].socket, (char*) &offline_msg, sizeof(msg_for_tcp), 0);
								DIE(n < 0, "send");
							}
						}
					} else {											
						memcpy(clients[clients_num].id, buffer, strlen(buffer));
						clients[clients_num].connected = true;						
						clients[clients_num].socket = newsockfd;
						clients_num++;
					}					
					printf("New client (%s) connected from %s : %d.\n", buffer,
						   inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));
				} else {
					// s-au primit date pe unul din socketii de client tcp
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");
					int poz;
					// caut clientul 						
					for (int k = 0; k < clients_num; k++) {
						if (clients[k].socket == i) {
							poz = k;
						}
					}
					// daca conexiunea s-a inchis
					if (n == 0) {
						// clientul va fi offline						
						clients[poz].connected = false;		
						printf("Client (%s) disconnected\n", clients[poz].id);
						close(i);						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else {
						// copiez bufferul in structura de mesaj 
						memcpy(&recv_msg, buffer, sizeof(recv_msg));	
						subscription newSubscription;
						memset(&newSubscription, 0, sizeof(subscription));
						// creez o subscriptie cu ajutorul datelor din buffer, 
						// convertite ulterior intr-o structura de tip mesaj
						strcpy(newSubscription.topic, recv_msg.topic);
						newSubscription.sf = recv_msg.sf; 
						bool isNew = true;		
						//primesc comanda subscribe de la clientul tcp
						if (recv_msg.type == 1) { 
							std::vector<subscription>::iterator itr;
							// daca clientul nu mai are alte subscriptii, o adaug pe cea primita
							if (clients[poz].subscriptions.size() == 0) {
								clients[poz].subscriptions.push_back(newSubscription);
								// in caz contrar verific mai intai daca exista topicul si daca da, actualizez doar sf
							} else {
								for (itr = clients[poz].subscriptions.begin(); itr != clients[poz].subscriptions.end(); itr++) {
									if (strcmp((*itr).topic, newSubscription.topic) == 0) {
										isNew = false;
										(*itr).sf = newSubscription.sf;
										break;
									} 
								}
								// daca topicul nu exista, adaug subscriptia
								if (isNew == true) {
									clients[poz].subscriptions.push_back(newSubscription);
								}
							}
						} else if (recv_msg.type == 0) { // primesc comanda unsubscribe de la clientul tcp							
							int p = -1;
							std::vector<subscription>::iterator it;
							// caut in subscribtiile clientului topicul primit
							for (it = clients[poz].subscriptions.begin(); it != clients[poz].subscriptions.end(); it++) {
								if (strncmp((*it).topic, newSubscription.topic, strlen((*it).topic)) == 0) {
									p = std::distance(clients[poz].subscriptions.begin(), it);								
								}
							}
							// tratez cazul in care clientul nu e abonat la acel topic
							if (p == -1) {
								fprintf(stderr, "not an exiting topic in subscriptions\n");
								printf("not an existing topic\n");
								exit(0);
							// daca clientul e abonat la acel topic, sterg topicul din vectorul de subscriptii
							} else {
								clients[poz].subscriptions.erase(clients[poz].subscriptions.begin() + p);
							}														
						}
					}
				}
			}
		}
	}

	close(sock_tcp);
	close(new_sock);
	close(sock_udp);

	return 0;
}