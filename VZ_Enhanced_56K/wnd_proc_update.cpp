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
#include "string_tables.h"
#include "connection.h"
#include "lite_ole32.h"
#include "message_log_utilities.h"

#define BTN_UPDATE_DOWNLOAD		1000
#define BTN_UPDATE_CANCEL		1001

HWND g_hWnd_update = NULL;

HWND g_hWnd_edit_update_info = NULL;
HWND g_hWnd_static_update_info = NULL;

HWND g_hWnd_download_update = NULL;
HWND g_hWnd_download_cancel = NULL;

UPDATE_CHECK_INFO *g_update_check_info = NULL;

unsigned char update_check_state = 0;

LRESULT CALLBACK UpdateWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			g_hWnd_edit_update_info = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_READONLY | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_update_info = _CreateWindowW( WC_STATIC, ST_Checking_for_updates___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_download_update = _CreateWindowW( WC_BUTTON, ST_Download_Update, WS_CHILD | WS_TABSTOP | WS_DISABLED | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_UPDATE_DOWNLOAD, NULL, NULL );
			g_hWnd_download_cancel = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_UPDATE_CANCEL, NULL, NULL );

			_SendMessageW( g_hWnd_edit_update_info, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_update_info, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_download_update, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_download_cancel, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our controls to move in relation to the parent window.
			HDWP hdwp = _BeginDeferWindowPos( 4 );

			_DeferWindowPos( hdwp, g_hWnd_download_cancel, HWND_TOP, rc.right - 90, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_download_update, HWND_TOP, rc.right - 205, rc.bottom - 32, 110, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_update_info, HWND_TOP, 10, rc.bottom - 27, 190, 20, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_update_info, HWND_TOP, 10, 10, rc.right - 20, rc.bottom - 52, SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = 510;
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = 240;

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			if ( ( HWND )lParam == g_hWnd_edit_update_info )
			{
				return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
			}
			else
			{
				return _DefWindowProcW( hWnd, msg, wParam, lParam );
			}
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_UPDATE_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_UPDATE_DOWNLOAD:
				{
					_EnableWindow( g_hWnd_download_update, FALSE );

					// g_update_check_info is freed in CheckForUpdates.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, ( void * )g_update_check_info, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						MESSAGE_LOG_OUTPUT( ML_WARNING, ST_The_download_has_failed_ )

						_SendMessageW( hWnd, WM_ALERT, 1, NULL );

						if ( g_update_check_info != NULL )
						{
							GlobalFree( g_update_check_info->notes );
							GlobalFree( g_update_check_info->download_url );
							GlobalFree( g_update_check_info );
						}
					}

					g_update_check_info = NULL;
				}
				break;
			}

			return 0;
		}
		break;

		case WM_ALERT:
		{
			_ShowWindow( hWnd, SW_SHOWNORMAL );
			_SetForegroundWindow( hWnd );

			bool open_homepage = false;

			if ( wParam == 0 )	// Problem checking for updates.
			{
				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Update_Check_Failed );
				_SendMessageW( g_hWnd_static_update_info, WM_SETTEXT, 0, ( LPARAM )ST_The_update_check_has_failed_ );

				if ( _MessageBoxW( hWnd, ST_PROMPT_update_check_failed, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
				{
					open_homepage = true;
				}
			}
			else if ( wParam == 1 )	// Problem downloading.
			{
				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Download_Failed );
				_SendMessageW( g_hWnd_static_update_info, WM_SETTEXT, 0, ( LPARAM )ST_The_download_has_failed_ );

				if ( _MessageBoxW( hWnd, ST_PROMPT_download_failed, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
				{
					open_homepage = true;
				}
			}

			if ( open_homepage )
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
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == 0 )	// Up to date.
			{
				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Up_To_Date );
				_SendMessageW( g_hWnd_static_update_info, WM_SETTEXT, 0, ( LPARAM )ST_VZ_Enhanced_56K_is_up_to_date_ );

				// Destroy the window if it's an automatic update check.
				if ( update_check_state == 2 )
				{
					_DestroyWindow( hWnd );

					break;
				}
			}
			else if ( wParam == 1 )	// New version.
			{
				if ( lParam != NULL )
				{
					g_update_check_info = ( UPDATE_CHECK_INFO * )lParam;

					char value[ 11 ];
					__snprintf( value, 11, "%lu", g_update_check_info->version );

					_SendMessageW( g_hWnd_edit_update_info, WM_SETTEXT, 0, ( LPARAM )ST__VERSION_ );

					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, -1 );
					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, -1, 0 );
					_SendMessageA( g_hWnd_edit_update_info, EM_REPLACESEL, 0, ( LPARAM )value );

					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, -1 );
					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, -1, 0 );
					_SendMessageA( g_hWnd_edit_update_info, EM_REPLACESEL, 0, ( LPARAM )"\r\n\r\n" );

					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, -1 );
					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, -1, 0 );
					_SendMessageW( g_hWnd_edit_update_info, EM_REPLACESEL, 0, ( LPARAM )ST__DOWNLOAD_URL_ );

					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, -1 );
					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, -1, 0 );
					_SendMessageA( g_hWnd_edit_update_info, EM_REPLACESEL, 0, ( LPARAM )g_update_check_info->download_url );

					if ( g_update_check_info->notes != NULL )
					{
						_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, -1 );
						_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, -1, 0 );
						_SendMessageA( g_hWnd_edit_update_info, EM_REPLACESEL, 0, ( LPARAM )"\r\n\r\n" );

						_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, -1 );
						_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, -1, 0 );
						_SendMessageW( g_hWnd_edit_update_info, EM_REPLACESEL, 0, ( LPARAM )ST__NOTES_ );

						_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, -1 );
						_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, -1, 0 );
						_SendMessageA( g_hWnd_edit_update_info, EM_REPLACESEL, 0, ( LPARAM )g_update_check_info->notes );
					}

					_SendMessageA( g_hWnd_edit_update_info, EM_SETSEL, 0, 0 );
					_SendMessageA( g_hWnd_edit_update_info, EM_SCROLLCARET, 0, 0 );

					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Download_Update );
					_SendMessageW( g_hWnd_static_update_info, WM_SETTEXT, 0, ( LPARAM )ST_A_new_version_is_available_ );

					_EnableWindow( g_hWnd_download_update, TRUE );
				}
			}
			else if ( wParam == 2 )	// Downloading.
			{
				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Downloading_Update___ );
				_SendMessageW( g_hWnd_static_update_info, WM_SETTEXT, 0, ( LPARAM )ST_Downloading_the_update___ );
			}
			else if ( wParam == 3 )	// Downloaded.
			{
				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Download_Complete );
				_SendMessageW( g_hWnd_static_update_info, WM_SETTEXT, 0, ( LPARAM )ST_The_download_has_completed_ );
			}

			_ShowWindow( hWnd, SW_SHOWNORMAL );
			_SetForegroundWindow( hWnd );
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
			if ( update_con.state == CONNECTION_ACTIVE )
			{
				// Forceful shutdown.
				update_con.state = CONNECTION_CANCEL;
				CleanupConnection( &update_con );
			}

			if ( g_update_check_info != NULL )
			{
				GlobalFree( g_update_check_info->notes );
				GlobalFree( g_update_check_info->download_url );
				GlobalFree( g_update_check_info );

				g_update_check_info = NULL;
			}

			g_hWnd_update = NULL;

			return 0;
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

            return FALSE;
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
