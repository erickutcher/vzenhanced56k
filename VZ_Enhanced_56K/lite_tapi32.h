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

#ifndef _LITE_TAPI32_H
#define _LITE_TAPI32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <tapi.h>

//#define TAPI32_USE_STATIC_LIB

#ifdef TAPI32_USE_STATIC_LIB

	//__pragma( comment( lib, "tapi32.lib" ) )

	#define _lineInitializeExW	lineInitializeExW
	#define _lineNegotiateAPIVersion	lineNegotiateAPIVersion
	#define _lineGetDevCapsW	lineGetDevCapsW
	#define _lineOpenW			lineOpenW
	#define _lineGetMessage		lineGetMessage
	#define _lineGetCallInfoA	lineGetCallInfoA
	//#define _lineGetCallInfoW	lineGetCallInfoW
	#define _lineAnswer			lineAnswer
	#define _lineDrop			lineDrop
	#define _lineDeallocateCall	lineDeallocateCall
	#define _lineClose			lineClose
	#define _lineShutdown		lineShutdown

	#define _lineGetIDW			lineGetIDW
	#define _lineGetDevConfigW	lineGetDevConfigW

#else

	#define TAPI32_STATE_SHUTDOWN	0
	#define TAPI32_STATE_RUNNING	1

	typedef LONG ( WINAPI *plineInitializeExW )( LPHLINEAPP lphLineApp, HINSTANCE hInstance, LINECALLBACK lpfnCallback, LPCWSTR lpszFriendlyAppName, LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams );
	typedef LONG ( WINAPI *plineNegotiateAPIVersion )( HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPILowVersion, DWORD dwAPIHighVersion, LPDWORD lpdwAPIVersion, LPLINEEXTENSIONID lpExtensionID );
	typedef LONG ( WINAPI *plineGetDevCapsW )( HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, DWORD dwExtVersion, LPLINEDEVCAPS lpLineDevCaps );
	typedef LONG ( WINAPI *plineOpenW )( HLINEAPP hLineApp, DWORD dwDeviceID, LPHLINE lphLine, DWORD dwAPIVersion, DWORD dwExtVersion, DWORD_PTR dwCallbackInstance, DWORD dwPrivileges, DWORD dwMediaModes, LPLINECALLPARAMS const lpCallParams );
	typedef LONG ( WINAPI *plineGetMessage )( HLINEAPP hLineApp, LPLINEMESSAGE lpMessage, DWORD dwTimeout );
	typedef LONG ( WINAPI *plineGetCallInfoA )( HCALL hCall, LPLINECALLINFO lpCallInfo );
	//typedef LONG ( WINAPI *plineGetCallInfoW )( HCALL hCall, LPLINECALLINFO lpCallInfo );
	typedef LONG ( WINAPI *plineAnswer )( HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize );
	typedef LONG ( WINAPI *plineDrop )( HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize );
	typedef LONG ( WINAPI *plineDeallocateCall )( HCALL hCall );
	typedef LONG ( WINAPI *plineClose )( HLINE hLine );
	typedef LONG ( WINAPI *plineShutdown )( HLINEAPP hLineApp );

	typedef LONG ( WINAPI *plineGetIDW )( HLINE hLine, DWORD dwAddressID, HCALL hCall, DWORD dwSelect, LPVARSTRING lpDeviceID, LPCWSTR lpszDeviceClass );
	typedef LONG ( WINAPI *plineGetDevConfigW )( DWORD dwDeviceID, LPVARSTRING lpDeviceConfig, LPCWSTR lpszDeviceClass );

	extern plineInitializeExW	_lineInitializeExW;
	extern plineNegotiateAPIVersion _lineNegotiateAPIVersion;
	extern plineGetDevCapsW		_lineGetDevCapsW;
	extern plineOpenW			_lineOpenW;
	extern plineGetMessage		_lineGetMessage;
	extern plineGetCallInfoA	_lineGetCallInfoA;
	//extern plineGetCallInfoW	_lineGetCallInfoW;
	extern plineAnswer			_lineAnswer;
	extern plineDrop			_lineDrop;
	extern plineDeallocateCall	_lineDeallocateCall;
	extern plineClose			_lineClose;
	extern plineShutdown		_lineShutdown;

	extern plineGetIDW			_lineGetIDW;
	extern plineGetDevConfigW	_lineGetDevConfigW;

	extern unsigned char tapi32_state;

	bool InitializeTAPI32();
	bool UnInitializeTAPI32();

#endif

#endif
