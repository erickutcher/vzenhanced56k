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

#ifndef _LITE_WINMM_H
#define _LITE_WINMM_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <mmsystem.h>

//#define WINMM_USE_STATIC_LIB

#ifdef WINMM_USE_STATIC_LIB

	//__pragma( comment( lib, "winmm.lib" ) )

	//#define _PlaySoundW	PlaySoundW

	#define _mciSendStringW	mciSendStringW

	#define _mmioClose		mmioClose
	#define _mmioAscend		mmioAscend
	#define _mmioDescend	mmioDescend
	#define _mmioRead		mmioRead
	#define _mmioOpenW		mmioOpenW

	#define _waveOutClose	waveOutClose
	#define _waveOutReset	waveOutReset
	#define _waveOutWrite	waveOutWrite
	#define _waveOutPrepareHeader	waveOutPrepareHeader
	#define _waveOutUnprepareHeader	waveOutUnprepareHeader
	#define _waveOutOpen	waveOutOpen

	//#define _waveOutGetDevCapsW	waveOutGetDevCapsW

#else

	#define WINMM_STATE_SHUTDOWN	0
	#define WINMM_STATE_RUNNING		1

	//typedef BOOL ( WINAPI *pPlaySoundW )( LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound );

	typedef MCIERROR ( WINAPI *pmciSendStringW )( LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback );

	typedef MMRESULT ( WINAPI *pmmioClose )( HMMIO hmmio, UINT wFlags );
	typedef MMRESULT ( WINAPI *pmmioAscend )( HMMIO hmmio, LPMMCKINFO lpck, UINT wFlags );
	typedef MMRESULT ( WINAPI *pmmioDescend )( HMMIO hmmio, LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags );
	typedef LONG ( WINAPI *pmmioRead )( HMMIO hmmio, HPSTR pch, LONG cch );
	typedef HMMIO ( WINAPI *pmmioOpenW )( LPWSTR szFilename, LPMMIOINFO lpmmioinfo, DWORD dwOpenFlags );

	typedef MMRESULT ( WINAPI *pwaveOutClose )( HWAVEOUT hwo );
	typedef MMRESULT ( WINAPI *pwaveOutReset )( HWAVEOUT hwo );
	typedef MMRESULT ( WINAPI *pwaveOutWrite )( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh );
	typedef MMRESULT ( WINAPI *pwaveOutPrepareHeader )( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh );
	typedef MMRESULT ( WINAPI *pwaveOutUnprepareHeader )( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh );
	typedef MMRESULT ( WINAPI *pwaveOutOpen )( LPHWAVEOUT phwo, UINT_PTR uDeviceID, LPWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwCallbackInstance, DWORD fdwOpen );

	//typedef MMRESULT ( WINAPI *pwaveOutGetDevCapsW )( UINT_PTR uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc );

	//extern pPlaySoundW	_PlaySoundW;

	extern pmciSendStringW _mciSendStringW;

	extern pmmioClose		_mmioClose;
	extern pmmioAscend		_mmioAscend;
	extern pmmioDescend		_mmioDescend;
	extern pmmioRead		_mmioRead;
	extern pmmioOpenW		_mmioOpenW;

	extern pwaveOutClose	_waveOutClose;
	extern pwaveOutReset	_waveOutReset;
	extern pwaveOutWrite	_waveOutWrite;
	extern pwaveOutPrepareHeader	_waveOutPrepareHeader;
	extern pwaveOutUnprepareHeader	_waveOutUnprepareHeader;
	extern pwaveOutOpen		_waveOutOpen;

	//extern pwaveOutGetDevCapsW _waveOutGetDevCapsW;

	extern unsigned char winmm_state;

	bool InitializeWinMM();
	bool UnInitializeWinMM();

#endif

#endif
