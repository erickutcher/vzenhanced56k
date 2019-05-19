/*
	VZ Enhanced 56K is a caller ID notifier that can block phone calls.
	Copyright (C) 2013-2019 Eric Kutcher

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

#ifndef _LITE_UXTHEME_H
#define _LITE_UXTHEME_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <uxtheme.h>
#include <vsstyle.h>

//#define UXTHEME_USE_STATIC_LIB

#ifdef UXTHEME_USE_STATIC_LIB

	//__pragma( comment( lib, "uxtheme.lib" ) )

	#define _OpenThemeData			OpenThemeData
	#define _CloseThemeData			CloseThemeData
	#define _DrawThemeBackground	DrawThemeBackground

#else

	#define UXTHEME_STATE_SHUTDOWN		0
	#define UXTHEME_STATE_RUNNING		1

	typedef HTHEME ( WINAPI *pOpenThemeData )( HWND hwnd, LPCWSTR pszClassList );
	typedef HRESULT ( WINAPI *pCloseThemeData )( HTHEME hTheme );
	typedef HRESULT ( WINAPI *pDrawThemeBackground )( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect );

	extern pOpenThemeData			_OpenThemeData;
	extern pCloseThemeData			_CloseThemeData;
	extern pDrawThemeBackground		_DrawThemeBackground;

	extern unsigned char uxtheme_state;

	bool InitializeUXTheme();
	bool UnInitializeUXTheme();

#endif

#endif
