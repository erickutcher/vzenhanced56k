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
#include "telephony.h"
#include "utilities.h"
#include "message_log_utilities.h"
#include "list_operations.h"

#include "lite_tapi32.h"
#include "string_tables.h"

#include "waveout.h"

void CreateIncomingCall( HCALL call );
void IgnoreIncomingCall( HCALL call );
void HandleNextIncomingCall();
void DropIncomingCall( HCALL call );
void HandleLineReply( DWORD request_id );
void CleanupIncomingCall( HCALL call );

CRITICAL_SECTION tt_cs;		// Queues additional telephony threads.
CRITICAL_SECTION ac_cs;		// Allow a clean shutdown of the telephony thread if a call was answered.

CRITICAL_SECTION acc_cs;	// Prevent other accepted call threads from running.

HANDLE telephony_semaphore = NULL;			// Blocks shutdown while the telephony thread is active.
bool in_telephony_thread = false;
unsigned char kill_telephony_thread_flag = 0;	// 1 = exit immediately, 2 = exit after call has finished processing.

HANDLE accepted_call_semaphore = NULL;
bool in_accepted_call_thread = false;
unsigned char kill_accepted_call_thread_flag = 0;	// 1 = state is disconnected, 2 = state is idle.

HLINEAPP ghLineApp = 0;
HLINE ghLine = 0;

DWORD gNumDevs = 0;
DWORD gAPIVersion = 0x00020002;
DWORD gExtVersion = 0;

DoublyLinkedList *g_incoming_call_queue = NULL;

bool g_voice_playback_supported = false;	// The device supports wave/out playback.
DWORD g_playback_device_id = 0;				// The device ID for the wave/out playback device

DoublyLinkedList *FindCallInfo( DoublyLinkedList *dll, HCALL call )
{
	if ( dll != NULL )
	{
		while ( dll != NULL )
		{
			incomingcallinfo *ici = ( incomingcallinfo * )dll->data;

			if ( ici != NULL && ici->incoming_call == call )
			{
				break;
			}
			
			dll = dll->next;
		}
	}

	return dll;
}

void CleanupCallInfo()
{
	DoublyLinkedList *dll_node = g_incoming_call_queue;
	while ( dll_node != NULL )
	{
		DoublyLinkedList *del_node = dll_node;

		dll_node = dll_node->next;

		GlobalFree( del_node->data );
		GlobalFree( del_node );
	}

	g_incoming_call_queue = NULL;
}

void kill_telephony_thread()
{
	if ( in_telephony_thread )
	{
		bool current_call_being_answered = false;

		// This semaphore will be released when the thread gets killed.
		telephony_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		EnterCriticalSection( &ac_cs );

		// See if the first item in our call queue is currently being answered.
		if ( g_incoming_call_queue != NULL )
		{
			incomingcallinfo *ici = ( incomingcallinfo * )g_incoming_call_queue->data;
			if ( ici != NULL && ici->state == 2 )
			{
				current_call_being_answered = true;
			}
		}

		if ( current_call_being_answered )
		{
			kill_telephony_thread_flag = 2;	// Exit after the phone call has finished processing.
		}
		else
		{
			kill_telephony_thread_flag = 1;	// Exit immediately.

			_lineClose( ghLine );
			_lineShutdown( ghLineApp );		// Causes the telephony thread to cease processing and release the semaphore.
		}

		LeaveCriticalSection( &ac_cs );

		// Wait for any active threads to complete. Drop time + 5 second timeout in case we miss the release.
		WaitForSingleObject( telephony_semaphore, 5000 + ( ( DWORD )cfg_drop_call_wait * 1000 ) );
		CloseHandle( telephony_semaphore );
		telephony_semaphore = NULL;
	}
}

void CreateIncomingCall( HCALL call )
{
	EnterCriticalSection( &ac_cs );

	incomingcallinfo *ici = ( incomingcallinfo * )GlobalAlloc( GMEM_FIXED, sizeof( incomingcallinfo ) );
	ici->incoming_call = call;
	ici->state = 0;

	DoublyLinkedList *dll_node = DLL_CreateNode( ici );
	DLL_AddNode( &g_incoming_call_queue, dll_node, -1 );

	LeaveCriticalSection( &ac_cs );
}

void HandleIncomingCallerID( HCALL call, LINECALLINFO *lci )
{
	if ( lci == NULL )
	{
		return;
	}

	SYSTEMTIME SystemTime;
	FILETIME FileTime;

	_lineGetCallInfoA( call, lci );

	// Use caller ID name/number pairs of the following.
	if ( ( ( lci->dwCallerIDFlags & LINECALLPARTYID_NAME ) && ( lci->dwCallerIDFlags & LINECALLPARTYID_ADDRESS ) ) ||
		   ( lci->dwCallerIDFlags & LINECALLPARTYID_BLOCKED ) ||
		   ( lci->dwCallerIDFlags & LINECALLPARTYID_OUTOFAREA ) ||
		   ( lci->dwCallerIDFlags & LINECALLPARTYID_UNAVAIL ) )
	{
		incomingcallinfo *ici = NULL;
		bool process_incoming_call = false;

		EnterCriticalSection( &ac_cs );

		// Make sure we haven't closed the program.
		if ( kill_telephony_thread_flag == 0 )
		{
			// If not, then allow us to answer the call.
			DoublyLinkedList *dll_node = FindCallInfo( g_incoming_call_queue, call );
			if ( dll_node != NULL )
			{
				ici = ( incomingcallinfo * )dll_node->data;

				// A call has not been process or answered.
				if ( ici != NULL && ici->state == 0 )
				{
					ici->state = 1;	// Process the call (display the caller ID info).

					process_incoming_call = true;
				}
			}
		}

		// We only want to process the call once.
		if ( process_incoming_call && ici != NULL )
		{
			GetLocalTime( &SystemTime );
			SystemTimeToFileTime( &SystemTime, &FileTime );

			displayinfo *di = ( displayinfo * )GlobalAlloc( GPTR, sizeof( displayinfo ) );

			// I don't see why this can't show up for BLOCKED, OUTOFAREA, and UNAVAIL.
			if ( ( lci->dwCallerIDFlags & LINECALLPARTYID_ADDRESS ) && lci->dwCallerIDSize > 1 )	// Ignore the empty string if that's all we get.
			{
				di->ci.call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * lci->dwCallerIDSize );
				_memcpy_s( di->ci.call_from, sizeof( char ) * lci->dwCallerIDSize, ( char * )lci + lci->dwCallerIDOffset, sizeof( char ) * lci->dwCallerIDSize );
				di->ci.call_from[ lci->dwCallerIDSize - 1 ] = 0;	// Sanity.
			}

			if ( ( lci->dwCallerIDFlags & LINECALLPARTYID_NAME ) && lci->dwCallerIDNameSize > 1 )	// Ignore the empty string if that's all we get.
			{
				di->ci.caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * lci->dwCallerIDNameSize );
				_memcpy_s( di->ci.caller_id, sizeof( char ) * lci->dwCallerIDNameSize, ( char * )lci + lci->dwCallerIDNameOffset, sizeof( char ) * lci->dwCallerIDNameSize );
				di->ci.caller_id[ lci->dwCallerIDNameSize - 1 ] = 0;	// Sanity.

				if ( di->ci.call_from == NULL )
				{
					di->ci.call_from = GlobalStrDupA( "Unavailable" );
				}
			}
			else if ( lci->dwCallerIDFlags & LINECALLPARTYID_BLOCKED )
			{
				di->ci.caller_id = GlobalStrDupA( "PRIVATE" );
				if ( di->ci.call_from == NULL )
				{
					di->ci.call_from = GlobalStrDupA( "PRIVATE" );
				}
			}
			else if ( lci->dwCallerIDFlags & LINECALLPARTYID_OUTOFAREA )
			{
				di->ci.caller_id = GlobalStrDupA( "Out of Area" );
				if ( di->ci.call_from == NULL )
				{
					di->ci.call_from = GlobalStrDupA( "Out of Area" );
				}
			}
			else// if ( lci->dwCallerIDFlags & LINECALLPARTYID_UNAVAIL )
			{
				di->ci.caller_id = GlobalStrDupA( "Unavailable" );
				if ( di->ci.call_from == NULL )
				{
					di->ci.call_from = GlobalStrDupA( "Unavailable" );
				}
			}

			di->time.LowPart = FileTime.dwLowDateTime;
			di->time.HighPart = FileTime.dwHighDateTime;
			di->process_incoming = true;

			di->incoming_call = ici->incoming_call;	// Save the incoming call handle so that we can ignore it later if we need to.

			// This will also ignore the call if it's in our lists.
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_call_log, ( void * )di, 0, NULL ) );
		}

		LeaveCriticalSection( &ac_cs );
	}
}

void IgnoreIncomingCall( HCALL call )
{
	EnterCriticalSection( &ac_cs );

	// Make sure we haven't closed the program.
	if ( kill_telephony_thread_flag == 0 )
	{
		DoublyLinkedList *dll = FindCallInfo( g_incoming_call_queue, call );
		if ( dll != NULL )
		{
			incomingcallinfo *ici = ( incomingcallinfo * )dll->data;
			if ( ici != NULL )
			{
				// If the call has been processed (we got the caller ID info), then answer it.
				if ( ici->state == 1 )
				{
					// Prevent call waiting calls from answering until the first call has finished.
					if ( dll == g_incoming_call_queue )
					{
						ici->state = 2;	// Answered.

						// Answer the call.
						LONG ret = _lineAnswer( ici->incoming_call, NULL, 0 );

/////////////////////////////////////
#ifdef USE_DEBUG_DIRECTORY
/////////////////////////////////////

wchar_t *msg_buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, 64 );
__snwprintf( msg_buf, 64, L"lineAnswer: %08x", ret );
MESSAGE_LOG_OUTPUT( ML_NOTICE | ML_FREE_INPUT, msg_buf )

/////////////////////////////////////
#endif
/////////////////////////////////////

						if ( ret < 0 )
						{
							ici->state = 1;	// Reset to processed. We'll let the call proceed unanswered
						}
					}
					else
					{
						ici->state = 3;	// Queue. To be answered.
					}
				}
			}
		}
	}

	LeaveCriticalSection( &ac_cs );
}

void HandleNextIncomingCall()
{
	EnterCriticalSection( &ac_cs );

	// If we attempted to close the program while a call was being processed, then we'll finish the shutdown of this thread here.
	if ( kill_telephony_thread_flag == 2 )
	{
		kill_telephony_thread_flag = 1;
	}
	else if ( kill_telephony_thread_flag == 0 )	// If we're not closing the program.
	{
		// See if the next call in the queue needs to be ignored.
		if ( g_incoming_call_queue != NULL )
		{
			incomingcallinfo *ici = ( incomingcallinfo * )g_incoming_call_queue->data;
			if ( ici != NULL && ici->state == 3 )
			{
				ici->state = 1; // Reset the state to processed so we can try to answer it again.

				IgnoreIncomingCall( ici->incoming_call );
			}
		}
	}

	LeaveCriticalSection( &ac_cs );
}

void DropIncomingCall( HCALL call )
{
	EnterCriticalSection( &ac_cs );

	DoublyLinkedList *dll_node = FindCallInfo( g_incoming_call_queue, call );
	if ( dll_node != NULL )
	{
		incomingcallinfo *ici = ( incomingcallinfo * )dll_node->data;

		if ( ici != NULL )
		{
			// If the drop failed, then clean up the call here.
			LONG ret = _lineDrop( call, NULL, 0 );

/////////////////////////////////////
#ifdef USE_DEBUG_DIRECTORY
/////////////////////////////////////

wchar_t *msg_buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, 64 );
__snwprintf( msg_buf, 64, L"lineDrop: %08x", ret );
MESSAGE_LOG_OUTPUT( ML_NOTICE | ML_FREE_INPUT, msg_buf )

/////////////////////////////////////
#endif
/////////////////////////////////////

			if ( ret < 0 )
			{
				DLL_RemoveNode( &g_incoming_call_queue, dll_node );

				GlobalFree( dll_node->data );
				GlobalFree( dll_node );

				_lineDeallocateCall( call );

				// See if the program is exiting.
				// If not, then see if the next call in the queue needs to be ignored.
				HandleNextIncomingCall();
			}
		}
	}

	LeaveCriticalSection( &ac_cs );
}

DWORD WINAPI HandleAcceptedCall( LPVOID lpParam ) 
{
	EnterCriticalSection( &acc_cs );

	in_accepted_call_thread = true;

	HCALL call = ( HCALL )lpParam;

	ULARGE_INTEGER start, stop;

	GetCurrentCounterTime( &start );

	// Play back a recording if we've enabled it if and our device supports it.
	if ( cfg_popup_enable_recording && g_voice_playback_supported )
	{
		// Play back the specified recording on the output device.
		PlaybackRecording( g_playback_device_id, default_recording );
	}

	GetCurrentCounterTime( &stop );

	// If the call state is not idle or disconnected, then determine how much time we need to wait before dropping the call.
	if ( kill_accepted_call_thread_flag == 0 )
	{
		// If our recording was shorter than our drop time, then wait the difference.
		DWORD remaining_time = GetRemainingDropCallWaitTime( start, stop );
		if ( remaining_time > 0 )
		{
			// Wait some time before we drop the call so that it doesn't stay answered.
			// A 4 second minimum seems to be what works.
			Sleep( remaining_time );
		}
	}

	// If the call state is not idle, then drop the call.
	if ( kill_accepted_call_thread_flag != 2 )
	{
		DropIncomingCall( call );
	}

	kill_accepted_call_thread_flag = 0;	// Reset so that other calls can be killed.

/////////////////////////////////////
#ifdef USE_DEBUG_DIRECTORY
/////////////////////////////////////

MESSAGE_LOG_OUTPUT( ML_NOTICE, L"Exiting Accepted Call Thread." )

/////////////////////////////////////
#endif
/////////////////////////////////////

	// Release the semaphore if we're killing the thread.
	if ( accepted_call_semaphore != NULL )
	{
		ReleaseSemaphore( accepted_call_semaphore, 1, NULL );
	}

	in_accepted_call_thread = false;

	LeaveCriticalSection( &acc_cs );

	_ExitThread( 0 );
	return 0;
}

void CleanupIncomingCall( HCALL call )
{
	EnterCriticalSection( &ac_cs );

	DoublyLinkedList *dll_node = FindCallInfo( g_incoming_call_queue, call );
	if ( dll_node != NULL )
	{
		DLL_RemoveNode( &g_incoming_call_queue, dll_node );

		GlobalFree( dll_node->data );
		GlobalFree( dll_node );
	}

	// In API versions 2.0 or later, lineDeallocateCall does not suspend outstanding LINE_REPLY messages;
	// Every asynchronous function that returns a dwRequestID to the application always results in the delivery of the associated LINE_REPLY message unless the application calls lineShutdown.
	_lineDeallocateCall( call );

	// See if the program is exiting.
	// If not, then see if the next call in the queue needs to be ignored.
	HandleNextIncomingCall();

	LeaveCriticalSection( &ac_cs );
}

bool InitializeTelephony()
{
	LINEEXTENSIONID leid;
	DWORD DeviceID = 0;

	//DWORD min_api_version = 0x00010003;
	DWORD min_api_version = 0x00020000;	// We want at least 2.0 since it'll guarantee a LINE_REPLY is received for asynchronous calls.
	DWORD max_api_version = 0x00020004;

	LINEINITIALIZEEXPARAMS hLineInitParams;
	_memzero( &hLineInitParams, sizeof( LINEINITIALIZEEXPARAMS ) );
	hLineInitParams.dwTotalSize = sizeof( LINEINITIALIZEEXPARAMS );
	hLineInitParams.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;

	// Accepted call thread.
	InitializeCriticalSection( &acc_cs );

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Initializing_telephony_interface )

	if ( _lineInitializeExW( &ghLineApp, ( HINSTANCE )NULL, ( LINECALLBACK )NULL, PROGRAM_CAPTION, &gNumDevs, &gAPIVersion, &hLineInitParams ) != 0 )
	{
		MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_initialize_telephony_interface )
		
		return false;
	}

	_lineNegotiateAPIVersion( ghLineApp, 0, min_api_version, max_api_version, &gAPIVersion, &leid );

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Detecting_available_modems )

	LINEDEVCAPS *hLineDevCaps = ( LINEDEVCAPS * )GlobalAlloc( GPTR, sizeof( LINEDEVCAPS ) + 4096 );

	// Find all the devices that support (at a minimum) a mode of LINEMEDIAMODE_DATAMODEM.
	for ( ; DeviceID < gNumDevs; ++DeviceID )
	{
		_memzero( hLineDevCaps, sizeof( LINEDEVCAPS ) + 4096 );
		hLineDevCaps->dwTotalSize = sizeof( LINEDEVCAPS ) + 4096;

		_lineGetDevCapsW( ghLineApp, DeviceID, gAPIVersion, gExtVersion, hLineDevCaps );

		if ( hLineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM )
		{
			modeminfo *mi = ( modeminfo * )GlobalAlloc( GMEM_FIXED, sizeof( modeminfo ) );
			mi->device_id = DeviceID;

			mi->permanent_line_guid = hLineDevCaps->PermanentLineGuid;

			mi->line_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, hLineDevCaps->dwLineNameSize );	// The size is in wide chars and include the NULL terminator.
			_memcpy_s( mi->line_name, hLineDevCaps->dwLineNameSize, ( wchar_t * )( ( char * )hLineDevCaps + hLineDevCaps->dwLineNameOffset ), hLineDevCaps->dwLineNameSize );
			mi->line_name[ ( hLineDevCaps->dwLineNameSize - sizeof( wchar_t ) ) / sizeof( wchar_t ) ] = 0;	// Sanity.
			
			if ( dllrbt_insert( modem_list, ( void * )&mi->permanent_line_guid, ( void * )mi ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( mi->line_name );
				GlobalFree( mi );
				mi = NULL;
			}

			// Set the default modem if the one we've saved is found on the system.
			if ( default_modem == NULL && mi != NULL && _memcmp( &mi->permanent_line_guid, &cfg_modem_guid, sizeof( GUID ) ) == 0 )
			{
				default_modem = mi;
			}
		}
	}

	GlobalFree( hLineDevCaps );

	// If no modem is in the list, then alert the user.
	if ( default_modem == NULL )
	{
		// Use the first modem in the list if none was found.
		node_type *node = dllrbt_get_head( modem_list );
		if ( node != NULL )
		{
			default_modem = ( modeminfo * )node->val;
		}

		// If there was still no modem found, then exit the thread.
		if ( default_modem == NULL )
		{
			MESSAGE_LOG_OUTPUT_MB( ML_NOTICE, ST_No_modem_found )
			
			return false;
		}
	}

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Opening_line_device )

	// Open the line with the default modem's device id.
	// Can't use LINEMEDIAMODE_INTERACTIVEVOICE with LINECALLPRIVILEGE_OWNER. See: https://web.archive.org/web/20110218222910/http://support.microsoft.com/kb/132192
	LONG ret = _lineOpenW( ghLineApp, default_modem->device_id, &ghLine, gAPIVersion, gExtVersion, NULL, LINECALLPRIVILEGE_MONITOR | LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_AUTOMATEDVOICE | LINEMEDIAMODE_DATAMODEM, NULL );
	if ( ret != 0 )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Voice_playback_is_not_supported )

		ret = _lineOpenW( ghLineApp, default_modem->device_id, &ghLine, gAPIVersion, gExtVersion, NULL, LINECALLPRIVILEGE_MONITOR | LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_DATAMODEM, NULL );
		if ( ret != 0 )
		{
			// We could not open a line with the modem.
			MESSAGE_LOG_OUTPUT_MB( ML_NOTICE, ST_Modem_features )
			
			return false;
		}
	}
	else	// Line was opened and it (possibly) supports voice input/output.
	{
		// Get the device ID for our wave output.
		VARSTRING *vs = ( VARSTRING * )GlobalAlloc( GPTR, sizeof( VARSTRING ) + 1024 );
		vs->dwTotalSize = sizeof( VARSTRING ) + 1024;

		if ( _lineGetIDW( ghLine, 0, NULL, LINECALLSELECT_LINE, vs, L"wave/out" ) == 0 )
		{
			g_playback_device_id = *( DWORD * )( ( char * )vs + vs->dwStringOffset );

			g_voice_playback_supported = InitializeWAVEOut( g_playback_device_id );
		}

		GlobalFree( vs );
	}

	return true;
}

void UnInitializeTelephony()
{
	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Closing_line_device )

	_lineClose( ghLine );

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Shutting_down_telephony_interface )

	_lineShutdown( ghLineApp );

	if ( g_voice_playback_supported )
	{
		UnInitializeWAVEOut();
	}

	// Accepted call thread.
	DeleteCriticalSection( &acc_cs );
}

THREAD_RETURN TelephonyPoll( void *pArguments )
{
	// This will block every other telephony thread from entering until the first thread is complete.
	EnterCriticalSection( &tt_cs );

	in_telephony_thread = true;

	LINEMESSAGE lm;

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Waiting_for_incoming_calls )

	LINECALLINFO *lci = ( LINECALLINFO * )GlobalAlloc( GPTR, sizeof( LINECALLINFO ) + 4096 );

	while ( kill_telephony_thread_flag != 1 )
	{
		if ( _lineGetMessage( ghLineApp, &lm, INFINITE ) < 0 )
		{
			continue;
		}


/////////////////////////////////////
#ifdef USE_DEBUG_DIRECTORY
/////////////////////////////////////

wchar_t *msg_buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, 256 );
__snwprintf( msg_buf, 256, L"dwMessageID: %08x, dwParam1: %08x, dwParam2: %08x, dwParam3: %08x, hDevice: %08x", lm.dwMessageID, lm.dwParam1, lm.dwParam2, lm.dwParam3, lm.hDevice );
MESSAGE_LOG_OUTPUT( ML_NOTICE | ML_FREE_INPUT, msg_buf )

/////////////////////////////////////
#endif
/////////////////////////////////////


		switch ( lm.dwMessageID )
		{
			case LINE_CALLINFO:
			{
				if ( lm.dwParam1 & LINECALLINFOSTATE_CALLERID )
				{
					_memzero( lci, sizeof( LINECALLINFO ) + 4096 );
					lci->dwTotalSize = sizeof( LINECALLINFO ) + 4096;

					HandleIncomingCallerID( ( HCALL )lm.hDevice, lci );
				}
			}
			break;

			case LINE_CALLSTATE:
			{
				// There's an incoming call. Add it to our process queue.
				if ( lm.dwParam1 & LINECALLSTATE_OFFERING )
				{
					CreateIncomingCall( ( HCALL )lm.hDevice );

					break;
				}

				/*// There's nothing coming through the line anymore, or the caller has hung up.
				// Clean up the call.
				if ( lm.dwParam1 & LINECALLSTATE_IDLE || lm.dwParam1 & LINECALLSTATE_DISCONNECTED )
				{
					CleanupIncomingCall( ( HCALL )lm.hDevice );

					break;
				}*/

				// There's nothing coming through the line anymore, or the caller has hung up.
				// Clean up the call.
				if ( lm.dwParam1 & LINECALLSTATE_IDLE || lm.dwParam1 & LINECALLSTATE_DISCONNECTED )
				{
					if ( in_accepted_call_thread )
					{
						// This semaphore will be released when the thread gets killed.
						accepted_call_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

						if ( lm.dwParam1 & LINECALLSTATE_DISCONNECTED )
						{
							kill_accepted_call_thread_flag = 1;	// Disconnected. Will attempt to drop the call before exiting the accepted call thread.

/////////////////////////////////////
#ifdef USE_DEBUG_DIRECTORY
/////////////////////////////////////

MESSAGE_LOG_OUTPUT( ML_NOTICE, L"Killing Accepted Call Thread while DISCONNECTED." )

/////////////////////////////////////
#endif
/////////////////////////////////////
						}
						else
						{
							kill_accepted_call_thread_flag = 2;	// Idle. Skips dropping the call in the accepted call thread.

/////////////////////////////////////
#ifdef USE_DEBUG_DIRECTORY
/////////////////////////////////////

MESSAGE_LOG_OUTPUT( ML_NOTICE, L"Killing Accepted Call Thread while IDLE." )

/////////////////////////////////////
#endif
/////////////////////////////////////
						}

						// Wait for any active threads to complete. 10 second timeout.
						WaitForSingleObject( accepted_call_semaphore, 10000 );
						CloseHandle( accepted_call_semaphore );
						accepted_call_semaphore = NULL;
					}

					CleanupIncomingCall( ( HCALL )lm.hDevice );

					break;
				}

/*
				// There's nothing coming through the line anymore.
				// Clean up the call.
				if ( lm.dwParam1 & LINECALLSTATE_IDLE )
				{
					if ( in_accepted_call_thread )
					{
						// This semaphore will be released when the thread gets killed.
						accepted_call_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

						kill_accepted_call_thread_flag = 2;	// Idle.

MESSAGE_LOG_OUTPUT( ML_NOTICE, L"Killing accepted call thread while IDLE." )

						// Wait for any active threads to complete. 5 second timeout.
						WaitForSingleObject( accepted_call_semaphore, 5000 );
						CloseHandle( accepted_call_semaphore );
						accepted_call_semaphore = NULL;
					}

					CleanupIncomingCall( ( HCALL )lm.hDevice );

					break;
				}

				// The caller has hung up.
				// Clean up the call.
				if ( lm.dwParam1 & LINECALLSTATE_DISCONNECTED )
				{
					if ( in_accepted_call_thread )
					{
						// This semaphore will be released when the thread gets killed.
						accepted_call_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

						kill_accepted_call_thread_flag = 1;	// Disconnected.

MESSAGE_LOG_OUTPUT( ML_NOTICE, L"Killing accepted call thread while DISCONNECTED." )

						// Wait for any active threads to complete. 5 second timeout.
						WaitForSingleObject( accepted_call_semaphore, 5000 );
						CloseHandle( accepted_call_semaphore );
						accepted_call_semaphore = NULL;
					}

					CleanupIncomingCall( ( HCALL )lm.hDevice );

					break;
				}
*/
				// We answered the call.
				// For reference, LINECALLSTATE_CONNECTED is when we place an outbound call and it gets connected.
				if ( lm.dwParam1 & LINECALLSTATE_ACCEPTED )
				{
					// Will play back audio if supported and sleep until our drop call wait time is reached.
					// By using a thread here, we'll be able to receive the disconnect state if we're still playing audio.
					HANDLE accept_call_handle = ( HANDLE )_CreateThread( NULL, 0, HandleAcceptedCall, ( void * )lm.hDevice, 0, NULL );

					// Make sure the thread was created. If not, then drop the call without playing audio.
					if ( accept_call_handle != NULL )
					{
						CloseHandle( accept_call_handle );
					}
					else
					{
						Sleep( ( ( DWORD )cfg_drop_call_wait ) * 1000 );

						DropIncomingCall( ( HCALL )lm.hDevice );
					}

					break;
				}
			}
			break;
		}
	}

	CleanupCallInfo();

	GlobalFree( lci );

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Exiting_telephony_poll )

	// Release the semaphore if we're killing the thread.
	if ( telephony_semaphore != NULL )
	{
		ReleaseSemaphore( telephony_semaphore, 1, NULL );
	}

	in_telephony_thread = false;

	// We're done. Let other telephony threads continue.
	LeaveCriticalSection( &tt_cs );

	_ExitThread( 0 );
	return 0;
}
