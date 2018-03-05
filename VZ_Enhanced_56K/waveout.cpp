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

#include "globals.h"
#include "waveout.h"

#include "lite_ntdll.h"

HANDLE g_event_playback = NULL;

WAVEDATA *g_wave_data = NULL;
WAVEFORMATEX *g_wave_format = NULL;
ringtoneinfo *g_last_recording = NULL;

WAVEDATA *LoadWAVEInfo( WAVEFORMATEX *wave_format, wchar_t *file_path )
{
	if ( wave_format == NULL || file_path == NULL )
	{
		return NULL;
	}

	MMCKINFO mmckinfoParent, mmckinfoSubchunk;

	DWORD format_size, chunk_size;

	WAVEDATA *wave_head = NULL, *wave_cur = NULL, *wave_prev = NULL;

	// Open the given file for reading using buffered I/O.
	HMMIO hMMIO = _mmioOpenW( file_path, NULL, MMIO_READ | MMIO_ALLOCBUF );
	if ( hMMIO == NULL )
	{
		return NULL;
	}

	// Locate a "RIFF" chunk with a "WAVE" form type to make sure it's a WAVE file.
	mmckinfoParent.fccType = mmioFOURCC( 'W', 'A', 'V', 'E' );
	if ( _mmioDescend( hMMIO, &mmckinfoParent, NULL, MMIO_FINDRIFF) != MMSYSERR_NOERROR )
	{
		goto CLEANUP;
	}

	//  Find the format chunk (form type "fmt "). It should be a subchunk of the "RIFF" parent chunk.
	mmckinfoSubchunk.ckid = mmioFOURCC( 'f', 'm', 't', ' ' );
	if ( _mmioDescend( hMMIO, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK ) != MMSYSERR_NOERROR )
	{
		goto CLEANUP;
	}

	// Get the size of the format chunk.
	format_size = mmckinfoSubchunk.cksize;
	if ( wave_format->cbSize < format_size )
	{
		goto CLEANUP;
	}

	// Read the format chunk.
	if ( _mmioRead( hMMIO, ( HPSTR )wave_format, format_size ) != ( LONG )format_size )
	{
		goto CLEANUP;
	}

	// Ascend out of the format subchunk.
	_mmioAscend( hMMIO, &mmckinfoSubchunk, 0 );

	// Find the "data" subchunk.
	mmckinfoSubchunk.ckid = mmioFOURCC( 'd', 'a', 't', 'a' );
	if ( _mmioDescend( hMMIO, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK ) != MMSYSERR_NOERROR )
	{
		goto CLEANUP;
	}

	// Get the size of the data subchunk.
	if ( mmckinfoSubchunk.cksize == 0 )
	{
		goto CLEANUP;
	}

	// Read the data and allocate WAVEDATA buffers
	chunk_size = ( wave_format->nAvgBytesPerSec / 4 );
	chunk_size -= chunk_size % wave_format->nBlockAlign;
	if ( chunk_size < wave_format->nBlockAlign )
	{
		goto CLEANUP;
	}

	// Create a linked list of wave headers.
	while ( true )
	{
		wave_cur = ( WAVEDATA * )GlobalAlloc( GPTR, chunk_size + sizeof( WAVEDATA ) );
		wave_cur->wavehdr.lpData = ( LPSTR )wave_cur->data;

		// Read the waveform data subchunk.
		LONG read = _mmioRead( hMMIO, ( HPSTR )wave_cur->data, chunk_size );

		// Read failed, or the was no more data to read.
		if ( read == -1 || read == 0 )
		{
			GlobalFree( wave_cur );

			break;
		}

		wave_cur->wavehdr.dwBufferLength = read;

		// Add our wave data to the linked list.
		if ( wave_prev != NULL )
		{
			wave_prev->next = wave_cur;
		}
		else if ( wave_head == NULL )
		{
			wave_head = wave_cur;
		}

		wave_prev = wave_cur;

		// If we've read less than our desired chunk size, then there's no more to read.
		if ( ( DWORD )read != chunk_size )
		{
			break;
		}
	}

CLEANUP:

	_mmioClose( hMMIO, 0 );

	return wave_head;
}

void CleanupWAVEInfo()
{
	// Clean up our wave header list.
	while ( g_wave_data != NULL )
	{
		WAVEDATA *current_wave_data = g_wave_data;

		g_wave_data = current_wave_data->next;

		GlobalFree( current_wave_data );
	}
}
void CleanupStreamData( HWAVEOUT hWO )
{
	if ( hWO != NULL )
	{
		_waveOutReset( hWO );

		//Sleep( 500 );  // Give it 1/2 sec to actually reset.

		_waveOutClose( hWO );

		//Sleep( 500 ); // Give it 1/2 sec to clean up.
	}
}

int ProcessHeader( HWAVEOUT hWO, WAVEHDR *wave_header )
{
	if ( hWO == NULL || wave_header == NULL )
	{
		return -1;
	}

	// Wait until the buffer has finished being processed.
	/*while ( wave_header->dwFlags & WHDR_INQUEUE )
	{
		Sleep( 250 );
	}

	// Unprepare the wave header if it was processed.
	if ( wave_header->dwFlags & WHDR_DONE )
	{
		if ( _waveOutUnprepareHeader( hWO, wave_header, sizeof( WAVEHDR ) ) != MMSYSERR_NOERROR )
		{
			return -1;
		}
		else
		{
			// Reset our flags.
			wave_header->dwFlags = 0;

			// We're finished processing all the wave headers.
			return 1;
		}
	}*/

	// Reset our flags.
	wave_header->dwFlags = 0;

	// Prepare a wave header for processing.
	if ( _waveOutPrepareHeader( hWO, wave_header, sizeof( WAVEHDR ) ) != MMSYSERR_NOERROR )
	{
		return -1;
	}

	// Send the wave header to the output device.
	if ( _waveOutWrite( hWO, wave_header, sizeof( WAVEHDR ) ) != MMSYSERR_NOERROR )
	{
		return -1;
	}

	// Make sure the wave header was queued for processing.
	if ( !( wave_header->dwFlags & WHDR_INQUEUE ) )
	{
		return -1;
	}

	return 0;
}

void BeginPlayback( HWAVEOUT hWO, WAVEDATA *wave_data )
{
	while ( true )
	{
		WaitForSingleObject( g_event_playback, 1000 );

		// Exit early if we're shutting down the program or the remote party disconnected before we finished playing the recording.
		if ( kill_telephony_thread_flag != 0 || kill_accepted_call_thread_flag != 0 )
		{
			return;
		}

		// Process two headers at a time. Prevents the audio from crackling.
		for ( unsigned char i = 0; i < 2; ++i )
		{
			// Write our data to the output device.
			if ( ProcessHeader( hWO, &wave_data->wavehdr ) != 0 )
			{
				return;
			}
			else
			{
				// Go to the next wave header to process.
				wave_data = wave_data->next;

				// If there's no more to process, then we're done processing. 
				if ( wave_data == NULL )
				{
					return;
				}
			}
		}
	}
}

void WaitForPlaybackCompletion( HWAVEOUT hWO, WAVEDATA *wave_data )
{
	while ( wave_data != NULL )
	{
		WAVEHDR *wave_header = &wave_data->wavehdr;

		// Wait until the buffer has finished being processed.
		while ( wave_header->dwFlags & WHDR_INQUEUE )
		{
			Sleep( 250 );
		}

		// Unprepare the wave header if it was processed.
		_waveOutUnprepareHeader( hWO, wave_header, sizeof( WAVEHDR ) );

		wave_data = wave_data->next;
	}
}

void PlaybackRecording( DWORD device_id, ringtoneinfo *recording ) 
{
	if ( recording != NULL && recording->ringtone_path != NULL )
	{
		HWAVEOUT wave_out = NULL;

		// Get our wave file information if it isn't set or currently cached.
		if ( g_last_recording != recording )
		{
			_memzero( g_wave_format, sizeof( WAVEFORMATEX ) + 4096 );
			g_wave_format->cbSize = ( sizeof( WAVEFORMATEX ) + 4096 );

			CleanupWAVEInfo();

			g_wave_data = LoadWAVEInfo( g_wave_format, recording->ringtone_path );
			if ( g_wave_data == NULL )
			{
				goto END;
			}
		}

		// Save the last recording info so we can skip loading the wave info and wave format if it's the same.
		g_last_recording = recording;

		// We can only play files with a sample rate of 7200 Hz, 8000 Hz, or 11025 Hz and one channel (mono).
		if ( g_wave_format->nChannels != 1 || ( g_wave_format->nSamplesPerSec != 7200 &&
												g_wave_format->nSamplesPerSec != 8000 &&
												g_wave_format->nSamplesPerSec != 11025 ) )
		{
			goto END;
		}

		// Open a waveform output device.
		if ( _waveOutOpen( &wave_out, device_id, g_wave_format, ( DWORD_PTR )g_event_playback, 0, CALLBACK_EVENT ) != MMSYSERR_NOERROR )
		{
			goto END;
		}

		for ( unsigned char i = 0; i < cfg_repeat_recording; ++i )
		{
			// Exit early if we're shutting down the program or the remote party disconnected before we finished playing the recording.
			if ( kill_telephony_thread_flag != 0 || kill_accepted_call_thread_flag != 0 )
			{
				break;
			}

			// Play our wave data on the output device.
			BeginPlayback( wave_out, g_wave_data );

			// Wait for the wave data to finish playing. Some of the data may be queued.
			WaitForPlaybackCompletion( wave_out, g_wave_data );
		}

	END:

		CleanupStreamData( wave_out );
	}
}

bool InitializeWAVEOut( DWORD device_id )
{
	bool init_winmm = false;
	#ifndef WINMM_USE_STATIC_LIB
		if ( winmm_state == WINMM_STATE_SHUTDOWN )
		{
			init_winmm = InitializeWinMM();
		}
	#endif

	if ( init_winmm )
	{
		g_event_playback = CreateEvent( NULL, FALSE, FALSE, NULL );

		g_wave_format = ( WAVEFORMATEX * )GlobalAlloc( GPTR, sizeof( WAVEFORMATEX ) + 4096 );

		//WAVEOUTCAPS woc;
		//_memzero( &woc, sizeof( WAVEOUTCAPS ) );
		//_waveOutGetDevCapsW( device_id, &woc, sizeof( WAVEOUTCAPS ) );
	}

	return init_winmm;
}

void UnInitializeWAVEOut()
{
	if ( g_wave_format != NULL )
	{
		GlobalFree( g_wave_format );
		g_wave_format = NULL;
	}

	CleanupWAVEInfo();

	if ( g_event_playback != NULL )
	{
		CloseHandle( g_event_playback );
		g_event_playback = NULL;
	}
}
