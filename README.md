# Kinect and Nexcom Minibot

## Abstract
本次專案是將 Microsoft 所推出的體感偵測器 Kinect V2 和 新漢股份有限公司所推出的六軸機械手臂 MiniBot (小寶) 作結合，以手勢控制來操作機械手臂的移動及氣動夾爪的夾取動作。

## Introduction

**所使用的軟體資源:**
* 程式語言 : C++
* 編譯器 : Visual Studio community 2017
* [Kinect for Windows SDK 2.0](https://www.microsoft.com/en-us/download/details.aspx?id=44561)
* [NexGMC](https://aiotcloud.net/market/singl.php?PID=132)

**所需要的硬體資源:**
* [Nexcom 教育用EtherCAT機器手臂](https://aiotcloud.net/market/singl.php?PID=110)
* 普通的網路線
* 網路分享器
* Kinect V2
* 一台 Win 10 的電腦

## Result 
### Kinect 環境安裝
首先先安裝好開發環境 [Visual Studio Community 2017](https://visualstudio.microsoft.com/zh-hant/vs/community/) ，安裝過程基本上照流程走即可，這邊特別需要注意的是在<font color=red>**使用C++的桌面開發**</font>的選項這邊要記得勾選 <font color=red>Windows 8.1 SDK 與UCRT SDK</font> 的選項要記得勾選，才能開發Kinect的相關程式

![](https://i.imgur.com/GlzgtCg.png)

安裝完 Visual Studio 後，安裝 [Kinect SDK](https://www.microsoft.com/en-us/download/details.aspx?id=44561)，流程只需照預設的方式安裝即可。安裝完之後，將我們的 Kinect 插上電並和我們的電腦連結，我們可以使用 Microsoft 先前所寫好的 Sample code 來測試一下，Sample code 就在安裝好的 SDK Browser v2.0中，隨意選擇任一個程式跑跑看，若程式運作成功代表安裝完成。

![](https://i.imgur.com/ANQvqMz.png)

### Kinect 手勢座標讀取
首先我們必須先知道 Kinect 的座標系統，如下圖所示，以鏡頭為原點，與鏡頭的距離為 Z 軸， 左右為 X 軸，上下為 Y 軸，根據這座標系，來判別我們所偵測的關節點座標 

![](https://i.imgur.com/w9kEP8T.png)

再來我們介紹 Kinect V2 所能偵測的關節點，和一代不同，二代所能使用的點為更多，如下圖所示，左方為一代，右方為二代，可以使用程式來決定你要哪個點位的座標。

![](https://i.imgur.com/TyFFbR3.jpg)

簡單介紹完之後進入我們程式的部分，首先我們使用 VS2017 新增一個 C++ 的專案，選擇空白專案。新增完之後在資源檔那邊加入一個 C++ 檔案 (.cpp)，這樣即可開始寫我們的程式了

![](https://i.imgur.com/87agAA9.png)

![](https://i.imgur.com/dUMCBm6.png)

在開發 Kinect 之前，我們必須先匯入 Kinect 的函式庫來做使用，函式庫我們已於先前裝 Kinect SDK 時已經裝好了，
在檔案總管的 project 名稱按右鍵選擇"屬性"，之後操作分為三步驟:
* 選擇 組態屬性 >>  "C/C++" >> "一般" >> "其他 include 目錄" >> 新增 C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\inc
* 選擇 組態屬性 >> "連結器" >> "一般" >> "其他程式庫目錄" >> 新增 C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\Lib\x86
* 選擇 組態屬性 >> "連結器" >> "輸入" >> "其他相依性" >> 新增 kinect20.lib

操作完成後我們先 include Kinect.h 來確定能否成功，若順利執行代表安裝完成。

![](https://i.imgur.com/WdFqsyM.png)

**Kinect讀取座標**

```C++
// Standard Library
#include <iostream>

// Kinect for Windows SDK Header
#include <Kinect.h>

#include <ctime>

#include <typeinfo>

using namespace std;

// output operator for CameraSpacePoint
ostream& operator<<(ostream& rOS, const CameraSpacePoint& rPos)
{
	rOS << "(" << rPos.X << "\t" << rPos.Y << "\t" << rPos.Z << ")";
	return rOS;
}

void Delay(int time) {
	clock_t now = clock();
	while(clock()-now<time);
}

int main(int argc, char** argv)
{
	// 1a. Get default Sensor
	cout << "Try to get default sensor" << endl;
	IKinectSensor* pSensor = nullptr;
	if (GetDefaultKinectSensor(&pSensor) != S_OK)
	{
		cerr << "Get Sensor failed" << endl;
		return -1;
	}

	// 1b. Open sensor
	cout << "Try to open sensor" << endl;
	if (pSensor->Open() != S_OK)
	{
		cerr << "Can't open sensor" << endl;
		return -1;
	}

	// 2a. Get frame source
	cout << "Try to get body source" << endl;
	IBodyFrameSource* pFrameSource = nullptr;
	if (pSensor->get_BodyFrameSource(&pFrameSource) != S_OK)
	{
		cerr << "Can't get body frame source" << endl;
		return -1;
	}

	// 2b. Get the number of body
	INT32 iBodyCount = 0;
	if (pFrameSource->get_BodyCount(&iBodyCount) != S_OK)
	{
		cerr << "Can't get body count" << endl;
		return -1;
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
		return -1;
	}

	// 2b. release Frame source
	cout << "Release frame source" << endl;
	pFrameSource->Release();
	pFrameSource = nullptr;

	// Enter main loop
	while (1)
	{
		// 4a. Get last frame
		IBodyFrame* pFrame = nullptr;
		if (pFrameReader->AcquireLatestFrame(&pFrame) == S_OK)
		{
			// 4b. get Body data
			if (pFrame->GetAndRefreshBodyData(iBodyCount, aBody) == S_OK)
			{
				// 4c. for each body
				for (int i = 0; i < iBodyCount; ++i)
				{
					IBody* pBody = aBody[i];

					// check if is tracked
					BOOLEAN bTracked = false;
					if ((pBody->get_IsTracked(&bTracked) == S_OK) && bTracked)
					{
						// get joint position
						Joint aJoints[JointType::JointType_Count];
						if (pBody->GetJoints(JointType::JointType_Count, aJoints) != S_OK)
						{
							cerr << "Get joints fail" << endl;
						}

						// output information
						JointType left_hand_JointType = JointType::JointType_HandLeft;

						const Joint& lJointPos = aJoints[left_hand_JointType];

						// Get Left Hand
						cout << " > Right Hand is ";
						if (lJointPos.TrackingState == TrackingState_NotTracked)
						{
							cout << "not tracked" << endl;
						}
						else
						{
							if (lJointPos.TrackingState == TrackingState_Inferred)
							{
								cout << "inferred ";
							}
							else if (lJointPos.TrackingState == TrackingState_Tracked)
							{
								cout << "tracked ";
							}

							cout << "at " << lJointPos.Position << endl;
						}
						Delay(400);
					}
				}
			}
			else
			{
				cerr << "Can't read body data" << endl;
			}

			// 4e. release frame
			pFrame->Release();
		}
	}

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
```

### 連線解決方案 TCPIP (win 10 <---> win7)

因為 Kinect V2 只能運行在 Win8 以上的系統而且我們的機械手臂是採用 win7，那麼就無法直接將 kinect 接在機械手臂的主機上，於是我使用 TCPIP 連線的方式在兩台電腦間傳值。設計的方式是將 機械手臂的主機作為 server 端來接收值， Kinect的主機作為 client 端負責傳值。
再撰寫程式之前，必須先做一些軟硬體的設定:
* 在win10上的操作，將網路線插在機械手臂的分享器上共享網路，並至控制台"變更介面卡選項"中的乙太網路"TCP/IPv4"那邊，將IP位置設為 192.168.1.1 ，子網路遮罩那邊設為 255.255.255.0 。同樣的操作也在win7上操作，不一樣的地方是將 IP位置改為 192.168.1.2，其餘都是一樣的。設定完成之後，可以去 cmd 中 ping 對方的IP試試看，如果成功就代表可以建立連線。

![](https://i.imgur.com/mTomaEu.png)

**TCPIP server(win7 機械手臂)**
```C++
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
```

**TCPIP client(win10 kinect)**
```C++
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

void main() {
	string ipAddress = "127.0.0.1";		// You can set the Ip address 
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
```

### 機械手臂控制程式

在win7機械手臂主機操作，所使用的機械手臂為 Nexcom 新漢股份有限公司的教育用機械手臂，在我們下載完 NexGMC 後就有控制機械手臂 P to P 的 Sample code 及相關的函式庫，我們再針對Sample code 來做修改即可，匯入函式庫的方法如　Kinect 那邊所提及一樣，這邊就只提需更改的項目
* C/C++ 一般 : C:\Program Files\NEXCOBOT\NexGMC\Header
* 連結器 一般 : C:\Program Files\NEXCOBOT\NexGMC\Lib
* 連結器 輸入 : NexMotion.lib

這邊也說一下 機械手臂的座標系統

![](https://i.imgur.com/e6Ux2gV.png)


程式函式庫就匯進來了，那麼就開始寫我們的程式
**MiniBot_GroupPTP程式**
```C++
// SampleCode.cpp : Defines the entry point for the console application.
//
#include "NexMotion.h"
#include "NexMotionError.h"

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define UNDER_WIN32_SIMULATION
#define GroupStatus_TargetReched    ( 1 << NMC_GROUP_STATUS_CSTP )
#define GroupStatus_Error           ( 1 << NMC_GROUP_STATUS_ERR )
#define GroupStatus_StandStill      ( 1 << NMC_GROUP_STATUS_ENA )

bool _targetCartPosArrivalCheck( I32_T DevID, I32_T GroupIndex );
bool _targetAcsPosArrivalCheck( I32_T DevID, I32_T GroupIndex );

int main()
{
    RTN_ERR ret                  = 0;
    I32_T   devIndex             = 0;
    I32_T   retDevID             = 0;
    I32_T   retGroupCount        = 0;
    I32_T   retGroupAxisCount    = 0;
    I32_T   countGroupAxis       = 0;
    I32_T   countGroup           = 0;
    I32_T   initGroupAxisCount   = 0;
    Pos_T   setHomePos           = {};
    I32_T   retState             = 0;
    I32_T   groupAxesIdxMask     = 0;
    I32_T   groupIndex           = 0;
    I32_T   retDevState          = 0;
    I32_T   runningAxis          = NMC_GROUP_AXIS_MASK_X;
    F64_T   cmdPostion           = 100.0;
    F64_T   cmdVelocity          = 5;
    Pos_T   refPosition_Inc      = { 5, 5, 5, 5, 5, 5 };// current position plus a incremential distance to be a command target
    Pos_T   cmdPosition          = {0};
    Pos_T   cmdPosPcs            = {0};

#ifdef UNDER_WIN32_SIMULATION
    I32_T devType   = NMC_DEVICE_TYPE_SIMULATOR;
    U32_T sleepTime = 500;
#else
    I32_T devType   = NMC_DEVICE_TYPE_ETHERCAT;
    U32_T sleepTime = 3000;
#endif

    printf( "=======================================================================================================================\n");
    printf( "** This is an example of how to use API GroupPtpAcs to control a 6R robot.\n" );
    printf( "**** Notification: The device must have at least one group.\n" );
    printf( "**** Notification: The first group must have at least six axes.\n" );
    printf( "**** Notification: The user must check whether the correct configuration file (.ini ) exists next to the executable file.\n");
    printf( "=======================================================================================================================\n\n");
    //=================================================
    //              Device open up
    //=================================================
    printf( "Start to openup device...\n" );

    ret = NMC_DeviceOpenUp( devType, devIndex, &retDevID );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_DeviceOpenUp: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        goto ERR_SHUTDOWN;
    }
    printf( "\nDevice open up succeed, device ID: %d.\n", retDevID );

    //get device state
    ret = NMC_DeviceGetState( retDevID, &retDevState );
    if( retDevState != NMC_DEVICE_STATE_OPERATION || ret != 0 )
    {
        printf( "ERROR! Device open up failed, device ID: %d.\n", retDevID );
        goto ERR_SHUTDOWN;
    }
    printf( "Device ID %d: state is OPERATION.\n", retDevID );

    //=================================================
    //              Get device infomation
    //=================================================
    //Get amount of GROUP
    ret = NMC_DeviceGetGroupCount( retDevID, &retGroupCount );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_DeviceGetGroupCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        goto ERR_SHUTDOWN;
    }

    if( retGroupCount == 0 )
    {
        printf( "ERROR! NMC_DeviceGetGroupCount = 0\n" );
        goto ERR_SHUTDOWN;
    }
    printf( "Get group count succeed, device has %d group.\n", retGroupCount );

    //Get amount of AXIS of each GROUP

    for( countGroup = 0; countGroup < retGroupCount; ++countGroup )
    {
        ret = NMC_DeviceGetGroupAxisCount( retDevID, countGroupAxis, &retGroupAxisCount );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_DeviceGetGroupAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            goto ERR_SHUTDOWN;
        }
        printf( "Get group axis count succeed, group index %d has %d axis.\n", countGroupAxis, retGroupAxisCount );
    }

    //=================================================
    //       Clean alarm of drives of each group
    //=================================================
    for( countGroup = 0; countGroup < retGroupCount; ++countGroup )
    {
        ret = NMC_GroupResetState( retDevID, countGroup );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupResetDriveAlmAll: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            goto ERR_SHUTDOWN;
        }
    }

    Sleep( sleepTime );

    //=================================================
    //       Enable all single axes and groups
    //=================================================
    ret = NMC_DeviceEnableAll( retDevID );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_DeviceEnableAll: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        goto ERR_SHUTDOWN;
    }
    printf( "\nReady to enable all single axes and groups...\n" );
    

    Sleep( sleepTime );

    //================================================
    //              Clear Mastering Data
    //=================================================
    //groupAxesIdxMask += NMC_GROUP_AXIS_MASK_A;

    //ret = NMC_GroupAxesHomeDrive( retDevID, groupIndex, groupAxesIdxMask );
    //if( ret != 0 )
    //{
    //    printf( "ERROR! NMC_GroupSetHomePos: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    //}

    //printf( "Group %d Home Moving\n", groupIndex );

    //if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
    //{
    //    printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
    //    goto ERR_SHUTDOWN;
    //}

    //groupAxesIdxMask = 0;
    //groupAxesIdxMask += NMC_GROUP_AXIS_MASK_X;
    //groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Y;
    //groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Z;
    //groupAxesIdxMask += NMC_GROUP_AXIS_MASK_B;
    //groupAxesIdxMask += NMC_GROUP_AXIS_MASK_C;

    //Sleep( sleepTime );

    //ret = NMC_GroupAxesHomeDrive( retDevID, groupIndex, groupAxesIdxMask );
    //if( ret != 0 )
    //{
    //    printf( "ERROR! NMC_GroupSetHomePos: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    //}

    //printf( "Group %d Home Moving\n", groupIndex );

    //if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
    //{
    //    printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
    //    goto ERR_SHUTDOWN;
    //}

    ////Group Get Actual PosPcs
    //ret = NMC_GroupGetActualPosAcs( retDevID, groupIndex, &cmdPosPcs );
    //if( ret != 0 )
    //{
    //    printf( "ERROR! NMC_GroupGetActualPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    //}

    //for( initGroupAxisCount = 0; initGroupAxisCount < retGroupAxisCount; ++initGroupAxisCount )
    //    printf( "ActualPosAcs[%d] = %f \n", initGroupAxisCount, cmdPosPcs.pos[initGroupAxisCount] );

    ////Group Get Command PosPcs
    //ret = NMC_GroupGetCommandPosAcs( retDevID, groupIndex, &cmdPosPcs );
    //if( ret != 0 )
    //{
    //    printf( "ERROR! NMC_GroupGetCommandPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    //}

    //for( initGroupAxisCount = 0; initGroupAxisCount < retGroupAxisCount; ++initGroupAxisCount )
    //    printf( "CommandPosAcs[%d] = %f \n", initGroupAxisCount, cmdPosPcs.pos[initGroupAxisCount] );

    //=================================================
    //       Perform the motion Group PTP
    //=================================================
	
	//  Group PTP
	Pos_T acsPos[] = {0,90,0,0,-90,0};

	ret = NMC_GroupPtpAcsAll( retDevID, groupIndex, 63, acsPos );
	 if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
    {
        printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
        goto ERR_SHUTDOWN;
    }
	
	// change absolute to relative
	ret = NMC_GroupSetParamI32( retDevID, groupIndex, 0x30, 0, 1 );  

	// Cart motivation
   Pos_T cartPos[] = { 90, 0, 0, 0, 0, 0 };
   ret = NMC_GroupPtpCartAll( retDevID, groupIndex, 63, cartPos ); 
   if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
   {
        printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
        goto ERR_SHUTDOWN;
   }

   // control the IO output
   U32_T sizeByte = 1;

   I32_T value = 0x01;

   ret = NMC_WriteOutputMemory( retDevID, 0, sizeByte, &value );
   if( ret != 0 )
	   printf("ERROR_NMC_WriteOutputMemory:%d\n", ret );


  /// printf( "\nStart to perform a motion of Group PTP to a axis .\n" );
  //  printf( "Axis %d running to targetPosition= %.3f\n", runningAxis + 1, cmdPostion );

  //  ret = NMC_GroupPtpAcs( retDevID, groupIndex, runningAxis, cmdPostion, &cmdVelocity );
  //  if( ret != 0 )
  //  {
  //      printf( "ERROR! NMC_GroupPtpAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
  //      goto ERR_SHUTDOWN;
  //  }
  //   printf( "\n NMC_GroupPtpAcs Success! Wait Command down.\n" );



    //=================================================
    //       Perform the motion Group PTP ALL AXES
    //=================================================
    //ret = NMC_GroupGetCommandPosAcs( retDevID, groupIndex, &cmdPosition );
    //if( ret != 0 )
    //{
    //    printf( "ERROR! NMC_GroupGetCommandPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    //}

    //for ( initGroupAxisCount = 0; initGroupAxisCount < retGroupAxisCount; ++initGroupAxisCount )
    //    cmdPosition.pos[initGroupAxisCount]= refPosition_Inc.pos[initGroupAxisCount];

    //printf( "\n====>Group%d Ptp Moving Command\n", groupIndex );
    //for( initGroupAxisCount = 0; initGroupAxisCount < retGroupAxisCount; ++initGroupAxisCount )
    //    printf( "CommandPosAcs[%d] = %f \n", initGroupAxisCount, cmdPosition.pos[initGroupAxisCount] );

    //ret = NMC_GroupPtpAcsAll( retDevID, groupIndex, 63, &cmdPosition );
    //if( ret != 0 )
    //{
    //    printf( "ERROR! NMC_GroupPtpAcsAll: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    //}

    //if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
    //{
    //    printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
    //    goto ERR_SHUTDOWN;
    //}

ERR_SHUTDOWN:
    //=================================================
    //       Disable all single axes and groups
    //=================================================
    ret = NMC_DeviceDisableAll( retDevID );
    if( ret != 0 )
        printf( "ERROR! NMC_DeviceDisableAll: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );

    printf( "\nReady to disable all single axes and groups...\n" );

    Sleep( sleepTime );

    //=================================================
    //              Shutdown device
    //=================================================
    ret = NMC_DeviceShutdown( retDevID );
    if( ret != 0 )
        printf( "ERROR! NMC_DeviceShutdown: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );

    printf( "\nDevice shutdown succeed.\n" );

    system( "pause" );
    return 0;
}

bool _targetAcsPosArrivalCheck( I32_T DevID, I32_T GroupIndex )
{
    I32_T   groupStatus     = 0;
    I32_T   groupAxisCount  = 0;
    I32_T   groupAxisIndex  = 0;
    U32_T   time            = 0;
    Pos_T   groupAxisActPos = {};
    RTN_ERR ret             = 0;

#ifdef UNDER_WIN32_SIMULATION
    U32_T sleepTime = 1000;
    U32_T waitTime  = 2000;
#else
    U32_T sleepTime = 2000;
    U32_T waitTime  = 1000;
#endif

    ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        return false;
    }

    while ( ( groupStatus & GroupStatus_TargetReched ) == 0 )
    {
        Sleep( sleepTime );
        ret = NMC_GroupGetActualPosAcs( DevID, GroupIndex , &groupAxisActPos );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetActualPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        ret = NMC_DeviceGetGroupAxisCount( DevID, GroupIndex, &groupAxisCount );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_DeviceGetGroupAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        printf( "P_A = " );
        for( groupAxisIndex = 0; groupAxisIndex < groupAxisCount; ++groupAxisIndex )
            printf( " %.1f\t", groupAxisActPos.pos[groupAxisIndex] );
        printf( "\n" );

        ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        if( waitTime < time )
        {
            printf( "targetAxisPosArrivalCheck TimeOut" );
            return false;
        }
        time++;
    }
    printf( "<Target arrival>\n" );
    return true;
}

bool _targetCartPosArrivalCheck( I32_T DevID, I32_T GroupIndex )
{
    I32_T   groupStatus    = 0;
    I32_T   groupAxisCount = 0;
    I32_T   groupAxisIndex = 0;
    U32_T   time           = 0;
    Pos_T   cartActPos     = {};
    RTN_ERR ret            = 0;

#ifdef UNDER_WIN32_SIMULATION
    U32_T sleepTime = 1000;
    U32_T waitTime  = 2000;
#else
    U32_T sleepTime = 2000;
    U32_T waitTime  = 1000;
#endif

    ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        return false;
    }

    while ( ( groupStatus & GroupStatus_TargetReched ) == 0 )
    {
        Sleep( sleepTime );

        ret = NMC_GroupGetActualPosPcs( DevID, GroupIndex, &cartActPos );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetActualPosPcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        ret = NMC_DeviceGetGroupAxisCount( DevID, GroupIndex, &groupAxisCount );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_DeviceGetGroupAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        printf( "P_P = " );
        for( groupAxisIndex = 0; groupAxisIndex < groupAxisCount; ++groupAxisIndex )
        {
            printf( "%.1f\t", cartActPos.pos[groupAxisIndex] );
        }
        printf( "\n" );

        ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        if( waitTime < time )
        {
            printf( "targetCartPosArrivalCheck TimeOut" );
            return false;
        }
        time++;
    }
    printf( "<Target arrival>\n" );
    return true;
}
```

### 全部整合

在win10上已能用 kinect 讀取座標了，在 win7 上也已能用程式控制機械手臂了，連接方式也有了，那就將程式整合在一起吧

**TCPIP(client)+Kinect** 
```C++
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
```

**TCPIP(server)+NexcomMiniBot**
```C++
// TCPIP server
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment (lib, "ws2_32.lib")
// miniBot
#include "NexMotion.h"
#include "NexMotionError.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GroupStatus_TargetReched    ( 1 << NMC_GROUP_STATUS_CSTP )
#define GroupStatus_Error           ( 1 << NMC_GROUP_STATUS_ERR )
#define GroupStatus_StandStill      ( 1 << NMC_GROUP_STATUS_ENA )

bool _targetCartPosArrivalCheck( I32_T DevID, I32_T GroupIndex );
bool _targetAcsPosArrivalCheck( I32_T DevID, I32_T GroupIndex );
void Handstate_control(  RTN_ERR ret, I32_T retDevID, char buff[4096] );

using namespace std;

int main()
{
	//=====================================================
	//						TCPIP server
	//=====================================================

	// Initialize winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResault = WSAStartup(ver, &data);

	if (wsResault != 0) {
		cerr << "Can not start Winsock, Error number: " << wsResault << endl;
		return 0;
	}

	// create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		cerr << "Can not create socket, Error : " << WSAGetLastError() << endl;
		return 0;
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


	//=====================================================
	//                     Mini Bot 
	//=====================================================

	RTN_ERR ret                  = 0;
    I32_T   devIndex             = 0;
    I32_T   retDevID             = 0;
    I32_T   retGroupCount        = 0;
    I32_T   retGroupAxisCount    = 0;
    I32_T   countGroupAxis       = 0;
    I32_T   countGroup           = 0;
    I32_T   initGroupAxisCount   = 0;
    Pos_T   setHomePos           = {};
    I32_T   retState             = 0;
    I32_T   groupAxesIdxMask     = 0;
    I32_T   groupIndex           = 0;
    I32_T   retDevState          = 0;
    I32_T   runningAxis          = NMC_GROUP_AXIS_MASK_X;
    F64_T   cmdPostion           = 100.0;
    F64_T   cmdVelocity          = 5;
    Pos_T   refPosition_Inc      = { 5, 5, 5, 5, 5, 5 };// current position plus a incremential distance to be a command target
    Pos_T   cmdPosition          = {0};
    Pos_T   cmdPosPcs            = {0};

#ifdef UNDER_WIN32_SIMULATION
    I32_T devType   = NMC_DEVICE_TYPE_SIMULATOR;
    U32_T sleepTime = 500;
#else
    I32_T devType   = NMC_DEVICE_TYPE_ETHERCAT;
    U32_T sleepTime = 3000;
#endif

    printf( "=======================================================================================================================\n");
    printf( "** This is an example of how to use API GroupPtpAcs to control a 6R robot.\n" );
    printf( "**** Notification: The device must have at least one group.\n" );
    printf( "**** Notification: The first group must have at least six axes.\n" );
    printf( "**** Notification: The user must check whether the correct configuration file (.ini ) exists next to the executable file.\n");
    printf( "=======================================================================================================================\n\n");
    //=================================================
    //              Device open up
    //=================================================
    printf( "Start to openup device...\n" );

    ret = NMC_DeviceOpenUp( devType, devIndex, &retDevID );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_DeviceOpenUp: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        goto ERR_SHUTDOWN;
    }
    printf( "\nDevice open up succeed, device ID: %d.\n", retDevID );

    //get device state
    ret = NMC_DeviceGetState( retDevID, &retDevState );
    if( retDevState != NMC_DEVICE_STATE_OPERATION || ret != 0 )
    {
        printf( "ERROR! Device open up failed, device ID: %d.\n", retDevID );
        goto ERR_SHUTDOWN;
    }
    printf( "Device ID %d: state is OPERATION.\n", retDevID );

    //=================================================
    //              Get device infomation
    //=================================================
    //Get amount of GROUP
    ret = NMC_DeviceGetGroupCount( retDevID, &retGroupCount );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_DeviceGetGroupCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        goto ERR_SHUTDOWN;
    }

    if( retGroupCount == 0 )
    {
        printf( "ERROR! NMC_DeviceGetGroupCount = 0\n" );
        goto ERR_SHUTDOWN;
    }
    printf( "Get group count succeed, device has %d group.\n", retGroupCount );

    //Get amount of AXIS of each GROUP

    for( countGroup = 0; countGroup < retGroupCount; ++countGroup )
    {
        ret = NMC_DeviceGetGroupAxisCount( retDevID, countGroupAxis, &retGroupAxisCount );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_DeviceGetGroupAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            goto ERR_SHUTDOWN;
        }
        printf( "Get group axis count succeed, group index %d has %d axis.\n", countGroupAxis, retGroupAxisCount );
    }

    //=================================================
    //       Clean alarm of drives of each group
    //=================================================
    for( countGroup = 0; countGroup < retGroupCount; ++countGroup )
    {
        ret = NMC_GroupResetState( retDevID, countGroup );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupResetDriveAlmAll: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            goto ERR_SHUTDOWN;
        }
    }

    Sleep( sleepTime );

    //=================================================
    //       Enable all single axes and groups
    //=================================================
    ret = NMC_DeviceEnableAll( retDevID );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_DeviceEnableAll: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        goto ERR_SHUTDOWN;
    }
    printf( "\nReady to enable all single axes and groups...\n" );

    Sleep( sleepTime );

	// ****************************************************
	//			while loop: do what you want to do
	// ****************************************************
	//  Group PTP
	Pos_T acsPos[] = {0,90,0,0,-90,0};

	ret = NMC_GroupPtpAcsAll( retDevID, groupIndex, 63, acsPos );
	 if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
    {
        printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
        goto ERR_SHUTDOWN;
    }

	printf(" *************  I am  ready *****************\n");
	// while loop : accept and echo message back to client
	// TCPIP buffer
	char buff[4096];
	int time = 1;
	int x_axis_move,y_axis_move,z_axis_move;

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

		// print out the data 
		if (time % 4 != 0) {
			cout << stof(buff) << "\t" ;	// X y z data
			if(time % 4 == 1){				// kinect x --->  Minibot y
				y_axis_move = stof(buff)*100;
				if( y_axis_move > 40 ){
					y_axis_move = 100;
				}
				else if( y_axis_move <-30 ){
					y_axis_move = -100;
				}
				else{
					y_axis_move = 0;
				}
			}
			else if(time % 4 == 2){			// kinect y --->  Minibot z
				z_axis_move = stof(buff)*100;
				if( z_axis_move > 50 ){
					z_axis_move = 100;
				}
				else if( z_axis_move <-10){
					z_axis_move = -100;
				}
				else{
					z_axis_move = 0;
				}
			}
			else if(time % 4 == 3){			// kinect z --->  Minibot x
				x_axis_move = stof(buff)*100;
				if( x_axis_move > 120 ){
					x_axis_move = 100;
				}
				else if( x_axis_move <80 ){
					x_axis_move = -100;
				}
				else{
					x_axis_move = 0;
				}
			}
		}
		else{
			cout << stoi(buff) << endl;	// Handstate

		//=============	
		//    Moving
		//=============
			// change absolute to relative
			ret = NMC_GroupSetParamI32( retDevID, groupIndex, 0x30, 0, 1 );  

			
			Pos_T cartPos[] = { x_axis_move, y_axis_move, z_axis_move, 0, 0, 0 };
			ret = NMC_GroupPtpCartAll( retDevID, groupIndex, 63, cartPos ); 
			if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) ){
				printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
				goto ERR_SHUTDOWN;
			}
			

		//====================
		//  Handstate control
		//====================
			Handstate_control(ret, retDevID, buff);
		}
		
		
		time += 1;

		// Echo message back to client
		send(clientSocket, buff, bytesReceived + 1, 0);
		
	}

	//=================================================
	//				TCPIP server shutdown
	//=================================================

	// close the socket
	closesocket(clientSocket);

	// shutdown winsock
	WSACleanup();

	//=================================================
	//					MiniBot shutdown
	//=================================================

	ERR_SHUTDOWN:
    //=================================================
    //       Disable all single axes and groups
    //=================================================
    ret = NMC_DeviceDisableAll( retDevID );
    if( ret != 0 )
        printf( "ERROR! NMC_DeviceDisableAll: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );

    printf( "\nReady to disable all single axes and groups...\n" );

    Sleep( sleepTime );

    //=================================================
    //              Shutdown device
    //=================================================
    ret = NMC_DeviceShutdown( retDevID );
    if( ret != 0 )
        printf( "ERROR! NMC_DeviceShutdown: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );

    printf( "\nDevice shutdown succeed.\n" );

    system( "pause" );
	return 0;
}

// 函式

bool _targetAcsPosArrivalCheck( I32_T DevID, I32_T GroupIndex )
{
    I32_T   groupStatus     = 0;
    I32_T   groupAxisCount  = 0;
    I32_T   groupAxisIndex  = 0;
    U32_T   time            = 0;
    Pos_T   groupAxisActPos = {};
    RTN_ERR ret             = 0;

#ifdef UNDER_WIN32_SIMULATION
    U32_T sleepTime = 1000;
    U32_T waitTime  = 2000;
#else
    U32_T sleepTime = 2000;
    U32_T waitTime  = 1000;
#endif

    ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        return false;
    }

    while ( ( groupStatus & GroupStatus_TargetReched ) == 0 )
    {
        Sleep( sleepTime );
        ret = NMC_GroupGetActualPosAcs( DevID, GroupIndex , &groupAxisActPos );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetActualPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        ret = NMC_DeviceGetGroupAxisCount( DevID, GroupIndex, &groupAxisCount );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_DeviceGetGroupAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        printf( "P_A = " );
        for( groupAxisIndex = 0; groupAxisIndex < groupAxisCount; ++groupAxisIndex )
            printf( " %.1f\t", groupAxisActPos.pos[groupAxisIndex] );
        printf( "\n" );

        ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        if( waitTime < time )
        {
            printf( "targetAxisPosArrivalCheck TimeOut" );
            return false;
        }
        time++;
    }
    printf( "<Target arrival>\n" );
    return true;
}

bool _targetCartPosArrivalCheck( I32_T DevID, I32_T GroupIndex )
{
    I32_T   groupStatus    = 0;
    I32_T   groupAxisCount = 0;
    I32_T   groupAxisIndex = 0;
    U32_T   time           = 0;
    Pos_T   cartActPos     = {};
    RTN_ERR ret            = 0;

#ifdef UNDER_WIN32_SIMULATION
    U32_T sleepTime = 1000;
    U32_T waitTime  = 2000;
#else
    U32_T sleepTime = 2000;
    U32_T waitTime  = 1000;
#endif

    ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
    if( ret != 0 )
    {
        printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
        return false;
    }

    while ( ( groupStatus & GroupStatus_TargetReched ) == 0 )
    {
        Sleep( sleepTime );

        ret = NMC_GroupGetActualPosPcs( DevID, GroupIndex, &cartActPos );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetActualPosPcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        ret = NMC_DeviceGetGroupAxisCount( DevID, GroupIndex, &groupAxisCount );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_DeviceGetGroupAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        printf( "P_P = " );
        for( groupAxisIndex = 0; groupAxisIndex < groupAxisCount; ++groupAxisIndex )
        {
            printf( "%.1f\t", cartActPos.pos[groupAxisIndex] );
        }
        printf( "\n" );

        ret = NMC_GroupGetStatus( DevID, GroupIndex, &groupStatus );
        if( ret != 0 )
        {
            printf( "ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
            return false;
        }

        if( waitTime < time )
        {
            printf( "targetCartPosArrivalCheck TimeOut" );
            return false;
        }
        time++;
    }
    printf( "<Target arrival>\n" );
    return true;
}

void Handstate_control( RTN_ERR ret, I32_T retDevID, char buff[4096] ){
	// MiniBot 宣告
	U32_T sizeByte = 1;
	I32_T value = 0x01;	// 關

	// hand state control
	if(stoi(buff) == 57){
		value = 0x01;
		ret = NMC_WriteOutputMemory( retDevID, 0, sizeByte, &value );
		if( ret != 0 )
			printf("ERROR_NMC_WriteOutputMemory:%d\n", ret );
	}
	else if(stoi(buff) == 87){
		value = 0x00;
		ret = NMC_WriteOutputMemory( retDevID, 0, sizeByte, &value );
		if( ret != 0 )
			printf("ERROR_NMC_WriteOutputMemory:%d\n", ret );
	}
}
```

## Reference
1. https://github.com/OpenKinect/libfreenect2/blob/master/README.md#linux
2. http://nw.tsuda.ac.jp/lec/kinect2/KinectV2/index-en.html
3. https://github.com/Kinect/PyKinect2/issues/48
4. https://blog.csdn.net/baolinq/article/details/52373574
5. https://www.cnblogs.com/kunyuanjushi/p/5204436.html
6. https://www.youtube.com/watch?v=0Zr_0Jy8mWE&t=9s
7. https://www.youtube.com/watch?v=WDn-htpBlnU&t=8s
