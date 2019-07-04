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

#include "globals.h"
#include "ringtone_utilities.h"
#include "utilities.h"

#include "lite_winmm.h"

HANDLE ringtone_update_semaphore = NULL;

HANDLE ringtone_update_trigger_semaphore = NULL;

bool kill_ringtone_update_thread_flag = false;

bool in_ringtone_update_thread = false;

CRITICAL_SECTION ringtone_update_cs;
CRITICAL_SECTION ringtone_queue_cs;

DoublyLinkedList *ringtone_queue = NULL;

unsigned char ringtone_manager_state = 0;

void cleanup_ringtone_queue()
{
	bool play = true;
	#ifndef WINMM_USE_STATIC_LIB
		if ( winmm_state == WINMM_STATE_SHUTDOWN )
		{
			play = InitializeWinMM();
		}
	#endif

	wchar_t mci_command[ MAX_PATH + 8 ];

	// Cleanup any message log items that were queued and not added to the list. (Unlikely to occur)
	DoublyLinkedList *ringtone_q_node = ringtone_queue;
	while ( ringtone_q_node != NULL )
	{
		DoublyLinkedList *del_ringtone_q_node = ringtone_q_node;

		ringtone_q_node = ringtone_q_node->next;

		RINGTONE_INFO *rti = ( RINGTONE_INFO * )del_ringtone_q_node->data;
		if ( rti != NULL )
		{
			if ( play )
			{
				// We need to close any ringtone that's currently playing.
				if ( rti->state == RINGTONE_CLOSE )
				{
					__snwprintf( mci_command, MAX_PATH + 8, L"close %s", rti->alias );

					_mciSendStringW( mci_command, NULL, 0, NULL );
				}
			}

			GlobalFree( rti->alias );
			GlobalFree( rti->file_path );
			GlobalFree( rti );
		}

		GlobalFree( del_ringtone_q_node );
	}
}

void kill_ringtone_update_thread()
{
	if ( in_ringtone_update_thread )
	{
		// This semaphore will be released when the thread gets killed.
		ringtone_update_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		kill_ringtone_update_thread_flag = true;	// Causes secondary threads to cease processing and release the semaphore.

		if ( ringtone_update_trigger_semaphore != NULL )
		{
			ReleaseSemaphore( ringtone_update_trigger_semaphore, 1, NULL );
		}

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( ringtone_update_semaphore, 5000 );
		CloseHandle( ringtone_update_semaphore );
		ringtone_update_semaphore = NULL;
	}
}

THREAD_RETURN RingtoneManager( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &ringtone_update_cs );

	in_ringtone_update_thread = true;

	DoublyLinkedList *ringtone_q_node = NULL;

	wchar_t mci_command[ MAX_PATH + 32 ];

	bool play = true;
	#ifndef WINMM_USE_STATIC_LIB
		if ( winmm_state == WINMM_STATE_SHUTDOWN )
		{
			play = InitializeWinMM();
		}
	#endif

	while ( !kill_ringtone_update_thread_flag && ringtone_manager_state == 1 )
	{
		DWORD wait_status = WaitForSingleObject( ringtone_update_trigger_semaphore, 300000 );	// Five minute timeout.

		if ( kill_ringtone_update_thread_flag )
		{
			break;
		}

		// If our queue has too many entries, process 32 at a time.
		for ( int i = 0; i < PROCESS_RINGTONE_COUNT; ++i )
		{
			if ( kill_ringtone_update_thread_flag )
			{
				break;
			}

			EnterCriticalSection( &ringtone_queue_cs );

			ringtone_q_node = ringtone_queue;

			if ( ringtone_q_node != NULL )
			{
				DLL_RemoveNode( &ringtone_queue, ringtone_q_node );
			}
			else if ( wait_status == WAIT_TIMEOUT )
			{
				CloseHandle( ringtone_update_trigger_semaphore );
				ringtone_update_trigger_semaphore = NULL;

				ringtone_manager_state = 2;	// Thread will be exiting/exited.
			}

			LeaveCriticalSection( &ringtone_queue_cs );

			// If our node, and by extension, the queue, is empty, then exit the loop.
			if ( ringtone_q_node == NULL )
			{
				break;
			}

			RINGTONE_INFO *rti = ( RINGTONE_INFO * )ringtone_q_node->data;
			if ( rti != NULL )
			{
				if ( play )
				{
					if ( rti->state == RINGTONE_PLAY )
					{
						if ( rti->file_path != NULL && rti->alias != NULL )
						{
							__snwprintf( mci_command, MAX_PATH + 32, L"open %s alias %s", rti->file_path, rti->alias );

							_mciSendStringW( mci_command, NULL, 0, NULL );

							__snwprintf( mci_command, MAX_PATH + 32, L"play %s", rti->alias );

							_mciSendStringW( mci_command, NULL, 0, NULL );
						}
					}
					else if ( rti->state == RINGTONE_CLOSE )
					{
						__snwprintf( mci_command, MAX_PATH + 32, L"close %s", rti->alias );

						_mciSendStringW( mci_command, NULL, 0, NULL );
					}
				}

				GlobalFree( rti->file_path );
				GlobalFree( rti->alias );
				GlobalFree( rti );
			}

			GlobalFree( ringtone_q_node );

			EnterCriticalSection( &ringtone_queue_cs );

			// Exit the for loop if our message log is empty.
			if ( ringtone_queue == NULL )
			{
				i = PROCESS_RINGTONE_COUNT;
			}

			LeaveCriticalSection( &ringtone_queue_cs );
		}
	}

	if ( kill_ringtone_update_thread_flag )
	{
		CloseHandle( ringtone_update_trigger_semaphore );
		ringtone_update_trigger_semaphore = NULL;

		cleanup_ringtone_queue();
	}

	// Release the semaphore if we're killing the thread.
	if ( ringtone_update_semaphore != NULL )
	{
		ReleaseSemaphore( ringtone_update_semaphore, 1, NULL );
	}

	in_ringtone_update_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &ringtone_update_cs );

	_ExitThread( 0 );
	return 0;
}

void HandleRingtone( unsigned char state, wchar_t *file_path, wchar_t *alias )
{
	wchar_t short_path[ MAX_PATH ];
	DWORD short_path_length = 0;

	if ( ( state == RINGTONE_PLAY && ( file_path == NULL || alias == NULL ) ) ||
		 ( state == RINGTONE_CLOSE && alias == NULL ) )
	{
		return;
	}

	if ( state == RINGTONE_PLAY )
	{
		short_path_length = GetShortPathNameW( file_path, short_path, MAX_PATH ) + 1;	// Include NULL terminator.
		if ( short_path_length >= MAX_PATH )
		{
			return;
		}
	}

	RINGTONE_INFO *rti = ( RINGTONE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( RINGTONE_INFO ) );

	if ( state == RINGTONE_PLAY )
	{
		rti->file_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * short_path_length );
		_wmemcpy_s( rti->file_path, short_path_length, short_path, short_path_length );
		rti->file_path[ short_path_length - 1 ] = 0;	// Sanity.
	}
	else
	{
		rti->file_path = NULL;
	}

	rti->alias = GlobalStrDupW( alias );

	rti->state = state;

	DoublyLinkedList *ringtone_node = DLL_CreateNode( ( void * )rti );

	EnterCriticalSection( &ringtone_queue_cs );

	if ( ringtone_manager_state != 1 )
	{
		ringtone_manager_state = 1;	// Thread will be running.

		// Make sure the semaphore is not in use.
		if ( ringtone_update_trigger_semaphore != NULL )
		{
			CloseHandle( ringtone_update_trigger_semaphore );
		}

		// Create the semaphore before we spawn the ringtone thread so that it can queue up messages. (Unlikely to occur)
		ringtone_update_trigger_semaphore = CreateSemaphore( NULL, 0, MAX_RINGTONE_COUNT, NULL );

		//CloseHandle( ( HANDLE )_CreateThread( NULL, 0, RingtoneManager, ( void * )NULL, 0, NULL ) );
		HANDLE update_ringtone_handle = _CreateThread( NULL, 0, RingtoneManager, ( void * )NULL, 0, NULL );
		SetThreadPriority( update_ringtone_handle, THREAD_PRIORITY_LOWEST );
		CloseHandle( update_ringtone_handle );
	}

	if ( ringtone_update_trigger_semaphore != NULL )
	{
		// Add the new entry to the tail end of the queue.
		DLL_AddNode( &ringtone_queue, ringtone_node, -1 );

		// This may fail if too many calls are made in a short amount of time.
		// RingtoneManager will process the ringtone queue in chunks of 32 if there's a backlog.
		ReleaseSemaphore( ringtone_update_trigger_semaphore, 1, NULL );
	}
	else
	{
		bool play = true;
		#ifndef WINMM_USE_STATIC_LIB
			if ( winmm_state == WINMM_STATE_SHUTDOWN )
			{
				play = InitializeWinMM();
			}
		#endif

		wchar_t mci_command[ MAX_PATH + 8 ];

		if ( play )
		{
			// We need to close any ringtone that's currently playing.
			__snwprintf( mci_command, MAX_PATH + 8, L"close %s", alias );

			_mciSendStringW( mci_command, NULL, 0, NULL );
		}

		GlobalFree( rti->alias );
		GlobalFree( rti->file_path );
		GlobalFree( rti );
		GlobalFree( ringtone_node );
	}

	LeaveCriticalSection( &ringtone_queue_cs );
}
