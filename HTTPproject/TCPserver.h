#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[1024];
	int len;
};

enum METHODS {OPTIONS, GET, HEAD, POST, PUT, _DELETE, TRACE};
enum OPTIONS {EMPTY, LISTEN, RECEIVE, IDLE, SEND};

const int TIME_PORT = 8080;
const int MAX_SOCKETS = 60;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void getMethod(int index, string s);
void sendMessage(int index);
string parse(string s, string delimiter);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;