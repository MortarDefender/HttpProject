#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include "Server.h"



// change to a local array -> or a field in a class
struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


void main() {
	WSAData wsaData;
	
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (INVALID_SOCKET == listenSocket) {
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(TIME_PORT);

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService))) {
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	if (SOCKET_ERROR == listen(listenSocket, 5)) {
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	while (true) {
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++) {
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++) {
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd = select(0, &waitRecv, &waitSend, NULL, NULL);  // last NULL -> is timeout for when there is no actions to make
		if (nfd == SOCKET_ERROR) {
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++) {
			if (FD_ISSET(sockets[i].id, &waitRecv)) {
				nfd--;
				switch (sockets[i].recv) {
					case LISTEN:
						acceptConnection(i);
						break;

					case RECEIVE:
						receiveMessage(i);
						break;
				}
			}
			else if (FD_ISSET(sockets[i].id, &waitSend)) {
				nfd--;
				if (sockets[i].send == SEND) {
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what) {
	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].recv == EMPTY) {
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return true;
		}
	}
	return false;
}

void removeSocket(int index) {
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index) {
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*) & from, &fromLen);
	if (INVALID_SOCKET == msgSocket) {
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	// Set the socket to be in non-blocking mode.
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;

	if (addSocket(msgSocket, RECEIVE) == false) {
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;  // needed ??
}

void receiveMessage(int index) {
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv || bytesRecv == 0) {
		if (SOCKET_ERROR == bytesRecv)
			cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;  // needed ??
	}
	else {
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		// command handler -> replace according to the protocol needed (HTTP)
		if (sockets[index].len > 0) {
			if (strncmp(sockets[index].buffer, "TimeString", 10) == 0) {
				sockets[index].send = SEND;
				sockets[index].sendSubType = SEND_TIME;
				memcpy(sockets[index].buffer, &sockets[index].buffer[10], sockets[index].len - 10);
				sockets[index].len -= 10;
				return;  // needed ??
			}
			else if (strncmp(sockets[index].buffer, "SecondsSince1970", 16) == 0) {
				sockets[index].send = SEND;
				sockets[index].sendSubType = SEND_SECONDS;
				memcpy(sockets[index].buffer, &sockets[index].buffer[16], sockets[index].len - 16);
				sockets[index].len -= 16;
				return;  // needed ??
			}
			else if (strncmp(sockets[index].buffer, "Exit", 4) == 0) {
				closesocket(msgSocket);
				removeSocket(index);
				return;  // needed ??
			}
		}
	}

}

void sendMessage(int index) {
	int bytesSent = 0;
	char sendBuff[255];

	// command handler -> replace with the needed handler class or function (HTTP)
	SOCKET msgSocket = sockets[index].id;
	if (sockets[index].sendSubType == SEND_TIME) {
		time_t timer;
		time(&timer);
		strcpy(sendBuff, ctime(&timer));
		sendBuff[strlen(sendBuff) - 1] = 0;
	}
	else if (sockets[index].sendSubType == SEND_SECONDS) {
		time_t timer;
		time(&timer);
		itoa((int)timer, sendBuff, 10);
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent) {
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Time Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";
	sockets[index].send = IDLE;
}
