#include "TCPserver.h"
#include "HTTPprotocol.h"
#include <fstream>

void createOrUpdateFile(string requestMessage);
string getFileName(string requestMessage);
void createFile(string fileName);
bool checkIfExists(string fileName);
bool checkHeaders(string s);
ServerResponse serverResponse;

int main() 
{
	WSAData wsaData; 
    	
	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
        cout<<"Server: Error at WSAStartup()\n";
		return 1;
	}

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket)
	{
        cout<<"Server: Error at socket(): "<<WSAGetLastError()<<endl;
        WSACleanup();
        return 1;
	}

	sockaddr_in serverService;
    serverService.sin_family = AF_INET; 
	
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(TIME_PORT);

    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *) &serverService, sizeof(serverService))) 
	{
		cout<<"Server: Error at bind(): "<<WSAGetLastError()<<endl;
        closesocket(listenSocket);
		WSACleanup();
        return 1;
    }

    if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
		WSACleanup();
        return 1;
	}
	addSocket(listenSocket, LISTEN);

	while (true)
	{
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout <<"Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return 1;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:			// Receiving the message from the Postman
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{ 
		 cout << "Server: Error at accept(): " << WSAGetLastError() << endl; 		 
		 return;
	}
	cout << "Server: Client "<<inet_ntoa(from.sin_addr)<<":"<<ntohs(from.sin_port)<<" is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag=1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout<<"Server: Error at ioctlsocket(): "<<WSAGetLastError()<<endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout<<"\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);
	
	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);				
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		string s = &sockets[index].buffer[len];
		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			getMethod(index, s);
		}
	}

}

void getMethod(int index, string s)
{
	//TODO: Check for the errors

	string delimiter = "\r\n\r\n";
	string messageBody = parse(s, delimiter);
	serverResponse.body = messageBody;

	if (strncmp(sockets[index].buffer, "OPTIONS", 7) == 0)
	{
		sockets[index].send = SEND;
		sockets[index].sendSubType = OPTIONS;
		serverResponse.status = "HTTP/1.1 200 OK";

		memcpy(sockets[index].buffer, &sockets[index].buffer[7], sockets[index].len - 7);
		sockets[index].len -= 7;
		return;
	}

	else if (strncmp(sockets[index].buffer, "GET", 3) == 0)
	{
		sockets[index].send = SEND;
		sockets[index].sendSubType = GET;

		memcpy(sockets[index].buffer, &sockets[index].buffer[3], sockets[index].len - 3);
		sockets[index].len -= 3;
		return;
	}

	else if (strncmp(sockets[index].buffer, "POST", 4) == 0)
	{
		sockets[index].send = SEND;
		sockets[index].sendSubType = POST;
		cout << "Client message: " << messageBody << endl;
		serverResponse.status = "HTTP/1.1 200 OK";

		return;
	}

	else if (strncmp(sockets[index].buffer, "PUT", 3) == 0)
	{
		sockets[index].send = SEND;
		sockets[index].sendSubType = PUT;
		if (checkHeaders(s))	// If there are headers that not implement, return 501(Not implemented) error response
		{
			createOrUpdateFile(&sockets[index].buffer[3]);
		}
			
		return;

	}

	else if (strncmp(sockets[index].buffer, "DELETE", 6) == 0)
	{

	}

	else if (strncmp(sockets[index].buffer, "TRACE", 5) == 0)
	{

	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	char* sendBuff = new char[256];
	strcpy(sendBuff, "");

	SOCKET msgSocket = sockets[index].id;
	if (sockets[index].sendSubType == OPTIONS)
	{
		string extraHeader = "Access-Control-Allow-Methods: ";
		string extraHeaderContent = "OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE";
		string message = createMessage(serverResponse.status.c_str() ,"", extraHeader, extraHeaderContent);
		strcpy(sendBuff, message.c_str());
	}

	else if (sockets[index].sendSubType == GET)
	{

	}

	else if(sockets[index].sendSubType == POST)
	{
		string response = "Server got POST request successfully";
		string message = createMessage(serverResponse.status.c_str(), response, "", "");
		strcpy(sendBuff, message.c_str());
	}


	else if (sockets[index].sendSubType == PUT)
	{
		string response;
		if (serverResponse.status == "HTTP/1.1 501 Not Implemented")
		{
			response = "Not all of the <Content-> headers are implemented on the server";
		}
		else if (serverResponse.status == "HTTP/1.1 201 Created")
		{
			response = "File was created successfully on the server";
		}
		else
		{
			response = "File was updated successfully on the server";
		}
		string message = createMessage(serverResponse.status.c_str(), response.c_str(), "", "");
		strcpy(sendBuff, message.c_str());
	}
		
	else if (sockets[index].sendSubType == _DELETE)
	{

	}

	else if (sockets[index].sendSubType == TRACE)
	{

	}


	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	cout<<"Sent: "<<bytesSent<<"\\"<<strlen(sendBuff)<<" bytes of\n \""<<sendBuff<<"\" message.\n";	

	sockets[index].send = IDLE;
}

string parse(string s, string delimiter)
{	
	size_t pos = 0;
	string token;
	while ((pos = s.find(delimiter)) != string::npos) 
	{
		token = s.substr(0, pos);
		s.erase(0, pos + delimiter.length());
	}
	return s;
}

void createOrUpdateFile(string requestMessage)
{
	string fileName = getFileName(requestMessage);
	createFile(fileName);
}

string getFileName(string requestMessage)
{
	int i = 1;
	string fileName;
	while (requestMessage[i] != ' ')
	{
		i++;
		fileName += requestMessage[i];
	}
	return fileName;
}

void createFile(string fileName)
{
	ofstream file;
	string path = "C:\\Temp\\" + fileName;
	
	if (!checkIfExists(path))
	{
		serverResponse.status = "HTTP/1.1 201 Created";
	}
	else
	{
		serverResponse.status = "HTTP/1.1 200 OK";
	}

	file.open(path.c_str());
	file << serverResponse.body;
	file.close();
}

bool checkIfExists(string fileName)
{
	if (FILE* file = fopen(fileName.c_str(), "r")) 
	{
		fclose(file);
		return true;
	}
	else 
	{
		return false;
	}
}

bool checkHeaders(string s)
{
	vector<string> supportedHeader = { "Content-Type", "Content-Length" };	
	string check = "Content-";
	bool areHeadersOK = true;

	size_t found = s.find(check);
	int index = found;
	while (found != string::npos)	// Check for all Content- headers
	{
		string header = "Content-";
		while (s[index + check.size()] != ':')
		{
			header += s[index + check.size()];
			index += 1;
		}

		if (!count(supportedHeader.begin(), supportedHeader.end(), header))
		{
			serverResponse.status = "HTTP/1.1 501 Not Implemented";
			areHeadersOK = false;
			break;
		}

		found = s.find(check, found + 1); 
		index = found;
	}
	return areHeadersOK;
}
