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
#include "lite_pcre2.h"
#include "list_operations.h"
#include "file_operations.h"
#include "utilities.h"
#include "message_log_utilities.h"
#include "menus.h"
#include "string_tables.h"

#include "telephony.h"

bool skip_list_draw = false;				// Prevents WM_DRAWITEM from accessing listview items while we're removing them.

void update_phone_number_matches( wchar_t *phone_number, unsigned char list_type, bool in_range, RANGE **range, bool add_value )
{
	if ( phone_number == NULL )
	{
		return;
	}

	if ( !in_range )	// Non-range phone number.
	{
		// Update each display_info item.
		DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )phone_number, true );
		if ( dll != NULL )
		{
			DoublyLinkedList *di_node = dll;
			while ( di_node != NULL )
			{
				display_info *mdi = ( display_info * )di_node->data;
				if ( mdi != NULL )
				{
					if ( list_type == LIST_TYPE_ALLOW )
					{
						mdi->allow_phone_number = add_value;
					}
					else
					{
						mdi->ignore_phone_number = add_value;
					}
				}

				di_node = di_node->next;
			}
		}
	}
	else	// See if the value we're adding is a range (has wildcard values in it).
	{
		wchar_t range_number[ 32 ];	// Dummy value.

		// Update each display_info item.
		node_type *node = dllrbt_get_head( call_log );
		while ( node != NULL )
		{
			DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
			while ( di_node != NULL )
			{
				display_info *mdi = ( display_info * )di_node->data;
				if ( mdi != NULL )
				{
					bool *phone_number_state;
					dllrbt_tree *list;

					if ( list_type == LIST_TYPE_ALLOW )
					{
						phone_number_state = &mdi->allow_phone_number;
						list = allow_list;
					}
					else
					{
						phone_number_state = &mdi->ignore_phone_number;
						list = ignore_list;
					}

					// Process display values that will be allowed/ignored/no longer allowed/ignored.
					if ( *phone_number_state != add_value )
					{
						// Handle values that are no longer allowed/ignored.
						if ( !add_value )
						{
							// First, see if the display value falls within another range.
							if ( range != NULL && RangeSearch( range, mdi->phone_number, range_number ) )
							{
								// If it does, then we'll skip the display value.
								break;
							}

							// Next, see if the display value exists as a non-range value.
							if ( dllrbt_find( list, ( void * )mdi->phone_number, false ) != NULL )
							{
								// If it does, then we'll skip the display value.
								break;
							}
						}

						// Finally, see if the display item falls within the range.
						if ( RangeCompare( phone_number, mdi->phone_number ) )
						{
							*phone_number_state = add_value;
						}
					}
				}

				di_node = di_node->next;
			}

			node = node->next;
		}
	}
}

// If the caller ID name matches a value in our info structure, then set the state if we're adding or removing it.
void update_caller_id_name_matches( void *cidi, unsigned char list_type, bool add_value )
{
	if ( cidi == NULL )
	{
		return;
	}

	wchar_t *caller_id = ( ( allow_ignore_cid_info * )cidi )->caller_id;
	bool match_case = ( ( ( allow_ignore_cid_info * )cidi )->match_flag & 0x02 ? true : false );
	bool match_whole_word = ( ( ( allow_ignore_cid_info * )cidi )->match_flag & 0x01 ? true : false );
	bool regular_expression = ( ( ( allow_ignore_cid_info * )cidi )->match_flag & 0x04 ? true : false );
	bool *active = &( ( ( allow_ignore_cid_info * )cidi )->active );

	bool update_cid_state = false;

	pcre2_code *regex_code = NULL;
	pcre2_match_data *regex_match_data = NULL;

	if ( regular_expression && g_use_regular_expressions )
	{
		int error_code;
		size_t error_offset;

		regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )caller_id, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );

		if ( regex_code != NULL )
		{
			regex_match_data = _pcre2_match_data_create_from_pattern_16( regex_code, NULL );
		}
	}

	// Update each display_info item to indicate that it is now allowed/ignored.
	node_type *node = dllrbt_get_head( call_log );
	while ( node != NULL )
	{
		DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
		while ( di_node != NULL )
		{
			display_info *mdi = ( display_info * )di_node->data;
			if ( mdi != NULL )
			{
				if ( regular_expression )
				{
					if ( g_use_regular_expressions )
					{
						if ( regex_match_data != NULL )
						{
							int caller_id_length = lstrlenW( mdi->caller_id );
							if ( _pcre2_match_16( regex_code, ( PCRE2_SPTR16 )mdi->caller_id, caller_id_length, 0, 0, regex_match_data, NULL ) >= 0 )
							{
								update_cid_state = true;
							}
						}
					}
				}
				else if ( match_case && match_whole_word )
				{
					if ( lstrcmpW( mdi->caller_id, caller_id ) == 0 )
					{
						update_cid_state = true;
					}
				}
				else if ( !match_case && match_whole_word )
				{
					if ( lstrcmpiW( mdi->caller_id, caller_id ) == 0 )
					{
						update_cid_state = true;
					}
				}
				else if ( match_case && !match_whole_word )
				{
					if ( _StrStrW( mdi->caller_id, caller_id ) != NULL )
					{
						update_cid_state = true;
					}
				}
				else if ( !match_case && !match_whole_word )
				{
					if ( _StrStrIW( mdi->caller_id, caller_id ) != NULL )
					{
						update_cid_state = true;
					}
				}

				if ( update_cid_state )
				{
					if ( add_value )
					{
						*active = true;

						if ( list_type == LIST_TYPE_ALLOW )
						{
							++( mdi->allow_cid_match_count );
						}
						else
						{
							++( mdi->ignore_cid_match_count );
						}
					}
					else
					{
						if ( list_type == LIST_TYPE_ALLOW )
						{
							--( mdi->allow_cid_match_count );

							if ( mdi->allow_cid_match_count == 0 )
							{
								*active = false;
							}
						}
						else
						{
							--( mdi->ignore_cid_match_count );

							if ( mdi->ignore_cid_match_count == 0 )
							{
								*active = false;
							}
						}
					}

					update_cid_state = false;
				}
			}

			di_node = di_node->next;
		}

		node = node->next;
	}

	if ( regex_match_data != NULL ) { _pcre2_match_data_free_16( regex_match_data ); }
	if ( regex_code != NULL ) { _pcre2_code_free_16( regex_code ); }
}

void LoadLists()
{
	HANDLE thread = NULL;

	if ( cfg_enable_call_log_history )
	{
		importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );

		// Include an empty string.
		iei->file_paths = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * ( MAX_PATH + 1 ) );
		_wmemcpy_s( iei->file_paths, MAX_PATH, base_directory, base_directory_length );
		_wmemcpy_s( iei->file_paths + ( base_directory_length + 1 ), MAX_PATH - ( base_directory_length - 1 ), L"call_log_history\0", 17 );
		iei->file_paths[ base_directory_length + 17 ] = 0;	// Sanity.
		iei->file_offset = ( unsigned short )( base_directory_length + 1 );
		iei->file_type = LOAD_CALL_LOG_HISTORY;

		// iei will be freed in the import_list thread.
		thread = ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL );
		if ( thread != NULL )
		{
			CloseHandle( thread );
		}
		else
		{
			GlobalFree( iei->file_paths );
			GlobalFree( iei );
		}
	}

	allow_ignore_update_info *aiui = ( allow_ignore_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_update_info ) );
	aiui->action = 2;	// Add all allow_list items.
	aiui->list_type = LIST_TYPE_ALLOW;	// Allow list.
	aiui->hWnd = g_hWnd_allow_list;

	// aiui is freed in the update_allow_ignore_list thread.
	thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_list, ( void * )aiui, 0, NULL );
	if ( thread != NULL )
	{
		CloseHandle( thread );
	}
	else
	{
		GlobalFree( aiui );
	}

	allow_ignore_cid_update_info *aicidui = ( allow_ignore_cid_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_update_info ) );
	aicidui->action = 2;	// Add all allow_cid_list items.
	aicidui->list_type = LIST_TYPE_ALLOW;	// Allow CID list.
	aicidui->hWnd = g_hWnd_allow_cid_list;

	// aicidui is freed in the update_allow_ignore_cid_list thread.
	thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_cid_list, ( void * )aicidui, 0, NULL );
	if ( thread != NULL )
	{
		CloseHandle( thread );
	}
	else
	{
		GlobalFree( aicidui );
	}

	//

	aiui = ( allow_ignore_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_update_info ) );
	aiui->action = 2;	// Add all ignore_list items.
	aiui->list_type = LIST_TYPE_IGNORE;	// Ignore list.
	aiui->hWnd = g_hWnd_ignore_list;

	// aiui is freed in the update_allow_ignore_list thread.
	thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_list, ( void * )aiui, 0, NULL );
	if ( thread != NULL )
	{
		CloseHandle( thread );
	}
	else
	{
		GlobalFree( aiui );
	}

	aicidui = ( allow_ignore_cid_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_update_info ) );
	aicidui->action = 2;	// Add all ignore_cid_list items.
	aicidui->list_type = LIST_TYPE_IGNORE;	// Ignore CID list.
	aicidui->hWnd = g_hWnd_ignore_cid_list;

	// aicidui is freed in the update_allow_ignore_cid_list thread.
	thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_cid_list, ( void * )aicidui, 0, NULL );
	if ( thread != NULL )
	{
		CloseHandle( thread );
	}
	else
	{
		GlobalFree( aicidui );
	}

	contact_update_info *cui = ( contact_update_info * )GlobalAlloc( GPTR, sizeof( contact_update_info ) );
	cui->action = 2;
				
	thread = ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )cui, 0, NULL );
	if ( thread != NULL )
	{
		CloseHandle( thread );
	}
	else
	{
		GlobalFree( cui );
	}
}

THREAD_RETURN export_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	importexportinfo *iei = ( importexportinfo * )pArguments;
	if ( iei != NULL )
	{
		switch ( iei->file_type )
		{
			case IE_ALLOW_PN_LIST:		{ hWnd = g_hWnd_allow_list; } break;
			case IE_ALLOW_CID_LIST:		{ hWnd = g_hWnd_allow_cid_list; } break;
			case IE_IGNORE_PN_LIST:		{ hWnd = g_hWnd_ignore_list; } break;
			case IE_IGNORE_CID_LIST:	{ hWnd = g_hWnd_ignore_cid_list; } break;
			case IE_CONTACT_LIST:		{ hWnd = g_hWnd_contact_list; } break;
			case IE_CALL_LOG_HISTORY:	{ hWnd = g_hWnd_call_log; } break;
		}
	}

	Processing_Window( hWnd, false );

	if ( iei != NULL )
	{
		if ( iei->file_paths != NULL )
		{
			wchar_t *msg = NULL;

			switch ( iei->file_type )
			{
				case IE_ALLOW_PN_LIST:		{ save_allow_ignore_list( iei->file_paths, allow_list, LIST_TYPE_ALLOW );			msg = ST_Exported_allow_phone_number_list; } break;
				case IE_ALLOW_CID_LIST:		{ save_allow_ignore_cid_list( iei->file_paths, allow_cid_list, LIST_TYPE_ALLOW );	msg = ST_Exported_allow_caller_ID_name_list; } break;
				case IE_IGNORE_PN_LIST:		{ save_allow_ignore_list( iei->file_paths, ignore_list, LIST_TYPE_IGNORE );			msg = ST_Exported_ignore_phone_number_list; } break;
				case IE_IGNORE_CID_LIST:	{ save_allow_ignore_cid_list( iei->file_paths, ignore_cid_list, LIST_TYPE_IGNORE );	msg = ST_Exported_ignore_caller_ID_name_list; } break;
				case IE_CONTACT_LIST:		{ save_contact_list( iei->file_paths );												msg = ST_Exported_contact_list; } break;
				case IE_CALL_LOG_HISTORY:	{ save_call_log_history( iei->file_paths );											msg = ST_Exported_call_log_history; } break;
			}

			GlobalFree( iei->file_paths );

			MESSAGE_LOG_OUTPUT( ML_NOTICE, msg )
		}

		GlobalFree( iei );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN import_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	importexportinfo *iei = ( importexportinfo * )pArguments;
	if ( iei != NULL )
	{
		switch ( iei->file_type )
		{
			case IE_ALLOW_PN_LIST:		{ hWnd = g_hWnd_allow_list; } break;
			case IE_ALLOW_CID_LIST:		{ hWnd = g_hWnd_allow_cid_list; } break;
			case IE_IGNORE_PN_LIST:		{ hWnd = g_hWnd_ignore_list; } break;
			case IE_IGNORE_CID_LIST:	{ hWnd = g_hWnd_ignore_cid_list; } break;
			case IE_CONTACT_LIST:		{ hWnd = g_hWnd_contact_list; } break;
			case IE_CALL_LOG_HISTORY:
			case LOAD_CALL_LOG_HISTORY:	{ hWnd = g_hWnd_call_log; } break;
		}
	}

	Processing_Window( hWnd, false );

	wchar_t file_path[ MAX_PATH ];
	wchar_t *filename = NULL;

	char status = 0, ml_type = 0;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	if ( iei != NULL )
	{
		if ( iei->file_paths != NULL )
		{
			filename = iei->file_paths;	// The last file path will be an empty string.

			// The first string is actually a directory.
			_wmemcpy_s( file_path, MAX_PATH, filename, iei->file_offset );
			file_path[ iei->file_offset - 1 ] = '\\';

			filename += iei->file_offset;

			// Make sure the path is not the empty string.
			while ( *filename != NULL )
			{
				int filename_length = lstrlenW( filename ) + 1;	// Include the NULL terminator.

				_wmemcpy_s( file_path + iei->file_offset, MAX_PATH - iei->file_offset, filename, filename_length );

				if ( iei->file_type == IE_CALL_LOG_HISTORY || iei->file_type == LOAD_CALL_LOG_HISTORY )
				{
					status = read_call_log_history( file_path );

					// Only save if we've imported - not loaded during startup.
					if ( iei->file_type == IE_CALL_LOG_HISTORY )
					{
						call_log_changed = true;	// Assume that entries were added so that we can save the new log during shutdown.
					}
				}
				else
				{
					// Go through our new list and add its items into our global list. Free any duplicates.
					// Items will be added to their appropriate range lists when read.
					switch ( iei->file_type )
					{
						case IE_ALLOW_PN_LIST:
						case IE_IGNORE_PN_LIST:
						{
							dllrbt_tree *new_list = dllrbt_create( dllrbt_compare_w );

							unsigned char list_type;
							dllrbt_tree *list;
							bool *changed;

							if ( iei->file_type == IE_ALLOW_PN_LIST )
							{
								list_type = 0;
								list = allow_list;
								changed = &allow_list_changed;
							}
							else
							{
								list_type = 1;
								list = ignore_list;
								changed = &ignore_list_changed;
							}

							status = read_allow_ignore_list( file_path, new_list, list_type );

							node_type *node = dllrbt_get_head( new_list );
							while ( node != NULL )
							{
								allow_ignore_info *aii = ( allow_ignore_info * )node->val;
								if ( aii != NULL )
								{
									// Attempt to insert the new item into the global list.
									if ( dllrbt_insert( list, ( void * )aii->phone_number, ( void * )aii ) == DLLRBT_STATUS_OK )
									{
										update_phone_number_matches( aii->phone_number, list_type, ( is_num_w( aii->phone_number ) == 1 ? true : false ), NULL, true );

										*changed = true;

										lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
										lvi.lParam = ( LPARAM )aii;
										_SendMessageW( hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
									}
									else	// Problem/duplicate
									{
										free_allowignoreinfo( &aii );
									}
								}

								node = node->next;
							}

							// All values stored in the tree will either have been added to a global list, or freed.
							dllrbt_delete_recursively( new_list );
						}
						break;

						case IE_ALLOW_CID_LIST:
						case IE_IGNORE_CID_LIST:
						{
							dllrbt_tree *new_list = dllrbt_create( dllrbt_icid_compare );

							unsigned char list_type;
							dllrbt_tree *list;
							bool *changed;

							if ( iei->file_type == IE_ALLOW_CID_LIST )
							{
								list_type = 0;
								list = allow_cid_list;
								changed = &allow_cid_list_changed;
							}
							else
							{
								list_type = 1;
								list = ignore_cid_list;
								changed = &ignore_cid_list_changed;
							}

							status = read_allow_ignore_cid_list( file_path, new_list, list_type );

							node_type *node = dllrbt_get_head( new_list );
							while ( node != NULL )
							{
								allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )node->val;
								if ( aicidi != NULL )
								{
									// Attempt to insert the new item into the global list.
									if ( dllrbt_insert( list, ( void * )aicidi, ( void * )aicidi ) == DLLRBT_STATUS_OK )
									{
										update_caller_id_name_matches( ( void * )aicidi, list_type, true );

										*changed = true;

										lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
										lvi.lParam = ( LPARAM )aicidi;
										_SendMessageW( hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
									}
									else	// Problem/duplicate
									{
										free_allowignorecidinfo( &aicidi );
									}
								}

								node = node->next;
							}

							// All values stored in the tree will either have been added to a global list, or freed.
							dllrbt_delete_recursively( new_list );
						}
						break;

						case IE_CONTACT_LIST:
						{
							dllrbt_tree *new_list = dllrbt_create( dllrbt_compare_ptr );

							status = read_contact_list( file_path, new_list );

							node_type *node = dllrbt_get_head( new_list );
							while ( node != NULL )
							{
								contact_info *ci = ( contact_info * )node->val;
								if ( ci != NULL )
								{
									// Attempt to insert the new item into the global list.
									if ( dllrbt_insert( contact_list, ( void * )ci, ( void * )ci ) == DLLRBT_STATUS_OK )
									{
										add_custom_caller_id_w( ci );

										contact_list_changed = true;

										lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
										lvi.lParam = ( LPARAM )ci;
										_SendMessageW( hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
									}
									else	// Problem/duplicate
									{
										free_contactinfo( &ci );
									}
								}

								node = node->next;
							}

							// All values stored in the tree will either have been added to a global list, or freed.
							dllrbt_delete_recursively( new_list );
						}
						break;
					}
				}

				if ( status == 0 )
				{
					ml_type |= 1;
				}
				else if ( status == -2 )
				{
					ml_type |= 2;
				}

				// Move to the next string.
				filename = filename + filename_length;
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );

			GlobalFree( iei->file_paths );

			if ( ml_type & 1 )
			{
				wchar_t *msg = NULL;

				switch ( iei->file_type )
				{
					case IE_ALLOW_PN_LIST:		{ msg = ST_Imported_allow_phone_number_list; } break;
					case IE_ALLOW_CID_LIST:		{ msg = ST_Imported_allow_caller_ID_name_list; } break;
					case IE_IGNORE_PN_LIST:		{ msg = ST_Imported_ignore_phone_number_list; } break;
					case IE_IGNORE_CID_LIST:	{ msg = ST_Imported_ignore_caller_ID_name_list; } break;
					case IE_CONTACT_LIST:		{ msg = ST_Imported_contact_list; } break;
					case IE_CALL_LOG_HISTORY:	{ msg = ST_Imported_call_log_history; } break;
					case LOAD_CALL_LOG_HISTORY:	{ msg = ST_Loaded_call_log_history; } break;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, msg )
			}

			if ( ml_type & 2 )	// Bad format.
			{
				if ( iei->file_type != LOAD_CALL_LOG_HISTORY )
				{
					MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_File_format_is_incorrect )
				}
			}
		}

		GlobalFree( iei );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN remove_items( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	// Prevent the listviews from drawing while freeing lParam values.
	skip_list_draw = true;

	removeinfo *ri = ( removeinfo * )pArguments;

	HWND hWnd = ri->hWnd;
	bool is_thread = ri->is_thread;

	if ( is_thread )
	{
		Processing_Window( hWnd, false );
	}

	wchar_t range_number[ 32 ];	// Dummy value.

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	int item_count = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

	int *index_array = NULL;

	bool handle_all = false;
	if ( item_count == sel_count )
	{
		handle_all = true;
	}
	else
	{
		_SendMessageW( hWnd, LVM_ENSUREVISIBLE, 0, FALSE );

		index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );

		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

		// Create an index list of selected items (in reverse order).
		for ( int i = 0; i < sel_count; i++ )
		{
			lvi.iItem = index_array[ sel_count - 1 - i ] = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		item_count = sel_count;
	}

	// Go through each item, and free their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		if ( handle_all )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = index_array[ i ];
		}

		_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

		if ( hWnd == g_hWnd_call_log )
		{
			display_info *di = ( display_info * )lvi.lParam;
			if ( di != NULL )
			{
				// Update each display_info item.
				dllrbt_iterator *itr = dllrbt_find( call_log, ( void * )di->phone_number, false );
				if ( itr != NULL )
				{
					// Head of the linked list.
					DoublyLinkedList *ll = ( DoublyLinkedList * )( ( node_type * )itr )->val;

					// Go through each linked list node and remove the one with di.

					DoublyLinkedList *current_node = ll;
					while ( current_node != NULL )
					{
						if ( ( display_info * )current_node->data == di )
						{
							DLL_RemoveNode( &ll, current_node );
							GlobalFree( current_node );

							if ( ll != NULL && ll->data != NULL )
							{
								// Reset the head in the tree.
								( ( node_type * )itr )->val = ( void * )ll;
								( ( node_type * )itr )->key = ( void * )( ( display_info * )ll->data )->phone_number;

								call_log_changed = true;
							}

							break;
						}

						current_node = current_node->next;
					}

					// If the head of the linked list is NULL, then we can remove the linked list from the call log tree.
					if ( ll == NULL )
					{
						dllrbt_remove( call_log, itr );	// Remove the node from the tree. The tree will rebalance itself.

						call_log_changed = true;
					}
				}

				free_displayinfo( &di );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_call_log_entry___entries ) }
		}
		else if ( hWnd == g_hWnd_contact_list )
		{
			contact_info *ci = ( contact_info * )lvi.lParam;
			if ( ci != NULL )
			{
				dllrbt_iterator *itr = dllrbt_find( contact_list, ( void * )ci, false );
				if ( itr != NULL )
				{
					dllrbt_remove( contact_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					contact_list_changed = true;	// Causes us to save the contact_list on shutdown.
				}

				remove_custom_caller_id_w( ci );

				free_contactinfo( &ci );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Deleted_contact ) }
		}
		else if ( hWnd == g_hWnd_allow_list )
		{
			allow_ignore_info *aii = ( allow_ignore_info * )lvi.lParam;
			if ( aii != NULL )
			{
				int range_index = lstrlenW( aii->phone_number );
				range_index = ( range_index > 0 ? range_index - 1 : 0 );

				// If the number we've removed is a range, then remove it from the range list.
				if ( is_num_w( aii->phone_number ) == 1 )
				{
					RangeRemove( &allow_range_list[ range_index ], aii->phone_number );

					// Update each display_info item to indicate that it is no longer allowed.
					update_phone_number_matches( aii->phone_number, 0, true, &allow_range_list[ range_index ], false );
				}
				else
				{
					// See if the value we remove falls within a range. If it doesn't, then set its new display values.
					if ( !RangeSearch( &allow_range_list[ range_index ], aii->phone_number, range_number ) )
					{
						// Update each display_info item to indicate that it is no longer allowed.
						update_phone_number_matches( aii->phone_number, 0, false, NULL, false );
					}
				}

				// See if the allow_list phone number exits. It should.
				dllrbt_iterator *itr = dllrbt_find( allow_list, ( void * )aii->phone_number, false );
				if ( itr != NULL )
				{
					dllrbt_remove( allow_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					allow_list_changed = true;	// Causes us to save the allow_list on shutdown.
				}

				free_allowignoreinfo( &aii );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_phone_number_s__from_allow_phone_number_list ) }
		}
		else if ( hWnd == g_hWnd_ignore_list )
		{
			allow_ignore_info *aii = ( allow_ignore_info * )lvi.lParam;
			if ( aii != NULL )
			{
				int range_index = lstrlenW( aii->phone_number );
				range_index = ( range_index > 0 ? range_index - 1 : 0 );

				// If the number we've removed is a range, then remove it from the range list.
				if ( is_num_w( aii->phone_number ) == 1 )
				{
					RangeRemove( &ignore_range_list[ range_index ], aii->phone_number );

					// Update each display_info item to indicate that it is no longer ignored.
					update_phone_number_matches( aii->phone_number, 1, true, &ignore_range_list[ range_index ], false );
				}
				else
				{
					// See if the value we remove falls within a range. If it doesn't, then set its new display values.
					if ( !RangeSearch( &ignore_range_list[ range_index ], aii->phone_number, range_number ) )
					{
						// Update each display_info item to indicate that it is no longer ignored.
						update_phone_number_matches( aii->phone_number, 1, false, NULL, false );
					}
				}

				// See if the ignore_list phone number exits. It should.
				dllrbt_iterator *itr = dllrbt_find( ignore_list, ( void * )aii->phone_number, false );
				if ( itr != NULL )
				{
					dllrbt_remove( ignore_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					ignore_list_changed = true;	// Causes us to save the ignore_list on shutdown.
				}

				free_allowignoreinfo( &aii );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_phone_number_s__from_ignore_phone_number_list ) }
		}
		else if ( hWnd == g_hWnd_allow_cid_list )
		{
			allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )lvi.lParam;
			if ( aicidi != NULL )
			{
				update_caller_id_name_matches( ( void * )aicidi, 0, false );

				// See if the allow_cid_list value exits. It should.
				dllrbt_iterator *itr = dllrbt_find( allow_cid_list, ( void * )aicidi, false );
				if ( itr != NULL )
				{
					dllrbt_remove( allow_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					allow_cid_list_changed = true;	// Causes us to save the allow_cid_list on shutdown.
				}

				free_allowignorecidinfo( &aicidi );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_caller_ID_name_s__from_allow_caller_ID_name_list ) }
		}
		else if ( hWnd == g_hWnd_ignore_cid_list )
		{
			allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )lvi.lParam;
			if ( aicidi != NULL )
			{
				update_caller_id_name_matches( ( void * )aicidi, 1, false );

				// See if the ignore_cid_list value exits. It should.
				dllrbt_iterator *itr = dllrbt_find( ignore_cid_list, ( void * )aicidi, false );
				if ( itr != NULL )
				{
					dllrbt_remove( ignore_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					ignore_cid_list_changed = true;	// Causes us to save the ignore_cid_list on shutdown.
				}

				free_allowignorecidinfo( &aicidi );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_caller_ID_name_s__from_ignore_caller_ID_name_list ) }
		}

		if ( !handle_all )
		{
			_SendMessageW( hWnd, LVM_DELETEITEM, index_array[ i ], 0 );
		}
	}

	if ( handle_all )
	{
		_SendMessageW( hWnd, LVM_DELETEALLITEMS, 0, 0 );
	}
	else
	{
		GlobalFree( index_array );
	}

	skip_list_draw = false;

	if ( is_thread )
	{
		GlobalFree( ri );

		Processing_Window( hWnd, true );
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	if ( is_thread )
	{
		_ExitThread( 0 );
	}
	return 0;
}

THREAD_RETURN update_allow_ignore_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	allow_ignore_update_info *aiui = ( allow_ignore_update_info * )pArguments;
	if ( aiui != NULL )
	{
		hWnd = aiui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( aiui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		HWND c_hWnd;
		dllrbt_tree *list;
		bool *changed;

		if ( aiui->list_type == LIST_TYPE_ALLOW )
		{
			list = allow_list;
			changed = &allow_list_changed;
			c_hWnd = g_hWnd_allow_list;
		}
		else
		{
			list = ignore_list;
			changed = &ignore_list_changed;
			c_hWnd = g_hWnd_ignore_list;
		}

		if ( aiui->hWnd == g_hWnd_allow_list || aiui->hWnd == g_hWnd_ignore_list )
		{
			if ( aiui->action == 0 )	// Add a single item to allow/ignore_list and allow/ignore list listview.
			{
				// Zero init.
				allow_ignore_info *aii = ( allow_ignore_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_info ) );

				aii->phone_number = aiui->phone_number;

				// Try to insert the allow/ignore_list info in the tree.
				if ( dllrbt_insert( list, ( void * )aii->phone_number, ( void * )aii ) != DLLRBT_STATUS_OK )
				{
					free_allowignoreinfo( &aii );

					MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_already_in_allow_list : ST_already_in_ignore_list ) )
				}
				else	// If it was able to be inserted into the tree, then update our display_info items and the ignore list listview.
				{
					aii->w_phone_number = FormatPhoneNumberW( aii->phone_number );

					aii->count = 0;
					aii->state = 0;

					// See if the value we're adding is a range (has wildcard values in it).
					bool is_range = ( is_num_w( aii->phone_number ) == 1 ? true : false );
					if ( is_range )
					{
						int phone_number_length = lstrlenW( aii->phone_number );

						if ( aiui->list_type == LIST_TYPE_ALLOW )
						{
							RangeAdd( &allow_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], aii->phone_number, phone_number_length );
						}
						else
						{
							RangeAdd( &ignore_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], aii->phone_number, phone_number_length );
						}
					}

					// Update each display_info item to indicate that it is now allowed/ignored.
					update_phone_number_matches( aii->phone_number, aiui->list_type, is_range, NULL, true );

					*changed = true;

					lvi.iItem = ( int )_SendMessageW( aiui->hWnd, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )aii;	// lParam = our contact_info structure from the connection thread.
					_SendMessageW( aiui->hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_Added_phone_number_to_allow_phone_number_list : ST_Added_phone_number_to_ignore_phone_number_list ) )
				}
			}
			else if ( aiui->action == 1 )	// Remove from allow/ignore_list and allow/ignore list listview.
			{
				removeinfo ri;
				ri.is_thread = false;
				ri.hWnd = aiui->hWnd;
				remove_items( ( void * )&ri );
			}
			else if ( aiui->action == 2 )	// Add all items in the allow/ignore_list to the allow/ignore list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( list );
				while ( node != NULL )
				{
					allow_ignore_info *aii = ( allow_ignore_info * )node->val;
					if ( aii != NULL )
					{
						lvi.iItem = ( int )_SendMessageW( aiui->hWnd, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )aii;	// lParam = our contact_info structure from the connection thread.
						_SendMessageW( aiui->hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_Loaded_allow_phone_number_list : ST_Loaded_ignore_phone_number_list ) )
			}
			else if ( aiui->action == 3 )	// Update entry.
			{
				allow_ignore_info *old_aii = aiui->aii;
				if ( old_aii != NULL )
				{
					// See if the new phone number is in our allow/ignore_list.
					dllrbt_iterator *itr = dllrbt_find( list, ( void * )aiui->phone_number, false );
					if ( itr == NULL )
					{
						// See if the old phone number is in our allow/ignore_list.
						itr = dllrbt_find( list, ( void * )old_aii->phone_number, false );
						if ( itr != NULL )
						{
							dllrbt_remove( list, itr );	// Remove the node from the tree. The tree will rebalance itself.

							wchar_t range_number[ 32 ];	// Dummy value.

							int range_index = lstrlenW( old_aii->phone_number );
							range_index = ( range_index > 0 ? range_index - 1 : 0 );

							// If the number we've removed is a range, then remove it from the range list.
							if ( is_num_w( old_aii->phone_number ) == 1 )
							{
								if ( aiui->list_type == LIST_TYPE_ALLOW )
								{
									RangeRemove( &allow_range_list[ range_index ], old_aii->phone_number );

									// Update each display_info item to indicate that it is no longer allowed.
									update_phone_number_matches( old_aii->phone_number, 0, true, &allow_range_list[ range_index ], false );
								}
								else
								{
									RangeRemove( &ignore_range_list[ range_index ], old_aii->phone_number );

									// Update each display_info item to indicate that it is no longer ignored.
									update_phone_number_matches( old_aii->phone_number, 1, true, &ignore_range_list[ range_index ], false );
								}
							}
							else
							{
								bool in_range;

								if ( aiui->list_type == LIST_TYPE_ALLOW )
								{
									in_range = RangeSearch( &allow_range_list[ range_index ], old_aii->phone_number, range_number );
								}
								else
								{
									in_range = RangeSearch( &ignore_range_list[ range_index ], old_aii->phone_number, range_number );
								}

								// See if the value we remove falls within a range. If it doesn't, then set its new display values.
								if ( !in_range )
								{
									// Update each display_info item to indicate that it is no longer allowed/ignored.
									update_phone_number_matches( old_aii->phone_number, aiui->list_type, false, NULL, false );
								}
							}

							GlobalFree( old_aii->phone_number );
							GlobalFree( old_aii->w_phone_number );
							GlobalFree( old_aii->w_last_called );
							old_aii->w_last_called = NULL;
							old_aii->last_called.QuadPart = 0;

							old_aii->phone_number = aiui->phone_number;

							// Try to insert the allow/ignore_list info in the tree.
							if ( dllrbt_insert( list, ( void * )old_aii->phone_number, ( void * )old_aii ) != DLLRBT_STATUS_OK )
							{
								free_allowignoreinfo( &old_aii );

								MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_already_in_allow_list : ST_already_in_ignore_list ) )
							}
							else	// If it was able to be inserted into the tree, then update our display_info items and the ignore list listview.
							{
								old_aii->w_phone_number = FormatPhoneNumberW( old_aii->phone_number );

								old_aii->count = 0;
								old_aii->state = 0;

								// See if the value we're adding is a range (has wildcard values in it).
								bool is_range = ( is_num_w( old_aii->phone_number ) == 1 ? true : false );
								if ( is_range )
								{
									int phone_number_length = lstrlenW( old_aii->phone_number );

									if ( aiui->list_type == LIST_TYPE_ALLOW )
									{
										RangeAdd( &allow_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], old_aii->phone_number, phone_number_length );
									}
									else
									{
										RangeAdd( &ignore_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], old_aii->phone_number, phone_number_length );
									}
								}

								// Update each display_info item to indicate that it is now allowed/ignored.
								update_phone_number_matches( old_aii->phone_number, aiui->list_type, is_range, NULL, true );

								*changed = true;

								MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_Updated_allowed_phone_number : ST_Updated_ignored_phone_number ) )
							}
						}
					}
					else
					{
						MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_already_in_allow_list : ST_already_in_ignore_list ) )

						GlobalFree( aiui->phone_number );
					}
				}
			}
			else if ( aiui->action == 4 )	// Update the recently called number's call count.
			{
				if ( aiui->aii != NULL )
				{
					++( aiui->aii->count );

					if ( aiui->last_called.QuadPart > 0 )
					{
						aiui->aii->last_called = aiui->last_called;

						GlobalFree( aiui->aii->w_last_called );

						aiui->aii->w_last_called = FormatTimestamp( aiui->aii->last_called );
					}

					*changed = true;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_Updated_allowed_phone_number_s_call_count : ST_Updated_ignored_phone_number_s_call_count ) )
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( aiui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );

			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the allow/ignore list listview and remove the items that have changed state.
			bool skip_range_warning = false;	// Skip the range warning if we've already displayed it.

			int last_selected = lvi.iItem;

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				if ( handle_all )
				{
					lvi.iItem = i;
				}
				else
				{
					last_selected = lvi.iItem = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				display_info *di = ( display_info * )lvi.lParam;
				if ( di != NULL )
				{
					bool *phone_number_state = ( aiui->list_type == LIST_TYPE_ALLOW ? &di->allow_phone_number : &di->ignore_phone_number );

					if ( *phone_number_state && aiui->action == 1 )		// Remove from allow/ignore_list.
					{
						wchar_t range_number[ 32 ];	// Dummy value.

						int range_index = lstrlenW( di->phone_number );
						range_index = ( range_index > 0 ? range_index - 1 : 0 );

						// First, see if the phone number is in our range list.
						bool in_range;
						if ( aiui->list_type == LIST_TYPE_ALLOW )
						{
							in_range = RangeSearch( &allow_range_list[ range_index ], di->phone_number, range_number );
						}
						else
						{
							in_range = RangeSearch( &ignore_range_list[ range_index ], di->phone_number, range_number );
						}

						if ( !in_range )
						{
							// If not, then see if the phone number is in our allow/ignore_list.
							dllrbt_iterator *itr = dllrbt_find( list, ( void * )di->phone_number, false );
							if ( itr != NULL )
							{
								// Update each display_info item to indicate that it is no longer allowed/ignored.
								update_phone_number_matches( di->phone_number, aiui->list_type, false, NULL, false );

								// If an allow/ignore list listview item also shows up in the call log listview, then we'll remove it after this loop.
								allow_ignore_info *aii = ( allow_ignore_info * )( ( node_type * )itr )->val;
								if ( aii != NULL )
								{
									aii->state = 1;
								}
								remove_state_changed = true;	// This will let us go through the allow/ignore list listview and free items with a state == 1.

								dllrbt_remove( list, itr );	// Remove the node from the tree. The tree will rebalance itself.

								*changed = true;	// Causes us to save the allow/ignore_list on shutdown.
							}

							*phone_number_state = false;
						}
						else
						{
							if ( !skip_range_warning )
							{
								MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_remove_from_allow_phone_number_list : ST_remove_from_ignore_phone_number_list ) )
								skip_range_warning = true;
							}
						}
					}
					else if ( !( *phone_number_state ) && aiui->action == 0 )	// Add to allow/ignore_list.
					{
						// Zero init.
						allow_ignore_info *aii = ( allow_ignore_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_info ) );

						aii->phone_number = GlobalStrDupW( di->phone_number );

						if ( dllrbt_insert( list, ( void * )aii->phone_number, ( void * )aii ) != DLLRBT_STATUS_OK )
						{
							free_allowignoreinfo( &aii );

							MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_already_in_allow_list : ST_already_in_ignore_list ) )
						}
						else	// Add to allow/ignore list listview as well.
						{
							aii->w_phone_number = FormatPhoneNumberW( aii->phone_number );

							aii->count = 0;
							aii->state = 0;

							// Update all nodes if it already exits.
							update_phone_number_matches( aii->phone_number, aiui->list_type, false, NULL, true );

							*phone_number_state = true;

							*changed = true;

							lvi.iItem = ( int )_SendMessageW( c_hWnd, LVM_GETITEMCOUNT, 0, 0 );
							lvi.lParam = ( LPARAM )aii;
							_SendMessageW( c_hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

							if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_Added_phone_number_s__to_allow_phone_number_list :
																												  ST_Added_phone_number_s__to_ignore_phone_number_list ) ) }
						}
					}
				}
			}

			// If we removed items in the call log listview from the allow/ignore_list, then remove them from the allow/ignore list listview.
			if ( remove_state_changed )
			{
				int item_count = ( int )_SendMessageW( c_hWnd, LVM_GETITEMCOUNT, 0, 0 );

				skip_list_draw = true;
				_EnableWindow( c_hWnd, FALSE );

				// Start from the end and work backwards.
				for ( lvi.iItem = item_count - 1; lvi.iItem >= 0; --lvi.iItem )
				{
					_SendMessageW( c_hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

					allow_ignore_info *aii = ( allow_ignore_info * )lvi.lParam;
					if ( aii != NULL && aii->state == 1 )
					{
						free_allowignoreinfo( &aii );

						_SendMessageW( c_hWnd, LVM_DELETEITEM, lvi.iItem, 0 );
					}
				}

				skip_list_draw = false;
				_EnableWindow( c_hWnd, TRUE );

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aiui->list_type == LIST_TYPE_ALLOW ? ST_Removed_phone_number_s__from_allow_phone_number_list :
																					  ST_Removed_phone_number_s__from_ignore_phone_number_list ) )
			}
		}

		GlobalFree( aiui );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_allow_ignore_cid_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	allow_ignore_cid_update_info *aicidui = ( allow_ignore_cid_update_info * )pArguments;
	if ( aicidui != NULL )
	{
		hWnd = aicidui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( aicidui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		HWND c_hWnd;
		dllrbt_tree *list;
		bool *changed;

		if ( aicidui->list_type == LIST_TYPE_ALLOW )
		{
			list = allow_cid_list;
			changed = &allow_cid_list_changed;
			c_hWnd = g_hWnd_allow_cid_list;
		}
		else
		{
			list = ignore_cid_list;
			changed = &ignore_cid_list_changed;
			c_hWnd = g_hWnd_ignore_cid_list;
		}

		if ( aicidui->hWnd == g_hWnd_allow_cid_list || aicidui->hWnd == g_hWnd_ignore_cid_list )
		{
			if ( aicidui->action == 0 )	// Add a single item to allow/ignore_cid_list and allow/ignore list listview.
			{
				// Zero init.
				allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_info ) );

				aicidi->caller_id = aicidui->caller_id;

				aicidi->match_flag = aicidui->match_flag;

				// Try to insert the allow/ignore_cid_list info in the tree.
				if ( dllrbt_insert( list, ( void * )aicidi, ( void * )aicidi ) != DLLRBT_STATUS_OK )
				{
					free_allowignorecidinfo( &aicidi );

					MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_cid_already_in_allow_cid_list : ST_cid_already_in_ignore_cid_list ) )
				}
				else	// If it was able to be inserted into the tree, then update our display_info items and the allow/ignore cid list listview.
				{
					aicidi->count = 0;
					aicidi->state = 0;
					aicidi->active = false;

					update_caller_id_name_matches( ( void * )aicidi, aicidui->list_type, true );

					*changed = true;

					lvi.iItem = ( int )_SendMessageW( aicidui->hWnd, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )aicidi;
					_SendMessageW( aicidui->hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_Added_caller_ID_name_to_allow_caller_ID_name_list : ST_Added_caller_ID_name_to_ignore_caller_ID_name_list ) )
				}
			}
			else if ( aicidui->action == 1 )	// Remove from allow/ignore_cid_list and allow/ignore cid list listview.
			{
				removeinfo ri;
				ri.is_thread = false;
				ri.hWnd = aicidui->hWnd;
				remove_items( ( void * )&ri );
			}
			else if ( aicidui->action == 2 )	// Add all items in the allow/ignore_cid_list to the allow/ignore cid list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( list );
				while ( node != NULL )
				{
					allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )node->val;
					if ( aicidi != NULL )
					{
						lvi.iItem = ( int )_SendMessageW( aicidui->hWnd, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )aicidi;	// lParam = our contact_info structure from the connection thread.
						_SendMessageW( aicidui->hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_Loaded_allow_caller_ID_name_list : ST_Loaded_ignore_caller_ID_name_list ) )
			}
			else if ( aicidui->action == 3 )	// Update entry
			{
				allow_ignore_cid_info *old_aicidi = aicidui->aicidi;
				if ( old_aicidi != NULL )
				{
					// This temporary structure is used to search our allow/ignore_cid_list tree.
					allow_ignore_cid_info new_aicidi;

					if ( aicidui->caller_id != NULL )
					{
						new_aicidi.caller_id = aicidui->caller_id;
					}
					else
					{
						new_aicidi.caller_id = old_aicidi->caller_id;	// DO NOT FREE THIS VALUE.
					}

					new_aicidi.match_flag = aicidui->match_flag;

					// See if our new entry already exists.
					dllrbt_iterator *itr = dllrbt_find( list, ( void * )&new_aicidi, false );
					if ( itr == NULL )
					{
						bool keep_active = false;
						bool update_cid_state1 = false, update_cid_state2 = false;

						pcre2_code *old_regex_code = NULL;
						pcre2_match_data *old_regex_match_data = NULL;

						pcre2_code *new_regex_code = NULL;
						pcre2_match_data *new_regex_match_data = NULL;

						int error_code;
						size_t error_offset;

						if ( g_use_regular_expressions )
						{
							if ( ( old_aicidi->match_flag & 0x04 ) )
							{
								old_regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )old_aicidi->caller_id, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );
								if ( old_regex_code != NULL )
								{
									old_regex_match_data = _pcre2_match_data_create_from_pattern_16( old_regex_code, NULL );
								}
							}

							if ( ( new_aicidi.match_flag & 0x04 ) )
							{
								new_regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )new_aicidi.caller_id, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );
								if ( new_regex_code != NULL )
								{
									new_regex_match_data = _pcre2_match_data_create_from_pattern_16( new_regex_code, NULL );
								}
							}
						}

						// Update each display_info item to indicate that it is allowed/ignored or no longer allowed/ignored.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								display_info *mdi = ( display_info * )di_node->data;
								if ( mdi != NULL )
								{
									// Find our old matches.
									if ( old_aicidi->match_flag & 0x04 )
									{
										if ( old_regex_match_data != NULL )
										{
											int caller_id_length = lstrlenW( mdi->caller_id );
											if ( _pcre2_match_16( old_regex_code, ( PCRE2_SPTR16 )mdi->caller_id, caller_id_length, 0, 0, old_regex_match_data, NULL ) >= 0 )
											{
												update_cid_state1 = true;
											}
										}
									}
									else if ( ( old_aicidi->match_flag & 0x02 ) && ( old_aicidi->match_flag & 0x01 ) )
									{
										if ( lstrcmpW( mdi->caller_id, old_aicidi->caller_id ) == 0 )
										{
											update_cid_state1 = true;
										}
									}
									else if ( !( old_aicidi->match_flag & 0x02 ) && ( old_aicidi->match_flag & 0x01 ) )
									{
										if ( lstrcmpiW( mdi->caller_id, old_aicidi->caller_id ) == 0 )
										{
											update_cid_state1 = true;
										}
									}
									else if ( ( old_aicidi->match_flag & 0x02 ) && !( old_aicidi->match_flag & 0x01 ) )
									{
										if ( _StrStrW( mdi->caller_id, old_aicidi->caller_id ) != NULL )
										{
											update_cid_state1 = true;
										}
									}
									else if ( !( old_aicidi->match_flag & 0x02 ) && !( old_aicidi->match_flag & 0x01 ) )
									{
										if ( _StrStrIW( mdi->caller_id, old_aicidi->caller_id ) != NULL )
										{
											update_cid_state1 = true;
										}
									}

									// Find our new matches.
									if ( new_aicidi.match_flag & 0x04 )
									{
										if ( new_regex_match_data != NULL )
										{
											int caller_id_length = lstrlenW( mdi->caller_id );
											if ( _pcre2_match_16( new_regex_code, ( PCRE2_SPTR16 )mdi->caller_id, caller_id_length, 0, 0, new_regex_match_data, NULL ) >= 0 )
											{
												update_cid_state2 = true;
											}
										}
									}
									else if ( ( new_aicidi.match_flag & 0x02 ) && ( new_aicidi.match_flag & 0x01 ) )
									{
										if ( lstrcmpW( mdi->caller_id, new_aicidi.caller_id ) == 0 )
										{
											update_cid_state2 = true;
										}
									}
									else if ( !( new_aicidi.match_flag & 0x02 ) && ( new_aicidi.match_flag & 0x01 ) )
									{
										if ( lstrcmpiW( mdi->caller_id, new_aicidi.caller_id ) == 0 )
										{
											update_cid_state2 = true;
										}
									}
									else if ( ( new_aicidi.match_flag & 0x02 ) && !( new_aicidi.match_flag & 0x01 ) )
									{
										if ( _StrStrW( mdi->caller_id, new_aicidi.caller_id ) != NULL )
										{
											update_cid_state2 = true;
										}
									}
									else if ( !( new_aicidi.match_flag & 0x02 ) && !( new_aicidi.match_flag & 0x01 ) )
									{
										if ( _StrStrIW( mdi->caller_id, new_aicidi.caller_id ) != NULL )
										{
											update_cid_state2 = true;
										}
									}

									if ( update_cid_state1 && update_cid_state2 )
									{
										keep_active = true;	// Skip updating values that haven't changed.
									}
									else if ( update_cid_state1 )
									{
										if ( aicidui->list_type == LIST_TYPE_ALLOW )
										{
											--( mdi->allow_cid_match_count );

											if ( mdi->allow_cid_match_count == 0 )
											{
												old_aicidi->active = false;
											}
										}
										else
										{
											--( mdi->ignore_cid_match_count );

											if ( mdi->ignore_cid_match_count == 0 )
											{
												old_aicidi->active = false;
											}
										}
									}
									else if ( update_cid_state2 )
									{
										if ( aicidui->list_type == LIST_TYPE_ALLOW )
										{
											++( mdi->allow_cid_match_count );

											if ( mdi->allow_cid_match_count > 0 )
											{
												old_aicidi->active = true;
											}
										}
										else
										{
											++( mdi->ignore_cid_match_count );

											if ( mdi->ignore_cid_match_count > 0 )
											{
												old_aicidi->active = true;
											}
										}
									}

									update_cid_state1 = update_cid_state2 = false;
								}

								di_node = di_node->next;
							}

							node = node->next;
						}

						if ( old_regex_match_data != NULL ) { _pcre2_match_data_free_16( old_regex_match_data ); }
						if ( old_regex_code != NULL ) { _pcre2_code_free_16( old_regex_code ); }

						if ( new_regex_match_data != NULL ) { _pcre2_match_data_free_16( new_regex_match_data ); }
						if ( new_regex_code != NULL ) { _pcre2_code_free_16( new_regex_code ); }

						if ( keep_active )
						{
							old_aicidi->active = true;
						}

						// Find the old icidi and remove it.
						itr = dllrbt_find( list, ( void * )old_aicidi, false );
						dllrbt_remove( list, itr );	// Remove the node from the tree. The tree will rebalance itself.

						old_aicidi->match_flag = new_aicidi.match_flag;

						if ( aicidui->caller_id != NULL )
						{
							GlobalFree( old_aicidi->caller_id );
							old_aicidi->caller_id = aicidui->caller_id;
						}

						old_aicidi->last_called.QuadPart = 0;
						GlobalFree( old_aicidi->w_last_called );
						old_aicidi->w_last_called = NULL;

						old_aicidi->count = 0;

						// Re-add the old icidi with updated values.
						dllrbt_insert( list, ( void * )old_aicidi, ( void * )old_aicidi );

						*changed = true;	// Makes us save the new allow/ignore_cid_list.

						MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_Updated_allowed_caller_ID_name : ST_Updated_ignored_caller_ID_name ) )
					}
					else
					{
						MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_cid_already_in_allow_cid_list : ST_cid_already_in_ignore_cid_list ) )

						GlobalFree( aicidui->caller_id );
					}
				}
			}
			else if ( aicidui->action == 4 )	// Update the recently called number's call count.
			{
				if ( aicidui->aicidi != NULL )
				{
					++( aicidui->aicidi->count );

					if ( aicidui->last_called.QuadPart > 0 )
					{
						aicidui->aicidi->last_called = aicidui->last_called;

						GlobalFree( aicidui->aicidi->w_last_called );

						aicidui->aicidi->w_last_called = FormatTimestamp( aicidui->aicidi->last_called );
					}

					*changed = true;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_Updated_allowed_caller_ID_name_s_call_count : ST_Updated_ignored_caller_ID_name_s_call_count ) )
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( aicidui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );

			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the allow/ignore list listview and remove the items that have changed state.
			bool skip_multi_cid_warning = false;	// Skip the multiple caller IDs warning if we've already displayed it.

			int last_selected = lvi.iItem;

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				if ( handle_all )
				{
					lvi.iItem = i;
				}
				else
				{
					last_selected = lvi.iItem = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				display_info *di = ( display_info * )lvi.lParam;
				if ( di != NULL )
				{
					unsigned int *cid_match_count_state = ( aicidui->list_type == LIST_TYPE_ALLOW ? &di->allow_cid_match_count : &di->ignore_cid_match_count );

					if ( *cid_match_count_state > 0 && aicidui->action == 1 )		// Remove from allow/ignore_cid_list.
					{
						if ( *cid_match_count_state <= 1 )
						{
							bool update_cid_state = false;

							int caller_id_length = lstrlenW( di->caller_id );

							// Find a keyword that matches the value we want to remove. Make sure that keyword only matches one set of values.
							node_type *node = dllrbt_get_head( list );
							while ( node != NULL )
							{
								// The keyword to remove.
								allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )node->val;
								if ( aicidi != NULL )
								{
									// Only remove active keyword matches.
									if ( aicidi->active )
									{
										if ( aicidi->match_flag & 0x04 )
										{
											int error_code;
											size_t error_offset;

											if ( g_use_regular_expressions )
											{
												pcre2_code *regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )aicidi->caller_id, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );
												if ( regex_code != NULL )
												{
													pcre2_match_data *regex_match_data = _pcre2_match_data_create_from_pattern_16( regex_code, NULL );
													if ( regex_match_data != NULL )
													{
														if ( _pcre2_match_16( regex_code, ( PCRE2_SPTR16 )di->caller_id, caller_id_length, 0, 0, regex_match_data, NULL ) >= 0 )
														{
															update_cid_state = true;
														}

														_pcre2_match_data_free_16( regex_match_data );
													}

													_pcre2_code_free_16( regex_code );
												}
											}
										}
										else if ( ( aicidi->match_flag & 0x02 ) && ( aicidi->match_flag & 0x01 ) )
										{
											if ( lstrcmpW( di->caller_id, aicidi->caller_id ) == 0 )
											{
												update_cid_state = true;
											}
										}
										else if ( !( aicidi->match_flag & 0x02 ) && ( aicidi->match_flag & 0x01 ) )
										{
											if ( lstrcmpiW( di->caller_id, aicidi->caller_id ) == 0 )
											{
												update_cid_state = true;
											}
										}
										else if ( ( aicidi->match_flag & 0x02 ) && !( aicidi->match_flag & 0x01 ) )
										{
											if ( _StrStrW( di->caller_id, aicidi->caller_id ) != NULL )
											{
												update_cid_state = true;
											}
										}
										else if ( !( aicidi->match_flag & 0x02 ) && !( aicidi->match_flag & 0x01 ) )
										{
											if ( _StrStrIW( di->caller_id, aicidi->caller_id ) != NULL )
											{
												update_cid_state = true;
											}
										}

										if ( update_cid_state )
										{
											update_caller_id_name_matches( ( void * )aicidi, aicidui->list_type, false );

											aicidi->state = 1;
											remove_state_changed = true;	// This will let us go through the allow/ignore cid list listview and free items with a state == 1.

											dllrbt_iterator *itr = dllrbt_find( list, ( void * )aicidi, false );
											dllrbt_remove( list, itr );	// Remove the node from the tree. The tree will rebalance itself.

											*changed = true;	// Causes us to save the allow/ignore_cid_list on shutdown.

											break;	// Break out of the keyword search.
										}
									}
								}

								node = node->next;
							}
						}
						else
						{
							if ( !skip_multi_cid_warning )
							{
								MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_remove_from_allow_caller_id_list : ST_remove_from_ignore_caller_id_list ) )
								skip_multi_cid_warning = true;
							}
						}
					}
					else if ( *cid_match_count_state == 0 && aicidui->action == 0 )	// Add to allow/ignore_cid_list.
					{
						// Zero init.
						allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_info ) );

						aicidi->caller_id = GlobalStrDupW( di->caller_id );

						aicidi->match_flag = aicidui->match_flag;

						// Try to insert the allow/ignore_cid_list info in the tree.
						if ( dllrbt_insert( list, ( void * )aicidi, ( void * )aicidi ) != DLLRBT_STATUS_OK )
						{
							free_allowignorecidinfo( &aicidi );

							MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_cid_already_in_allow_cid_list : ST_cid_already_in_ignore_cid_list ) )
						}
						else	// If it was able to be inserted into the tree, then update our display_info items and the allow/ignore cid list listview.
						{
							aicidi->count = 0;
							aicidi->state = 0;
							aicidi->active = false;

							update_caller_id_name_matches( ( void * )aicidi, aicidui->list_type, true );

							*changed = true;

							lvi.iItem = ( int )_SendMessageW( c_hWnd, LVM_GETITEMCOUNT, 0, 0 );
							lvi.lParam = ( LPARAM )aicidi;
							_SendMessageW( c_hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

							if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_Added_caller_ID_name_s__to_allow_caller_ID_name_list : ST_Added_caller_ID_name_s__to_ignore_caller_ID_name_list ) ) }
						}
					}
				}
			}

			// If we removed items in the call log listview from the allow/ignore_cid_list, then remove them from the allow/ignore cid list listview.
			if ( remove_state_changed )
			{
				int item_count = ( int )_SendMessageW( c_hWnd, LVM_GETITEMCOUNT, 0, 0 );

				skip_list_draw = true;
				_EnableWindow( c_hWnd, FALSE );

				// Start from the end and work backwards.
				for ( lvi.iItem = item_count - 1; lvi.iItem >= 0; --lvi.iItem )
				{
					_SendMessageW( c_hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

					allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )lvi.lParam;
					if ( aicidi != NULL && aicidi->state == 1 )
					{
						free_allowignorecidinfo( &aicidi );

						_SendMessageW( c_hWnd, LVM_DELETEITEM, lvi.iItem, 0 );
					}
				}

				skip_list_draw = false;
				_EnableWindow( c_hWnd, TRUE );

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ( aicidui->list_type == LIST_TYPE_ALLOW ? ST_Removed_caller_ID_name_s__from_allow_caller_ID_name_list : ST_Removed_caller_ID_name_s__from_ignore_caller_ID_name_list ) )
			}
		}

		GlobalFree( aicidui );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_contact_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_contact_list, false );

	wchar_t *val = NULL;
	int val_length = 0;

	contact_update_info *cui = ( contact_update_info * )pArguments;	// Freed at the end of this thread.
	
	if ( cui != NULL )
	{
		// Insert a row into our listview.
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
		lvi.iSubItem = 0;

		if ( cui->action == 0 ) // Add a single item to the contact_list and contact list listview
		{
			contact_info *ci = cui->old_ci;

			if ( ci != NULL )
			{
				if ( dllrbt_insert( contact_list, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
				{
					free_contactinfo( &ci );

					MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_already_in_contact_list )
				}
				else
				{
					ci->w_home_phone_number = FormatPhoneNumberW( ci->home_phone_number );
					ci->w_cell_phone_number = FormatPhoneNumberW( ci->cell_phone_number );
					ci->w_office_phone_number = FormatPhoneNumberW( ci->office_phone_number );
					ci->w_other_phone_number = FormatPhoneNumberW( ci->other_phone_number );
					ci->w_work_phone_number = FormatPhoneNumberW( ci->work_phone_number );
					ci->w_fax_number = FormatPhoneNumberW( ci->fax_number );

					add_custom_caller_id_w( ci );

					contact_list_changed = true;

					lvi.iItem = ( int )_SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )ci;
					_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Added_contact )
				}
			}
		}
		else if ( cui->action == 1 )	// Remove from contact_list and contact list listview.
		{
			removeinfo ri;
			ri.is_thread = false;
			ri.hWnd = g_hWnd_contact_list;
			remove_items( ( void * )&ri );
		}
		else if ( cui->action == 2 )	// Add all items in the contact_list to the contact list listview.
		{
			// Insert a row into our listview.
			node_type *node = dllrbt_get_head( contact_list );
			while ( node != NULL )
			{
				contact_info *ci = ( contact_info * )node->val;
				if ( ci != NULL )
				{
					lvi.iItem = ( int )_SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )ci;	// lParam = our contact_info structure from the connection thread.
					_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
				}
				
				node = node->next;
			}

			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Loaded_contact_list )
		}
		else if ( cui->action == 3 )	// Update a contact.
		{
			contact_info *ci = cui->old_ci;

			if ( ci != NULL )
			{
				contact_info *uci = cui->new_ci;	// Free uci when done.

				if ( uci != NULL )
				{
					for ( char i = 0, j = 0; i < 16; ++i )
					{
						switch ( i )
						{
							case 0:
							case 4:
							case 6:
							case 10:
							case 11:
							case 15:
							{
								if ( uci->w_contact_info_phone_numbers[ j ] != NULL )
								{
									remove_custom_caller_id_w( ci );

									GlobalFree( ci->w_contact_info_phone_numbers[ j ] );
									ci->w_contact_info_phone_numbers[ j ] = uci->w_contact_info_phone_numbers[ j ];

									GlobalFree( ci->w_contact_info_values[ i ] );
									ci->w_contact_info_values[ i ] = FormatPhoneNumberW( ci->w_contact_info_phone_numbers[ j ] );

									add_custom_caller_id_w( ci );
								}
								
								++j;
							}
							break;

							default:
							{
								if ( uci->w_contact_info_values[ i ] != NULL )
								{
									GlobalFree( ci->w_contact_info_values[ i ] );
									ci->w_contact_info_values[ i ] = uci->w_contact_info_values[ i ];
								}
							}
							break;
						}
					}

					if ( uci->picture_path != NULL || cui->remove_picture )
					{
						GlobalFree( ci->picture_path );
						ci->picture_path = uci->picture_path;
					}

					ci->rti = uci->rti;

					GlobalFree( uci );

					contact_list_changed = true;

					MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Updated_contact_information )
				}
			}
		}

		GlobalFree( cui );
	}

	Processing_Window( g_hWnd_contact_list, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_call_log( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	wchar_t range_number[ 32 ];

	display_info *di = ( display_info * )pArguments;

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Received_incoming_caller_ID_information )

	if ( di != NULL )
	{
		// Each number in our call log contains a linked list. Find the number if it exists and add it to the linked list.

		// Create the node to insert into a linked list.
		DoublyLinkedList *di_node = DLL_CreateNode( ( void * )di );

		// See if our tree has the phone number to add the node to.
		DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->phone_number, true );
		if ( dll == NULL )
		{
			// If no phone number exits, insert the node into the tree.
			if ( dllrbt_insert( call_log, ( void * )di->phone_number, ( void * )di_node ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( di_node );	// This shouldn't happen.
			}
			else
			{
				call_log_changed = true;
			}
		}
		else	// If a phone number exits, insert the node into the linked list.
		{
			DLL_AddNode( &dll, di_node, -1 );	// Insert at the end of the doubly linked list.

			call_log_changed = true;
		}

		// Search our lists.
		// The allow lists have precedence over the ignore lists.
		// The phone number list has precedence over the caller ID name list.
		unsigned char update_type = 0;	// 0 = None, 1 = allow_list, 2 = allow_cid_list, 3 = ignore_list, 4 = ignore_cid_list.

		// Search the allow_list for a match.
		allow_ignore_info *a_aii = ( allow_ignore_info * )dllrbt_find( allow_list, ( void * )di->phone_number, true );

		// Try searching the range list.
		if ( a_aii == NULL )
		{
			_memzero( range_number, 32 * sizeof( wchar_t ) );

			int range_index = lstrlenW( di->phone_number );
			range_index = ( range_index > 0 ? range_index - 1 : 0 );

			if ( RangeSearch( &allow_range_list[ range_index ], di->phone_number, range_number ) )
			{
				a_aii = ( allow_ignore_info * )dllrbt_find( allow_list, ( void * )range_number, true );
			}
		}

		if ( a_aii != NULL )
		{
			update_type = 1;

			di->allow_phone_number = true;	// Show Yes in the Allow Phone Number column.
		}

		// Search for the first allow caller ID list match. di->allow_cid_match_count will be updated here for all keyword matches.
		allow_ignore_cid_info *a_aicidi = find_caller_id_name_match( di, allow_cid_list, LIST_TYPE_ALLOW );
		if ( a_aii == NULL && a_aicidi != NULL )
		{
			if ( update_type == 0 )
			{
				update_type = 2;
			}
		}

		// Search the ignore_list for a match.
		allow_ignore_info *i_aii = ( allow_ignore_info * )dllrbt_find( ignore_list, ( void * )di->phone_number, true );

		// Try searching the range list.
		if ( i_aii == NULL )
		{
			_memzero( range_number, 32 * sizeof( wchar_t ) );

			int range_index = lstrlenW( di->phone_number );
			range_index = ( range_index > 0 ? range_index - 1 : 0 );

			if ( RangeSearch( &ignore_range_list[ range_index ], di->phone_number, range_number ) )
			{
				i_aii = ( allow_ignore_info * )dllrbt_find( ignore_list, ( void * )range_number, true );
			}
		}

		if ( i_aii != NULL )
		{
			if ( update_type == 0 )
			{
				IgnoreIncomingCall( di->incoming_call );

				di->ignored = true;	// The incoming call was ignored (display the call log text in red).

				di->process_incoming = false;	// Don't display the Ignore Incoming Call menu item.

				update_type = 3;
			}

			di->ignore_phone_number = true;	// Show Yes in the Ignore Phone Number column.
		}

		// Search for the first ignore caller ID list match. di->ignore_cid_match_count will be updated here for all keyword matches.
		allow_ignore_cid_info *i_aicidi = find_caller_id_name_match( di, ignore_cid_list, LIST_TYPE_IGNORE );
		if ( i_aii == NULL && i_aicidi != NULL )
		{
			if ( update_type == 0 )
			{
				IgnoreIncomingCall( di->incoming_call );

				di->ignored = true;	// The incoming call was ignored (display the call log text in red).

				di->process_incoming = false;	// Don't display the Ignore Incoming Call menu item.

				update_type = 4;
			}
		}

		switch ( update_type )
		{
			case 1:	// Allow List
			{
				allow_ignore_update_info *aiui = ( allow_ignore_update_info * )GlobalAlloc( GMEM_FIXED, sizeof( allow_ignore_update_info ) );
				aiui->last_called.QuadPart = di->time.QuadPart;
				aiui->aii = a_aii;
				aiui->phone_number = NULL;
				aiui->action = 4;	// Update the call count and last called time.
				aiui->list_type = LIST_TYPE_ALLOW;	// Allow list.
				aiui->hWnd = g_hWnd_allow_list;

				// aiui is freed in the update_allow_ignore_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_list, ( void * )aiui, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( aiui );
				}
			}
			break;

			case 2:	// Allow Caller ID List
			{
				allow_ignore_cid_update_info *aicidui = ( allow_ignore_cid_update_info * )GlobalAlloc( GMEM_FIXED, sizeof( allow_ignore_cid_update_info ) );
				aicidui->last_called.QuadPart = di->time.QuadPart;
				aicidui->aicidi = a_aicidi;
				aicidui->caller_id = NULL;
				aicidui->action = 4;	// Update the call count and last called time.
				aicidui->list_type = LIST_TYPE_ALLOW;	// Allow list.
				aicidui->hWnd = g_hWnd_allow_cid_list;
				aicidui->match_flag = 0x00;

				// aicidui is freed in the update_allow_ignore_cid_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_cid_list, ( void * )aicidui, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( aicidui );
				}
			}
			break;

			case 3:	// Ignore List
			{
				allow_ignore_update_info *aiui = ( allow_ignore_update_info * )GlobalAlloc( GMEM_FIXED, sizeof( allow_ignore_update_info ) );
				aiui->last_called.QuadPart = di->time.QuadPart;
				aiui->aii = i_aii;
				aiui->phone_number = NULL;
				aiui->action = 4;	// Update the call count and last called time.
				aiui->list_type = LIST_TYPE_IGNORE;	// Ignore list.
				aiui->hWnd = g_hWnd_ignore_list;

				// aiui is freed in the update_allow_ignore_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_list, ( void * )aiui, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( aiui );
				}
			}
			break;

			case 4: // Ignore Caller ID List
			{
				allow_ignore_cid_update_info *aicidui = ( allow_ignore_cid_update_info * )GlobalAlloc( GMEM_FIXED, sizeof( allow_ignore_cid_update_info ) );
				aicidui->last_called.QuadPart = di->time.QuadPart;
				aicidui->aicidi = i_aicidi;
				aicidui->caller_id = NULL;
				aicidui->action = 4;	// Update the call count and last called time.
				aicidui->list_type = LIST_TYPE_IGNORE;	// Ignore list.
				aicidui->hWnd = g_hWnd_ignore_cid_list;
				aicidui->match_flag = 0x00;

				// aicidui is freed in the update_allow_ignore_cid_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_cid_list, ( void * )aicidui, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( aicidui );
				}
			}
			break;
		}

		// If the incoming number matches a contact, then update the caller ID value.
		di->custom_caller_id = get_custom_caller_id_w( di->phone_number, &di->ci );

		di->w_phone_number = FormatPhoneNumberW( di->phone_number );

		if ( di->time.QuadPart > 0 )
		{
			di->w_time = FormatTimestamp( di->time );
		}

		_SendNotifyMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 0 ), ( LPARAM )di );	// Add entry to listview and show popup.
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN create_call_log_csv_file( void *file_path )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_call_log, false );

	if ( file_path != NULL )
	{
		save_call_log_csv_file( ( wchar_t * )file_path );

		GlobalFree( file_path );
	}

	Processing_Window( g_hWnd_call_log, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN copy_items( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	copyinfo *c_i = ( copyinfo * )pArguments;

	HWND hWnd = c_i->hWnd;

	unsigned column_type = 0;

	if		( hWnd == g_hWnd_allow_list )		{ column_type = 0; }
	else if ( hWnd == g_hWnd_allow_cid_list )	{ column_type = 1; }
	else if ( hWnd == g_hWnd_call_log )			{ column_type = 2; }
	else if ( hWnd == g_hWnd_contact_list )		{ column_type = 3; }
	else if ( hWnd == g_hWnd_ignore_list )		{ column_type = 4; }
	else if ( hWnd == g_hWnd_ignore_cid_list )	{ column_type = 5; }

	Processing_Window( hWnd, false );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;
	lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

	int item_count = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );
	
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

	int arr[ 17 ];

	// Set to however many columns we'll copy.
	int column_start = 0;
	int column_end = 1;

	if ( c_i->column == MENU_COPY_SEL )
	{
		switch ( column_type )
		{
			case 0: { column_end = g_total_columns1; } break;
			case 1: { column_end = g_total_columns2; } break;
			case 2: { column_end = g_total_columns3; } break;
			case 3: { column_end = g_total_columns4; } break;
			case 4: { column_end = g_total_columns5; } break;
			case 5: { column_end = g_total_columns6; } break;
		}

		_SendMessageW( hWnd, LVM_GETCOLUMNORDERARRAY, column_end, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		switch ( column_type )
		{
			case 0: { OffsetVirtualIndices( arr, allow_list_columns, NUM_COLUMNS1, g_total_columns1 ); } break;
			case 1: { OffsetVirtualIndices( arr, allow_cid_list_columns, NUM_COLUMNS2, g_total_columns2 ); } break;
			case 2: { OffsetVirtualIndices( arr, call_log_columns, NUM_COLUMNS3, g_total_columns3 ); } break;
			case 3: { OffsetVirtualIndices( arr, contact_list_columns, NUM_COLUMNS4, g_total_columns4 ); } break;
			case 4: { OffsetVirtualIndices( arr, ignore_list_columns, NUM_COLUMNS5, g_total_columns5 ); } break;
			case 5: { OffsetVirtualIndices( arr, ignore_cid_list_columns, NUM_COLUMNS6, g_total_columns6 ); } break;
		}
	}
	else
	{
		switch ( c_i->column )
		{
			case MENU_COPY_SEL_COL1:
			case MENU_COPY_SEL_COL21:
			case MENU_COPY_SEL_COL31:
			case MENU_COPY_SEL_COL41: { arr[ 0 ] = 1; } break;

			case MENU_COPY_SEL_COL2:
			case MENU_COPY_SEL_COL22:
			case MENU_COPY_SEL_COL32:
			case MENU_COPY_SEL_COL42:{ arr[ 0 ] = 2; } break;

			case MENU_COPY_SEL_COL3:
			case MENU_COPY_SEL_COL23:
			case MENU_COPY_SEL_COL33:
			case MENU_COPY_SEL_COL43: { arr[ 0 ] = 3; } break;

			case MENU_COPY_SEL_COL4:
			case MENU_COPY_SEL_COL24:
			case MENU_COPY_SEL_COL44: { arr[ 0 ] = 4; } break;

			case MENU_COPY_SEL_COL5:
			case MENU_COPY_SEL_COL25:
			case MENU_COPY_SEL_COL45: { arr[ 0 ] = 5; } break;

			case MENU_COPY_SEL_COL6:
			case MENU_COPY_SEL_COL26:
			case MENU_COPY_SEL_COL46: { arr[ 0 ] = 6; } break;

			case MENU_COPY_SEL_COL7:
			case MENU_COPY_SEL_COL27: { arr[ 0 ] = 7; } break;

			case MENU_COPY_SEL_COL28: { arr[ 0 ] = 8; } break;
			case MENU_COPY_SEL_COL29: { arr[ 0 ] = 9; } break;
			case MENU_COPY_SEL_COL210: { arr[ 0 ] = 10; } break;
			case MENU_COPY_SEL_COL211: { arr[ 0 ] = 11; } break;
			case MENU_COPY_SEL_COL212: { arr[ 0 ] = 12; } break;
			case MENU_COPY_SEL_COL213: { arr[ 0 ] = 13; } break;
			case MENU_COPY_SEL_COL214: { arr[ 0 ] = 14; } break;
			case MENU_COPY_SEL_COL215: { arr[ 0 ] = 15; } break;
			case MENU_COPY_SEL_COL216: { arr[ 0 ] = 16; } break;
		}
	}

	wchar_t tbuf[ 512 ];
	wchar_t *copy_string = NULL;
	bool add_newline = false;
	bool add_tab = false;

	// Go through each item, and copy their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		if ( copy_all )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

		display_info *di = NULL;
		contact_info *ci = NULL;
		allow_ignore_info *aii = NULL;
		allow_ignore_cid_info *aicidi = NULL;

		switch ( column_type )
		{
			case 0:
			case 4: { aii = ( allow_ignore_info * )lvi.lParam; } break;
			case 1:
			case 5: { aicidi = ( allow_ignore_cid_info * )lvi.lParam; } break;
			case 2: { di = ( display_info * )lvi.lParam; } break;
			case 3: { ci = ( contact_info * )lvi.lParam; } break;
		}

		add_newline = add_tab = false;

		for ( int j = column_start; j < column_end; ++j )
		{
			copy_string = NULL;

			int index;
			if ( arr[ j ] > 0 )
			{
				index = arr[ j ] - 1;

				if ( column_type == 2 )
				{
					switch ( index )
					{
						case 0: { copy_string = ( di->allow_cid_match_count > 0 ? ST_Yes : ST_No ); } break;
						case 1: { copy_string = ( di->allow_phone_number ? ST_Yes : ST_No ); } break;
						case 2:
						{
							if ( di->custom_caller_id != NULL )
							{
								copy_string = tbuf;	// Reset the buffer pointer.

								__snwprintf( copy_string, 512, L"%s (%s)", di->custom_caller_id, di->caller_id );
							}
							else
							{
								copy_string = di->caller_id;
							}
						}
						break;
						case 3: { copy_string = di->w_time; } break;
						case 4: { copy_string = ( di->ignore_cid_match_count > 0 ? ST_Yes : ST_No ); } break;
						case 5: { copy_string = ( di->ignore_phone_number ? ST_Yes : ST_No ); } break;
						case 6: { copy_string = di->w_phone_number; } break;
					}
				}
				else if ( column_type == 3 )
				{
					copy_string = ci->w_contact_info_values[ index ];
				}
				else if ( column_type == 0 || column_type == 4 )
				{
					switch ( index )
					{
						case 0: { copy_string = aii->w_last_called; } break;
						case 1: { copy_string = aii->w_phone_number; } break;
						case 2:
						{
							copy_string = tbuf;	// Reset the buffer pointer.

							__snwprintf( copy_string, 11, L"%lu", aii->count );
						}
						break;
					}
				}
				else if ( column_type == 1 || column_type == 5 )
				{
					switch ( index )
					{
						case 0: { copy_string = aicidi->caller_id; } break;
						case 1: { copy_string = aicidi->w_last_called; } break;
						case 2: { copy_string = ( ( aicidi->match_flag & 0x02 ) ? ST_Yes : ST_No ); } break;
						case 3: { copy_string = ( ( aicidi->match_flag & 0x01 ) ? ST_Yes : ST_No ); } break;
						case 4: { copy_string = ( ( aicidi->match_flag & 0x04 ) ? ST_Yes : ST_No ); } break;
						case 5:
						{
							copy_string = tbuf;	// Reset the buffer pointer.

							__snwprintf( copy_string, 11, L"%lu", aicidi->count );
						}
						break;
					}
				}
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

	if ( _OpenClipboard( hWnd ) )
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
	GlobalFree( c_i );

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN search_list( void *pArguments )
{
	SEARCH_INFO *si = ( SEARCH_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_call_log, false );

	if ( si != NULL )
	{
		if ( si->text != NULL )
		{
			LVITEM lvi, new_lvi;

			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM | LVIF_STATE;
			lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;

			_memzero( &new_lvi, sizeof( LVITEM ) );
			new_lvi.mask = LVIF_STATE;
			new_lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
			new_lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;

			int item_count = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );

			int current_item_index;

			if ( si->search_all )
			{
				current_item_index = 0;
			}
			else
			{
				current_item_index = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED ) + 1;
			}

			// Go through each item, and delete the file.
			for ( int i = 0; i < item_count; ++i, ++current_item_index )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				if ( current_item_index >= item_count )
				{
					current_item_index = 0;
				}

				lvi.iItem = current_item_index;
				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				display_info *di = ( display_info * )lvi.lParam;

				if ( di != NULL )
				{
					bool found_match = false;

					wchar_t *text = ( si->type == 1 ? di->w_phone_number : di->caller_id );

					if ( si->search_flag == 0x04 )	// Regular expression search.
					{
						int error_code;
						size_t error_offset;

						if ( g_use_regular_expressions )
						{
							pcre2_code *regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )si->text, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );

							if ( regex_code != NULL )
							{
								pcre2_match_data *match = _pcre2_match_data_create_from_pattern_16( regex_code, NULL );

								if ( match != NULL )
								{
									if ( _pcre2_match_16( regex_code, ( PCRE2_SPTR16 )text, lstrlenW( text ), 0, 0, match, NULL ) >= 0 )
									{
										found_match = true;
									}

									_pcre2_match_data_free_16( match );
								}

								_pcre2_code_free_16( regex_code );
							}
						}
					}
					else
					{
						if ( si->search_flag == ( 0x01 | 0x02 ) )	// Match case and whole word.
						{
							if ( lstrcmpW( text, si->text ) == 0 )
							{
								found_match = true;
							}
						}
						else if ( si->search_flag == 0x02 )	// Match whole word.
						{
							if ( lstrcmpiW( text, si->text ) == 0 )
							{
								found_match = true;
							}
						}
						else if ( si->search_flag == 0x01 )	// Match case.
						{
							if ( _StrStrW( text, si->text ) != NULL )
							{
								found_match = true;
							}
						}
						else
						{
							if ( _StrStrIW( text, si->text ) != NULL )
							{
								found_match = true;
							}
						}
					}

					if ( found_match )
					{
						if ( !si->search_all )
						{
							new_lvi.state = 0;
							_SendMessageW( g_hWnd_call_log, LVM_SETITEMSTATE, -1, ( LPARAM )&new_lvi );
						}

						new_lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
						_SendMessageW( g_hWnd_call_log, LVM_SETITEMSTATE, current_item_index, ( LPARAM )&new_lvi );

						if ( !si->search_all )
						{
							_SendMessageW( g_hWnd_call_log, LVM_ENSUREVISIBLE, current_item_index, FALSE );

							break;
						}
					}
					else
					{
						if ( lvi.state & LVIS_SELECTED )
						{
							new_lvi.state = 0;
							_SendMessageW( g_hWnd_call_log, LVM_SETITEMSTATE, current_item_index, ( LPARAM )&new_lvi );
						}
					}
				}
			}

			GlobalFree( si->text );
		}

		GlobalFree( si );
	}

	_SendMessageW( g_hWnd_search, WM_PROPAGATE, 1, 0 );

	Processing_Window( g_hWnd_call_log, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}
