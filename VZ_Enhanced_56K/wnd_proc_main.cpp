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
#include "wnd_subproc.h"
#include "connection.h"
#include "menus.h"
#include "utilities.h"
#include "ringtone_utilities.h"
#include "list_operations.h"
#include "file_operations.h"
#include "string_tables.h"

#include "lite_gdi32.h"
#include "lite_comdlg32.h"
#include "lite_ole32.h"

#include "telephony.h"

// Object variables
HWND g_hWnd_call_log = NULL;			// Handle to the call log listview control.
HWND g_hWnd_contact_list = NULL;
HWND g_hWnd_ignore_tab = NULL;
HWND g_hWnd_ignore_cid_list = NULL;
HWND g_hWnd_ignore_list = NULL;
HWND g_hWnd_allow_tab = NULL;
HWND g_hWnd_allow_cid_list = NULL;
HWND g_hWnd_allow_list = NULL;
HWND g_hWnd_edit = NULL;				// Handle to the listview edit control.
HWND g_hWnd_tab = NULL;

HWND g_hWnd_tooltip = NULL;

wchar_t *tooltip_buffer = NULL;
int last_tooltip_item = -1;				// Prevent our hot tracking from calling the tooltip on the same item.

WNDPROC ListsProc = NULL;
WNDPROC TabListsProc = NULL;

HCURSOR wait_cursor = NULL;				// Temporary cursor while processing entries.

NOTIFYICONDATA g_nid;					// Tray icon information.

// Window variables
int cx = 0;								// Current x (left) position of the main window based on the mouse.
int cy = 0;								// Current y (top) position of the main window based on the mouse.

unsigned char g_total_tabs = 0;

unsigned char g_total_columns1 = 0;
unsigned char g_total_columns2 = 0;
unsigned char g_total_columns3 = 0;
unsigned char g_total_columns4 = 0;
unsigned char g_total_columns5 = 0;
unsigned char g_total_columns6 = 0;

bool last_menu = false;		// true if context menu was last open, false if main menu was last open. See: WM_ENTERMENULOOP

bool main_active = false;	// Ugly work-around for misaligned listview rows when calling LVM_ENSUREVISIBLE and the window has not initially been shown.

#define IDT_UPDATE_TIMER	10000
#define IDT_SAVE_TIMER		10001

VOID CALLBACK UpdateTimerProc( HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime )
{
	// We'll check the setting again in case the user turned it off before the 10 second grace period.
	// We'll also check to see if the update was manually checked.
	if ( cfg_check_for_updates && update_check_state == 0 )
	{
		// Create the update window so that our update check can send messages to it.
		if ( g_hWnd_update == NULL )
		{
			g_hWnd_update = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"update", ST_Checking_For_Updates___, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 510 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 240 ) / 2 ), 510, 240, NULL, NULL, NULL, NULL );

			update_check_state = 2;	// Automatic update check.

			UPDATE_CHECK_INFO *update_info = ( UPDATE_CHECK_INFO * )GlobalAlloc( GPTR, sizeof( UPDATE_CHECK_INFO ) );
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, ( void * )update_info, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
			else
			{
				GlobalFree( update_info );
			}
		}
	}

	_KillTimer( hWnd, IDT_UPDATE_TIMER );
}

VOID CALLBACK SaveTimerProc( HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime )
{
	CloseHandle( ( HANDLE )_CreateThread( NULL, 0, AutoSave, ( void * )NULL, 0, NULL ) );
}

// Sort function for columns.
int CALLBACK CompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	sort_info *si = ( sort_info * )lParamSort;

	int arr[ 17 ];

	if ( si->hWnd == g_hWnd_call_log )
	{
		display_info *fi1 = ( display_info * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		display_info *fi2 = ( display_info * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_call_log, LVM_GETCOLUMNORDERARRAY, g_total_columns3, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, call_log_columns, NUM_COLUMNS3, g_total_columns3 );

		switch ( arr[ si->column ] )
		{
			case 1: { return ( fi1->allow_cid_match_count > fi2->allow_cid_match_count ); } break;

			case 2:
			{
				if ( fi1->allow_phone_number == fi2->allow_phone_number )
				{
					return 0;
				}
				else if ( fi1->allow_phone_number && !fi2->allow_phone_number )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;

			case 3:
			{
				wchar_t *caller_id_1 = ( fi1->custom_caller_id != NULL ? fi1->custom_caller_id : fi1->caller_id );
				wchar_t *caller_id_2 = ( fi2->custom_caller_id != NULL ? fi2->custom_caller_id : fi2->caller_id );

				return _wcsicmp_s( caller_id_1, caller_id_2 );
			}
			break;

			case 4: { return ( fi1->time.QuadPart > fi2->time.QuadPart ); } break;
			case 5: { return ( fi1->ignore_cid_match_count > fi2->ignore_cid_match_count ); } break;

			case 6:
			{
				if ( fi1->ignore_phone_number == fi2->ignore_phone_number )
				{
					return 0;
				}
				else if ( fi1->ignore_phone_number && !fi2->ignore_phone_number )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;

			case 7: { return _wcsicmp_s( fi1->w_phone_number, fi2->w_phone_number ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}
	else if ( si->hWnd == g_hWnd_contact_list )
	{
		contact_info *fi1 = ( contact_info * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		contact_info *fi2 = ( contact_info * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_contact_list, LVM_GETCOLUMNORDERARRAY, g_total_columns4, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, contact_list_columns, NUM_COLUMNS4, g_total_columns4 );

		return _wcsicmp_s( fi1->w_contact_info_values[ arr[ si->column ] - 1 ], fi2->w_contact_info_values[ arr[ si->column ] - 1 ] );
	}
	else if ( si->hWnd == g_hWnd_ignore_list )
	{
		allow_ignore_info *fi1 = ( allow_ignore_info * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		allow_ignore_info *fi2 = ( allow_ignore_info * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_ignore_list, LVM_GETCOLUMNORDERARRAY, g_total_columns5, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, ignore_list_columns, NUM_COLUMNS5, g_total_columns5 );

		switch ( arr[ si->column ] )
		{
			case 1: { return ( fi1->last_called.QuadPart > fi2->last_called.QuadPart ); } break;
			case 2: { return _wcsicmp_s( fi1->w_phone_number, fi2->w_phone_number ); } break;
			case 3:	{ return ( fi1->count > fi2->count ); } break;

			default:
			{
				return 0;
			}
			break;
		}
	}
	else if ( si->hWnd == g_hWnd_ignore_cid_list )
	{
		allow_ignore_cid_info *fi1 = ( allow_ignore_cid_info * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		allow_ignore_cid_info *fi2 = ( allow_ignore_cid_info * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_ignore_cid_list, LVM_GETCOLUMNORDERARRAY, g_total_columns6, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, ignore_cid_list_columns, NUM_COLUMNS6, g_total_columns6 );

		switch ( arr[ si->column ] )
		{
			case 1: { return _wcsicmp_s( fi1->caller_id, fi2->caller_id ); } break;
			case 2: { return ( fi1->last_called.QuadPart > fi2->last_called.QuadPart ); } break;

			case 3:
			{
				if ( ( fi1->match_flag & 0x02 ) == ( fi2->match_flag & 0x02 ) )
				{
					return 0;
				}
				else if ( ( fi1->match_flag & 0x02 ) && !( fi2->match_flag & 0x02 ) )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;

			case 4:
			{
				if ( ( fi1->match_flag & 0x01 ) == ( fi2->match_flag & 0x01 ) )
				{
					return 0;
				}
				else if ( ( fi1->match_flag & 0x01 ) && !( fi2->match_flag & 0x01 ) )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;

			case 5:
			{
				if ( ( fi1->match_flag & 0x04 ) == ( fi2->match_flag & 0x04 ) )
				{
					return 0;
				}
				else if ( ( fi1->match_flag & 0x04 ) && !( fi2->match_flag & 0x04 ) )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;
			
			case 6:	{ return ( fi1->count > fi2->count ); } break;

			default:
			{
				return 0;
			}
			break;
		}
	}

	return 0;
}

wchar_t *GetListViewInfoString( unsigned char column_info_type, void *info, int column, int item_index, wchar_t *tbuf, unsigned short tbuf_size )
{
	wchar_t *buf = NULL;

	switch ( column_info_type )
	{
		case 0:
		case 4:
		{
			allow_ignore_info *aii = ( allow_ignore_info * )info;

			switch ( column )
			{
				case 0:
				{
					buf = tbuf;	// Reset the buffer pointer.

					__snwprintf( buf, tbuf_size, L"%lu", item_index );
				}
				break;
				case 1: { buf = aii->w_last_called; } break;
				case 2: { buf = aii->w_phone_number; } break;
				case 3:
				{
					buf = tbuf;	// Reset the buffer pointer.

					__snwprintf( buf, tbuf_size, L"%lu", aii->count );
				}
				break;
			}
		}
		break;
	
		case 1:
		case 5:
		{
			allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )info;

			switch ( column )
			{
				case 0:
				{
					buf = tbuf;	// Reset the buffer pointer.

					__snwprintf( buf, tbuf_size, L"%lu", item_index );
				}
				break;
				case 1: { buf = aicidi->caller_id; } break;
				case 2: { buf = aicidi->w_last_called; } break;
				case 3: { buf = ( ( aicidi->match_flag & 0x02 ) ? ST_Yes : ST_No ); } break;
				case 4: { buf = ( ( aicidi->match_flag & 0x01 ) ? ST_Yes : ST_No ); } break;
				case 5: { buf = ( ( aicidi->match_flag & 0x04 ) ? ST_Yes : ST_No ); } break;
				case 6:
				{
					buf = tbuf;	// Reset the buffer pointer.

					__snwprintf( buf, tbuf_size, L"%lu", aicidi->count );
				}
				break;
			}
		}
		break;

		case 2:
		{
			display_info *di = ( display_info * )info;

			// Save the appropriate text in our buffer for the current column.
			switch ( column )
			{
				case 0:
				{
					buf = tbuf;	// Reset the buffer pointer.

					__snwprintf( buf, tbuf_size, L"%lu", item_index );
				}
				break;
				case 1: { buf = ( di->allow_cid_match_count > 0 ? ST_Yes : ST_No ); } break;
				case 2: { buf = ( di->allow_phone_number ? ST_Yes : ST_No ); } break;
				case 3: { buf = ( di->custom_caller_id != NULL ? di->custom_caller_id : di->caller_id ); } break;
				case 4: { buf = di->w_time; } break;
				case 5: { buf = ( di->ignore_cid_match_count > 0 ? ST_Yes : ST_No ); } break;
				case 6: { buf = ( di->ignore_phone_number ? ST_Yes : ST_No ); } break;
				case 7: { buf = di->w_phone_number; } break;
			}
		}
		break;
	
		case 3:
		{
			contact_info *ci = ( contact_info * )info;

			switch ( column )
			{
				case 0:
				{
					buf = tbuf;	// Reset the buffer pointer.

					__snwprintf( buf, tbuf_size, L"%lu", item_index );
				}
				break;
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				{
					buf = ci->w_contact_info_values[ column - 1 ];
				}
				break;
			}
		}
		break;
	}

	return buf;
}

LRESULT CALLBACK ListsSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		// This will essentially reconstruct the string that the open file dialog box creates when selecting multiple files.
		case WM_DROPFILES:
		{
			int count = _DragQueryFileW( ( HDROP )wParam, -1, NULL, 0 );

			importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GPTR, sizeof( importexportinfo ) );

			if		( hWnd == g_hWnd_call_log )			{ iei->file_type = IE_CALL_LOG_HISTORY; }
			else if ( hWnd == g_hWnd_contact_list )		{ iei->file_type = IE_CONTACT_LIST; }
			else if ( hWnd == g_hWnd_allow_cid_list )	{ iei->file_type = IE_ALLOW_CID_LIST; }
			else if ( hWnd == g_hWnd_allow_list )		{ iei->file_type = IE_ALLOW_PN_LIST; }
			else if ( hWnd == g_hWnd_ignore_cid_list )	{ iei->file_type = IE_IGNORE_CID_LIST; }
			else if ( hWnd == g_hWnd_ignore_list )		{ iei->file_type = IE_IGNORE_PN_LIST; }

			wchar_t file_path[ MAX_PATH ];

			int file_paths_offset = 0;	// Keeps track of the last file in filepath.
			int file_paths_length = ( MAX_PATH * count ) + 1;

			// Go through the list of paths.
			for ( int i = 0; i < count; ++i )
			{
				// Get the file path and its length.
				int file_path_length = _DragQueryFileW( ( HDROP )wParam, i, file_path, MAX_PATH ) + 1;	// Include the NULL terminator.

				// Skip any folders that were dropped.
				if ( !( GetFileAttributesW( file_path ) & FILE_ATTRIBUTE_DIRECTORY ) )
				{
					if ( iei->file_paths == NULL )
					{
						iei->file_paths = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * file_paths_length );
						iei->file_offset = file_path_length;

						// Find the last occurance of "\" in the string.
						while ( iei->file_offset != 0 && file_path[ --iei->file_offset ] != L'\\' );

						// Save the root directory name.
						_wmemcpy_s( iei->file_paths, file_paths_length - iei->file_offset, file_path, iei->file_offset );

						file_paths_offset = ++iei->file_offset;
					}

					// Copy the file name. Each is separated by the NULL character.
					_wmemcpy_s( iei->file_paths + file_paths_offset, file_paths_length - file_paths_offset, file_path + iei->file_offset, file_path_length - iei->file_offset );

					file_paths_offset += ( file_path_length - iei->file_offset );
				}
			}

			_DragFinish( ( HDROP )wParam );

			if ( iei->file_paths != NULL )
			{
				// iei will be freed in the import_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL );
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
			else	// No files were dropped.
			{
				GlobalFree( iei );
			}

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case HDN_DIVIDERDBLCLICK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					char **columns;
					unsigned char num_columns;
					int **columns_width;
					unsigned char column_info_type;

					if ( hWnd == g_hWnd_allow_list )
					{
						columns = allow_list_columns;
						num_columns = NUM_COLUMNS1;
						columns_width = allow_list_columns_width;
						column_info_type = 0;
					}
					else if ( hWnd == g_hWnd_allow_cid_list )
					{
						columns = allow_cid_list_columns;
						num_columns = NUM_COLUMNS2;
						columns_width = allow_cid_list_columns_width;
						column_info_type = 1;
					}
					else if ( hWnd == g_hWnd_call_log )
					{
						columns = call_log_columns;
						num_columns = NUM_COLUMNS3;
						columns_width = call_log_columns_width;
						column_info_type = 2;
					}
					else if ( hWnd == g_hWnd_contact_list )
					{
						columns = contact_list_columns;
						num_columns = NUM_COLUMNS4;
						columns_width = contact_list_columns_width;
						column_info_type = 3;
					}
					else if ( hWnd == g_hWnd_ignore_list )
					{
						columns = ignore_list_columns;
						num_columns = NUM_COLUMNS5;
						columns_width = ignore_list_columns_width;
						column_info_type = 4;
					}
					else if ( hWnd == g_hWnd_ignore_cid_list )
					{
						columns = ignore_cid_list_columns;
						num_columns = NUM_COLUMNS6;
						columns_width = ignore_cid_list_columns_width;
						column_info_type = 5;
					}

					int largest_width;

					int virtual_index = GetVirtualIndexFromColumnIndex( nmh->iItem, columns, num_columns );

					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						largest_width = LVSCW_AUTOSIZE_USEHEADER;
					}
					else
					{
						largest_width = 26;	// 5 + 16 + 5.

						wchar_t tbuf[ 11 ];

						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );

						int index = ( int )_SendMessageW( hWnd, LVM_GETTOPINDEX, 0, 0 );
						int index_end = ( int )_SendMessageW( hWnd, LVM_GETCOUNTPERPAGE, 0, 0 ) + index;

						RECT rc;
						HDC hDC = _GetDC( hWnd );
						HFONT ohf = ( HFONT )_SelectObject( hDC, hFont );
						_DeleteObject( ohf );

						for ( ; index <= index_end; ++index )
						{
							lvi.iItem = index;
							lvi.mask = LVIF_PARAM;
							if ( _SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi ) == TRUE )
							{
								if ( lvi.lParam != NULL )
								{
									wchar_t *buf = GetListViewInfoString( column_info_type, ( void * )lvi.lParam, virtual_index, index + 1, tbuf, 11 );

									if ( buf == NULL )
									{
										tbuf[ 0 ] = L'\0';
										buf = tbuf;
									}

									rc.bottom = rc.left = rc.right = rc.top = 0;

									_DrawTextW( hDC, buf, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT );

									int width = ( rc.right - rc.left ) + 10;	// 5 + 5 padding.
									if ( width > largest_width )
									{
										largest_width = width;
									}
								}
							}
							else
							{
								break;
							}
						}

						_ReleaseDC( hWnd, hDC );
					}

					_SendMessageW( hWnd, LVM_SETCOLUMNWIDTH, nmh->iItem, largest_width );

					// Save our new column width.
					*columns_width[ virtual_index ] = ( int )_SendMessageW( hWnd, LVM_GETCOLUMNWIDTH, nmh->iItem, 0 );

					return TRUE;
				}
				break;
			}
		}
		break;
	}

	return _CallWindowProcW( ListsProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK TabListsSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_MEASUREITEM:
		{
			// Set the row height of the list view.
			if ( ( ( LPMEASUREITEMSTRUCT )lParam )->CtlType = ODT_LISTVIEW )
			{
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = row_height;
			}
			return TRUE;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_LISTVIEW && dis->itemData != NULL )
			{
				// Alternate item color's background.
				if ( dis->itemID & 1 )	// Even rows will have a light grey background.
				{
					HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xF7, 0xF7, 0xF7 ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					if ( skip_list_draw )
					{
						return TRUE;	// Don't draw selected items because their lParam values are being deleted.
					}

					HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
					selected = true;
				}

				// Get the item's text.
				wchar_t tbuf[ 11 ];
				wchar_t *buf = NULL;

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				int arr[ 17 ];
				int arr2[ 17 ];

				unsigned char column_info_type;

				int column_count = 0;
				if ( dis->hwndItem == g_hWnd_allow_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, g_total_columns1, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS1 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, allow_list_columns, NUM_COLUMNS1, g_total_columns1 );

					column_count = g_total_columns1;

					column_info_type = 0;
				}
				else if ( dis->hwndItem == g_hWnd_allow_cid_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, g_total_columns2, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS2 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, allow_cid_list_columns, NUM_COLUMNS2, g_total_columns2 );

					column_count = g_total_columns2;

					column_info_type = 1;
				}
				else if ( dis->hwndItem == g_hWnd_call_log )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, g_total_columns3, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS3 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, call_log_columns, NUM_COLUMNS3, g_total_columns3 );

					column_count = g_total_columns3;

					column_info_type = 2;
				}
				else if ( dis->hwndItem == g_hWnd_contact_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, g_total_columns4, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS4 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, contact_list_columns, NUM_COLUMNS4, g_total_columns4 );

					column_count = g_total_columns4;

					column_info_type = 3;
				}
				else if ( dis->hwndItem == g_hWnd_ignore_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, g_total_columns5, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS5 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, ignore_list_columns, NUM_COLUMNS5, g_total_columns5 );

					column_count = g_total_columns5;

					column_info_type = 4;
				}
				else if ( dis->hwndItem == g_hWnd_ignore_cid_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, g_total_columns6, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS6 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, ignore_cid_list_columns, NUM_COLUMNS6, g_total_columns6 );

					column_count = g_total_columns6;

					column_info_type = 5;
				}

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i < column_count; ++i )
				{
					// Save the appropriate text in our buffer for the current column.
					buf = GetListViewInfoString( column_info_type, ( void * )dis->itemData, arr2[ i ], dis->itemID + 1, tbuf, 11 );

					if ( buf == NULL )
					{
						tbuf[ 0 ] = L'\0';
						buf = tbuf;
					}

					// Get the dimensions of the listview column
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMN, arr[ i ], ( LPARAM )&lvc );

					last_rc = dis->rcItem;

					// This will adjust the text to fit nicely into the rectangle.
					last_rc.left = 5 + last_left;
					last_rc.right = lvc.cx + last_left - 5;

					// Save the last left position of our column.
					last_left += lvc.cx;

					// Save the height and width of this region.
					int width = last_rc.right - last_rc.left;
					if ( width <= 0 )
					{
						continue;
					}

					int height = last_rc.bottom - last_rc.top;

					// Normal text position.
					RECT rc;
					rc.left = 0;
					rc.top = 0;
					rc.right = width;
					rc.bottom = height;

					// Create and save a bitmap in memory to paint to.
					HDC hdcMem = _CreateCompatibleDC( dis->hDC );
					HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, width, height );
					HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
					_DeleteObject( ohbm );
					_DeleteObject( hbm );
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
					_DeleteObject( ohf );

					// Transparent background for text.
					_SetBkMode( hdcMem, TRANSPARENT );

					// Draw selected text
					if ( selected )
					{
						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// White text.
						_SetTextColor( hdcMem, _GetSysColor( COLOR_WINDOW ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
					}
					else	// Draw normal text.
					{
						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// Black text for normal entries and red for ignored.
						if ( dis->hwndItem == g_hWnd_call_log && ( ( display_info * )dis->itemData )->ignored )
						{
							_SetTextColor( hdcMem, RGB( 0xFF, 0x00, 0x00 ) );
						}
						else
						{
							_SetTextColor( hdcMem, _GetSysColor( COLOR_WINDOWTEXT ) );
						}
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCAND );
					}

					// Delete our back buffer.
					_DeleteDC( hdcMem );
				}
			}
			return TRUE;
		}
		break;
	}

	return _CallWindowProcW( TabListsProc, hWnd, msg, wParam, lParam );
}

void HandleCommand( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	switch ( LOWORD( wParam ) )
	{
		case MENU_CLOSE_TAB:
		{
			switch ( context_tab_index )
			{
				case 0: { _SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_ALLOW_LISTS, 0 ), 0 ); } break;
				case 1: { _SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_CALL_LOG, 0 ), 0 ); } break;
				case 2: { _SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_CONTACT_LIST, 0 ), 0 ); } break;
				case 3: { _SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_IGNORE_LISTS, 0 ), 0 ); } break;
			}
		}
		break;

		case MENU_VIEW_ALLOW_LISTS:
		case MENU_VIEW_CALL_LOG:
		case MENU_VIEW_CONTACT_LIST:
		case MENU_VIEW_IGNORE_LISTS:
		{
			char *tab_index = NULL;
			HWND t_hWnd = NULL;

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;			// The tab will have text and an lParam value.

			if ( LOWORD( wParam ) == MENU_VIEW_ALLOW_LISTS )
			{
				tab_index = &cfg_tab_order1;
				t_hWnd = g_hWnd_allow_tab;
				ti.pszText = ( LPWSTR )ST_Allow_Lists;		// This will simply set the width of each tab item. We're not going to use it.
			}
			else if ( LOWORD( wParam ) == MENU_VIEW_CALL_LOG )
			{
				tab_index = &cfg_tab_order2;
				t_hWnd = g_hWnd_call_log;
				ti.pszText = ( LPWSTR )ST_Call_Log;		// This will simply set the width of each tab item. We're not going to use it.
			}
			else if ( LOWORD( wParam ) == MENU_VIEW_CONTACT_LIST )
			{
				tab_index = &cfg_tab_order3;
				t_hWnd = g_hWnd_contact_list;
				ti.pszText = ( LPWSTR )ST_Contact_List;		// This will simply set the width of each tab item. We're not going to use it.
			}
			else if ( LOWORD( wParam ) == MENU_VIEW_IGNORE_LISTS )
			{
				tab_index = &cfg_tab_order4;
				t_hWnd = g_hWnd_ignore_tab;
				ti.pszText = ( LPWSTR )ST_Ignore_Lists;		// This will simply set the width of each tab item. We're not going to use it.
			}

			if ( *tab_index != -1 )	// Remove the tab.
			{
				// Go through all the tabs and decrease the order values that are greater than the index we removed.
				for ( unsigned char i = 0; i < NUM_TABS; ++i )
				{
					if ( *tab_order[ i ] != -1 && *tab_order[ i ] > *tab_index )
					{
						( *( tab_order[ i ] ) )--;
					}
				}

				*tab_index = -1;

				for ( int i = 0; i < g_total_tabs; ++i )
				{
					_SendMessageW( g_hWnd_tab, TCM_GETITEM, i, ( LPARAM )&ti );

					if ( ( HWND )ti.lParam == t_hWnd )
					{
						int index = ( int )_SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab

						// If the tab we remove is the last tab and it is focused, then set focus to the tab on the left.
						// If the tab we remove is focused and there is a tab to the right, then set focus to the tab on the right.
						if ( g_total_tabs > 1 && i == index )
						{
							_SendMessageW( g_hWnd_tab, TCM_SETCURFOCUS, ( i < g_total_tabs - 1 ? i + 1 : i - 1 ), 0 );	// Give the new tab focus.
						}

						_SendMessageW( g_hWnd_tab, TCM_DELETEITEM, i, 0 );
						--g_total_tabs;

						// Hide the tab control if no more tabs are visible.
						if ( g_total_tabs == 0 )
						{
							_ShowWindow( ( HWND )ti.lParam, SW_HIDE );
							_ShowWindow( g_hWnd_tab, SW_HIDE );
						}
					}
				}
			}
			else	// Add the tab.
			{
				*tab_index = g_total_tabs;	// The new tab will be added to the end.

				ti.lParam = ( LPARAM )t_hWnd;
				_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, g_total_tabs, ( LPARAM )&ti );	// Insert a new tab at the end.

				// If no tabs were previously visible, then show the tab control.
				if ( g_total_tabs == 0 )
				{
					_ShowWindow( g_hWnd_tab, SW_SHOW );
					_ShowWindow( t_hWnd, SW_SHOW );

					_SendMessageW( hWnd, WM_SIZE, 0, 0 );	// Forces the window to resize the listview.
				}

				++g_total_tabs;
			}

			_CheckMenuItem( g_hMenu, LOWORD( wParam ), ( *tab_index != -1 ? MF_CHECKED : MF_UNCHECKED ) );
		}
		break;

		case MENU_COPY_SEL:
		case MENU_COPY_SEL_COL1:
		case MENU_COPY_SEL_COL2:
		case MENU_COPY_SEL_COL3:
		case MENU_COPY_SEL_COL4:
		case MENU_COPY_SEL_COL5:
		case MENU_COPY_SEL_COL6:
		case MENU_COPY_SEL_COL7:
		case MENU_COPY_SEL_COL21:
		case MENU_COPY_SEL_COL22:
		case MENU_COPY_SEL_COL23:
		case MENU_COPY_SEL_COL24:
		case MENU_COPY_SEL_COL25:
		case MENU_COPY_SEL_COL26:
		case MENU_COPY_SEL_COL27:
		case MENU_COPY_SEL_COL28:
		case MENU_COPY_SEL_COL29:
		case MENU_COPY_SEL_COL210:
		case MENU_COPY_SEL_COL211:
		case MENU_COPY_SEL_COL212:
		case MENU_COPY_SEL_COL213:
		case MENU_COPY_SEL_COL214:
		case MENU_COPY_SEL_COL215:
		case MENU_COPY_SEL_COL216:
		case MENU_COPY_SEL_COL31:
		case MENU_COPY_SEL_COL32:
		case MENU_COPY_SEL_COL33:
		case MENU_COPY_SEL_COL41:
		case MENU_COPY_SEL_COL42:
		case MENU_COPY_SEL_COL43:
		case MENU_COPY_SEL_COL44:
		case MENU_COPY_SEL_COL45:
		case MENU_COPY_SEL_COL46:
		{
			HWND c_hWnd = GetCurrentListView();
			if ( c_hWnd != NULL )
			{
				copyinfo *ci = ( copyinfo * )GlobalAlloc( GMEM_FIXED, sizeof( copyinfo ) );
				ci->column = LOWORD( wParam );
				ci->hWnd = c_hWnd;

				// ci is freed in the copy_items thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, copy_items, ( void * )ci, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( ci );
				}
			}
		}
		break;

		case MENU_REMOVE_SEL:
		{
			HWND c_hWnd = GetCurrentListView();
			if ( c_hWnd != NULL )
			{
				if ( c_hWnd == g_hWnd_call_log )
				{
					if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
					{
						removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
						ri->is_thread = true;
						ri->hWnd = c_hWnd;

						// ri is freed in the remove_items thread.
						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );
						if ( thread != NULL )
						{
							CloseHandle( thread );
						}
						else
						{
							GlobalFree( ri );
						}
					}
				}
				else
				{
					// Remove the first selected (not the focused & selected).
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_PARAM;
					lvi.iItem = ( int )_SendMessageW( c_hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

					if ( lvi.iItem != -1 )
					{
						if ( c_hWnd == g_hWnd_contact_list )
						{
							if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries_contact_list, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
							{
								contact_update_info *cui = ( contact_update_info * )GlobalAlloc( GPTR, sizeof( contact_update_info ) );
								cui->action = 1;	// 1 = Remove, 0 = Add

								// cui is freed in the update_contact_list thread.
								HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )cui, 0, NULL );
								if ( thread != NULL )
								{
									CloseHandle( thread );
								}
								else
								{
									GlobalFree( cui );
								}
							}
						}
						else if ( c_hWnd == g_hWnd_allow_list || c_hWnd == g_hWnd_ignore_list )
						{
							if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
							{
								allow_ignore_update_info *aiui = ( allow_ignore_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_update_info ) );
								aiui->action = 1;	// 1 = Remove, 0 = Add
								aiui->list_type = ( c_hWnd == g_hWnd_allow_list ? LIST_TYPE_ALLOW : LIST_TYPE_IGNORE );
								aiui->hWnd = c_hWnd;

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
						}
						else if ( c_hWnd == g_hWnd_allow_cid_list || c_hWnd == g_hWnd_ignore_cid_list )
						{
							if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
							{
								allow_ignore_cid_update_info *aicidui = ( allow_ignore_cid_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_update_info ) );
								aicidui->action = 1;	// 1 = Remove, 0 = Add
								aicidui->list_type = ( c_hWnd == g_hWnd_allow_cid_list ? LIST_TYPE_ALLOW : LIST_TYPE_IGNORE );
								aicidui->hWnd = c_hWnd;

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
						}
					}
				}
			}
		}
		break;

		case MENU_SELECT_ALL:
		{
			HWND c_hWnd = GetCurrentListView();
			if ( c_hWnd != NULL )
			{
				// Set the state of all items to selected.
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_STATE;
				lvi.state = LVIS_SELECTED;
				lvi.stateMask = LVIS_SELECTED;
				_SendMessageW( c_hWnd, LVM_SETITEMSTATE, -1, ( LPARAM )&lvi );

				UpdateMenus( UM_ENABLE );
			}
		}
		break;

		case MENU_OPEN_WEB_PAGE:
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ( int )_SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				contact_info *ci = ( contact_info * )lvi.lParam;
				if ( ci != NULL && ci->w_web_page != NULL )
				{
					bool destroy = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							destroy = InitializeOle32();
						}
					#endif

					if ( destroy )
					{
						_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
					}

					_ShellExecuteW( NULL, L"open", ci->w_web_page, NULL, NULL, SW_SHOWNORMAL );

					if ( destroy )
					{
						_CoUninitialize();
					}
				}
			}
		}
		break;

		case MENU_SEARCH_WITH_1:
		case MENU_SEARCH_WITH_2:
		case MENU_SEARCH_WITH_3:
		case MENU_SEARCH_WITH_4:
		case MENU_SEARCH_WITH_5:
		case MENU_SEARCH_WITH_6:
		case MENU_SEARCH_WITH_7:
		case MENU_SEARCH_WITH_8:
		case MENU_SEARCH_WITH_9:
		case MENU_SEARCH_WITH_10:
		{
			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_DATA;
			_GetMenuItemInfoW( g_hMenu, MENU_SEARCH_WITH, FALSE, &mii );

			unsigned short column = ( unsigned short )mii.dwItemData;

			wchar_t *phone_number = GetSelectedColumnPhoneNumber( GetCurrentListView(), column );
			if ( phone_number != NULL && phone_number[ 0 ] == L'+' )
			{
				++phone_number;
			}

			wchar_t *url = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * 128 );

			switch ( LOWORD( wParam ) )
			{
				case MENU_SEARCH_WITH_1: { __snwprintf( url, 128, L"https://800notes.com/Phone.aspx/%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_2: { __snwprintf( url, 128, L"https://www.bing.com/search?q=%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_3: { __snwprintf( url, 128, L"https://callerr.com/%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_4: { __snwprintf( url, 128, L"https://www.google.com/search?&q=%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_5: { __snwprintf( url, 128, L"https://www.nomorobo.com/lookup/%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_6: { __snwprintf( url, 128, L"https://www.okcaller.com/detail.php?number=%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_7: { __snwprintf( url, 128, L"https://www.phonetray.com/lookup/Number/%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_8: { __snwprintf( url, 128, L"https://www.whitepages.com/phone/%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_9: { __snwprintf( url, 128, L"https://whocallsme.com/Phone-Number.aspx/%s", SAFESTRW( phone_number ) ); } break;
				case MENU_SEARCH_WITH_10: { __snwprintf( url, 128, L"https://www.whycall.me/%s.html", SAFESTRW( phone_number ) ); } break;
			}

			bool destroy = true;
			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					destroy = InitializeOle32();
				}
			#endif

			if ( destroy )
			{
				_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
			}

			_ShellExecuteW( NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL );

			if ( destroy )
			{
				_CoUninitialize();
			}

			GlobalFree( url );
		}
		break;

		case MENU_OPEN_EMAIL_ADDRESS:
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ( int )_SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				contact_info *ci = ( contact_info * )lvi.lParam;
				if ( ci != NULL && ci->w_email_address != NULL )
				{
					int email_address_length = lstrlenW( ci->w_email_address );
					wchar_t *mailto = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( email_address_length + 8 ) );
					_wmemcpy_s( mailto, email_address_length + 8, L"mailto:", 7 );
					_wmemcpy_s( mailto + 7, email_address_length + 1, ci->w_email_address, email_address_length );
					mailto[ email_address_length + 7 ] = 0;	// Sanity.

					bool destroy = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							destroy = InitializeOle32();
						}
					#endif

					if ( destroy )
					{
						_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
					}

					_ShellExecuteW( NULL, L"open", mailto, NULL, NULL, SW_SHOWNORMAL );

					if ( destroy )
					{
						_CoUninitialize();
					}

					GlobalFree( mailto );
				}
			}
		}
		break;

		case MENU_INCOMING_IGNORE:
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				display_info *di = ( display_info * )lvi.lParam;

				di->process_incoming = false;
				di->ignored = true;

				_InvalidateRect( g_hWnd_call_log, NULL, TRUE );

				IgnoreIncomingCall( di->incoming_call );
			}
		}
		break;

		case MENU_ADD_ALLOW_IGNORE_LIST:
		case MENU_EDIT_ALLOW_IGNORE_LIST:	// ignore list listview right click.
		{
			if ( g_hWnd_ignore_phone_number == NULL )
			{
				// Allow wildcard input. (Last parameter of CreateWindow is 1)
				g_hWnd_ignore_phone_number = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"phone", ST_Ignore_Phone_Number, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 258 ) / 2 ), 205, 258, NULL, NULL, NULL, ( LPVOID )1 );
			}

			HWND c_hWnd = GetCurrentListView();

			unsigned char add_ignore_type = ( c_hWnd == g_hWnd_allow_list ? 0x04 : 0x00 );

			if ( LOWORD( wParam ) == MENU_ADD_ALLOW_IGNORE_LIST )
			{
				_SendMessageW( g_hWnd_ignore_phone_number, WM_PROPAGATE, 0x01 | add_ignore_type, 0 );

				_SetForegroundWindow( g_hWnd_ignore_phone_number );
			}
			else// if ( LOWORD( wParam ) == MENU_EDIT_ALLOW_IGNORE_LIST )
			{
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM;
				lvi.iItem = ( int )_SendMessageW( c_hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

				if ( lvi.iItem != -1 )
				{
					_SendMessageW( c_hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );
					_SendMessageW( g_hWnd_ignore_phone_number, WM_PROPAGATE, 0x02 | add_ignore_type, lvi.lParam );

					_SetForegroundWindow( g_hWnd_ignore_phone_number );
				}
			}
		}
		break;

		case MENU_ADD_ALLOW_IGNORE_CID_LIST:
		case MENU_EDIT_ALLOW_IGNORE_CID_LIST:	// ignore cid list listview right click.
		{
			if ( g_hWnd_ignore_cid == NULL )
			{
				g_hWnd_ignore_cid = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"cid", ST_Ignore_Caller_ID_Name, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 193 ) / 2 ), 205, 193, NULL, NULL, NULL, ( LPVOID )0 );
			}

			HWND c_hWnd = GetCurrentListView();

			unsigned char add_ignore_type = ( c_hWnd == g_hWnd_allow_cid_list ? 0x04 : 0x00 );

			if ( LOWORD( wParam ) == MENU_ADD_ALLOW_IGNORE_CID_LIST )
			{
				_SendMessageW( g_hWnd_ignore_cid, WM_PROPAGATE, 0x01 | add_ignore_type, 0 );

				_SetForegroundWindow( g_hWnd_ignore_cid );
			}
			else// if ( LOWORD( wParam ) == MENU_EDIT_ALLOW_IGNORE_CID_LIST )
			{
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM;
				lvi.iItem = ( int )_SendMessageW( c_hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

				if ( lvi.iItem != -1 )
				{
					_SendMessageW( c_hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );
					_SendMessageW( g_hWnd_ignore_cid, WM_PROPAGATE, 0x02 | add_ignore_type, lvi.lParam );

					_SetForegroundWindow( g_hWnd_ignore_cid );
				}
			}
		}
		break;

		case MENU_ALLOW_LIST:
		case MENU_IGNORE_LIST:
		case MENU_ALLOW_CID_LIST:
		case MENU_IGNORE_CID_LIST:	// call log listview right click.
		{
			// Retrieve the lParam value from the selected listview item.
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				if ( lvi.lParam != NULL )
				{
					bool has_match;

					if ( LOWORD( wParam ) == MENU_ALLOW_LIST || LOWORD( wParam ) == MENU_IGNORE_LIST )
					{
						has_match = ( LOWORD( wParam ) == MENU_ALLOW_LIST ? ( ( display_info * )lvi.lParam )->allow_phone_number :
																			( ( display_info * )lvi.lParam )->ignore_phone_number );

						if ( has_match )
						{
							if ( _MessageBoxW( hWnd, ( LOWORD( wParam ) == MENU_ALLOW_LIST ? ST_PROMPT_remove_entries_apn : ST_PROMPT_remove_entries_ipn ), PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
							{
								allow_ignore_update_info *aiui = ( allow_ignore_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_update_info ) );
								aiui->action = 1;	// 1 = Remove, 0 = Add
								aiui->list_type = ( LOWORD( wParam ) == MENU_ALLOW_LIST ? LIST_TYPE_ALLOW : LIST_TYPE_IGNORE );
								aiui->hWnd = g_hWnd_call_log;

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
						}
						else	// Add items to the allow/ignore list.
						{
							allow_ignore_update_info *aiui = ( allow_ignore_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_update_info ) );
							aiui->action = 0;	// 1 = Remove, 0 = Add
							aiui->list_type = ( LOWORD( wParam ) == MENU_ALLOW_LIST ? LIST_TYPE_ALLOW : LIST_TYPE_IGNORE );
							aiui->hWnd = g_hWnd_call_log;

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
					}
					else if ( LOWORD( wParam ) == MENU_ALLOW_CID_LIST || LOWORD( wParam ) == MENU_IGNORE_CID_LIST )
					{
						if ( LOWORD( wParam ) == MENU_ALLOW_CID_LIST )
						{
							has_match = ( ( ( display_info * )lvi.lParam )->allow_cid_match_count > 0 ? true : false );
						}
						else
						{
							has_match = ( ( ( display_info * )lvi.lParam )->ignore_cid_match_count > 0 ? true : false );
						}

						if ( has_match )
						{
							int mb_ret;

							if ( LOWORD( wParam ) == MENU_ALLOW_CID_LIST )
							{
								mb_ret = _MessageBoxW( hWnd, ST_PROMPT_remove_entries_acid, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO );
							}
							else
							{
								mb_ret = _MessageBoxW( hWnd, ST_PROMPT_remove_entries_icid, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO );
							}

							if ( mb_ret == IDYES )
							{
								allow_ignore_cid_update_info *aicidui = ( allow_ignore_cid_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_update_info ) );
								aicidui->action = 1;	// 1 = Remove, 0 = Add
								aicidui->list_type = ( LOWORD( wParam ) == MENU_ALLOW_CID_LIST ? LIST_TYPE_ALLOW : LIST_TYPE_IGNORE );
								aicidui->hWnd = g_hWnd_call_log;

								// Remove items.
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
						}
						else	// The item we've selected is not in the allow/ignorecidlist tree. Prompt the user.
						{
							if ( g_hWnd_ignore_cid == NULL )
							{
								g_hWnd_ignore_cid = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"cid", ST_Ignore_Caller_ID_Name, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 193 ) / 2 ), 205, 193, NULL, NULL, NULL, ( LPVOID )0 );
							}

							_SendMessageW( g_hWnd_ignore_cid, WM_PROPAGATE, 0x01 | ( LOWORD( wParam ) == MENU_ALLOW_CID_LIST ? 0x04 : 0x00 ) | ( _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 ) > 1 ? 0x08 : 0x00 ), lvi.lParam );

							_SetForegroundWindow( g_hWnd_ignore_cid );
						}
					}
				}
			}
		}
		break;

		case MENU_COLUMN_1:
		case MENU_COLUMN_2:
		case MENU_COLUMN_3:
		case MENU_COLUMN_4:
		case MENU_COLUMN_5:
		case MENU_COLUMN_6:
		case MENU_COLUMN_7:
		case MENU_COLUMN_8:
		case MENU_COLUMN_9:
		case MENU_COLUMN_10:
		case MENU_COLUMN_11:
		case MENU_COLUMN_12:
		case MENU_COLUMN_13:
		case MENU_COLUMN_14:
		case MENU_COLUMN_15:
		case MENU_COLUMN_16:
		case MENU_COLUMN_17:
		{
			HWND c_hWnd = GetCurrentListView();
			if ( c_hWnd != NULL )
			{
				UpdateColumns( c_hWnd, LOWORD( wParam ) );
			}
		}
		break;

		case MENU_EDIT_CONTACT:
		case MENU_ADD_CONTACT:
		{
			if ( g_hWnd_contact == NULL )
			{
				g_hWnd_contact = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"contact", ST_Contact_Information, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 550 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 344 ) / 2 ), 550, 344, NULL, NULL, NULL, NULL );
			}

			if ( LOWORD( wParam ) == MENU_EDIT_CONTACT )
			{
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM;
				lvi.iItem = ( int )_SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

				if ( lvi.iItem != -1 )
				{
					_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					_SendMessageW( g_hWnd_contact, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 0 ), lvi.lParam );	// Edit contact.
				}
			}
			else
			{
				_SendMessageW( g_hWnd_contact, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 0 ), 0 );	// Add contact.
			}

			_SetForegroundWindow( g_hWnd_contact );
		}
		break;

		case MENU_MESSAGE_LOG:
		{
			if ( _IsWindowVisible( g_hWnd_message_log ) == TRUE )
			{
				_SetForegroundWindow( g_hWnd_message_log );
			}
			else
			{
				ShowWindow( g_hWnd_message_log, SW_SHOWNORMAL );
			}
		}
		break;

		case MENU_SAVE_CALL_LOG:
		{
			wchar_t *file_path = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"CSV (Comma delimited) (*.csv)\0*.csv\0";
			ofn.lpstrDefExt = L"csv";
			ofn.lpstrTitle = ST_Save_Call_Log;
			ofn.lpstrFile = file_path;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

			if ( _GetSaveFileNameW( &ofn ) )
			{
				// file_path will be freed in the create_call_log_csv_file thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, create_call_log_csv_file, ( void * )file_path, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( file_path );
				}
			}
			else
			{
				GlobalFree( file_path );
			}
		}
		break;

		case MENU_EXPORT_LIST:
		{
			wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"Allow Caller ID Name List\0*.*\0" \
							  L"Allow Phone Number List\0*.*\0" \
							  L"Call Log History\0*.*\0" \
							  L"Contact List\0*.*\0" \
							  L"Ignore Caller ID Name List\0*.*\0" \
							  L"Ignore Phone Number List\0*.*\0";
			//ofn.lpstrDefExt = L"txt";
			ofn.lpstrTitle = ST_Export;
			ofn.lpstrFile = file_name;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

			HWND c_hWnd = GetCurrentListView();
			if ( c_hWnd != NULL )
			{
				if		( c_hWnd == g_hWnd_allow_cid_list )		{ ofn.nFilterIndex = 1; }
				else if ( c_hWnd == g_hWnd_allow_list )			{ ofn.nFilterIndex = 2; }
				else if ( c_hWnd == g_hWnd_call_log )			{ ofn.nFilterIndex = 3; }
				else if ( c_hWnd == g_hWnd_contact_list )		{ ofn.nFilterIndex = 4; }
				else if ( c_hWnd == g_hWnd_ignore_cid_list )	{ ofn.nFilterIndex = 5; }
				else if ( c_hWnd == g_hWnd_ignore_list )		{ ofn.nFilterIndex = 6; }
			}

			if ( _GetSaveFileNameW( &ofn ) )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
				iei->file_paths = file_name;
				iei->file_type = ( unsigned char )( 6 - ofn.nFilterIndex );

				// iei will be freed in the export_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, export_list, ( void * )iei, 0, NULL );
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
			else
			{
				GlobalFree( file_name );
			}
		}
		break;

		case MENU_IMPORT_LIST:
		{
			wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * ( MAX_PATH * MAX_PATH ) );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"Allow Caller ID Name List\0*.*\0" \
							  L"Allow Phone Number List\0*.*\0" \
							  L"Call Log History\0*.*\0" \
							  L"Contact List\0*.*\0" \
							  L"Ignore Caller ID Name List\0*.*\0" \
							  L"Ignore Phone Number List\0*.*\0";
			ofn.lpstrTitle = ST_Import;
			ofn.lpstrFile = file_name;
			ofn.nMaxFile = MAX_PATH * MAX_PATH;
			ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY;

			HWND c_hWnd = GetCurrentListView();
			if ( c_hWnd != NULL )
			{
				if		( c_hWnd == g_hWnd_allow_cid_list )		{ ofn.nFilterIndex = 1; }
				else if ( c_hWnd == g_hWnd_allow_list )			{ ofn.nFilterIndex = 2; }
				else if ( c_hWnd == g_hWnd_call_log )			{ ofn.nFilterIndex = 3; }
				else if ( c_hWnd == g_hWnd_contact_list )		{ ofn.nFilterIndex = 4; }
				else if ( c_hWnd == g_hWnd_ignore_cid_list )	{ ofn.nFilterIndex = 5; }
				else if ( c_hWnd == g_hWnd_ignore_list )		{ ofn.nFilterIndex = 6; }
			}

			if ( _GetOpenFileNameW( &ofn ) )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
				iei->file_paths = file_name;
				iei->file_offset = ofn.nFileOffset;
				iei->file_type = ( unsigned char )( 6 - ofn.nFilterIndex );

				// iei will be freed in the import_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL );
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
			else
			{
				GlobalFree( file_name );
			}
		}
		break;

		case MENU_SEARCH:
		{
			if ( g_hWnd_search == NULL )
			{
				g_hWnd_search = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"search", ST_Search, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 320 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 210 ) / 2 ), 320, 210, NULL, NULL, NULL, NULL );
			}
			_SendMessageW( g_hWnd_search, WM_PROPAGATE, 0, 0 );
		}
		break;

		case MENU_OPTIONS:
		{
			if ( g_hWnd_options == NULL )
			{
				g_hWnd_options = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"options", ST_Options, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 470 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 395 ) / 2 ), 470, 395, NULL, NULL, NULL, NULL );
				_ShowWindow( g_hWnd_options, SW_SHOWNORMAL );
			}
			_SetForegroundWindow( g_hWnd_options );
		}
		break;

		case MENU_VZ_ENHANCED_HOME_PAGE:
		{
			bool destroy = true;
			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					destroy = InitializeOle32();
				}
			#endif

			if ( destroy )
			{
				_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
			}

			_ShellExecuteW( NULL, L"open", HOME_PAGE, NULL, NULL, SW_SHOWNORMAL );

			if ( destroy )
			{
				_CoUninitialize();
			}
		}
		break;

		case MENU_CHECK_FOR_UPDATES:
		{
			update_check_state = 1;	// Manual update check.

			if ( g_hWnd_update == NULL )
			{
				g_hWnd_update = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"update", ST_Checking_For_Updates___, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 510 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 240 ) / 2 ), 510, 240, NULL, NULL, NULL, NULL );

				UPDATE_CHECK_INFO *update_info = ( UPDATE_CHECK_INFO * )GlobalAlloc( GPTR, sizeof( UPDATE_CHECK_INFO ) );
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, ( void * )update_info, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( update_info );
				}
			}

			_ShowWindow( g_hWnd_update, SW_SHOWNORMAL );
			_SetForegroundWindow( g_hWnd_update );
		}
		break;

		case MENU_ABOUT:
		{
			wchar_t msg[ 512 ];
			__snwprintf( msg, 512, L"VZ Enhanced 56K is made free under the GPLv3 license.\r\n\r\n" \
								   L"Version 1.0.0.6 (%u-bit)\r\n\r\n" \
								   L"Built on %s, %s %d, %04d %d:%02d:%02d %s (UTC)\r\n\r\n" \
								   L"Copyright \xA9 2013-2019 Eric Kutcher",
#ifdef _WIN64
								   64,
#else
								   32,
#endif
								   GetDay( g_compile_time.wDayOfWeek ),
								   GetMonth( g_compile_time.wMonth ),
								   g_compile_time.wDay,
								   g_compile_time.wYear,
								   ( g_compile_time.wHour > 12 ? g_compile_time.wHour - 12 : ( g_compile_time.wHour != 0 ? g_compile_time.wHour : 12 ) ),
								   g_compile_time.wMinute,
								   g_compile_time.wSecond,
								   ( g_compile_time.wHour >= 12 ? L"PM" : L"AM" ) );

			_MessageBoxW( hWnd, msg, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
		}
		break;

		case MENU_RESTORE:
		{
			if ( _IsIconic( hWnd ) )	// If minimized, then restore the window.
			{
				_ShowWindow( hWnd, SW_RESTORE );
			}
			else if ( _IsWindowVisible( hWnd ) == TRUE )	// If already visible, then flash the window.
			{
				_FlashWindow( hWnd, TRUE );
			}
			else	// If hidden, then show the window.
			{
				_ShowWindow( hWnd, SW_SHOW );
			}
		}
		break;

		case MENU_EXIT:
		{
			_SendMessageW( hWnd, WM_EXIT, 0, 0 );
		}
		break;
	}
}

LRESULT CALLBACK MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			g_hWnd_tab = _CreateWindowW( WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			// Create our listview windows.
			g_hWnd_call_log = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, 0, 0, g_hWnd_tab, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_call_log, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_contact_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, 0, 0, g_hWnd_tab, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_contact_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_ignore_tab = _CreateWindowW( WC_TABCONTROL, NULL, WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0, g_hWnd_tab, NULL, NULL, NULL );

			g_hWnd_ignore_cid_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW | WS_VISIBLE, 0, 0, 0, 0, g_hWnd_ignore_tab, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_ignore_cid_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_ignore_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, 0, 0, g_hWnd_ignore_tab, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_ignore_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_allow_tab = _CreateWindowW( WC_TABCONTROL, NULL, WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0, g_hWnd_tab, NULL, NULL, NULL );

			g_hWnd_allow_cid_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW | WS_VISIBLE, 0, 0, 0, 0, g_hWnd_allow_tab, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_allow_cid_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_allow_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, 0, 0, g_hWnd_allow_tab, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_allow_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			_SendMessageW( g_hWnd_tab, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_call_log, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_contact_list, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ignore_tab, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ignore_cid_list, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ignore_list, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_allow_tab, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_allow_cid_list, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_allow_list, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Allow drag and drop for the listview.
			_DragAcceptFiles( g_hWnd_call_log, TRUE );
			_DragAcceptFiles( g_hWnd_contact_list, TRUE );
			_DragAcceptFiles( g_hWnd_ignore_cid_list, TRUE );
			_DragAcceptFiles( g_hWnd_ignore_list, TRUE );
			_DragAcceptFiles( g_hWnd_allow_cid_list, TRUE );
			_DragAcceptFiles( g_hWnd_allow_list, TRUE );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;			// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_Caller_ID_Names;		// This will simply set the width of each tab item. We're not going to use it.
			ti.lParam = ( LPARAM )g_hWnd_ignore_cid_list;
			_SendMessageW( g_hWnd_ignore_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.lParam = ( LPARAM )g_hWnd_allow_cid_list;
			_SendMessageW( g_hWnd_allow_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Phone_Numbers;	// This will simply set the width of each tab item. We're not going to use it.
			ti.lParam = ( LPARAM )g_hWnd_ignore_list;
			_SendMessageW( g_hWnd_ignore_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.lParam = ( LPARAM )g_hWnd_allow_list;
			_SendMessageW( g_hWnd_allow_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			// Handles drag and drop in the listviews.
			ListsProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_call_log, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_call_log, GWLP_WNDPROC, ( LONG_PTR )ListsSubProc );
			_SetWindowLongPtrW( g_hWnd_contact_list, GWLP_WNDPROC, ( LONG_PTR )ListsSubProc );
			_SetWindowLongPtrW( g_hWnd_ignore_cid_list, GWLP_WNDPROC, ( LONG_PTR )ListsSubProc );
			_SetWindowLongPtrW( g_hWnd_ignore_list, GWLP_WNDPROC, ( LONG_PTR )ListsSubProc );
			_SetWindowLongPtrW( g_hWnd_allow_cid_list, GWLP_WNDPROC, ( LONG_PTR )ListsSubProc );
			_SetWindowLongPtrW( g_hWnd_allow_list, GWLP_WNDPROC, ( LONG_PTR )ListsSubProc );

			// Handles the drawing of the listviews in the tab controls.
			TabListsProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_tab, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_tab, GWLP_WNDPROC, ( LONG_PTR )TabListsSubProc );
			_SetWindowLongPtrW( g_hWnd_ignore_tab, GWLP_WNDPROC, ( LONG_PTR )TabListsSubProc );
			_SetWindowLongPtrW( g_hWnd_allow_tab, GWLP_WNDPROC, ( LONG_PTR )TabListsSubProc );

			// Handles the drawing of the main tab control.
			TabProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_tab, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_tab, GWLP_WNDPROC, ( LONG_PTR )TabSubProc );

			_SendMessageW( g_hWnd_tab, WM_PROPAGATE, 1, 0 );	// Open theme data. Must be done after we subclass the control. Theme object will be closed when the tab control is destroyed.

			for ( unsigned char i = 0; i < NUM_TABS; ++i )
			{
				if ( i == cfg_tab_order1 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_allow_tab, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Allow_Lists;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_allow_tab;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, g_total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order2 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_call_log, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Call_Log;		// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_call_log;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, g_total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order3 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_contact_list, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Contact_List;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_contact_list;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, g_total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order4 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_ignore_tab, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Ignore_Lists;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_ignore_tab;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, g_total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
			}

			if ( g_total_tabs == 0 )
			{
				_ShowWindow( g_hWnd_tab, SW_HIDE );
			}

			int arr[ 17 ];

			// Initialize our listview columns
			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_WIDTH | LVCF_TEXT;

			for ( char i = 0; i < NUM_COLUMNS1; ++i )
			{
				if ( *allow_list_columns[ i ] != -1 )
				{
					lvc.pszText = allow_ignore_list_string_table[ i ];
					lvc.cx = *allow_list_columns_width[ i ];
					_SendMessageW( g_hWnd_allow_list, LVM_INSERTCOLUMN, g_total_columns1, ( LPARAM )&lvc );

					arr[ g_total_columns1++ ] = *allow_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_allow_list, LVM_SETCOLUMNORDERARRAY, g_total_columns1, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS2; ++i )
			{
				if ( *allow_cid_list_columns[ i ] != -1 )
				{
					lvc.pszText = allow_ignore_cid_list_string_table[ i ];
					lvc.cx = *allow_cid_list_columns_width[ i ];
					_SendMessageW( g_hWnd_allow_cid_list, LVM_INSERTCOLUMN, g_total_columns2, ( LPARAM )&lvc );

					arr[ g_total_columns2++ ] = *allow_cid_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_allow_cid_list, LVM_SETCOLUMNORDERARRAY, g_total_columns2, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS3; ++i )
			{
				if ( *call_log_columns[ i ] != -1 )
				{
					lvc.pszText = call_log_string_table[ i ];
					lvc.cx = *call_log_columns_width[ i ];
					_SendMessageW( g_hWnd_call_log, LVM_INSERTCOLUMN, g_total_columns3, ( LPARAM )&lvc );

					arr[ g_total_columns3++ ] = *call_log_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_call_log, LVM_SETCOLUMNORDERARRAY, g_total_columns3, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS4; ++i )
			{
				if ( *contact_list_columns[ i ] != -1 )
				{
					lvc.pszText = contact_list_string_table[ i ];
					lvc.cx = *contact_list_columns_width[ i ];
					_SendMessageW( g_hWnd_contact_list, LVM_INSERTCOLUMN, g_total_columns4, ( LPARAM )&lvc );

					arr[ g_total_columns4++ ] = *contact_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_contact_list, LVM_SETCOLUMNORDERARRAY, g_total_columns4, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS5; ++i )
			{
				if ( *ignore_list_columns[ i ] != -1 )
				{
					lvc.pszText = allow_ignore_list_string_table[ i ];
					lvc.cx = *ignore_list_columns_width[ i ];
					_SendMessageW( g_hWnd_ignore_list, LVM_INSERTCOLUMN, g_total_columns5, ( LPARAM )&lvc );

					arr[ g_total_columns5++ ] = *ignore_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_ignore_list, LVM_SETCOLUMNORDERARRAY, g_total_columns5, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS6; ++i )
			{
				if ( *ignore_cid_list_columns[ i ] != -1 )
				{
					lvc.pszText = allow_ignore_cid_list_string_table[ i ];
					lvc.cx = *ignore_cid_list_columns_width[ i ];
					_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTCOLUMN, g_total_columns6, ( LPARAM )&lvc );

					arr[ g_total_columns6++ ] = *ignore_cid_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_ignore_cid_list, LVM_SETCOLUMNORDERARRAY, g_total_columns6, ( LPARAM )arr );

			if ( cfg_tray_icon )
			{
				_memzero( &g_nid, sizeof( NOTIFYICONDATA ) );
				g_nid.cbSize = NOTIFYICONDATA_V2_SIZE;	// 5.0 (Windows 2000) and newer.
				g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				g_nid.hWnd = hWnd;
				g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
				g_nid.uID = 1000;
				g_nid.hIcon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
				_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ), L"VZ Enhanced 56K\0", 16 );
				g_nid.szTip[ 15 ] = 0;	// Sanity.
				_Shell_NotifyIconW( NIM_ADD, &g_nid );
			}

			g_hWnd_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWnd_call_log, NULL, NULL, NULL );

			TOOLINFO tti;
			_memzero( &tti, sizeof( TOOLINFO ) );
			tti.cbSize = sizeof( TOOLINFO );
			tti.uFlags = TTF_SUBCLASS;
			tti.hwnd = g_hWnd_call_log;

			_SendMessageW( g_hWnd_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );
			_SendMessageW( g_hWnd_tooltip, TTM_SETMAXTIPWIDTH, 0, sizeof( wchar_t ) * ( 2 * MAX_PATH ) );
			_SendMessageW( g_hWnd_tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767 );
			_SendMessageW( g_hWnd_tooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 2000 );

			_SendMessageW( g_hWnd_call_log, LVM_SETTOOLTIPS, ( WPARAM )g_hWnd_tooltip, 0 );

			// The custom caller ID shouldn't be larger than 257 characters.
			tooltip_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 512 );

			// Create the menus after we've gotten the total active columns (total_columns1, 2, etc.).
			CreateMenus();

			// Set our menu bar.
			_SetMenu( hWnd, g_hMenu );


			//////////////////////////////

			LoadLists();

			if ( cfg_check_for_updates )
			{
				// Check after 10 seconds.
				_SetTimer( hWnd, IDT_UPDATE_TIMER, 10000, ( TIMERPROC )UpdateTimerProc );
			}

			// Automatically save our call log and lists after 12 hours.
			_SetTimer( hWnd, IDT_SAVE_TIMER, 43200000, ( TIMERPROC )SaveTimerProc );

			return 0;
		}
		break;

		case WM_KEYDOWN:
		{
			// We'll just give the listview control focus since it's handling our keypress events.
			_SetFocus( GetCurrentListView() );

			return 0;
		}
		break;

		/////////////////////////////////////
		#ifdef USE_DEBUG_DIRECTORY
		/////////////////////////////////////
		case WM_MBUTTONUP:
		{
			SYSTEMTIME SystemTime;
			FILETIME FileTime;

			GetLocalTime( &SystemTime );
			SystemTimeToFileTime( &SystemTime, &FileTime );

			display_info *di = ( display_info * )GlobalAlloc( GPTR, sizeof( display_info ) );

			di->caller_id = GlobalStrDupW( L"Caller ID" );
			di->phone_number = GlobalStrDupW( L"8181239999" );

			di->time.LowPart = FileTime.dwLowDateTime;
			di->time.HighPart = FileTime.dwHighDateTime;
			di->process_incoming = true;

			// This will also ignore the call if it's in our lists.
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_call_log, ( void * )di, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
			else
			{
				// This is all that's set above.
				GlobalFree( di->phone_number );
				GlobalFree( di->caller_id );
				GlobalFree( di );
			}

			return 0;
		}
		break;
		/////////////////////////////////////
		#endif
		/////////////////////////////////////

		case WM_WINDOWPOSCHANGED:
		{
			// Only handle main window changes.
			if ( hWnd == g_hWnd_main )
			{
				// Don't want to save minimized and maximized size and position values.
				if ( _IsIconic( hWnd ) == TRUE )
				{
					cfg_min_max = 1;
				}
				else if ( _IsZoomed( hWnd ) == TRUE )
				{
					cfg_min_max = 2;
				}
				else
				{
					// This will capture MoveWindow and SetWindowPos changes.
					WINDOWPOS *wp = ( WINDOWPOS * )lParam;

					if ( !( wp->flags & SWP_NOMOVE ) )
					{
						cfg_pos_x = wp->x;
						cfg_pos_y = wp->y;
					}

					if ( !( wp->flags & SWP_NOSIZE ) )
					{
						cfg_width = wp->cx;
						cfg_height = wp->cy;
					}

					cfg_min_max = 0;
				}
			}

			// Let it fall through so we can still get the WM_SIZE message.
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_MOVING:
		{
			POINT cur_pos;
			RECT wa;
			RECT *rc = ( RECT * )lParam;

			_GetCursorPos( &cur_pos );
			_OffsetRect( rc, cur_pos.x - ( rc->left + cx ), cur_pos.y - ( rc->top + cy ) );

			// Allow our main window to attach to the desktop edge.
			_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );			
			if ( is_close( rc->left, wa.left ) )				// Attach to left side of the desktop.
			{
				_OffsetRect( rc, wa.left - rc->left, 0 );
			}
			else if ( is_close( wa.right, rc->right ) )		// Attach to right side of the desktop.
			{
				_OffsetRect( rc, wa.right - rc->right, 0 );
			}

			if ( is_close( rc->top, wa.top ) )				// Attach to top of the desktop.
			{
				_OffsetRect( rc, 0, wa.top - rc->top );
			}
			else if ( is_close( wa.bottom, rc->bottom ) )	// Attach to bottom of the desktop.
			{
				_OffsetRect( rc, 0, wa.bottom - rc->bottom );
			}

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;
			cfg_width = rc->right - rc->left;
			cfg_height = rc->bottom - rc->top;

			return TRUE;
		}
		break;

		case WM_ENTERMENULOOP:
		{
			// If we've clicked the menu bar.
			if ( ( BOOL )wParam == FALSE )
			{
				// And a context menu was open, then revert the context menu additions.
				if ( last_menu )
				{
					UpdateMenus( UM_ENABLE );

					last_menu = false;	// Prevent us from calling UpdateMenus again.
				}

				// Allow us to save the call log if there are any entries in the call log listview.
				_EnableMenuItem( g_hMenu, MENU_SAVE_CALL_LOG, ( _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 ) > 0 ? MF_ENABLED : MF_DISABLED ) );
			}
			else if ( ( BOOL )wParam == TRUE )
			{
				last_menu = true;	// The last menu to be open was a context menu.
			}

			return 0;
		}
		break;

		case WM_ENTERSIZEMOVE:
		{
			//Get the current position of our window before it gets moved.
			POINT cur_pos;
			RECT rc;
			_GetWindowRect( hWnd, &rc );
			_GetCursorPos( &cur_pos );
			cx = cur_pos.x - rc.left;
			cy = cur_pos.y - rc.top;

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc, rc_tab;
			_GetClientRect( hWnd, &rc );

			// Calculate the display rectangle, assuming the tab control is the size of the client area.
			_SetRect( &rc_tab, 0, 0, rc.right, rc.bottom );
			_SendMessageW( g_hWnd_tab, TCM_ADJUSTRECT, FALSE, ( LPARAM )&rc_tab );

			_SetWindowPos( g_hWnd_tab, NULL, 1, 0, rc.right, rc.bottom, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_call_log, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_contact_list, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );

			_SetWindowPos( g_hWnd_allow_tab, NULL, 8, 8 + rc_tab.top, rc.right - rc.left - 16, rc.bottom - rc_tab.top - 16, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ignore_tab, NULL, 8, 8 + rc_tab.top, rc.right - rc.left - 16, rc.bottom - rc_tab.top - 16, SWP_NOZORDER );

			_GetClientRect( g_hWnd_allow_tab, &rc );
			_SetRect( &rc_tab, 0, 0, rc.right, rc.bottom );
			_SendMessageW( g_hWnd_allow_tab, TCM_ADJUSTRECT, FALSE, ( LPARAM )&rc_tab );

			_SetWindowPos( g_hWnd_ignore_cid_list, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ignore_list, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );

			_SetWindowPos( g_hWnd_allow_cid_list, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_allow_list, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );

			return 0;
		}
		break;

		case WM_SIZING:
		{
			RECT *rc = ( RECT * )lParam;

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;
			cfg_width = rc->right - rc->left;
			cfg_height = rc->bottom - rc->top;
		}
		break;

		case WM_CHANGE_CURSOR:
		{
			// SetCursor must be called from the window thread.
			if ( wParam == TRUE )
			{
				wait_cursor = _LoadCursorW( NULL, IDC_APPSTARTING );	// Arrow + hourglass.
				_SetCursor( wait_cursor );
			}
			else
			{
				_SetCursor( _LoadCursorW( NULL, IDC_ARROW ) );	// Default arrow.
				wait_cursor = NULL;
			}
		}
		break;

		case WM_SETCURSOR:
		{
			if ( wait_cursor != NULL )
			{
				_SetCursor( wait_cursor );	// Keep setting our cursor if it reverts back to the default.
				return TRUE;
			}

			_DefWindowProcW( hWnd, msg, wParam, lParam );
			return FALSE;
		}
		break;

		case WM_COMMAND:
		{
			HandleCommand( hWnd, wParam, lParam );

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = ( int )_SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( ( HWND )( tie.lParam ), SW_HIDE );
					}
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
                {
					NMHDR *nmhdr = ( NMHDR * )lParam;

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = ( int )_SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information

						_ShowWindow( ( HWND )tie.lParam, SW_SHOW );

						if ( ( HWND )tie.lParam == g_hWnd_allow_tab || ( HWND )tie.lParam == g_hWnd_ignore_tab )
						{
							index = ( int )_SendMessageW( ( HWND )tie.lParam, TCM_GETCURSEL, 0, 0 );	// Get the selected tab
							if ( index != -1 )
							{
								_SendMessageW( ( HWND )tie.lParam, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
							}
						}

						_InvalidateRect( nmhdr->hwndFrom, NULL, TRUE );	// Repaint the control

						ChangeMenuByHWND( ( HWND )tie.lParam );

						_SetFocus( ( HWND )tie.lParam );	// Allows us to scroll the listview.

						UpdateMenus( UM_ENABLE );
					}
                }
				break;

				case HDN_ENDDRAG:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;
					HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );

					// Prevent the # columns from moving and the other columns from becoming the first column.
					if ( nmh->iItem == 0 || nmh->pitem->iOrder == 0 )
					{
						// Make sure the # columns are visible.
						if ( hWnd_parent == g_hWnd_allow_list && *allow_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, allow_list_columns, NUM_COLUMNS1 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_allow_cid_list && *allow_cid_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, allow_cid_list_columns, NUM_COLUMNS2 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_call_log && *call_log_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, call_log_columns, NUM_COLUMNS3 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_contact_list && *contact_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, contact_list_columns, NUM_COLUMNS4 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_ignore_list && *ignore_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, ignore_list_columns, NUM_COLUMNS5 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_ignore_cid_list && *ignore_cid_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, ignore_cid_list_columns, NUM_COLUMNS6 );
							return TRUE;
						}
					}
				}
				break;

				case HDN_ENDTRACK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;
					HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );

					int index = nmh->iItem;

					if ( hWnd_parent == g_hWnd_allow_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, allow_list_columns, NUM_COLUMNS1 );

						if ( index != -1 )
						{
							*allow_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_allow_cid_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, allow_cid_list_columns, NUM_COLUMNS2 );

						if ( index != -1 )
						{
							*allow_cid_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_call_log )
					{
						index = GetVirtualIndexFromColumnIndex( index, call_log_columns, NUM_COLUMNS3 );

						if ( index != -1 )
						{
							*call_log_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_contact_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, contact_list_columns, NUM_COLUMNS4 );

						if ( index != -1 )
						{
							*contact_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_ignore_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, ignore_list_columns, NUM_COLUMNS5 );

						if ( index != -1 )
						{
							*ignore_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_ignore_cid_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, ignore_cid_list_columns, NUM_COLUMNS6 );

						if ( index != -1 )
						{
							*ignore_cid_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
					if ( _GetKeyState( VK_CONTROL ) & 0x8000 && !in_worker_thread )
					{
						NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

						// Determine which key was pressed.
						switch ( ( ( LPNMLVKEYDOWN )lParam )->wVKey )
						{
							case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_SELECT_ALL, 0 );
								}
							}
							break;

							case 'C':	// Copy selected items if Ctrl + C is down and there are selected items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_COPY_SEL, 0 );
								}
							}
							break;

							case 'S':	// Save Call Log items if Ctrl + S is down and there are items in the list.
							{
								if ( _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_SAVE_CALL_LOG, 0 );
								}
							}
							break;
						}
					}
				}
				break;

				case LVN_COLUMNCLICK:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					LVCOLUMN lvc;
					_memzero( &lvc, sizeof( LVCOLUMN ) );
					lvc.mask = LVCF_FMT | LVCF_ORDER;
					_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

					sort_info si;
					si.column = lvc.iOrder;
					si.hWnd = nmlv->hdr.hwndFrom;

					if ( HDF_SORTUP & lvc.fmt )	// Column is sorted upward.
					{
						si.direction = 0;	// Now sort down.

						// Sort down
						lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, ( WPARAM )nmlv->iSubItem, ( LPARAM )&lvc );
					}
					else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
					{
						si.direction = 1;	// Now sort up.

						// Sort up
						lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
					}
					else	// Column has no sorting set.
					{
						unsigned char column_count = ( nmlv->hdr.hwndFrom == g_hWnd_allow_list ? g_total_columns1 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_allow_cid_list ? g_total_columns2 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_call_log ? g_total_columns3 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_contact_list ? g_total_columns4 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_ignore_list ? g_total_columns5 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_ignore_cid_list ? g_total_columns6 : 0 ) ) ) ) ) );

						// Remove the sort format for all columns.
						for ( unsigned char i = 0; i < column_count; ++i )
						{
							// Get the current format
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, i, ( LPARAM )&lvc );
							// Remove sort up and sort down
							lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, i, ( LPARAM )&lvc );
						}

						// Read current the format from the clicked column
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						si.direction = 0;	// Start the sort going down.

						// Sort down to start.
						lvc.fmt = lvc.fmt | HDF_SORTDOWN;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
					}

					_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CompareFunc );
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					HandleRightClick( nmitem->hdr.hwndFrom );
				}
				break;

				case NM_DBLCLK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_contact_list )
					{
						_SendMessageW( hWnd, WM_COMMAND, MENU_EDIT_CONTACT, 0 );
					}
					else if ( nmitem->hdr.hwndFrom == g_hWnd_allow_list || nmitem->hdr.hwndFrom == g_hWnd_ignore_list )
					{
						_SendMessageW( hWnd, WM_COMMAND, MENU_EDIT_ALLOW_IGNORE_LIST, 0 );
					}
					else if ( nmitem->hdr.hwndFrom == g_hWnd_allow_cid_list || nmitem->hdr.hwndFrom == g_hWnd_ignore_cid_list )
					{
						_SendMessageW( hWnd, WM_COMMAND, MENU_EDIT_ALLOW_IGNORE_CID_LIST, 0 );
					}
				}
				break;

				case LVN_DELETEITEM:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					// Item count will be 1 since the item hasn't yet been deleted.
					if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) == 1 )
					{
						// Disable the menus that require at least one item in the list.
						UpdateMenus( UM_DISABLE_OVERRIDE );
					}
				}
				break;

				case LVN_ITEMCHANGED:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					if ( !in_worker_thread )
					{
						UpdateMenus( ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 ? UM_ENABLE : UM_DISABLE ) );
					}
				}
				break;

				case LVN_HOTTRACK:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					if ( nmlv->hdr.hwndFrom == g_hWnd_call_log )
					{
						LVHITTESTINFO lvhti;
						_memzero( &lvhti, sizeof( LVHITTESTINFO ) );
						lvhti.pt = nmlv->ptAction;

						_SendMessageW( g_hWnd_call_log, LVM_HITTEST, 0, ( LPARAM )&lvhti );

						if ( lvhti.iItem != last_tooltip_item )
						{
							// Save the last item that was hovered so we don't have to keep calling everything below.
							last_tooltip_item = lvhti.iItem;

							TOOLINFO ti;
							_memzero( &ti, sizeof( TOOLINFO ) );
							ti.cbSize = sizeof( TOOLINFO );
							ti.hwnd = g_hWnd_call_log;

							_SendMessageW( g_hWnd_tooltip, TTM_GETTOOLINFO, 0, ( LPARAM )&ti );

							ti.lpszText = NULL;	// If we aren't hovered over an item or the download info is NULL, then we'll end up not showing the tooltip text.

							if ( lvhti.iItem != -1 )
							{
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM;
								lvi.iItem = lvhti.iItem;

								_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

								display_info *di = ( display_info * )lvi.lParam;

								if ( di != NULL )
								{
									if ( di->custom_caller_id != NULL )
									{
										__snwprintf( tooltip_buffer, 512, L"%s (%s)\r\n%s\r\n%s", di->custom_caller_id, di->caller_id, di->w_phone_number, di->w_time );
									}
									else
									{
										__snwprintf( tooltip_buffer, 512, L"%s\r\n%s\r\n%s", di->caller_id, di->w_phone_number, di->w_time );
									}

									ti.lpszText = tooltip_buffer;
								}
							}

							_SendMessageW( g_hWnd_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );
						}
					}
				}
				break;
			}
			return FALSE;
		}
		break;

		case WM_SHOWWINDOW:
		{
			if ( wParam == TRUE )
			{
				main_active = true;
			}
			return 0;
		}
		break;

		case WM_SYSCOMMAND:
		{
			if ( wParam == SC_MINIMIZE && cfg_tray_icon && cfg_minimize_to_tray )
			{
				_ShowWindow( hWnd, SW_HIDE );

				return 0;
			}

			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_PROPAGATE:
		{
			if ( LOWORD( wParam ) == CW_MODIFY && lParam != NULL )
			{
				// Insert a row into our listview.
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
				lvi.iItem = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
				lvi.iSubItem = 0;
				lvi.lParam = lParam;
				int index = ( int )_SendMessageW( g_hWnd_call_log, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

				// 0 = show popups, etc. 1 = don't show.
				if ( HIWORD( wParam ) == 0 )
				{
					// Work-around for misaligned listview rows.
					if ( main_active )
					{
						// Scroll to the newest entry.
						_SendMessageW( g_hWnd_call_log, LVM_ENSUREVISIBLE, index, FALSE );
					}

					if ( cfg_enable_popups )
					{
						CreatePopup( ( display_info * )lParam );
					}
				}
			}
		}
		break;

		case WM_ACTIVATE:
		{
			_SetFocus( GetCurrentListView() );	// Allows us to scroll the listview.
		}
		break;

		case WM_CLOSE:
		{
			if ( cfg_tray_icon && cfg_close_to_tray )
			{
				_ShowWindow( hWnd, SW_HIDE );
			}
			else
			{
				_SendMessageW( hWnd, WM_EXIT, 0, 0 );
			}
			return 0;
		}
		break;

		case WM_EXIT:
		{
			// Prevent the possibility of running additional processes.
			if ( g_hWnd_options != NULL )
			{
				_EnableWindow( g_hWnd_options, FALSE );
				_ShowWindow( g_hWnd_options, SW_HIDE );
			}
			if ( g_hWnd_search != NULL )
			{
				_EnableWindow( g_hWnd_search, FALSE );
				_ShowWindow( g_hWnd_search, SW_HIDE );
			}
			if ( g_hWnd_update != NULL )
			{
				_EnableWindow( g_hWnd_update, FALSE );
				_ShowWindow( g_hWnd_update, SW_HIDE );
			}
			if ( g_hWnd_contact != NULL )
			{
				_EnableWindow( g_hWnd_contact, FALSE );
				_ShowWindow( g_hWnd_contact, SW_HIDE );
			}
			if ( g_hWnd_ignore_phone_number != NULL )
			{
				_EnableWindow( g_hWnd_ignore_phone_number, FALSE );
				_ShowWindow( g_hWnd_ignore_phone_number, SW_HIDE );
			}
			if ( g_hWnd_ignore_cid != NULL )
			{
				_EnableWindow( g_hWnd_ignore_cid, FALSE );
				_ShowWindow( g_hWnd_ignore_cid, SW_HIDE );
			}
			if ( g_hWnd_message_log != NULL )
			{
				_EnableWindow( g_hWnd_message_log, FALSE );
				_ShowWindow( g_hWnd_message_log, SW_HIDE );
			}

			_EnableWindow( hWnd, FALSE );
			_ShowWindow( hWnd, SW_HIDE );

			if ( cfg_tray_icon )
			{
				// Remove the icon from the notification area.
				_Shell_NotifyIconW( NIM_DELETE, &g_nid );
			}

			// If we're in a secondary thread, then kill it (cleanly) and wait for it to exit.
			if ( in_worker_thread ||
				 in_telephony_thread ||
				 in_update_check_thread ||
				 in_ml_update_thread ||
				 in_ml_worker_thread ||
				 in_ringtone_update_thread )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, cleanup, ( void * )NULL, 0, NULL ) );
			}
			else	// Otherwise, destroy the window normally.
			{
				kill_telephony_thread_flag = 1;
				update_con.state = CONNECTION_KILL;
				kill_worker_thread_flag = true;
				kill_ml_update_thread_flag = true;
				kill_ml_worker_thread_flag = true;
				_SendMessageW( hWnd, WM_DESTROY_ALT, 0, 0 );
			}
		}
		break;

		case WM_DESTROY_ALT:
		{
			if ( g_hWnd_options != NULL )
			{
				_DestroyWindow( g_hWnd_options );
			}

			if ( g_hWnd_search != NULL )
			{
				_DestroyWindow( g_hWnd_search );
			}

			if ( g_hWnd_update != NULL )
			{
				_DestroyWindow( g_hWnd_update );
			}

			if ( g_hWnd_contact != NULL )
			{
				_DestroyWindow( g_hWnd_contact );
			}

			if ( g_hWnd_ignore_phone_number != NULL )
			{
				_DestroyWindow( g_hWnd_ignore_phone_number );
			}

			if ( g_hWnd_ignore_cid != NULL )
			{
				_DestroyWindow( g_hWnd_ignore_cid );
			}

			if ( g_hWnd_message_log != NULL )
			{
				_DestroyWindow( g_hWnd_message_log );
			}

			_KillTimer( hWnd, IDT_SAVE_TIMER );

			if ( cfg_enable_call_log_history && call_log_changed )
			{
				_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\call_log_history\0", 18 );
				base_directory[ base_directory_length + 17 ] = 0;	// Sanity.

				save_call_log_history( base_directory );
				call_log_changed = false;
			}

			// Get the number of items in the listview.
			int num_items = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			// Go through each item, and free their lParam values.
			for ( lvi.iItem = 0; lvi.iItem < num_items; ++lvi.iItem )
			{
				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				display_info *di = ( display_info * )lvi.lParam;
				if ( di != NULL )
				{
					free_displayinfo( &di );
				}
			}

			UpdateColumnOrders();

			DestroyMenus();

			if ( tooltip_buffer != NULL )
			{
				GlobalFree( tooltip_buffer );
				tooltip_buffer = NULL;
			}

			_DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			_PostQuitMessage( 0 );
			return 0;
		}
		break;

		case WM_TRAY_NOTIFY:
		{
			switch ( lParam )
			{
				case WM_LBUTTONDOWN:
				{
					if ( _IsWindowVisible( hWnd ) == FALSE )
					{
						_ShowWindow( hWnd, SW_SHOW );
						_SetForegroundWindow( hWnd );
					}
					else
					{
						_ShowWindow( hWnd, SW_HIDE );
					}
				}
				break;

				case WM_RBUTTONDOWN:
				case WM_CONTEXTMENU:
				{
					_SetForegroundWindow( hWnd );	// Must set so that the menu can close.

					// Show our edit context menu as a popup.
					POINT p;
					_GetCursorPos( &p ) ;
					_TrackPopupMenu( g_hMenuSub_tray, 0, p.x, p.y, 0, hWnd, NULL );
				}
				break;
			}
		}
		break;

		case WM_ENDSESSION:
		{
			// Save before we shut down/restart/log off of Windows.
			save_config();

			AutoSave( ( void * )1 );	// A non-zero parameter will prevent the function from calling ExitThread.

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return TRUE;
}
