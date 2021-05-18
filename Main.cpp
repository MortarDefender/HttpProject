#include <iostream>
#include "Server.h"

using namespace std;

int main() {
	Server* server = new Server();
	server->serverMain();
	delete server;
	return 0;
}