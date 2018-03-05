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

#ifndef _LITE_ADVAPI32_H
#define _LITE_ADVAPI32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <wincrypt.h>

//#define ADVAPI32_USE_STATIC_LIB

#ifdef ADVAPI32_USE_STATIC_LIB

	//__pragma( comment( lib, "advapi32.lib" ) )

	#define _CryptAcquireContextW	CryptAcquireContextW
	#define _CryptGenRandom			CryptGenRandom
	#define _CryptReleaseContext	CryptReleaseContext

	#define _GetUserNameW			GetUserNameW

#else

	#define ADVAPI32_STATE_SHUTDOWN		0
	#define ADVAPI32_STATE_RUNNING		1

	typedef BOOL ( WINAPI *pCryptAcquireContextW )( HCRYPTPROV *phProv, LPCTSTR pszContainer, LPCTSTR pszProvider, DWORD dwProvType, DWORD dwFlags );
	typedef BOOL ( WINAPI *pCryptGenRandom )( HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer );
	typedef BOOL ( WINAPI *pCryptReleaseContext )( HCRYPTPROV hProv, DWORD dwFlags );

	typedef BOOL ( WINAPI *pGetUserNameW )( LPTSTR lpBuffer, LPDWORD lpnSize );

	extern pCryptAcquireContextW	_CryptAcquireContextW;
	extern pCryptGenRandom			_CryptGenRandom;
	extern pCryptReleaseContext		_CryptReleaseContext;

	extern pGetUserNameW			_GetUserNameW;

	extern unsigned char advapi32_state;

	bool InitializeAdvApi32();
	bool UnInitializeAdvApi32();

#endif

#endif
