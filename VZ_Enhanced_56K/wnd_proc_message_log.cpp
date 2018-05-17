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
#include "message_log_utilities.h"
#include "lite_gdi32.h"
#include "lite_comdlg32.h"
#include "string_tables.h"
#include "utilities.h"

#define BTN_SAVE_LOG		1000
#define BTN_CLEAR_LOG		1001
#define BTN_CLOSE_LOG_WND	1002

#define MENU_ML_COPY_SEL	2000
#define MENU_ML_SELECT_ALL	2001

HWND g_hWnd_message_log_list = NULL;
HWND g_hWnd_save_log = NULL;
HWND g_hWnd_clear_log = NULL;
HWND g_hWnd_close_log_wnd = NULL;

HMENU g_hMenuSub_message_log = NULL;

bool skip_message_log_draw = false;
bool add_to_ml_top = true;

// Sort function for columns.
int CALLBACK MLCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	sortinfo *si = ( sortinfo * )lParamSort;

	if ( si->hWnd == g_hWnd_message_log_list )
	{
		MESSAGE_LOG_INFO *mli1 = ( MESSAGE_LOG_INFO * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		MESSAGE_LOG_INFO *mli2 = ( MESSAGE_LOG_INFO * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		switch ( si->column )
		{
			case 1: { return ( mli1->date_and_time.QuadPart > mli2->date_and_time.QuadPart ); } break;
			case 2: { return ( mli1->level < mli2->level ); } break;

			case 3: { return _wcsicmp_s( mli1->message, mli2->message ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}

	return 0;
}

LRESULT CALLBACK MessageLogWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_message_log_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_message_log_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );

			g_hWnd_save_log = _CreateWindowW( WC_BUTTON, ST_Save_Message_Log___,  WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SAVE_LOG, NULL, NULL );
			g_hWnd_clear_log = _CreateWindowW( WC_BUTTON, ST_Clear_Message_Log,  WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CLEAR_LOG, NULL, NULL );
			g_hWnd_close_log_wnd = _CreateWindowW( WC_BUTTON, ST_Close,  BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CLOSE_LOG_WND, NULL, NULL );

			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = ST_MLL_NUM;
			lvc.cx = 35;
			_SendMessageW( g_hWnd_message_log_list, LVM_INSERTCOLUMN, 0, ( LPARAM )&lvc );

			lvc.pszText = ST_MLL_Date_and_Time_UTC;
			lvc.cx = 155;
			_SendMessageW( g_hWnd_message_log_list, LVM_INSERTCOLUMN, 1, ( LPARAM )&lvc );

			lvc.pszText = ST_MLL_Level;
			lvc.cx = 70;
			_SendMessageW( g_hWnd_message_log_list, LVM_INSERTCOLUMN, 2, ( LPARAM )&lvc );

			lvc.pszText = ST_MLL_Message;
			lvc.cx = 700;
			_SendMessageW( g_hWnd_message_log_list, LVM_INSERTCOLUMN, 3, ( LPARAM )&lvc );

			g_hMenuSub_message_log = _CreatePopupMenu();

			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_Copy_Selected;
			mii.cch = 13;
			mii.wID = MENU_ML_COPY_SEL;
			mii.fState = MFS_ENABLED;
			_InsertMenuItemW( g_hMenuSub_message_log, 0, TRUE, &mii );

			mii.fType = MFT_SEPARATOR;
			_InsertMenuItemW( g_hMenuSub_message_log, 1, TRUE, &mii );

			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_Select_All;
			mii.cch = 10;
			mii.wID = MENU_ML_SELECT_ALL;
			_InsertMenuItemW( g_hMenuSub_message_log, 2, TRUE, &mii );

			_SendMessageW( g_hWnd_message_log_list, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_clear_log, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_save_log, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_close_log_wnd, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_CLOSE_LOG_WND:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_SAVE_LOG:
				{
					wchar_t *file_path = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"CSV (Comma delimited) (*.csv)\0*.csv\0";
					ofn.lpstrDefExt = L"csv";
					ofn.lpstrTitle = ST_Save_Message_Log;
					ofn.lpstrFile = file_path;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

					if ( _GetSaveFileNameW( &ofn ) )
					{
						// file_path will be freed in the create_message_log_csv_file thread.
						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, create_message_log_csv_file, ( void * )file_path, 0, NULL );
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

				case BTN_CLEAR_LOG:
				{
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, clear_message_log, ( void * )NULL, 0, NULL ) );
				}
				break;

				case MENU_ML_COPY_SEL:
				{
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, copy_message_log, ( void * )NULL, 0, NULL ) );
				}
				break;

				case MENU_ML_SELECT_ALL:
				{
					// Set the state of all items to selected.
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.state = LVIS_SELECTED;
					lvi.stateMask = LVIS_SELECTED;
					_SendMessageW( g_hWnd_message_log_list, LVM_SETITEMSTATE, -1, ( LPARAM )&lvi );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
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

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )MLCompareFunc );
					}
					else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
					{
						si.direction = 1;	// Now sort up.

						// Sort up
						lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )MLCompareFunc );
					}
					else	// Column has no sorting set.
					{
						// Remove the sort format for all columns.
						for ( unsigned char i = 0; i < 4; ++i )
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

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )MLCompareFunc );
					}

					// Add items to the top of the list, or the bottom.
					if ( si.column == 1 )
					{
						add_to_ml_top = ( si.direction == 0 ? true : false );
					}
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_message_log_list )
					{
						POINT p;
						_GetCursorPos( &p );

						int item_count = _SendMessageW( nmitem->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 );
						int sel_count = _SendMessageW( nmitem->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 );
						
						_EnableMenuItem( g_hMenuSub_message_log, MENU_ML_COPY_SEL, ( sel_count > 0 ? MF_ENABLED : MF_DISABLED ) );
						_EnableMenuItem( g_hMenuSub_message_log, MENU_ML_SELECT_ALL, ( sel_count == item_count ? MF_DISABLED : MF_ENABLED ) );

						_TrackPopupMenu( g_hMenuSub_message_log, 0, p.x, p.y, 0, g_hWnd_message_log, NULL );
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
					if ( _GetKeyState( VK_CONTROL ) & 0x8000 && !in_ml_worker_thread )
					{
						NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

						// Determine which key was pressed.
						switch ( ( ( LPNMLVKEYDOWN )lParam )->wVKey )
						{
							case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_ML_SELECT_ALL, 0 );
								}
							}
							break;

							case 'C':	// Copy selected items if Ctrl + C is down and there are selected items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_ML_COPY_SEL, 0 );
								}
							}
							break;
						}
					}
				}
				break;
			}
			return FALSE;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_LISTVIEW && dis->itemData != NULL )
			{
				MESSAGE_LOG_INFO *mli = ( MESSAGE_LOG_INFO * )dis->itemData;

				// If an item is being deleted, then don't draw it.
				if ( skip_message_log_draw && dis->hwndItem == g_hWnd_message_log_list )
				{
					return TRUE;
				}

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
					HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
					selected = true;
				}

				wchar_t tbuf[ 11 ];
				wchar_t *buf = tbuf;

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i < 4; ++i )
				{
					// Save the appropriate text in our buffer for the current column.
					switch ( i )
					{
						case 0:
						{
							buf = tbuf;	// Reset the buffer pointer.

							__snwprintf( buf, 11, L"%lu", dis->itemID + 1 );
						}
						break;

						case 1:
						{
							buf = mli->w_date_and_time;
						}
						break;

						case 2:
						{
							buf = mli->w_level;
						}
						break;

						case 3:
						{
							buf = mli->message;
						}
						break;
					}

					if ( buf == NULL )
					{
						tbuf[ 0 ] = L'\0';
						buf = tbuf;
					}

					// Get the dimensions of the listview column
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMN, i, ( LPARAM )&lvc );

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

						// Set the level to either red, orange, or blue.
						if ( i == 2 )
						{
							_SetTextColor( hdcMem, ( mli->level == 0 ? RGB( 0xFF, 0x00, 0x00 ) : ( mli->level == 1 ? RGB( 0xFF, 0x80, 0x00 ) : ( mli->level == 2 ? RGB( 0x80, 0x80, 0xFF ) : RGB( 0x00, 0x00, 0x00 ) ) ) ) );
						}
						else
						{
							// Black text.
							_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );
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

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = MIN_WIDTH;
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = MIN_HEIGHT;
			
			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 4 );
			_DeferWindowPos( hdwp, g_hWnd_message_log_list, HWND_TOP, 10, 10, rc.right - 20, rc.bottom - 52, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_save_log, HWND_TOP, 10, rc.bottom - 32, 130, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_clear_log, HWND_TOP, 150, rc.bottom - 32, 130, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_close_log_wnd, HWND_TOP, rc.right - 90, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

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

		case WM_ALERT:
		{
			HandleMessageLogInput( ( DWORD )wParam, ( void * )lParam );
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

            return FALSE;
		}
		break;

		case WM_CLOSE:
		{
			_ShowWindow( hWnd, SW_HIDE );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			cleanup_message_log();

			_DestroyMenu( g_hMenuSub_message_log );

			g_hWnd_message_log = NULL;

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
