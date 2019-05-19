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

#ifndef _GLOBALS_H
#define _GLOBALS_H

// Pretty window.
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )

#define STRICT
#define WIN32_LEAN_AND_MEAN

//#define USE_DEBUG_DIRECTORY

#include <windows.h>
#include <commctrl.h>
#include <process.h>

#include "lite_user32.h"
#include "lite_shell32.h"
#include "lite_ntdll.h"
#include "lite_tapi32.h"

#include "doublylinkedlist.h"
#include "dllrbt.h"
#include "range.h"

#include "resource.h"

#define PROGRAM_CAPTION		L"VZ Enhanced 56K"
#define PROGRAM_CAPTION_A	"VZ Enhanced 56K"

#define HOME_PAGE			L"https://erickutcher.github.io/#VZ_Enhanced_56K"

#define MIN_WIDTH			640
#define MIN_HEIGHT			480

#define MAX_POPUP_TIME		300		// Maximum time in seconds that the popup window can be shown.
#define SNAP_WIDTH			10		// The minimum distance at which our windows will attach together.

#define WM_DESTROY_ALT		WM_APP		// Allows non-window threads to call DestroyWindow.
#define WM_PROPAGATE		WM_APP + 1
#define WM_CHANGE_CURSOR	WM_APP + 2	// Updates the window cursor.

#define WM_TRAY_NOTIFY		WM_APP + 3
#define WM_EXIT				WM_APP + 4
#define WM_ALERT			WM_APP + 5

#define FILETIME_TICKS_PER_SECOND		10000000LL
#define FILETIME_TICKS_PER_MILLISECOND	10000LL

///// Contact Window messages /////
#define CW_MODIFY			0	// Add or Update contacts.
#define CW_UPDATE			1	// Update various windows in the Contact Window.
///////////////////////////////////

#define LINE_TIME			1
#define LINE_CALLER_ID		2
#define LINE_PHONE			3
#define TOTAL_LINES			3

#define NUM_TABS			4

#define NUM_COLUMNS1		4
#define NUM_COLUMNS2		7
#define NUM_COLUMNS3		8
#define NUM_COLUMNS4		17
#define NUM_COLUMNS5		4
#define NUM_COLUMNS6		7

#define LIST_TYPE_ALLOW		0
#define LIST_TYPE_IGNORE	1

#define _wcsicmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiW( a, b ) )
#define _stricmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiA( a, b ) )

#define SAFESTRA( s ) ( s != NULL ? s : "" )
#define SAFESTR2A( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : "" ) )

#define SAFESTRW( s ) ( s != NULL ? s : L"" )
#define SAFESTR2W( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : L"" ) )

#define is_close( a, b ) ( _abs( a - b ) < SNAP_WIDTH )

#define is_digit_w( c ) ( c - L'0' + 0U <= 9U )
#define is_digit( c ) ( c - '0' + 0U <= 9U )

#define GET_X_LPARAM( lp )	( ( int )( short )LOWORD( lp ) )
#define GET_Y_LPARAM( lp )	( ( int )( short )HIWORD( lp ) )

struct contact_info;
struct allow_ignore_info;

struct modem_info
{
	GUID permanent_line_guid;
	wchar_t *line_name;
	DWORD device_id;
};

struct ringtone_info
{
	wchar_t *ringtone_path;
	wchar_t *ringtone_file;
};

struct display_info
{
	ULARGE_INTEGER time;

	contact_info *ci;

	wchar_t *caller_id;					// Caller ID
	wchar_t *w_time;					// Date and Time
	wchar_t *w_phone_number;			// Phone Number

	wchar_t *phone_number;				// The incoming phone number. Unformatted.
	wchar_t *custom_caller_id;

	HCALL incoming_call;

	unsigned int allow_cid_match_count;		// Number of allow cid matches.
	unsigned int ignore_cid_match_count;	// Number of ignore cid matches.

	bool allow_phone_number;				// The phone number is in allow_list
	bool ignore_phone_number;				// The phone number is in ignore_list

	bool ignored;							// The phone number has been ignored

	bool process_incoming;					// false, true = ignore
};

struct contact_info
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *w_cell_phone_number;	// Cell Phone Number
			wchar_t *w_business_name;		// Company
			wchar_t *w_department;			// Department
			wchar_t *w_email_address;		// Email Address
			wchar_t *w_fax_number;			// Fax Number
			wchar_t *w_first_name;			// First Name
			wchar_t *w_home_phone_number;	// Home Phone Number
			wchar_t *w_designation;			// Job Title
			wchar_t *w_last_name;			// Last Name
			wchar_t *w_nickname;			// Nickname
			wchar_t *w_office_phone_number;	// Office Phone Number
			wchar_t *w_other_phone_number;	// Other Phone Number
			wchar_t *w_category;			// Profession
			wchar_t *w_title;				// Title
			wchar_t *w_web_page;			// Web Page
			wchar_t *w_work_phone_number;	// Work Phone Number
		};

		wchar_t *w_contact_info_values[ 16 ];
	};

	union
	{
		struct	// Keep these in alphabetical order.
		{
			// Unformatted numbers.
			wchar_t *cell_phone_number;
			wchar_t *fax_number;
			wchar_t *home_phone_number;
			wchar_t *office_phone_number;
			wchar_t *other_phone_number;
			wchar_t *work_phone_number;
		};

		wchar_t *w_contact_info_phone_numbers[ 6 ];
	};

	wchar_t *picture_path;		// Local path to picture.
	ringtone_info *rti;			// Local path and file name of the ringtone.
};

struct contact_update_info
{
	contact_info *old_ci;
	contact_info *new_ci;
	unsigned char action;	// 0 Add, 1 Remove, 2 Add all from contact_list, 3 Update ci.
	bool remove_picture;
};

struct allow_ignore_info
{
	ULARGE_INTEGER last_called;

	wchar_t *w_last_called;
	wchar_t *w_phone_number;	// This number is formatted.

	wchar_t *phone_number;		// This number is unformatted and use for comparisons.

	unsigned int count;			// Number of times the call has been ignored.
	unsigned char state;		// 0 = keep, 1 = remove.
};

struct allow_ignore_cid_info
{
	ULARGE_INTEGER last_called;

	wchar_t *w_last_called;

	wchar_t *caller_id;

	unsigned int count;		// Number of times the call has been ignored.
	unsigned char state;	// 0 = keep, 1 = remove.

	unsigned char match_flag;	// 0x00 None, 0x01 Match Whole Word, 0x02 Match Case, 0x04 Regular Expression.

	bool active;			// A caller ID value has been matched.
};

struct allow_ignore_update_info
{
	ULARGE_INTEGER last_called;
	HWND hWnd;
	wchar_t *phone_number;		// Phone number to add.
	allow_ignore_info *aii;		// Allow/Ignore info to update.
	unsigned char action;		// 0 Add, 1 Remove, 2 Add all from ignore_list, 3 Update aii, 4 Update call count and last called time.
	unsigned char list_type;	// 0 Allow, 1 = Ignore.
};

struct allow_ignore_cid_update_info
{
	ULARGE_INTEGER last_called;
	HWND hWnd;
	wchar_t *caller_id;				// Caller ID to add.
	allow_ignore_cid_info *aicidi;	// Allow/Ignore info to update.
	unsigned char action;			// 0 Add, 1 Remove, 2 Add all from ignore_cid_list, 3 Update aicidi, 4 Update call count and last called time.
	unsigned char match_flag;		// 0x00 None, 0x01 Match Whole Word, 0x02 Match Case, 0x04 Regular Expression.
	unsigned char list_type;		// 0 Allow, 1 = Ignore.
};

struct sort_info
{
	HWND hWnd;
	int column;
	unsigned char direction;
};

struct WINDOW_SETTINGS
{
	POINT window_position;
	POINT drag_position;
	bool is_dragging;
};

struct CONTACT_PICTURE_INFO
{
	wchar_t *picture_path;
	HBITMAP picture;
	unsigned int height;
	unsigned int width;
	bool free_picture_path;	// Used for previewing.
};

struct SHARED_SETTINGS	// These are settings that the popup window will need.
{
	CONTACT_PICTURE_INFO contact_picture_info;
	COLORREF popup_background_color1;
	COLORREF popup_background_color2;
	ringtone_info *rti;
	unsigned short popup_time;
	unsigned char popup_gradient_direction;
	bool popup_gradient;
};

struct POPUP_INFO
{
	wchar_t *font_face;
	COLORREF font_color;
	COLORREF font_shadow_color;
	LONG font_height;
	LONG font_weight;
	BYTE font_italic;
	BYTE font_underline;
	BYTE font_strikeout;
	unsigned char justify_line;
	char line_order;
	bool font_shadow;
};

struct POPUP_SETTINGS
{
	POPUP_INFO popup_info;
	WINDOW_SETTINGS window_settings;
	HFONT font;
	SHARED_SETTINGS *shared_settings;
	wchar_t *line_text;
};

struct SEARCH_INFO
{
	wchar_t *text;
	unsigned char type;			// 0 = Caller ID Name, 1 = Phone Number
	unsigned char search_flag;	// 0x00 = None, 0x01 = Match case, 0x02 = Match whole word, 0x04 = Regular expression.
	bool search_all;
};

// These are all variables that are shared among the separate .cpp files.

extern SYSTEMTIME g_compile_time;

// Object handles.
extern HWND g_hWnd_main;				// Handle to our main window.
extern HWND g_hWnd_options;				// Handle to our options window.
extern HWND g_hWnd_search;
extern HWND g_hWnd_contact;
extern HWND g_hWnd_ignore_phone_number;
extern HWND g_hWnd_ignore_cid;
extern HWND g_hWnd_message_log;
extern HWND g_hWnd_update;
extern HWND g_hWnd_call_log;				// Handle to the call log listview control.
extern HWND g_hWnd_contact_list;
extern HWND g_hWnd_ignore_tab;
extern HWND g_hWnd_ignore_cid_list;
extern HWND g_hWnd_ignore_list;
extern HWND g_hWnd_allow_tab;
extern HWND g_hWnd_allow_cid_list;
extern HWND g_hWnd_allow_list;
extern HWND g_hWnd_tab;
extern HWND g_hWnd_active;				// Handle to the active window. Used to handle tab stops.

extern CRITICAL_SECTION pe_cs;			// Allow only one worker thread to be active.
extern CRITICAL_SECTION tt_cs;			// Allow only one telephony thread to be active.
extern CRITICAL_SECTION ac_cs;			// Allow a clean shutdown of the telephony thread if a call was answered.
extern CRITICAL_SECTION cut_cs;			// Allow only one update check thread to be active.
extern CRITICAL_SECTION cuc_cs;			// Blocks the CleanupConnection().

extern CRITICAL_SECTION ml_cs;			// Allow only one message log worker thread to be active.
extern CRITICAL_SECTION ml_update_cs;	// Allow only one message log update thread to be active.
extern CRITICAL_SECTION ml_queue_cs;	// Used when adding/removing values from the message log queue.

extern CRITICAL_SECTION ringtone_update_cs;	// Allow only one ringtone update thread to be active.
extern CRITICAL_SECTION ringtone_queue_cs;	// Used when adding/removing values from the ringtone queue.

extern HANDLE telephony_semaphore;

extern HANDLE update_check_semaphore;

extern HANDLE worker_semaphore;			// Blocks shutdown while a worker thread is active.

extern HANDLE ml_update_semaphore;
extern HANDLE ml_worker_semaphore;

extern HFONT hFont;						// Handle to the system's message font.

extern int row_height;					// Listview row height based on font size.

extern HCURSOR wait_cursor;				// Temporary cursor while processing entries.

extern NOTIFYICONDATA g_nid;			// Tray icon information.

extern wchar_t *base_directory;
extern unsigned int base_directory_length;

extern wchar_t *app_directory;
extern unsigned int app_directory_length;

// Thread variables
extern bool kill_worker_thread_flag;	// Allow for a clean shutdown.

extern unsigned char kill_telephony_thread_flag;
extern unsigned char kill_accepted_call_thread_flag;

extern bool kill_ml_update_thread_flag;
extern bool kill_ml_worker_thread_flag;
extern bool kill_ringtone_update_thread_flag;

extern bool in_worker_thread;			// Flag to indicate that we're in a worker thread.
extern bool in_telephony_thread;
extern bool in_update_check_thread;
extern bool in_ml_update_thread;
extern bool in_ml_worker_thread;
extern bool in_ringtone_update_thread;

extern bool skip_list_draw;					// Prevents WM_DRAWITEM from accessing listview items while we're removing them.

extern RANGE *allow_range_list[ 16 ];
extern RANGE *ignore_range_list[ 16 ];

extern dllrbt_tree *modem_list;
extern modem_info *default_modem;

extern dllrbt_tree *recording_list;
extern ringtone_info *default_recording;

extern dllrbt_tree *ringtone_list;
extern ringtone_info *default_ringtone;

extern dllrbt_tree *allow_list;
extern bool allow_list_changed;

extern dllrbt_tree *allow_cid_list;
extern bool allow_cid_list_changed;

extern dllrbt_tree *ignore_list;
extern bool ignore_list_changed;

extern dllrbt_tree *ignore_cid_list;
extern bool ignore_cid_list_changed;

extern dllrbt_tree *contact_list;
extern bool contact_list_changed;

extern dllrbt_tree *call_log;
extern bool call_log_changed;

extern unsigned char update_check_state;

extern unsigned char g_total_tabs;

extern unsigned char g_total_columns1;
extern unsigned char g_total_columns2;
extern unsigned char g_total_columns3;
extern unsigned char g_total_columns4;
extern unsigned char g_total_columns5;
extern unsigned char g_total_columns6;

// Saved configuration variables
extern int cfg_pos_x;
extern int cfg_pos_y;
extern int cfg_width;
extern int cfg_height;

extern char cfg_min_max;

extern bool cfg_tray_icon;
extern bool cfg_close_to_tray;
extern bool cfg_minimize_to_tray;
extern bool cfg_silent_startup;

extern bool cfg_always_on_top;

extern bool cfg_enable_call_log_history;

extern bool cfg_enable_message_log;

extern bool cfg_check_for_updates;

extern bool cfg_enable_popups;
extern bool cfg_popup_hide_border;
extern int cfg_popup_width;
extern int cfg_popup_height;
extern unsigned char cfg_popup_position;
extern bool cfg_popup_show_contact_picture;
extern unsigned short cfg_popup_time;
extern unsigned char cfg_popup_transparency;
extern bool cfg_popup_gradient;
extern unsigned char cfg_popup_gradient_direction;
extern COLORREF cfg_popup_background_color1;
extern COLORREF cfg_popup_background_color2;
extern unsigned char cfg_popup_time_format;
extern bool cfg_popup_enable_ringtones;
extern wchar_t *cfg_popup_ringtone;

extern POPUP_INFO cfg_popup_info1;
extern POPUP_INFO cfg_popup_info2;
extern POPUP_INFO cfg_popup_info3;

extern char cfg_tab_order1;
extern char cfg_tab_order2;
extern char cfg_tab_order3;
extern char cfg_tab_order4;

// Allow
extern int cfg_column_al_width1;
extern int cfg_column_al_width2;
extern int cfg_column_al_width3;
extern int cfg_column_al_width4;

extern char cfg_column_al_order1;
extern char cfg_column_al_order2;
extern char cfg_column_al_order3;
extern char cfg_column_al_order4;

// Allow CID
extern int cfg_column_acidl_width1;
extern int cfg_column_acidl_width2;
extern int cfg_column_acidl_width3;
extern int cfg_column_acidl_width4;
extern int cfg_column_acidl_width5;
extern int cfg_column_acidl_width6;
extern int cfg_column_acidl_width7;

extern char cfg_column_acidl_order1;
extern char cfg_column_acidl_order2;
extern char cfg_column_acidl_order3;
extern char cfg_column_acidl_order4;
extern char cfg_column_acidl_order5;
extern char cfg_column_acidl_order6;
extern char cfg_column_acidl_order7;

// Call Log
extern int cfg_column_cll_width1;
extern int cfg_column_cll_width2;
extern int cfg_column_cll_width3;
extern int cfg_column_cll_width4;
extern int cfg_column_cll_width5;
extern int cfg_column_cll_width6;
extern int cfg_column_cll_width7;
extern int cfg_column_cll_width8;

extern char cfg_column_cll_order1;
extern char cfg_column_cll_order2;
extern char cfg_column_cll_order3;
extern char cfg_column_cll_order4;
extern char cfg_column_cll_order5;
extern char cfg_column_cll_order6;
extern char cfg_column_cll_order7;
extern char cfg_column_cll_order8;

// Contact
extern int cfg_column_cl_width1;
extern int cfg_column_cl_width2;
extern int cfg_column_cl_width3;
extern int cfg_column_cl_width4;
extern int cfg_column_cl_width5;
extern int cfg_column_cl_width6;
extern int cfg_column_cl_width7;
extern int cfg_column_cl_width8;
extern int cfg_column_cl_width9;
extern int cfg_column_cl_width10;
extern int cfg_column_cl_width11;
extern int cfg_column_cl_width12;
extern int cfg_column_cl_width13;
extern int cfg_column_cl_width14;
extern int cfg_column_cl_width15;
extern int cfg_column_cl_width16;
extern int cfg_column_cl_width17;

extern char cfg_column_cl_order1;
extern char cfg_column_cl_order2;
extern char cfg_column_cl_order3;
extern char cfg_column_cl_order4;
extern char cfg_column_cl_order5;
extern char cfg_column_cl_order6;
extern char cfg_column_cl_order7;
extern char cfg_column_cl_order8;
extern char cfg_column_cl_order9;
extern char cfg_column_cl_order10;
extern char cfg_column_cl_order11;
extern char cfg_column_cl_order12;
extern char cfg_column_cl_order13;
extern char cfg_column_cl_order14;
extern char cfg_column_cl_order15;
extern char cfg_column_cl_order16;
extern char cfg_column_cl_order17;

// Ignore
extern int cfg_column_il_width1;
extern int cfg_column_il_width2;
extern int cfg_column_il_width3;
extern int cfg_column_il_width4;

extern char cfg_column_il_order1;
extern char cfg_column_il_order2;
extern char cfg_column_il_order3;
extern char cfg_column_il_order4;

// Ignore CID
extern int cfg_column_icidl_width1;
extern int cfg_column_icidl_width2;
extern int cfg_column_icidl_width3;
extern int cfg_column_icidl_width4;
extern int cfg_column_icidl_width5;
extern int cfg_column_icidl_width6;
extern int cfg_column_icidl_width7;

extern char cfg_column_icidl_order1;
extern char cfg_column_icidl_order2;
extern char cfg_column_icidl_order3;
extern char cfg_column_icidl_order4;
extern char cfg_column_icidl_order5;
extern char cfg_column_icidl_order6;
extern char cfg_column_icidl_order7;

extern bool cfg_popup_enable_recording;
extern wchar_t *cfg_recording;

extern unsigned char cfg_repeat_recording;

extern unsigned char cfg_drop_call_wait;
extern GUID cfg_modem_guid;

extern bool g_voice_playback_supported;
extern bool g_use_regular_expressions;

extern POPUP_INFO *g_popup_info[];

extern char *tab_order[];

extern char *call_log_columns[];
extern char *contact_list_columns[];
extern char *ignore_list_columns[];
extern char *ignore_cid_list_columns[];
extern char *allow_list_columns[];
extern char *allow_cid_list_columns[];

extern int *call_log_columns_width[];
extern int *contact_list_columns_width[];
extern int *ignore_list_columns_width[];
extern int *ignore_cid_list_columns_width[];
extern int *allow_list_columns_width[];
extern int *allow_cid_list_columns_width[];

#endif
