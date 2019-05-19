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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "globals.h"

#include "string_tables.h"

// Options Window
extern HWND g_hWnd_general_tab;
extern HWND g_hWnd_modem_tab;
extern HWND g_hWnd_popup_tab;

extern HWND g_hWnd_apply;

extern bool options_state_changed;

bool check_settings();


// General Tab
extern HWND g_hWnd_chk_tray_icon;
extern HWND g_hWnd_chk_minimize;
extern HWND g_hWnd_chk_close;
extern HWND g_hWnd_chk_silent_startup;
extern HWND g_hWnd_chk_always_on_top;
extern HWND g_hWnd_chk_enable_history;
extern HWND g_hWnd_chk_message_log;
extern HWND g_hWnd_chk_check_for_updates;


// Modem Tab
extern HWND g_hWnd_default_modem;

extern HWND g_hWnd_chk_enable_recording;
extern HWND g_hWnd_static_default_recording;
extern HWND g_hWnd_default_recording;
extern HWND g_hWnd_play_default_recording;
extern HWND g_hWnd_stop_default_recording;

extern HWND g_hWnd_static_repeat_recording;
extern HWND g_hWnd_repeat_recording;
extern HWND g_hWnd_ud_repeat_recording;

extern HWND g_hWnd_droptime;
extern HWND g_hWnd_ud_droptime;


// Popup Tab
struct TREEITEM
{
	DoublyLinkedList *node;
	unsigned int index;
};

extern HWND g_hWnd_chk_enable_popups;

extern HWND g_hWnd_static_hoz1;
extern HWND g_hWnd_static_hoz2;

extern HWND g_hWnd_height;
extern HWND g_hWnd_width;
extern HWND g_hWnd_ud_height;
extern HWND g_hWnd_ud_width;
extern HWND g_hWnd_position;
extern HWND g_hWnd_chk_contact_picture;
extern HWND g_hWnd_static_example;
extern HWND g_hWnd_time;
extern HWND g_hWnd_ud_time;
extern HWND g_hWnd_transparency;
extern HWND g_hWnd_ud_transparency;
extern HWND g_hWnd_chk_gradient;
extern HWND g_hWnd_btn_background_color2;
extern HWND g_hWnd_chk_border;
extern HWND g_hWnd_chk_shadow;
extern HWND g_hWnd_shadow_color;

extern HWND g_hWnd_font_color;
extern HWND g_hWnd_btn_font_color;
extern HWND g_hWnd_font;
extern HWND g_hWnd_btn_font;

extern HWND g_hWnd_font_settings;
extern HWND g_hWnd_move_up;
extern HWND g_hWnd_move_down;

extern HWND g_hWnd_group_justify;
extern HWND g_hWnd_rad_left_justify;
extern HWND g_hWnd_rad_center_justify;
extern HWND g_hWnd_rad_right_justify;

extern HWND g_hWnd_group_gradient_direction;
extern HWND g_hWnd_rad_gradient_horz;
extern HWND g_hWnd_rad_gradient_vert;

extern HWND g_hWnd_group_time_format;
extern HWND g_hWnd_rad_12_hour;
extern HWND g_hWnd_rad_24_hour;

extern HWND g_hWnd_btn_font_shadow_color;

extern HWND g_hWnd_static_width;
extern HWND g_hWnd_static_height;
extern HWND g_hWnd_static_position;
extern HWND g_hWnd_static_delay;
extern HWND g_hWnd_static_transparency;
extern HWND g_hWnd_static_background_color;
extern HWND g_hWnd_background_color;
extern HWND g_hWnd_preview;

extern HWND g_hWnd_chk_enable_ringtones;
extern HWND g_hWnd_static_default_ringtone;
extern HWND g_hWnd_default_ringtone;
extern HWND g_hWnd_play_default_ringtone;
extern HWND g_hWnd_stop_default_ringtone;

extern DoublyLinkedList *g_ll;

extern HTREEITEM hti_selected;

void Enable_Disable_Popup_Windows( BOOL enable );

#endif
