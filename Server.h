#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include "HttpProtocol.h"

// Constants
#define PORT 27015
#define MAX_SOCKETS 60

enum class Action { EMPTY, LISTEN, RECEIVE, IDLE, SEND };

struct SocketState {
	SOCKET id;
	Action action;
	char buffer[BUFFER_SIZE];
	int len;
};

class Server {
	private:
		int socketsCount;
		HttpProtocol* proc;
		struct SocketState sockets[MAX_SOCKETS] = { 0 };

		bool addSocket(SOCKET id, Action what);
		void removeSocket(int index);
		void acceptConnection(int index);
		void receiveMessage(int index);
		void sendMessage(int index);
	public:
		Server();
		int serverMain();
};
