#include <iostream>
#include "Server.h"

using namespace std;

int main() {
	if (DEBUG)
		cout << "http project" << endl;
	Server* server = new Server();
	server->serverMain();
	delete server;
	return 0;
}
