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

#ifndef _LITE_WINSOCK_H
#define _LITE_WINSOCK_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

//#define WS2_32_USE_STATIC_LIB

#define WS2_32_STATE_SHUTDOWN		0
#define WS2_32_STATE_RUNNING		1

#ifdef WS2_32_USE_STATIC_LIB

	//__pragma( comment( lib, "ws2_32.lib" ) )

	#define _WSAStartup		WSAStartup
	#define _WSACleanup		WSACleanup

	//#define _WSAWaitForMultipleEvents	WSAWaitForMultipleEvents
	//#define _WSACreateEvent	WSACreateEvent
	//#define _WSASetEvent	WSASetEvent
	//#define _WSAResetEvent	WSAResetEvent
	//#define _WSAEventSelect WSAEventSelect
	//#define _WSAEnumNetworkEvents	WSAEnumNetworkEvents
	//#define _WSACloseEvent	WSACloseEvent

	//#define _WSAGetLastError	WSAGetLastError

	#define _socket			socket
	#define _connect		connect
	#define _shutdown		shutdown
	#define _closesocket	closesocket

	#define _setsockopt		setsockopt

	//#define _ioctlsocket	ioctlsocket

	#define _send			send
	#define _recv			recv

	//#define _select			select

	#define _getaddrinfo	getaddrinfo
	#define _freeaddrinfo	freeaddrinfo
	#define _getpeername	getpeername

	#define _inet_ntoa		inet_ntoa

#else

	typedef int ( WSAAPI *pWSAStartup )( WORD wVersionRequested, LPWSADATA lpWSAData );
	typedef int ( WSAAPI *pWSACleanup )( void );

	//typedef DWORD ( WSAAPI *pWSAWaitForMultipleEvents )( DWORD cEvents, const WSAEVENT *lphEvents, BOOL fWaitAll, DWORD dwTimeout, BOOL fAlertable );
	//typedef WSAEVENT ( WSAAPI *pWSACreateEvent )( void );
	//typedef BOOL ( WSAAPI *pWSASetEvent )( WSAEVENT hEvent );
	//typedef BOOL ( WSAAPI *pWSAResetEvent )( WSAEVENT hEvent );
	//typedef int ( WSAAPI *pWSAEventSelect )( SOCKET s, WSAEVENT hEventObject, long lNetworkEvents );
	//typedef int ( WSAAPI *pWSAEnumNetworkEvents )( SOCKET s, WSAEVENT hEventObject, LPWSANETWORKEVENTS lpNetworkEvents );
	//typedef BOOL ( WSAAPI *pWSACloseEvent )( WSAEVENT hEvent );

	//typedef int ( WSAAPI *pWSAGetLastError )( void );

	typedef SOCKET ( WSAAPI *psocket )( int af, int type, int protocol );
	typedef int ( WSAAPI *pconnect )( SOCKET s, const struct sockaddr *name, int namelen );
	typedef int ( WSAAPI *pshutdown )( SOCKET s, int how );
	typedef int ( WSAAPI *pclosesocket )( SOCKET s );

	typedef int ( WSAAPI *psetsockopt )( SOCKET s, int level, int optname, const char *optval, int optlen );

	//typedef int ( WSAAPI *pioctlsocket )( SOCKET s, long cmd, u_long *argp );

	typedef int ( WSAAPI *psend )( SOCKET s, const char *buf, int len, int flags );
	typedef int ( WSAAPI *precv )( SOCKET s, const char *buf, int len, int flags );

	//typedef int ( WSAAPI *pselect )( int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout );

	typedef int ( WSAAPI *pgetaddrinfo )( PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA *pHints, PADDRINFOA *ppResult );
	typedef void ( WSAAPI *pfreeaddrinfo )( struct addrinfo *ai );
	typedef int ( WSAAPI *pgetpeername )( SOCKET s, struct sockaddr *name, int *namelen );

	typedef char *FAR ( WSAAPI *pinet_ntoa )( struct in_addr in );

	extern pWSAStartup		_WSAStartup;
	extern pWSACleanup		_WSACleanup;

	//extern pWSAWaitForMultipleEvents	_WSAWaitForMultipleEvents;
	//extern pWSACreateEvent	_WSACreateEvent;
	//extern pWSASetEvent		_WSASetEvent;
	//extern pWSAResetEvent	_WSAResetEvent;
	//extern pWSAEventSelect	_WSAEventSelect;
	//extern pWSAEnumNetworkEvents	_WSAEnumNetworkEvents;
	//extern pWSACloseEvent	_WSACloseEvent;

	//extern pWSAGetLastError	_WSAGetLastError;

	extern psocket			_socket;
	extern pconnect			_connect;
	extern pshutdown		_shutdown;
	extern pclosesocket		_closesocket;

	extern psetsockopt		_setsockopt;

	//extern pioctlsocket		_ioctlsocket;

	extern psend			_send;
	extern precv			_recv;

	//extern pselect			_select;

	extern pgetaddrinfo		_getaddrinfo;
	extern pfreeaddrinfo	_freeaddrinfo;
	extern pgetpeername		_getpeername;

	extern pinet_ntoa		_inet_ntoa;

	bool InitializeWS2_32();
	bool UnInitializeWS2_32();

#endif

void StartWS2_32();
void EndWS2_32();

extern unsigned char ws2_32_state;

#endif
