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
#include "lite_tapi32.h"

#ifndef TAPI32_USE_STATIC_LIB

	plineInitializeExW	_lineInitializeExW;
	plineNegotiateAPIVersion _lineNegotiateAPIVersion;
	plineGetDevCapsW	_lineGetDevCapsW;
	plineOpenW			_lineOpenW;
	plineGetMessage		_lineGetMessage;
	plineGetCallInfoA	_lineGetCallInfoA;
	//plineGetCallInfoW	_lineGetCallInfoW;
	plineAnswer			_lineAnswer;
	plineDrop			_lineDrop;
	plineDeallocateCall	_lineDeallocateCall;
	plineClose			_lineClose;
	plineShutdown		_lineShutdown;

	plineGetIDW			_lineGetIDW;
	plineGetDevConfigW	_lineGetDevConfigW;

	HMODULE hModule_tapi32 = NULL;

	unsigned char tapi32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeTAPI32()
	{
		if ( tapi32_state != TAPI32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_tapi32 = LoadLibraryDEMW( L"tapi32.dll" );

		if ( hModule_tapi32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineInitializeExW, "lineInitializeExW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineNegotiateAPIVersion, "lineNegotiateAPIVersion" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineGetDevCapsW, "lineGetDevCapsW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineOpenW, "lineOpenW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineGetMessage, "lineGetMessage" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineGetCallInfoA, "lineGetCallInfoA" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineGetCallInfoW, "lineGetCallInfoW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineAnswer, "lineAnswer" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineDrop, "lineDrop" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineDeallocateCall, "lineDeallocateCall" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineClose, "lineClose" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineShutdown, "lineShutdown" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineGetIDW, "lineGetIDW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_tapi32, ( void ** )&_lineGetDevConfigW, "lineGetDevConfigW" ) )

		tapi32_state = TAPI32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeTAPI32()
	{
		if ( tapi32_state != TAPI32_STATE_SHUTDOWN )
		{
			tapi32_state = TAPI32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_tapi32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
