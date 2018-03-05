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

#ifndef _ML_UTILITIES_H
#define _ML_UTILITIES_H

// Message Log Window messages (wParam mask values)
#define ML_WRITE_OUTPUT_W	0x01	// Write wchar_t text to the message log.
#define ML_WRITE_OUTPUT_A	0x02	// Write char text to the message log.
#define ML_MESSAGE_BOX_W	0x04	// Display a wchar_t message box.
#define ML_MESSAGE_BOX_A	0x08	// Display a char message box.
#define ML_FREE_INPUT		0x10	// Free the input text

#define ML_ERROR			0x20
#define ML_WARNING			0x40
#define ML_NOTICE			0x80
//

#define MAX_ITEM_COUNT		10000	// Should handle at least a day's worth of entries.
#define PROCESS_ITEM_COUNT	32

struct MESSAGE_LOG_INFO
{
	ULARGE_INTEGER date_and_time;
	wchar_t *w_date_and_time;
	wchar_t *message;
	wchar_t *w_level;
	unsigned char level;
};

THREAD_RETURN MessageLogManager( void *pArguments );
THREAD_RETURN create_message_log_csv_file( void *file_path );
THREAD_RETURN clear_message_log( void *pArguments );
THREAD_RETURN copy_message_log( void *pArguments );

void save_message_log_csv_file( wchar_t *file_path );

void cleanup_message_log_queue();
void cleanup_message_log();

void kill_ml_update_thread();
void kill_ml_worker_thread();

void HandleMessageLogInput( DWORD type, void *data );

extern HANDLE ml_update_trigger_semaphore;

extern HWND g_hWnd_message_log_list;
extern HWND g_hWnd_save_log;
extern HWND g_hWnd_clear_log;

extern bool skip_message_log_draw;

extern bool add_to_ml_top;	// Add to the top or bottom of the message log depending on the date/time column sort value (up or down).

#define MESSAGE_LOG_OUTPUT( level, output ) if ( cfg_enable_message_log ) { _SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_W | level, ( LPARAM )output ); }
#define MESSAGE_LOG_OUTPUT_MB( level, output ) _SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ( cfg_enable_message_log ? ML_WRITE_OUTPUT_W : 0 ) | ML_MESSAGE_BOX_W | level, ( LPARAM )output );
#define MESSAGE_LOG_OUTPUT_FREE_A( level, output ) if ( cfg_enable_message_log ) { _SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_FREE_INPUT | level, ( LPARAM )output ); } else { GlobalFree( output ); }

#endif
