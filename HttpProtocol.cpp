#define _CRT_SECURE_NO_WARNINGS
#include "HttpProtocol.h"
#include <iostream>
#include <string.h>
#include <vector>
#include <time.h>
#include <ostream>
#include <fstream>

HttpProtocol::HttpProtocol() : request_status(200) {
	// set the http error codes
	errorCodes[100] = "Continue";
	errorCodes[200] = "OK";
	errorCodes[201] = "Created";
	errorCodes[202] = "Accepted";
	errorCodes[204] = "No Content";
	errorCodes[301] = "Moved Permanently";
	errorCodes[307] = "Temporary Redirect";
	errorCodes[308] = "Permanent Redirect";
	errorCodes[400] = "Bad Request";
	errorCodes[401] = "Unauthorized";
	errorCodes[403] = "Forbidden";
	errorCodes[404] = "Not Found";
	errorCodes[414] = "URI Too Long";
	errorCodes[405] = "Method Not Allowed";
	errorCodes[500] = "Internal Server Error";
	errorCodes[501] = "Not Implemented";
	errorCodes[505] = "HTTP Version Not Supported";

	/* Initialize allowed methods for the main site */
	allowed_methods["/*"] = { "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE" };
	allowed_methods["/"] = { "OPTIONS", "GET", "HEAD", "POST", "TRACE" }; // index page
}

unordered_map<string, string> HttpProtocol::parse(std::string data, Protocol& method, std::string& url, std::string& version, std::string& body) {
	/* parse http request and return by refrence the method, url and version requested */
	this->request_status = 200;
	unordered_map<string, string> variables;
	vector<string> request_lines = split(data, "\r\n");
	if (request_lines.size() == 0) {     // error
		body = this->getErrorHtml(400);  // bad request
		return variables;
	}

	vector<string> first_line = split(request_lines.at(0), " ");
	if (first_line.size() == 0) {        // error
		body = this->getErrorHtml(400);  // bad request
		return variables;
	}

	method = getProtocol(first_line.at(0));
	url = first_line.at(1);
	version = convertUp(first_line.at(2));

	if (version != "HTTP/1.1" && version != "HTTP/1.0") {
		body = this->getErrorHtml(505);  // HTTP Version Not Supported
		return variables;
	}

	if (url.size() > 100) {
		body = this->getErrorHtml(414);  // URI Too Long
		return variables;
	}

	if (FULL_DEBUG) {
		std::cout << "data: "    << data << std::endl;
		std::cout << "method: "  << first_line.at(0) << std::endl;
		std::cout << "url: "	 << url << std::endl;
		std::cout << "version: " << version << std::endl;
	}

	// get the inline / date variables
	variables = this->getVariables(data, url, method);
	this->urlParser(url, method, variables, body);
	return variables;
}

unordered_map<string, string> HttpProtocol::getVariables(string data, string& url, Protocol method) {
	/* return a map of string to string of vaeiables type to variables from the data, and method given */
	unordered_map<string, string> variables;
	
	if (method == Protocol::Error) { // function such as PATCH and CONNECT are not allowed
		request_status = 501;        // Not Implemented
		return variables;
	}
	else if (method == Protocol::POST) {
		if (data.find("Content-Type: application/x-www-form-urlencoded") != string::npos) {
			vector<string> req_parts = this->split(data, "\r\n\r\n");
			for (unsigned int i = 1; i < req_parts.size(); i++) {
				vector<string> variables_parts = this->split(req_parts.at(i), "=");
				if (variables_parts.size() != 2) { // error -> there shold be only 2 parts
					if (DEBUG)
						cout << "error: variables_parts size: " << variables_parts.size() << endl;
					continue;
				}
				if (variables.find(variables_parts.at(0)) == variables.end())
					variables[variables_parts.at(0)] = variables_parts.at(1);
				else { // two or more variables with the same variable type
					request_status = 400;         // bad request
					return variables;
				}
			}
		}
		else if (data.find("Content-Type: multipart/form-data;") != string::npos) {
			string req_parts = this->getDelimiter(data, "\r\n\r\n");
			string delimiter = this->getDelimiter(data, "boundary=", "\"");
			vector<string> variables_parts = this->split(req_parts, "--" + delimiter);
			for (auto part : variables_parts) {
				// string content_disposition = this->getDelimiter(part, "Content-Disposition: ", ";");
				string var_name = this->getDelimiter(part, "name=\"", "\"");
				string file_name = this->getDelimiter(part, "filename=\"", "\"");
				string content = this->getDelimiter(part, "\r\n");

				// add identifiaction according to the file name given
				variables[var_name] = content;
			}
		}
		else { // The Content Type is not recognizable
			request_status = 400;         // bad request
			return variables;
		}
	}
	else if (method == Protocol::PUT || method == Protocol::DEL) {
		vector<string> req_parts = this->split(data, "\r\n\r\n");
		vector<string> url_parts = this->split(url, "/");  // ignored / for some reason
		string fileName = url_parts.at(url_parts.size() - 1);
		if (fileName.find(".") != string::npos) {
			if (fileName.find(".txt") != string::npos)
				variables["file name"] = fileName;
			else { // the file wanted to update or create is not a txt file
				request_status = 403;     // Forbidden
				return variables;
			}
		}
		else {                            // error
			request_status = 400;         // bad request
			return variables;
		}
		
		if (method == Protocol::PUT) {
			if (req_parts.size() == 2) {
				string content = req_parts.at(1);
				variables["content"] = stringReplace(this->getDelimiter(content, "content="), '+', ' ');
			}
			else {                      // error
				request_status = 400;   // bad request
				return variables;
			}
		}
	}
	else if (method == Protocol::GET) {
		vector<string> url_parts = this->split(url, "?");
		if (url_parts.size() != 2) {  // error
			if (DEBUG)
				cout << "error: url_parts size: " << url_parts.size() << endl;
			return variables;
		}
		url = url_parts.at(0);
		vector<string> variables_parts = this->split(url_parts.at(1), "&");
		for (auto part : variables_parts) {
			vector<string> vparts = this->split(part, "=");
			if (vparts.size() != 2) { // error -> there shold be only 2 parts
				if (DEBUG)
					cout << "error: variables_parts size: " << vparts.size() << endl;
				continue;
			}
			if (variables.find(vparts.at(0)) == variables.end())
				variables[vparts.at(0)] = vparts.at(1);
			else { // two or more variables with the same variable type
				request_status = 400;   // bad request
				return variables;
			}
		}

		vector<string> url_parts2 = this->split(url, "/");
		string fileName = url_parts2.at(url_parts2.size() - 1);
		if (fileName.find(".") != string::npos) {
			if (fileName.find(".txt") != string::npos)
				variables["file name"] = fileName;
			else { // the file wanted to update or create is not a txt file
				request_status = 403;     // Forbidden
				return variables;
			}
		}
	}
	return variables;
}

int HttpProtocol::urlParser(std::string url, Protocol& method, unordered_map<string, string> variables, std::string& body) {
	/* check if the url is valid and check if there has been an accident before hande */
	
	if (method == Protocol::GET && variables.find("method") != variables.end())
		method = this->getProtocol(variables.at("method"));

	if (this->request_status != 200 && body == "")
		body = this->getErrorHtml(this->request_status);
	else if (method != Protocol::PUT && this->allowed_methods.find(url) == this->allowed_methods.end())
		body = this->getErrorHtml(404);
	else if (method != Protocol::PUT && !this->isAllowed(url, method))
		body = this->getErrorHtml(405);
	else if (url == "/")
		body = getFromFile("index.html");
	else if (this->allowed_methods.find(url) != this->allowed_methods.end() && url.find('.') != string::npos)
		body = getFromFile(url.substr(1));

	if (DEBUG) {
		if (variables.size() != 0)
			cout << "user params: " << endl;
		for (auto part : variables)
			cout << part.first << " = " << part.second << endl;
	}
	return this->request_status;
}

Protocol HttpProtocol::getProtocol(std::string proc) {
	/* convert between the string given to the enum Protocol */
	proc = convertUp(proc);
	if (proc == "GET")
		return Protocol::GET;
	else if (proc == "POST")
		return Protocol::POST;
	else if (proc == "HEAD")
		return Protocol::HEAD;
	else if (proc == "OPTIONS")
		return Protocol::OPTIONS;
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

string HttpProtocol::getDelimiter(string data, string start, string finish) {
	/* find and retrieve the string from the end of start to the starting position of finish */
	string res = "";
	int start_loc = data.find(start);
	if (start_loc == string::npos) // didnt found return nothing
		return "";

	start_loc += start.size();
	if (finish == "") {
		for (; start_loc != data.size(); start_loc++)
			res += data.at(start_loc);
	}
	else {
		for(; start_loc != data.size() && data.at(start_loc) != finish.at(0); start_loc++)
			res += data.at(start_loc);
	}
	return res;
}

string HttpProtocol::intToString(int number) {
	/* convert int to string */
	string res = "", reverse_res = "";
	char temp = number % 10;
	number /= 10;
	for (; number > 0; temp = number % 10, number /= 10)
		res += temp + '0';
	res += temp + '0';

	for (int n = res.length() - 1; n >= 0; n--)
		reverse_res.push_back(res[n]);

	return reverse_res;
}

string HttpProtocol::stringReplace(string data, char old, char newC) {
	/* replace a char with another char in the data given */
	string res = "";
	
	for (auto letter : data) {
		if (letter == old)
			res += newC;
		else
			res += letter;
	}
	return res;
}

string HttpProtocol::getErrorHtml(int status) {  // TODO: create a better error html
	/* return a string containing the status code and the status information */
	this->request_status = status;
	return "<html><body style='background-color: #0575E6'><h1 style='text-align: center;'>" + intToString(status) + " - " + this->errorCodes[status] + "</h1></body</html>";
}

string HttpProtocol::getResponse(string request, string version, int status, string parsedUrl, unordered_map<string, string> variables, Protocol method, string response_body) {
	/* create the response according to the method requested */

	// check if there was an error code before, if there was than show the response body, Error HTML, to the user
	if (status != 200)
		return this->Get(version, status, response_body);

	// Otherwise, continue to preforme the method requested
	switch (method) {
		case Protocol::GET:
			return this->Get(version, status, response_body);
		case Protocol::POST:
			return this->Post(version, status, response_body);
		case Protocol::HEAD:
			return this->Head(version, status);
		case Protocol::OPTIONS:
			return this->Options(version, parsedUrl);
		case Protocol::DEL:
			if (variables.find("file name") != variables.end())
				return this->Delete(variables.at("file name"), version, parsedUrl);
			else // error in variabels
				return this->Get(version, 400, this->getErrorHtml(400)); // Bad Request
		case Protocol::PUT: {
			if (variables.find("file name") != variables.end() && variables.find("content") != variables.end())
				return this->Put(variables.at("file name"), variables.at("content"), version, parsedUrl);
			else // error in variabels
				return this->Get(version, 400, this->getErrorHtml(400)); // Bad Request
		}
		case Protocol::TRACE:
			return this->Trace(request, version, status);
		case Protocol::Error: // error in the protocol
			return this->Get(version, 400, this->getErrorHtml(400));     // page not found
		default:
			break;
	}
	return this->Get(version, 404, this->getErrorHtml(404));             // page requested not found
}

string HttpProtocol::createMessage(string version, int status, string response_body, string optional) {
	/* create an 'http' message using the version, status and response_body given */
	if (DEBUG)
		cout << "created msg" << endl;
	string code = this->errorCodes[status];

	char response[BUFFER_SIZE];
	// the format of a 'generic' http response message
	string format = std::string("%s %d %s\r\n") +
					"%s" + // Date: %s, %d %s %d GMT\r\n
					"Content-Type: text/html; charset=UTF-8\r\n" +
					"Content-Length: %d\r\n" +
					"%s" + // Last-Modified: %s, %d %s %d GMT\r\n
					"Server: Http Server/2.0 (Windows)\r\n"
					"Accept-Ranges: bytes\r\n" +
					"%s" + // optional add ons
					"Connection: close\r\n\r\n" +   // can be removed
					"%s";
	// Content-Language: en
	sprintf(response, format.c_str(), version.c_str(), status, code.c_str(), getCurrentDate("Date").c_str(), response_body.size(), getCurrentDate("Last-Modified").c_str(), optional.c_str(), response_body.c_str());
	if (FULL_DEBUG)
		std::cout << "response: " << response << std::endl;
	return string(response);
}

string HttpProtocol::getCurrentDate(string format) {
	/* return the current date and time, format is the start of the returned string */
	char res[50];
	time_t current = time(0);
	tm* timeStruct = localtime(&current);
	string month = this->month[timeStruct->tm_mon];
	string day = this->day[timeStruct->tm_wday];
	
	sprintf(res, "%s: %s, %d %s %d GMT\r\n", format.c_str(), day.c_str(), timeStruct->tm_mday, month.c_str(), 1900 + timeStruct->tm_year);
	return string(res);
}

string HttpProtocol::Get(string version, int status, string response_body) {
	/* create a GET message */
	return this->createMessage(version, status, response_body);
}

string HttpProtocol::Post(string version, int status, string response_body) {
	/* create a POST message */
	return this->createMessage(version, status, response_body);
}

string HttpProtocol::Head(string version, int status) {
	/* create a HEAD message */
	return this->createMessage(version, status);
}

string HttpProtocol::Options(string version, string parsed_url) {
	/* create a OPTION message */
	char response[BUFFER_SIZE] = "";

	// Creating a single string for all of the allowed methods
	string cur_options = "";
	vector<string> options = this->allowed_methods[parsed_url];
	for (auto option : options)
		cur_options += option + ", ";

	string code = this->errorCodes[this->request_status];
	string format = std::string("%s %d %s\r\n") +
					"Allow: %s\r\n" +
					"%s" +  // Date: %s, %d %s %d GMT\r\n
					"Content-Length: 0\r\n" +
					"Accept-Ranges: bytes\r\n" +
					"Connection: close\r\n\r\n";

	sprintf(response, format.c_str(), version.c_str(), this->request_status, code.c_str(), cur_options.substr(0, cur_options.size() - 2).c_str(), getCurrentDate("Date").c_str());
	return response;
}

string HttpProtocol::Delete(string filename, string version, string parsedUrl) {
	/* create a DELETE message */
	char success[50] = "";
	char response[BUFFER_SIZE] = "";
	if (this->fileExists(filename.substr(1))) {
		if (!remove(filename.substr(1).c_str())) {
			sprintf(success, "File deleted successfully");
			this->allowed_methods.erase(filename);   // Removing the allowed methods from the deleted file
		}
		else {
			sprintf(success, "Error in file deletion");
			this->request_status = 500;  // server error
		}
	}
	else {
		sprintf(success, "File does not exist");
		this->request_status = 404;      // file not found
	}

	string code = this->errorCodes[this->request_status];
	string format = std::string("%s %d %s\r\n") +
					"%s" +  // Date: %s, %d %s %d GMT\r\n
					"%s\r\n" + // Delete status
					"Content-Location: %s\r\n" +
					"Content-Length: 0\r\n" +
					"Accept-Ranges: bytes\r\n" +
					"Connection: close\r\n\r\n";

	sprintf(response, format.c_str(), version.c_str(), this->request_status, code.c_str(), getCurrentDate("Date").c_str(), success, parsedUrl.c_str());
	return string(response);
}

string HttpProtocol::Put(string fileName, string field, string version, string parsedUrl) {
	/* create a PUT message */
	char response[BUFFER_SIZE];
	if (this->fileExists(fileName))
		this->request_status = 204;  // no content
	else {
		this->request_status = 201;  // created
		this->allowed_methods[fileName] = { "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE" }; // Adding the allowed methods to the file to be created
	}
	this->createFile(fileName, field);

	string code = this->errorCodes[this->request_status];
	string format = std::string("%s %d %s\r\n") +
					"%s" +  // Date: %s, %d %s %d GMT\r\n
					"Content-Location: %s\r\n" + 
					"Content-Length: 0\r\n" +
					"Accept-Ranges: bytes\r\n" +
					"Connection: close\r\n\r\n";

	sprintf(response, format.c_str(), version.c_str(), this->request_status, code.c_str(), getCurrentDate("Date").c_str(), parsedUrl.c_str());
	return string(response);
}

string HttpProtocol::Trace(string request, string version, int status) {
	/* create a TRACE message */
	char response[BUFFER_SIZE];
	string code = this->errorCodes[status];
	string format = std::string("%s %d %s\r\n") + 
					"Content-Type: text/html; charset=UTF-8\r\n" +
					"%s" +
					"Content-Length: 0\r\n" + 
					"Accept-Ranges: bytes\r\n" + 
					"Connection: close\r\n\r\n";

	string user_headers = "";
	vector<string> parts = split(request, version + "\r\n");
	if (parts.size() != 2)
		user_headers = parts.at(0);
	else
		user_headers = parts.at(1);

	sprintf(response, format.c_str(), version.c_str(), status, code.c_str(), user_headers.substr(0, user_headers.size() - 2).c_str());
	return string(response);

}

string HttpProtocol::handleRequest(string req) {
	/* handle the request given and return the needed response */
	Protocol method;
	string url = "", version = "", body = "";
	
	if (DEBUG)
		cout << "\r\nstart req: " << req << endl;
	unordered_map<string, string> variables = this->parse(req, method, url, version, body);
	return this->getResponse(req, version, this->request_status, url, variables, method, body);
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

void HttpProtocol::createFile(string fileName, string content) {
	/* create a file with the content given */
	fileName = fileName.substr(1);
	int action = ios_base::trunc;
	if (this->fileExists(fileName))
		action = ios_base::app;
	ofstream myfile(fileName, action);
	myfile << content << endl;
	myfile.close();
}

bool HttpProtocol::isAllowed(string url, Protocol method) {
	/* check according to the url if the method given is allowed */
	if (this->allowed_methods.find(url) != this->allowed_methods.end()) {
		for (auto allowedMethod : this->allowed_methods.at(url)) {
			if (this->getProtocol(allowedMethod) == method)
				return true;
		}
	}
	return false;
}
