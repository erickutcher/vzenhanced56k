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
#include "utilities.h"
#include "message_log_utilities.h"

#include "lite_comdlg32.h"
#include "lite_gdi32.h"
#include "lite_advapi32.h"
#include "lite_comctl32.h"

#define BTN_BACKGROUND_COLOR		1000
#define BTN_FONT_COLOR				1001
#define BTN_CHOOSE_FONT				1002
#define BTN_PREVIEW					1003
#define EDIT_HEIGHT					1004
#define EDIT_WIDTH					1005
#define EDIT_TIME					1006
#define EDIT_TRANSPARENCY			1007
#define BTN_GRADIENT				1008
#define BTN_BACKGROUND_COLOR2		1009
#define BTN_BORDER					1010
#define BTN_SHADOW					1011
#define BTN_FONT_SHADOW_COLOR		1012
#define RADIO_LEFT					1013
#define RADIO_CENTER				1014
#define RADIO_RIGHT					1015
#define LIST_SETTINGS				1016
#define BTN_MOVE_UP					1017
#define	BTN_MOVE_DOWN				1018

#define RADIO_GRADIENT_HORZ			1019
#define RADIO_GRADIENT_VERT			1020

#define RADIO_12_HOUR				1021
#define RADIO_24_HOUR				1022

#define CB_POSITION					1023
#define BTN_CONTACT_PICTURE			1024

#define	BTN_ENABLE_RINGTONES		1025
#define CB_DEFAULT_RINGTONE			1026
#define BTN_PLAY_DEFAULT_RINGTONE	1027
#define BTN_STOP_DEFAULT_RINGTONE	1028

#define BTN_ENABLE_POPUPS			1029

#define TVN_CHECKSTATECHANGE		WM_APP + 6

// This is defined in <lmcons.h>
#define UNLEN 256

HWND g_hWnd_chk_enable_popups = NULL;

HWND g_hWnd_static_hoz1 = NULL;
HWND g_hWnd_static_hoz2 = NULL;

HWND g_hWnd_height = NULL;
HWND g_hWnd_width = NULL;
HWND g_hWnd_ud_height = NULL;
HWND g_hWnd_ud_width = NULL;
HWND g_hWnd_position = NULL;
HWND g_hWnd_chk_contact_picture = NULL;
HWND g_hWnd_static_example = NULL;
HWND g_hWnd_time = NULL;
HWND g_hWnd_ud_time = NULL;
HWND g_hWnd_transparency = NULL;
HWND g_hWnd_ud_transparency = NULL;
HWND g_hWnd_chk_gradient = NULL;
HWND g_hWnd_btn_background_color2 = NULL;
HWND g_hWnd_chk_border = NULL;
HWND g_hWnd_chk_shadow = NULL;
HWND g_hWnd_shadow_color = NULL;

HWND g_hWnd_font_color = NULL;
HWND g_hWnd_btn_font_color = NULL;
HWND g_hWnd_font = NULL;
HWND g_hWnd_btn_font = NULL;

HWND g_hWnd_font_settings = NULL;
HWND g_hWnd_move_up = NULL;
HWND g_hWnd_move_down = NULL;

HWND g_hWnd_group_justify = NULL;
HWND g_hWnd_rad_left_justify = NULL;
HWND g_hWnd_rad_center_justify = NULL;
HWND g_hWnd_rad_right_justify = NULL;

HWND g_hWnd_group_gradient_direction = NULL;
HWND g_hWnd_rad_gradient_horz = NULL;
HWND g_hWnd_rad_gradient_vert = NULL;

HWND g_hWnd_group_time_format = NULL;
HWND g_hWnd_rad_12_hour = NULL;
HWND g_hWnd_rad_24_hour = NULL;

HWND g_hWnd_btn_font_shadow_color = NULL;

HWND g_hWnd_static_width = NULL;
HWND g_hWnd_static_height = NULL;
HWND g_hWnd_static_position = NULL;
HWND g_hWnd_static_delay = NULL;
HWND g_hWnd_static_transparency = NULL;
HWND g_hWnd_static_background_color = NULL;
HWND g_hWnd_background_color = NULL;
HWND g_hWnd_preview = NULL;

HWND g_hWnd_chk_enable_ringtones = NULL;
HWND g_hWnd_static_default_ringtone = NULL;
HWND g_hWnd_default_ringtone = NULL;
HWND g_hWnd_play_default_ringtone = NULL;
HWND g_hWnd_stop_default_ringtone = NULL;

DoublyLinkedList *g_ll = NULL;

bool options_ringtone_played = false;

HTREEITEM hti_selected = NULL;	// The currently selected treeview item.

void Enable_Disable_Windows( POPUP_SETTINGS *ps )
{
	if ( ps->popup_info.line_order > 0 )
	{
		_EnableWindow( g_hWnd_btn_font_shadow_color, ( ps->popup_info.font_shadow ? TRUE : FALSE ) );

		_EnableWindow( g_hWnd_chk_shadow, TRUE );
		_EnableWindow( g_hWnd_static_example, TRUE );
		_EnableWindow( g_hWnd_font_color, TRUE );
		_EnableWindow( g_hWnd_btn_font_color, TRUE );
		_EnableWindow( g_hWnd_font, TRUE );
		_EnableWindow( g_hWnd_btn_font, TRUE );
		_EnableWindow( g_hWnd_group_justify, TRUE );
		_EnableWindow( g_hWnd_rad_left_justify, TRUE );
		_EnableWindow( g_hWnd_rad_center_justify, TRUE );
		_EnableWindow( g_hWnd_rad_right_justify, TRUE );

		if ( _abs( ps->popup_info.line_order ) == LINE_TIME )
		{
			_EnableWindow( g_hWnd_group_time_format, TRUE );
			_EnableWindow( g_hWnd_rad_12_hour, TRUE );
			_EnableWindow( g_hWnd_rad_24_hour, TRUE );
		}
	}
	else
	{
		_EnableWindow( g_hWnd_chk_shadow, FALSE );
		_EnableWindow( g_hWnd_btn_font_shadow_color, FALSE );
		_EnableWindow( g_hWnd_static_example, FALSE );
		_EnableWindow( g_hWnd_font, FALSE );
		_EnableWindow( g_hWnd_btn_font, FALSE );
		_EnableWindow( g_hWnd_font_color, FALSE );
		_EnableWindow( g_hWnd_btn_font_color, FALSE );
		_EnableWindow( g_hWnd_group_justify, FALSE );
		_EnableWindow( g_hWnd_rad_left_justify, FALSE );
		_EnableWindow( g_hWnd_rad_center_justify, FALSE );
		_EnableWindow( g_hWnd_rad_right_justify, FALSE );

		if ( _abs( ps->popup_info.line_order ) == LINE_TIME )
		{
			_EnableWindow( g_hWnd_group_time_format, FALSE );
			_EnableWindow( g_hWnd_rad_12_hour, FALSE );
			_EnableWindow( g_hWnd_rad_24_hour, FALSE );
		}
	}
}

void Enable_Disable_Popup_Windows( BOOL enable )
{
	_EnableWindow( g_hWnd_static_hoz1, enable );
	_EnableWindow( g_hWnd_static_hoz2, enable );

	_EnableWindow( g_hWnd_static_width, enable );
	_EnableWindow( g_hWnd_static_height, enable );
	_EnableWindow( g_hWnd_static_position, enable );
	_EnableWindow( g_hWnd_static_delay, enable );
	_EnableWindow( g_hWnd_static_transparency, enable );
	//_EnableWindow( g_hWnd_static_sample, enable );
	_EnableWindow( g_hWnd_static_background_color, enable );

	_EnableWindow( g_hWnd_height, enable );
	_EnableWindow( g_hWnd_ud_height, enable );
	_EnableWindow( g_hWnd_width, enable );
	_EnableWindow( g_hWnd_ud_width, enable );
	_EnableWindow( g_hWnd_position, enable );
	_EnableWindow( g_hWnd_time, enable );
	_EnableWindow( g_hWnd_ud_time, enable );
	_EnableWindow( g_hWnd_transparency, enable );
	_EnableWindow( g_hWnd_ud_transparency, enable );
	_EnableWindow( g_hWnd_chk_contact_picture, enable );
	_EnableWindow( g_hWnd_chk_border, enable );
	_EnableWindow( g_hWnd_chk_enable_ringtones, enable );

	if ( enable == TRUE && _SendMessageW( g_hWnd_chk_enable_ringtones, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		_EnableWindow( g_hWnd_static_default_ringtone, TRUE );
		_EnableWindow( g_hWnd_default_ringtone, TRUE );

		if ( _SendMessageW( g_hWnd_default_ringtone, CB_GETCURSEL, 0, 0 ) == 0 )
		{
			_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
			_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
		}
		else
		{
			_EnableWindow( g_hWnd_play_default_ringtone, TRUE );
			_EnableWindow( g_hWnd_stop_default_ringtone, TRUE );
		}
	}
	else
	{
		_EnableWindow( g_hWnd_static_default_ringtone, FALSE );
		_EnableWindow( g_hWnd_default_ringtone, FALSE );
		_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
		_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
	}

	_EnableWindow( g_hWnd_font_settings, enable );

	_EnableWindow( g_hWnd_font, enable );
	_EnableWindow( g_hWnd_btn_font, enable );
	_EnableWindow( g_hWnd_font_color, enable );
	_EnableWindow( g_hWnd_btn_font_color, enable );

	_EnableWindow( g_hWnd_shadow_color, enable );
	_EnableWindow( g_hWnd_chk_shadow, enable );
	_EnableWindow( g_hWnd_btn_font_shadow_color, enable );

	_EnableWindow( g_hWnd_group_justify, enable );
	_EnableWindow( g_hWnd_rad_left_justify, enable );
	_EnableWindow( g_hWnd_rad_center_justify, enable );
	_EnableWindow( g_hWnd_rad_right_justify, enable );

	_EnableWindow( g_hWnd_group_time_format, enable );
	_EnableWindow( g_hWnd_rad_12_hour, enable );
	_EnableWindow( g_hWnd_rad_24_hour, enable );

	_EnableWindow( g_hWnd_static_example, enable );

	_EnableWindow( g_hWnd_preview, enable );

	_EnableWindow( g_hWnd_background_color, enable );
	_EnableWindow( g_hWnd_chk_gradient, enable );

	if ( enable == TRUE && _SendMessageW( g_hWnd_chk_gradient, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		_EnableWindow( g_hWnd_btn_background_color2, TRUE );
		_EnableWindow( g_hWnd_group_gradient_direction, TRUE );
		_EnableWindow( g_hWnd_rad_gradient_horz, TRUE );
		_EnableWindow( g_hWnd_rad_gradient_vert, TRUE );
	}
	else
	{
		_EnableWindow( g_hWnd_btn_background_color2, FALSE );
		_EnableWindow( g_hWnd_group_gradient_direction, FALSE );
		_EnableWindow( g_hWnd_rad_gradient_horz, FALSE );
		_EnableWindow( g_hWnd_rad_gradient_vert, FALSE );
	}

	if ( enable == TRUE )
	{
		if ( hti_selected != NULL )
		{
			TVITEM tvi;
			_memzero( &tvi, sizeof( TVITEM ) );
			tvi.mask = TVIF_PARAM;
			tvi.hItem = hti_selected;

			if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
			{
				TREEITEM *ti = ( TREEITEM * )tvi.lParam;
				if ( ti != NULL )
				{
					if ( ti != NULL )
					{
						if ( ti->index == 0 )
						{
							_EnableWindow( g_hWnd_move_up, FALSE );
							_EnableWindow( g_hWnd_move_down, enable );
						}
						else if ( ti->index == 2 )
						{
							_EnableWindow( g_hWnd_move_up, enable );
							_EnableWindow( g_hWnd_move_down, FALSE );
						}
						else
						{
							_EnableWindow( g_hWnd_move_up, enable );
							_EnableWindow( g_hWnd_move_down, enable );
						}
					}

					DoublyLinkedList *ll = ti->node;

					// Enable or disable the appropriate line setting windows.
					if ( ll != NULL && ll->data != NULL )
					{
						Enable_Disable_Windows( ( POPUP_SETTINGS * )ll->data );
					}
				}
			}
		}
	}
	else
	{
		_EnableWindow( g_hWnd_move_up, FALSE );
		_EnableWindow( g_hWnd_move_down, FALSE );
	}
}

LRESULT CALLBACK PopupTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			RECT wa;
			_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );

			g_hWnd_chk_enable_popups = _CreateWindowW( WC_BUTTON, ST_Enable_popup_windows_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 150, 20, hWnd, ( HMENU )BTN_ENABLE_POPUPS, NULL, NULL );

			g_hWnd_static_hoz1 = CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 28, rc.right - 10, 5, hWnd, NULL, NULL, NULL );

			g_hWnd_static_width = _CreateWindowW( WC_STATIC, ST_Width__pixels__, WS_CHILD | WS_VISIBLE, 0, 36, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_width = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 51, 60, 23, hWnd, ( HMENU )EDIT_WIDTH, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_width = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 50, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_width, EM_LIMITTEXT, 4, 0 );
			_SendMessageW( g_hWnd_ud_width, UDM_SETBUDDY, ( WPARAM )g_hWnd_width, 0 );
            _SendMessageW( g_hWnd_ud_width, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_width, UDM_SETRANGE32, 20, wa.right );


			g_hWnd_static_height = _CreateWindowW( WC_STATIC, ST_Height__pixels__, WS_CHILD | WS_VISIBLE, 100, 36, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_height = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 51, 60, 23, hWnd, ( HMENU )EDIT_HEIGHT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_height = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 160, 50, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_height, EM_LIMITTEXT, 4, 0 );
			_SendMessageW( g_hWnd_ud_height, UDM_SETBUDDY, ( WPARAM )g_hWnd_height, 0 );
            _SendMessageW( g_hWnd_ud_height, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_height, UDM_SETRANGE32, 20, wa.bottom );
			

			g_hWnd_static_position = _CreateWindowW( WC_STATIC, ST_Screen_Position_, WS_CHILD | WS_VISIBLE, 200, 36, 140, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_position = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, L"", CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 200, 51, 140, 23, hWnd, ( HMENU )CB_POSITION, NULL, NULL );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Top_Left );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Top_Right );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Bottom_Left );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Bottom_Right );


			g_hWnd_static_transparency = _CreateWindowW( WC_STATIC, ST_Transparency_, WS_CHILD | WS_VISIBLE, 0, 79, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_transparency = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 94, 60, 23, hWnd, ( HMENU )EDIT_TRANSPARENCY, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_transparency = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 93, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_transparency, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_transparency, UDM_SETBUDDY, ( WPARAM )g_hWnd_transparency, 0 );
            _SendMessageW( g_hWnd_ud_transparency, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_transparency, UDM_SETRANGE32, 0, 255 );

			g_hWnd_static_delay = _CreateWindowW( WC_STATIC, ST_Delay_Time__seconds__, WS_CHILD | WS_VISIBLE, 100, 79, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_time = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 94, 60, 23, hWnd, ( HMENU )EDIT_TIME, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_time = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 160, 93, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_time, EM_LIMITTEXT, 3, 0 );	// This should be based on MAX_POPUP_TIME.
			_SendMessageW( g_hWnd_ud_time, UDM_SETBUDDY, ( WPARAM )g_hWnd_time, 0 );
            _SendMessageW( g_hWnd_ud_time, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_time, UDM_SETRANGE32, 0, MAX_POPUP_TIME );


			g_hWnd_chk_border = _CreateWindowW( WC_BUTTON, ST_Hide_window_border, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 122, 130, 20, hWnd, ( HMENU )BTN_BORDER, NULL, NULL );

			g_hWnd_chk_contact_picture = _CreateWindowW( WC_BUTTON, ST_Show_contact_picture, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 142, 130, 20, hWnd, ( HMENU )BTN_CONTACT_PICTURE, NULL, NULL );

			g_hWnd_chk_enable_ringtones = _CreateWindowW( WC_BUTTON, ST_Enable_ringtone_s_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 162, 150, 20, hWnd, ( HMENU )BTN_ENABLE_RINGTONES, NULL, NULL );
			g_hWnd_static_default_ringtone = _CreateWindowW( WC_STATIC, ST_Default_ringtone_, WS_CHILD | WS_VISIBLE, 0, 188, 95, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_ringtone = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, L"", CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 95, 185, 175, 23, hWnd, ( HMENU )CB_DEFAULT_RINGTONE, NULL, NULL );
			_SendMessageW( g_hWnd_default_ringtone, CB_ADDSTRING, 0, ( LPARAM )L"" );
			_SendMessageW( g_hWnd_default_ringtone, CB_SETCURSEL, 0, 0 );
			g_hWnd_play_default_ringtone = _CreateWindowW( WC_BUTTON, ST_Play, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 280, 185, 50, 23, hWnd, ( HMENU )BTN_PLAY_DEFAULT_RINGTONE, NULL, NULL );
			g_hWnd_stop_default_ringtone = _CreateWindowW( WC_BUTTON, ST_Stop, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 335, 185, 50, 23, hWnd, ( HMENU )BTN_STOP_DEFAULT_RINGTONE, NULL, NULL );


			g_hWnd_static_hoz2 = CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 215, rc.right - 10, 5, hWnd, NULL, NULL, NULL );


			g_hWnd_font_settings = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL, TVS_DISABLEDRAGDROP | TVS_FULLROWSELECT | TVS_SHOWSELALWAYS | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 225, 130, 60, hWnd, ( HMENU )LIST_SETTINGS, NULL, NULL );
			_SetWindowLongPtrW( g_hWnd_font_settings, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_font_settings, GWL_STYLE ) | TVS_CHECKBOXES );

			g_hWnd_move_up = _CreateWindowW( WC_BUTTON, L"\u2191"/*ST_Up*/, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 135, 225, 23, 23, hWnd, ( HMENU )BTN_MOVE_UP, NULL, NULL );
			g_hWnd_move_down = _CreateWindowW( WC_BUTTON, L"\u2193"/*ST_Down*/, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 135, 262, 23, 23, hWnd, ( HMENU )BTN_MOVE_DOWN, NULL, NULL );


			g_hWnd_font = _CreateWindowW( WC_STATIC, ST_Font_, WS_CHILD | WS_VISIBLE, 0, 298, 175, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_font = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 295, 35, 23, hWnd, ( HMENU )BTN_CHOOSE_FONT, NULL, NULL );

			g_hWnd_font_color = _CreateWindowW( WC_STATIC, ST_Font_color_, WS_CHILD | WS_VISIBLE, 0, 323, 175, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_font_color = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 320, 35, 23, hWnd, ( HMENU )BTN_FONT_COLOR, NULL, NULL );

			g_hWnd_chk_shadow = _CreateWindowW( WC_BUTTON, ST_Font_shadow_color_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 348, 175, 15, hWnd, ( HMENU )BTN_SHADOW, NULL, NULL );
			g_hWnd_btn_font_shadow_color = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 345, 35, 23, hWnd, ( HMENU )BTN_FONT_SHADOW_COLOR, NULL, NULL );

			g_hWnd_static_background_color = _CreateWindowW( WC_STATIC, ST_Background_color_, WS_CHILD | WS_VISIBLE, 0, 373, 175, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_background_color = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 370, 35, 23, hWnd, ( HMENU )BTN_BACKGROUND_COLOR, NULL, NULL );

			g_hWnd_chk_gradient = _CreateWindowW( WC_BUTTON, ST_Gradient_background_color_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 398, 175, 15, hWnd, ( HMENU )BTN_GRADIENT, NULL, NULL );
			g_hWnd_btn_background_color2 = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 395, 35, 23, hWnd, ( HMENU )BTN_BACKGROUND_COLOR2, NULL, NULL );


			g_hWnd_group_gradient_direction = _CreateWindowW( WC_STATIC, ST_Gradient_direction_, WS_CHILD | WS_VISIBLE, 15, 425, 110, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_rad_gradient_vert = _CreateWindowW( WC_BUTTON, ST_Vertical, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 125, 422, 80, 20, hWnd, ( HMENU )RADIO_GRADIENT_VERT, NULL, NULL );
			g_hWnd_rad_gradient_horz = _CreateWindowW( WC_BUTTON, ST_Horizontal, BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE, 125, 442, 80, 20, hWnd, ( HMENU )RADIO_GRADIENT_HORZ, NULL, NULL );


			g_hWnd_group_justify = _CreateWindowW( WC_STATIC, ST_Justify_text_, WS_CHILD | WS_VISIBLE, 220, 295, 80, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_rad_left_justify = _CreateWindowW( WC_BUTTON, ST_Left, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 300, 292, 80, 20, hWnd, ( HMENU )RADIO_LEFT, NULL, NULL );
			g_hWnd_rad_center_justify = _CreateWindowW( WC_BUTTON, ST_Center, BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE, 300, 312, 80, 20, hWnd, ( HMENU )RADIO_CENTER, NULL, NULL );
			g_hWnd_rad_right_justify = _CreateWindowW( WC_BUTTON, ST_Right, BS_AUTORADIOBUTTON  | WS_CHILD | WS_VISIBLE, 300, 332, 80, 20, hWnd, ( HMENU )RADIO_RIGHT, NULL, NULL );

			g_hWnd_group_time_format = _CreateWindowW( WC_STATIC, ST_Time_format_, WS_CHILD | WS_VISIBLE, 220, 368, 80, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_rad_12_hour = _CreateWindowW( WC_BUTTON, ST_12_hour, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 300, 365, 80, 20, hWnd, ( HMENU )RADIO_12_HOUR, NULL, NULL );
			g_hWnd_rad_24_hour = _CreateWindowW( WC_BUTTON, ST_24_hour, BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE, 300, 385, 80, 20, hWnd, ( HMENU )RADIO_24_HOUR, NULL, NULL );



			g_hWnd_static_example = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 175, 225, 210, 60, hWnd, NULL, NULL, NULL );



			g_hWnd_preview = _CreateWindowW( WC_BUTTON, ST_Preview_Popup, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 285, 0, 100, 23, hWnd, ( HMENU )BTN_PREVIEW, NULL, NULL );



			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 462 + 13;	// Value is the position and height of the bottom most control. Needs 13px more padding for Windows 10.
			si.nPage = ( rc.bottom - rc.top );
			si.nPos = 0;
			_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );


			_SendMessageW( g_hWnd_chk_enable_popups, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_border, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_width, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_height, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_height, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_width, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_position, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_position, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_contact_picture, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_delay, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_time, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_transparency, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_transparency, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_example, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_background_color, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_background_color, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_shadow, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_font_color, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_font_color, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_font, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_font, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_preview, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_gradient, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_background_color2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_font_shadow_color, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_font_shadow_color, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_font_settings, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_move_up, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_move_down, WM_SETFONT, ( WPARAM )hFont, 0 );
			//_SendMessageW( g_hWnd_static_sample, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_group_justify, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_rad_left_justify, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_rad_center_justify, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_rad_right_justify, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_group_gradient_direction, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_rad_gradient_horz, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_rad_gradient_vert, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_group_time_format, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_rad_12_hour, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_rad_24_hour, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_btn_font_color, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_font, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_enable_ringtones, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_default_ringtone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_ringtone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_play_default_ringtone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_stop_default_ringtone, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_MBUTTONUP:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		{
			_SetFocus( hWnd );

			return 0;
		}
		break;

		case WM_MOUSEWHEEL:
		case WM_VSCROLL:
		{
			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			int delta = si.nPos;

			if ( msg == WM_VSCROLL )
			{
				// Only process the standard scroll bar.
				if ( lParam != NULL )
				{
					return _DefWindowProcW( hWnd, msg, wParam, lParam );
				}

				switch ( LOWORD( wParam ) )
				{
					case SB_LINEUP: { si.nPos -= 10; } break;
					case SB_LINEDOWN: { si.nPos += 10; } break;
					case SB_PAGEUP: { si.nPos -= 50; } break;
					case SB_PAGEDOWN: { si.nPos += 50; } break;
					//case SB_THUMBPOSITION:
					case SB_THUMBTRACK: { si.nPos = ( int )HIWORD( wParam ); } break;
					default: { return 0; } break;
				}
			}
			else if ( msg == WM_MOUSEWHEEL )
			{
				si.nPos -= ( GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA ) * 20;
			}

			_SetScrollPos( hWnd, SB_VERT, si.nPos, TRUE );

			si.fMask = SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			if ( si.nPos != delta )
			{
				_ScrollWindow( hWnd, 0, delta - si.nPos, NULL, NULL );
			}

			return 0;
		}
		break;

		// Handles item selection and checkbox clicks.
		case TVN_CHECKSTATECHANGE:
		{
			HTREEITEM  hItemChanged = ( HTREEITEM )lParam;

			if ( hItemChanged != NULL )
			{
				TVITEM tvi;
				_memzero( &tvi, sizeof( TVITEM ) );
				tvi.mask = TVIF_PARAM | TVIF_STATE;
				tvi.hItem = hItemChanged;
				tvi.stateMask = TVIS_STATEIMAGEMASK;

				hti_selected = hItemChanged;

				// Force the selection of the item if they only clicked the checkbox.
				_SendMessageW( g_hWnd_font_settings, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hItemChanged );

				if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
				{
					bool line_state_changed = false;

					TREEITEM *ti = ( TREEITEM * )tvi.lParam;

					if ( ti != NULL )
					{
						DoublyLinkedList *ll = ti->node;
						POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;
						
						//  Unchecked = 1, Checked = 2
						if ( ( tvi.state & INDEXTOSTATEIMAGEMASK( 1 ) && ps->popup_info.line_order > 0 ) || ( tvi.state & INDEXTOSTATEIMAGEMASK( 2 ) && ps->popup_info.line_order < 0 ) )
						{
							ps->popup_info.line_order = -( ps->popup_info.line_order );
							line_state_changed = true;
						}

						if ( ti->index == 0 )
						{
							_EnableWindow( g_hWnd_move_up, FALSE );
							_EnableWindow( g_hWnd_move_down, TRUE );
						}
						else if ( ti->index == 2 )
						{
							_EnableWindow( g_hWnd_move_up, TRUE );
							_EnableWindow( g_hWnd_move_down, FALSE );
						}
						else if ( ti->index == 1 )
						{
							_EnableWindow( g_hWnd_move_up, TRUE );
							_EnableWindow( g_hWnd_move_down, TRUE );
						}

						if ( ps->popup_info.justify_line == 2 )
						{
							_SendMessageW( g_hWnd_rad_right_justify, BM_SETCHECK, BST_CHECKED, 0 );
							_SendMessageW( g_hWnd_rad_center_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
							_SendMessageW( g_hWnd_rad_left_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
						}
						else if ( ps->popup_info.justify_line == 1 )
						{
							_SendMessageW( g_hWnd_rad_right_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
							_SendMessageW( g_hWnd_rad_center_justify, BM_SETCHECK, BST_CHECKED, 0 );
							_SendMessageW( g_hWnd_rad_left_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
						}
						else
						{
							_SendMessageW( g_hWnd_rad_right_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
							_SendMessageW( g_hWnd_rad_center_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
							_SendMessageW( g_hWnd_rad_left_justify, BM_SETCHECK, BST_CHECKED, 0 );
						}

						_SendMessageW( g_hWnd_chk_shadow, BM_SETCHECK, ( ps->popup_info.font_shadow ? BST_CHECKED : BST_UNCHECKED ), 0 );

						if ( _abs( ps->popup_info.line_order ) == LINE_TIME )
						{
							_ShowWindow( g_hWnd_group_time_format, SW_SHOW );
							_ShowWindow( g_hWnd_rad_12_hour, SW_SHOW );
							_ShowWindow( g_hWnd_rad_24_hour, SW_SHOW );
						}
						else
						{
							_ShowWindow( g_hWnd_group_time_format, SW_HIDE );
							_ShowWindow( g_hWnd_rad_12_hour, SW_HIDE );
							_ShowWindow( g_hWnd_rad_24_hour, SW_HIDE );
						}

						Enable_Disable_Windows( ps );

						if ( line_state_changed )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
					else
					{
						_EnableWindow( g_hWnd_move_up, FALSE );
						_EnableWindow( g_hWnd_move_down, FALSE );
					}

					_InvalidateRect( g_hWnd_static_example, NULL, TRUE );
				}
			}
		}
		break;

		case WM_NOTIFY:
		{
			NMHDR *nmhdr = ( NMHDR * )lParam;
			switch ( nmhdr->code )
			{
				case NM_CLICK:
				{
					if ( nmhdr->hwndFrom == g_hWnd_font_settings )
					{
						TVHITTESTINFO tvhti;
						_memzero( &tvhti, sizeof( TVHITTESTINFO ) );

						DWORD dwpos = _GetMessagePos();

						tvhti.pt.x = GET_X_LPARAM( dwpos );
						tvhti.pt.y = GET_Y_LPARAM( dwpos );

						_ScreenToClient( g_hWnd_font_settings, &tvhti.pt );

						_SendMessageW( g_hWnd_font_settings, TVM_HITTEST, 0, ( LPARAM )&tvhti );

						PostMessage( hWnd, TVN_CHECKSTATECHANGE, 0, ( LPARAM )tvhti.hItem );
					}
				}
				break;

				case TVN_KEYDOWN:
				{
					NMTVKEYDOWN *nmtvkd = ( NMTVKEYDOWN * )lParam;

					HTREEITEM hti = NULL;
					if ( nmtvkd->wVKey == VK_DOWN )
					{
						hti = ( HTREEITEM )_SendMessageW( g_hWnd_font_settings, TVM_GETNEXTITEM, TVGN_NEXT, ( LPARAM )hti_selected );
					}
					else if ( nmtvkd->wVKey == VK_UP )
					{
						hti = ( HTREEITEM )_SendMessageW( g_hWnd_font_settings, TVM_GETNEXTITEM, TVGN_PREVIOUS, ( LPARAM )hti_selected );
					}
					else if ( nmtvkd->wVKey == VK_SPACE )
					{
						hti = hti_selected;
					}

					if ( hti != NULL )
					{
						hti_selected = hti;
						PostMessage( hWnd, TVN_CHECKSTATECHANGE, 0, ( LPARAM )hti_selected );
					}

					return TRUE;
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_ENABLE_POPUPS:
				{
					Enable_Disable_Popup_Windows( ( _SendMessageW( g_hWnd_chk_enable_popups, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE ) );

					_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case CB_POSITION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case BTN_MOVE_UP:
				{
					if ( hti_selected != NULL )
					{
						wchar_t item_text1[ 64 ];

						TVITEM tvi;
						_memzero( &tvi, sizeof( TVITEM ) );
						tvi.mask = TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
						tvi.stateMask = TVIS_STATEIMAGEMASK;
						tvi.hItem = hti_selected;
						tvi.pszText = ( LPWSTR )item_text1;
						tvi.cchTextMax = 64;

						if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
						{
							HTREEITEM hti2 = ( HTREEITEM )_SendMessageW( g_hWnd_font_settings, TVM_GETNEXTITEM, TVGN_PREVIOUS, ( LPARAM )hti_selected );

							if ( hti2 != NULL )
							{
								wchar_t item_text2[ 64 ];

								TVITEM tvi2;
								_memzero( &tvi2, sizeof( TVITEM ) );
								tvi2.mask = TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
								tvi2.stateMask = TVIS_STATEIMAGEMASK;
								tvi2.hItem = hti2;
								tvi2.pszText = ( LPWSTR )item_text2;
								tvi2.cchTextMax = 64;

								if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi2 ) == TRUE && tvi2.lParam != NULL )
								{
									TREEITEM *ti1 = ( TREEITEM * )tvi.lParam;
									TREEITEM *ti2 = ( TREEITEM * )tvi2.lParam;

									UINT state = tvi.state;

									--ti1->index;
									tvi.state = tvi2.state;
									tvi.pszText = item_text2;
									tvi.lParam = ( LPARAM )ti2;
									_SendMessageW( g_hWnd_font_settings, TVM_SETITEM, 0, ( LPARAM )&tvi );

									++ti2->index;
									tvi2.state = state;
									tvi2.pszText = item_text1;
									tvi2.lParam = ( LPARAM )ti1;
									_SendMessageW( g_hWnd_font_settings, TVM_SETITEM, 0, ( LPARAM )&tvi2 );

									DoublyLinkedList *ll1 = ti1->node;

									DLL_RemoveNode( &g_ll, ll1 );	// ll1 is not freed.
									DLL_AddNode( &g_ll, ll1, ti1->index );

									hti_selected = hti2;
									_SendMessageW( g_hWnd_font_settings, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti2 );

									if ( ti1->index == 1 )
									{
										_EnableWindow( g_hWnd_move_up, TRUE );
										_EnableWindow( g_hWnd_move_down, TRUE );
									}
									else if ( ti1->index == 0 )
									{
										_EnableWindow( g_hWnd_move_up, FALSE );
										_EnableWindow( g_hWnd_move_down, TRUE );
									}
									else
									{
										_EnableWindow( g_hWnd_move_up, FALSE );
										_EnableWindow( g_hWnd_move_down, FALSE );
									}

									options_state_changed = true;
									_EnableWindow( g_hWnd_apply, TRUE );
								}
							}
						}
					}
					break;
				}
				break;

				case BTN_MOVE_DOWN:
				{
					if ( hti_selected != NULL )
					{
						wchar_t item_text1[ 64 ];

						TVITEM tvi;
						_memzero( &tvi, sizeof( TVITEM ) );
						tvi.mask = TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
						tvi.stateMask = TVIS_STATEIMAGEMASK;
						tvi.hItem = hti_selected;
						tvi.pszText = ( LPWSTR )item_text1;
						tvi.cchTextMax = 64;

						if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
						{
							HTREEITEM hti2 = ( HTREEITEM )_SendMessageW( g_hWnd_font_settings, TVM_GETNEXTITEM, TVGN_NEXT, ( LPARAM )hti_selected );

							if ( hti2 != NULL )
							{
								wchar_t item_text2[ 64 ];

								TVITEM tvi2;
								_memzero( &tvi2, sizeof( TVITEM ) );
								tvi2.mask = TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
								tvi2.stateMask = TVIS_STATEIMAGEMASK;
								tvi2.hItem = hti2;
								tvi2.pszText = ( LPWSTR )item_text2;
								tvi2.cchTextMax = 64;

								if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi2 ) == TRUE && tvi2.lParam != NULL )
								{
									TREEITEM *ti1 = ( TREEITEM * )tvi.lParam;
									TREEITEM *ti2 = ( TREEITEM * )tvi2.lParam;

									UINT state = tvi.state;

									++ti1->index;
									tvi.state = tvi2.state;
									tvi.pszText = item_text2;
									tvi.lParam = ( LPARAM )ti2;
									_SendMessageW( g_hWnd_font_settings, TVM_SETITEM, 0, ( LPARAM )&tvi );

									--ti2->index;
									tvi2.state = state;
									tvi2.pszText = item_text1;
									tvi2.lParam = ( LPARAM )ti1;
									_SendMessageW( g_hWnd_font_settings, TVM_SETITEM, 0, ( LPARAM )&tvi2 );

									DoublyLinkedList *ll1 = ti1->node;

									DLL_RemoveNode( &g_ll, ll1 );	// ll1 is not freed.
									DLL_AddNode( &g_ll, ll1, ti1->index );

									hti_selected = hti2;
									_SendMessageW( g_hWnd_font_settings, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti2 );

									if ( ti1->index == 1 )
									{
										_EnableWindow( g_hWnd_move_up, TRUE );
										_EnableWindow( g_hWnd_move_down, TRUE );
									}
									else if ( ti1->index == 2 )
									{
										_EnableWindow( g_hWnd_move_up, TRUE );
										_EnableWindow( g_hWnd_move_down, FALSE );
									}
									else
									{
										_EnableWindow( g_hWnd_move_up, FALSE );
										_EnableWindow( g_hWnd_move_down, FALSE );
									}

									options_state_changed = true;
									_EnableWindow( g_hWnd_apply, TRUE );
								}
							}
						}
					}
					break;
				}
				break;

				case RADIO_LEFT:
				case RADIO_CENTER:
				case RADIO_RIGHT:
				{
					if ( hti_selected != NULL )
					{
						TVITEM tvi;
						_memzero( &tvi, sizeof( TVITEM ) );
						tvi.mask = TVIF_PARAM;
						tvi.hItem = hti_selected;

						if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
						{
							TREEITEM *ti = ( TREEITEM * )tvi.lParam;
							if ( ti != NULL )
							{
								DoublyLinkedList *ll = ti->node;
								POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;
								ps->popup_info.justify_line = ( LOWORD( wParam ) == RADIO_RIGHT ? 2 : ( LOWORD( wParam ) == RADIO_CENTER ? 1 : 0 ) );

								_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

								options_state_changed = true;
								_EnableWindow( g_hWnd_apply, TRUE );
							}
						}
					}
				}
				break;

				case RADIO_GRADIENT_VERT:
				case RADIO_GRADIENT_HORZ:
				{
					if ( g_ll != NULL && g_ll->data != NULL && ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings != NULL )
					{
						( ( POPUP_SETTINGS * )g_ll->data )->shared_settings->popup_gradient_direction = ( LOWORD( wParam ) == RADIO_GRADIENT_HORZ ? 1 : 0 );

						_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case RADIO_24_HOUR:
				case RADIO_12_HOUR:
				{
					_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_TRANSPARENCY:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 255 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"255" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_popup_transparency )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_WIDTH:
				case EDIT_HEIGHT:
				{
					DWORD sel_start = 0;
					char value[ 11 ];

					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 5, ( LPARAM )value );
						LONG num = _strtoul( value, NULL, 10 );

						RECT wa;
						_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );

						if ( ( HWND )lParam == g_hWnd_height && num > wa.bottom )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							__snprintf( value, 11, "%d", wa.bottom );
							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )value );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( ( HWND )lParam == g_hWnd_width && num > wa.right )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							__snprintf( value, 11, "%d", wa.right );
							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )value );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( ( ( HWND )lParam == g_hWnd_height && num != cfg_popup_height ) || ( ( HWND )lParam == g_hWnd_width && num != cfg_popup_width ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
					else if ( HIWORD( wParam ) == EN_KILLFOCUS )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 5, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( ( ( HWND )lParam == g_hWnd_height || ( HWND )lParam == g_hWnd_width ) && num < 20 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"20" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( ( ( HWND )lParam == g_hWnd_height && num != cfg_popup_height ) || ( ( HWND )lParam == g_hWnd_width && num != cfg_popup_width ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case BTN_PREVIEW:
				{
					// Ensures that ps and ss below are valid and not NULL.
					if ( !check_settings() )
					{
						break;
					}

					DoublyLinkedList *temp_ll = g_ll;
					POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )temp_ll->data;
					SHARED_SETTINGS *ss = ( ( POPUP_SETTINGS * )temp_ll->data )->shared_settings;

					char value[ 5 ];
					_SendMessageA( g_hWnd_width, WM_GETTEXT, 5, ( LPARAM )value );
					int width = _strtoul( value, NULL, 10 );
					_SendMessageA( g_hWnd_height, WM_GETTEXT, 5, ( LPARAM )value );
					int height = _strtoul( value, NULL, 10 );

					RECT wa;
					_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );	

					if ( width > wa.right )
					{
						width = wa.right;
					}

					if ( height > wa.bottom )
					{
						height = wa.bottom;
					}

					int index = ( int )_SendMessageW( g_hWnd_position, CB_GETCURSEL, 0, 0 );

					int left = 0;
					int top = 0;

					if ( index == 0 )		// Top Left
					{
						left = wa.left;
						top = wa.top;
					}
					else if ( index == 1 )	// Top Right
					{
						left = wa.right - width;
						top = wa.top;
					}
					else if ( index == 2 )	// Bottom Left
					{
						left = wa.left;
						top = wa.bottom - height;
					}
					else					// Bottom Right
					{
						left = wa.right - width;
						top = wa.bottom - height;
					}

					_SendMessageA( g_hWnd_time, WM_GETTEXT, 4, ( LPARAM )value );
					int popup_time = ( unsigned short )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_transparency, WM_GETTEXT, 4, ( LPARAM )value );
					int transparency = ( unsigned char )_strtoul( value, NULL, 10 );

					bool show_contact_picture = ( _SendMessageW( g_hWnd_chk_contact_picture, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					bool popup_hide_border = ( _SendMessageW( g_hWnd_chk_border, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					unsigned char popup_time_format = ( _SendMessageW( g_hWnd_rad_24_hour, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					HWND hWnd_popup = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"popup", NULL, WS_CLIPCHILDREN | WS_POPUP | ( popup_hide_border ? NULL : WS_THICKFRAME ), left, top, width, height, g_hWnd_main, NULL, NULL, NULL );

					_SetWindowLongPtrW( hWnd_popup, GWL_EXSTYLE, _GetWindowLongPtrW( hWnd_popup, GWL_EXSTYLE ) | WS_EX_LAYERED );
					_SetLayeredWindowAttributes( hWnd_popup, 0, transparency, LWA_ALPHA );

					// ALL OF THIS IS FREED IN WM_DESTROY OF THE POPUP WINDOW.

					wchar_t *phone_line = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 15 );
					_wmemcpy_s( phone_line, 15, L"(555) 555-1234\0", 15 );
					phone_line[ 14 ] = 0;	// Sanity
					wchar_t *caller_id_line = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( UNLEN + 1 ) );
					_wmemcpy_s( caller_id_line, UNLEN + 1, L"Caller ID\0", 10 );	// Default value if we can't get the username.
					caller_id_line[ 9 ] = 0;	// Sanity.

					DWORD size = UNLEN + 1;

					bool get_username = true;
					#ifndef ADVAPI32_USE_STATIC_LIB
						if ( advapi32_state == ADVAPI32_STATE_SHUTDOWN )
						{
							get_username = InitializeAdvApi32();
						}
					#endif

					if ( get_username )
					{
						_GetUserNameW( caller_id_line, &size );
					}

					wchar_t *program_data_folder = NULL;
					if ( show_contact_picture )
					{
						program_data_folder = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );
						_SHGetFolderPathW( NULL, CSIDL_COMMON_APPDATA, NULL, 0, program_data_folder );

						int program_data_folder_length = lstrlenW( program_data_folder );
						_wmemcpy_s( program_data_folder + program_data_folder_length, MAX_PATH - program_data_folder_length, L"\\Microsoft\\User Account Pictures\\User.bmp\0", 42 );
						program_data_folder[ program_data_folder_length + 41 ] = 0;	// Sanity.
					}

					wchar_t *time_line = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 16 );

					SYSTEMTIME SystemTime;
					GetLocalTime( &SystemTime );

					if ( popup_time_format == 0 )	// 12 hour
					{
						__snwprintf( time_line, 16, L"%d:%02d %s", ( SystemTime.wHour > 12 ? SystemTime.wHour - 12 : ( SystemTime.wHour != 0 ? SystemTime.wHour : 12 ) ), SystemTime.wMinute, ( SystemTime.wHour >= 12 ? L"PM" : L"AM" ) );
					}
					else	// 24 hour
					{
						__snwprintf( time_line, 16, L"%s%d:%02d", ( SystemTime.wHour > 9 ? L"" : L"0" ), SystemTime.wHour, SystemTime.wMinute );
					}

					LOGFONT lf;
					_memzero( &lf, sizeof( LOGFONT ) );

					SHARED_SETTINGS *shared_settings = ( SHARED_SETTINGS * )GlobalAlloc( GPTR, sizeof( SHARED_SETTINGS ) );
					shared_settings->popup_gradient = ss->popup_gradient;
					shared_settings->popup_gradient_direction = ss->popup_gradient_direction;
					shared_settings->popup_background_color1 = ss->popup_background_color1;
					shared_settings->popup_background_color2 = ss->popup_background_color2;
					shared_settings->popup_time = popup_time;
					shared_settings->rti = ss->rti;
					shared_settings->contact_picture_info.free_picture_path = show_contact_picture;
					shared_settings->contact_picture_info.picture_path = program_data_folder;

					DoublyLinkedList *t_ll = NULL;

					while ( temp_ll != NULL )
					{
						ps = ( POPUP_SETTINGS * )temp_ll->data;

						POPUP_SETTINGS *ls = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
						ls->shared_settings = shared_settings;
						ls->window_settings.drag_position = ps->window_settings.drag_position;
						ls->window_settings.is_dragging = ps->window_settings.is_dragging;
						ls->window_settings.window_position = ps->window_settings.window_position;
						_memcpy_s( &ls->popup_info, sizeof( POPUP_INFO ), &ps->popup_info, sizeof( POPUP_INFO ) );
						ls->popup_info.font_face = NULL;
						ls->line_text = ( _abs( ps->popup_info.line_order ) == LINE_PHONE ? phone_line : ( _abs( ps->popup_info.line_order ) == LINE_CALLER_ID ? caller_id_line : time_line ) );

						if ( ps->popup_info.font_face != NULL )
						{
							_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, ps->popup_info.font_face, LF_FACESIZE );
							lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.
							lf.lfHeight = ps->popup_info.font_height;
							lf.lfWeight = ps->popup_info.font_weight;
							lf.lfItalic = ps->popup_info.font_italic;
							lf.lfUnderline = ps->popup_info.font_underline;
							lf.lfStrikeOut = ps->popup_info.font_strikeout;
							ls->font = _CreateFontIndirectW( &lf );
						}
						else
						{
							ls->font = NULL;
						}

						DoublyLinkedList *ll_1 = DLL_CreateNode( ( void * )ls );
						DLL_AddNode( &t_ll, ll_1, -1 );

						temp_ll = temp_ll->next;
					}

					_SetWindowLongPtrW( hWnd_popup, 0, ( LONG_PTR )t_ll );

					_SendMessageW( hWnd_popup, WM_PROPAGATE, 0, 0 );
				}
				break;

				case BTN_BACKGROUND_COLOR:
				case BTN_BACKGROUND_COLOR2:
				{
					if ( g_ll != NULL && g_ll->data != NULL && ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings != NULL )
					{
						SHARED_SETTINGS *shared_settings = ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings;

						CHOOSECOLOR cc;
						_memzero( &cc, sizeof( CHOOSECOLOR ) );
						static COLORREF CustomColors[ 16 ];
						_memzero( &CustomColors, sizeof( CustomColors ) );

						cc.lStructSize = sizeof( CHOOSECOLOR );
						cc.Flags = CC_FULLOPEN | CC_RGBINIT;
						cc.lpCustColors = CustomColors;
						cc.hwndOwner = hWnd;
						cc.rgbResult = ( LOWORD( wParam ) == BTN_BACKGROUND_COLOR ? shared_settings->popup_background_color1 : shared_settings->popup_background_color2 );

						if ( _ChooseColorW( &cc ) == TRUE )
						{
							if ( LOWORD( wParam ) == BTN_BACKGROUND_COLOR )
							{
								shared_settings->popup_background_color1 = cc.rgbResult;
							}
							else
							{
								shared_settings->popup_background_color2 = cc.rgbResult;
							}

							_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case BTN_FONT_SHADOW_COLOR:
				case BTN_FONT_COLOR:
				{
					if ( hti_selected != NULL )
					{
						TVITEM tvi;
						_memzero( &tvi, sizeof( TVITEM ) );
						tvi.mask = TVIF_PARAM;
						tvi.hItem = hti_selected;

						if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
						{
							TREEITEM *ti = ( TREEITEM * )tvi.lParam;
							if ( ti != NULL )
							{
								DoublyLinkedList *ll = ti->node;
								POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;

								CHOOSECOLOR cc;
								_memzero( &cc, sizeof( CHOOSECOLOR ) );
								static COLORREF CustomColors[ 16 ];
								_memzero( &CustomColors, sizeof( CustomColors ) );

								cc.lStructSize = sizeof( CHOOSECOLOR );
								cc.Flags = CC_FULLOPEN | CC_RGBINIT;
								cc.lpCustColors = CustomColors;
								cc.hwndOwner = hWnd;
								cc.rgbResult = ( LOWORD( wParam ) == BTN_FONT_COLOR ? ps->popup_info.font_color : ps->popup_info.font_shadow_color );

								if ( _ChooseColorW( &cc ) == TRUE )
								{
									if ( LOWORD( wParam ) == BTN_FONT_COLOR )
									{
										ps->popup_info.font_color = cc.rgbResult;
									}
									else
									{
										ps->popup_info.font_shadow_color = cc.rgbResult;
									}

									_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

									options_state_changed = true;
									_EnableWindow( g_hWnd_apply, TRUE );
								}
							}
						}
					}
				}
				break;

				case BTN_CHOOSE_FONT:
				{
					if ( hti_selected != NULL )
					{
						TVITEM tvi;
						_memzero( &tvi, sizeof( TVITEM ) );
						tvi.mask = TVIF_PARAM;
						tvi.hItem = hti_selected;

						if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
						{
							TREEITEM *ti = ( TREEITEM * )tvi.lParam;
							if ( ti != NULL )
							{
								DoublyLinkedList *ll = ti->node;
								POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;

								CHOOSEFONT cf;
								_memzero( &cf, sizeof( CHOOSEFONT ) );
								static LOGFONT lf;
								_memzero( &lf, sizeof( LOGFONT ) );

								cf.lStructSize = sizeof( CHOOSEFONT );
								cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
								cf.lpLogFont = &lf;
								cf.hwndOwner = hWnd;

								cf.rgbColors = ps->popup_info.font_color;
								_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, ps->popup_info.font_face, LF_FACESIZE );
								lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.
								lf.lfHeight = ps->popup_info.font_height;
								lf.lfWeight = ps->popup_info.font_weight;
								lf.lfItalic = ps->popup_info.font_italic;
								lf.lfUnderline = ps->popup_info.font_underline;
								lf.lfStrikeOut = ps->popup_info.font_strikeout;

								if ( _ChooseFontW( &cf ) == TRUE )
								{
									if ( ps->popup_info.font_face != NULL )
									{
										GlobalFree( ps->popup_info.font_face );
									}
									ps->popup_info.font_face = GlobalStrDupW( lf.lfFaceName );
									ps->popup_info.font_color = cf.rgbColors;
									ps->popup_info.font_height = lf.lfHeight;
									ps->popup_info.font_weight = lf.lfWeight;
									ps->popup_info.font_italic = lf.lfItalic;
									ps->popup_info.font_underline = lf.lfUnderline;
									ps->popup_info.font_strikeout = lf.lfStrikeOut;

									if ( ps->font != NULL )
									{
										_DeleteObject( ps->font );
									}
									ps->font = _CreateFontIndirectW( cf.lpLogFont );

									_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

									options_state_changed = true;
									_EnableWindow( g_hWnd_apply, TRUE );
								}
							}
						}
					}
				}
				break;

				case BTN_SHADOW:
				{
					if ( hti_selected != NULL )
					{
						TVITEM tvi;
						_memzero( &tvi, sizeof( TVITEM ) );
						tvi.mask = TVIF_PARAM;
						tvi.hItem = hti_selected;

						if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
						{
							TREEITEM *ti = ( TREEITEM * )tvi.lParam;
							if ( ti != NULL )
							{
								DoublyLinkedList *ll = ti->node;
								POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;
								ps->popup_info.font_shadow = ( _SendMessageW( g_hWnd_chk_shadow, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

								if ( ps->popup_info.font_shadow )
								{
									_EnableWindow( g_hWnd_btn_font_shadow_color, TRUE );
								}
								else
								{
									_EnableWindow( g_hWnd_btn_font_shadow_color, FALSE );
								}

								_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

								options_state_changed = true;
								_EnableWindow( g_hWnd_apply, TRUE );
							}
						}
					}
				}
				break;

				case BTN_ENABLE_RINGTONES:
				{
					if ( g_ll != NULL && g_ll->data != NULL && ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings != NULL )
					{
						SHARED_SETTINGS *shared_settings = ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings;

						if ( _SendMessageW( g_hWnd_chk_enable_ringtones, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						{
							_EnableWindow( g_hWnd_static_default_ringtone, TRUE );
							_EnableWindow( g_hWnd_default_ringtone, TRUE );

							int current_ringtone_index = ( int )_SendMessageW( g_hWnd_default_ringtone, CB_GETCURSEL, 0, 0 );
							if ( current_ringtone_index == 0 )
							{
								shared_settings->rti = NULL;

								_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
								_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
							}
							else
							{
								ringtone_info *rti = ( ringtone_info * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
								if ( rti != NULL && rti != ( ringtone_info * )CB_ERR )
								{
									shared_settings->rti = rti;
								}
								else
								{
									shared_settings->rti = NULL;
								}

								_EnableWindow( g_hWnd_play_default_ringtone, TRUE );
								_EnableWindow( g_hWnd_stop_default_ringtone, TRUE );
							}
						}
						else
						{
							shared_settings->rti = NULL;

							_EnableWindow( g_hWnd_static_default_ringtone, FALSE );
							_EnableWindow( g_hWnd_default_ringtone, FALSE );
							_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
							_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
						}

						if ( options_ringtone_played )
						{
							HandleRingtone( RINGTONE_CLOSE, NULL, L"options" );

							options_ringtone_played = false;
						}

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case CB_DEFAULT_RINGTONE:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						if ( g_ll != NULL && g_ll->data != NULL && ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings != NULL )
						{
							SHARED_SETTINGS *shared_settings = ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings;

							int current_ringtone_index = ( int )_SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 );
							if ( current_ringtone_index == 0 )
							{
								shared_settings->rti = NULL;

								_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
								_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
							}
							else
							{
								ringtone_info *rti = ( ringtone_info * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
								shared_settings->rti = ( ( rti != NULL && rti != ( ringtone_info * )CB_ERR ) ? rti : NULL );

								_EnableWindow( g_hWnd_play_default_ringtone, TRUE );
								_EnableWindow( g_hWnd_stop_default_ringtone, TRUE );
							}

							if ( options_ringtone_played )
							{
								HandleRingtone( RINGTONE_CLOSE, NULL, L"options" );

								options_ringtone_played = false;
							}

							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case BTN_PLAY_DEFAULT_RINGTONE:
				{
					if ( options_ringtone_played )
					{
						HandleRingtone( RINGTONE_CLOSE, NULL, L"options" );

						options_ringtone_played = false;
					}

					int current_ringtone_index = ( int )_SendMessageW( g_hWnd_default_ringtone, CB_GETCURSEL, 0, 0 );
					if ( current_ringtone_index > 0 )
					{
						ringtone_info *rti = ( ringtone_info * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
						if ( rti != NULL && rti != ( ringtone_info * )CB_ERR )
						{
							HandleRingtone( RINGTONE_PLAY, rti->ringtone_path, L"options" );

							options_ringtone_played = true;
						}
					}
				}
				break;

				case BTN_STOP_DEFAULT_RINGTONE:
				{
					if ( options_ringtone_played )
					{
						HandleRingtone( RINGTONE_CLOSE, NULL, L"options" );

						options_ringtone_played = false;
					}
				}
				break;

				case BTN_CONTACT_PICTURE:
				case BTN_BORDER:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_GRADIENT:
				{
					if ( g_ll != NULL && g_ll->data != NULL && ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings != NULL )
					{
						SHARED_SETTINGS *shared_settings = ( ( POPUP_SETTINGS * )g_ll->data )->shared_settings;

						shared_settings->popup_gradient = ( _SendMessageW( g_hWnd_chk_gradient, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						if ( shared_settings->popup_gradient )
						{
							_EnableWindow( g_hWnd_btn_background_color2, TRUE );
							_EnableWindow( g_hWnd_group_gradient_direction, TRUE );
							_EnableWindow( g_hWnd_rad_gradient_horz, TRUE );
							_EnableWindow( g_hWnd_rad_gradient_vert, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_btn_background_color2, FALSE );
							_EnableWindow( g_hWnd_group_gradient_direction, FALSE );
							_EnableWindow( g_hWnd_rad_gradient_horz, FALSE );
							_EnableWindow( g_hWnd_rad_gradient_vert, FALSE );
						}

						_InvalidateRect( g_hWnd_static_example, NULL, TRUE );

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case EDIT_TIME:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > MAX_POPUP_TIME )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							__snprintf( value, 11, "%d", MAX_POPUP_TIME );
							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )value );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_popup_time )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_STATIC && dis->hwndItem == g_hWnd_static_example )
			{
				if ( hti_selected != NULL )
				{
					TVITEM tvi;
					_memzero( &tvi, sizeof( TVITEM ) );
					tvi.mask = TVIF_PARAM;
					tvi.hItem = hti_selected;

					if ( _SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi ) == TRUE && tvi.lParam != NULL )
					{
						TREEITEM *ti = ( TREEITEM * )tvi.lParam;
						if ( ti != NULL )
						{
							DoublyLinkedList *ll = ti->node;
							// Create and save a bitmap in memory to paint to.
							HDC hdcMem = _CreateCompatibleDC( dis->hDC );
							HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top );
							HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
							_DeleteObject( ohbm );
							_DeleteObject( hbm );

							if ( ll != NULL && ll->data != NULL && ( ( POPUP_SETTINGS * )ll->data )->shared_settings != NULL && _SendMessageW( g_hWnd_chk_enable_popups, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
							{
								POPUP_SETTINGS *p_s = ( POPUP_SETTINGS * )ll->data;

								if ( p_s->popup_info.line_order > 0 )
								{

									SHARED_SETTINGS *shared_settings = ( ( POPUP_SETTINGS * )ll->data )->shared_settings;

									if ( !shared_settings->popup_gradient )
									{
										HBRUSH background = _CreateSolidBrush( shared_settings->popup_background_color1 );
										_FillRect( hdcMem, &dis->rcItem, background );
										_DeleteObject( background );
									}
									else
									{
										// Stupid thing wants big-endian.
										TRIVERTEX vertex[ 2 ];
										vertex[ 0 ].x = 0;
										vertex[ 0 ].y = 0;
										vertex[ 0 ].Red = ( COLOR16 )GetRValue( shared_settings->popup_background_color1 ) << 8;
										vertex[ 0 ].Green = ( COLOR16 )GetGValue( shared_settings->popup_background_color1 ) << 8;
										vertex[ 0 ].Blue  = ( COLOR16 )GetBValue( shared_settings->popup_background_color1 ) << 8;
										vertex[ 0 ].Alpha = 0x0000;

										vertex[ 1 ].x = dis->rcItem.right - dis->rcItem.left;
										vertex[ 1 ].y = dis->rcItem.bottom - dis->rcItem.top; 
										vertex[ 1 ].Red = ( COLOR16 )GetRValue( shared_settings->popup_background_color2 ) << 8;
										vertex[ 1 ].Green = ( COLOR16 )GetGValue( shared_settings->popup_background_color2 ) << 8;
										vertex[ 1 ].Blue  = ( COLOR16 )GetBValue( shared_settings->popup_background_color2 ) << 8;
										vertex[ 1 ].Alpha = 0x0000;

										// Create a GRADIENT_RECT structure that references the TRIVERTEX vertices.
										GRADIENT_RECT gRc;
										gRc.UpperLeft = 0;
										gRc.LowerRight = 1;

										// Draw a shaded rectangle. 
										_GdiGradientFill( hdcMem, vertex, 2, &gRc, 1, ( shared_settings->popup_gradient_direction == 1 ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V ) );
									}

									HFONT ohf = ( HFONT )_SelectObject( hdcMem, p_s->font );
									_DeleteObject( ohf );

									// Transparent background for text.
									_SetBkMode( hdcMem, TRANSPARENT );

									RECT rc_line = dis->rcItem;
									_DrawTextW( hdcMem, L"AaBbYyZz", -1, &rc_line, DT_CALCRECT );
									rc_line.right = dis->rcItem.right - 5;
									rc_line.left = 5;
									LONG tmp_height = rc_line.bottom - rc_line.top;
									rc_line.top = ( dis->rcItem.bottom - ( rc_line.bottom - rc_line.top ) - 5 ) / 2;
									rc_line.bottom = rc_line.top + tmp_height;

									if ( p_s->popup_info.font_shadow )
									{
										RECT rc_shadow_line = rc_line;
										rc_shadow_line.left += 2;
										rc_shadow_line.top += 2;
										rc_shadow_line.right += 2;
										rc_shadow_line.bottom += 2;

										_SetTextColor( hdcMem, p_s->popup_info.font_shadow_color );

										_DrawTextW( hdcMem, L"AaBbYyZz", -1, &rc_shadow_line, ( p_s->popup_info.justify_line == 0 ? DT_LEFT : ( p_s->popup_info.justify_line == 1 ? DT_CENTER : DT_RIGHT ) ) );
									}

									_SetTextColor( hdcMem, p_s->popup_info.font_color );

									_DrawTextW( hdcMem, L"AaBbYyZz", -1, &rc_line, ( p_s->popup_info.justify_line == 0 ? DT_LEFT : ( p_s->popup_info.justify_line == 1 ? DT_CENTER : DT_RIGHT ) ) );
								}
								else
								{
									HBRUSH hBrush = _GetSysColorBrush( COLOR_WINDOW );//( HBRUSH )_GetStockObject( WHITE_BRUSH );//GetClassLong( _GetParent( hWnd ), GCL_HBRBACKGROUND );
									_FillRect( hdcMem, &dis->rcItem, hBrush );
								}
							}
							else
							{
								HBRUSH hBrush = _GetSysColorBrush( COLOR_WINDOW );//( HBRUSH )_GetStockObject( WHITE_BRUSH );//GetClassLong( _GetParent( hWnd ), GCL_HBRBACKGROUND );
								_FillRect( hdcMem, &dis->rcItem, hBrush );
							}

							_BitBlt( dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top, hdcMem, 0, 0, SRCCOPY );
			
							// Delete our back buffer.
							_DeleteDC( hdcMem );
						}
					}
				}
			}
			return TRUE;
		}
		break;

		case WM_DESTROY:
		{
			hti_selected = NULL;

			// Free the TREEITEM structs stored in the lParams of the treeview.
			HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_font_settings, TVM_GETNEXTITEM, TVGN_ROOT, NULL );
			while ( hti != NULL )
			{
				TVITEM tvi;
				_memzero( &tvi, sizeof( TVITEM ) );
				tvi.mask = TVIF_PARAM;
				tvi.hItem = hti;
				_SendMessageW( g_hWnd_font_settings, TVM_GETITEM, 0, ( LPARAM )&tvi );

				if ( tvi.lParam != NULL )
				{
					GlobalFree( ( TREEITEM * )tvi.lParam );
				}

				hti = ( HTREEITEM )_SendMessageW( g_hWnd_font_settings, TVM_GETNEXTITEM, TVGN_NEXT, ( LPARAM )hti );
			}

			// We must free the image list created for the checkboxes.
			HIMAGELIST hil = ( HIMAGELIST )_SendMessageW( g_hWnd_font_settings, TVM_GETIMAGELIST, ( WPARAM )TVSIL_STATE, 0 );

			bool destroy = true;
			#ifndef COMCTL32_USE_STATIC_LIB
				if ( comctl32_state == COMCTL32_STATE_SHUTDOWN )
				{
					destroy = InitializeComCtl32();
				}
			#endif

			if ( hil != NULL )
			{
				if ( destroy )
				{
					_ImageList_Destroy( hil );
				}
				else	// Warn of leak if we can't destroy.
				{
					MESSAGE_LOG_OUTPUT( ML_WARNING, ST_TreeView_leak )
				}
			}

			if ( options_ringtone_played )
			{
				HandleRingtone( RINGTONE_CLOSE, NULL, L"options" );

				options_ringtone_played = false;
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
