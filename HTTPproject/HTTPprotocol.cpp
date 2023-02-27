#include "HTTPprotocol.h"

string createMessage(string status, string response_body, string extraHeader, string extraHeaderContent) {
	char responseBodySize[255];
	string newLine = "\n";
	_itoa(response_body.size(), responseBodySize, 10);

	//HTTP/1.1 status code number status code name
	//Server: HTTP server/2.0 (Windows)
	//Date
	//Content-Type: text/html
	//Content-Length: number of bytes
	//extra header: extra header context
	//Connection: connection type
	//response body

	string response = status + newLine
		+ "Server: HTTP server/2.0 (Windows)" + newLine
		+ getCurrentDate() + newLine
		+ "Content-Type: text/html" + newLine
		+ "Content-Length: " + responseBodySize + newLine;
	if (!extraHeader.empty())
	{
		response += extraHeader + extraHeaderContent + newLine;
	}
	response += "Connection: close" + newLine + newLine + response_body;
	return response;
}

string getCurrentDate()
{
	const char* months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
	const char* days[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	char date[50];
	time_t t = std::time(0);   // get time now
	tm* now = std::localtime(&t);

	//Tue, 24 May 2022 15:44:31 GMT -> Date format example
	sprintf(date, "Date: %s, %d %s %d %d:%d:%d (GMT+3)", days[now->tm_wday], now->tm_mday,
		months[now->tm_mon], now->tm_year + 1900, now->tm_hour, now->tm_min, now->tm_sec);
	return string(date);
}