#pragma once
#include <vector>
#include <unordered_map>
#include <iostream>
using namespace std;

#define PATH "C:\\temp"
#define DEBUG true
#define FULL_DEBUG false
#define BUFFER_SIZE 4096

enum class Protocol { GET, POST, HEAD, OPTIONS, DEL, PUT, TRACE, Error };

class HttpProtocol {
	private:
		int request_status;
		string error_response = "";
		string error_extention = "";
		unordered_map<int, string> errorCodes;
		unordered_map<string, vector<string>> allowed_methods; // Mapping to each URL a list of Allowed Methods

		string day[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		string month[12] = { "January" , "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
		
		unordered_map<string, string> parse(string data, Protocol& method, string& url, string& version, string& body);
		unordered_map<string, string> getVariables(string data, string& url, Protocol method);
		string createMessage(string version, int status, string response_body = "", string optional = "");
		string getResponse(string request, string version, int status, string parsedUrl, unordered_map<string, string> variables, Protocol method, string response_body="");
		string getCurrentDate(string format);

		Protocol getProtocol(std::string proc);
		vector<std::string> split(std::string data, std::string delimiter);
		string getDelimiter(string data, string start, string finish="");
		string intToString(int number);
		string stringReplace(string data, char old, char newC);
		bool isAllowed(string url, Protocol method);

		int urlParser(string url, Protocol& method, unordered_map<string, string> variables, string& body);
		string convertUp(string data);
		string getFromFile(string fileName);
		void createFile(string fileName, string content);
		string getErrorHtml(int status, string error_response, string error_extention="");

		string Get(string version, int status, string response_body);
		string Post(string version, int status, string response_body);
		string Head(string version, int status);
		string Options(string version, string parsed_url);
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
