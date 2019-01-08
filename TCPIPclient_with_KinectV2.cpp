#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define kinect_X 0
#define kinect_Y 1
#define kinect_Z 2

#include <Kinect.h>
#include <ctime>

using namespace std;

// output operator for CameraSpacePoint
ostream& operator<<(ostream& rOS, const CameraSpacePoint& rPos)
{
	rOS << "(" << rPos.X << "\t" << rPos.Y << "\t" << rPos.Z << ")";
	return rOS;
}

// delay function
void Delay(int time){
	clock_t now = clock();
	while (clock()-now < time);
}

void  data_to_string_coordinate_x(const Joint& rJointPos, SOCKET sock,int num) {
	float test_x = rJointPos.Position.X;
	string final_ans = to_string(test_x);
	char buff[4096];
	int sendResault = send(sock, final_ans.c_str(), final_ans.size() + 1, 0);
	if (sendResault != SOCKET_ERROR) {
		// wait for responec
		ZeroMemory(buff, 4096);
		int bytesReceived = recv(sock, buff, 4096, 0);
		if (bytesReceived > 0) {
			// Echo response to console
			cout << string(buff, 0, bytesReceived) ;
		}
	}
}

void  data_to_string_coordinate_y(const Joint& rJointPos, SOCKET sock, int num) {
	float test_x = rJointPos.Position.Y;
	string final_ans = to_string(test_x);
	char buff[4096];
	int sendResault = send(sock, final_ans.c_str(), final_ans.size() + 1, 0);
	if (sendResault != SOCKET_ERROR) {
		// wait for responec
		ZeroMemory(buff, 4096);
		int bytesReceived = recv(sock, buff, 4096, 0);
		if (bytesReceived > 0) {
			// Echo response to console
			cout <<string(buff, 0, bytesReceived);
		}
	}
}

void  data_to_string_coordinate_z(const Joint& rJointPos, SOCKET sock, int num) {
	float test_x = rJointPos.Position.Z;
	string final_ans = to_string(test_x);
	char buff[4096];
	int sendResault = send(sock, final_ans.c_str(), final_ans.size() + 1, 0);
	if (sendResault != SOCKET_ERROR) {
		// wait for responec
		ZeroMemory(buff, 4096);
		int bytesReceived = recv(sock, buff, 4096, 0);
		if (bytesReceived > 0) {
			// Echo response to console
			cout << string(buff, 0, bytesReceived);
		}
	}
}

void data_to_string_Handstate(HandState leftHandstate, IBody* pBody, SOCKET sock, int num) {
	HRESULT hresult = pBody->get_HandLeftState(&leftHandstate);
	if (leftHandstate == HandState::HandState_Closed) {
		float test_x = 57;
		string final_ans = to_string(test_x);
		char buff[4096];
		int sendResault = send(sock, final_ans.c_str(), final_ans.size() + 1, 0);
		if (sendResault != SOCKET_ERROR) {
			// wait for responec
			ZeroMemory(buff, 4096);
			int bytesReceived = recv(sock, buff, 4096, 0);
			if (bytesReceived > 0) {
				// Echo response to console
				cout << string(buff, 0, bytesReceived) << "\t";
			}
		}
	}
	else {
		float test_x = 87;
		string final_ans = to_string(test_x);
		char buff[4096];
		int sendResault = send(sock, final_ans.c_str(), final_ans.size() + 1, 0);
		if (sendResault != SOCKET_ERROR) {
			// wait for responec
			ZeroMemory(buff, 4096);
			int bytesReceived = recv(sock, buff, 4096, 0);
			if (bytesReceived > 0) {
				// Echo response to console
				cout << string(buff, 0, bytesReceived) << "\t";
			}
		}
	}
}

int main() {
	string ipAddress = "192.168.1.2";		// Ip address
	int port = 54000;					// Listening port on the server
	int kinect_x = 0;
	int kinect_y = 1;
	int kinect_z = 2;

	//-----Initialize Sock------
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResault = WSAStartup(ver, &data);

	if (wsResault != 0) {
		cerr << "Can not start Winsock, Error number: " << wsResault << endl;
		return 0;
	}

	//-----Create Socket------
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		cerr << "Can not create socket, Error : " << WSAGetLastError() << endl;
		return 0;
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
		return 0;
	}

	//************* Kinect ******************
	
	// 1a. Get default Sensor
	cout << "Try to get default sensor" << endl;
	IKinectSensor* pSensor = nullptr;
	if (GetDefaultKinectSensor(&pSensor) != S_OK)
	{
		cerr << "Get Sensor failed" << endl;
		return 0;
	}

	// 1b. Open sensor
	cout << "Try to open sensor" << endl;
	if (pSensor->Open() != S_OK)
	{
		cerr << "Can't open sensor" << endl;
		return 0;
	}

	// 2a. Get frame source
	cout << "Try to get body source" << endl;
	IBodyFrameSource* pFrameSource = nullptr;
	if (pSensor->get_BodyFrameSource(&pFrameSource) != S_OK)
	{
		cerr << "Can't get body frame source" << endl;
		return 0;
	}

	// 2b. Get the number of body
	INT32 iBodyCount = 0;
	if (pFrameSource->get_BodyCount(&iBodyCount) != S_OK)
	{
		cerr << "Can't get body count" << endl;
		return 0;
	}
	cout << " > Can trace " << iBodyCount << " bodies" << endl;
	IBody** aBody = new IBody*[iBodyCount];
	for (int i = 0; i < iBodyCount; ++i)
		aBody[i] = nullptr;

	// 3a. get frame reader
	cout << "Try to get body frame reader" << endl;
	IBodyFrameReader* pFrameReader = nullptr;
	if (pFrameSource->OpenReader(&pFrameReader) != S_OK)
	{
		cerr << "Can't get body frame reader" << endl;
		return 0;
	}

	// 2b. release Frame source
	cout << "Release frame source" << endl;
	pFrameSource->Release();
	pFrameSource = nullptr;

	//************* Kinect End ******************

	//************* send and receive data *************
	char buff[4096];
	string userinput;

	while(1){
		IBodyFrame* pFrame = nullptr;
		if (pFrameReader->AcquireLatestFrame(&pFrame) == S_OK)
		{
			pFrame->GetAndRefreshBodyData(iBodyCount, aBody);
			for (int i = 0; i < iBodyCount; ++i)
			{
				IBody* pBody = aBody[i];
				// check if is tracked
				BOOLEAN bTracked = false;
				if ((pBody->get_IsTracked(&bTracked) == S_OK) && bTracked)
				{
					// get joint position
					Joint aJoints[JointType::JointType_Count];
					pBody->GetJoints(JointType::JointType_Count, aJoints);

					// Handstate control
					HandState leftHandstate = HandState::HandState_Unknown;

					// output information
					JointType eJointType = JointType::JointType_HandRight;
					const Joint& rJointPos = aJoints[eJointType];


					if (rJointPos.TrackingState != TrackingState_NotTracked)
					{	
						data_to_string_coordinate_x(rJointPos, sock, 0);
						cout << "\t" ;
						data_to_string_coordinate_y(rJointPos, sock, 0);
						cout << "\t" ;
						data_to_string_coordinate_z(rJointPos, sock, 0);
						cout << "\t";
						data_to_string_Handstate(leftHandstate,pBody,sock,0);
						cout << endl;
						Sleep(1000);
					}
				}
			}
			// 4e. release frame
			pFrame->Release();
		}
	}

	// ----- while loop end ----> Close everything -----
	closesocket(sock);
	WSACleanup();
	// delete body data array
	delete[] aBody;

	// 3b. release frame reader
	cout << "Release frame reader" << endl;
	pFrameReader->Release();
	pFrameReader = nullptr;

	// 1c. Close Sensor
	cout << "close sensor" << endl;
	pSensor->Close();

	// 1d. Release Sensor
	cout << "Release sensor" << endl;
	pSensor->Release();
	pSensor = nullptr;

	return 0;
}