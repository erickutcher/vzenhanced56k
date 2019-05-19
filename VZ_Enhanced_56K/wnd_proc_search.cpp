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
#include "lite_gdi32.h"
#include "list_operations.h"
#include "string_tables.h"

#define BTN_SEARCH_ALL			1000
#define BTN_SEARCH				1001
#define BTN_SEARCH_CANCEL		1002

#define EDIT_SEARCH_FOR			1003

#define BTN_TYPE_FILENAME		1004
#define BTN_TYPE_URL			1005

#define BTN_MATCH_CASE			1006
#define BTN_MATCH_WHOLE_WORD	1007
#define BTN_REGULAR_EXPRESSION	1008

HWND g_hWnd_search = NULL;

HWND g_hWnd_static_search_for = NULL;
HWND g_hWnd_search_for = NULL;
HWND g_hWnd_chk_type_cid = NULL;
HWND g_hWnd_chk_type_phone_number = NULL;
HWND g_hWnd_chk_match_case = NULL;
HWND g_hWnd_chk_match_whole_word = NULL;
HWND g_hWnd_chk_regular_expression = NULL;
HWND g_hWnd_btn_search_all = NULL;
HWND g_hWnd_btn_search = NULL;
HWND g_hWnd_search_cancel = NULL;

LRESULT CALLBACK SearchWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_static_search_for = _CreateWindowW( WC_STATIC, ST_Search_for_, WS_CHILD | WS_VISIBLE, 20, 20, 60, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_search_for = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 35, rc.right - 40, 23, hWnd, ( HMENU )EDIT_SEARCH_FOR, NULL, NULL );

			HWND hWnd_btn_search_type = _CreateWindowW( WC_BUTTON, ST_Search_Type, BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 20, 65, 130, 60, hWnd, NULL, NULL, NULL );
			g_hWnd_chk_type_cid = _CreateWindowW( WC_BUTTON, ST_Caller_ID_Name, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 30, 80, 110, 20, hWnd, ( HMENU )BTN_TYPE_FILENAME, NULL, NULL );
			g_hWnd_chk_type_phone_number = _CreateWindowW( WC_BUTTON, ST_Phone_Number, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 30, 100, 110, 20, hWnd, ( HMENU )BTN_TYPE_URL, NULL, NULL );

			g_hWnd_chk_match_case = _CreateWindowW( WC_BUTTON, ST_Match_case, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 165, 65, 130, 20, hWnd, ( HMENU )BTN_MATCH_CASE, NULL, NULL );
			g_hWnd_chk_match_whole_word = _CreateWindowW( WC_BUTTON, ST_Match_whole_word, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 165, 85, 130, 20, hWnd, ( HMENU )BTN_MATCH_WHOLE_WORD, NULL, NULL );
			g_hWnd_chk_regular_expression = _CreateWindowW( WC_BUTTON, ST_Regular_expression, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE | ( g_use_regular_expressions ? 0 : WS_DISABLED ), 165, 105, 130, 20, hWnd, ( HMENU )BTN_REGULAR_EXPRESSION, NULL, NULL );

			g_hWnd_btn_search_all = _CreateWindowW( WC_BUTTON, ST_Search_All, WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, rc.right - 280, rc.bottom - 32, 85, 23, hWnd, ( HMENU )BTN_SEARCH_ALL, NULL, NULL );
			g_hWnd_btn_search = _CreateWindowW( WC_BUTTON, ST_Search_Next, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, rc.right - 190, rc.bottom - 32, 95, 23, hWnd, ( HMENU )BTN_SEARCH, NULL, NULL );
			g_hWnd_search_cancel = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_SEARCH_CANCEL, NULL, NULL );

			_SendMessageW( g_hWnd_static_search_for, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_search_for, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_btn_search_type, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_cid, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_match_case, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_match_whole_word, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_regular_expression, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_search_all, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_search, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_search_cancel, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_cid, BM_SETCHECK, BST_CHECKED, 0 );

			_SetFocus( g_hWnd_search_for );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc, frame_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_3DFACE ) );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			frame_rc = client_rc;
			frame_rc.left += 10;
			frame_rc.right -= 10;
			frame_rc.top += 10;
			frame_rc.bottom -= 40;

			// Fill the frame.
			color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
			_FillRect( hdcMem, &frame_rc, color );
			_DeleteObject( color );

			// Draw the frame's border.
			_DrawEdge( hdcMem, &frame_rc, EDGE_ETCHED, BF_RECT );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_SEARCH:
				case BTN_SEARCH_ALL:
				{
					// Prevents the user from holding down the button and queuing up a bunch of threads.
					if ( in_worker_thread )
					{
						break;
					}

					SEARCH_INFO *si = ( SEARCH_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SEARCH_INFO ) );
					si->search_all = ( LOWORD( wParam ) == BTN_SEARCH_ALL ? true : false );

					if ( _SendMessageW( g_hWnd_chk_regular_expression, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						si->search_flag = 0x04;	// Use regular expression.
					}
					else
					{
						si->search_flag = ( _SendMessageW( g_hWnd_chk_match_case, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 0x01 : 0x00 );
						si->search_flag |= ( _SendMessageW( g_hWnd_chk_match_whole_word, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 0x02 : 0x00 );
					}

					si->type = ( _SendMessageW( g_hWnd_chk_type_phone_number, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					unsigned int text_length = ( unsigned int )_SendMessageW( g_hWnd_search_for, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					si->text = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * text_length );
					_SendMessageW( g_hWnd_search_for, WM_GETTEXT, text_length, ( LPARAM )si->text );

					// Enabled, when our thread completes.
					_EnableWindow( g_hWnd_btn_search_all, FALSE );
					_EnableWindow( g_hWnd_btn_search, FALSE );

					// si is freed in search_list.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, search_list, ( void * )si, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						_EnableWindow( g_hWnd_btn_search_all, TRUE );
						_EnableWindow( g_hWnd_btn_search, TRUE );

						GlobalFree( si->text );
						GlobalFree( si );
					}
				}
				break;

				case EDIT_SEARCH_FOR:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						if ( _SendMessageW( ( HWND )lParam, WM_GETTEXTLENGTH, 0, 0 ) > 0 )
						{
							_EnableWindow( g_hWnd_btn_search_all, TRUE );
							_EnableWindow( g_hWnd_btn_search, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_btn_search_all, FALSE );
							_EnableWindow( g_hWnd_btn_search, FALSE );
						}
					}
				}
				break;

				case BTN_REGULAR_EXPRESSION:
				{
					if ( _SendMessageW( g_hWnd_chk_regular_expression, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_chk_match_case, FALSE );
						_EnableWindow( g_hWnd_chk_match_whole_word, FALSE );
					}
					else
					{
						_EnableWindow( g_hWnd_chk_match_case, TRUE );
						_EnableWindow( g_hWnd_chk_match_whole_word, TRUE );
					}
				}
				break;

				case BTN_SEARCH_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}

			return 0;
		}

		case WM_PROPAGATE:
		{
			if ( wParam == 1 )
			{
				_EnableWindow( g_hWnd_btn_search_all, TRUE );
				_EnableWindow( g_hWnd_btn_search, TRUE );
			}
			else
			{
				_ShowWindow( hWnd, SW_SHOWNORMAL );
				_SetForegroundWindow( hWnd );

				_SetFocus( g_hWnd_search_for );
			}
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
			g_hWnd_search = NULL;

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
