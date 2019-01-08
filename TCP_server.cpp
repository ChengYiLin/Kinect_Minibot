#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

using namespace std;

void main()
{
	// Initialize winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResault = WSAStartup(ver, &data);

	if (wsResault != 0) {
		cerr << "Can not start Winsock, Error number: " << wsResault << endl;
		return;
	}

	// create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		cerr << "Can not create socket, Error : " << WSAGetLastError() << endl;
		return;
	}

	// bind the IPaddress and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (sockaddr*)&hint, sizeof(hint));
	
	// Tell winsock the socket is for listening
	listen(listening, SOMAXCONN);

	// wait for a connection
	sockaddr_in client;
	int clientsize = sizeof(client);

	SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientsize);

	char host[NI_MAXHOST];
	char service[NI_MAXHOST];

	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXHOST);

	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
		cout << host << "Connected to port " << service << endl;
	}
	else
	{
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		cout << host << "Connect on port " <<
			ntohs(client.sin_port) << endl;
	}
	// close listening socket
	closesocket(listening);
	// while loop : accept and echo message back to client
	char buff[4096];

	while (1) {
		ZeroMemory(buff, 4096);

		//Wait for client to send data
		int bytesReceived = recv(clientSocket, buff, 4096, 0);
		if (bytesReceived == SOCKET_ERROR) {
			cerr << "Error in recv().  Quit~" << endl;
			break;
		}

		if (bytesReceived == 0) {
			cout << "Client disconnect !!! " << endl;
			break;
		}

		cout << string(buff, 0, bytesReceived);
		// Echo message back to client
		send(clientSocket, buff, bytesReceived + 1, 0);
	}
	// close the socket
	closesocket(clientSocket);

	// shutdown winsock
	WSACleanup();
}