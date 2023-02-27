/*Here will be header for the HTTPprotocol*/

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <ctime>

string createMessage(string status, string response_body, string extraHeader, string extraHeaderContent);
string getCurrentDate();

class ServerResponse 
{       
public:
	string body;
	string status;

	ServerResponse()
	{
		body = "";
		status = "";
	}
};
