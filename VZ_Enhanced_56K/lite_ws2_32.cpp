/*
	VZ Enhanced 56K is a caller ID notifier that can block phone calls.
	Copyright (C) 2013-2018 Eric Kutcher

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "lite_dlls.h"
#include "lite_ws2_32.h"

unsigned char ws2_32_state = 0;	// 0 = Not running, 1 = running.

#ifndef WS2_32_USE_STATIC_LIB

	pWSAStartup		_WSAStartup;
	pWSACleanup		_WSACleanup;

	//pWSAWaitForMultipleEvents	_WSAWaitForMultipleEvents;
	//pWSACreateEvent	_WSACreateEvent;
	//pWSASetEvent	_WSASetEvent;
	//pWSAResetEvent	_WSAResetEvent;
	//pWSAEventSelect	_WSAEventSelect;
	//pWSAEnumNetworkEvents	_WSAEnumNetworkEvents;
	//pWSACloseEvent	_WSACloseEvent;

	//pWSAGetLastError	_WSAGetLastError;

	psocket			_socket;
	pconnect		_connect;
	pshutdown		_shutdown;
	pclosesocket	_closesocket;

	psetsockopt		_setsockopt;

	//pioctlsocket	_ioctlsocket;

	psend			_send;
	precv			_recv;

	//pselect			_select;

	pgetaddrinfo	_getaddrinfo;
	pfreeaddrinfo	_freeaddrinfo;
	pgetpeername	_getpeername;

	pinet_ntoa		_inet_ntoa;

	HMODULE hModule_ws2_32 = NULL;

	bool InitializeWS2_32()
	{
		if ( ws2_32_state != WS2_32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_ws2_32 = LoadLibraryDEMW( L"ws2_32.dll" );

		if ( hModule_ws2_32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSAStartup, "WSAStartup" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSACleanup, "WSACleanup" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSAWaitForMultipleEvents, "WSAWaitForMultipleEvents" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSACreateEvent, "WSACreateEvent" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSASetEvent, "WSASetEvent" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSAResetEvent, "WSAResetEvent" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSAEventSelect, "WSAEventSelect" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSAEnumNetworkEvents, "WSAEnumNetworkEvents" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSACloseEvent, "WSACloseEvent" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_WSAGetLastError, "WSAGetLastError" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_socket, "socket" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_connect, "connect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_shutdown, "shutdown" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_closesocket, "closesocket" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_setsockopt, "setsockopt" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_ioctlsocket, "ioctlsocket" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_send, "send" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_recv, "recv" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_select, "select" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_getaddrinfo, "getaddrinfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_freeaddrinfo, "freeaddrinfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_getpeername, "getpeername" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32, ( void ** )&_inet_ntoa, "inet_ntoa" ) )

		StartWS2_32();

		return true;
	}

	bool UnInitializeWS2_32()
	{
		if ( ws2_32_state != WS2_32_STATE_SHUTDOWN )
		{
			EndWS2_32();

			return ( FreeLibrary( hModule_ws2_32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif

void StartWS2_32()
{
	if ( ws2_32_state != WS2_32_STATE_RUNNING )
	{
		// Initialize Winsock
		WSADATA wsaData;
		_WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	}

	ws2_32_state = WS2_32_STATE_RUNNING;
}

void EndWS2_32()
{
	if ( ws2_32_state != WS2_32_STATE_SHUTDOWN )
	{
		// Cleanup Winsock.
		_WSACleanup();
	}

	ws2_32_state = WS2_32_STATE_SHUTDOWN;
}
