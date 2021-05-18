//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#include <iostream>
//using namespace std;
//#pragma comment(lib, "Ws2_32.lib")
//#include <winsock2.h> 
//#include <string.h>
//#include <iomanip>
//#define TIME_PORT 27015
//
//
//// debug only client -> real client will be a web browser
//int main() {
//	WSAData wsaData;
//	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
//		cout << "Time Client: Error at WSAStartup()\n";
//		return 1;
//	}
//
//	SOCKET connSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (INVALID_SOCKET == connSocket) {
//		cout << "Time Client: Error at socket(): " << WSAGetLastError() << endl;
//		WSACleanup();
//		return 1;
//	}
//
//	sockaddr_in server;
//	server.sin_family = AF_INET;
//	server.sin_addr.s_addr = inet_addr("127.0.0.1");
//	server.sin_port = htons(TIME_PORT);
//
//	if (SOCKET_ERROR == connect(connSocket, (SOCKADDR*)&server, sizeof(server))) {
//		cout << "Time Client: Error at connect(): " << WSAGetLastError() << endl;
//		closesocket(connSocket);
//		WSACleanup();
//		return 1;
//	}
//
//	int bytesSent = 0, bytesRecv = 0, option = 0;
//	char sendBuff[255];
//	char recvBuff[255];
//
//	while (option != 3) {
//		// change according to the protocol (HTTP)
//		cout << "\nPlease choose an option:\n";
//		cout << "1 - Get current time.\n";
//		cout << "2 - Get current time in seconds.\n";
//		cout << "3 - Exit.\n";
//		cout << ">> ";
//
//		cin >> option;
//
//		if (option == 1)
//			strcpy(sendBuff, "TimeString");
//		else if (option == 2)
//			strcpy(sendBuff, "SecondsSince1970");
//		else if (option == 3)
//			strcpy(sendBuff, "Exit");
//
//		
//		bytesSent = send(connSocket, sendBuff, (int)strlen(sendBuff), 0);
//		if (SOCKET_ERROR == bytesSent) {
//			cout << "Time Client: Error at send(): " << WSAGetLastError() << endl;
//			closesocket(connSocket);
//			WSACleanup();
//			return 1;
//		}
//		cout << "Time Client: Sent: " << bytesSent << "/" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";
//
//		if (option == 1 || option == 2) {
//			bytesRecv = recv(connSocket, recvBuff, 255, 0);
//			if (SOCKET_ERROR == bytesRecv) {
//				cout << "Time Client: Error at recv(): " << WSAGetLastError() << endl;
//				closesocket(connSocket);
//				WSACleanup();
//				return 1;
//			}
//			if (bytesRecv == 0) {
//				cout << "Server closed the connection\n";
//				return 0;
//			}
//
//			recvBuff[bytesRecv] = '\0'; //add the null-terminating to make it a string
//			cout << "Time Client: Recieved: " << bytesRecv << " bytes of \"" << recvBuff << "\" message.\n";
//		}
//		else if (option == 3) {
//			// Closing connections and Winsock.
//			cout << "Time Client: Closing Connection.\n";
//			closesocket(connSocket);
//			WSACleanup();
//			return 0;
//		}
//	}
//	return 0;
//}