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
#include "lite_comctl32.h"

#ifndef COMCTL32_USE_STATIC_LIB

	pImageList_Destroy		_ImageList_Destroy;

	HMODULE hModule_comctl32 = NULL;

	unsigned char comctl32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeComCtl32()
	{
		if ( comctl32_state != COMCTL32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_comctl32 = LoadLibraryDEMW( L"comctl32.dll" );

		if ( hModule_comctl32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_Destroy, "ImageList_Destroy" ) )

		comctl32_state = COMCTL32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeComCtl32()
	{
		if ( comctl32_state != COMCTL32_STATE_SHUTDOWN )
		{
			comctl32_state = COMCTL32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_comctl32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
