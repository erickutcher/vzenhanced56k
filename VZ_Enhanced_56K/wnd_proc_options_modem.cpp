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
#include "ringtone_utilities.h"

#define CB_DEFAULT_MODEM			1000

#define BTN_ENABLE_RECORDING		1001
#define CB_DEFAULT_RECORDING		1002
#define BTN_PLAY_DEFAULT_RECORDING	1003
#define BTN_STOP_DEFAULT_RECORDING	1004

#define EDIT_REPEAT_RECORDING		1005

#define EDIT_DROPTIME				1006

HWND g_hWnd_default_modem = NULL;

HWND g_hWnd_chk_enable_recording = NULL;
HWND g_hWnd_static_default_recording = NULL;
HWND g_hWnd_default_recording = NULL;
HWND g_hWnd_play_default_recording = NULL;
HWND g_hWnd_stop_default_recording = NULL;

HWND g_hWnd_static_repeat_recording = NULL;
HWND g_hWnd_repeat_recording = NULL;
HWND g_hWnd_ud_repeat_recording = NULL;

HWND g_hWnd_droptime = NULL;
HWND g_hWnd_ud_droptime = NULL;

bool options_recording_played = false;

LRESULT CALLBACK ModemTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static_default_modem = _CreateWindowW( WC_STATIC, ST_Default_modem_, WS_CHILD | WS_VISIBLE, 0, 0, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_modem = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 15, 250, 23, hWnd, ( HMENU )CB_DEFAULT_MODEM, NULL, NULL );

			g_hWnd_chk_enable_recording = _CreateWindowW( WC_BUTTON, ST_Play_recording_when_call_is_answered, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 48, 250, 20, hWnd, ( HMENU )BTN_ENABLE_RECORDING, NULL, NULL );
			g_hWnd_static_default_recording = _CreateWindowW( WC_STATIC, ST_Default_recording_, WS_CHILD | WS_VISIBLE, 0, 71, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_recording = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, L"", CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 100, 68, 175, 23, hWnd, ( HMENU )CB_DEFAULT_RECORDING, NULL, NULL );
			g_hWnd_play_default_recording = _CreateWindowW( WC_BUTTON, ST_Play, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 285, 68, 50, 23, hWnd, ( HMENU )BTN_PLAY_DEFAULT_RECORDING, NULL, NULL );
			g_hWnd_stop_default_recording = _CreateWindowW( WC_BUTTON, ST_Stop, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 340, 68, 50, 23, hWnd, ( HMENU )BTN_STOP_DEFAULT_RECORDING, NULL, NULL );


			g_hWnd_static_repeat_recording = _CreateWindowW( WC_STATIC, ST_Repeat_recording_, WS_CHILD | WS_VISIBLE, 0, 96, 200, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_repeat_recording = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 111, 60, 23, hWnd, ( HMENU )EDIT_REPEAT_RECORDING, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_repeat_recording = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 110, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_repeat_recording, EM_LIMITTEXT, 2, 0 );
			_SendMessageW( g_hWnd_ud_repeat_recording, UDM_SETBUDDY, ( WPARAM )g_hWnd_repeat_recording, 0 );
            _SendMessageW( g_hWnd_ud_repeat_recording, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_repeat_recording, UDM_SETRANGE32, 1, 10 );


			HWND hWnd_static_droptime = _CreateWindowW( WC_STATIC, ST_Drop_answered_calls_after__seconds__, WS_CHILD | WS_VISIBLE, 0, 144, 200, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_droptime = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 159, 60, 23, hWnd, ( HMENU )EDIT_DROPTIME, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_droptime = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 158, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_droptime, EM_LIMITTEXT, 2, 0 );
			_SendMessageW( g_hWnd_ud_droptime, UDM_SETBUDDY, ( WPARAM )g_hWnd_droptime, 0 );
            _SendMessageW( g_hWnd_ud_droptime, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_droptime, UDM_SETRANGE32, 1, 10 );

			HWND hWnd_static_droptime_info = _CreateWindowW( WC_STATIC, ST_Recordings_take_priority_if_longer_, WS_CHILD | WS_VISIBLE, 70 + _GetSystemMetrics( SM_CXVSCROLL ), 165, 200, 15, hWnd, NULL, NULL, NULL );


			_SendMessageW( hWnd_static_default_modem, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_modem, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_enable_recording, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_default_recording, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_recording, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_play_default_recording, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_stop_default_recording, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_repeat_recording, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_repeat_recording, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_droptime, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_droptime, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_droptime_info, WM_SETFONT, ( WPARAM )hFont, 0 );

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
				case EDIT_REPEAT_RECORDING:
				{
					DWORD sel_start = 0;
					char value[ 11 ];

					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 3, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 10 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"10" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_repeat_recording )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
					else if ( HIWORD( wParam ) == EN_KILLFOCUS )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 3, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num <= 0 || num > 10 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_repeat_recording )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_DROPTIME:
				{
					DWORD sel_start = 0;
					char value[ 11 ];

					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 3, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 10 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"10" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_drop_call_wait )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
					else if ( HIWORD( wParam ) == EN_KILLFOCUS )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 3, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num <= 0 || num > 10 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"10" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_drop_call_wait )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case CB_DEFAULT_MODEM:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						// See if the modem we selected is not the default that was loaded during startup.
						int current_modem_index = ( int )_SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 );
						if ( current_modem_index != CB_ERR )
						{
							modem_info *mi = ( modem_info * )_SendMessageW( ( HWND )lParam, CB_GETITEMDATA, current_modem_index, 0 );
							if ( mi != default_modem )
							{
								_MessageBoxW( hWnd, ST_must_restart_program, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
							}
						}

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case BTN_ENABLE_RECORDING:
				{
					BOOL enabled = ( _SendMessageW( g_hWnd_chk_enable_recording, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_static_default_recording, enabled );

					if ( _SendMessageW( g_hWnd_default_recording, CB_GETCOUNT, 0, 0 ) == 0 )
					{
						_EnableWindow( g_hWnd_default_recording, FALSE );
						_EnableWindow( g_hWnd_play_default_recording, FALSE );
						_EnableWindow( g_hWnd_stop_default_recording, FALSE );
					}
					else
					{
						_EnableWindow( g_hWnd_default_recording, enabled );
						_EnableWindow( g_hWnd_play_default_recording, enabled );
						_EnableWindow( g_hWnd_stop_default_recording, enabled );
					}

					_EnableWindow( g_hWnd_static_repeat_recording, enabled );
					_EnableWindow( g_hWnd_repeat_recording, enabled );
					_EnableWindow( g_hWnd_ud_repeat_recording, enabled );

					if ( options_recording_played )
					{
						HandleRingtone( RINGTONE_CLOSE, NULL, L"telephony" );

						options_recording_played = false;
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case CB_DEFAULT_RECORDING:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						if ( options_recording_played )
						{
							HandleRingtone( RINGTONE_CLOSE, NULL, L"telephony" );

							options_recording_played = false;
						}

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case BTN_PLAY_DEFAULT_RECORDING:
				{
					if ( options_recording_played )
					{
						HandleRingtone( RINGTONE_CLOSE, NULL, L"telephony" );

						options_recording_played = false;
					}

					int current_recording_index = ( int )_SendMessageW( g_hWnd_default_recording, CB_GETCURSEL, 0, 0 );
					if ( current_recording_index != CB_ERR )
					{
						ringtone_info *rti = ( ringtone_info * )_SendMessageW( g_hWnd_default_recording, CB_GETITEMDATA, current_recording_index, 0 );
						if ( rti != NULL && rti != ( ringtone_info * )CB_ERR )
						{
							HandleRingtone( RINGTONE_PLAY, rti->ringtone_path, L"telephony" );

							options_recording_played = true;
						}
					}
				}
				break;

				case BTN_STOP_DEFAULT_RECORDING:
				{
					if ( options_recording_played )
					{
						HandleRingtone( RINGTONE_CLOSE, NULL, L"telephony" );

						options_recording_played = false;
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( options_recording_played )
			{
				HandleRingtone( RINGTONE_CLOSE, NULL, L"telephony" );

				options_recording_played = false;
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
