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

    //================================================
    //              Clear Mastering Data
    //=================================================
    // groupAxesIdxMask += NMC_GROUP_AXIS_MASK_A;

    // ret = NMC_GroupAxesHomeDrive( retDevID, groupIndex, groupAxesIdxMask );
    // if( ret != 0 )
    // {
    //    printf( "ERROR! NMC_GroupSetHomePos: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    // }

    // printf( "Group %d Home Moving\n", groupIndex );

    // if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
    // {
    //    printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
    //    goto ERR_SHUTDOWN;
    // }

    // groupAxesIdxMask = 0;
    // groupAxesIdxMask += NMC_GROUP_AXIS_MASK_X;
    // groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Y;
    // groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Z;
    // groupAxesIdxMask += NMC_GROUP_AXIS_MASK_B;
    // groupAxesIdxMask += NMC_GROUP_AXIS_MASK_C;

    // Sleep( sleepTime );

    // ret = NMC_GroupAxesHomeDrive( retDevID, groupIndex, groupAxesIdxMask );
    // if( ret != 0 )
    // {
    //    printf( "ERROR! NMC_GroupSetHomePos: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    // }

    // printf( "Group %d Home Moving\n", groupIndex );

    // if( !_targetAcsPosArrivalCheck( retDevID, groupIndex ) )
    // {
    //    printf( "ERROR! Group%d _targetAcsPosArrivalCheck\n", groupIndex );
    //    goto ERR_SHUTDOWN;
    // }

    // //Group Get Actual PosPcs
    // ret = NMC_GroupGetActualPosAcs( retDevID, groupIndex, &cmdPosPcs );
    // if( ret != 0 )
    // {
    //    printf( "ERROR! NMC_GroupGetActualPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    // }

    // for( initGroupAxisCount = 0; initGroupAxisCount < retGroupAxisCount; ++initGroupAxisCount )
    //    printf( "ActualPosAcs[%d] = %f \n", initGroupAxisCount, cmdPosPcs.pos[initGroupAxisCount] );

    // //Group Get Command PosPcs
    // ret = NMC_GroupGetCommandPosAcs( retDevID, groupIndex, &cmdPosPcs );
    // if( ret != 0 )
    // {
    //    printf( "ERROR! NMC_GroupGetCommandPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription( ret, NULL, 0 ) );
    //    goto ERR_SHUTDOWN;
    // }

    // for( initGroupAxisCount = 0; initGroupAxisCount < retGroupAxisCount; ++initGroupAxisCount )
    //    printf( "CommandPosAcs[%d] = %f \n", initGroupAxisCount, cmdPosPcs.pos[initGroupAxisCount] );

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

// �禡

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
	// MiniBot �ŧi
	U32_T sizeByte = 1;
	I32_T value = 0x01;	// ��

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