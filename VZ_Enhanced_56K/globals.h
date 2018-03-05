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

#define MIN_WIDTH			480
#define MIN_HEIGHT			320

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

#define NUM_TABS			3

#define NUM_COLUMNS1		6
#define NUM_COLUMNS2		17
#define NUM_COLUMNS3		3
#define NUM_COLUMNS4		5

#define _wcsicmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiW( a, b ) )
#define _stricmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiA( a, b ) )

#define SAFESTRA( s ) ( s != NULL ? s : "" )
#define SAFESTR2A( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : "" ) )

#define SAFESTRW( s ) ( s != NULL ? s : L"" )
#define SAFESTR2W( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : L"" ) )

#define is_close( a, b ) ( _abs( a - b ) < SNAP_WIDTH )

#define is_digit( c ) ( c - '0' + 0U <= 9U )

#define GET_X_LPARAM( lp )	( ( int )( short )LOWORD( lp ) )
#define GET_Y_LPARAM( lp )	( ( int )( short )HIWORD( lp ) )

struct contactinfo;
struct ignoreinfo;

struct modeminfo
{
	GUID permanent_line_guid;
	wchar_t *line_name;
	DWORD device_id;
};

struct ringtoneinfo
{
	wchar_t *ringtone_path;
	wchar_t *ringtone_file;
};

struct playback_info
{
	ringtoneinfo *rti;
	DWORD device_id;
};

struct callerinfo
{
	char *call_from;
	char *caller_id;

	bool ignored;			// The phone number has been ignored
};

struct displayinfo
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *caller_id;					// Caller ID
			wchar_t *w_time;					// Date and Time
			wchar_t *w_ignore_caller_id;		// Ignore Caller ID
			wchar_t *w_ignore_phone_number;		// Ignore Phone Number
			wchar_t *phone_number;				// Phone Number
		};

		wchar_t *display_values[ 5 ];
	};

	callerinfo ci;

	ULARGE_INTEGER time;

	contactinfo *contact_info;

	HCALL incoming_call;

	unsigned int ignore_cid_match_count;	// Number of ignore cid matches.

	bool ignore_phone_number;		// phone number is in ignore_list

	bool process_incoming;	// false, true = ignore
};

struct CONTACT
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			char *cell_phone_number;	// Cell Phone Number
			char *business_name;		// Company
			char *department;			// Department
			char *email_address;		// Email Address
			char *fax_number;			// Fax Number
			char *first_name;			// First Name
			char *home_phone_number;	// Home Phone Number
			char *designation;			// Job Title
			char *last_name;			// Last Name
			char *nickname;				// Nickname
			char *office_phone_number;	// Office Phone Number
			char *other_phone_number;	// Other Phone Number
			char *category;				// Profession
			char *title;				// Title
			char *web_page;				// Web Page
			char *work_phone_number;	// Work Phone Number
		};

		char *contact_values[ 16 ];
	};
};

struct contactinfo
{
	CONTACT contact;

	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *cell_phone_number;		// Cell Phone Number
			wchar_t *business_name;			// Company
			wchar_t *department;			// Department
			wchar_t *email_address;			// Email Address
			wchar_t *fax_number;			// Fax Number
			wchar_t *first_name;			// First Name
			wchar_t *home_phone_number;		// Home Phone Number
			wchar_t *designation;			// Job Title
			wchar_t *last_name;				// Last Name
			wchar_t *nickname;				// Nickname
			wchar_t *office_phone_number;	// Office Phone Number
			wchar_t *other_phone_number;	// Other Phone Number
			wchar_t *category;				// Profession
			wchar_t *title;					// Title
			wchar_t *web_page;				// Web Page
			wchar_t *work_phone_number;		// Work Phone Number
		};

		wchar_t *contactinfo_values[ 16 ];
	};

	wchar_t *picture_path;			// Local path to picture.
	ringtoneinfo *ringtone_info;	// Local path and file name of the ringtone.
};

struct contactupdateinfo
{
	contactinfo *old_ci;
	contactinfo *new_ci;
	unsigned char action;	// 0 Add, 1 Remove, 2 Add all from contact_list, 3 Update ci.
	bool remove_picture;
};

struct ignoreinfo
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			char *c_phone_number;
			char *c_total_calls;
		};

		char *c_ignoreinfo_values[ 2 ];
	};

	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *phone_number;
			wchar_t *total_calls;
		};

		wchar_t *ignoreinfo_values[ 2 ];
	};

	unsigned int count;		// Number of times the call has been ignored.
	unsigned char state;	// 0 = keep, 1 = remove.
};

struct ignorecidinfo
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			char *c_caller_id;
			char *c_match_case;
			char *c_match_whole_word;
			char *c_total_calls;
		};

		char *c_ignorecidinfo_values[ 4 ];
	};

	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *caller_id;
			wchar_t *w_match_case;
			wchar_t *w_match_whole_word;
			wchar_t *total_calls;
		};

		wchar_t *ignorecidinfo_values[ 4 ];
	};

	unsigned int count;		// Number of times the call has been ignored.
	unsigned char state;	// 0 = keep, 1 = remove.

	bool match_case;
	bool match_whole_word;

	bool active;			// A caller ID value has been matched.
};

struct ignoreupdateinfo
{
	HWND hWnd;
	char *phone_number;		// Phone number to add.
	ignoreinfo *ii;			// Ignore info to update.
	unsigned char action;	// 0 Add, 1 Remove, 2 Add all from ignore_list, 3 Update ii.
};

struct ignorecidupdateinfo
{
	HWND hWnd;
	char *caller_id;		// Caller ID to add.
	ignorecidinfo *icidi;	// Ignore info to update.
	unsigned char action;	// 0 Add, 1 Remove, 2 Add all from ignore_cid_list, 3 Update icidi.
	bool match_case;
	bool match_whole_word;
};

struct sortinfo
{
	HWND hWnd;
	int column;
	unsigned char direction;
};


// These are all variables that are shared among the separate .cpp files.

extern SYSTEMTIME g_compile_time;

// Object handles.
extern HWND g_hWnd_main;				// Handle to our main window.
extern HWND g_hWnd_options;				// Handle to our options window.
extern HWND g_hWnd_columns;
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

extern bool skip_log_draw;				// Prevents WM_DRAWITEM from accessing listview items while we're removing them.
extern bool skip_contact_draw;
extern bool skip_ignore_draw;
extern bool skip_ignore_cid_draw;

extern RANGE *ignore_range_list[ 16 ];

extern dllrbt_tree *modem_list;
extern modeminfo *default_modem;

extern dllrbt_tree *recording_list;
extern ringtoneinfo *default_recording;

extern dllrbt_tree *ringtone_list;
extern ringtoneinfo *default_ringtone;

extern dllrbt_tree *ignore_list;
extern bool ignore_list_changed;

extern dllrbt_tree *ignore_cid_list;
extern bool ignore_cid_list_changed;

extern dllrbt_tree *contact_list;
extern bool contact_list_changed;

extern dllrbt_tree *call_log;
extern bool call_log_changed;

extern unsigned char update_check_state;

extern unsigned char total_tabs;

extern unsigned char total_columns1;
extern unsigned char total_columns2;
extern unsigned char total_columns3;
extern unsigned char total_columns4;

// Saved configuration variables
extern int cfg_pos_x;
extern int cfg_pos_y;
extern int cfg_width;
extern int cfg_height;

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

extern wchar_t *cfg_popup_font_face1;
extern wchar_t *cfg_popup_font_face2;
extern wchar_t *cfg_popup_font_face3;
extern COLORREF cfg_popup_font_color1;
extern COLORREF cfg_popup_font_color2;
extern COLORREF cfg_popup_font_color3;
extern LONG cfg_popup_font_height1;
extern LONG cfg_popup_font_height2;
extern LONG cfg_popup_font_height3;
extern LONG cfg_popup_font_weight1;
extern LONG cfg_popup_font_weight2;
extern LONG cfg_popup_font_weight3;
extern BYTE cfg_popup_font_italic1;
extern BYTE cfg_popup_font_italic2;
extern BYTE cfg_popup_font_italic3;
extern BYTE cfg_popup_font_underline1;
extern BYTE cfg_popup_font_underline2;
extern BYTE cfg_popup_font_underline3;
extern BYTE cfg_popup_font_strikeout1;
extern BYTE cfg_popup_font_strikeout2;
extern BYTE cfg_popup_font_strikeout3;
extern bool cfg_popup_font_shadow1;
extern bool cfg_popup_font_shadow2;
extern bool cfg_popup_font_shadow3;
extern COLORREF cfg_popup_font_shadow_color1;
extern COLORREF cfg_popup_font_shadow_color2;
extern COLORREF cfg_popup_font_shadow_color3;
extern unsigned char cfg_popup_justify_line1;
extern unsigned char cfg_popup_justify_line2;
extern unsigned char cfg_popup_justify_line3;
extern char cfg_popup_line_order1;
extern char cfg_popup_line_order2;
extern char cfg_popup_line_order3;
extern unsigned char cfg_popup_time_format;

extern bool cfg_popup_enable_ringtones;
extern wchar_t *cfg_popup_ringtone;

extern char cfg_tab_order1;
extern char cfg_tab_order2;
extern char cfg_tab_order3;

extern int cfg_column_width1;
extern int cfg_column_width2;
extern int cfg_column_width3;
extern int cfg_column_width4;
extern int cfg_column_width5;
extern int cfg_column_width6;

extern char cfg_column_order1;
extern char cfg_column_order2;
extern char cfg_column_order3;
extern char cfg_column_order4;
extern char cfg_column_order5;
extern char cfg_column_order6;

extern int cfg_column2_width1;
extern int cfg_column2_width2;
extern int cfg_column2_width3;
extern int cfg_column2_width4;
extern int cfg_column2_width5;
extern int cfg_column2_width6;
extern int cfg_column2_width7;
extern int cfg_column2_width8;
extern int cfg_column2_width9;
extern int cfg_column2_width10;
extern int cfg_column2_width11;
extern int cfg_column2_width12;
extern int cfg_column2_width13;
extern int cfg_column2_width14;
extern int cfg_column2_width15;
extern int cfg_column2_width16;
extern int cfg_column2_width17;

extern char cfg_column2_order1;
extern char cfg_column2_order2;
extern char cfg_column2_order3;
extern char cfg_column2_order4;
extern char cfg_column2_order5;
extern char cfg_column2_order6;
extern char cfg_column2_order7;
extern char cfg_column2_order8;
extern char cfg_column2_order9;
extern char cfg_column2_order10;
extern char cfg_column2_order11;
extern char cfg_column2_order12;
extern char cfg_column2_order13;
extern char cfg_column2_order14;
extern char cfg_column2_order15;
extern char cfg_column2_order16;
extern char cfg_column2_order17;

extern int cfg_column3_width1;
extern int cfg_column3_width2;
extern int cfg_column3_width3;

extern char cfg_column3_order1;
extern char cfg_column3_order2;
extern char cfg_column3_order3;

extern int cfg_column4_width1;
extern int cfg_column4_width2;
extern int cfg_column4_width3;
extern int cfg_column4_width4;
extern int cfg_column4_width5;

extern char cfg_column4_order1;
extern char cfg_column4_order2;
extern char cfg_column4_order3;
extern char cfg_column4_order4;
extern char cfg_column4_order5;

extern bool cfg_connection_auto_login;
extern bool cfg_connection_reconnect;
extern unsigned char cfg_connection_retries;

extern unsigned short cfg_connection_timeout;
extern unsigned char cfg_connection_ssl_version;

extern bool cfg_popup_enable_recording;
extern wchar_t *cfg_recording;

extern unsigned char cfg_repeat_recording;

extern unsigned char cfg_drop_call_wait;
extern GUID cfg_modem_guid;

extern bool g_voice_playback_supported;

extern char *tab_order[ NUM_TABS ];

extern char *call_log_columns[ NUM_COLUMNS1 ];
extern char *contact_list_columns[ NUM_COLUMNS2 ];
extern char *ignore_list_columns[ NUM_COLUMNS3 ];
extern char *ignore_cid_list_columns[ NUM_COLUMNS4 ];

extern int *call_log_columns_width[ NUM_COLUMNS1 ];
extern int *contact_list_columns_width[ NUM_COLUMNS2 ];
extern int *ignore_list_columns_width[ NUM_COLUMNS3 ];
extern int *ignore_cid_list_columns_width[ NUM_COLUMNS4 ];

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
	ringtoneinfo *ringtone_info;
	unsigned short popup_time;
	unsigned char popup_gradient_direction;
	bool popup_gradient;
};

struct POPUP_SETTINGS
{
	WINDOW_SETTINGS window_settings;
	HFONT font;
	COLORREF font_color;
	COLORREF font_shadow_color;
	LONG font_height;
	LONG font_weight;
	wchar_t *font_face;
	wchar_t *line_text;
	SHARED_SETTINGS *shared_settings;
	unsigned char popup_justify;
	char popup_line_order;
	BYTE font_italic;
	BYTE font_underline;
	BYTE font_strikeout;
	bool font_shadow;
};

struct LOGIN_SETTINGS
{
	char *username;
	char *password;
};

#endif
