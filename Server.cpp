#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include "Server.h"
#include "HttpProtocol.h"


Server::Server() : socketsCount(0), proc(new HttpProtocol()) {}

int Server::serverMain() {
	/* start the server loop. return 1 if there was an exception or an error, otherwise return 0 */
	WSAData wsaData;
	
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		cout << "Time Server: Error at WSAStartup()\n";
		return 1;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	unsigned long flag = 1;
	if (ioctlsocket(listenSocket, FIONBIO, &flag) != 0)
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;

	if (INVALID_SOCKET == listenSocket) {
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(PORT);

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService))) {
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (SOCKET_ERROR == listen(listenSocket, 5)) {
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	addSocket(listenSocket, Action::LISTEN);

	while (true) {
		fd_set waitRecv;
		FD_ZERO(&waitRecv);

		fd_set waitSend;
		FD_ZERO(&waitSend);

		for (int i = 0; i < MAX_SOCKETS; i++) {
			if (sockets[i].action == Action::SEND)
				FD_SET(sockets[i].id, &waitSend);
			else if ((sockets[i].action == Action::LISTEN) || (sockets[i].action == Action::RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		int nfd = select(0, &waitRecv, &waitSend, NULL, NULL);  // last NULL -> is timeout for when there is no actions to make
		if (nfd == SOCKET_ERROR) {
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return 1;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++) {
			if (FD_ISSET(sockets[i].id, &waitSend)) {
				nfd--;
				if (sockets[i].action == Action::SEND) {
					sendMessage(i);
					break;
				}
			}
			else if (FD_ISSET(sockets[i].id, &waitRecv)) {
				nfd--;
				switch (sockets[i].action) {
					case Action::LISTEN:
						acceptConnection(i);
						break;

					case Action::RECEIVE:
						receiveMessage(i);
						break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	closesocket(listenSocket);
	WSACleanup();
	return 0;
}

bool Server::addSocket(SOCKET id, Action what) {
	/* add socket to the array, and convert the sotcket to a non blocking */
	unsigned long flag = 1;
	if (ioctlsocket(id, FIONBIO, &flag) != 0)
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;

	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].action == Action::EMPTY) {
			sockets[i].id = id;
			sockets[i].action = what;
			sockets[i].len = 0;
			socketsCount++;
			return true;
		}
	}
	return false;
}

void Server::removeSocket(int index) {
	/* remove the socket with the index given */
	sockets[index].action = Action::EMPTY;
	socketsCount--;
}

void Server::acceptConnection(int index) {
	/* accept connection with a socket -> add the socket to the array */
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*) & from, &fromLen);
	if (INVALID_SOCKET == msgSocket) {
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}

	if (addSocket(msgSocket, Action::RECEIVE) == false) {
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
}

void Server::receiveMessage(int index) {
	/* revceive a message from a socket withe the index given */
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv || bytesRecv == 0) {
		if (SOCKET_ERROR == bytesRecv)
			cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
	}
	else {
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		sockets[index].len += bytesRecv;

		// command handler -> protocol HTTP
		if (sockets[index].len > 0) {	
			sockets[index].action = Action::SEND;
			cout << "\r\nstart buffer: " << sockets[index].buffer << endl;
		}
	}
}

void Server::sendMessage(int index) {
	/* send a message to the socket with the index given */
	cout << "send message" << endl;
	int bytesSent = 0;
	char sendBuff[BUFFER_SIZE];  // 255

	// command handler -> handler class HTTP
	SOCKET msgSocket = sockets[index].id;
	string res = proc->handleRequest(sockets[index].buffer);
	cout << "\r\nstart res: " << res << endl;
	strcpy(sendBuff, res.c_str());
	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent) {
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	sockets[index].action = Action::IDLE;
}
