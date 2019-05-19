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
#include "utilities.h"
#include "list_operations.h"
#include "string_tables.h"
#include "connection.h"

#include "lite_gdi32.h"

#define BTN_CID_ACTION			1001
#define EDIT_CID_VALUE			1002
#define BTN_REGULAR_EXPRESSION	1003

HWND g_hWnd_ignore_cid = NULL;

HWND g_hWnd_caller_id = NULL;
HWND g_hWnd_cid_action = NULL;

HWND g_hWnd_chk_cid_match_case = NULL;
HWND g_hWnd_chk_cid_match_whole_word = NULL;
HWND g_hWnd_chk_cid_regular_expression = NULL;

display_info *edit_aicid_di = NULL;			// Display the caler ID we want to allow/ignore (when allowing/ignoring a caller ID in the call log listview).
allow_ignore_cid_info *edit_aicid = NULL;	// Display the caller ID we want to edit (when editing a caller ID in the allow/ignore caller id list listview).

bool multiple_caller_ids = false;	// Change the caller ID if we're adding multiple caller IDs.
unsigned char caller_id_type = LIST_TYPE_ALLOW;

LRESULT CALLBACK CIDWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			//HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_phone_static1 = _CreateWindowW( WC_STATIC, ST_Caller_ID_Name_, WS_CHILD | WS_VISIBLE, 20, 20, ( rc.right - rc.left ) - 40, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_caller_id = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NOHIDESEL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 35, ( rc.right - rc.left ) - 40, 23, hWnd, ( HMENU )EDIT_CID_VALUE, NULL, NULL );

			g_hWnd_chk_cid_match_case = _CreateWindowW( WC_BUTTON, ST_Match_case, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 63, rc.right - 40, 20, hWnd, NULL, NULL, NULL );
			g_hWnd_chk_cid_match_whole_word = _CreateWindowW( WC_BUTTON, ST_Match_whole_word, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 83, rc.right - 40, 20, hWnd, NULL, NULL, NULL );
			g_hWnd_chk_cid_regular_expression = _CreateWindowW( WC_BUTTON, ST_Regular_expression, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE | ( g_use_regular_expressions ? 0 : WS_DISABLED ), 20, 103, rc.right - 40, 20, hWnd, ( HMENU )BTN_REGULAR_EXPRESSION, NULL, NULL );

			g_hWnd_cid_action = _CreateWindowW( WC_BUTTON, ST_Ignore_Caller_ID_Name, WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) - 40 ) / 2, rc.bottom - 32, ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) + 40, 23, hWnd, ( HMENU )BTN_CID_ACTION, NULL, NULL );

			_SendMessageW( g_hWnd_caller_id, EM_LIMITTEXT, 15, 0 );

			_SendMessageW( g_hWnd_chk_cid_match_case, BM_SETCHECK, BST_CHECKED, 0 );
			_SendMessageW( g_hWnd_chk_cid_match_whole_word, BM_SETCHECK, BST_CHECKED, 0 );

			_SendMessageW( g_hWnd_phone_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_caller_id, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_cid_match_case, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_cid_match_whole_word, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_cid_regular_expression, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_cid_action, WM_SETFONT, ( WPARAM )hFont, 0 );

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
				case BTN_CID_ACTION:
				{
					int length = 0;

					if ( !multiple_caller_ids )
					{
						length = ( int )_SendMessageW( g_hWnd_caller_id, WM_GETTEXTLENGTH, 0, 0 );

						if ( length <= 0 )
						{
							_MessageBoxW( hWnd, ST_enter_valid_caller_id, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
							_SetFocus( g_hWnd_caller_id );
							break;
						}
					}

					unsigned char match_flag = 0x00;

					if ( _SendMessageW( g_hWnd_chk_cid_regular_expression, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						match_flag = 0x04;
					}
					else
					{
						match_flag |= ( _SendMessageW( g_hWnd_chk_cid_match_case, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 0x02 : 0x00 );
						match_flag |= ( _SendMessageW( g_hWnd_chk_cid_match_whole_word, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 0x01 : 0x00 );
					}

					allow_ignore_cid_update_info *aicidui = ( allow_ignore_cid_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_update_info ) );
					aicidui->action = 0;	// Add = 0, 1 = Remove, 2 = Add all tree items, 3 = Update
					aicidui->list_type = caller_id_type;
					aicidui->hWnd = ( caller_id_type == LIST_TYPE_ALLOW ? g_hWnd_allow_cid_list : g_hWnd_ignore_cid_list );
					aicidui->match_flag = match_flag;

					if ( edit_aicid_di != NULL )
					{
						aicidui->hWnd = g_hWnd_call_log;
					}
					else	// Add or Update entry.
					{
						aicidui->caller_id = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( length + 1 ) );
						_SendMessageW( g_hWnd_caller_id, WM_GETTEXT, length + 1, ( LPARAM )aicidui->caller_id );

						if ( edit_aicid != NULL )
						{
							aicidui->aicidi = edit_aicid;

							aicidui->action = 3;	// Update entry.
						}
					}

					// aicidui is freed in the update_allow_ignore_cid_list thread.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_cid_list, ( void * )aicidui, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						GlobalFree( aicidui->caller_id );
						GlobalFree( aicidui );
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_REGULAR_EXPRESSION:
				{
					if ( _SendMessageW( g_hWnd_chk_cid_regular_expression, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_chk_cid_match_case, FALSE );
						_EnableWindow( g_hWnd_chk_cid_match_whole_word, FALSE );

						if ( !multiple_caller_ids )
						{
							_SendMessageW( g_hWnd_caller_id, EM_LIMITTEXT, 0, 0 );
						}
					}
					else
					{
						_EnableWindow( g_hWnd_chk_cid_match_case, TRUE );
						_EnableWindow( g_hWnd_chk_cid_match_whole_word, TRUE );

						if ( !multiple_caller_ids )
						{
							wchar_t caller_id[ 16 ];
							int length = ( int )_SendMessageW( g_hWnd_caller_id, WM_GETTEXTLENGTH, 0, 0 );
							if ( length > 15 )
							{
								_SendMessageW( g_hWnd_caller_id, WM_GETTEXT, 16, ( LPARAM )caller_id );
								caller_id[ 15 ] = 0; // Sanity.

								_SendMessageW( g_hWnd_caller_id, WM_SETTEXT, 0, ( LPARAM )caller_id );
							}

							_SendMessageW( g_hWnd_caller_id, EM_LIMITTEXT, 15, 0 );
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			caller_id_type = ( ( wParam & 0x04 ) ? LIST_TYPE_ALLOW : LIST_TYPE_IGNORE );

			if ( ( wParam & 0x01 ) || ( wParam & 0x08 ) )	// Allow/Ignore caller ID(s).
			{
				edit_aicid = NULL;

				edit_aicid_di = ( display_info * )lParam;
				multiple_caller_ids = ( ( wParam & 0x08 ) ? true : false );

				if ( caller_id_type == LIST_TYPE_ALLOW )
				{
					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Caller_ID_Name );
					_SendMessageW( g_hWnd_cid_action, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Caller_ID_Name );
				}
				else
				{
					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Caller_ID_Name );
					_SendMessageW( g_hWnd_cid_action, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Caller_ID_Name );
				}

				_SendMessageW( g_hWnd_chk_cid_match_case, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_cid_match_whole_word, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_cid_regular_expression, BM_SETCHECK, BST_UNCHECKED, 0 );

				_EnableWindow( g_hWnd_chk_cid_match_case, TRUE );
				_EnableWindow( g_hWnd_chk_cid_match_whole_word, TRUE );
				_EnableWindow( g_hWnd_chk_cid_regular_expression, TRUE );

				if ( edit_aicid_di != NULL )	// If this is not NULL, then we're adding a caller ID from the call log listview.
				{
					if ( multiple_caller_ids )
					{
						_SendMessageW( g_hWnd_caller_id, WM_SETTEXT, 0, ( LPARAM )ST__multiple_caller_ID_names_ );

						_EnableWindow( g_hWnd_caller_id, FALSE );		// We don't want to edit this value.
					}
					else
					{
						_SendMessageW( g_hWnd_caller_id, WM_SETTEXT, 0, ( LPARAM )edit_aicid_di->caller_id );

						_EnableWindow( g_hWnd_caller_id, TRUE );

						_SetFocus( g_hWnd_caller_id );

						_SendMessageW( g_hWnd_caller_id, EM_SETSEL, 0, -1 );

						edit_aicid_di = NULL;	// Add normally.
					}
				}
				else	// This shouldn't be NULL.
				{
					_EnableWindow( g_hWnd_caller_id, TRUE );		// Allow us to edit this value.

					_SendMessageW( g_hWnd_caller_id, WM_SETTEXT, 0, 0 );

					_SetFocus( g_hWnd_caller_id );
				}
			}
			else if ( wParam & 0x02 )	// Update allow/ignore information.
			{
				multiple_caller_ids = false;
				edit_aicid_di = NULL;

				edit_aicid = ( allow_ignore_cid_info * )lParam;

				if ( edit_aicid != NULL )	// If not NULL, then allow update.
				{
					if ( caller_id_type == LIST_TYPE_ALLOW )
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Caller_ID_Name );
					}
					else
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Caller_ID_Name );
					}

					_SendMessageW( g_hWnd_cid_action, WM_SETTEXT, 0, ( LPARAM )ST_Update_Caller_ID_Name );

					_SendMessageW( g_hWnd_caller_id, WM_SETTEXT, 0, ( LPARAM )edit_aicid->caller_id );
					_SendMessageW( g_hWnd_chk_cid_match_case, BM_SETCHECK, ( ( edit_aicid->match_flag & 0x02 ) ? BST_CHECKED : BST_UNCHECKED ), 0 );
					_SendMessageW( g_hWnd_chk_cid_match_whole_word, BM_SETCHECK, ( ( edit_aicid->match_flag & 0x01 ) ? BST_CHECKED : BST_UNCHECKED ), 0 );
					_SendMessageW( g_hWnd_chk_cid_regular_expression, BM_SETCHECK, ( ( edit_aicid->match_flag & 0x04 ) ? BST_CHECKED : BST_UNCHECKED ), 0 );

					if ( edit_aicid->match_flag & 0x04 )
					{
						_EnableWindow( g_hWnd_chk_cid_match_case, FALSE );
						_EnableWindow( g_hWnd_chk_cid_match_whole_word, FALSE );

						_SendMessageW( g_hWnd_caller_id, EM_LIMITTEXT, 0, 0 );
					}
					else
					{
						_EnableWindow( g_hWnd_chk_cid_match_case, TRUE );
						_EnableWindow( g_hWnd_chk_cid_match_whole_word, TRUE );

						_SendMessageW( g_hWnd_caller_id, EM_LIMITTEXT, 15, 0 );
					}
				}
				else	// If NULL, then I suppose we'll just add new numbers.
				{
					if ( caller_id_type == LIST_TYPE_ALLOW )
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Caller_ID_Name );
						_SendMessageW( g_hWnd_cid_action, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Caller_ID_Name );
					}
					else
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Caller_ID_Name );
						_SendMessageW( g_hWnd_cid_action, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Caller_ID_Name );
					}

					_SendMessageW( g_hWnd_caller_id, WM_SETTEXT, 0, 0 );
					_SendMessageW( g_hWnd_chk_cid_match_case, BM_SETCHECK, BST_CHECKED, 0 );
					_SendMessageW( g_hWnd_chk_cid_match_whole_word, BM_SETCHECK, BST_CHECKED, 0 );
					_SendMessageW( g_hWnd_chk_cid_regular_expression, BM_SETCHECK, BST_UNCHECKED, 0 );

					_EnableWindow( g_hWnd_chk_cid_match_case, TRUE );
					_EnableWindow( g_hWnd_chk_cid_match_whole_word, TRUE );

					_SendMessageW( g_hWnd_caller_id, EM_LIMITTEXT, 15, 0 );
				}

				_EnableWindow( g_hWnd_chk_cid_regular_expression, TRUE );
				_EnableWindow( g_hWnd_caller_id, TRUE );

				_SetFocus( g_hWnd_caller_id );

				_SendMessageW( g_hWnd_caller_id, EM_SETSEL, 0, -1 );
			}

			_ShowWindow( hWnd, SW_SHOWNORMAL );
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
			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			edit_aicid_di = NULL;
			edit_aicid = NULL;
			multiple_caller_ids = false;

			g_hWnd_ignore_cid = NULL;

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
