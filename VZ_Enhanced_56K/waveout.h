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

#ifndef _WAVEOUT_H
#define _WAVEOUT_H

#include "lite_winmm.h"

struct WAVEDATA
{
	WAVEHDR wavehdr;
	WAVEDATA *next;
	BYTE data[ 1 ];
};

WAVEDATA *LoadWaveInfo( WAVEFORMATEX *wave_format, wchar_t *file_path );
void CleanupStreamData( HWAVEOUT hWO );
int ProcessHeader( HWAVEOUT hWO, WAVEHDR *wave_header );

void BeginPlayback( WAVEDATA *wave_data );
void WaitForPlaybackCompletion( WAVEDATA *wave_data );

void PlaybackRecording( DWORD device_id, ringtoneinfo *recording );

bool InitializeWAVEOut( DWORD device_id );
void UnInitializeWAVEOut();

#endif
