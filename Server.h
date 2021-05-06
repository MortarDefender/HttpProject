#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>

// Constants
#define TIME_PORT 27015
#define MAX_SOCKETS 60
#define EMPTY 0
#define LISTEN 1
#define RECEIVE 2
#define IDLE 3
#define SEND 4
#define SEND_TIME 1
#define SEND_SECONDS 2

// Change struct
struct SocketState {
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[128];
	int len;
};


bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);