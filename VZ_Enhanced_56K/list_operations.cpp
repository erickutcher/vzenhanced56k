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
#include "list_operations.h"
#include "file_operations.h"
#include "utilities.h"
#include "message_log_utilities.h"
#include "menus.h"
#include "string_tables.h"

#include "telephony.h"

bool skip_log_draw = false;				// Prevents WM_DRAWITEM from accessing listview items while we're removing them.
bool skip_contact_draw = false;
bool skip_ignore_draw = false;
bool skip_ignore_cid_draw = false;

void update_phone_number_matches( char *phone_number, unsigned char info_type, bool in_range, RANGE **range, bool add_value )
{
	if ( phone_number == NULL || ( info_type != 0 && info_type != 1 ) )
	{
		return;
	}

	if ( !in_range )	// Non-range phone number.
	{
		// Update each displayinfo item.
		DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )phone_number, true );
		if ( dll != NULL )
		{
			DoublyLinkedList *di_node = dll;
			while ( di_node != NULL )
			{
				displayinfo *mdi = ( displayinfo * )di_node->data;
				if ( mdi != NULL )
				{
					if ( info_type == 0 )	// Handle ignore phone number info.
					{
						if ( mdi->ignore_phone_number != add_value )
						{
							mdi->ignore_phone_number = add_value;
							GlobalFree( mdi->w_ignore_phone_number );
							mdi->w_ignore_phone_number = GlobalStrDupW( ( add_value ? ST_Yes : ST_No ) );
						}
					}
				}

				di_node = di_node->next;
			}
		}
	}
	else	// See if the value we're adding is a range (has wildcard values in it).
	{
		char range_number[ 32 ];	// Dummy value.

		// Update each displayinfo item.
		node_type *node = dllrbt_get_head( call_log );
		while ( node != NULL )
		{
			DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
			while ( di_node != NULL )
			{
				displayinfo *mdi = ( displayinfo * )di_node->data;
				if ( mdi != NULL )
				{
					if ( info_type == 0 )	// Handle ignore phone number info.
					{
						// Process display values that will be ignored/no longer ignored.
						if ( mdi->ignore_phone_number != add_value )
						{
							// Handle values that are no longer ignored.
							if ( !add_value )
							{
								// First, see if the display value falls within another range.
								if ( range != NULL && RangeSearch( range, mdi->ci.call_from, range_number ) )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Next, see if the display value exists as a non-range value.
								if ( dllrbt_find( ignore_list, ( void * )mdi->ci.call_from, false ) != NULL )
								{
									// If it does, then we'll skip the display value.
									break;
								}
							}

							// Finally, see if the display item falls within the range.
							if ( RangeCompare( phone_number, mdi->ci.call_from ) )
							{
								mdi->ignore_phone_number = add_value;
								GlobalFree( mdi->w_ignore_phone_number );
								mdi->w_ignore_phone_number = GlobalStrDupW( ( add_value ? ST_Yes : ST_No ) );
							}
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
void update_caller_id_name_matches( void *cidi, unsigned char info_type, bool add_value )
{
	if ( cidi == NULL )
	{
		return;
	}

	char *caller_id = NULL;
	bool match_case = true;
	bool match_whole_word = true;
	bool *active = NULL;

	if ( info_type == 0 )	// Handle ignore cid info.
	{
		caller_id = ( ( ignorecidinfo * )cidi )->c_caller_id;
		match_case = ( ( ignorecidinfo * )cidi )->match_case;
		match_whole_word = ( ( ignorecidinfo * )cidi )->match_whole_word;
		active = &( ( ( ignorecidinfo * )cidi )->active );
	}
	else
	{
		return;
	}

	bool update_cid_state = false;

	// Update each displayinfo item to indicate that it is now ignored.
	node_type *node = dllrbt_get_head( call_log );
	while ( node != NULL )
	{
		DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
		while ( di_node != NULL )
		{
			displayinfo *mdi = ( displayinfo * )di_node->data;
			if ( mdi != NULL )
			{
				if ( match_case && match_whole_word )
				{
					if ( lstrcmpA( mdi->ci.caller_id, caller_id ) == 0 )
					{
						update_cid_state = true;
					}
				}
				else if ( !match_case && match_whole_word )
				{
					if ( lstrcmpiA( mdi->ci.caller_id, caller_id ) == 0 )
					{
						update_cid_state = true;
					}
				}
				else if ( match_case && !match_whole_word )
				{
					if ( _StrStrA( mdi->ci.caller_id, caller_id ) != NULL )
					{
						update_cid_state = true;
					}
				}
				else if ( !match_case && !match_whole_word )
				{
					if ( _StrStrIA( mdi->ci.caller_id, caller_id ) != NULL )
					{
						update_cid_state = true;
					}
				}

				if ( update_cid_state )
				{
					if ( add_value )
					{
						*active = true;

						if ( info_type == 0 )
						{
							++( mdi->ignore_cid_match_count );
							GlobalFree( mdi->w_ignore_caller_id );
							mdi->w_ignore_caller_id = GlobalStrDupW( ST_Yes );
						}
					}
					else
					{
						if ( info_type == 0 )
						{
							--( mdi->ignore_cid_match_count );

							if ( mdi->ignore_cid_match_count == 0 )
							{
								*active = false;

								GlobalFree( mdi->w_ignore_caller_id );
								mdi->w_ignore_caller_id = GlobalStrDupW( ST_No );
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
		hWnd = ( iei->file_type == IE_CALL_LOG_HISTORY ? g_hWnd_call_log :
			   ( iei->file_type == IE_CONTACT_LIST ? g_hWnd_contact_list :
			   ( iei->file_type == IE_IGNORE_CID_LIST ? g_hWnd_ignore_cid_list : g_hWnd_ignore_list ) ) );
	}

	Processing_Window( hWnd, false );

	if ( iei != NULL )
	{
		if ( iei->file_paths != NULL )
		{
			switch ( iei->file_type )
			{
				case IE_IGNORE_PN_LIST:
				{
					save_ignore_list( iei->file_paths );
				}
				break;

				case IE_IGNORE_CID_LIST:
				{
					save_ignore_cid_list( iei->file_paths );
				}
				break;

				case IE_CONTACT_LIST:
				{
					save_contact_list( iei->file_paths );
				}
				break;

				case IE_CALL_LOG_HISTORY:
				{
					save_call_log_history( iei->file_paths );
				}
				break;
			}

			GlobalFree( iei->file_paths );

			MESSAGE_LOG_OUTPUT( ML_NOTICE, ( iei->file_type == IE_CALL_LOG_HISTORY ? ST_Exported_call_log_history :
										   ( iei->file_type == IE_CONTACT_LIST ? ST_Exported_contact_list :
										   ( iei->file_type == IE_IGNORE_CID_LIST ? ST_Exported_ignore_caller_ID_name_list : ST_Exported_ignore_phone_number_list ) ) ) )
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
		hWnd = ( iei->file_type == IE_CALL_LOG_HISTORY || iei->file_type == LOAD_CALL_LOG_HISTORY ? g_hWnd_call_log :
			   ( iei->file_type == IE_CONTACT_LIST ? g_hWnd_contact_list :
			   ( iei->file_type == IE_IGNORE_CID_LIST ? g_hWnd_ignore_cid_list : g_hWnd_ignore_list ) ) );
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
						case IE_IGNORE_PN_LIST:
						{
							dllrbt_tree *new_list = dllrbt_create( dllrbt_compare_a );

							status = read_ignore_list( file_path, new_list );

							node_type *node = dllrbt_get_head( new_list );
							while ( node != NULL )
							{
								ignoreinfo *ii = ( ignoreinfo * )node->val;
								if ( ii != NULL )
								{
									// Attempt to insert the new item into the global list.
									if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) == DLLRBT_STATUS_OK )
									{
										update_phone_number_matches( ii->c_phone_number, 0, ( ( ii->c_phone_number != NULL && is_num( ii->c_phone_number ) == 1 ) ? true : false ), NULL, true );

										ignore_list_changed = true;

										lvi.iItem = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
										lvi.lParam = ( LPARAM )ii;
										_SendMessageW( hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
									}
									else	// Problem/duplicate
									{
										free_ignoreinfo( &ii );
									}
								}

								node = node->next;
							}

							// All values stored in the tree will either have been added to a global list, or freed.
							dllrbt_delete_recursively( new_list );
						}
						break;

						case IE_IGNORE_CID_LIST:
						{
							dllrbt_tree *new_list = dllrbt_create( dllrbt_icid_compare );

							status = read_ignore_cid_list( file_path, new_list );

							node_type *node = dllrbt_get_head( new_list );
							while ( node != NULL )
							{
								ignorecidinfo *icidi = ( ignorecidinfo * )node->val;
								if ( icidi != NULL )
								{
									// Attempt to insert the new item into the global list.
									if ( dllrbt_insert( ignore_cid_list, ( void * )icidi, ( void * )icidi ) == DLLRBT_STATUS_OK )
									{
										update_caller_id_name_matches( ( void * )icidi, 0, true );

										ignore_cid_list_changed = true;

										lvi.iItem = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
										lvi.lParam = ( LPARAM )icidi;
										_SendMessageW( hWnd, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
									}
									else	// Problem/duplicate
									{
										free_ignorecidinfo( &icidi );
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
								contactinfo *ci = ( contactinfo * )node->val;
								if ( ci != NULL )
								{
									// Attempt to insert the new item into the global list.
									if ( dllrbt_insert( contact_list, ( void * )ci, ( void * )ci ) == DLLRBT_STATUS_OK )
									{
										add_custom_caller_id( ci );

										contact_list_changed = true;

										lvi.iItem = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
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
				MESSAGE_LOG_OUTPUT( ML_NOTICE, ( iei->file_type == LOAD_CALL_LOG_HISTORY ? ST_Loaded_call_log_history :
											   ( iei->file_type == IE_CALL_LOG_HISTORY ? ST_Imported_call_log_history :
											   ( iei->file_type == IE_CONTACT_LIST ? ST_Imported_contact_list :
											   ( iei->file_type == IE_IGNORE_CID_LIST ? ST_Imported_ignore_caller_ID_name_list : ST_Imported_ignore_phone_number_list ) ) ) ) )
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
	removeinfo *ri = ( removeinfo * )pArguments;
	
	HWND hWnd = NULL;
	bool disable_critical_section = false;

	if ( ri != NULL )
	{
		disable_critical_section = ri->disable_critical_section;
		hWnd = ri->hWnd;

		GlobalFree( ri );
	}

	// This will block every other thread from entering until the first thread is complete.
	if ( !disable_critical_section )
	{
		EnterCriticalSection( &pe_cs );
	}

	in_worker_thread = true;
	
	bool *skip_draw = NULL;

	// Prevent the listviews from drawing while freeing lParam values.
	if ( hWnd == g_hWnd_call_log )
	{
		skip_draw = &skip_log_draw;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		skip_draw = &skip_contact_draw;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		skip_draw = &skip_ignore_draw;
	}
	else if ( hWnd == g_hWnd_ignore_cid_list )
	{
		skip_draw = &skip_ignore_cid_draw;
	}
	
	if ( skip_draw != NULL )
	{
		*skip_draw = true;
	}

	if ( !disable_critical_section )
	{
		Processing_Window( hWnd, false );
	}

	char range_number[ 32 ];	// Dummy value.

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	int item_count = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

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
			lvi.iItem = index_array[ sel_count - 1 - i ] = _SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
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
			displayinfo *di = ( displayinfo * )lvi.lParam;
			if ( di != NULL )
			{
				// Update each displayinfo item.
				dllrbt_iterator *itr = dllrbt_find( call_log, ( void * )di->ci.call_from, false );
				if ( itr != NULL )
				{
					// Head of the linked list.
					DoublyLinkedList *ll = ( DoublyLinkedList * )( ( node_type * )itr )->val;

					// Go through each linked list node and remove the one with di.

					DoublyLinkedList *current_node = ll;
					while ( current_node != NULL )
					{
						if ( ( displayinfo * )current_node->data == di )
						{
							DLL_RemoveNode( &ll, current_node );
							GlobalFree( current_node );

							if ( ll != NULL && ll->data != NULL )
							{
								// Reset the head in the tree.
								( ( node_type * )itr )->val = ( void * )ll;
								( ( node_type * )itr )->key = ( void * )( ( displayinfo * )ll->data )->ci.call_from;

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
			contactinfo *ci = ( contactinfo * )lvi.lParam;
			if ( ci != NULL )
			{
				dllrbt_iterator *itr = dllrbt_find( contact_list, ( void * )ci, false );
				if ( itr != NULL )
				{
					dllrbt_remove( contact_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					contact_list_changed = true;	// Causes us to save the contact_list on shutdown.
				}

				remove_custom_caller_id( ci );

				free_contactinfo( &ci );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Deleted_contact ) }
		}
		else if ( hWnd == g_hWnd_ignore_list )
		{
			ignoreinfo *ii = ( ignoreinfo * )lvi.lParam;
			if ( ii != NULL )
			{
				int range_index = lstrlenA( ii->c_phone_number );
				range_index = ( range_index > 0 ? range_index - 1 : 0 );

				// If the number we've removed is a range, then remove it from the range list.
				if ( is_num( ii->c_phone_number ) == 1 )
				{
					RangeRemove( &ignore_range_list[ range_index ], ii->c_phone_number );

					// Update each displayinfo item to indicate that it is no longer ignored.
					update_phone_number_matches( ii->c_phone_number, 0, true, &ignore_range_list[ range_index ], false );
				}
				else
				{
					// See if the value we remove falls within a range. If it doesn't, then set its new display values.
					if ( !RangeSearch( &ignore_range_list[ range_index ], ii->c_phone_number, range_number ) )
					{
						// Update each displayinfo item to indicate that it is no longer ignored.
						update_phone_number_matches( ii->c_phone_number, 0, false, NULL, false );
					}
				}

				// See if the ignore_list phone number exits. It should.
				dllrbt_iterator *itr = dllrbt_find( ignore_list, ( void * )ii->c_phone_number, false );
				if ( itr != NULL )
				{
					dllrbt_remove( ignore_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					ignore_list_changed = true;	// Causes us to save the ignore_list on shutdown.
				}

				free_ignoreinfo( &ii );
			}

			if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_phone_number_s__from_ignore_phone_number_list ) }
		}
		else if ( hWnd == g_hWnd_ignore_cid_list )
		{
			ignorecidinfo *icidi = ( ignorecidinfo * )lvi.lParam;
			if ( icidi != NULL )
			{
				update_caller_id_name_matches( ( void * )icidi, 0, false );

				// See if the ignore_cid_list value exits. It should.
				dllrbt_iterator *itr = dllrbt_find( ignore_cid_list, ( void * )icidi, false );
				if ( itr != NULL )
				{
					dllrbt_remove( ignore_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					ignore_cid_list_changed = true;	// Causes us to save the ignore_cid_list on shutdown.
				}

				free_ignorecidinfo( &icidi );
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

	if ( skip_draw != NULL )
	{
		*skip_draw = false;
	}

	if ( !disable_critical_section )
	{
		Processing_Window( hWnd, true );

		// Release the semaphore if we're killing the thread.
		if ( worker_semaphore != NULL )
		{
			ReleaseSemaphore( worker_semaphore, 1, NULL );
		}
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	if ( !disable_critical_section )
	{
		LeaveCriticalSection( &pe_cs );
	}

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_ignore_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	ignoreupdateinfo *iui = ( ignoreupdateinfo * )pArguments;
	if ( iui != NULL )
	{
		hWnd = iui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( iui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		if ( iui->hWnd == g_hWnd_ignore_list )
		{
			if ( iui->action == 0 )	// Add a single item to ignore_list and ignore list listview.
			{
				// Zero init.
				ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GPTR, sizeof( ignoreinfo ) );

				ii->c_phone_number = iui->phone_number;

				// Try to insert the ignore_list info in the tree.
				if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
				{
					free_ignoreinfo( &ii );

					MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_already_in_ignore_list )
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the ignore list listview.
				{
					ii->phone_number = FormatPhoneNumber( iui->phone_number );

					ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*ii->c_total_calls = '0';
					*( ii->c_total_calls + 1 ) = 0;	// Sanity

					int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

					ii->total_calls = val;

					ii->count = 0;
					ii->state = 0;

					// See if the value we're adding is a range (has wildcard values in it).
					if ( ii->c_phone_number != NULL && is_num( ii->c_phone_number ) == 1 )
					{
						int phone_number_length = lstrlenA( ii->c_phone_number );

						RangeAdd( &ignore_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], ii->c_phone_number, phone_number_length );

						// Update each displayinfo item to indicate that it is now ignored.
						update_phone_number_matches( ii->c_phone_number, 0, true, NULL, true );
					}
					else
					{
						// Update each displayinfo item to indicate that it is now ignored.
						update_phone_number_matches( ii->c_phone_number, 0, false, NULL, true );
					}

					ignore_list_changed = true;

					lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )ii;	// lParam = our contactinfo structure from the connection thread.
					_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Added_phone_number_to_ignore_phone_number_list )
				}
			}
			else if ( iui->action == 1 )	// Remove from ignore_list and ignore list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_ignore_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( iui->action == 2 )	// Add all items in the ignore_list to the ignore list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( ignore_list );
				while ( node != NULL )
				{
					ignoreinfo *ii = ( ignoreinfo * )node->val;
					if ( ii != NULL )
					{
						lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )ii;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Loaded_ignore_phone_number_list )
			}
			else if ( iui->action == 3 )	// Update the recently called number's call count.
			{
				if ( iui->ii != NULL )
				{
					++( iui->ii->count );

					GlobalFree( iui->ii->c_total_calls );

					iui->ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( iui->ii->c_total_calls, 11, "%lu", iui->ii->count );

					GlobalFree( iui->ii->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, iui->ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, iui->ii->c_total_calls, -1, val, val_length );

					iui->ii->total_calls = val;

					ignore_list_changed = true;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Updated_ignored_phone_number_s_call_count )
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( iui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the ignore list listview and remove the items that have changed state.
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
					last_selected = lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;
				if ( di != NULL )
				{
					if ( di->ignore_phone_number && iui->action == 1 )		// Remove from ignore_list.
					{
						char range_number[ 32 ];	// Dummy value.

						int range_index = lstrlenA( di->ci.call_from );
						range_index = ( range_index > 0 ? range_index - 1 : 0 );

						// First, see if the phone number is in our range list.
						if ( !RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) )
						{
							// If not, then see if the phone number is in our ignore_list.
							dllrbt_iterator *itr = dllrbt_find( ignore_list, ( void * )di->ci.call_from, false );
							if ( itr != NULL )
							{
								// Update each displayinfo item to indicate that it is no longer ignored.
								update_phone_number_matches( di->ci.call_from, 0, false, NULL, false );

								// If an ignore list listview item also shows up in the call log listview, then we'll remove it after this loop.
								ignoreinfo *ii = ( ignoreinfo * )( ( node_type * )itr )->val;
								if ( ii != NULL )
								{
									ii->state = 1;
								}
								remove_state_changed = true;	// This will let us go through the ignore list listview and free items with a state == 1.

								dllrbt_remove( ignore_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

								ignore_list_changed = true;	// Causes us to save the ignore_list on shutdown.
							}

							di->ignore_phone_number = false;
							GlobalFree( di->w_ignore_phone_number );
							di->w_ignore_phone_number = GlobalStrDupW( ST_No );
						}
						else
						{
							if ( !skip_range_warning )
							{
								MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_remove_from_ignore_phone_number_list )
								skip_range_warning = true;
							}
						}
					}
					else if ( !di->ignore_phone_number && iui->action == 0 )	// Add to ignore_list.
					{
						// Zero init.
						ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GPTR, sizeof( ignoreinfo ) );

						ii->c_phone_number = GlobalStrDupA( di->ci.call_from );

						if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
						{
							free_ignoreinfo( &ii );

							MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_already_in_ignore_list )
						}
						else	// Add to ignore list listview as well.
						{
							ii->phone_number = FormatPhoneNumber( di->ci.call_from );

							ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
							*ii->c_total_calls = '0';
							*( ii->c_total_calls + 1 ) = 0;	// Sanity

							int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
							wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
							MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

							ii->total_calls = val;

							ii->count = 0;

							ii->state = 0;

							// Update all nodes if it already exits.
							update_phone_number_matches( di->ci.call_from, 0, false, NULL, true );

							di->ignore_phone_number = true;
							GlobalFree( di->w_ignore_phone_number );
							di->w_ignore_phone_number = GlobalStrDupW( ST_Yes );

							/*	// Wildcard values shouldn't appear in the call log listview.
							// See if the value we're adding is a range (has wildcard values in it). Only allow 10 digit numbers.
							if ( ii->c_phone_number != NULL && lstrlenA( ii->c_phone_number ) == 10 && is_num( ii->c_phone_number ) == 1 )
							{
								AddRange( &range_list, ii->c_phone_number );
							}*/

							ignore_list_changed = true;

							lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
							lvi.lParam = ( LPARAM )ii;
							_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

							if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Added_phone_number_s__to_ignore_phone_number_list ) }
						}
					}
				}
			}

			// If we removed items in the call log listview from the ignore_list, then remove them from the ignore list listview.
			if ( remove_state_changed )
			{
				int item_count = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_ignore_draw = true;
				_EnableWindow( g_hWnd_ignore_list, FALSE );

				// Start from the end and work backwards.
				for ( lvi.iItem = item_count - 1; lvi.iItem >= 0; --lvi.iItem )
				{
					_SendMessageW( g_hWnd_ignore_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					ignoreinfo *ii = ( ignoreinfo * )lvi.lParam;
					if ( ii != NULL && ii->state == 1 )
					{
						free_ignoreinfo( &ii );

						_SendMessageW( g_hWnd_ignore_list, LVM_DELETEITEM, lvi.iItem, 0 );
					}
				}

				skip_ignore_draw = false;
				_EnableWindow( g_hWnd_ignore_list, TRUE );

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_phone_number_s__from_ignore_phone_number_list )
			}
		}

		GlobalFree( iui );
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

THREAD_RETURN update_ignore_cid_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )pArguments;
	if ( icidui != NULL )
	{
		hWnd = icidui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( icidui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		if ( icidui->hWnd == g_hWnd_ignore_cid_list )
		{
			if ( icidui->action == 0 )	// Add a single item to ignore_cid_list and ignore list listview.
			{
				// Zero init.
				ignorecidinfo *icidi = ( ignorecidinfo * )GlobalAlloc( GPTR, sizeof( ignorecidinfo ) );

				icidi->c_caller_id = icidui->caller_id;

				icidi->match_case = icidui->match_case;
				icidi->match_whole_word = icidui->match_whole_word;

				// Try to insert the ignore_cid_list info in the tree.
				if ( dllrbt_insert( ignore_cid_list, ( void * )icidi, ( void * )icidi ) != DLLRBT_STATUS_OK )
				{
					free_ignorecidinfo( &icidi );

					MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_cid_already_in_ignore_cid_list )
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the ignore cid list listview.
				{
					int val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, val, val_length );

					icidi->caller_id = val;

					icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*icidi->c_total_calls = '0';
					*( icidi->c_total_calls + 1 ) = 0;	// Sanity

					val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, val, val_length );

					icidi->total_calls = val;

					if ( !icidi->match_case )
					{
						icidi->c_match_case = GlobalStrDupA( "No" );
						icidi->w_match_case = GlobalStrDupW( ST_No );
					}
					else
					{
						icidi->c_match_case = GlobalStrDupA( "Yes" );
						icidi->w_match_case = GlobalStrDupW( ST_Yes );
					}

					if ( !icidi->match_whole_word )
					{
						icidi->c_match_whole_word = GlobalStrDupA( "No" );
						icidi->w_match_whole_word = GlobalStrDupW( ST_No );
					}
					else
					{
						icidi->c_match_whole_word = GlobalStrDupA( "Yes" );
						icidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
					}

					icidi->count = 0;
					icidi->state = 0;
					icidi->active = false;

					update_caller_id_name_matches( ( void * )icidi, 0, true );

					ignore_cid_list_changed = true;

					lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )icidi;
					_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Added_caller_ID_name_to_ignore_caller_ID_name_list )
				}
			}
			else if ( icidui->action == 1 )	// Remove from ignore_cid_list and ignore cid list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_ignore_cid_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( icidui->action == 2 )	// Add all items in the ignore_cid_list to the ignore cid list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( ignore_cid_list );
				while ( node != NULL )
				{
					ignorecidinfo *icidi = ( ignorecidinfo * )node->val;
					if ( icidi != NULL )
					{
						lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )icidi;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Loaded_ignore_caller_ID_name_list )
			}
			else if ( icidui->action == 3 )	// Update entry
			{
				ignorecidinfo *old_icidi = icidui->icidi;
				if ( old_icidi != NULL )
				{
					// This temporary structure is used to search our ignore_cid_list tree.
					ignorecidinfo *new_icidi = ( ignorecidinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidinfo ) );

					new_icidi->c_caller_id = old_icidi->c_caller_id;	// DO NOT FREE THIS VALUE.
					new_icidi->match_case = icidui->match_case;
					new_icidi->match_whole_word = icidui->match_whole_word;

					// See if our new entry already exists.
					dllrbt_iterator *itr = dllrbt_find( ignore_cid_list, ( void * )new_icidi, false );
					if ( itr == NULL )
					{
						bool keep_active = false;
						bool update_cid_state1 = false, update_cid_state2 = false;

						// Update each displayinfo item to indicate that it is ignored or no longer ignored.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;
								if ( mdi != NULL )
								{
									// Find our old matches.
									if ( old_icidi->match_case && old_icidi->match_whole_word )
									{
										if ( lstrcmpA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
										{
											update_cid_state1 = true;
										}
									}
									else if ( !old_icidi->match_case && old_icidi->match_whole_word )
									{
										if ( lstrcmpiA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
										{
											update_cid_state1 = true;
										}
									}
									else if ( old_icidi->match_case && !old_icidi->match_whole_word )
									{
										if ( _StrStrA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
										{
											update_cid_state1 = true;
										}
									}
									else if ( !old_icidi->match_case && !old_icidi->match_whole_word )
									{
										if ( _StrStrIA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
										{
											update_cid_state1 = true;
										}
									}

									// Find our new matches.
									if ( new_icidi->match_case && new_icidi->match_whole_word )
									{
										if ( lstrcmpA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
										{
											update_cid_state2 = true;
										}
									}
									else if ( !new_icidi->match_case && new_icidi->match_whole_word )
									{
										if ( lstrcmpiA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
										{
											update_cid_state2 = true;
										}
									}
									else if ( new_icidi->match_case && !new_icidi->match_whole_word )
									{
										if ( _StrStrA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
										{
											update_cid_state2 = true;
										}
									}
									else if ( !new_icidi->match_case && !new_icidi->match_whole_word )
									{
										if ( _StrStrIA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
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
										--( mdi->ignore_cid_match_count );

										if ( mdi->ignore_cid_match_count == 0 )
										{
											old_icidi->active = false;

											GlobalFree( mdi->w_ignore_caller_id );
											mdi->w_ignore_caller_id = GlobalStrDupW( ST_No );
										}
									}
									else if ( update_cid_state2 )
									{
										++( mdi->ignore_cid_match_count );

										if ( mdi->ignore_cid_match_count > 0 )
										{
											old_icidi->active = true;
										}

										GlobalFree( mdi->w_ignore_caller_id );
										mdi->w_ignore_caller_id = GlobalStrDupW( ST_Yes );
									}

									update_cid_state1 = update_cid_state2 = false;
								}

								di_node = di_node->next;
							}

							node = node->next;
						}

						if ( keep_active )
						{
							old_icidi->active = true;
						}

						// Find the old icidi and remove it.
						itr = dllrbt_find( ignore_cid_list, ( void * )old_icidi, false );
						dllrbt_remove( ignore_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

						old_icidi->match_case = new_icidi->match_case;
						old_icidi->match_whole_word = new_icidi->match_whole_word;

						// Re-add the old icidi with updated values.
						dllrbt_insert( ignore_cid_list, ( void * )old_icidi, ( void * )old_icidi );

						GlobalFree( old_icidi->c_match_case );
						GlobalFree( old_icidi->w_match_case );
						GlobalFree( old_icidi->c_match_whole_word );
						GlobalFree( old_icidi->w_match_whole_word );

						if ( !old_icidi->match_case )
						{
							old_icidi->c_match_case = GlobalStrDupA( "No" );
							old_icidi->w_match_case = GlobalStrDupW( ST_No );
						}
						else
						{
							old_icidi->c_match_case = GlobalStrDupA( "Yes" );
							old_icidi->w_match_case = GlobalStrDupW( ST_Yes );
						}

						if ( !old_icidi->match_whole_word )
						{
							old_icidi->c_match_whole_word = GlobalStrDupA( "No" );
							old_icidi->w_match_whole_word = GlobalStrDupW( ST_No );
						}
						else
						{
							old_icidi->c_match_whole_word = GlobalStrDupA( "Yes" );
							old_icidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
						}

						ignore_cid_list_changed = true;	// Makes us save the new ignore_cid_list.

						MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Updated_ignored_caller_ID_name )
					}
					else
					{
						// See if the value we're updating is different from the match above.
						if ( old_icidi != ( ignorecidinfo * )( ( node_type * )itr )->val )
						{
							MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_cid_already_in_ignore_cid_list )
						}
					}

					GlobalFree( new_icidi );	// We don't need this anymore.
				}
			}
			else if ( icidui->action == 4 )	// Update the recently called number's call count.
			{
				if ( icidui->icidi != NULL )
				{
					++( icidui->icidi->count );

					GlobalFree( icidui->icidi->c_total_calls );

					icidui->icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( icidui->icidi->c_total_calls, 11, "%lu", icidui->icidi->count );

					GlobalFree( icidui->icidi->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, icidui->icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, icidui->icidi->c_total_calls, -1, val, val_length );

					icidui->icidi->total_calls = val;

					ignore_cid_list_changed = true;
				}

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Updated_ignored_caller_ID_name_s_call_count )
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( icidui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the ignore list listview and remove the items that have changed state.
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
					last_selected = lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;
				if ( di != NULL )
				{
					if ( di->ignore_cid_match_count > 0 && icidui->action == 1 )		// Remove from ignore_cid_list.
					{
						if ( di->ignore_cid_match_count <= 1 )
						{
							bool update_cid_state = false;

							// Find a keyword that matches the value we want to remove. Make sure that keyword only matches one set of values.
							node_type *node = dllrbt_get_head( ignore_cid_list );
							while ( node != NULL )
							{
								// The keyword to remove.
								ignorecidinfo *icidi = ( ignorecidinfo * )node->val;
								if ( icidi != NULL )
								{
									// Only remove active keyword matches.
									if ( icidi->active )
									{
										if ( icidi->match_case && icidi->match_whole_word )
										{
											if ( lstrcmpA( di->ci.caller_id, icidi->c_caller_id ) == 0 )
											{
												update_cid_state = true;
											}
										}
										else if ( !icidi->match_case && icidi->match_whole_word )
										{
											if ( lstrcmpiA( di->ci.caller_id, icidi->c_caller_id ) == 0 )
											{
												update_cid_state = true;
											}
										}
										else if ( icidi->match_case && !icidi->match_whole_word )
										{
											if ( _StrStrA( di->ci.caller_id, icidi->c_caller_id ) != NULL )
											{
												update_cid_state = true;
											}
										}
										else if ( !icidi->match_case && !icidi->match_whole_word )
										{
											if ( _StrStrIA( di->ci.caller_id, icidi->c_caller_id ) != NULL )
											{
												update_cid_state = true;
											}
										}

										if ( update_cid_state )
										{
											update_caller_id_name_matches( ( void * )icidi, 0, false );

											icidi->state = 1;
											remove_state_changed = true;	// This will let us go through the ignore cid list listview and free items with a state == 1.

											dllrbt_iterator *itr = dllrbt_find( ignore_cid_list, ( void * )icidi, false );
											dllrbt_remove( ignore_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

											ignore_cid_list_changed = true;	// Causes us to save the ignore_cid_list on shutdown.

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
								MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_remove_from_ignore_caller_id_list )
								skip_multi_cid_warning = true;
							}
						}
					}
					else if ( di->ignore_cid_match_count == 0 && icidui->action == 0 )	// Add to ignore_cid_list.
					{
						// Zero init.
						ignorecidinfo *icidi = ( ignorecidinfo * )GlobalAlloc( GPTR, sizeof( ignorecidinfo ) );

						icidi->c_caller_id = GlobalStrDupA( di->ci.caller_id );

						icidi->match_case = icidui->match_case;
						icidi->match_whole_word = icidui->match_whole_word;

						// Try to insert the ignore_cid_list info in the tree.
						if ( dllrbt_insert( ignore_cid_list, ( void * )icidi, ( void * )icidi ) != DLLRBT_STATUS_OK )
						{
							free_ignorecidinfo( &icidi );

							MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_cid_already_in_ignore_cid_list )
						}
						else	// If it was able to be inserted into the tree, then update our displayinfo items and the ignore cid list listview.
						{
							int val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
							wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
							MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, val, val_length );

							icidi->caller_id = val;

							icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
							*icidi->c_total_calls = '0';
							*( icidi->c_total_calls + 1 ) = 0;	// Sanity

							val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
							val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
							MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, val, val_length );

							icidi->total_calls = val;

							if ( !icidi->match_case )
							{
								icidi->c_match_case = GlobalStrDupA( "No" );
								icidi->w_match_case = GlobalStrDupW( ST_No );
							}
							else
							{
								icidi->c_match_case = GlobalStrDupA( "Yes" );
								icidi->w_match_case = GlobalStrDupW( ST_Yes );
							}

							if ( !icidi->match_whole_word )
							{
								icidi->c_match_whole_word = GlobalStrDupA( "No" );
								icidi->w_match_whole_word = GlobalStrDupW( ST_No );
							}
							else
							{
								icidi->c_match_whole_word = GlobalStrDupA( "Yes" );
								icidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
							}

							icidi->count = 0;
							icidi->state = 0;
							icidi->active = false;

							update_caller_id_name_matches( ( void * )icidi, 0, true );

							ignore_cid_list_changed = true;

							lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );
							lvi.lParam = ( LPARAM )icidi;
							_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

							if ( i == 0 ) { MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Added_caller_ID_name_s__to_ignore_caller_ID_name_list ) }
						}
					}
				}
			}

			// If we removed items in the call log listview from the ignore_cid_list, then remove them from the ignore cid list listview.
			if ( remove_state_changed )
			{
				int item_count = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_ignore_cid_draw = true;
				_EnableWindow( g_hWnd_ignore_cid_list, FALSE );

				// Start from the end and work backwards.
				for ( lvi.iItem = item_count - 1; lvi.iItem >= 0; --lvi.iItem )
				{
					_SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					ignorecidinfo *icidi = ( ignorecidinfo * )lvi.lParam;
					if ( icidi != NULL && icidi->state == 1 )
					{
						free_ignorecidinfo( &icidi );

						_SendMessageW( g_hWnd_ignore_cid_list, LVM_DELETEITEM, lvi.iItem, 0 );
					}
				}

				skip_ignore_cid_draw = false;
				_EnableWindow( g_hWnd_ignore_cid_list, TRUE );

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_caller_ID_name_s__from_ignore_caller_ID_name_list )
			}
		}

		GlobalFree( icidui );
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

	contactupdateinfo *cui = ( contactupdateinfo * )pArguments;	// Freed at the end of this thread.
	
	if ( cui != NULL )
	{
		// Insert a row into our listview.
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
		lvi.iSubItem = 0;

		if ( cui->action == 0 ) // Add a single item to the contact_list and contact list listview
		{
			contactinfo *ci = cui->old_ci;

			if ( ci != NULL )
			{
				if ( dllrbt_insert( contact_list, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
				{
					free_contactinfo( &ci );

					MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_already_in_contact_list )
				}
				else
				{
					ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );
					ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );
					ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );
					ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );

					ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );
					ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );

					add_custom_caller_id( ci );

					contact_list_changed = true;

					lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )ci;
					_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Added_contact )
				}
			}
		}
		else if ( cui->action == 1 )	// Remove from contact_list and contact list listview.
		{
			removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
			ri->disable_critical_section = true;
			ri->hWnd = g_hWnd_contact_list;

			// ri will be freed in the remove_items thread.
			HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

			if ( hThread != NULL )
			{
				WaitForSingleObject( hThread, INFINITE );	// Wait for the remove_items thread to finish.
				CloseHandle( hThread );
			}
		}
		else if ( cui->action == 2 )	// Add all items in the contact_list to the contact list listview.
		{
			// Insert a row into our listview.
			node_type *node = dllrbt_get_head( contact_list );
			while ( node != NULL )
			{
				contactinfo *ci = ( contactinfo * )node->val;
				if ( ci != NULL )
				{
					lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )ci;	// lParam = our contactinfo structure from the connection thread.
					_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
				}
				
				node = node->next;
			}

			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Loaded_contact_list )
		}
		else if ( cui->action == 3 )	// Update a contact.
		{
			contactinfo *ci = cui->old_ci;

			if ( ci != NULL )
			{
				contactinfo *uci = cui->new_ci;	// Free uci when done.

				if ( uci != NULL )
				{
					if ( uci->contact.first_name != NULL )
					{
						GlobalFree( ci->contact.first_name );
						ci->contact.first_name = uci->contact.first_name;
						GlobalFree( ci->first_name );
						ci->first_name = uci->first_name;
					}

					if ( uci->contact.last_name != NULL )
					{
						GlobalFree( ci->contact.last_name );
						ci->contact.last_name = uci->contact.last_name;
						GlobalFree( ci->last_name );
						ci->last_name = uci->last_name;
					}

					if ( uci->contact.nickname != NULL )
					{
						GlobalFree( ci->contact.nickname );
						ci->contact.nickname = uci->contact.nickname;
						GlobalFree( ci->nickname );
						ci->nickname = uci->nickname;
					}

					if ( uci->contact.title != NULL )
					{
						GlobalFree( ci->contact.title );
						ci->contact.title = uci->contact.title;
						GlobalFree( ci->title );
						ci->title = uci->title;
					}

					if ( uci->contact.business_name != NULL )
					{
						GlobalFree( ci->contact.business_name );
						ci->contact.business_name = uci->contact.business_name;
						GlobalFree( ci->business_name );
						ci->business_name = uci->business_name;
					}

					if ( uci->contact.designation != NULL )
					{
						GlobalFree( ci->contact.designation );
						ci->contact.designation = uci->contact.designation;
						GlobalFree( ci->designation );
						ci->designation = uci->designation;
					}

					if ( uci->contact.department != NULL )
					{
						GlobalFree( ci->contact.department );
						ci->contact.department = uci->contact.department;
						GlobalFree( ci->department );
						ci->department = uci->department;
					}

					if ( uci->contact.category != NULL )
					{
						GlobalFree( ci->contact.category );
						ci->contact.category = uci->contact.category;
						GlobalFree( ci->category );
						ci->category = uci->category;
					}

					if ( uci->contact.email_address != NULL )
					{
						GlobalFree( ci->contact.email_address );
						ci->contact.email_address = uci->contact.email_address;
						GlobalFree( ci->email_address );
						ci->email_address = uci->email_address;
					}

					if ( uci->contact.web_page != NULL )
					{
						GlobalFree( ci->contact.web_page );
						ci->contact.web_page = uci->contact.web_page;
						GlobalFree( ci->web_page );
						ci->web_page = uci->web_page;
					}

					if ( uci->contact.home_phone_number != NULL )
					{
						remove_custom_caller_id( ci );

						GlobalFree( ci->contact.home_phone_number );
						ci->contact.home_phone_number = uci->contact.home_phone_number;
						GlobalFree( ci->home_phone_number );
						ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );

						add_custom_caller_id( ci );
					}

					if ( uci->contact.cell_phone_number != NULL )
					{
						remove_custom_caller_id( ci );

						GlobalFree( ci->contact.cell_phone_number );
						ci->contact.cell_phone_number = uci->contact.cell_phone_number;
						GlobalFree( ci->cell_phone_number );
						ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );

						add_custom_caller_id( ci );
					}

					if ( uci->contact.office_phone_number != NULL )
					{
						remove_custom_caller_id( ci );

						GlobalFree( ci->contact.office_phone_number );
						ci->contact.office_phone_number = uci->contact.office_phone_number;
						GlobalFree( ci->office_phone_number );
						ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );

						add_custom_caller_id( ci );
					}

					if ( uci->contact.other_phone_number != NULL )
					{
						remove_custom_caller_id( ci );

						GlobalFree( ci->contact.other_phone_number );
						ci->contact.other_phone_number = uci->contact.other_phone_number;
						GlobalFree( ci->other_phone_number );
						ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );

						add_custom_caller_id( ci );
					}

					if ( uci->contact.work_phone_number != NULL )
					{
						remove_custom_caller_id( ci );

						GlobalFree( ci->contact.work_phone_number );
						ci->contact.work_phone_number = uci->contact.work_phone_number;
						GlobalFree( ci->work_phone_number );
						ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );

						add_custom_caller_id( ci );
					}

					if ( uci->contact.fax_number != NULL )
					{
						remove_custom_caller_id( ci );

						GlobalFree( ci->contact.fax_number );
						ci->contact.fax_number = uci->contact.fax_number;
						GlobalFree( ci->fax_number );
						ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );

						add_custom_caller_id( ci );
					}

					if ( uci->picture_path != NULL || cui->remove_picture )
					{
						GlobalFree( ci->picture_path );
						ci->picture_path = uci->picture_path;
					}

					ci->ringtone_info = uci->ringtone_info;

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

	char range_number[ 32 ];

	displayinfo *di = ( displayinfo * )pArguments;

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Received_incoming_caller_ID_information )

	if ( di != NULL )
	{
		// Each number in our call log contains a linked list. Find the number if it exists and add it to the linked list.

		// Create the node to insert into a linked list.
		DoublyLinkedList *di_node = DLL_CreateNode( ( void * )di );

		// See if our tree has the phone number to add the node to.
		DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
		if ( dll == NULL )
		{
			// If no phone number exits, insert the node into the tree.
			if ( dllrbt_insert( call_log, ( void * )di->ci.call_from, ( void * )di_node ) != DLLRBT_STATUS_OK )
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

		// Search the ignore_list for a match.
		ignoreinfo *ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )di->ci.call_from, true );

		// Try searching the range list.
		if ( ii == NULL )
		{
			_memzero( range_number, 32 );

			int range_index = lstrlenA( di->ci.call_from );
			range_index = ( range_index > 0 ? range_index - 1 : 0 );

			if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) )
			{
				ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )range_number, true );
			}
		}

		if ( ii != NULL )
		{
			di->process_incoming = false;
			di->ignore_phone_number = true;
			di->ci.ignored = true;

			IgnoreIncomingCall( di->incoming_call );

			ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
			iui->ii = ii;
			iui->phone_number = NULL;
			iui->action = 3;	// Update the call count.
			iui->hWnd = g_hWnd_ignore_list;

			// iui is freed in the update_ignore_list thread.
//			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
		}
		else
		{
			// Search for the first ignore caller ID list match. di->ignore_cid_match_count will be updated here for all keyword matches.
			ignorecidinfo *icidi = find_ignore_caller_id_name_match( di );
			if ( icidi != NULL )
			{
				di->process_incoming = false;
				di->ci.ignored = true;

				IgnoreIncomingCall( di->incoming_call );

				ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidupdateinfo ) );
				icidui->icidi = icidi;
				icidui->caller_id = NULL;
				icidui->action = 4;	// Update the call count.
				icidui->hWnd = g_hWnd_ignore_cid_list;
				icidui->match_case = false;
				icidui->match_whole_word = false;

				// icidui is freed in the update_ignore_cid_list thread.
//				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_cid_list, ( void * )icidui, 0, NULL ) );
			}
		}

		// If the incoming number matches a contact, then update the caller ID value.
		char *custom_caller_id = get_custom_caller_id( di->ci.call_from, &di->contact_info );
		if ( custom_caller_id != NULL )
		{
			GlobalFree( di->ci.caller_id );
			di->ci.caller_id = custom_caller_id;
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

	if ( hWnd == g_hWnd_call_log )
	{
		column_type = 0;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		column_type = 1;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		column_type = 2;
	}
	else if ( hWnd == g_hWnd_ignore_cid_list )
	{
		column_type = 3;
	}

	Processing_Window( hWnd, false );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;
	lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

	int item_count = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );
	
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
		column_end = ( column_type == 0 ? total_columns1 :
					 ( column_type == 1 ? total_columns2 :
					 ( column_type == 2 ? total_columns3 : total_columns4 ) ) );
		_SendMessageW( hWnd, LVM_GETCOLUMNORDERARRAY, column_end, ( LPARAM )arr );

		if ( column_type == 0 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, call_log_columns, NUM_COLUMNS1, total_columns1 );
		}
		else if ( column_type == 1 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, contact_list_columns, NUM_COLUMNS2, total_columns2 );
		}
		else if ( column_type == 2 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, ignore_list_columns, NUM_COLUMNS3, total_columns3 );
		}
		else if ( column_type == 3 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, ignore_cid_list_columns, NUM_COLUMNS4, total_columns4 );
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
			case MENU_COPY_SEL_COL43: { arr[ 0 ] = 3; } break;

			case MENU_COPY_SEL_COL4:
			case MENU_COPY_SEL_COL24:
			case MENU_COPY_SEL_COL44: { arr[ 0 ] = 4; } break;

			case MENU_COPY_SEL_COL5:
			case MENU_COPY_SEL_COL25: { arr[ 0 ] = 5; } break;

			case MENU_COPY_SEL_COL26: { arr[ 0 ] = 6; } break;
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
			lvi.iItem = _SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

		displayinfo *di = NULL;
		contactinfo *ci = NULL;
		ignoreinfo *ii = NULL;
		ignorecidinfo *icidi = NULL;

		if ( column_type == 0 )
		{
			di = ( displayinfo * )lvi.lParam;
		}
		else if ( column_type == 1 )
		{
			ci = ( contactinfo * )lvi.lParam;
		}
		else if ( column_type == 2 )
		{
			ii = ( ignoreinfo * )lvi.lParam;
		}
		else if ( column_type == 3 )
		{
			icidi = ( ignorecidinfo * )lvi.lParam;
		}

		add_newline = add_tab = false;

		for ( int j = column_start; j < column_end; ++j )
		{
			switch ( arr[ j ] )
			{
				case 1:
				case 2:
				{
					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] :
								  ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] :
								  ( column_type == 2 ? ii->ignoreinfo_values[ arr[ j ] - 1 ] : icidi->ignorecidinfo_values[ arr[ j ] - 1 ] ) ) );
				}
				break;

				case 3:
				{
					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] :
								  ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] : icidi->ignorecidinfo_values[ arr[ j ] - 1 ] ) );
				}
				break;

				case 4:
				{
					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] :
								  ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] : icidi->ignorecidinfo_values[ arr[ j ] - 1 ] ) );
				}
				break;

				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				{
					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] : ci->contactinfo_values[ arr[ j ] - 1 ] );
				}
				break;

				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				{
					copy_string = ci->contactinfo_values[ arr[ j ] - 1 ];
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
