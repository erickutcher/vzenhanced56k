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
#include "lite_winmm.h"

#ifndef WINMM_USE_STATIC_LIB

	//pPlaySoundW	_PlaySoundW;

	pmciSendStringW _mciSendStringW;

	pmmioClose		_mmioClose;
	pmmioAscend		_mmioAscend;
	pmmioDescend	_mmioDescend;
	pmmioRead		_mmioRead;
	pmmioOpenW		_mmioOpenW;

	pwaveOutClose	_waveOutClose;
	pwaveOutReset	_waveOutReset;
	pwaveOutWrite	_waveOutWrite;
	pwaveOutPrepareHeader	_waveOutPrepareHeader;
	pwaveOutUnprepareHeader	_waveOutUnprepareHeader;
	pwaveOutOpen	_waveOutOpen;

	//pwaveOutGetDevCapsW _waveOutGetDevCapsW;

	HMODULE hModule_winmm = NULL;

	unsigned char winmm_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeWinMM()
	{
		if ( winmm_state != WINMM_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_winmm = LoadLibraryDEMW( L"winmm.dll" );

		if ( hModule_winmm == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_PlaySoundW, "PlaySoundW" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_mciSendStringW, "mciSendStringW" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_mmioClose, "mmioClose" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_mmioAscend, "mmioAscend" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_mmioDescend, "mmioDescend" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_mmioRead, "mmioRead" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_mmioOpenW, "mmioOpenW" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_waveOutClose, "waveOutClose" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_waveOutReset, "waveOutReset" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_waveOutWrite, "waveOutWrite" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_waveOutPrepareHeader, "waveOutPrepareHeader" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_waveOutUnprepareHeader, "waveOutUnprepareHeader" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_waveOutOpen, "waveOutOpen" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_waveOutGetDevCapsW, "waveOutGetDevCapsW" ) )

		winmm_state = WINMM_STATE_RUNNING;

		return true;
	}

	bool UnInitializeWinMM()
	{
		if ( winmm_state != WINMM_STATE_SHUTDOWN )
		{
			winmm_state = WINMM_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_winmm ) == FALSE ? false : true );
		}

		return true;
	}

#endif
