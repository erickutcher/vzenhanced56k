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
#include "utilities.h"
#include "message_log_utilities.h"
#include "ringtone_utilities.h"
#include "file_operations.h"
#include "string_tables.h"

#include "lite_comdlg32.h"
#include "lite_gdi32.h"
#include "lite_advapi32.h"
#include "lite_comctl32.h"

#define TVN_CHECKSTATECHANGE	WM_APP + 6

// This is defined in <lmcons.h>
#define UNLEN 256

#define BTN_OK					1000
#define BTN_MINIMIZE_TO_TRAY	1001
#define BTN_CLOSE_TO_TRAY		1002
#define BTN_BACKGROUND_COLOR	1003
#define BTN_FONT_COLOR			1004
#define BTN_CHOOSE_FONT			1005
#define BTN_PREVIEW				1006
#define EDIT_HEIGHT				1007
#define EDIT_WIDTH				1008
#define EDIT_TIME				1009
#define EDIT_TRANSPARENCY		1010
#define BTN_GRADIENT			1011
#define BTN_BACKGROUND_COLOR2	1012
#define BTN_BORDER				1013
#define BTN_SHADOW				1014
#define BTN_FONT_SHADOW_COLOR	1015
#define RADIO_LEFT				1016
#define RADIO_CENTER			1017
#define RADIO_RIGHT				1018
#define LIST_SETTINGS			1019
#define BTN_MOVE_UP				1020
#define	BTN_MOVE_DOWN			1021

#define RADIO_GRADIENT_HORZ		1022
#define RADIO_GRADIENT_VERT		1023

#define RADIO_12_HOUR			1024
#define RADIO_24_HOUR			1025
#define BTN_CANCEL				1026
#define BTN_APPLY				1027

#define CB_POSITION				1028
#define BTN_CONTACT_PICTURE		1029
#define	BTN_ENABLE_RINGTONES	1030
#define CB_DEFAULT_RINGTONE		1031
#define BTN_PLAY_DEFAULT_RINGTONE	1032
#define BTN_STOP_DEFAULT_RINGTONE	1033

#define BTN_ENABLE_POPUPS		1034
#define BTN_RECONNECT			1035
#define EDIT_RETRIES			1036

#define EDIT_TIMEOUT			1037

#define BTN_TRAY_ICON			1038

#define BTN_SILENT_STARTUP		1039

#define CB_SSL_VERSION			1040
#define BTN_ENABLE_HISTORY		1041

#define BTN_ENABLE_MESSAGE_LOG	1042

#define BTN_ALWAYS_ON_TOP		1043

#define BTN_CHECK_FOR_UPDATES	1044

#define CB_DEFAULT_MODEM		1045

#define BTN_ENABLE_RECORDING	1046
#define CB_DEFAULT_RECORDING	1047
#define BTN_PLAY_DEFAULT_RECORDING	1048
#define BTN_STOP_DEFAULT_RECORDING	1049

#define EDIT_REPEAT_RECORDING	1050

#define EDIT_DROPTIME			1051


HWND g_hWnd_options_tab = NULL;

HWND g_hWnd_chk_tray_icon = NULL;
HWND g_hWnd_chk_minimize = NULL;
HWND g_hWnd_chk_close = NULL;
HWND g_hWnd_chk_silent_startup = NULL;
HWND g_hWnd_chk_always_on_top = NULL;
HWND g_hWnd_chk_enable_history = NULL;
HWND g_hWnd_chk_message_log = NULL;

HWND g_hWnd_chk_reconnect = NULL;
HWND g_hWnd_static_retry = NULL;
HWND g_hWnd_retries = NULL;
HWND g_hWnd_ud_retries = NULL;

HWND g_hWnd_static_timeout = NULL;
HWND g_hWnd_timeout = NULL;
HWND g_hWnd_ud_timeout = NULL;

HWND g_hWnd_static_ssl_version = NULL;
HWND g_hWnd_ssl_version = NULL;

HWND g_hWnd_chk_check_for_updates = NULL;

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
//HWND g_hWnd_static_sample = NULL;
HWND g_hWnd_static_background_color = NULL;
HWND g_hWnd_background_color = NULL;
HWND g_hWnd_preview = NULL;

HWND g_hWnd_chk_enable_ringtones = NULL;
HWND g_hWnd_static_default_ringtone = NULL;
HWND g_hWnd_default_ringtone = NULL;
HWND g_hWnd_play_default_ringtone = NULL;
HWND g_hWnd_stop_default_ringtone = NULL;

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

HWND g_hWnd_apply = NULL;

HWND g_hWnd_general_tab = NULL;
HWND g_hWnd_connection_tab = NULL;
HWND g_hWnd_modem_tab = NULL;
HWND g_hWnd_popup_tab = NULL;

HTREEITEM hti_selected = NULL;	// The currently selected treeview item.

struct TREEITEM
{
	DoublyLinkedList *node;
	unsigned int index;
};

DoublyLinkedList *g_ll = NULL;

bool options_state_changed = false;

bool options_ringtone_played = false;
bool options_recording_played = false;

int popup_tab_height = 0;
int popup_tab_scroll_pos = 0;

void create_settings()
{
	if ( cfg_popup_line_order1 == cfg_popup_line_order2 || cfg_popup_line_order1 == cfg_popup_line_order3 || cfg_popup_line_order2 == cfg_popup_line_order3 ||
		 cfg_popup_line_order1 > TOTAL_LINES || cfg_popup_line_order2 > TOTAL_LINES || cfg_popup_line_order3 > TOTAL_LINES ||
		 cfg_popup_line_order1 == 0 || cfg_popup_line_order2 == 0 || cfg_popup_line_order3 == 0 )
	{
		cfg_popup_line_order1 = LINE_TIME;
		cfg_popup_line_order2 = LINE_CALLER_ID;
		cfg_popup_line_order3 = LINE_PHONE;
	}

	SHARED_SETTINGS *shared_settings = ( SHARED_SETTINGS * )GlobalAlloc( GPTR, sizeof( SHARED_SETTINGS ) );
	shared_settings->popup_background_color1 = cfg_popup_background_color1;
	shared_settings->popup_background_color2 = cfg_popup_background_color2;
	shared_settings->popup_gradient = cfg_popup_gradient;
	shared_settings->popup_gradient_direction = cfg_popup_gradient_direction;
	if ( cfg_popup_enable_ringtones )
	{
		shared_settings->ringtone_info = default_ringtone;
	}

	LOGFONT lf;
	_memzero( &lf, sizeof( LOGFONT ) );
	
	POPUP_SETTINGS *ls1 = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
	ls1->shared_settings = shared_settings;
	ls1->window_settings.drag_position.x = 0;
	ls1->window_settings.drag_position.y = 0;
	ls1->window_settings.window_position.x = 0;
	ls1->window_settings.window_position.y = 0;
	ls1->window_settings.is_dragging = false;
	ls1->popup_line_order = cfg_popup_line_order1;
	ls1->popup_justify = cfg_popup_justify_line1;
	ls1->font_color = cfg_popup_font_color1;
	ls1->font_shadow = cfg_popup_font_shadow1;
	ls1->font_shadow_color = cfg_popup_font_shadow_color1;
	ls1->font_height = cfg_popup_font_height1;
	ls1->font_italic = cfg_popup_font_italic1;
	ls1->font_weight = cfg_popup_font_weight1;
	ls1->font_strikeout = cfg_popup_font_strikeout1;
	ls1->font_underline = cfg_popup_font_underline1;
	ls1->font_face = GlobalStrDupW( cfg_popup_font_face1 );

	if ( cfg_popup_font_face1 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face1, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

		lf.lfHeight = cfg_popup_font_height1;
		lf.lfWeight = cfg_popup_font_weight1;
		lf.lfItalic = cfg_popup_font_italic1;
		lf.lfUnderline = cfg_popup_font_underline1;
		lf.lfStrikeOut = cfg_popup_font_strikeout1;

		ls1->font = _CreateFontIndirectW( &lf );
	}
	else
	{
		ls1->font = NULL;
	}

	ls1->line_text = NULL;

	POPUP_SETTINGS *ls2 = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
	ls2->shared_settings = shared_settings;
	ls2->window_settings.drag_position.x = 0;
	ls2->window_settings.drag_position.y = 0;
	ls2->window_settings.window_position.x = 0;
	ls2->window_settings.window_position.y = 0;
	ls2->window_settings.is_dragging = false;
	ls2->popup_line_order = cfg_popup_line_order2;
	ls2->popup_justify = cfg_popup_justify_line2;
	ls2->font_color = cfg_popup_font_color2;
	ls2->font_shadow = cfg_popup_font_shadow2;
	ls2->font_shadow_color = cfg_popup_font_shadow_color2;
	ls2->font_height = cfg_popup_font_height2;
	ls2->font_italic = cfg_popup_font_italic2;
	ls2->font_weight = cfg_popup_font_weight2;
	ls2->font_strikeout = cfg_popup_font_strikeout2;
	ls2->font_underline = cfg_popup_font_underline2;
	ls2->font_face = GlobalStrDupW( cfg_popup_font_face2 );
	
	if ( cfg_popup_font_face2 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face2, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

		lf.lfHeight = cfg_popup_font_height2;
		lf.lfWeight = cfg_popup_font_weight2;
		lf.lfItalic = cfg_popup_font_italic2;
		lf.lfUnderline = cfg_popup_font_underline2;
		lf.lfStrikeOut = cfg_popup_font_strikeout2;

		ls2->font = _CreateFontIndirectW( &lf );
	}
	else
	{
		ls2->font = NULL;
	}

	ls2->line_text = NULL;

	POPUP_SETTINGS *ls3 = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
	ls3->shared_settings = shared_settings;
	ls3->window_settings.drag_position.x = 0;
	ls3->window_settings.drag_position.y = 0;
	ls3->window_settings.window_position.x = 0;
	ls3->window_settings.window_position.y = 0;
	ls3->window_settings.is_dragging = false;
	ls3->popup_line_order = cfg_popup_line_order3;
	ls3->popup_justify = cfg_popup_justify_line3;
	ls3->font_color = cfg_popup_font_color3;
	ls3->font_shadow = cfg_popup_font_shadow3;
	ls3->font_shadow_color = cfg_popup_font_shadow_color3;
	ls3->font_height = cfg_popup_font_height3;
	ls3->font_italic = cfg_popup_font_italic3;
	ls3->font_weight = cfg_popup_font_weight3;
	ls3->font_strikeout = cfg_popup_font_strikeout3;
	ls3->font_underline = cfg_popup_font_underline3;
	ls3->font_face = GlobalStrDupW( cfg_popup_font_face3 );
	
	if ( cfg_popup_font_face3 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face3, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

		lf.lfHeight = cfg_popup_font_height3;
		lf.lfWeight = cfg_popup_font_weight3;
		lf.lfItalic = cfg_popup_font_italic3;
		lf.lfUnderline = cfg_popup_font_underline3;
		lf.lfStrikeOut = cfg_popup_font_strikeout3;

		ls3->font = _CreateFontIndirectW( &lf );
	}
	else
	{
		ls3->font = NULL;
	}

	ls3->line_text = NULL;

	g_ll = DLL_CreateNode( ( void * )ls1 );

	DoublyLinkedList *ll = DLL_CreateNode( ( void * )ls2 );
	DLL_AddNode( &g_ll, ll, -1 );

	ll = DLL_CreateNode( ( void * )ls3 );
	DLL_AddNode( &g_ll, ll, -1 );
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
		shared_settings->ringtone_info = ( cfg_popup_enable_ringtones ? default_ringtone : NULL );
	}

	POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )temp_ll->data;
	ps->shared_settings = shared_settings;
	ps->window_settings.drag_position.x = 0;
	ps->window_settings.drag_position.y = 0;
	ps->window_settings.window_position.x = 0;
	ps->window_settings.window_position.y = 0;
	ps->window_settings.is_dragging = false;
	ps->popup_line_order = cfg_popup_line_order1;
	ps->popup_justify = cfg_popup_justify_line1;
	ps->font_color = cfg_popup_font_color1;
	ps->font_shadow = cfg_popup_font_shadow1;
	ps->font_shadow_color = cfg_popup_font_shadow_color1;
	ps->font_height = cfg_popup_font_height1;
	ps->font_italic = cfg_popup_font_italic1;
	ps->font_weight = cfg_popup_font_weight1;
	ps->font_strikeout = cfg_popup_font_strikeout1;
	ps->font_underline = cfg_popup_font_underline1;
	if ( ps->font_face != NULL )
	{
		GlobalFree( ps->font_face );
	}
	ps->font_face = GlobalStrDupW( cfg_popup_font_face1 );

	if ( cfg_popup_font_face1 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face1, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

		lf.lfHeight = cfg_popup_font_height1;
		lf.lfWeight = cfg_popup_font_weight1;
		lf.lfItalic = cfg_popup_font_italic1;
		lf.lfUnderline = cfg_popup_font_underline1;
		lf.lfStrikeOut = cfg_popup_font_strikeout1;

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

	ps = ( POPUP_SETTINGS * )temp_ll->data;
	ps->shared_settings = shared_settings;
	ps->window_settings.drag_position.x = 0;
	ps->window_settings.drag_position.y = 0;
	ps->window_settings.window_position.x = 0;
	ps->window_settings.window_position.y = 0;
	ps->window_settings.is_dragging = false;
	ps->popup_line_order = cfg_popup_line_order2;
	ps->popup_justify = cfg_popup_justify_line2;
	ps->font_color = cfg_popup_font_color2;
	ps->font_shadow = cfg_popup_font_shadow2;
	ps->font_shadow_color = cfg_popup_font_shadow_color2;
	ps->font_height = cfg_popup_font_height2;
	ps->font_italic = cfg_popup_font_italic2;
	ps->font_weight = cfg_popup_font_weight2;
	ps->font_strikeout = cfg_popup_font_strikeout2;
	ps->font_underline = cfg_popup_font_underline2;
	if ( ps->font_face != NULL )
	{
		GlobalFree( ps->font_face );
	}
	ps->font_face = GlobalStrDupW( cfg_popup_font_face2 );
	
	if ( cfg_popup_font_face2 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face2, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

		lf.lfHeight = cfg_popup_font_height2;
		lf.lfWeight = cfg_popup_font_weight2;
		lf.lfItalic = cfg_popup_font_italic2;
		lf.lfUnderline = cfg_popup_font_underline2;
		lf.lfStrikeOut = cfg_popup_font_strikeout2;

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

	ps->line_text = NULL;

	temp_ll = temp_ll->next;

	if ( temp_ll == NULL || ( temp_ll != NULL && temp_ll->data == NULL ) )
	{
		return;
	}

	ps = ( POPUP_SETTINGS * )temp_ll->data;
	ps->shared_settings = shared_settings;
	ps->window_settings.drag_position.x = 0;
	ps->window_settings.drag_position.y = 0;
	ps->window_settings.window_position.x = 0;
	ps->window_settings.window_position.y = 0;
	ps->window_settings.is_dragging = false;
	ps->popup_line_order = cfg_popup_line_order3;
	ps->popup_justify = cfg_popup_justify_line3;
	ps->font_color = cfg_popup_font_color3;
	ps->font_shadow = cfg_popup_font_shadow3;
	ps->font_shadow_color = cfg_popup_font_shadow_color3;
	ps->font_height = cfg_popup_font_height3;
	ps->font_italic = cfg_popup_font_italic3;
	ps->font_weight = cfg_popup_font_weight3;
	ps->font_strikeout = cfg_popup_font_strikeout3;
	ps->font_underline = cfg_popup_font_underline3;
	if ( ps->font_face != NULL )
	{
		GlobalFree( ps->font_face );
	}
	ps->font_face = GlobalStrDupW( cfg_popup_font_face3 );
	
	if ( cfg_popup_font_face3 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face3, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.

		lf.lfHeight = cfg_popup_font_height3;
		lf.lfWeight = cfg_popup_font_weight3;
		lf.lfItalic = cfg_popup_font_italic3;
		lf.lfUnderline = cfg_popup_font_underline3;
		lf.lfStrikeOut = cfg_popup_font_strikeout3;

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

	ps->line_text = NULL;
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
				// Do not free shared_settings->ringtone_info.
				GlobalFree( popup_settings->shared_settings );
			}

			_DeleteObject( popup_settings->font );
			GlobalFree( popup_settings->font_face );
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

void Enable_Disable_Windows( POPUP_SETTINGS *ps )
{
	if ( ps->popup_line_order > 0 )
	{
		_EnableWindow( g_hWnd_btn_font_shadow_color, ( ps->font_shadow ? TRUE : FALSE ) );

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

		if ( _abs( ps->popup_line_order ) == LINE_TIME )
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

		if ( _abs( ps->popup_line_order ) == LINE_TIME )
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
		unsigned char order = _abs( ( ( POPUP_SETTINGS * )ll->data )->popup_line_order );

		TREEITEM *ti = ( TREEITEM * )GlobalAlloc( GMEM_FIXED, sizeof( TREEITEM ) );
		ti->node = ll;
		ti->index = count++;

		TVINSERTSTRUCT tvis;
		_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
		tvis.item.lParam = ( LPARAM )ti;
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		tvis.item.state = INDEXTOSTATEIMAGEMASK( ( ( ( POPUP_SETTINGS * )ll->data )->popup_line_order > 0 ? 2 : 1 ) );

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

	_SendMessageW( g_hWnd_ud_timeout, UDM_SETPOS, 0, cfg_connection_timeout );
	if ( cfg_connection_timeout == 0 )
	{
		_SendMessageW( g_hWnd_timeout, WM_SETTEXT, 0, ( LPARAM )L"0" );
	}
	_SendMessageW( g_hWnd_ud_retries, UDM_SETPOS, 0, cfg_connection_retries );
	_SendMessageW( g_hWnd_ud_width, UDM_SETPOS, 0, cfg_popup_width );
	_SendMessageW( g_hWnd_ud_height, UDM_SETPOS, 0, cfg_popup_height );
	_SendMessageW( g_hWnd_position, CB_SETCURSEL, cfg_popup_position, 0 );
	_SendMessageW( g_hWnd_ud_time, UDM_SETPOS, 0, cfg_popup_time );
	_SendMessageW( g_hWnd_ud_transparency, UDM_SETPOS, 0, cfg_popup_transparency );

	_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, cfg_connection_ssl_version, 0 );

	_SendMessageW( g_hWnd_chk_minimize, BM_SETCHECK, ( cfg_minimize_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
	_SendMessageW( g_hWnd_chk_close, BM_SETCHECK, ( cfg_close_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
	_SendMessageW( g_hWnd_chk_silent_startup, BM_SETCHECK, ( cfg_silent_startup ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_always_on_top, BM_SETCHECK, ( cfg_always_on_top ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_enable_history, BM_SETCHECK, ( cfg_enable_call_log_history ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_message_log, BM_SETCHECK, ( cfg_enable_message_log ? BST_CHECKED : BST_UNCHECKED ), 0 );

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

	_SendMessageW( g_hWnd_chk_check_for_updates, BM_SETCHECK, ( cfg_check_for_updates ? BST_CHECKED : BST_UNCHECKED ), 0 );

	if ( cfg_connection_reconnect )
	{
		_SendMessageW( g_hWnd_chk_reconnect, BM_SETCHECK, BST_CHECKED, 0 );
		_EnableWindow( g_hWnd_static_retry, TRUE );
		_EnableWindow( g_hWnd_retries, TRUE );
		_EnableWindow( g_hWnd_ud_retries, TRUE );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_reconnect, BM_SETCHECK, BST_UNCHECKED, 0 );
		_EnableWindow( g_hWnd_static_retry, FALSE );
		_EnableWindow( g_hWnd_retries, FALSE );
		_EnableWindow( g_hWnd_ud_retries, FALSE );
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

	_EnableWindow( g_hWnd_btn_font_shadow_color, ( cfg_popup_font_shadow1 ? TRUE : FALSE ) );

	_SendMessageW( g_hWnd_chk_contact_picture, BM_SETCHECK, ( cfg_popup_show_contact_picture ? BST_CHECKED : BST_UNCHECKED ), 0 );
	_SendMessageW( g_hWnd_chk_border, BM_SETCHECK, ( cfg_popup_hide_border ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_shadow, BM_SETCHECK, ( cfg_popup_font_shadow1 ? BST_CHECKED : BST_UNCHECKED ), 0 );

	if ( cfg_popup_justify_line1 == 2 )
	{
		_SendMessageW( g_hWnd_rad_right_justify, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_rad_center_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
		_SendMessageW( g_hWnd_rad_left_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
	}
	else if ( cfg_popup_justify_line1 == 1 )
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

	if ( _abs( cfg_popup_line_order1 ) == LINE_TIME )
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
		modeminfo *mi = ( modeminfo * )node->val;

		if ( mi != NULL )
		{
			int add_index = _SendMessageW( g_hWnd_default_modem, CB_ADDSTRING, 0, ( LPARAM )mi->line_name );

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
		ringtoneinfo *rti = ( ringtoneinfo * )node->val;

		if ( rti != NULL )
		{
			int add_index = _SendMessageW( g_hWnd_default_recording, CB_ADDSTRING, 0, ( LPARAM )rti->ringtone_file );

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
		ringtoneinfo *rti = ( ringtoneinfo * )node->val;

		if ( rti != NULL )
		{
			int add_index = _SendMessageW( g_hWnd_default_ringtone, CB_ADDSTRING, 0, ( LPARAM )rti->ringtone_file );

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




LRESULT CALLBACK ConnectionTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			g_hWnd_chk_reconnect = _CreateWindowW( WC_BUTTON, ST_Reconnect_upon_connection_loss_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 200, 20, hWnd, ( HMENU )BTN_RECONNECT, NULL, NULL );

			g_hWnd_static_retry = _CreateWindowW( WC_STATIC, ST_Retries_, WS_CHILD | WS_VISIBLE, 15, 20, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retries = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 35, 60, 20, hWnd, ( HMENU )EDIT_RETRIES, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_retries = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 75, 34, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retries, EM_LIMITTEXT, 2, 0 );
			_SendMessageW( g_hWnd_ud_retries, UDM_SETBUDDY, ( WPARAM )g_hWnd_retries, 0 );
            _SendMessageW( g_hWnd_ud_retries, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_retries, UDM_SETRANGE32, 1, 10 );


			g_hWnd_static_timeout = _CreateWindowW( WC_STATIC, ST_Timeout__seconds__, WS_CHILD | WS_VISIBLE, 0, 65, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_timeout = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 60, 20, hWnd, ( HMENU )EDIT_TIMEOUT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_timeout = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 79, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_timeout, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_timeout, UDM_SETBUDDY, ( WPARAM )g_hWnd_timeout, 0 );
            _SendMessageW( g_hWnd_ud_timeout, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_timeout, UDM_SETRANGE32, 60, 300 );

			g_hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 115, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 130, 100, 20, hWnd, ( HMENU )CB_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_2_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_3_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_1 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_2 );

			g_hWnd_chk_check_for_updates = _CreateWindowW( WC_BUTTON, ST_Check_for_updates_upon_startup, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 165, 200, 20, hWnd, ( HMENU )BTN_CHECK_FOR_UPDATES, NULL, NULL );

			_SendMessageW( g_hWnd_chk_reconnect, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_retry, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_retries, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_check_for_updates, WM_SETFONT, ( WPARAM )hFont, 0 );

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
				case BTN_RECONNECT:
				{
					if ( _SendMessageW( g_hWnd_chk_reconnect, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_static_retry, TRUE );
						_EnableWindow( g_hWnd_retries, TRUE );
						_EnableWindow( g_hWnd_ud_retries, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_static_retry, FALSE );
						_EnableWindow( g_hWnd_retries, FALSE );
						_EnableWindow( g_hWnd_ud_retries, FALSE );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_RETRIES:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 3, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 10 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"10" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_connection_retries )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_TIMEOUT:
				{
					DWORD sel_start = 0;
					char value[ 11 ];

					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 300 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"300" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_connection_timeout )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
					else if ( HIWORD( wParam ) == EN_KILLFOCUS )
					{
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 0 && num < 60 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"60" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_connection_timeout )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case BTN_CHECK_FOR_UPDATES:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case CB_SSL_VERSION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						if ( cfg_connection_ssl_version != ( unsigned char )_SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 ) )
						{
							_MessageBoxW( hWnd, ST_must_restart_program, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
						}

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
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

LRESULT CALLBACK ModemTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static_default_modem = _CreateWindowW( WC_STATIC, ST_Default_modem_, WS_CHILD | WS_VISIBLE, 0, 0, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_modem = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 15, 250, 20, hWnd, ( HMENU )CB_DEFAULT_MODEM, NULL, NULL );

			g_hWnd_chk_enable_recording = _CreateWindowW( WC_BUTTON, ST_Play_recording_when_call_is_answered, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 50, 250, 20, hWnd, ( HMENU )BTN_ENABLE_RECORDING, NULL, NULL );
			g_hWnd_static_default_recording = _CreateWindowW( WC_STATIC, ST_Default_recording_, WS_CHILD | WS_VISIBLE, 0, 72, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_recording = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, L"", CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 100, 70, 175, 20, hWnd, ( HMENU )CB_DEFAULT_RECORDING, NULL, NULL );
			g_hWnd_play_default_recording = _CreateWindowW( WC_BUTTON, ST_Play, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 285, 70, 50, 20, hWnd, ( HMENU )BTN_PLAY_DEFAULT_RECORDING, NULL, NULL );
			g_hWnd_stop_default_recording = _CreateWindowW( WC_BUTTON, ST_Stop, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 340, 70, 50, 20, hWnd, ( HMENU )BTN_STOP_DEFAULT_RECORDING, NULL, NULL );


			g_hWnd_static_repeat_recording = _CreateWindowW( WC_STATIC, ST_Repeat_recording_, WS_CHILD | WS_VISIBLE, 0, 105, 200, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_repeat_recording = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, 60, 20, hWnd, ( HMENU )EDIT_REPEAT_RECORDING, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_repeat_recording = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 119, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_repeat_recording, EM_LIMITTEXT, 2, 0 );
			_SendMessageW( g_hWnd_ud_repeat_recording, UDM_SETBUDDY, ( WPARAM )g_hWnd_repeat_recording, 0 );
            _SendMessageW( g_hWnd_ud_repeat_recording, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_repeat_recording, UDM_SETRANGE32, 1, 10 );


			HWND hWnd_static_droptime = _CreateWindowW( WC_STATIC, ST_Drop_answered_calls_after__seconds__, WS_CHILD | WS_VISIBLE, 0, 155, 200, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_droptime = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 170, 60, 20, hWnd, ( HMENU )EDIT_DROPTIME, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_droptime = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 169, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_droptime, EM_LIMITTEXT, 2, 0 );
			_SendMessageW( g_hWnd_ud_droptime, UDM_SETBUDDY, ( WPARAM )g_hWnd_droptime, 0 );
            _SendMessageW( g_hWnd_ud_droptime, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_droptime, UDM_SETRANGE32, 1, 10 );

			HWND hWnd_static_droptime_info = _CreateWindowW( WC_STATIC, ST_Recordings_take_priority_if_longer_, WS_CHILD | WS_VISIBLE, 70 + _GetSystemMetrics( SM_CXVSCROLL ), 173, 200, 15, hWnd, NULL, NULL, NULL );


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
			switch( LOWORD( wParam ) )
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
						int current_modem_index = _SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 );
						if ( current_modem_index != CB_ERR )
						{
							modeminfo *mi = ( modeminfo * )_SendMessageW( ( HWND )lParam, CB_GETITEMDATA, current_modem_index, 0 );
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

					int current_recording_index = _SendMessageW( g_hWnd_default_recording, CB_GETCURSEL, 0, 0 );
					if ( current_recording_index != CB_ERR )
					{
						ringtoneinfo *rti = ( ringtoneinfo * )_SendMessageW( g_hWnd_default_recording, CB_GETITEMDATA, current_recording_index, 0 );
						if ( rti != NULL && rti != ( ringtoneinfo * )CB_ERR )
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

			g_hWnd_static_hoz1 = CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 25, rc.right - 10, 5, hWnd, NULL, NULL, NULL );

			g_hWnd_static_width = _CreateWindowW( WC_STATIC, ST_Width__pixels__, WS_CHILD | WS_VISIBLE, 0, 30, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_width = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 45, 60, 20, hWnd, ( HMENU )EDIT_WIDTH, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_width = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 44, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_width, EM_LIMITTEXT, 4, 0 );
			_SendMessageW( g_hWnd_ud_width, UDM_SETBUDDY, ( WPARAM )g_hWnd_width, 0 );
            _SendMessageW( g_hWnd_ud_width, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_width, UDM_SETRANGE32, 20, wa.right );


			g_hWnd_static_height = _CreateWindowW( WC_STATIC, ST_Height__pixels__, WS_CHILD | WS_VISIBLE, 100, 30, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_height = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 45, 60, 20, hWnd, ( HMENU )EDIT_HEIGHT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_height = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 160, 44, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_height, EM_LIMITTEXT, 4, 0 );
			_SendMessageW( g_hWnd_ud_height, UDM_SETBUDDY, ( WPARAM )g_hWnd_height, 0 );
            _SendMessageW( g_hWnd_ud_height, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_height, UDM_SETRANGE32, 20, wa.bottom );
			

			g_hWnd_static_position = _CreateWindowW( WC_STATIC, ST_Screen_Position_, WS_CHILD | WS_VISIBLE, 200, 30, 140, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_position = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, L"", CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 200, 45, 140, 20, hWnd, ( HMENU )CB_POSITION, NULL, NULL );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Top_Left );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Top_Right );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Bottom_Left );
			_SendMessageW( g_hWnd_position, CB_ADDSTRING, 0, ( LPARAM )ST_Bottom_Right );


			g_hWnd_static_transparency = _CreateWindowW( WC_STATIC, ST_Transparency_, WS_CHILD | WS_VISIBLE, 0, 70, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_transparency = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 85, 60, 20, hWnd, ( HMENU )EDIT_TRANSPARENCY, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_transparency = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 60, 84, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_transparency, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_transparency, UDM_SETBUDDY, ( WPARAM )g_hWnd_transparency, 0 );
            _SendMessageW( g_hWnd_ud_transparency, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_transparency, UDM_SETRANGE32, 0, 255 );

			g_hWnd_static_delay = _CreateWindowW( WC_STATIC, ST_Delay_Time__seconds__, WS_CHILD | WS_VISIBLE, 100, 70, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_time = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 85, 60, 20, hWnd, ( HMENU )EDIT_TIME, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_time = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 160, 84, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_time, EM_LIMITTEXT, 3, 0 );	// This should be based on MAX_POPUP_TIME.
			_SendMessageW( g_hWnd_ud_time, UDM_SETBUDDY, ( WPARAM )g_hWnd_time, 0 );
            _SendMessageW( g_hWnd_ud_time, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_time, UDM_SETRANGE32, 0, MAX_POPUP_TIME );


			g_hWnd_chk_border = _CreateWindowW( WC_BUTTON, ST_Hide_window_border, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 110, 130, 20, hWnd, ( HMENU )BTN_BORDER, NULL, NULL );

			g_hWnd_chk_contact_picture = _CreateWindowW( WC_BUTTON, ST_Show_contact_picture, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 130, 110, 130, 20, hWnd, ( HMENU )BTN_CONTACT_PICTURE, NULL, NULL );

			g_hWnd_chk_enable_ringtones = _CreateWindowW( WC_BUTTON, ST_Enable_ringtone_s_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 130, 150, 20, hWnd, ( HMENU )BTN_ENABLE_RINGTONES, NULL, NULL );
			g_hWnd_static_default_ringtone = _CreateWindowW( WC_STATIC, ST_Default_ringtone_, WS_CHILD | WS_VISIBLE, 0, 152, 95, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_ringtone = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, L"", CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 95, 150, 175, 20, hWnd, ( HMENU )CB_DEFAULT_RINGTONE, NULL, NULL );
			_SendMessageW( g_hWnd_default_ringtone, CB_ADDSTRING, 0, ( LPARAM )L"" );
			_SendMessageW( g_hWnd_default_ringtone, CB_SETCURSEL, 0, 0 );
			g_hWnd_play_default_ringtone = _CreateWindowW( WC_BUTTON, ST_Play, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 280, 150, 50, 20, hWnd, ( HMENU )BTN_PLAY_DEFAULT_RINGTONE, NULL, NULL );
			g_hWnd_stop_default_ringtone = _CreateWindowW( WC_BUTTON, ST_Stop, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 335, 150, 50, 20, hWnd, ( HMENU )BTN_STOP_DEFAULT_RINGTONE, NULL, NULL );


			g_hWnd_static_hoz2 = CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 180, rc.right - 10, 5, hWnd, NULL, NULL, NULL );


			g_hWnd_font_settings = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL, TVS_DISABLEDRAGDROP | TVS_FULLROWSELECT | TVS_SHOWSELALWAYS | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 190, 130, 60, hWnd, ( HMENU )LIST_SETTINGS, NULL, NULL );
			DWORD style = _GetWindowLongW( g_hWnd_font_settings, GWL_STYLE );
			_SetWindowLongW( g_hWnd_font_settings, GWL_STYLE, style | TVS_CHECKBOXES );

			g_hWnd_move_up = _CreateWindowW( WC_BUTTON, L"\u2191"/*ST_Up*/, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 135, 190, 20, 20, hWnd, ( HMENU )BTN_MOVE_UP, NULL, NULL );
			g_hWnd_move_down = _CreateWindowW( WC_BUTTON, L"\u2193"/*ST_Down*/, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 135, 230, 20, 20, hWnd, ( HMENU )BTN_MOVE_DOWN, NULL, NULL );


			g_hWnd_font = _CreateWindowW( WC_STATIC, ST_Font_, WS_CHILD | WS_VISIBLE, 0, 260, 175, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_font = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 257, 35, 20, hWnd, ( HMENU )BTN_CHOOSE_FONT, NULL, NULL );

			g_hWnd_font_color = _CreateWindowW( WC_STATIC, ST_Font_color_, WS_CHILD | WS_VISIBLE, 0, 285, 175, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_font_color = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 282, 35, 20, hWnd, ( HMENU )BTN_FONT_COLOR, NULL, NULL );

			g_hWnd_chk_shadow = _CreateWindowW( WC_BUTTON, ST_Font_shadow_color_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 310, 175, 15, hWnd, ( HMENU )BTN_SHADOW, NULL, NULL );
			g_hWnd_btn_font_shadow_color = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 307, 35, 20, hWnd, ( HMENU )BTN_FONT_SHADOW_COLOR, NULL, NULL );

			g_hWnd_static_background_color = _CreateWindowW( WC_STATIC, ST_Background_color_, WS_CHILD | WS_VISIBLE, 0, 335, 175, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_background_color = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 332, 35, 20, hWnd, ( HMENU )BTN_BACKGROUND_COLOR, NULL, NULL );

			g_hWnd_chk_gradient = _CreateWindowW( WC_BUTTON, ST_Gradient_background_color_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 360, 175, 15, hWnd, ( HMENU )BTN_GRADIENT, NULL, NULL );
			g_hWnd_btn_background_color2 = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 357, 35, 20, hWnd, ( HMENU )BTN_BACKGROUND_COLOR2, NULL, NULL );


			g_hWnd_group_gradient_direction = _CreateWindowW( WC_STATIC, ST_Gradient_direction_, WS_CHILD | WS_VISIBLE, 15, 380, 110, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_rad_gradient_vert = _CreateWindowW( WC_BUTTON, ST_Vertical, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 125, 377, 80, 20, hWnd, ( HMENU )RADIO_GRADIENT_VERT, NULL, NULL );
			g_hWnd_rad_gradient_horz = _CreateWindowW( WC_BUTTON, ST_Horizontal, BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE, 125, 397, 80, 20, hWnd, ( HMENU )RADIO_GRADIENT_HORZ, NULL, NULL );


			g_hWnd_group_justify = _CreateWindowW( WC_STATIC, ST_Justify_text_, WS_CHILD | WS_VISIBLE, 220, 260, 80, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_rad_left_justify = _CreateWindowW( WC_BUTTON, ST_Left, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 300, 257, 80, 20, hWnd, ( HMENU )RADIO_LEFT, NULL, NULL );
			g_hWnd_rad_center_justify = _CreateWindowW( WC_BUTTON, ST_Center, BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE, 300, 277, 80, 20, hWnd, ( HMENU )RADIO_CENTER, NULL, NULL );
			g_hWnd_rad_right_justify = _CreateWindowW( WC_BUTTON, ST_Right, BS_AUTORADIOBUTTON  | WS_CHILD | WS_VISIBLE, 300, 297, 80, 20, hWnd, ( HMENU )RADIO_RIGHT, NULL, NULL );

			g_hWnd_group_time_format = _CreateWindowW( WC_STATIC, ST_Time_format_, WS_CHILD | WS_VISIBLE, 220, 330, 80, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_rad_12_hour = _CreateWindowW( WC_BUTTON, ST_12_hour, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 300, 327, 80, 20, hWnd, ( HMENU )RADIO_12_HOUR, NULL, NULL );
			g_hWnd_rad_24_hour = _CreateWindowW( WC_BUTTON, ST_24_hour, BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE, 300, 347, 80, 20, hWnd, ( HMENU )RADIO_24_HOUR, NULL, NULL );



			//g_hWnd_static_sample = _CreateWindowW( WC_STATIC, ST_Sample_, WS_CHILD | WS_VISIBLE, 220, 190, 80, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_static_example = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 175, 190, 210, 60, hWnd, NULL, NULL, NULL );



			g_hWnd_preview = _CreateWindowW( WC_BUTTON, ST_Preview_Popup, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 285, 0, 100, 20, hWnd, ( HMENU )BTN_PREVIEW, NULL, NULL );



			popup_tab_scroll_pos = 0;
			popup_tab_height = ( rc.bottom - rc.top ) + 35;

			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = popup_tab_height;
			si.nPage = ( ( popup_tab_height / 2 ) + ( ( popup_tab_height % 2 ) != 0 ) );
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
			int delta = 0;

			if ( msg == WM_VSCROLL )
			{
				// Only process the standard scroll bar.
				if ( lParam != NULL )
				{
					return _DefWindowProcW( hWnd, msg, wParam, lParam );
				}

				switch ( LOWORD( wParam ) )
				{
					case SB_LINEUP: { delta = -10; } break;
					case SB_LINEDOWN: { delta = 10; } break;
					case SB_PAGEUP: { delta = -50; } break;
					case SB_PAGEDOWN: { delta = 50; } break;
					//case SB_THUMBPOSITION:
					case SB_THUMBTRACK: { delta = ( int )HIWORD( wParam ) - popup_tab_scroll_pos; } break;
					default: { return 0; } break;
				}
			}
			else if ( msg == WM_MOUSEWHEEL )
			{
				delta = -( GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA ) * 20;
			}

			popup_tab_scroll_pos += delta;

			if ( popup_tab_scroll_pos < 0 )
			{
				delta -= popup_tab_scroll_pos;
				popup_tab_scroll_pos = 0;
			}
			else if ( popup_tab_scroll_pos > ( ( popup_tab_height / 2 ) + ( ( popup_tab_height % 2 ) != 0 ) ) )
			{
				delta -= ( popup_tab_scroll_pos - ( ( popup_tab_height / 2 ) + ( ( popup_tab_height % 2 ) != 0 ) ) );
				popup_tab_scroll_pos = ( ( popup_tab_height / 2 ) + ( ( popup_tab_height % 2 ) != 0 ) );
			}

			if ( delta != 0 )
			{
				_SetScrollPos( hWnd, SB_VERT, popup_tab_scroll_pos, TRUE );
				_ScrollWindow( hWnd, 0, -delta, NULL, NULL );
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
						if ( ( tvi.state & INDEXTOSTATEIMAGEMASK( 1 ) && ps->popup_line_order > 0 ) || ( tvi.state & INDEXTOSTATEIMAGEMASK( 2 ) && ps->popup_line_order < 0 ) )
						{
							ps->popup_line_order = -( ps->popup_line_order );
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

						if ( ps->popup_justify == 2 )
						{
							_SendMessageW( g_hWnd_rad_right_justify, BM_SETCHECK, BST_CHECKED, 0 );
							_SendMessageW( g_hWnd_rad_center_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
							_SendMessageW( g_hWnd_rad_left_justify, BM_SETCHECK, BST_UNCHECKED, 0 );
						}
						else if ( ps->popup_justify == 1 )
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

						_SendMessageW( g_hWnd_chk_shadow, BM_SETCHECK, ( ps->font_shadow ? BST_CHECKED : BST_UNCHECKED ), 0 );

						if ( _abs( ps->popup_line_order ) == LINE_TIME )
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
			switch( LOWORD( wParam ) )
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

									ti1->index--;
									tvi.state = tvi2.state;
									tvi.pszText = item_text2;
									tvi.lParam = ( LPARAM )ti2;
									_SendMessageW( g_hWnd_font_settings, TVM_SETITEM, 0, ( LPARAM )&tvi );

									ti2->index++;
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

									ti1->index++;
									tvi.state = tvi2.state;
									tvi.pszText = item_text2;
									tvi.lParam = ( LPARAM )ti2;
									_SendMessageW( g_hWnd_font_settings, TVM_SETITEM, 0, ( LPARAM )&tvi );

									ti2->index--;
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
								ps->popup_justify = ( LOWORD( wParam ) == RADIO_RIGHT ? 2 : ( LOWORD( wParam ) == RADIO_CENTER ? 1 : 0 ) );

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

					int index = _SendMessageW( g_hWnd_position, CB_GETCURSEL, 0, 0 );

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

					_SetWindowLongW( hWnd_popup, GWL_EXSTYLE, _GetWindowLongW( hWnd_popup, GWL_EXSTYLE ) | WS_EX_LAYERED );
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
					shared_settings->ringtone_info = ss->ringtone_info;
					shared_settings->contact_picture_info.free_picture_path = show_contact_picture;
					shared_settings->contact_picture_info.picture_path = program_data_folder;

					DoublyLinkedList *t_ll = NULL;

					while ( temp_ll != NULL )
					{
						ps = ( POPUP_SETTINGS * )temp_ll->data;

						POPUP_SETTINGS *ls = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
						ls->shared_settings = shared_settings;
						ls->window_settings = ps->window_settings;
						ls->popup_line_order = ps->popup_line_order;
						ls->popup_justify = ps->popup_justify;
						ls->font_color = ps->font_color;
						ls->font_shadow = ps->font_shadow;
						ls->font_shadow_color = ps->font_shadow_color;
						ls->font_face = NULL;
						ls->line_text = ( _abs( ps->popup_line_order ) == LINE_PHONE ? phone_line : ( _abs( ps->popup_line_order ) == LINE_CALLER_ID ? caller_id_line : time_line ) );

						if ( ps->font_face != NULL )
						{
							_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, ps->font_face, LF_FACESIZE );
							lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.
							lf.lfHeight = ps->font_height;
							lf.lfWeight = ps->font_weight;
							lf.lfItalic = ps->font_italic;
							lf.lfUnderline = ps->font_underline;
							lf.lfStrikeOut = ps->font_strikeout;
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

					_SetWindowLongW( hWnd_popup, 0, ( LONG_PTR )t_ll );

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
								cc.rgbResult = ( LOWORD( wParam ) == BTN_FONT_COLOR ? ps->font_color : ps->font_shadow_color );

								if ( _ChooseColorW( &cc ) == TRUE )
								{
									if ( LOWORD( wParam ) == BTN_FONT_COLOR )
									{
										ps->font_color = cc.rgbResult;
									}
									else
									{
										ps->font_shadow_color = cc.rgbResult;
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

								cf.rgbColors = ps->font_color;
								_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, ps->font_face, LF_FACESIZE );
								lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.
								lf.lfHeight = ps->font_height;
								lf.lfWeight = ps->font_weight;
								lf.lfItalic = ps->font_italic;
								lf.lfUnderline = ps->font_underline;
								lf.lfStrikeOut = ps->font_strikeout;

								if ( _ChooseFontW( &cf ) == TRUE )
								{
									if ( ps->font_face != NULL )
									{
										GlobalFree( ps->font_face );
									}
									ps->font_face = GlobalStrDupW( lf.lfFaceName );
									ps->font_color = cf.rgbColors;
									ps->font_height = lf.lfHeight;
									ps->font_weight = lf.lfWeight;
									ps->font_italic = lf.lfItalic;
									ps->font_underline = lf.lfUnderline;
									ps->font_strikeout = lf.lfStrikeOut;

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
								ps->font_shadow = ( _SendMessageW( g_hWnd_chk_shadow, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

								if ( ps->font_shadow )
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

							int current_ringtone_index = _SendMessageW( g_hWnd_default_ringtone, CB_GETCURSEL, 0, 0 );
							if ( current_ringtone_index == 0 )
							{
								shared_settings->ringtone_info = NULL;

								_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
								_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
							}
							else
							{
								ringtoneinfo *rti = ( ringtoneinfo * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
								if ( rti != NULL && rti != ( ringtoneinfo * )CB_ERR )
								{
									shared_settings->ringtone_info = rti;
								}
								else
								{
									shared_settings->ringtone_info = NULL;
								}

								_EnableWindow( g_hWnd_play_default_ringtone, TRUE );
								_EnableWindow( g_hWnd_stop_default_ringtone, TRUE );
							}
						}
						else
						{
							shared_settings->ringtone_info = NULL;

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

							int current_ringtone_index = _SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 );
							if ( current_ringtone_index == 0 )
							{
								shared_settings->ringtone_info = NULL;

								_EnableWindow( g_hWnd_play_default_ringtone, FALSE );
								_EnableWindow( g_hWnd_stop_default_ringtone, FALSE );
							}
							else
							{
								ringtoneinfo *rti = ( ringtoneinfo * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
								shared_settings->ringtone_info = ( ( rti != NULL && rti != ( ringtoneinfo * )CB_ERR ) ? rti : NULL );

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

					int current_ringtone_index = _SendMessageW( g_hWnd_default_ringtone, CB_GETCURSEL, 0, 0 );
					if ( current_ringtone_index > 0 )
					{
						ringtoneinfo *rti = ( ringtoneinfo * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
						if ( rti != NULL && rti != ( ringtoneinfo * )CB_ERR )
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

								if ( p_s->popup_line_order > 0 )
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

									if ( p_s->font_shadow )
									{
										RECT rc_shadow_line = rc_line;
										rc_shadow_line.left += 2;
										rc_shadow_line.top += 2;
										rc_shadow_line.right += 2;
										rc_shadow_line.bottom += 2;

										_SetTextColor( hdcMem, p_s->font_shadow_color );

										_DrawTextW( hdcMem, L"AaBbYyZz", -1, &rc_shadow_line, ( p_s->popup_justify == 0 ? DT_LEFT : ( p_s->popup_justify == 1 ? DT_CENTER : DT_RIGHT ) ) );
									}

									_SetTextColor( hdcMem, p_s->font_color );

									_DrawTextW( hdcMem, L"AaBbYyZz", -1, &rc_line, ( p_s->popup_justify == 0 ? DT_LEFT : ( p_s->popup_justify == 1 ? DT_CENTER : DT_RIGHT ) ) );
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

			g_hWnd_chk_always_on_top = _CreateWindowW( WC_BUTTON, ST_Always_on_top, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 90, 200, 20, hWnd, ( HMENU )BTN_ALWAYS_ON_TOP, NULL, NULL );

			g_hWnd_chk_enable_history = _CreateWindowW( WC_BUTTON, ST_Enable_Call_Log_history, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, 200, 20, hWnd, ( HMENU )BTN_ENABLE_HISTORY, NULL, NULL );

			g_hWnd_chk_message_log = _CreateWindowW( WC_BUTTON, ST_Log_events_to_Message_Log, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 150, 200, 20, hWnd, ( HMENU )BTN_ENABLE_MESSAGE_LOG, NULL, NULL );

			_SendMessageW( g_hWnd_chk_tray_icon, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_minimize, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_close, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_silent_startup, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_enable_history, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_message_log, WM_SETFONT, ( WPARAM )hFont, 0 );

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

LRESULT CALLBACK OptionsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_General;
			ti.lParam = ( LPARAM )&g_hWnd_general_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Connection;
			ti.lParam = ( LPARAM )&g_hWnd_connection_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Modem;
			ti.lParam = ( LPARAM )&g_hWnd_modem_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Popup;
			ti.lParam = ( LPARAM )&g_hWnd_popup_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 3, ( LPARAM )&ti );	// Insert a new tab at the end.

			HWND g_hWnd_ok = _CreateWindowW( WC_BUTTON, ST_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 260, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );
			HWND g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CANCEL, NULL, NULL );
			g_hWnd_apply = _CreateWindowW( WC_BUTTON, ST_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_APPLY, NULL, NULL );

			_SendMessageW( g_hWnd_options_tab, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Set the tab control's font so we can get the correct tab height to adjust its children.
			RECT rc_tab;
			_GetClientRect( g_hWnd_options_tab, &rc );

			_SendMessageW( g_hWnd_options_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			g_hWnd_general_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"general_tab", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_connection_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"connection_tab", NULL, WS_CHILD | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_modem_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"modem_tab", NULL, WS_CHILD | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_popup_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"popup_tab", NULL, WS_CHILD | WS_VSCROLL | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );

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
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_HIDE );
					}

					return FALSE;
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
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_SHOW );

						_SetFocus( *( ( HWND * )tie.lParam ) );
					}

					return FALSE;
                }
				break;
			}

			return FALSE;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
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

							ringtoneinfo *rti = NULL;
							wchar_t *ringtone = NULL;
							int current_ringtone_index = _SendMessageW( g_hWnd_default_ringtone, CB_GETCURSEL, 0, 0 );
							if ( current_ringtone_index != CB_ERR )
							{
								if ( current_ringtone_index == 0 )
								{
									ringtone = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) );
								}
								else
								{
									rti = ( ringtoneinfo * )_SendMessageW( g_hWnd_default_ringtone, CB_GETITEMDATA, current_ringtone_index, 0 );
									if ( rti != NULL )
									{
										if ( rti != ( ringtoneinfo * )CB_ERR )
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

							cfg_popup_line_order1 = ps->popup_line_order;
							cfg_popup_font_color1 = ps->font_color;
							cfg_popup_font_shadow1 = ps->font_shadow;
							cfg_popup_font_shadow_color1 = ps->font_shadow_color;
							cfg_popup_justify_line1 = ps->popup_justify;
							cfg_popup_font_height1 = ps->font_height;
							cfg_popup_font_italic1 = ps->font_italic;
							cfg_popup_font_weight1 = ps->font_weight;
							cfg_popup_font_strikeout1 = ps->font_strikeout;
							cfg_popup_font_underline1 = ps->font_underline;
							if ( cfg_popup_font_face1 != NULL )
							{
								GlobalFree( cfg_popup_font_face1 );
							}
							cfg_popup_font_face1 = GlobalStrDupW( ps->font_face );
						}
						else if ( count == 1 )
						{
							cfg_popup_line_order2 = ps->popup_line_order;
							cfg_popup_font_color2 = ps->font_color;
							cfg_popup_font_shadow2 = ps->font_shadow;
							cfg_popup_font_shadow_color2 = ps->font_shadow_color;
							cfg_popup_justify_line2 = ps->popup_justify;
							cfg_popup_font_height2 = ps->font_height;
							cfg_popup_font_italic2 = ps->font_italic;
							cfg_popup_font_weight2 = ps->font_weight;
							cfg_popup_font_strikeout2 = ps->font_strikeout;
							cfg_popup_font_underline2 = ps->font_underline;
							if ( cfg_popup_font_face2 != NULL )
							{
								GlobalFree( cfg_popup_font_face2 );
							}
							cfg_popup_font_face2 = GlobalStrDupW( ps->font_face );
						}
						else if ( count == 2 )
						{
							cfg_popup_line_order3 = ps->popup_line_order;
							cfg_popup_font_color3 = ps->font_color;
							cfg_popup_font_shadow3 = ps->font_shadow;
							cfg_popup_font_shadow_color3 = ps->font_shadow_color;
							cfg_popup_justify_line3 = ps->popup_justify;
							cfg_popup_font_height3 = ps->font_height;
							cfg_popup_font_italic3 = ps->font_italic;
							cfg_popup_font_weight3 = ps->font_weight;
							cfg_popup_font_strikeout3 = ps->font_strikeout;
							cfg_popup_font_underline3 = ps->font_underline;
							if ( cfg_popup_font_face3 != NULL )
							{
								GlobalFree( cfg_popup_font_face3 );
							}
							cfg_popup_font_face3 = GlobalStrDupW( ps->font_face );
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
						_SendMessageA( g_hWnd_retries, WM_GETTEXT, 3, ( LPARAM )value );
						cfg_connection_retries = ( unsigned char )_strtoul( value, NULL, 10 );
						_SendMessageA( g_hWnd_timeout, WM_GETTEXT, 4, ( LPARAM )value );
						cfg_connection_timeout = ( unsigned short )_strtoul( value, NULL, 10 );
						_SendMessageA( g_hWnd_repeat_recording, WM_GETTEXT, 3, ( LPARAM )value );
						cfg_repeat_recording = ( unsigned char )_strtoul( value, NULL, 10 );
						_SendMessageA( g_hWnd_droptime, WM_GETTEXT, 3, ( LPARAM )value );
						cfg_drop_call_wait = ( unsigned char )_strtoul( value, NULL, 10 );

						cfg_popup_enable_recording = ( _SendMessageW( g_hWnd_chk_enable_recording, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

						ringtoneinfo *rti = NULL;
						wchar_t *recording = NULL;
						int current_recording_index = _SendMessageW( g_hWnd_default_recording, CB_GETCURSEL, 0, 0 );
						if ( current_recording_index != CB_ERR )
						{
							rti = ( ringtoneinfo * )_SendMessageW( g_hWnd_default_recording, CB_GETITEMDATA, current_recording_index, 0 );
							if ( rti != NULL )
							{
								if ( rti != ( ringtoneinfo * )CB_ERR )
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


						cfg_connection_ssl_version = ( unsigned char )_SendMessageW( g_hWnd_ssl_version, CB_GETCURSEL, 0, 0 );

						cfg_connection_reconnect = ( _SendMessageW( g_hWnd_chk_reconnect, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
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

						int current_modem_index = _SendMessageW( g_hWnd_default_modem, CB_GETCURSEL, 0, 0 );
						if ( current_modem_index != CB_ERR )
						{
							modeminfo *mi = ( modeminfo * )_SendMessageW( g_hWnd_default_modem, CB_GETITEMDATA, current_modem_index, 0 );
							if ( mi != NULL &&  mi != ( modeminfo * )CB_ERR )
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
							if ( g_hWnd_columns != NULL ){ _SetWindowPos( g_hWnd_columns, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_contact != NULL ){ _SetWindowPos( g_hWnd_contact, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_ignore_phone_number != NULL ){ _SetWindowPos( g_hWnd_ignore_phone_number, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_ignore_cid != NULL ){ _SetWindowPos( g_hWnd_ignore_cid, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
							if ( g_hWnd_message_log != NULL ){ _SetWindowPos( g_hWnd_message_log, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
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
