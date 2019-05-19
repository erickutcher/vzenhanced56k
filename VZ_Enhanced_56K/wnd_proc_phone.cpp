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

#define BTN_0			1000
#define BTN_1			1001
#define BTN_2			1002
#define BTN_3			1003
#define BTN_4			1004
#define BTN_5			1005
#define BTN_6			1006
#define BTN_7			1007
#define BTN_8			1008
#define BTN_9			1009

#define BTN_ACTION		1012
#define EDIT_NUMBER		1013

HWND g_hWnd_ignore_phone_number = NULL;

HWND g_hWnd_number = NULL;
HWND g_hWnd_phone_action = NULL;

allow_ignore_info *edit_aii = NULL;		// Display the Phone Number we want to edit (when editing a Phone Number in the allow/ignore phone number list listview).

unsigned char phone_number_type = LIST_TYPE_ALLOW;

WNDPROC EditProc = NULL;

LRESULT CALLBACK EditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CHAR:
		{
			char current_phone_number[ 17 ];
			_memzero( current_phone_number, 17 );
			int current_phone_number_length = ( int )_SendMessageA( hWnd, WM_GETTEXT, 17, ( LPARAM )current_phone_number );

			DWORD offset_begin = 0, offset_end = 0;
			_SendMessageA( hWnd, EM_GETSEL, ( WPARAM )&offset_begin, ( WPARAM )&offset_end );

			// If a '*' or '+' was entered, set the text and return. Don't perform the default window procedure.
			if ( wParam == '*' )
			{
				if ( ( current_phone_number[ 0 ] == '+' && offset_begin == 0 && ( offset_end == 0 || ( offset_end == 1 && current_phone_number_length >= 16 ) ) ) ||
					 ( current_phone_number[ 0 ] != '+' && offset_begin == offset_end && current_phone_number_length >= 15 ) )
				{
					return 0;
				}

				_SendMessageA( hWnd, EM_REPLACESEL, FALSE, ( LPARAM )"*" );
				return 0;
			}
			else if ( wParam == '+' )
			{
				if ( ( current_phone_number[ 0 ] != '+' && offset_begin == 0 && offset_end == 0 ) || ( offset_begin == 0 && offset_end > 0 ) )
				{
					_SendMessageA( hWnd, EM_REPLACESEL, FALSE, ( LPARAM )"+" );
					return 0;
				}
			}
			else if ( wParam >= 0x30 && wParam <= 0x39 )
			{
				if ( ( current_phone_number[ 0 ] == '+' && offset_begin == 0 && ( offset_end == 0 || ( offset_end == 1 && current_phone_number_length >= 16 ) ) ) ||
					 ( current_phone_number[ 0 ] != '+' && offset_begin == offset_end && current_phone_number_length >= 15 ) )
				{
					return 0;
				}
			}
		}
		break;

		case WM_PASTE:
		{
			if ( _OpenClipboard( hWnd ) )
			{
				bool ret = true;
				char phone_number[ 17 ];
				_memzero( phone_number, 17 );
				char paste_length = 0;

				// Get ASCII text from the clipboard.
				HANDLE hglbPaste = _GetClipboardData( CF_TEXT );
				if ( hglbPaste != NULL )
				{
					// Get whatever text data is in the clipboard.
					char *lpstrPaste = ( char * )GlobalLock( hglbPaste );
					if ( lpstrPaste != NULL )
					{
						// Find the length of the data and limit it to 15 + 1 ("+") characters.
						paste_length = ( char )min( 16, lstrlenA( lpstrPaste ) );

						// Copy the data to our phone number buffer.
						_memcpy_s( phone_number, 17, lpstrPaste, paste_length );
						phone_number[ paste_length ] = 0;	// Sanity.
					}

					GlobalUnlock( hglbPaste );
				}

				_CloseClipboard();

				if ( paste_length > 0 )
				{
					char i = 0;

					if ( phone_number[ 0 ] == '+' )
					{
						++i;
					}
					else if ( paste_length == 16 )
					{
						phone_number[ --paste_length ] = 0;
					}

					// Make sure the phone number contains only numbers or wildcard values.
					for ( ; i < paste_length; ++i )
					{
						if ( phone_number[ i ] != '*' && !is_digit( phone_number[ i ] ) )
						{
							ret = false;
							break;
						}
					}

					// If we have a value and all characters are acceptable, then set the text in the text box and return. Don't perform the default window procedure.
					if ( ret )
					{
						char current_phone_number[ 17 ];
						_memzero( current_phone_number, 17 );
						int current_phone_number_length = ( int )_SendMessageA( hWnd, WM_GETTEXT, 17, ( LPARAM )current_phone_number );

						DWORD offset_begin = 0, offset_end = 0;
						_SendMessageA( hWnd, EM_GETSEL, ( WPARAM )&offset_begin, ( LPARAM )&offset_end );

						if ( current_phone_number[ 0 ] == '+' && phone_number[ 0 ] != '+' && offset_begin > 0 && offset_end > 0 ||
							 current_phone_number[ 0 ] != '+' && ( ( phone_number[ 0 ] != '+' && ( ( current_phone_number_length + paste_length ) <= 15 ) ) ||
																   ( phone_number[ 0 ] == '+' && offset_begin == 0 ) ) )
						{
							_SendMessageA( hWnd, EM_REPLACESEL, FALSE, ( LPARAM )phone_number );
						}

						return 0;
					}
				}
			}
		}
		break;
	}

	// Everything that we don't handle gets passed back to the parent to process.
	return _CallWindowProcW( EditProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK PhoneWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// 0 = default, 1 = allow wildcards
			CHAR enable_items = ( CHAR )( ( CREATESTRUCT * )lParam )->lpCreateParams;

			//HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_phone_static1 = _CreateWindowW( WC_STATIC, ST_Phone_Number_, WS_CHILD | WS_VISIBLE, 20, 20, ( rc.right - rc.left ) - 40, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NOHIDESEL | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 35, ( rc.right - rc.left ) - 40, 23, hWnd, ( HMENU )EDIT_NUMBER, NULL, NULL );

			HWND g_hWnd_1 = _CreateWindowW( WC_BUTTON, L"1", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 68, 50, 25, hWnd, ( HMENU )BTN_1, NULL, NULL );
			HWND g_hWnd_2 = _CreateWindowW( WC_BUTTON, L"2 ABC", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 68, 50, 25, hWnd, ( HMENU )BTN_2, NULL, NULL );
			HWND g_hWnd_3 = _CreateWindowW( WC_BUTTON, L"3 DEF", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 130, 68, 50, 25, hWnd, ( HMENU )BTN_3, NULL, NULL );

			HWND g_hWnd_4 = _CreateWindowW( WC_BUTTON, L"4 GHI", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 98, 50, 25, hWnd, ( HMENU )BTN_4, NULL, NULL );
			HWND g_hWnd_5 = _CreateWindowW( WC_BUTTON, L"5 JKL", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 98, 50, 25, hWnd, ( HMENU )BTN_5, NULL, NULL );
			HWND g_hWnd_6 = _CreateWindowW( WC_BUTTON, L"6 MNO", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 130, 98, 50, 25, hWnd, ( HMENU )BTN_6, NULL, NULL );

			HWND g_hWnd_7 = _CreateWindowW( WC_BUTTON, L"7 PQRS", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 128, 50, 25, hWnd, ( HMENU )BTN_7, NULL, NULL );
			HWND g_hWnd_8 = _CreateWindowW( WC_BUTTON, L"8 TUV", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 128, 50, 25, hWnd, ( HMENU )BTN_8, NULL, NULL );
			HWND g_hWnd_9 = _CreateWindowW( WC_BUTTON, L"9 WXYZ", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 130, 128, 50, 25, hWnd, ( HMENU )BTN_9, NULL, NULL );

			HWND g_hWnd_0 = _CreateWindowW( WC_BUTTON, L"0", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 158, 50, 25, hWnd, ( HMENU )BTN_0, NULL, NULL );

			g_hWnd_phone_action = _CreateWindowW( WC_BUTTON, ST_Ignore_Phone_Number, WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) - 40 ) / 2, rc.bottom - 32, ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) + 40, 23, hWnd, ( HMENU )BTN_ACTION, NULL, NULL );

			_SendMessageW( g_hWnd_number, EM_LIMITTEXT, ( enable_items != 0 ? 16 : 15 ), 0 );

			_SendMessageW( g_hWnd_phone_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_number, WM_SETFONT, ( WPARAM )hFont, 0 );

			

			_SendMessageW( g_hWnd_phone_action, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_3, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_4, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_5, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_6, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_7, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_8, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_9, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_0, WM_SETFONT, ( WPARAM )hFont, 0 );

			if ( enable_items != 0 )
			{
				EditProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_number, GWLP_WNDPROC );
				_SetWindowLongPtrW( g_hWnd_number, GWLP_WNDPROC, ( LONG_PTR )EditSubProc );
			}

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
				case BTN_ACTION:
				{
					int length = ( int )_SendMessageW( g_hWnd_number, WM_GETTEXTLENGTH, 0, 0 );
					if ( length <= 0 )
					{
						_MessageBoxW( hWnd, ST_enter_valid_phone_number, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
						_SetFocus( g_hWnd_number );
					}
					else
					{
						allow_ignore_update_info *aiui = ( allow_ignore_update_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_update_info ) );
						aiui->action = 0;	// Add = 0, 1 = Remove, 2 = Add all tree items, 3 = Update
						aiui->list_type = phone_number_type;
						aiui->hWnd = ( phone_number_type == LIST_TYPE_ALLOW ? g_hWnd_allow_list : g_hWnd_ignore_list );

						aiui->phone_number = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( length + 1 ) );
						_SendMessageW( g_hWnd_number, WM_GETTEXT, length + 1, ( LPARAM )aiui->phone_number );

						if ( edit_aii != NULL )
						{
							aiui->aii = edit_aii;

							aiui->action = 3;	// Update entry.
						}

						// Add items. aiui is freed in the update_allow_ignore_list thread.
						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_allow_ignore_list, ( void * )aiui, 0, NULL );
						if ( thread != NULL )
						{
							CloseHandle( thread );
						}
						else
						{
							GlobalFree( aiui->phone_number );
							GlobalFree( aiui );
						}

						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
					}
				}
				break;

				case EDIT_NUMBER:
				{
					if ( HIWORD( wParam ) == EN_SETFOCUS )
					{
						_PostMessageW( g_hWnd_number, EM_SETSEL, 0, -1 );
					}
				}
				break;

				case BTN_1: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"1" ); } break;
				case BTN_2: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"2" ); } break;
				case BTN_3: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"3" ); } break;
				case BTN_4: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"4" ); } break;
				case BTN_5: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"5" ); } break;
				case BTN_6: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"6" ); } break;
				case BTN_7: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"7" ); } break;
				case BTN_8: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"8" ); } break;
				case BTN_9: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"9" ); } break;
				case BTN_0: { _SendMessageW( g_hWnd_number, EM_REPLACESEL, FALSE, ( LPARAM )L"0" ); } break;
			}

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			phone_number_type = ( ( wParam & 0x04 ) ? LIST_TYPE_ALLOW : LIST_TYPE_IGNORE );

			if ( wParam & 0x01 )	// Ignore Phone Number.
			{
				edit_aii = NULL;

				if ( phone_number_type == LIST_TYPE_ALLOW )
				{
					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Phone_Number );
					_SendMessageW( g_hWnd_phone_action, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Phone_Number );
				}
				else
				{
					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Phone_Number );
					_SendMessageW( g_hWnd_phone_action, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Phone_Number );
				}

				_SendMessageW( g_hWnd_number, WM_SETTEXT, 0, 0 );
			}
			else if ( wParam & 0x02 )	// Edit Phone Number.
			{
				edit_aii = ( allow_ignore_info * )lParam;

				if ( edit_aii != NULL )
				{
					if ( phone_number_type == LIST_TYPE_ALLOW )
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Allow_Phone_Number );
					}
					else
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Phone_Number );
					}

					_SendMessageW( g_hWnd_phone_action, WM_SETTEXT, 0, ( LPARAM )ST_Update_Phone_Number );

					_SendMessageW( g_hWnd_number, WM_SETTEXT, 0, ( LPARAM )edit_aii->phone_number );
				}
			}

			_SetFocus( g_hWnd_number );

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
			edit_aii = NULL;

			g_hWnd_ignore_phone_number = NULL;

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
