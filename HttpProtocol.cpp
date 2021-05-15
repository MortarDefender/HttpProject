#define _CRT_SECURE_NO_WARNINGS
#include "HttpProtocol.h"
#include <iostream>
#include <string.h>
#include <vector>
#include <time.h>
#include <ostream>
#include <fstream>
// using namespace std;

HttpProtocol::HttpProtocol() : request_status(200) {
	// set the http error codes
	errorCodes[100] = "Continue";
	errorCodes[200] = "OK";
	errorCodes[201] = "Created";
	errorCodes[204] = "No Content";
	errorCodes[301] = "Moved Permanently";
	errorCodes[307] = "Temporary Redirect";
	errorCodes[308] = "Permanent Redirect";
	errorCodes[400] = "Bad Request";
	errorCodes[401] = "Unauthorized";
	errorCodes[403] = "Forbidden";
	errorCodes[404] = "Not Found";
	errorCodes[500] = "Internal Server Error";
	errorCodes[501] = "Not Implemented";
	errorCodes[505] = "HTTP Version Not Supported";
}

unordered_map<string, string> HttpProtocol::parse(std::string data, Protocol& method, std::string& url, std::string& version) {
	/* parse http request and return by refrence the method, url and version requested */
	unordered_map<string, string> variables;
	this->request_status = 200;
	vector<string> request_lines = split(data, "\r\n");
	if (request_lines.size() == 0) {  // error
		request_status = 400;         // bad request
		return variables;
	}

	vector<string> first_line = split(request_lines.at(0), " ");
	if (first_line.size() == 0) {     // error
		request_status = 400;         // bad request
		return variables;
	}

	// method = first_line.at(0);
	method = getProtocol(first_line.at(0));
	url = first_line.at(1);
	version = convertUp(first_line.at(2));

	if (version != "HTTP/1.1" && version != "HTTP/1.0") {
		this->request_status = 505;  // HTTP Version Not Supported
		return variables;
	}

	// get the inline / date variables
	if (method == Protocol::POST || method == Protocol::PUT) {
		vector<string> req_parts = this->split(data, "\r\n\r\n");
		for (unsigned int i = 1; i < req_parts.size(); i++) {
			vector<string> variables_parts = this->split(req_parts.at(i), "=");
			if (variables_parts.size() != 2) { // error -> there shold be only 2 parts
				cout << "error: variables_parts size: " << variables_parts.size() << endl;
				continue; // return;
			}
			variables[variables_parts.at(0)] = variables_parts.at(1);
		}
	}
	else if (method == Protocol::GET) {
		vector<string> url_parts = this->split(url, "?");
		if (url_parts.size() != 2) {  // error
			cout << "error: url_parts size: " << url_parts.size() << endl;
			return variables;
		}
		url = url_parts.at(0);
		vector<string> variables_parts = this->split(url_parts.at(1), "&");
		for (auto part : variables_parts) {
			vector<string> vparts = this->split(part, "=");
			if (vparts.size() != 2) { // error -> there shold be only 2 parts
				cout << "error: variables_parts size: " << vparts.size() << endl;
				continue; // return;
			}
			variables[vparts.at(0)] = vparts.at(1);
		}
	}

	if (DEBUG) {
		std::cout << "data: "    << data << std::endl;
		std::cout << "method: "  << first_line.at(0) << std::endl;
		std::cout << "url: "	 << url << std::endl;
		std::cout << "version: " << version << std::endl;
	}
	return variables;
}

int HttpProtocol::getStatus(std::string url, Protocol method, unordered_map<string, string> variables, std::string& body) {
	/* check if the url is valid and check if there has been an accident before hande */
	// get the html file accurding to the url -> add url scheme detection
	// TODO: add ALL methods options according to the "protocol" given

	if (url.find(".ico") != string::npos) {  // favicon exp
		this->request_status = 404;
		body = "";
	}
	else
		body = getFromFile("index.html");

	if (variables.size() != 0)
		cout << "user params: " << endl;
	for (auto part : variables)
		cout << part.first << " = " << part.second << endl;

	return this->request_status;
}

Protocol HttpProtocol::getProtocol(std::string proc) {
	/* convert between the proc to the enum Protocol */
	if (proc == "GET")
		return Protocol::GET;
	else if (proc == "POST")
		return Protocol::POST;
	else if (proc == "HEAD")
		return Protocol::HEAD;
	else if (proc == "OPTION")
		return Protocol::OPTION;
	else if (proc == "DELETE")
		return Protocol::DEL;
	else if (proc == "PUT")
		return Protocol::PUT;
	else if (proc == "TRACE")
		return Protocol::TRACE;
	else
		return Protocol::Error;
}

vector<std::string> HttpProtocol::split(std::string data, std::string delimiter) {
	/* create a vector of strings from the data given using the delimiter */
	string token;
	vector<std::string> res;
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();

	if (!data.find(delimiter)) {
		res.push_back(data);
		return res;
	}

	// divide the data according to the delimiter
	while ((pos_end = data.find(delimiter, pos_start)) != string::npos) {
		token = data.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(data.substr(pos_start));
	return res;
}

string HttpProtocol::createMessage(string version, int status, string response_body) {
	/* create an 'http' message using the version, status and response_body given */
	cout << "created msg" << endl;
	time_t current = time(0);
	tm* timeStruct = localtime(&current);
	
	string code = this->errorCodes[status];
	string month = this->month[timeStruct->tm_mon];
	string day = this->day[timeStruct->tm_wday];

	char response[BUFFER_SIZE];
	// the format of a 'generic' http response message
	string format = std::string("%s %d %s\r\n") +
					"Date: %s, %d %s %d GMT\r\n" +
					"Content-Type: text/html; charset=UTF-8\r\n" + 
					"Content-Length: %d\r\n" +
					"Last-Modified: %s, %d %s %d GMT\r\n" +
					"Server: Apache/1.3.3.7 (Unix) (Red-Hat/Linux)\r\n" + // can be removed or replaced
					"Accept-Ranges: bytes\r\n" +
					"Connection: close\r\n\r\n" +                         // can be removed
					"%s";

	sprintf(response, format.c_str(), version.c_str(), status, code.c_str(), day.c_str(), 
		timeStruct->tm_mday, month.c_str(), 1900 + timeStruct->tm_year, response_body.size(), 
		day.c_str(), timeStruct->tm_mday, month.c_str(), 1900 + timeStruct->tm_year, response_body.c_str());
	if (DEBUG)
		std::cout << "response: " << response << std::endl;
	return string(response);
}

void HttpProtocol::Get() {
	/* create a GET message */
}

void HttpProtocol::Post() {
	/* create a POST message */
}

void HttpProtocol::Head() {
	/* create a HEAD message */
}

void HttpProtocol::Option() {
	/* create a OPTION message */
}

void HttpProtocol::Delete() {
	/* create a DELETE message */
}

void HttpProtocol::Put() {
	/* create a PUT message */
}

void HttpProtocol::Trace() {
	/* create a TRACE message */
}

string HttpProtocol::handleRequest(string req) {
	/* handle the request given and return the needed response */
	Protocol method;
	string url = "", version = "", body = "";
	
	cout << "\r\nstart req: " << req << endl;
	unordered_map<string, string> variables = this->parse(req, method, url, version);
	this->getStatus(url, method, variables, body);
	return this->createMessage(version, this->request_status, body);
}

string HttpProtocol::convertUp(string data) {
	/* create an all upper case string from data */
	string res = "";
	for (auto letter : data)
		res += toupper(letter);
	return res;
}

string HttpProtocol::getFromFile(string fileName) {
	/* get the information from the file given return nullptr if any error accured */
	char value;
	string res = "";

	ifstream infile(fileName);
	if (!infile)
		return "null";

	while (infile.get(value))
		res += value;

	return res;
}