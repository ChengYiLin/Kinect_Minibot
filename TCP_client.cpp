#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

void main() {
	string ipAddress = "127.0.0.1";		// Ip address (you can change here)
	int port = 54000;					// Listening port on the server

	//-----Initialize Sock------
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResault = WSAStartup(ver, &data);

	if (wsResault != 0) {
		cerr << "Can not start Winsock, Error number: " << wsResault << endl;
		return;
	}

	//-----Create Socket------
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		cerr << "Can not create socket, Error : " << WSAGetLastError() << endl;
		return;
	}

	// what server and what port we want to connect to
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);
	 
	//----- Connect to Server -----
	int connResault = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResault == SOCKET_ERROR) {
		cerr << "Can't connect to server, Error number : " << WSAGetLastError << endl;
		closesocket(sock);
		WSACleanup();
		return;
	}

	//------ Do while loop to send and receive data -----
	char buff[4096];
	string userinput;

	do {
		// user inpput the word
		cout << "> ";
		getline(cin, userinput);

		// send the resault
		if (userinput.size() > 0) {			// make sure input something
			int sendResault = send(sock, userinput.c_str(), userinput.size() + 1, 0);
			if (sendResault != SOCKET_ERROR) {
				// wait for responec
				ZeroMemory(buff, 4096);
				int bytesReceived = recv(sock, buff, 4096, 0);
				if (bytesReceived > 0) {
				// Echo response to console
					cout << "server > " << string(buff, 0, bytesReceived) << endl;
				}
			}
		}
	} while (userinput.size() > 0);

	// ----- Close everything -----
	closesocket(sock);
	WSACleanup();
}