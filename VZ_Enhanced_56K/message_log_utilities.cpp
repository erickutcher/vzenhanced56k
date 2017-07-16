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

#include "globals.h"
#include "message_log_utilities.h"
#include "utilities.h"

HANDLE ml_update_semaphore = NULL;
HANDLE ml_worker_semaphore = NULL;

HANDLE ml_update_trigger_semaphore = NULL;

bool kill_ml_update_thread_flag = false;
bool kill_ml_worker_thread_flag = false;

bool in_ml_update_thread = false;
bool in_ml_worker_thread = false;

CRITICAL_SECTION ml_cs;	// Queues additional worker threads.
CRITICAL_SECTION ml_update_cs;
CRITICAL_SECTION ml_queue_cs;

DoublyLinkedList *message_log_queue = NULL;

unsigned char update_message_log_state = 0;

// Overwrites the string argument and returns it when done.
wchar_t *decode_message_event_w( wchar_t *string )
{
	wchar_t *p = NULL;
	wchar_t *q = NULL;

	bool add_space = true;

	if ( string == NULL )
	{
		return NULL;
	}

	for ( p = q = string; *p != NULL; ++p ) 
	{
		if ( *p == L'\r' || *p == L'\n' || *p == L'\t' )
		{
			if ( add_space )
			{
				add_space = false;

				*q = L' ';
				++q;
			}
		}
		else
		{
			add_space = true;

			*q = *p;
			++q;
		}
	}

	*q = 0;	// Sanity.

	return string;
}

void Processing_ML_Window( bool enable )
{
	if ( !enable )
	{
		_EnableWindow( g_hWnd_message_log_list, FALSE );			// Prevent any interaction with the listview while we're processing.
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, TRUE, 0 );	// SetCursor only works from the main thread. Set it to an arrow with hourglass.
	}
	else
	{
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, FALSE, 0 );	// Reset the cursor.
		_EnableWindow( g_hWnd_message_log_list, TRUE );				// Allow the listview to be interactive. Also forces a refresh to update the item count column.
		_SetFocus( g_hWnd_message_log_list );						// Give focus back to the listview to allow shortcut keys.
	}
}

void cleanup_message_log_queue()
{
	// Cleanup any message log items that were queued and not added to the list. (Unlikely to occur)
	DoublyLinkedList *mlq_node = message_log_queue;
	while ( mlq_node != NULL )
	{
		DoublyLinkedList *del_mlq_node = mlq_node;

		mlq_node = mlq_node->next;

		MESSAGE_LOG_INFO *mli = ( MESSAGE_LOG_INFO * )del_mlq_node->data;
		if ( mli != NULL )
		{
			GlobalFree( mli->w_date_and_time );
			GlobalFree( mli->w_level );
			GlobalFree( mli->message );
			GlobalFree( mli );
		}

		GlobalFree( del_mlq_node );
	}
}

void cleanup_message_log()
{
	// Get the number of items in the listview.
	int num_items = _SendMessageW( g_hWnd_message_log_list, LVM_GETITEMCOUNT, 0, 0 );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	// Go through each item, and free their lParam values.
	for ( lvi.iItem = 0; lvi.iItem < num_items; ++lvi.iItem )
	{
		_SendMessageW( g_hWnd_message_log_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

		MESSAGE_LOG_INFO *mli = ( MESSAGE_LOG_INFO * )lvi.lParam;
		if ( mli != NULL )
		{
			GlobalFree( mli->w_date_and_time );
			GlobalFree( mli->w_level );
			GlobalFree( mli->message );
			GlobalFree( mli );
		}
	}
}

void kill_ml_update_thread()
{
	if ( in_ml_update_thread )
	{
		// This semaphore will be released when the thread gets killed.
		ml_update_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		kill_ml_update_thread_flag = true;	// Causes secondary threads to cease processing and release the semaphore.

		if ( ml_update_trigger_semaphore != NULL )
		{
			ReleaseSemaphore( ml_update_trigger_semaphore, 1, NULL );
		}

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( ml_update_semaphore, 5000 );
		CloseHandle( ml_update_semaphore );
		ml_update_semaphore = NULL;
	}
}

void kill_ml_worker_thread()
{
	if ( in_ml_worker_thread )
	{
		// This semaphore will be released when the thread gets killed.
		ml_worker_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		kill_ml_worker_thread_flag = true;	// Causes secondary threads to cease processing and release the semaphore.

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( ml_worker_semaphore, 5000 );
		CloseHandle( ml_worker_semaphore );
		ml_worker_semaphore = NULL;
	}
}

THREAD_RETURN MessageLogManager( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &ml_update_cs );

	in_ml_update_thread = true;

	DoublyLinkedList *mlq_node = NULL;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
	lvi.iSubItem = 0;

	while ( !kill_ml_update_thread_flag && update_message_log_state == 1 )
	{
		DWORD wait_status = WaitForSingleObject( ml_update_trigger_semaphore, 300000 );	// Five minute timeout.

		if ( kill_ml_update_thread_flag )
		{
			break;
		}

		// This will block every other thread from entering until the first thread is complete.
		EnterCriticalSection( &ml_cs );

		// If our queue has too many entries, process 32 at a time.
		for ( int i = 0; i < PROCESS_ITEM_COUNT; ++i )
		{
			if ( kill_ml_update_thread_flag )
			{
				break;
			}

			EnterCriticalSection( &ml_queue_cs );

			mlq_node = message_log_queue;

			if ( mlq_node != NULL )
			{
				DLL_RemoveNode( &message_log_queue, mlq_node );
			}
			else if ( wait_status == WAIT_TIMEOUT )
			{
				CloseHandle( ml_update_trigger_semaphore );
				ml_update_trigger_semaphore = NULL;

				update_message_log_state = 2;	// Thread will be exiting/exited.
			}

			LeaveCriticalSection( &ml_queue_cs );

			// If our node, and by extension, the queue, is empty, then exit the loop.
			if ( mlq_node == NULL )
			{
				break;
			}

			int item_count = _SendMessageW( g_hWnd_message_log_list, LVM_GETITEMCOUNT, 0, 0 );

			// Truncate the number of items.
			if ( ( item_count > 0 ) && ( item_count >= MAX_ITEM_COUNT ) )
			{
				skip_message_log_draw = true;

				while ( item_count >= MAX_ITEM_COUNT )
				{
					--item_count;

					if ( add_to_ml_top )
					{
						lvi.iItem = item_count;
					}
					else
					{
						// Remove items in reverse order (highest index to lowest) so we don't have to repaint non-visible entries.
						lvi.iItem = item_count - ( MAX_ITEM_COUNT - 1 );
					}

					BOOL test = _SendMessageW( g_hWnd_message_log_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					MESSAGE_LOG_INFO *mli = ( MESSAGE_LOG_INFO * )lvi.lParam;
					if ( mli != NULL )
					{
						GlobalFree( mli->w_date_and_time );
						GlobalFree( mli->w_level );
						GlobalFree( mli->message );
						GlobalFree( mli );
					}

					_SendMessageW( g_hWnd_message_log_list, LVM_DELETEITEM, lvi.iItem, 0 );
				}

				skip_message_log_draw = false;

				_InvalidateRect( g_hWnd_message_log_list, NULL, TRUE );
			}

			lvi.iItem = ( add_to_ml_top ? 0 : item_count );
			lvi.lParam = ( LPARAM )mlq_node->data;
			_SendMessageW( g_hWnd_message_log_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

			GlobalFree( mlq_node );

			_EnableWindow( g_hWnd_save_log, TRUE );
			_EnableWindow( g_hWnd_clear_log, TRUE );

			EnterCriticalSection( &ml_queue_cs );

			// Exit the for loop if our message log is empty.
			if ( message_log_queue == NULL )
			{
				i = PROCESS_ITEM_COUNT;
			}

			LeaveCriticalSection( &ml_queue_cs );
		}

		// We're done. Let other threads continue.
		LeaveCriticalSection( &ml_cs );
	}

	if ( kill_ml_update_thread_flag )
	{
		CloseHandle( ml_update_trigger_semaphore );
		ml_update_trigger_semaphore = NULL;

		cleanup_message_log_queue();
	}

	// Release the semaphore if we're killing the thread.
	if ( ml_update_semaphore != NULL )
	{
		ReleaseSemaphore( ml_update_semaphore, 1, NULL );
	}

	in_ml_update_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &ml_update_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN clear_message_log( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &ml_cs );

	in_ml_worker_thread = true;

	skip_message_log_draw = true;

	Processing_ML_Window( false );

	cleanup_message_log();

	_SendMessageW( g_hWnd_message_log_list, LVM_DELETEALLITEMS, 0, 0 );

	skip_message_log_draw = false;

	_EnableWindow( g_hWnd_save_log, FALSE );
	_EnableWindow( g_hWnd_clear_log, FALSE );

	Processing_ML_Window( true );

	// Release the semaphore if we're killing the thread.
	if ( ml_worker_semaphore != NULL )
	{
		ReleaseSemaphore( ml_worker_semaphore, 1, NULL );
	}

	in_ml_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &ml_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN copy_message_log( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &ml_cs );

	in_ml_worker_thread = true;

	Processing_ML_Window( false );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;
	lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

	int item_count = _SendMessageW( g_hWnd_message_log_list, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = _SendMessageW( g_hWnd_message_log_list, LVM_GETSELECTEDCOUNT, 0, 0 );
	
	bool copy_all = false;
	if ( item_count == sel_count )
	{
		copy_all = true;
	}
	else
	{
		item_count = sel_count;
	}

	unsigned int buffer_size = 8192;
	unsigned int buffer_offset = 0;
	wchar_t *copy_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_size );	// Allocate 8 kilobytes.

	int value_length = 0;

	wchar_t *copy_string = NULL;
	bool add_newline = false;
	bool add_tab = false;

	// Go through each item, and copy their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_ml_worker_thread_flag )
		{
			break;
		}

		if ( copy_all )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = _SendMessageW( g_hWnd_message_log_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		_SendMessageW( g_hWnd_message_log_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

		MESSAGE_LOG_INFO *mli = ( MESSAGE_LOG_INFO * )lvi.lParam;

		add_newline = add_tab = false;

		for ( int j = 0; j < 3; ++j )
		{
			switch ( j )
			{
				case 0:
				{
					copy_string = mli->w_date_and_time;
				}
				break;

				case 1:
				{
					copy_string = mli->w_level;
				}
				break;

				case 2:
				{
					copy_string = mli->message;
				}
				break;
			}

			if ( copy_string == NULL || ( copy_string != NULL && copy_string[ 0 ] == NULL ) )
			{
				if ( j == 0 )
				{
					add_tab = false;
				}

				continue;
			}

			if ( j > 0 && add_tab )
			{
				*( copy_buffer + buffer_offset ) = L'\t';
				++buffer_offset;
			}

			add_tab = true;

			value_length = lstrlenW( copy_string );
			while ( buffer_offset + value_length + 3 >= buffer_size )	// Add +3 for \t and \r\n
			{
				buffer_size += 8192;
				wchar_t *realloc_buffer = ( wchar_t * )GlobalReAlloc( copy_buffer, sizeof( wchar_t ) * buffer_size, GMEM_MOVEABLE );
				if ( realloc_buffer == NULL )
				{
					goto CLEANUP;
				}

				copy_buffer = realloc_buffer;
			}
			_wmemcpy_s( copy_buffer + buffer_offset, buffer_size - buffer_offset, copy_string, value_length );
			buffer_offset += value_length;

			copy_string = NULL;
			add_newline = true;
		}

		if ( i < item_count - 1 && add_newline )	// Add newlines for every item except the last.
		{
			*( copy_buffer + buffer_offset ) = L'\r';
			++buffer_offset;
			*( copy_buffer + buffer_offset ) = L'\n';
			++buffer_offset;
		}
		else if ( ( i == item_count - 1 && !add_newline ) && buffer_offset >= 2 )	// If add_newline is false for the last item, then a newline character is in the buffer.
		{
			buffer_offset -= 2;	// Ignore the last newline in the buffer.
		}
	}

	if ( _OpenClipboard( g_hWnd_message_log_list ) )
	{
		_EmptyClipboard();

		DWORD len = buffer_offset;

		// Allocate a global memory object for the text.
		HGLOBAL hglbCopy = GlobalAlloc( GMEM_MOVEABLE, sizeof( wchar_t ) * ( len + 1 ) );
		if ( hglbCopy != NULL )
		{
			// Lock the handle and copy the text to the buffer. lptstrCopy doesn't get freed.
			wchar_t *lptstrCopy = ( wchar_t * )GlobalLock( hglbCopy );
			if ( lptstrCopy != NULL )
			{
				_wmemcpy_s( lptstrCopy, len + 1, copy_buffer, len );
				lptstrCopy[ len ] = 0; // Sanity
			}

			GlobalUnlock( hglbCopy );

			if ( _SetClipboardData( CF_UNICODETEXT, hglbCopy ) == NULL )
			{
				GlobalFree( hglbCopy );	// Only free this Global memory if SetClipboardData fails.
			}

			_CloseClipboard();
		}
	}

CLEANUP:

	GlobalFree( copy_buffer );

	Processing_ML_Window( true );

	// Release the semaphore if we're killing the thread.
	if ( ml_worker_semaphore != NULL )
	{
		ReleaseSemaphore( ml_worker_semaphore, 1, NULL );
	}

	in_ml_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &ml_cs );

	_ExitThread( 0 );
	return 0;
}

void save_message_log_csv_file( wchar_t *file_path )
{
	HANDLE hFile_message_log = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_message_log != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		// Write the UTF-8 BOM and CSV column titles.
		WriteFile( hFile_message_log, "\xEF\xBB\xBF\"Date and Time (UTC)\",\"Level\",\"Message\"", 42, &write, NULL );

		// Get the number of items in the listview.
		int num_items = _SendMessageW( g_hWnd_message_log_list, LVM_GETITEMCOUNT, 0, 0 );

		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		// Go through each item, and free their lParam values.
		for ( lvi.iItem = 0; lvi.iItem < num_items; ++lvi.iItem )
		{
			// Stop processing and exit the thread.
			/*if ( kill_ml_worker_thread_flag )
			{
				break;
			}*/

			_SendMessageW( g_hWnd_message_log_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

			MESSAGE_LOG_INFO *mli = ( MESSAGE_LOG_INFO * )lvi.lParam;

			int time_length = WideCharToMultiByte( CP_UTF8, 0, mli->w_date_and_time, -1, NULL, 0, NULL, NULL );
			char *utf8_time = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * time_length ); // Size includes the null character.
			time_length = WideCharToMultiByte( CP_UTF8, 0, mli->w_date_and_time, -1, utf8_time, time_length, NULL, NULL ) - 1;

			int level_length = WideCharToMultiByte( CP_UTF8, 0, mli->w_level, -1, NULL, 0, NULL, NULL );
			char *utf8_level = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * level_length ); // Size includes the null character.
			level_length = WideCharToMultiByte( CP_UTF8, 0, mli->w_level, -1, utf8_level, level_length, NULL, NULL ) - 1;
			
			int message_length = WideCharToMultiByte( CP_UTF8, 0, mli->message, -1, NULL, 0, NULL, NULL );
			char *utf8_message = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * message_length ); // Size includes the null character.
			message_length = WideCharToMultiByte( CP_UTF8, 0, mli->message, -1, utf8_message, message_length, NULL, NULL ) - 1;

			char *escaped_message = escape_csv( utf8_message );
			if ( escaped_message != NULL )
			{
				message_length = lstrlenA( escaped_message );
			}

			// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
			if ( pos + time_length + level_length + message_length + 8 > size )
			{
				// Dump the buffer.
				WriteFile( hFile_message_log, write_buf, pos, &write, NULL );
				pos = 0;
			}

			write_buf[ pos++ ] = '\r';
			write_buf[ pos++ ] = '\n';

			write_buf[ pos++ ] = '\"';
			_memcpy_s( write_buf + pos, size - pos, utf8_time, time_length );
			pos += time_length;
			write_buf[ pos++ ] = '\"';
			write_buf[ pos++ ] = ',';

			_memcpy_s( write_buf + pos, size - pos, utf8_level, level_length );
			pos += level_length;
			write_buf[ pos++ ] = ',';

			write_buf[ pos++ ] = '\"';
			_memcpy_s( write_buf + pos, size - pos, utf8_message, message_length );
			pos += message_length;
			write_buf[ pos++ ] = '\"';

			GlobalFree( escaped_message );
			GlobalFree( utf8_message );
			GlobalFree( utf8_time );
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_message_log, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_message_log );
	}
}

THREAD_RETURN create_message_log_csv_file( void *file_path )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &ml_cs );

	in_ml_worker_thread = true;

	Processing_ML_Window( false );

	if ( file_path != NULL )
	{
		save_message_log_csv_file( ( wchar_t * )file_path );

		GlobalFree( file_path );
	}

	Processing_ML_Window( true );

	// Release the semaphore if we're killing the thread.
	if ( ml_worker_semaphore != NULL )
	{
		ReleaseSemaphore( ml_worker_semaphore, 1, NULL );
	}

	in_ml_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &ml_cs );

	_ExitThread( 0 );
	return 0;
}

void HandleMessageLogInput( DWORD type, void *data )
{
	if ( type & ML_WRITE_OUTPUT_W || type & ML_WRITE_OUTPUT_A )
	{
		SYSTEMTIME st;
		FILETIME ft;
		GetSystemTime( &st );
		SystemTimeToFileTime( &st, &ft );

		MESSAGE_LOG_INFO *mli = ( MESSAGE_LOG_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( MESSAGE_LOG_INFO ) );
		mli->message = NULL;
		mli->w_date_and_time = NULL;
		mli->w_level = NULL;

		mli->date_and_time.LowPart = ft.dwLowDateTime;
		mli->date_and_time.HighPart = ft.dwHighDateTime;

		mli->level = ( type & ML_ERROR ? 0 : ( type & ML_WARNING ? 1 : ( type & ML_NOTICE ? 2 : 3 ) ) );

		if ( mli->level == 0 )
		{
			mli->w_level = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 6 );
			_wmemcpy_s( mli->w_level, 6, L"ERROR\0", 6 );
		}
		else if ( mli->level == 1 )
		{
			mli->w_level = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 8 );
			_wmemcpy_s( mli->w_level, 8, L"WARNING\0", 8 );
		}
		else if ( mli->level == 2 )
		{
			mli->w_level = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 7 );
			_wmemcpy_s( mli->w_level, 7, L"NOTICE\0", 7 );
		}

		mli->w_date_and_time = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 26 );
		__snwprintf( mli->w_date_and_time, 26, L"%02d/%02d/%04d (%02d:%02d:%02d.%03d)", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );

		if ( type & ML_WRITE_OUTPUT_W )
		{
			mli->message = GlobalStrDupW( ( wchar_t * )data );
		}
		else if ( type & ML_WRITE_OUTPUT_A )
		{
			int message_length = MultiByteToWideChar( CP_UTF8, 0, ( char * )data, -1, NULL, 0 );	// Include the NULL terminator.
			mli->message = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * message_length );
			MultiByteToWideChar( CP_UTF8, 0, ( char * )data, -1, mli->message, message_length );
		}

		// Replaces runs of \r, \n, and \t with a single space to keep things looking nice.
		// This is really meant to handle server error messages since we don't know how they're formatted.
		mli->message = decode_message_event_w( mli->message );

		DoublyLinkedList *ml_node = DLL_CreateNode( ( void * )mli );

		EnterCriticalSection( &ml_queue_cs );

		if ( update_message_log_state != 1 )
		{
			update_message_log_state = 1;	// Thread will be running.

			// Make sure the semaphore is not in use.
			if ( ml_update_trigger_semaphore != NULL )
			{
				CloseHandle( ml_update_trigger_semaphore );
			}

			// Create the semaphore before we spawn the ringtone thread so that it can queue up messages. (Unlikely to occur)
			ml_update_trigger_semaphore = CreateSemaphore( NULL, 0, MAX_ITEM_COUNT, NULL );

			//CloseHandle( ( HANDLE )_CreateThread( NULL, 0, MessageLogManager, ( void * )NULL, 0, NULL ) );
			HANDLE update_message_log_handle = _CreateThread( NULL, 0, MessageLogManager, ( void * )NULL, 0, NULL );
			SetThreadPriority( update_message_log_handle, THREAD_PRIORITY_LOWEST );
			CloseHandle( update_message_log_handle );
		}

		if ( ml_update_trigger_semaphore != NULL )
		{
			// Add the new entry to the tail end of the queue.
			DLL_AddNode( &message_log_queue, ml_node, -1 );

			// This may fail if too many calls are made in a short amount of time.
			// MessageLogManager will process the message log queue in chunks of 32 if there's a backlog.
			ReleaseSemaphore( ml_update_trigger_semaphore, 1, NULL );
		}
		else
		{
			GlobalFree( mli->w_date_and_time );
			GlobalFree( mli->w_level );
			GlobalFree( mli->message );
			GlobalFree( mli );
			GlobalFree( ml_node );
		}

		LeaveCriticalSection( &ml_queue_cs );
	}

	if ( type & ML_MESSAGE_BOX_W )
	{
		_MessageBoxW( g_hWnd_message_log, ( LPCWSTR )data, PROGRAM_CAPTION, MB_APPLMODAL | ( type & ML_ERROR ? MB_ICONERROR : ( type & ML_WARNING ? MB_ICONWARNING : ( type & ML_NOTICE ? MB_ICONINFORMATION : 0 ) ) ) | MB_SETFOREGROUND );
	}
	else if ( type & ML_MESSAGE_BOX_A )
	{
		_MessageBoxA( g_hWnd_message_log, ( LPCSTR )data, PROGRAM_CAPTION_A, MB_APPLMODAL | ( type & ML_ERROR ? MB_ICONERROR : ( type & ML_WARNING ? MB_ICONWARNING : ( type & ML_NOTICE ? MB_ICONINFORMATION : 0 ) ) ) | MB_SETFOREGROUND );
	}

	if ( type & ML_FREE_INPUT )
	{
		GlobalFree( data );
	}
}
