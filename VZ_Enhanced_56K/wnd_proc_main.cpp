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
#include "wnd_subproc.h"
#include "connection.h"
#include "menus.h"
#include "utilities.h"
#include "ringtone_utilities.h"
#include "list_operations.h"
#include "file_operations.h"
#include "string_tables.h"
#include <commdlg.h>

#include "lite_gdi32.h"
#include "lite_comdlg32.h"
#include "lite_ole32.h"

#include "telephony.h"

// Object variables
HWND g_hWnd_columns = NULL;
HWND g_hWnd_options = NULL;
HWND g_hWnd_contact = NULL;
HWND g_hWnd_ignore_phone_number = NULL;
HWND g_hWnd_ignore_cid = NULL;
HWND g_hWnd_call_log = NULL;			// Handle to the call log listview control.
HWND g_hWnd_contact_list = NULL;
HWND g_hWnd_ignore_tab = NULL;
HWND g_hWnd_ignore_cid_list = NULL;
HWND g_hWnd_ignore_list = NULL;
HWND g_hWnd_edit = NULL;				// Handle to the listview edit control.
HWND g_hWnd_tab = NULL;
HWND g_hWnd_update = NULL;

WNDPROC ListsProc = NULL;
WNDPROC TabListsProc = NULL;

HCURSOR wait_cursor = NULL;				// Temporary cursor while processing entries.

NOTIFYICONDATA g_nid;					// Tray icon information.

// Window variables
int cx = 0;								// Current x (left) position of the main window based on the mouse.
int cy = 0;								// Current y (top) position of the main window based on the mouse.

unsigned char total_tabs = 0;

unsigned char total_columns1 = 0;
unsigned char total_columns2 = 0;
unsigned char total_columns3 = 0;
unsigned char total_columns4 = 0;

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
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, ( void * )update_info, 0, NULL ) );
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
	sortinfo *si = ( sortinfo * )lParamSort;

	int arr[ 17 ];

	if ( si->hWnd == g_hWnd_call_log )
	{
		displayinfo *fi1 = ( displayinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		displayinfo *fi2 = ( displayinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_call_log, LVM_GETCOLUMNORDERARRAY, total_columns1, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, call_log_columns, NUM_COLUMNS1, total_columns1 );

		switch ( arr[ si->column ] )
		{
			case 4:
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

			case 1: { return _stricmp_s( fi1->ci.caller_id, fi2->ci.caller_id ); } break;

			case 5: { return _wcsicmp_s( fi1->display_values[ arr[ si->column ] - 1 ], fi2->display_values[ arr[ si->column ] - 1 ] ); } break;

			case 2: { return ( fi1->time.QuadPart > fi2->time.QuadPart ); } break;
			case 3: { return ( fi1->ignore_cid_match_count > fi2->ignore_cid_match_count ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}
	else if ( si->hWnd == g_hWnd_contact_list )
	{
		contactinfo *fi1 = ( contactinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		contactinfo *fi2 = ( contactinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_contact_list, LVM_GETCOLUMNORDERARRAY, total_columns2, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, contact_list_columns, NUM_COLUMNS2, total_columns2 );

		switch ( arr[ si->column ] )
		{
			case 1:
			case 5:
			case 7:
			case 11:
			case 12:
			case 16: { return _wcsicmp_s( fi1->contactinfo_values[ arr[ si->column ] - 1 ], fi2->contactinfo_values[ arr[ si->column ] - 1 ] ); } break;
			
			case 2:
			case 3:
			case 4:
			case 6:
			case 8:
			case 9:
			case 10:
			case 13:
			case 14:
			case 15: { return _stricmp_s( fi1->contact.contact_values[ arr[ si->column ] - 1 ], fi2->contact.contact_values[ arr[ si->column ] - 1 ] ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}
	else if ( si->hWnd == g_hWnd_ignore_list )
	{
		ignoreinfo *fi1 = ( ignoreinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		ignoreinfo *fi2 = ( ignoreinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_ignore_list, LVM_GETCOLUMNORDERARRAY, total_columns3, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, ignore_list_columns, NUM_COLUMNS3, total_columns3 );

		switch ( arr[ si->column ] )
		{
			case 1: { return _wcsicmp_s( fi1->ignoreinfo_values[ arr[ si->column ] - 1 ], fi2->ignoreinfo_values[ arr[ si->column ] - 1 ] ); } break;
			case 2:	{ return ( fi1->count > fi2->count ); } break;

			default:
			{
				return 0;
			}
			break;
		}
	}
	else if ( si->hWnd == g_hWnd_ignore_cid_list )
	{
		ignorecidinfo *fi1 = ( ignorecidinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		ignorecidinfo *fi2 = ( ignorecidinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_ignore_cid_list, LVM_GETCOLUMNORDERARRAY, total_columns4, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, ignore_cid_list_columns, NUM_COLUMNS4, total_columns4 );

		switch ( arr[ si->column ] )
		{
			case 1: { return _stricmp_s( fi1->c_caller_id, fi2->c_caller_id ); } break;
			
			case 2:
			{
				if ( fi1->match_case == fi2->match_case )
				{
					return 0;
				}
				else if ( fi1->match_case && !fi2->match_case )
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
				if ( fi1->match_whole_word == fi2->match_whole_word )
				{
					return 0;
				}
				else if ( fi1->match_whole_word && !fi2->match_whole_word )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;
			
			case 4:	{ return ( fi1->count > fi2->count ); } break;

			default:
			{
				return 0;
			}
			break;
		}
	}

	return 0;
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
			else if ( hWnd == g_hWnd_ignore_cid_list )	{ iei->file_type = IE_IGNORE_CID_LIST; }
			else /*if ( hWnd == g_hWnd_ignore_list )*/	{ iei->file_type = IE_IGNORE_PN_LIST; }

			wchar_t file_path[ MAX_PATH ];

			int file_paths_offset = 0;	// Keeps track of the last file in filepath.
			int file_paths_length = ( MAX_PATH * count ) + 1;

			// Go through the list of paths.
			for ( int i = 0; i < count; ++i )
			{
				// Get the file path and its length.
				int file_path_length = _DragQueryFileW( ( HDROP )wParam, i, file_path, MAX_PATH ) + 1;	// Include the NULL terminator.

				// Skip any folders that were dropped.
				if ( ( GetFileAttributesW( file_path ) & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
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
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL ) );
			}
			else	// No files were dropped.
			{
				GlobalFree( iei );
			}

			return 0;
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
				if ( dis->itemID % 2 )	// Even rows will have a light grey background.
				{
					HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xF7, 0xF7, 0xF7 ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					if ( ( skip_log_draw && dis->hwndItem == g_hWnd_call_log ) ||
						 ( skip_contact_draw && dis->hwndItem == g_hWnd_contact_list ) ||
						 ( skip_ignore_draw && dis->hwndItem == g_hWnd_ignore_list ) ||
						 ( skip_ignore_cid_draw && dis->hwndItem == g_hWnd_ignore_cid_list ) )
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
				wchar_t *buf = tbuf;

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				int arr[ 17 ];
				int arr2[ 17 ];

				int column_count = 0;
				if ( dis->hwndItem == g_hWnd_call_log )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns1, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS1 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, call_log_columns, NUM_COLUMNS1, total_columns1 );

					column_count = total_columns1;
				}
				else if ( dis->hwndItem == g_hWnd_contact_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns2, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS2 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, contact_list_columns, NUM_COLUMNS2, total_columns2 );

					column_count = total_columns2;
				}
				else if ( dis->hwndItem == g_hWnd_ignore_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns3, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS3 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, ignore_list_columns, NUM_COLUMNS3, total_columns3 );

					column_count = total_columns3;
				}
				else if ( dis->hwndItem == g_hWnd_ignore_cid_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns4, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS4 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, ignore_cid_list_columns, NUM_COLUMNS4, total_columns4 );

					column_count = total_columns4;
				}

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i < column_count; ++i )
				{
					if ( dis->hwndItem == g_hWnd_call_log )
					{
						// Save the appropriate text in our buffer for the current column.
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, 11, L"%lu", dis->itemID + 1 );
							}
							break;
							case 1:
							case 2:
							case 3:
							case 4:
							case 5: { buf = ( ( displayinfo * )dis->itemData )->display_values[ arr2[ i ] - 1 ]; } break;
						}
					}
					else if ( dis->hwndItem == g_hWnd_contact_list )
					{
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, 11, L"%lu", dis->itemID + 1 );
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
								buf = ( ( contactinfo * )dis->itemData )->contactinfo_values[ arr2[ i ] - 1 ];
							}
							break;
						}
					}
					else if ( dis->hwndItem == g_hWnd_ignore_list )
					{
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, 11, L"%lu", dis->itemID + 1 );
							}
							break;
							case 1:
							case 2: { buf = ( ( ignoreinfo * )dis->itemData )->ignoreinfo_values[ arr2[ i ] - 1 ]; } break;
						}
					}
					else if ( dis->hwndItem == g_hWnd_ignore_cid_list )
					{
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, 11, L"%lu", dis->itemID + 1 );
							}
							break;
							case 1:
							case 2:
							case 3:
							case 4: { buf = ( ( ignorecidinfo * )dis->itemData )->ignorecidinfo_values[ arr2[ i ] - 1 ]; } break;
						}
					}

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
						_SetTextColor( hdcMem, RGB( 0xFF, 0xFF, 0xFF ) );
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
						if ( dis->hwndItem != g_hWnd_call_log )
						{
							_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );
						}
						else
						{
							_SetTextColor( hdcMem, ( ( displayinfo * )dis->itemData )->ci.ignored ? RGB( 0xFF, 0x00, 0x00 ) : RGB( 0x00, 0x00, 0x00 ) );
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

LRESULT CALLBACK MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			CreateMenus();

			// Set our menu bar.
			_SetMenu( hWnd, g_hMenu );

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

			_SendMessageW( g_hWnd_tab, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_call_log, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_contact_list, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ignore_tab, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ignore_cid_list, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ignore_list, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Allow drag and drop for the listview.
			_DragAcceptFiles( g_hWnd_call_log, TRUE );
			_DragAcceptFiles( g_hWnd_contact_list, TRUE );
			_DragAcceptFiles( g_hWnd_ignore_cid_list, TRUE );
			_DragAcceptFiles( g_hWnd_ignore_list, TRUE );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;			// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_Caller_ID_Names;		// This will simply set the width of each tab item. We're not going to use it.
			ti.lParam = ( LPARAM )g_hWnd_ignore_cid_list;
			_SendMessageW( g_hWnd_ignore_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Phone_Numbers;	// This will simply set the width of each tab item. We're not going to use it.
			ti.lParam = ( LPARAM )g_hWnd_ignore_list;
			_SendMessageW( g_hWnd_ignore_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			// Handles drag and drop in the listviews.
			ListsProc = ( WNDPROC )_GetWindowLongW( g_hWnd_call_log, GWL_WNDPROC );
			_SetWindowLongW( g_hWnd_call_log, GWL_WNDPROC, ( LONG )ListsSubProc );
			_SetWindowLongW( g_hWnd_contact_list, GWL_WNDPROC, ( LONG )ListsSubProc );
			_SetWindowLongW( g_hWnd_ignore_cid_list, GWL_WNDPROC, ( LONG )ListsSubProc );
			_SetWindowLongW( g_hWnd_ignore_list, GWL_WNDPROC, ( LONG )ListsSubProc );

			// Handles the drawing of the listviews in the tab controls.
			TabListsProc = ( WNDPROC )_GetWindowLongW( g_hWnd_tab, GWL_WNDPROC );
			_SetWindowLongW( g_hWnd_tab, GWL_WNDPROC, ( LONG )TabListsSubProc );
			_SetWindowLongW( g_hWnd_ignore_tab, GWL_WNDPROC, ( LONG )TabListsSubProc );

			// Handles the drawing of the main tab control.
			TabProc = ( WNDPROC )_GetWindowLongW( g_hWnd_tab, GWL_WNDPROC );
			_SetWindowLongW( g_hWnd_tab, GWL_WNDPROC, ( LONG )TabSubProc );

			_SendMessageW( g_hWnd_tab, WM_PROPAGATE, 1, 0 );	// Open theme data. Must be done after we subclass the control. Theme object will be closed when the tab control is destroyed.

			for ( int i = 0; i < 4; ++i )
			{
				if ( i == cfg_tab_order1 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_call_log, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Call_Log;		// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_call_log;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order2 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_contact_list, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Contact_List;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_contact_list;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order3 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_ignore_tab, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Ignore_Lists;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_ignore_tab;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
			}

			if ( total_tabs == 0 )
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
				if ( *call_log_columns[ i ] != -1 )
				{
					lvc.pszText = call_log_string_table[ i ];
					lvc.cx = *call_log_columns_width[ i ];
					_SendMessageW( g_hWnd_call_log, LVM_INSERTCOLUMN, total_columns1, ( LPARAM )&lvc );

					arr[ total_columns1++ ] = *call_log_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_call_log, LVM_SETCOLUMNORDERARRAY, total_columns1, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS2; ++i )
			{
				if ( *contact_list_columns[ i ] != -1 )
				{
					lvc.pszText = contact_list_string_table[ i ];
					lvc.cx = *contact_list_columns_width[ i ];
					_SendMessageW( g_hWnd_contact_list, LVM_INSERTCOLUMN, total_columns2, ( LPARAM )&lvc );

					arr[ total_columns2++ ] = *contact_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_contact_list, LVM_SETCOLUMNORDERARRAY, total_columns2, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS3; ++i )
			{
				if ( *ignore_list_columns[ i ] != -1 )
				{
					lvc.pszText = ignore_list_string_table[ i ];
					lvc.cx = *ignore_list_columns_width[ i ];
					_SendMessageW( g_hWnd_ignore_list, LVM_INSERTCOLUMN, total_columns3, ( LPARAM )&lvc );

					arr[ total_columns3++ ] = *ignore_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_ignore_list, LVM_SETCOLUMNORDERARRAY, total_columns3, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS4; ++i )
			{
				if ( *ignore_cid_list_columns[ i ] != -1 )
				{
					lvc.pszText = ignore_cid_list_string_table[ i ];
					lvc.cx = *ignore_cid_list_columns_width[ i ];
					_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTCOLUMN, total_columns4, ( LPARAM )&lvc );

					arr[ total_columns4++ ] = *ignore_cid_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_ignore_cid_list, LVM_SETCOLUMNORDERARRAY, total_columns4, ( LPARAM )arr );

			if ( cfg_tray_icon )
			{
				_memzero( &g_nid, sizeof( NOTIFYICONDATA ) );
				g_nid.cbSize = sizeof( g_nid );
				g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				g_nid.hWnd = hWnd;
				g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
				g_nid.uID = 1000;
				g_nid.hIcon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
				_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"VZ Enhanced 56K\0", 16 );
				g_nid.szTip[ 15 ] = 0;	// Sanity.
				_Shell_NotifyIconW( NIM_ADD, &g_nid );
			}

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
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL ) );
			}

			ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
			iui->ii = NULL;
			iui->phone_number = NULL;
			iui->action = 2;	// Add all ignore_list items.
			iui->hWnd = g_hWnd_ignore_list;

			// iui is freed in the update_ignore_list thread.
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );

			ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidupdateinfo ) );
			icidui->icidi = NULL;
			icidui->caller_id = NULL;
			icidui->action = 2;	// Add all ignore_cid_list items.
			icidui->hWnd = g_hWnd_ignore_cid_list;
			icidui->match_case = false;
			icidui->match_whole_word = false;

			// icidui is freed in the update_ignore_cid_list thread.
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_cid_list, ( void * )icidui, 0, NULL ) );

			contactupdateinfo *cui = ( contactupdateinfo * )GlobalAlloc( GPTR, sizeof( contactupdateinfo ) );
			cui->action = 2;
						
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )cui, 0, NULL ) );

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

		case WM_MOVING:
		{
			POINT cur_pos;
			RECT wa;
			RECT *rc = ( RECT * )lParam;

			_GetCursorPos( &cur_pos );
			_OffsetRect( rc, cur_pos.x - ( rc->left + cx ), cur_pos.y - ( rc->top + cy ) );

			// Allow our main window to attach to the desktop edge.
			_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );			
			if( is_close( rc->left, wa.left ) )				// Attach to left side of the desktop.
			{
				_OffsetRect( rc, wa.left - rc->left, 0 );
			}
			else if ( is_close( wa.right, rc->right ) )		// Attach to right side of the desktop.
			{
				_OffsetRect( rc, wa.right - rc->right, 0 );
			}

			if( is_close( rc->top, wa.top ) )				// Attach to top of the desktop.
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

			_SetWindowPos( g_hWnd_ignore_tab, NULL, 8, 8 + rc_tab.top, rc.right - rc.left - 16, rc.bottom - rc_tab.top - 16, SWP_NOZORDER );

			_GetClientRect( g_hWnd_ignore_tab, &rc );
			_SetRect( &rc_tab, 0, 0, rc.right, rc.bottom );
			_SendMessageW( g_hWnd_ignore_tab, TCM_ADJUSTRECT, FALSE, ( LPARAM )&rc_tab );

			_SetWindowPos( g_hWnd_ignore_cid_list, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ignore_list, NULL, 1, rc_tab.top - 1, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 1, SWP_NOZORDER );

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
			// Check to see if our command is a menu item.
			if ( HIWORD( wParam ) == 0 )
			{
				// Get the id of the menu item.
				switch ( LOWORD( wParam ) )
				{
					case MENU_CLOSE_TAB:
					{
						switch ( context_tab_index )
						{
							case 0:
							{
								_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_CALL_LOG, 0 ), 0 );
							}
							break;

							case 1:
							{
								_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_CONTACT_LIST, 0 ), 0 );
							}
							break;

							case 2:
							{
								_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_IGNORE_LISTS, 0 ), 0 );
							}
							break;
						}
					}
					break;

					case MENU_VIEW_CALL_LOG:
					case MENU_VIEW_CONTACT_LIST:
					case MENU_VIEW_IGNORE_LISTS:
					{
						char *tab_index = NULL;
						HWND t_hWnd = NULL;

						TCITEM ti;
						_memzero( &ti, sizeof( TCITEM ) );
						ti.mask = TCIF_PARAM | TCIF_TEXT;			// The tab will have text and an lParam value.

						if ( LOWORD( wParam ) == MENU_VIEW_CALL_LOG )
						{
							tab_index = &cfg_tab_order1;
							t_hWnd = g_hWnd_call_log;
							ti.pszText = ( LPWSTR )ST_Call_Log;		// This will simply set the width of each tab item. We're not going to use it.
						}
						else if ( LOWORD( wParam ) == MENU_VIEW_CONTACT_LIST )
						{
							tab_index = &cfg_tab_order2;
							t_hWnd = g_hWnd_contact_list;
							ti.pszText = ( LPWSTR )ST_Contact_List;		// This will simply set the width of each tab item. We're not going to use it.
						}
						else if ( LOWORD( wParam ) == MENU_VIEW_IGNORE_LISTS )
						{
							tab_index = &cfg_tab_order3;
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

							for ( int i = 0; i < total_tabs; ++i )
							{
								_SendMessageW( g_hWnd_tab, TCM_GETITEM, i, ( LPARAM )&ti );

								if ( ( HWND )ti.lParam == t_hWnd )
								{
									int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab

									// If the tab we remove is the last tab and it is focused, then set focus to the tab on the left.
									// If the tab we remove is focused and there is a tab to the right, then set focus to the tab on the right.
									if ( total_tabs > 1 && i == index )
									{
										_SendMessageW( g_hWnd_tab, TCM_SETCURFOCUS, ( i < total_tabs - 1 ? i + 1 : i - 1 ), 0 );	// Give the new tab focus.
									}

									_SendMessageW( g_hWnd_tab, TCM_DELETEITEM, i, 0 );
									--total_tabs;

									// Hide the tab control if no more tabs are visible.
									if ( total_tabs == 0 )
									{
										_ShowWindow( ( HWND )ti.lParam, SW_HIDE );
										_ShowWindow( g_hWnd_tab, SW_HIDE );
									}
								}
							}
						}
						else	// Add the tab.
						{
							*tab_index = total_tabs;	// The new tab will be added to the end.

							ti.lParam = ( LPARAM )t_hWnd;
							_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs, ( LPARAM )&ti );	// Insert a new tab at the end.

							// If no tabs were previously visible, then show the tab control.
							if ( total_tabs == 0 )
							{
								_ShowWindow( g_hWnd_tab, SW_SHOW );
								_ShowWindow( t_hWnd, SW_SHOW );

								_SendMessageW( hWnd, WM_SIZE, 0, 0 );	// Forces the window to resize the listview.
							}

							++total_tabs;
						}

						_CheckMenuItem( g_hMenu, LOWORD( wParam ), ( *tab_index != -1 ? MF_CHECKED : MF_UNCHECKED ) );
					}
					break;

					case MENU_COPY_SEL_COL1:
					case MENU_COPY_SEL_COL2:
					case MENU_COPY_SEL_COL3:
					case MENU_COPY_SEL_COL4:
					case MENU_COPY_SEL_COL5:
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
					case MENU_COPY_SEL_COL41:
					case MENU_COPY_SEL_COL42:
					case MENU_COPY_SEL_COL43:
					case MENU_COPY_SEL_COL44:
					case MENU_COPY_SEL:
					{
						HWND c_hWnd = GetCurrentListView();
						if ( c_hWnd != NULL )
						{
							copyinfo *ci = ( copyinfo * )GlobalAlloc( GMEM_FIXED, sizeof( copyinfo ) );
							ci->column = LOWORD( wParam );
							ci->hWnd = c_hWnd;

							// ci is freed in the copy items thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, copy_items, ( void * )ci, 0, NULL ) );
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
									ri->disable_critical_section = false;
									ri->hWnd = g_hWnd_call_log;

									// ri will be freed in the remove_items thread.
									CloseHandle( ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL ) );
								}
							}
							else if ( c_hWnd == g_hWnd_contact_list )
							{
								// Remove the first selected (not the focused & selected).
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM;
								lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries_contact_list, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
									{
										contactupdateinfo *cui = ( contactupdateinfo * )GlobalAlloc( GPTR, sizeof( contactupdateinfo ) );
										cui->action = 1;	// 1 = Remove, 0 = Add

										// cui is freed in the update_contact_list thread.
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )cui, 0, NULL ) );
									}
								}
							}
							else if ( c_hWnd == g_hWnd_ignore_list )
							{
								// Retrieve the lParam value from the selected listview item.
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM;
								lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
									{
										ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
										iui->ii = NULL;
										iui->phone_number = NULL;
										iui->action = 1;	// 1 = Remove, 0 = Add
										iui->hWnd = g_hWnd_ignore_list;

										// iui is freed in the update_ignore_list thread.
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
									}
								}
							}
							else if ( c_hWnd == g_hWnd_ignore_cid_list )
							{
								// Retrieve the lParam value from the selected listview item.
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM;
								lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
									{
										ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidupdateinfo ) );
										icidui->icidi = NULL;
										icidui->caller_id = NULL;
										icidui->match_case = false;
										icidui->match_whole_word = false;
										icidui->action = 1;	// 1 = Remove, 0 = Add
										icidui->hWnd = g_hWnd_ignore_cid_list;

										// icidui is freed in the update_ignore_cid_list thread.
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_cid_list, ( void * )icidui, 0, NULL ) );
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
						lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

							contactinfo *ci = ( contactinfo * )lvi.lParam;
							if ( ci != NULL && ci->web_page != NULL )
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

								_ShellExecuteW( NULL, L"open", ci->web_page, NULL, NULL, SW_SHOWNORMAL );

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
					{
						MENUITEMINFO mii;
						_memzero( &mii, sizeof( MENUITEMINFO ) );
						mii.cbSize = sizeof( MENUITEMINFO );
						mii.fMask = MIIM_DATA;
						_GetMenuItemInfoW( g_hMenu, MENU_SEARCH_WITH, FALSE, &mii );

						unsigned short column = ( unsigned short )mii.dwItemData;

						char *phone_number = GetSelectedColumnPhoneNumber( column );
						if ( phone_number != NULL && phone_number[ 0 ] == '+' )
						{
							++phone_number;
						}

						wchar_t *url = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * 128 );

						switch ( LOWORD( wParam ) )
						{
							case MENU_SEARCH_WITH_1: { __snwprintf( url, 128, L"http://800notes.com/Phone.aspx/%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_2: { __snwprintf( url, 128, L"https://www.bing.com/search?q=%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_3: { __snwprintf( url, 128, L"http://callerr.com/%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_4: { __snwprintf( url, 128, L"https://www.google.com/search?&q=%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_5: { __snwprintf( url, 128, L"http://www.okcaller.com/detail.php?number=%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_6: { __snwprintf( url, 128, L"https://www.phonetray.com/lookup/Number/%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_7: { __snwprintf( url, 128, L"http://www.whitepages.com/phone/%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_8: { __snwprintf( url, 128, L"http://whocallsme.com/Phone-Number.aspx/%S", SAFESTRA( phone_number ) ); } break;
							case MENU_SEARCH_WITH_9: { __snwprintf( url, 128, L"http://www.whycall.me/%S.html", SAFESTRA( phone_number ) ); } break;
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
						lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

							contactinfo *ci = ( contactinfo * )lvi.lParam;
							if ( ci != NULL && ci->email_address != NULL )
							{
								int email_address_length = lstrlenW( ci->email_address );
								wchar_t *mailto = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( email_address_length + 8 ) );
								_wmemcpy_s( mailto, email_address_length + 8, L"mailto:", 7 );
								_wmemcpy_s( mailto + 7, email_address_length + 1, ci->email_address, email_address_length );
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
						lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

							displayinfo *di = ( displayinfo * )lvi.lParam;

							di->process_incoming = false;
							di->ci.ignored = true;

							_InvalidateRect( g_hWnd_call_log, NULL, TRUE );

							IgnoreIncomingCall( di->incoming_call );
						}
					}
					break;

					case MENU_ADD_IGNORE_LIST:	// ignore list listview right click.
					{
						if ( g_hWnd_ignore_phone_number == NULL )
						{
							// Allow wildcard input. (Last parameter of CreateWindow is 1)
							g_hWnd_ignore_phone_number = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"phone", ST_Ignore_Phone_Number, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 255 ) / 2 ), 205, 255, NULL, NULL, NULL, ( LPVOID )1 );
						}

						_SendMessageW( g_hWnd_ignore_phone_number, WM_PROPAGATE, 0, 0 );

						_SetForegroundWindow( g_hWnd_ignore_phone_number );
					}
					break;

					case MENU_EDIT_IGNORE_CID_LIST:
					case MENU_ADD_IGNORE_CID_LIST:	// ignore cid list listview right click.
					{
						if ( g_hWnd_ignore_cid == NULL )
						{
							g_hWnd_ignore_cid = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"cid", ST_Ignore_Caller_ID_Name, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 170 ) / 2 ), 205, 170, NULL, NULL, NULL, ( LPVOID )0 );
						}

						if ( LOWORD( wParam ) == MENU_ADD_IGNORE_CID_LIST )
						{
							_SendMessageW( g_hWnd_ignore_cid, WM_PROPAGATE, 1, 0 );
						}
						else if ( LOWORD( wParam ) == MENU_EDIT_IGNORE_CID_LIST )
						{
							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.mask = LVIF_PARAM;
							lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );
							if ( lvi.iItem != -1 )
							{
								_SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEM, 0, ( LPARAM )&lvi );
								_SendMessageW( g_hWnd_ignore_cid, WM_PROPAGATE, 2, lvi.lParam );
							}
						}

						_SetForegroundWindow( g_hWnd_ignore_cid );
					}
					break;

					case MENU_IGNORE_LIST:	// call log listview right click.
					case MENU_IGNORE_CID_LIST:	// call log listview right click.
					{
						// Retrieve the lParam value from the selected listview item.
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;
						lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

							if ( LOWORD( wParam ) == MENU_IGNORE_LIST )
							{
								// This item we've selected is in the ignorelist tree.
								if ( ( ( displayinfo * )lvi.lParam )->ignore_phone_number )
								{
									if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries_ipn, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
									{
										ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
										iui->ii = NULL;
										iui->phone_number = NULL;
										iui->action = 1;	// 1 = Remove, 0 = Add
										iui->hWnd = g_hWnd_call_log;

										// iui is freed in the update_ignore_list thread.
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
									}
								}
								else	// Add items to the ignore list.
								{
									ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
									iui->ii = NULL;
									iui->phone_number = NULL;
									iui->action = 0;	// 1 = Remove, 0 = Add
									iui->hWnd = g_hWnd_call_log;

									// iui is freed in the update_ignore_list thread.
									CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
								}
							}
							else if ( LOWORD( wParam ) == MENU_IGNORE_CID_LIST )
							{
								// This item we've selected is in the ignorecidlist tree.
								if ( ( ( displayinfo * )lvi.lParam )->ignore_cid_match_count > 0 )
								{
									if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries_icid, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES )
									{
										ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidupdateinfo ) );
										icidui->icidi = NULL;
										icidui->caller_id = NULL;
										icidui->match_case = false;
										icidui->match_whole_word = false;
										icidui->action = 1;	// 1 = Remove, 0 = Add
										icidui->hWnd = g_hWnd_call_log;

										// Remove items.
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_cid_list, ( void * )icidui, 0, NULL ) );
									}
								}
								else	// The item we've selected is not in the ignorecidlist tree. Prompt the user.
								{
									if ( g_hWnd_ignore_cid == NULL )
									{
										g_hWnd_ignore_cid = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"cid", ST_Ignore_Caller_ID_Name, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 170 ) / 2 ), 205, 170, NULL, NULL, NULL, ( LPVOID )0 );
									}

									_SendMessageW( g_hWnd_ignore_cid, WM_PROPAGATE, ( _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 ) > 1 ? 3 : 1 ), lvi.lParam );

									_SetForegroundWindow( g_hWnd_ignore_cid );
								}
							}
						}
					}
					break;

					case MENU_SELECT_COLUMNS:
					{
						if ( g_hWnd_columns == NULL )
						{
							// Show columns window.
							g_hWnd_columns = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"columns", ST_Select_Columns, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 410 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - MIN_HEIGHT ) / 2 ), 410, MIN_HEIGHT, NULL, NULL, NULL, NULL );
						}

						unsigned char index = 0;

						HWND c_hWnd = GetCurrentListView();
						if ( c_hWnd != NULL )
						{
							if ( c_hWnd == g_hWnd_call_log )
							{
								index = 0;
							}
							else if ( c_hWnd == g_hWnd_contact_list )
							{
								index = 1;
							}
							else if ( c_hWnd == g_hWnd_ignore_list || c_hWnd == g_hWnd_ignore_cid_list )
							{
								index = 2;
							}
						}
						
						_SendMessageW( g_hWnd_columns, WM_PROPAGATE, index, 0 );
						
						_SetForegroundWindow( g_hWnd_columns );
					}
					break;

					case MENU_EDIT_CONTACT:
					case MENU_ADD_CONTACT:
					{
						if ( g_hWnd_contact == NULL )
						{
							g_hWnd_contact = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"contact", ST_Contact_Information, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 550 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 320 ) / 2 ), 550, 320, NULL, NULL, NULL, NULL );
						}

						if ( LOWORD( wParam ) == MENU_EDIT_CONTACT )
						{
							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.mask = LVIF_PARAM;
							lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

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
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, create_call_log_csv_file, ( void * )file_path, 0, NULL ) );
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
						ofn.lpstrFilter = L"Call Log History\0*.*\0" \
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
							if		( c_hWnd == g_hWnd_call_log )			{ ofn.nFilterIndex = 1; }
							else if ( c_hWnd == g_hWnd_contact_list )		{ ofn.nFilterIndex = 2; }
							else if ( c_hWnd == g_hWnd_ignore_cid_list )	{ ofn.nFilterIndex = 3; }
							else if ( c_hWnd == g_hWnd_ignore_list )		{ ofn.nFilterIndex = 4; }
						}

						if ( _GetSaveFileNameW( &ofn ) )
						{
							importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );

							/*// Change the default extension for the call log history.
							if ( ofn.nFilterIndex == 1 )
							{
								if ( ofn.nFileExtension > 0 )
								{
									if ( ofn.nFileExtension <= MAX_PATH - 4 )	// Overwrite the existing extension.
									{
										_wmemcpy_s( file_name + ofn.nFileExtension, MAX_PATH - ofn.nFileExtension, L"bin\0", 4 );
									}
								}
								//else	// No extension or the file was surrounded by quotes.
								//{
								//	int file_name_length = lstrlenW( file_name + ofn.nFileOffset );
								//	if ( ( ofn.nFileOffset + file_name_length ) <= MAX_PATH - 5 )	// Append the extension.
								//	{
								//		_wmemcpy_s( file_name + ofn.nFileOffset + file_name_length, MAX_PATH - ofn.nFileOffset + file_name_length, L".bin\0", 5 );
								//	}
								//}
							}*/

							iei->file_paths = file_name;

							iei->file_type = ( unsigned char )( 4 - ofn.nFilterIndex );

							// iei will be freed in the export_list thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, export_list, ( void * )iei, 0, NULL ) );
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
						ofn.lpstrFilter = L"Call Log History\0*.*\0" \
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
							if		( c_hWnd == g_hWnd_call_log )			{ ofn.nFilterIndex = 1; }
							else if ( c_hWnd == g_hWnd_contact_list )		{ ofn.nFilterIndex = 2; }
							else if ( c_hWnd == g_hWnd_ignore_cid_list )	{ ofn.nFilterIndex = 3; }
							else if ( c_hWnd == g_hWnd_ignore_list )		{ ofn.nFilterIndex = 4; }
						}

						if ( _GetOpenFileNameW( &ofn ) )
						{
							importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );

							iei->file_paths = file_name;
							iei->file_offset = ofn.nFileOffset;
							iei->file_type = ( unsigned char )( 4 - ofn.nFilterIndex );

							// iei will be freed in the import_list thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL ) );
						}
						else
						{
							GlobalFree( file_name );
						}
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
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, ( void * )update_info, 0, NULL ) );
						}

						_ShowWindow( g_hWnd_update, SW_SHOWNORMAL );
						_SetForegroundWindow( g_hWnd_update );
					}
					break;

					case MENU_ABOUT:
					{
						wchar_t msg[ 512 ];
						__snwprintf( msg, 512, L"VZ Enhanced 56K is made free under the GPLv3 license.\r\n\r\n" \
											   L"Version 1.0.0.3\r\n\r\n" \
											   L"Built on %s, %s %d, %04d %d:%02d:%02d %s (UTC)\r\n\r\n" \
											   L"Copyright \xA9 2013-2018 Eric Kutcher", GetDay( g_compile_time.wDayOfWeek ), GetMonth( g_compile_time.wMonth ), g_compile_time.wDay, g_compile_time.wYear, ( g_compile_time.wHour > 12 ? g_compile_time.wHour - 12 : ( g_compile_time.wHour != 0 ? g_compile_time.wHour : 12 ) ), g_compile_time.wMinute, g_compile_time.wSecond, ( g_compile_time.wHour >= 12 ? L"PM" : L"AM" ) );

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
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( ( HWND )( tie.lParam ), SW_HIDE );
					}

					return FALSE;
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
                {
					NMHDR *nmhdr = ( NMHDR * )lParam;

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information

						_ShowWindow( ( HWND )tie.lParam, SW_SHOW );

						if ( ( HWND )tie.lParam == g_hWnd_ignore_tab )
						{
							index = _SendMessageW( ( HWND )tie.lParam, TCM_GETCURSEL, 0, 0 );	// Get the selected tab
							if ( index != -1 )
							{
								_SendMessageW( ( HWND )tie.lParam, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
							}
						}

						_InvalidateRect( nmhdr->hwndFrom, NULL, TRUE );	// Repaint the control
					}

					ChangeMenuByHWND( ( HWND )tie.lParam );

					UpdateMenus( UM_ENABLE );

					return FALSE;
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
						if ( hWnd_parent == g_hWnd_call_log && *call_log_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, call_log_columns, NUM_COLUMNS1 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_contact_list && *contact_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, contact_list_columns, NUM_COLUMNS2 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_ignore_list && *ignore_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, ignore_list_columns, NUM_COLUMNS3 );
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_ignore_cid_list && *ignore_cid_list_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, ignore_cid_list_columns, NUM_COLUMNS4 );
							return TRUE;
						}
					}

					return FALSE;
				}
				break;

				case HDN_ENDTRACK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;
					HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );

					int index = nmh->iItem;

					if ( hWnd_parent == g_hWnd_call_log )
					{
						index = GetVirtualIndexFromColumnIndex( index, call_log_columns, NUM_COLUMNS1 );

						if ( index != -1 )
						{
							*call_log_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_contact_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, contact_list_columns, NUM_COLUMNS2 );

						if ( index != -1 )
						{
							*contact_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_ignore_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, ignore_list_columns, NUM_COLUMNS3 );

						if ( index != -1 )
						{
							*ignore_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( hWnd_parent == g_hWnd_ignore_cid_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, ignore_cid_list_columns, NUM_COLUMNS4 );

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

					sortinfo si;
					si.column = lvc.iOrder;
					si.hWnd = nmlv->hdr.hwndFrom;

					if ( HDF_SORTUP & lvc.fmt )	// Column is sorted upward.
					{
						si.direction = 0;	// Now sort down.

						// Sort down
						lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, ( WPARAM )nmlv->iSubItem, ( LPARAM )&lvc );

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CompareFunc );
					}
					else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
					{
						si.direction = 1;	// Now sort up.

						// Sort up
						lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CompareFunc );
					}
					else	// Column has no sorting set.
					{
						unsigned char column_count = ( nmlv->hdr.hwndFrom == g_hWnd_call_log ? total_columns1 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_contact_list ? total_columns2 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_ignore_list ? total_columns3 :
													 ( nmlv->hdr.hwndFrom == g_hWnd_ignore_cid_list ? total_columns4 : 0 ) ) ) );

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

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CompareFunc );
					}
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					HandleRightClick( nmitem->hdr.hwndFrom );
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
			if ( LOWORD( wParam ) == CW_MODIFY )
			{
				displayinfo *di = ( displayinfo * )lParam;	// lParam = our displayinfo structure from the connection thread.

				FormatDisplayInfo( di );

				// Insert a row into our listview.
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
				lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
				lvi.iSubItem = 0;
				lvi.lParam = lParam;
				int index = _SendMessageW( g_hWnd_call_log, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

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
						CreatePopup( di );
					}
				}
			}
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
			if ( g_hWnd_columns != NULL )
			{
				_EnableWindow( g_hWnd_columns, FALSE );
				_ShowWindow( g_hWnd_columns, SW_HIDE );
			}
			if ( g_hWnd_options != NULL )
			{
				_EnableWindow( g_hWnd_options, FALSE );
				_ShowWindow( g_hWnd_options, SW_HIDE );
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
			if ( g_hWnd_columns != NULL )
			{
				_DestroyWindow( g_hWnd_columns );
			}

			if ( g_hWnd_options != NULL )
			{
				_DestroyWindow( g_hWnd_options );
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
			int num_items = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			// Go through each item, and free their lParam values.
			for ( lvi.iItem = 0; lvi.iItem < num_items; ++lvi.iItem )
			{
				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;
				if ( di != NULL )
				{
					free_displayinfo( &di );
				}
			}

			UpdateColumnOrders();

			DestroyMenus();

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
