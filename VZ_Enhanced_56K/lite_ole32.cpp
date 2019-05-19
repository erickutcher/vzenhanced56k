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

#include "lite_dlls.h"
#include "lite_ole32.h"

#ifndef OLE32_USE_STATIC_LIB

	//pCoTaskMemFree	_CoTaskMemFree;

	//pOleInitialize	_OleInitialize;
	//pOleUninitialize	_OleUninitialize;

	pCoInitializeEx	_CoInitializeEx;
	pCoUninitialize	_CoUninitialize;

	HMODULE hModule_ole32 = NULL;

	unsigned char ole32_state = OLE32_STATE_SHUTDOWN;

	bool InitializeOle32()
	{
		if ( ole32_state != OLE32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_ole32 = LoadLibraryDEMW( L"ole32.dll" );

		if ( hModule_ole32 == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ole32, ( void ** )&_CoTaskMemFree, "CoTaskMemFree" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ole32, ( void ** )&_OleInitialize, "OleInitialize" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ole32, ( void ** )&_OleUninitialize, "OleUninitialize" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ole32, ( void ** )&_CoInitializeEx, "CoInitializeEx" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ole32, ( void ** )&_CoUninitialize, "CoUninitialize" ) )

		ole32_state = OLE32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeOle32()
	{
		if ( ole32_state != OLE32_STATE_SHUTDOWN )
		{
			ole32_state = OLE32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_ole32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
