#include <cstring>
#include <bits/stdc++.h>

// structura pentru mesaj trimis de la server catre clientii tcp
struct msg_for_tcp {
	char ip[16];
	uint16_t portno;
	char topic[51];
	int data_type;
	char data[1501];
};

// structura pentru mesaj trimis de la clientul tcp catre server
struct msg_for_server {
	int type;
	char topic[51];
	int sf;
};

// structura pentru abonamentul fiecarui client
struct subscription {
	char topic[51];
	int sf;
};

// structura pentru client
struct client {
	char id[11];
	int socket;
	bool connected;
	std::vector<subscription> subscriptions;
	std::queue<msg_for_tcp> offlineMsg;

};

// structura pentru mesaj trimis de la clientul udp serverului
struct msg_from_udp {
    char topic[50];
    uint8_t data_type;
    char data[1501];
};


