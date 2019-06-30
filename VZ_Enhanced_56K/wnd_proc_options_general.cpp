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

#include "options.h"

#define BTN_TRAY_ICON			1000
#define BTN_MINIMIZE_TO_TRAY	1001
#define BTN_CLOSE_TO_TRAY		1002
#define BTN_SILENT_STARTUP		1003
#define BTN_ALWAYS_ON_TOP		1004
#define BTN_ENABLE_HISTORY		1005
#define BTN_ENABLE_MESSAGE_LOG	1006
#define BTN_CHECK_FOR_UPDATES	1007

#define BTN_ADD_TO_ALLOW		1008
#define BTN_ADD_TO_IGNORE		1009

HWND g_hWnd_chk_tray_icon = NULL;
HWND g_hWnd_chk_minimize = NULL;
HWND g_hWnd_chk_close = NULL;
HWND g_hWnd_chk_silent_startup = NULL;
HWND g_hWnd_chk_always_on_top = NULL;
HWND g_hWnd_chk_enable_history = NULL;
HWND g_hWnd_chk_message_log = NULL;
HWND g_hWnd_chk_check_for_updates = NULL;

HWND g_hWnd_chk_add_to_allow = NULL;
HWND g_hWnd_chk_add_to_ignore = NULL;

LRESULT CALLBACK GeneralTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			g_hWnd_chk_tray_icon = _CreateWindowW( WC_BUTTON, ST_Enable_System_Tray_icon_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 200, 20, hWnd, ( HMENU )BTN_TRAY_ICON, NULL, NULL );
			g_hWnd_chk_minimize = _CreateWindowW( WC_BUTTON, ST_Minimize_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 20, 200, 20, hWnd, ( HMENU )BTN_MINIMIZE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_close = _CreateWindowW( WC_BUTTON, ST_Close_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 40, 200, 20, hWnd, ( HMENU )BTN_CLOSE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_silent_startup = _CreateWindowW( WC_BUTTON, ST_Silent_startup, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 60, 200, 20, hWnd, ( HMENU )BTN_SILENT_STARTUP, NULL, NULL );

			g_hWnd_chk_always_on_top = _CreateWindowW( WC_BUTTON, ST_Always_on_top, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 200, 20, hWnd, ( HMENU )BTN_ALWAYS_ON_TOP, NULL, NULL );

			g_hWnd_chk_enable_history = _CreateWindowW( WC_BUTTON, ST_Enable_Call_Log_history, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 100, 200, 20, hWnd, ( HMENU )BTN_ENABLE_HISTORY, NULL, NULL );

			g_hWnd_chk_message_log = _CreateWindowW( WC_BUTTON, ST_Log_events_to_Message_Log, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, 200, 20, hWnd, ( HMENU )BTN_ENABLE_MESSAGE_LOG, NULL, NULL );

			g_hWnd_chk_check_for_updates = _CreateWindowW( WC_BUTTON, ST_Check_for_updates_upon_startup, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 140, 200, 20, hWnd, ( HMENU )BTN_CHECK_FOR_UPDATES, NULL, NULL );

			HWND hWnd_static_when_allowed = _CreateWindowW( WC_STATIC, ST_When_call_is_allowed_by_its_CID_Name_, WS_CHILD | WS_VISIBLE, 0, 165, 300, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_chk_add_to_allow = _CreateWindowW( WC_BUTTON, ST_Add_phone_number_to_Allow_PN_list, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 180, 350, 20, hWnd, ( HMENU )BTN_ADD_TO_ALLOW, NULL, NULL );

			HWND hWnd_static_when_ignored = _CreateWindowW( WC_STATIC, ST_When_call_is_ignored_by_its_CID_Name_, WS_CHILD | WS_VISIBLE, 0, 205, 300, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_chk_add_to_ignore = _CreateWindowW( WC_BUTTON, ST_Add_phone_number_to_Ignore_PN_list, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 220, 350, 20, hWnd, ( HMENU )BTN_ADD_TO_IGNORE, NULL, NULL );

			_SendMessageW( g_hWnd_chk_tray_icon, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_minimize, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_close, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_silent_startup, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_enable_history, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_message_log, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_check_for_updates, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_when_allowed, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_add_to_allow, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_when_ignored, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_add_to_ignore, WM_SETFONT, ( WPARAM )hFont, 0 );

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
			switch ( LOWORD( wParam ) )
			{
				case BTN_TRAY_ICON:
				{
					if ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_chk_minimize, TRUE );
						_EnableWindow( g_hWnd_chk_close, TRUE );
						_EnableWindow( g_hWnd_chk_silent_startup, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_chk_minimize, FALSE );
						_EnableWindow( g_hWnd_chk_close, FALSE );
						_EnableWindow( g_hWnd_chk_silent_startup, FALSE );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_CLOSE_TO_TRAY:
				case BTN_MINIMIZE_TO_TRAY:
				case BTN_SILENT_STARTUP:
				case BTN_ALWAYS_ON_TOP:
				case BTN_ENABLE_HISTORY:
				case BTN_ENABLE_MESSAGE_LOG:
				case BTN_CHECK_FOR_UPDATES:
				case BTN_ADD_TO_ALLOW:
				case BTN_ADD_TO_IGNORE:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;
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
