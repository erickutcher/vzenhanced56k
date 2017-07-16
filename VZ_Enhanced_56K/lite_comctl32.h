/*
	VZ Enhanced 56K is a caller ID notifier that can block phone calls.
	Copyright (C) 2013-2017 Eric Kutcher

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

#ifndef _LITE_COMCTL32_H
#define _LITE_COMCTL32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>

//#define COMCTL32_USE_STATIC_LIB

#ifdef COMCTL32_USE_STATIC_LIB

	//__pragma( comment( lib, "comctl32.lib" ) )

	#define _ImageList_Destroy		ImageList_Destroy

#else

	#define COMCTL32_STATE_SHUTDOWN		0
	#define COMCTL32_STATE_RUNNING		1

	typedef BOOL ( WINAPI *pImageList_Destroy )( HIMAGELIST himl );

	extern pImageList_Destroy		_ImageList_Destroy;

	extern unsigned char comctl32_state;

	bool InitializeComCtl32();
	bool UnInitializeComCtl32();

#endif

#endif
