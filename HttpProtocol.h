#pragma once
#include <vector>
#include <unordered_map>
#include <iostream>
using namespace std;

#define DEBUG false
#define BUFFER_SIZE 1024

enum class Protocol { GET, POST, HEAD, OPTION, DEL, PUT, TRACE, Error };

class HttpProtocol {
	private:
		int request_status; // can be removed and given at each method as a veriable

		unordered_map<int, string> errorCodes;

		string day[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		string month[12] = { "January" , "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
		
		unordered_map<string, string> parse(string data, Protocol& method, string& url, string& version);  // string& method
		string createMessage(string version, int status, string response_body);
		
		Protocol getProtocol(std::string proc);
		vector<std::string> split(std::string data, std::string delimiter);
		
		int getStatus(string url, Protocol method, unordered_map<string, string> variables, string& body); // string method
		string convertUp(string data);
		string getFromFile(string fileName);

		void Get();
		void Post();
		void Head();
		void Option();
		void Delete();
		void Put();
		void Trace();

	public:
		HttpProtocol();
		string handleRequest(string req);
};