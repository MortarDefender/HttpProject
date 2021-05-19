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
		unordered_map<string, vector<string>> allowed_methods; // Mapping to each URL a list of Allowed Methods
		unordered_map<int, string> errorCodes;

		string day[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		string month[12] = { "January" , "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
		
		unordered_map<string, string> parse(string data, Protocol& method, string& url, string& version);  // string& method
		string createMessage(string version, int status, string response_body = "");
		string getResponse(string request, string version, int status, string parsedUrl, unordered_map<string, string> variables, Protocol method, string response_body="");
		string getCurrentDate(string format);

		Protocol getProtocol(std::string proc);
		vector<std::string> split(std::string data, std::string delimiter);
		string getDelimiter(string data, string start, string finish="");
		string intToString(int number);

		int urlParser(string url, Protocol method, unordered_map<string, string> variables, string& body); // string method
		string convertUp(string data);
		string getFromFile(string fileName);
		void createFile(string fileName, string content);

		string Get(string version, int status, string response_body);
		string Post(string version, int status, string response_body);
		string Head(string version, int status);
		string Option(string version, string parsed_url);
		string Delete(string filename, string version, string parsedUrl);
		string Put(string fileName, string field, string version, string parsedUrl);
		string Trace(string request, string version, int status);

		inline bool fileExists(const std::string& name) {
			struct stat buffer;
			return (stat(name.c_str(), &buffer) == 0);
		}

	public:
		HttpProtocol();
		string handleRequest(string req);
};
