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
#include "utilities.h"
#include "list_operations.h"
#include "string_tables.h"
#include "connection.h"

#define BTN_CID_ACTION			1001
#define EDIT_CID_VALUE			1002
#define EDIT_CID_NUMBER			1003
#define BTN_MATCH_CASE			1004
#define BTN_MATCH_WHOLE_WORD	1005

displayinfo *edit_icid_di = NULL;	// Display the caler ID we want to ignore (when ignoring a caller ID in the call log listview).
ignorecidinfo *edit_cid_ii = NULL;	// Display the caller ID we want to edit (when editing a caller ID in the ignore caller id list listview).

HWND g_hWnd_chk_match_case = NULL;
HWND g_hWnd_chk_match_whole_word = NULL;

bool multiple_icaller_ids = false;	// Change the caller ID if we're adding multiple caller IDs.

WNDPROC CIDEditProc = NULL;

LRESULT CALLBACK CIDEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_CHAR:
		{
			if ( !( _GetKeyState( VK_CONTROL ) & 0x8000 ) && wParam != 0x08 && ( wParam < 0x20 || wParam > 0x7E ) )
			{
				return 0;
			}
		}
		break;

		case WM_PASTE:
		{
			if ( _OpenClipboard( hWnd ) )
			{
				bool ret = true;
				char caller_id[ 16 ];
				_memzero( caller_id, 16 );
				char paste_length = 0;

				// Get ASCII text from the clipboard.
				HANDLE hglbPaste = _GetClipboardData( CF_TEXT );
				if ( hglbPaste != NULL )
				{
					// Get whatever text data is in the clipboard.
					char *lpstrPaste = ( char * )GlobalLock( hglbPaste );
					if ( lpstrPaste != NULL )
					{
						// Find the length of the data and limit it to 15 characters.
						paste_length = ( char )min( 15, lstrlenA( lpstrPaste ) );

						// Copy the data to our caller ID buffer.
						_memcpy_s( caller_id, 16, lpstrPaste, paste_length );
						caller_id[ paste_length ] = 0;	// Sanity.
					}

					GlobalUnlock( hglbPaste );
				}

				_CloseClipboard();

				if ( paste_length > 0 )
				{
					char i = 0;

					if ( paste_length == 16 )
					{
						caller_id[ --paste_length ] = 0;
					}

					// Make sure the phone number contains only numbers or wildcard values.
					for ( ; i < paste_length; ++i )
					{
						if ( caller_id[ i ] != 0x08 && ( caller_id[ i ] < 0x20 || caller_id[ i ] > 0x7E ) )
						{
							return 0;
						}
					}

					// If we have a value and all characters are acceptable, then set the text in the text box and return. Don't perform the default window procedure.
					_SendMessageA( hWnd, EM_REPLACESEL, FALSE, ( LPARAM )caller_id );
					return 0;
				}
			}
		}
		break;
	}
	
	// Everything that we don't handle gets passed back to the parent to process.
	return _CallWindowProcW( CIDEditProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK CIDWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// 0 = ignore
			CHAR enable_items = ( CHAR )( ( CREATESTRUCT * )lParam )->lpCreateParams;

			HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_phone_static1 = _CreateWindowW( WC_STATIC, ST_Caller_ID_Name_, WS_CHILD | WS_VISIBLE, 20, 20, ( rc.right - rc.left ) - 40, 15, hWnd, NULL, NULL, NULL );
			HWND g_hWnd_caller_id = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NOHIDESEL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 35, ( rc.right - rc.left ) - 40, 20, hWnd, ( HMENU )EDIT_CID_VALUE, NULL, NULL );

			g_hWnd_chk_match_case = _CreateWindowW( WC_BUTTON, ST_Match_case, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 60, rc.right - 40, 20, hWnd, ( HMENU )BTN_MATCH_CASE, NULL, NULL );
			g_hWnd_chk_match_whole_word = _CreateWindowW( WC_BUTTON, ST_Match_whole_word, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 80, rc.right - 40, 20, hWnd, ( HMENU )BTN_MATCH_WHOLE_WORD, NULL, NULL );

			HWND g_hWnd_dial_action = _CreateWindowW( WC_BUTTON, ST_Ignore_Caller_ID_Name, WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) - 40 ) / 2, rc.bottom - 32, ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) + 40, 23, hWnd, ( HMENU )BTN_CID_ACTION, NULL, NULL );

			_SendMessageW( g_hWnd_caller_id, EM_LIMITTEXT, 15, 0 );

			_SendMessageW( g_hWnd_chk_match_case, BM_SETCHECK, BST_CHECKED, 0 );
			_SendMessageW( g_hWnd_chk_match_whole_word, BM_SETCHECK, BST_CHECKED, 0 );

			_SendMessageW( g_hWnd_phone_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_caller_id, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_match_case, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_match_whole_word, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_dial_action, WM_SETFONT, ( WPARAM )hFont, 0 );

			CIDEditProc = ( WNDPROC )_GetWindowLongW( g_hWnd_caller_id, GWL_WNDPROC );
			_SetWindowLongW( g_hWnd_caller_id, GWL_WNDPROC, ( LONG )CIDEditSubProc );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_CID_ACTION:
				{
					char caller_id[ 16 ];
					int length = 0;

					if ( hWnd == g_hWnd_ignore_cid && !multiple_icaller_ids )
					{
						length = _SendMessageA( _GetDlgItem( hWnd, EDIT_CID_VALUE ), WM_GETTEXT, 16, ( LPARAM )caller_id );

						if ( length <= 0 )
						{
							_MessageBoxW( hWnd, ST_enter_valid_caller_id, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
							_SetFocus( _GetDlgItem( hWnd, EDIT_CID_VALUE ) );
							break;
						}
					}
					else
					{
						caller_id[ 0 ] = 0;
					}

					bool match_case = ( _SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_CASE ), BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					bool match_whole_word = ( _SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_WHOLE_WORD ), BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					if ( hWnd == g_hWnd_ignore_cid )
					{
						ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidupdateinfo ) );
						icidui->caller_id = NULL;
						icidui->icidi = NULL;
						icidui->action = 0;	// Add = 0, 1 = Remove, 2 = Add all tree items, 3 = Update
						icidui->hWnd = g_hWnd_ignore_cid_list;
						icidui->match_case = match_case;
						icidui->match_whole_word = match_whole_word;

						if ( edit_icid_di != NULL )
						{
							icidui->hWnd = g_hWnd_call_log;
						}
						else if ( edit_cid_ii != NULL )
						{
							icidui->icidi = edit_cid_ii;

							icidui->action = 3;	// Update entry.
						}
						else	// Add entry.
						{
							char *ignore_cid = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length + 1 ) );
							_memcpy_s( ignore_cid, length + 1, caller_id, length );
							ignore_cid[ length ] = 0;	// Sanity

							icidui->caller_id = ignore_cid;
						}

						// icidui is freed in the update_ignore_cid_list thread.
						CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_cid_list, ( void * )icidui, 0, NULL ) );
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case EDIT_CID_NUMBER:
				{
					if ( HIWORD( wParam ) == EN_SETFOCUS )
					{
						_PostMessageW( _GetDlgItem( hWnd, EDIT_CID_NUMBER ), EM_SETSEL, 0, -1 );
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == 1 || wParam == 3 )	// Ignore caller ID.
			{
				edit_cid_ii = NULL;

				edit_icid_di = ( displayinfo * )lParam;
				multiple_icaller_ids = ( wParam == 3 ? true : false );

				_SendMessageW( _GetDlgItem( hWnd, BTN_CID_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Caller_ID_Name );

				_SendMessageW( _GetDlgItem( hWnd, EDIT_CID_NUMBER ), WM_SETTEXT, 0, 0 );
				_SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_CASE ), BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_WHOLE_WORD ), BM_SETCHECK, BST_CHECKED, 0 );

				if ( edit_icid_di != NULL )	// If this is not NULL, then we're adding a caller ID from the call log listview.
				{
					if ( !multiple_icaller_ids )
					{
						_SendMessageA( _GetDlgItem( hWnd, EDIT_CID_VALUE ), WM_SETTEXT, 0, ( LPARAM )edit_icid_di->ci.caller_id );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_CID_VALUE ), WM_SETTEXT, 0, ( LPARAM )ST__multiple_caller_ID_names_ );
					}

					_EnableWindow( _GetDlgItem( hWnd, EDIT_CID_VALUE ), FALSE );		// We don't want to edit this value.
					_SetFocus( _GetDlgItem( hWnd, EDIT_CID_NUMBER ) );
				}
				else	// This shouldn't be NULL.
				{
					_EnableWindow( _GetDlgItem( hWnd, EDIT_CID_VALUE ), TRUE );		// Allow us to edit this value.

					_SendMessageW( _GetDlgItem( hWnd, EDIT_CID_VALUE ), WM_SETTEXT, 0, 0 );

					_SetFocus( _GetDlgItem( hWnd, EDIT_CID_VALUE ) );
				}
			}
			else if ( wParam == 2 )	// Update ignore information.
			{
				multiple_icaller_ids = false;
				edit_icid_di = NULL;

				edit_cid_ii = ( ignorecidinfo * )lParam;

				if ( edit_cid_ii != NULL )	// If not NULL, then allow update.
				{
					_SendMessageW( _GetDlgItem( hWnd, BTN_CID_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Update_Caller_ID_Name );

					_SendMessageA( _GetDlgItem( hWnd, EDIT_CID_VALUE ), WM_SETTEXT, 0, ( LPARAM )edit_cid_ii->c_caller_id );
					_SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_CASE ), BM_SETCHECK, ( edit_cid_ii->match_case ? BST_CHECKED : BST_UNCHECKED ), 0 );
					_SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_WHOLE_WORD ), BM_SETCHECK, ( edit_cid_ii->match_whole_word ? BST_CHECKED : BST_UNCHECKED ), 0 );

					_EnableWindow( _GetDlgItem( hWnd, EDIT_CID_VALUE ), FALSE );	// We don't want to edit this value.

					_SetFocus( _GetDlgItem( hWnd, EDIT_CID_NUMBER ) );
				}
				else	// If NULL, then I suppose we'll just add new numbers.
				{
					_SendMessageW( _GetDlgItem( hWnd, BTN_CID_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Caller_ID_Name );

					_EnableWindow( _GetDlgItem( hWnd, EDIT_CID_VALUE ), TRUE );	// Allow us to edit this value.

					_SendMessageW( _GetDlgItem( hWnd, EDIT_CID_VALUE ), WM_SETTEXT, 0, 0 );
					_SendMessageW( _GetDlgItem( hWnd, EDIT_CID_NUMBER ), WM_SETTEXT, 0, 0 );
					_SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_CASE ), BM_SETCHECK, BST_CHECKED, 0 );
					_SendMessageA( _GetDlgItem( hWnd, BTN_MATCH_WHOLE_WORD ), BM_SETCHECK, BST_CHECKED, 0 );

					_SetFocus( _GetDlgItem( hWnd, EDIT_CID_VALUE ) );
				}
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
			if ( hWnd == g_hWnd_ignore_cid )
			{
				edit_icid_di = NULL;
				edit_cid_ii = NULL;
				multiple_icaller_ids = false;

				g_hWnd_ignore_cid = NULL;
			}

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
