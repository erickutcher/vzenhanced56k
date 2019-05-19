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
#include "utilities.h"

#include "ringtone_utilities.h"
#include "file_operations.h"
#include "string_tables.h"

#include "lite_gdi32.h"

#define BTN_OK					1000
#define BTN_CANCEL				1026
#define BTN_APPLY				1027

HWND g_hWnd_options = NULL;

HWND g_hWnd_general_tab = NULL;
HWND g_hWnd_modem_tab = NULL;
HWND g_hWnd_popup_tab = NULL;

HWND g_hWnd_apply = NULL;

bool options_state_changed = false;

void create_settings()
{
	if ( cfg_popup_info1.line_order == cfg_popup_info2.line_order ||
		 cfg_popup_info1.line_order == cfg_popup_info3.line_order ||
		 cfg_popup_info2.line_order == cfg_popup_info3.line_order ||
		 cfg_popup_info1.line_order > TOTAL_LINES ||
		 cfg_popup_info2.line_order > TOTAL_LINES ||
		 cfg_popup_info3.line_order > TOTAL_LINES ||
		 cfg_popup_info1.line_order == 0 ||
		 cfg_popup_info2.line_order == 0 ||
		 cfg_popup_info3.line_order == 0 )
	{
		cfg_popup_info1.line_order = LINE_TIME;
		cfg_popup_info2.line_order = LINE_CALLER_ID;
		cfg_popup_info3.line_order = LINE_PHONE;
	}

	SHARED_SETTINGS *shared_settings = ( SHARED_SETTINGS * )GlobalAlloc( GPTR, sizeof( SHARED_SETTINGS ) );
	shared_settings->popup_background_color1 = cfg_popup_background_color1;
	shared_settings->popup_background_color2 = cfg_popup_background_color2;
	shared_settings->popup_gradient = cfg_popup_gradient;
	shared_settings->popup_gradient_direction = cfg_popup_gradient_direction;
	if ( cfg_popup_enable_ringtones )
	{
		shared_settings->rti = default_ringtone;
	}

	LOGFONT lf;
	_memzero( &lf, sizeof( LOGFONT ) );

	for ( char i = 0; i < 3; ++i )
	{
		POPUP_SETTINGS *ls = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
		ls->shared_settings = shared_settings;
		ls->window_settings.drag_position.x = 0;
		ls->window_settings.drag_position.y = 0;
		ls->window_settings.window_position.x = 0;
		ls->window_settings.window_position.y = 0;
		ls->window_settings.is_dragging = false;
		
		_memcpy_s( &ls->popup_info, sizeof( POPUP_INFO ), g_popup_info[ i ], sizeof( POPUP_INFO ) );

		ls->popup_info.font_face = GlobalStrDupW( g_popup_info[ i ]->font_face );

		if ( g_popup_info[ i ]->font_face != NULL )
		{
			_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, g_popup_info[ i ]->font_face, LF_FACESIZE );
			lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

			lf.lfHeight = g_popup_info[ i ]->font_height;
			lf.lfWeight = g_popup_info[ i ]->font_weight;
			lf.lfItalic = g_popup_info[ i ]->font_italic;
			lf.lfUnderline = g_popup_info[ i ]->font_underline;
			lf.lfStrikeOut = g_popup_info[ i ]->font_strikeout;

			ls->font = _CreateFontIndirectW( &lf );
		}
		else
		{
			ls->font = NULL;
		}

		ls->line_text = NULL;

		if ( g_ll == NULL )
		{
			g_ll = DLL_CreateNode( ( void * )ls );
		}
		else
		{
			DoublyLinkedList *ll = DLL_CreateNode( ( void * )ls );
			DLL_AddNode( &g_ll, ll, -1 );
		}
	}
}

void reset_settings()
{
	LOGFONT lf;
	_memzero( &lf, sizeof( LOGFONT ) );

	DoublyLinkedList *temp_ll = g_ll;

	if ( temp_ll == NULL || ( temp_ll != NULL && temp_ll->data == NULL ) )
	{
		return;
	}

	SHARED_SETTINGS *shared_settings = ( ( POPUP_SETTINGS * )temp_ll->data )->shared_settings;
	if ( shared_settings != NULL )
	{
		shared_settings->popup_background_color1 = cfg_popup_background_color1;
		shared_settings->popup_background_color2 = cfg_popup_background_color2;
		shared_settings->popup_gradient = cfg_popup_gradient;
		shared_settings->popup_gradient_direction = cfg_popup_gradient_direction;
		shared_settings->rti = ( cfg_popup_enable_ringtones ? default_ringtone : NULL );
	}

	for ( char i = 0; i < 3; ++i )
	{
		POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )temp_ll->data;
		ps->shared_settings = shared_settings;
		ps->window_settings.drag_position.x = 0;
		ps->window_settings.drag_position.y = 0;
		ps->window_settings.window_position.x = 0;
		ps->window_settings.window_position.y = 0;
		ps->window_settings.is_dragging = false;

		if ( ps->popup_info.font_face != NULL )
		{
			GlobalFree( ps->popup_info.font_face );
		}

		_memcpy_s( &ps->popup_info, sizeof( POPUP_INFO ), g_popup_info[ i ], sizeof( POPUP_INFO ) );
		
		ps->popup_info.font_face = GlobalStrDupW( g_popup_info[ i ]->font_face );

		if ( g_popup_info[ i ]->font_face != NULL )
		{
			_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, g_popup_info[ i ]->font_face, LF_FACESIZE );
			lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

			lf.lfHeight = g_popup_info[ i ]->font_height;
			lf.lfWeight = g_popup_info[ i ]->font_weight;
			lf.lfItalic = g_popup_info[ i ]->font_italic;
			lf.lfUnderline = g_popup_info[ i ]->font_underline;
			lf.lfStrikeOut = g_popup_info[ i ]->font_strikeout;

			if ( ps->font != NULL )
			{
				_DeleteObject( ps->font );
			}
			ps->font = _CreateFontIndirectW( &lf );
		}
		else
		{
			ps->font = NULL;
		}

		ps->line_text = NULL;	// Not used

		temp_ll = temp_ll->next;

		if ( temp_ll == NULL || ( temp_ll != NULL && temp_ll->data == NULL ) )
		{
			return;
		}
	}
}

void delete_settings()
{
	DoublyLinkedList *current_node = g_ll;

	for ( unsigned char i = 0; i < TOTAL_LINES && current_node != NULL; ++i )
	{
		DoublyLinkedList *del_node = current_node;
		current_node = current_node->next;

		POPUP_SETTINGS *popup_settings = ( POPUP_SETTINGS * )del_node->data;
		if ( popup_settings != NULL )
		{
			// We can free the shared memory from the first node.
			if ( i == 0 && popup_settings->shared_settings != NULL )
			{
				// Do not free shared_settings->rti.
				GlobalFree( popup_settings->shared_settings );
			}

			_DeleteObject( popup_settings->font );
			GlobalFree( popup_settings->popup_info.font_face );
			GlobalFree( popup_settings->line_text );
			GlobalFree( popup_settings );
		}
		GlobalFree( del_node );
	}

	g_ll = NULL;
}

bool check_settings()
{
	DoublyLinkedList *current_node = g_ll;

	unsigned char count = 0;
	while ( count++ < TOTAL_LINES )
	{
		if ( current_node == NULL ||
		   ( current_node != NULL && current_node->data == NULL ) ||
		   ( current_node != NULL && current_node->data != NULL && ( ( POPUP_SETTINGS * )current_node->data )->shared_settings == NULL ) )
		{
			return false;
		}

		current_node = current_node->next;
	}

	return true;
}

void Set_Window_Settings()
{
	if ( !check_settings() )
	{
		// Bad list, recreate.

		delete_settings();

		create_settings();
	}
	else	// Reset values.
	{
		reset_settings();
	}

	//_SendMessageW( g_hWnd_font_settings, LB_RESETCONTENT, 0, 0 );

	int count = 0;
	DoublyLinkedList *ll = g_ll;
	while ( ll != NULL && ll->data != NULL )
	{
		unsigned char order = _abs( ( ( POPUP_SETTINGS * )ll->data )->popup_info.line_order );

		TREEITEM *ti = ( TREEITEM * )GlobalAlloc( GMEM_FIXED, sizeof( TREEITEM ) );
		ti->node = ll;
		ti->index = count++;

		TVINSERTSTRUCT tvis;
		_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
		tvis.item.lParam = ( LPARAM )ti;
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		tvis.item.state = INDEXTOSTATEIMAGEMASK( ( ( ( POPUP_SETTINGS * )ll->data )->popup_info.line_order > 0 ? 2 : 1 ) );

		switch ( order )
		{
			case LINE_TIME: { tvis.item.pszText = ST_Time; } break;
			case LINE_CALLER_ID: { tvis.item.pszText = ST_Caller_ID_Name; } break;
			case LINE_PHONE: { tvis.item.pszText = ST_Phone_Number; } break;
		}

		_SendMessageW( g_hWnd_font_settings, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

		ll = ll->next;
	}

	
	hti_selected = ( HTREEITEM )_SendMessageW( g_hWnd_font_settings, TVM_GETNEXTITEM, TVGN_ROOT, NULL );
	_SendMessageW( g_hWnd_font_settings, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti_selected );


	_SendMessageW( g_hWnd_ud_width, UDM_SETPOS, 0, cfg_popup_width );
	_SendMessageW( g_hWnd_ud_height, UDM_SETPOS, 0, cfg_popup_height );
	_SendMessageW( g_hWnd_position, CB_SETCURSEL, cfg_popup_position, 0 );
	_SendMessageW( g_hWnd_ud_time, UDM_SETPOS, 0, cfg_popup_time );
	_SendMessageW( g_hWnd_ud_transparency, UDM_SETPOS, 0, cfg_popup_transparency );

	_SendMessageW( g_hWnd_chk_minimize, BM_SETCHECK, ( cfg_minimize_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
	_SendMessageW( g_hWnd_chk_close, BM_SETCHECK, ( cfg_close_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
	_SendMessageW( g_hWnd_chk_silent_startup, BM_SETCHECK, ( cfg_silent_startup ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_always_on_top, BM_SETCHECK, ( cfg_always_on_top ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_enable_history, BM_SETCHECK, ( cfg_enable_call_log_history ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_message_log, BM_SETCHECK, ( cfg_enable_message_log ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_check_for_updates, BM_SETCHECK, ( cfg_check_for_updates ? BST_CHECKED : BST_UNCHECKED ), 0 );

	if ( cfg_tray_icon )
	{
		_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_CHECKED, 0 );
		_EnableWindow( g_hWnd_chk_minimize, TRUE );
		_EnableWindow( g_hWnd_chk_close, TRUE );
		_EnableWindow( g_hWnd_chk_silent_startup, TRUE );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_UNCHECKED, 0 );
		_EnableWindow( g_hWnd_chk_minimize, FALSE );
		_EnableWindow( g_hWnd_chk_close, FALSE );
		_EnableWindow( g_hWnd_chk_silent_startup, FALSE );
	}

	if ( cfg_popup_gradient )
	{
		_SendMessageW( g_hWnd_chk_gradient, BM_SETCHECK, BST_CHECKED, 0 );
		_EnableWindow( g_hWnd_btn_background_color2, TRUE );
		_EnableWindow( g_hWnd_group_gradient_direction, TRUE );
		_EnableWindow( g_hWnd_rad_gradient_horz, TRUE );
		_EnableWindow( g_hWnd_rad_gradient_vert, TRUE );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_gradient, BM_SETCHECK, BST_UNCHECKED, 0 );
		_EnableWindow( g_hWnd_btn_background_color2, FALSE );
		_EnableWindow( g_hWnd_group_gradient_direction, FALSE );
		_EnableWindow( g_hWnd_rad_gradient_horz, FALSE );
		_EnableWindow( g_hWnd_rad_gradient_vert, FALSE );
	}

	_EnableWindow( g_hWnd_btn_font_shadow_color, ( cfg_popup_info1.font_shadow ? TRUE : FALSE ) );

	_SendMessageW( g_hWnd_chk_contact_picture, BM_SETCHECK, ( cfg_popup_show_contact_picture ? BST_CHECKED : BST_UNCHECKED ), 0 );
	_SendMessageW( g_hWnd_chk_border, BM_SETCHECK, ( cfg_popup_hide_border ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_shadow, BM_SETCHECK, ( cfg_popup_info1.font_shadow ? BST_CHECKED : BST_UNCHECKED ), 0 );

	if ( cfg_popup_info1.justify_line == 2 )
	{
		_SendMessageW( g_hWnd_rad_right_justify, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_rad_center_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
		_SendMessageW( g_hWnd_rad_left_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
	}
	else if ( cfg_popup_info1.justify_line == 1 )
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

	if ( cfg_popup_gradient_direction == 1 )
	{
		_SendMessageW( g_hWnd_rad_gradient_horz, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_rad_gradient_vert, BM_SETCHECK, BST_UNCHECKED, 0 );
	}
	else
	{
		_SendMessageW( g_hWnd_rad_gradient_horz, BM_SETCHECK, BST_UNCHECKED, 0 );
		_SendMessageW( g_hWnd_rad_gradient_vert, BM_SETCHECK, BST_CHECKED, 0 );
	}

	if ( cfg_popup_time_format == 1 )
	{
		_SendMessageW( g_hWnd_rad_12_hour, BM_SETCHECK, BST_UNCHECKED, 0 );
		_SendMessageW( g_hWnd_rad_24_hour, BM_SETCHECK, BST_CHECKED, 0 );
	}
	else
	{
		_SendMessageW( g_hWnd_rad_12_hour, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_rad_24_hour, BM_SETCHECK, BST_UNCHECKED, 0 );
	}

	if ( _abs( cfg_popup_info1.line_order ) == LINE_TIME )
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

	int set_index = CB_ERR;

	node_type *node = dllrbt_get_head( modem_list );
	while ( node != NULL )
	{
		modem_info *mi = ( modem_info * )node->val;

		if ( mi != NULL )
		{
			int add_index = ( int )_SendMessageW( g_hWnd_default_modem, CB_ADDSTRING, 0, ( LPARAM )mi->line_name );

			if ( set_index == CB_ERR && mi == default_modem )
			{
				set_index = add_index;
			}

			_SendMessageW( g_hWnd_default_modem, CB_SETITEMDATA, add_index, ( LPARAM )mi );
		}

		node = node->next;
	}

	if ( set_index == CB_ERR )
	{
		set_index = 0;
	}

	_SendMessageW( g_hWnd_default_modem, CB_SETCURSEL, set_index, 0 );

	if ( default_modem == NULL )
	{
		_EnableWindow( g_hWnd_default_modem, FALSE );
	}

	_SendMessageW( g_hWnd_ud_repeat_recording, UDM_SETPOS, 0, cfg_repeat_recording );
	_SendMessageW( g_hWnd_ud_droptime, UDM_SETPOS, 0, cfg_drop_call_wait );

	// RECORDING

	set_index = CB_ERR;

	node = dllrbt_get_head( recording_list );
	while ( node != NULL )
	{
		ringtone_info *rti = ( ringtone_info * )node->val;

		if ( rti != NULL )
		{
			int add_index = ( int )_SendMessageW( g_hWnd_default_recording, CB_ADDSTRING, 0, ( LPARAM )rti->ringtone_file );

			if ( set_index == CB_ERR && rti == default_recording )
			{
				set_index = add_index;
			}

			_SendMessageW( g_hWnd_default_recording, CB_SETITEMDATA, add_index, ( LPARAM )rti );
		}

		node = node->next;
	}

	_SendMessageW( g_hWnd_default_recording, CB_SETCURSEL, ( set_index == CB_ERR ? 0 : set_index ), 0 );

	if ( cfg_popup_enable_recording && g_voice_playback_supported )
	{
		_SendMessageW( g_hWnd_chk_enable_recording, BM_SETCHECK, BST_CHECKED, 0 );
		_EnableWindow( g_hWnd_static_default_recording, TRUE );

		if ( _SendMessageW( g_hWnd_default_recording, CB_GETCOUNT, 0, 0 ) == 0 )
		{
			_EnableWindow( g_hWnd_default_recording, FALSE );
			_EnableWindow( g_hWnd_play_default_recording, FALSE );
			_EnableWindow( g_hWnd_stop_default_recording, FALSE );
		}
		else
		{
			_EnableWindow( g_hWnd_default_recording, TRUE );
			_EnableWindow( g_hWnd_play_default_recording, TRUE );
			_EnableWindow( g_hWnd_stop_default_recording, TRUE );
		}

		_EnableWindow( g_hWnd_static_repeat_recording, TRUE );
		_EnableWindow( g_hWnd_repeat_recording, TRUE );
		_EnableWindow( g_hWnd_ud_repeat_recording, TRUE );
	}
	else
	{
		_EnableWindow( g_hWnd_chk_enable_recording, ( g_voice_playback_supported ? TRUE : FALSE ) );

		_SendMessageW( g_hWnd_chk_enable_recording, BM_SETCHECK, ( cfg_popup_enable_recording ? BST_CHECKED : BST_UNCHECKED ), 0 );
		_EnableWindow( g_hWnd_static_default_recording, FALSE );
		_EnableWindow( g_hWnd_default_recording, FALSE );
		_EnableWindow( g_hWnd_play_default_recording, FALSE );
		_EnableWindow( g_hWnd_stop_default_recording, FALSE );

		_EnableWindow( g_hWnd_static_repeat_recording, FALSE );
		_EnableWindow( g_hWnd_repeat_recording, FALSE );
		_EnableWindow( g_hWnd_ud_repeat_recording, FALSE );
	}

	// RINGTONES

	set_index = CB_ERR;

	node = dllrbt_get_head( ringtone_list );
	while ( node != NULL )
	{
		ringtone_info *rti = ( ringtone_info * )node->val;

		if ( rti != NULL )
		{
			int add_index = ( int )_SendMessageW( g_hWnd_default_ringtone, CB_ADDSTRING, 0, ( LPARAM )rti->ringtone_file );

			if ( set_index == CB_ERR && rti == default_ringtone )
			{
				set_index = add_index;
			}

			_SendMessageW( g_hWnd_default_ringtone, CB_SETITEMDATA, add_index, ( LPARAM )rti );
		}

		node = node->next;
	}

	if ( set_index == CB_ERR )
	{
		set_index = 0;
	}

	_SendMessageW( g_hWnd_default_ringtone, CB_SETCURSEL, set_index, 0 );

	if ( cfg_popup_enable_ringtones )
	{
		_SendMessageW( g_hWnd_chk_enable_ringtones, BM_SETCHECK, BST_CHECKED, 0 );
		_EnableWindow( g_hWnd_static_default_ringtone, TRUE );
		_EnableWindow( g_hWnd_default_ringtone, TRUE );
		
		if ( set_index == 0 )
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
		_SendMessageW( g_hWnd_chk_enable_ringtones, BM_SETCHECK, BST_UNCHECKED, 0 );
		_EnableWindow( g_hWnd_static_default_ringtone, FALSE );
		_EnableWindow( g_hWnd_default_ringtone, FALSE );
		_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
		_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
	}

	if ( cfg_enable_popups )
	{
		_SendMessageW( g_hWnd_chk_enable_popups, BM_SETCHECK, BST_CHECKED, 0 );
		Enable_Disable_Popup_Windows( TRUE );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_enable_popups, BM_SETCHECK, BST_UNCHECKED, 0 );
		Enable_Disable_Popup_Windows( FALSE );
	}

	options_state_changed = false;
	_EnableWindow( g_hWnd_apply, FALSE );	// Handles Enable line - windows.
}

LRESULT CALLBACK OptionsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_General;
			ti.lParam = ( LPARAM )&g_hWnd_general_tab;
			_SendMessageW( hWnd_options_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Modem;
			ti.lParam = ( LPARAM )&g_hWnd_modem_tab;
			_SendMessageW( hWnd_options_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Popup;
			ti.lParam = ( LPARAM )&g_hWnd_popup_tab;
			_SendMessageW( hWnd_options_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			HWND g_hWnd_ok = _CreateWindowW( WC_BUTTON, ST_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 260, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );
			HWND g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CANCEL, NULL, NULL );
			g_hWnd_apply = _CreateWindowW( WC_BUTTON, ST_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_APPLY, NULL, NULL );

			_SendMessageW( hWnd_options_tab, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Set the tab control's font so we can get the correct tab height to adjust its children.
			RECT rc_tab;
			_GetClientRect( hWnd_options_tab, &rc );

			_SendMessageW( hWnd_options_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			g_hWnd_general_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"general_tab", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_modem_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"modem_tab", NULL, WS_CHILD | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_popup_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"popup_tab", NULL, WS_CHILD | WS_VSCROLL | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), hWnd_options_tab, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_ok, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_cancel, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_apply, WM_SETFONT, ( WPARAM )hFont, 0 );

			Set_Window_Settings();

			_SetFocus( g_hWnd_general_tab );

			return 0;
		}
		break;

		/*case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;*/

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
					int index = ( int )_SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_HIDE );
					}
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
                {
					NMHDR *nmhdr = ( NMHDR * )lParam;

					HWND hWnd_focused = GetFocus();
					if ( hWnd_focused != hWnd && hWnd_focused != nmhdr->hwndFrom )
					{
						SetFocus( GetWindow( nmhdr->hwndFrom, GW_CHILD ) );
					}

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = ( int )_SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_SHOW );

						_SetFocus( *( ( HWND * )tie.lParam ) );
					}
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
				case IDOK:
				case BTN_OK:
				case BTN_APPLY:
				{
					if ( !options_state_changed )
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
						break;
					}

					bool error_saving = false;
					unsigned char count = 0;

					DoublyLinkedList *current_node = g_ll;
					while ( current_node != NULL )
					{
						if ( current_node->data == NULL || ( current_node->data != NULL && ( ( POPUP_SETTINGS * )current_node->data )->shared_settings == NULL ) || count >= TOTAL_LINES )
						{
							error_saving = true;
							break;
						}

						POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )current_node->data;

						if ( count == 0 )
						{
							// Set shared settings one time.
							SHARED_SETTINGS *shared_settings = ( ( POPUP_SETTINGS * )current_node->data )->shared_settings;
							cfg_popup_background_color1 = shared_settings->popup_background_color1;
							cfg_popup_background_color2 = shared_settings->popup_background_color2;
							cfg_popup_gradient = shared_settings->popup_gradient;
							cfg_popup_gradient_direction = shared_settings->popup_gradient_direction;

							ringtone_info *rti = NULL;
							wchar_t *ringtone = NULL;
							int current_ringtone_index = ( int )_SendMessageW( g_hWnd_default_ringtone, CB_GETCURSEL, 0, 0 );
							if ( current_ringtone_index != CB_ERR )
							{
								if ( current_ringtone_index == 0 )
								{
									ringtone = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) );
								}
								else
								{
									rti = ( ringtone_info * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
									if ( rti != NULL )
									{
										if ( rti != ( ringtone_info * )CB_ERR )
										{
											ringtone = GlobalStrDupW( rti->ringtone_path );
										}
										else
										{
											rti = NULL;
										}
									}
								}
							}

							if ( cfg_popup_ringtone != NULL )
							{
								GlobalFree( cfg_popup_ringtone );
							}
							cfg_popup_ringtone = ringtone;
							default_ringtone = rti;

							cfg_popup_info1.line_order = ps->popup_info.line_order;
							cfg_popup_info1.font_color = ps->popup_info.font_color;
							cfg_popup_info1.font_shadow = ps->popup_info.font_shadow;
							cfg_popup_info1.font_shadow_color = ps->popup_info.font_shadow_color;
							cfg_popup_info1.justify_line = ps->popup_info.justify_line;
							cfg_popup_info1.font_height = ps->popup_info.font_height;
							cfg_popup_info1.font_italic = ps->popup_info.font_italic;
							cfg_popup_info1.font_weight = ps->popup_info.font_weight;
							cfg_popup_info1.font_strikeout = ps->popup_info.font_strikeout;
							cfg_popup_info1.font_underline = ps->popup_info.font_underline;
							if ( cfg_popup_info1.font_face != NULL )
							{
								GlobalFree( cfg_popup_info1.font_face );
							}
							cfg_popup_info1.font_face = GlobalStrDupW( ps->popup_info.font_face );
						}
						else if ( count == 1 )
						{
							cfg_popup_info2.line_order = ps->popup_info.line_order;
							cfg_popup_info2.font_color = ps->popup_info.font_color;
							cfg_popup_info2.font_shadow = ps->popup_info.font_shadow;
							cfg_popup_info2.font_shadow_color = ps->popup_info.font_shadow_color;
							cfg_popup_info2.justify_line = ps->popup_info.justify_line;
							cfg_popup_info2.font_height = ps->popup_info.font_height;
							cfg_popup_info2.font_italic = ps->popup_info.font_italic;
							cfg_popup_info2.font_weight = ps->popup_info.font_weight;
							cfg_popup_info2.font_strikeout = ps->popup_info.font_strikeout;
							cfg_popup_info2.font_underline = ps->popup_info.font_underline;
							if ( cfg_popup_info2.font_face != NULL )
							{
								GlobalFree( cfg_popup_info2.font_face );
							}
							cfg_popup_info2.font_face = GlobalStrDupW( ps->popup_info.font_face );
						}
						else if ( count == 2 )
						{
							cfg_popup_info3.line_order = ps->popup_info.line_order;
							cfg_popup_info3.font_color = ps->popup_info.font_color;
							cfg_popup_info3.font_shadow = ps->popup_info.font_shadow;
							cfg_popup_info3.font_shadow_color = ps->popup_info.font_shadow_color;
							cfg_popup_info3.justify_line = ps->popup_info.justify_line;
							cfg_popup_info3.font_height = ps->popup_info.font_height;
							cfg_popup_info3.font_italic = ps->popup_info.font_italic;
							cfg_popup_info3.font_weight = ps->popup_info.font_weight;
							cfg_popup_info3.font_strikeout = ps->popup_info.font_strikeout;
							cfg_popup_info3.font_underline = ps->popup_info.font_underline;
							if ( cfg_popup_info3.font_face != NULL )
							{
								GlobalFree( cfg_popup_info3.font_face );
							}
							cfg_popup_info3.font_face = GlobalStrDupW( ps->popup_info.font_face );
						}
						else
						{
							error_saving = true;
							break;
						}

						++count;

						current_node = current_node->next;
					}

					if ( !error_saving )
					{
						char value[ 5 ];
						_SendMessageA( g_hWnd_width, WM_GETTEXT, 5, ( LPARAM )value );
						cfg_popup_width = _strtoul( value, NULL, 10 );
						_SendMessageA( g_hWnd_height, WM_GETTEXT, 5, ( LPARAM )value );
						cfg_popup_height = _strtoul( value, NULL, 10 );

						_SendMessageA( g_hWnd_time, WM_GETTEXT, 4, ( LPARAM )value );
						cfg_popup_time = ( unsigned short )_strtoul( value, NULL, 10 );
						_SendMessageA( g_hWnd_transparency, WM_GETTEXT, 4, ( LPARAM )value );
						cfg_popup_transparency = ( unsigned char )_strtoul( value, NULL, 10 );
						_SendMessageA( g_hWnd_repeat_recording, WM_GETTEXT, 3, ( LPARAM )value );
						cfg_repeat_recording = ( unsigned char )_strtoul( value, NULL, 10 );
						_SendMessageA( g_hWnd_droptime, WM_GETTEXT, 3, ( LPARAM )value );
						cfg_drop_call_wait = ( unsigned char )_strtoul( value, NULL, 10 );

						cfg_popup_enable_recording = ( _SendMessageW( g_hWnd_chk_enable_recording, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						ringtone_info *rti = NULL;
						wchar_t *recording = NULL;
						int current_recording_index = ( int )_SendMessageW( g_hWnd_default_recording, CB_GETCURSEL, 0, 0 );
						if ( current_recording_index != CB_ERR )
						{
							rti = ( ringtone_info * )_SendMessageW( g_hWnd_default_recording, CB_GETITEMDATA, current_recording_index, 0 );
							if ( rti != NULL )
							{
								if ( rti != ( ringtone_info * )CB_ERR )
								{
									recording = GlobalStrDupW( rti->ringtone_path );
								}
								else
								{
									rti = NULL;
								}
							}
						}

						if ( cfg_recording != NULL )
						{
							GlobalFree( cfg_recording );
						}
						cfg_recording = recording;
						default_recording = rti;

						cfg_check_for_updates = ( _SendMessageW( g_hWnd_chk_check_for_updates, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
						cfg_popup_position = ( unsigned char )_SendMessageW( g_hWnd_position, CB_GETCURSEL, 0, 0 );
						
						cfg_minimize_to_tray = ( _SendMessageW( g_hWnd_chk_minimize, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
						cfg_close_to_tray = ( _SendMessageW( g_hWnd_chk_close, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
						cfg_silent_startup = ( _SendMessageW( g_hWnd_chk_silent_startup, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
						cfg_popup_enable_ringtones = ( _SendMessageW( g_hWnd_chk_enable_ringtones, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						cfg_enable_call_log_history = ( _SendMessageW( g_hWnd_chk_enable_history, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						cfg_popup_show_contact_picture = ( _SendMessageW( g_hWnd_chk_contact_picture, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
						cfg_popup_hide_border = ( _SendMessageW( g_hWnd_chk_border, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
						cfg_popup_time_format = ( _SendMessageW( g_hWnd_rad_24_hour, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

						cfg_enable_popups = ( _SendMessageW( g_hWnd_chk_enable_popups, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						int current_modem_index = ( int )_SendMessageW( g_hWnd_default_modem, CB_GETCURSEL, 0, 0 );
						if ( current_modem_index != CB_ERR )
						{
							modem_info *mi = ( modem_info * )_SendMessageW( g_hWnd_default_modem, CB_GETITEMDATA, current_modem_index, 0 );
							if ( mi != NULL &&  mi != ( modem_info * )CB_ERR )
							{
								cfg_modem_guid = mi->permanent_line_guid;
								default_modem = mi;
							}
						}

						bool tray_icon = ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						// We don't want to allow a silent startup if the tray icon is hidden.
						if ( !tray_icon && cfg_silent_startup )
						{
							cfg_silent_startup = false;
							_SendMessageW( g_hWnd_chk_silent_startup, BM_SETCHECK, BST_UNCHECKED, 0 );
						}

						// Add the tray icon if it was not previously enabled.
						if ( tray_icon && !cfg_tray_icon )
						{
							_memzero( &g_nid, sizeof( NOTIFYICONDATA ) );
							g_nid.cbSize = NOTIFYICONDATA_V2_SIZE;	// 5.0 (Windows 2000) and newer.
							g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
							g_nid.hWnd = g_hWnd_main;
							g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
							g_nid.uID = 1000;
							g_nid.hIcon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
							_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ), L"VZ Enhanced 56K\0", 16 );
							g_nid.szTip[ 15 ] = 0;	// Sanity.
							_Shell_NotifyIconW( NIM_ADD, &g_nid );
						}
						else if ( !tray_icon && cfg_tray_icon )	// Remove the tray icon if it was previously enabled.
						{
							// Make sure that the main window is not hidden before we delete the tray icon.
							if ( _IsWindowVisible( g_hWnd_main ) == FALSE )
							{
								_ShowWindow( g_hWnd_main, SW_SHOWNOACTIVATE );
							}

							_Shell_NotifyIconW( NIM_DELETE, &g_nid );
						}

						cfg_tray_icon = tray_icon;

						bool always_on_top = ( _SendMessageW( g_hWnd_chk_always_on_top, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						// Set any active windows if we've changed the extended style.
						if ( always_on_top != cfg_always_on_top )
						{
							cfg_always_on_top = always_on_top;

							if ( g_hWnd_main != NULL ){ _SetWindowPos( g_hWnd_main, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_contact != NULL ){ _SetWindowPos( g_hWnd_contact, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_ignore_phone_number != NULL ){ _SetWindowPos( g_hWnd_ignore_phone_number, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_ignore_cid != NULL ){ _SetWindowPos( g_hWnd_ignore_cid, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_message_log != NULL ){ _SetWindowPos( g_hWnd_message_log, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_search != NULL ){ _SetWindowPos( g_hWnd_search, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_options != NULL ){ _SetWindowPos( g_hWnd_options, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						}

						cfg_enable_message_log = ( _SendMessageW( g_hWnd_chk_message_log, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						save_config();
					}
					else
					{
						Set_Window_Settings();

						_MessageBoxW( hWnd, ST_error_while_saving_settings, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONERROR );
					}

					options_state_changed = false;

					if ( LOWORD( wParam ) == BTN_APPLY )
					{
						_EnableWindow( g_hWnd_apply, FALSE );
					}
					else
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
					}
				}
				break;

				case BTN_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}

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

		case WM_CLOSE:
		{
			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			options_state_changed = false;	// Reset the state if it changed, but the options were never saved.

			delete_settings();

			g_hWnd_options = NULL;

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
