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
#include "utilities.h"
#include "message_log_utilities.h"
#include "ringtone_utilities.h"
#include "menus.h"
#include "string_tables.h"
#include "lite_gdi32.h"
#include "lite_pcre2.h"

#define CID_LIST_COUNT	6

HANDLE worker_semaphore = NULL;			// Blocks shutdown while a worker thread is active.
bool kill_worker_thread_flag = false;	// Allow for a clean shutdown.

CRITICAL_SECTION pe_cs;					// Queues additional worker threads.

bool in_worker_thread = false;			// Flag to indicate that we're in a worker thread.

dllrbt_tree *call_log = NULL;
bool call_log_changed = false;

dllrbt_tree **custom_cid_list = NULL;

int cfg_pos_x = 0;
int cfg_pos_y = 0;
int cfg_width = MIN_WIDTH;
int cfg_height = MIN_HEIGHT;

char cfg_min_max = 0;	// 0 = normal, 1 = minimized, 2 = maximized.

bool cfg_tray_icon = true;
bool cfg_close_to_tray = false;
bool cfg_minimize_to_tray = false;
bool cfg_silent_startup = false;

bool cfg_always_on_top = false;

bool cfg_enable_call_log_history = true;

bool cfg_enable_message_log = false;

bool cfg_check_for_updates = false;

bool cfg_enable_popups = false;
bool cfg_popup_hide_border = false;
bool cfg_popup_show_contact_picture = false;
int cfg_popup_width = 240;
int cfg_popup_height = 106;
unsigned char cfg_popup_position = 3;		// 0 = Top Left, 1 = Top Right, 2 = Bottom Left, 3 = Bottom Right

unsigned short cfg_popup_time = 10;			// Time in seconds.
unsigned char cfg_popup_transparency = 255;	// 255 = opaque
COLORREF cfg_popup_background_color1 = RGB( 255, 255, 255 );

bool cfg_popup_gradient = false;
unsigned char cfg_popup_gradient_direction = 0;	// 0 = Vertical, 1 = Horizontal
COLORREF cfg_popup_background_color2 = RGB( 255, 255, 255 );

unsigned char cfg_popup_time_format = 0;	// 0 = 12 hour, 1 = 24 hour

bool cfg_popup_enable_ringtones = false;
wchar_t *cfg_popup_ringtone = NULL;

POPUP_INFO cfg_popup_info1 = { NULL, RGB( 0x00, 0x00, 0x00 ), RGB( 0x00, 0x00, 0x00 ), 16, FW_NORMAL, FALSE, FALSE, FALSE, 2, LINE_TIME, false };
POPUP_INFO cfg_popup_info2 = { NULL, RGB( 0x00, 0x00, 0x00 ), RGB( 0x00, 0x00, 0x00 ), 16, FW_NORMAL, FALSE, FALSE, FALSE, 0, LINE_CALLER_ID, false };
POPUP_INFO cfg_popup_info3 = { NULL, RGB( 0x00, 0x00, 0x00 ), RGB( 0x00, 0x00, 0x00 ), 16, FW_NORMAL, FALSE, FALSE, FALSE, 0, LINE_PHONE, false };

char cfg_tab_order1 = 2;		// 0 Allow List
char cfg_tab_order2 = 0;		// 1 Call Log
char cfg_tab_order3 = 1;		// 2 Contact List
char cfg_tab_order4 = 3;		// 3 Ignore List

// Allow
int cfg_column_al_width1 = 35;
int cfg_column_al_width2 = 215;
int cfg_column_al_width3 = 120;
int cfg_column_al_width4 = 70;

// Column (1-6) / Virtual position (0-5)
// Set the visible column to the position indicated in the virtual list.
char cfg_column_al_order1 = 0;	// 0 # (always 0)
char cfg_column_al_order2 = 2;	// 1 Last Called
char cfg_column_al_order3 = 3;	// 2 Phone Number
char cfg_column_al_order4 = 1;	// 3 Total Calls

// Allow CID
int cfg_column_acidl_width1 = 35;
int cfg_column_acidl_width2 = 120;
int cfg_column_acidl_width3 = 215;
int cfg_column_acidl_width4 = 75;
int cfg_column_acidl_width5 = 110;
int cfg_column_acidl_width6 = 110;
int cfg_column_acidl_width7 = 70;

char cfg_column_acidl_order1 = 0;	// 0 # (always 0)
char cfg_column_acidl_order2 = 1;	// 1 Caller ID
char cfg_column_acidl_order3 = 3;	// 2 Last Called
char cfg_column_acidl_order4 = 4;	// 3 Match Case
char cfg_column_acidl_order5 = 5;	// 4 Match Whole Word
char cfg_column_acidl_order6 = 6;	// 5 Regular Expression
char cfg_column_acidl_order7 = 2;	// 6 Total Calls

// Call Log
int cfg_column_cll_width1 = 35;
int cfg_column_cll_width2 = 130;
int cfg_column_cll_width3 = 130;
int cfg_column_cll_width4 = 120;
int cfg_column_cll_width5 = 215;
int cfg_column_cll_width6 = 130;
int cfg_column_cll_width7 = 130;
int cfg_column_cll_width8 = 120;

char cfg_column_cll_order1 = 0;		// 0 # (always 0)
char cfg_column_cll_order2 = -1;	// 1 Allowed Caller ID
char cfg_column_cll_order3 = -1;	// 2 Allowed Phone Number
char cfg_column_cll_order4 = 1;		// 3 Caller ID
char cfg_column_cll_order5 = 3;		// 4 Date and Time
char cfg_column_cll_order6 = -1;	// 5 Ignored Caller ID
char cfg_column_cll_order7 = -1;	// 6 Ignored Phone Number
char cfg_column_cll_order8 = 2;		// 7 Phone Number

// Contact
int cfg_column_cl_width1 = 35;
int cfg_column_cl_width2 = 120;
int cfg_column_cl_width3 = 100;
int cfg_column_cl_width4 = 100;
int cfg_column_cl_width5 = 150;
int cfg_column_cl_width6 = 120;
int cfg_column_cl_width7 = 100;
int cfg_column_cl_width8 = 120;
int cfg_column_cl_width9 = 100;
int cfg_column_cl_width10 = 100;
int cfg_column_cl_width11 = 100;
int cfg_column_cl_width12 = 120;
int cfg_column_cl_width13 = 120;
int cfg_column_cl_width14 = 100;
int cfg_column_cl_width15 = 70;
int cfg_column_cl_width16 = 300;
int cfg_column_cl_width17 = 120;

char cfg_column_cl_order1 = 0;		// 0 # (always 0)
char cfg_column_cl_order2 = 14;		// 1 Cell Phone
char cfg_column_cl_order3 = 6;		// 2 Company
char cfg_column_cl_order4 = 9;		// 3 Department
char cfg_column_cl_order5 = 10;		// 4 Email Address
char cfg_column_cl_order6 = 7;		// 5 Fax Number
char cfg_column_cl_order7 = 1;		// 6 First Name
char cfg_column_cl_order8 = 13;		// 7 Home Phone
char cfg_column_cl_order9 = 8;		// 8 Job Title
char cfg_column_cl_order10 = 2;		// 9 Last Name
char cfg_column_cl_order11 = 16;	// 10 Nickname
char cfg_column_cl_order12 = 3;		// 11 Office Phone
char cfg_column_cl_order13 = 11;	// 12 Other Phone
char cfg_column_cl_order14 = 12;	// 13 Profession
char cfg_column_cl_order15 = 5;		// 14 Title
char cfg_column_cl_order16 = 4;		// 15 Web Page
char cfg_column_cl_order17 = 15;	// 16 Work Phone

// Ignore
int cfg_column_il_width1 = 35;
int cfg_column_il_width2 = 215;
int cfg_column_il_width3 = 120;
int cfg_column_il_width4 = 70;

char cfg_column_il_order1 = 0;	// 0 # (always 0)
char cfg_column_il_order2 = 2;	// 1 Last Called
char cfg_column_il_order3 = 3;	// 2 Phone Number
char cfg_column_il_order4 = 1;	// 3 Total Calls

// Ignore CID
int cfg_column_icidl_width1 = 35;
int cfg_column_icidl_width2 = 120;
int cfg_column_icidl_width3 = 215;
int cfg_column_icidl_width4 = 75;
int cfg_column_icidl_width5 = 110;
int cfg_column_icidl_width6 = 110;
int cfg_column_icidl_width7 = 70;

char cfg_column_icidl_order1 = 0;	// 0 # (always 0)
char cfg_column_icidl_order2 = 1;	// 1 Caller ID
char cfg_column_icidl_order3 = 3;	// 2 Last Called
char cfg_column_icidl_order4 = 4;	// 3 Match Case
char cfg_column_icidl_order5 = 5;	// 4 Match Whole Word
char cfg_column_icidl_order6 = 6;	// 5 Regular Expression
char cfg_column_icidl_order7 = 2;	// 6 Total Calls

bool cfg_popup_enable_recording = false;
wchar_t *cfg_recording = NULL;

unsigned char cfg_repeat_recording = 1;

unsigned char cfg_drop_call_wait = 5;	// Seconds.
GUID cfg_modem_guid;

POPUP_INFO *g_popup_info[] = { &cfg_popup_info1, &cfg_popup_info2, &cfg_popup_info3 };

char *tab_order[] = { &cfg_tab_order1, &cfg_tab_order2, &cfg_tab_order3, &cfg_tab_order4 };

char *allow_list_columns[] =		{ &cfg_column_al_order1, &cfg_column_al_order2, &cfg_column_al_order3, &cfg_column_al_order4 };
char *allow_cid_list_columns[] =	{ &cfg_column_acidl_order1, &cfg_column_acidl_order2, &cfg_column_acidl_order3, &cfg_column_acidl_order4, &cfg_column_acidl_order5, &cfg_column_acidl_order6, &cfg_column_acidl_order7 };

char *call_log_columns[] =			{ &cfg_column_cll_order1, &cfg_column_cll_order2, &cfg_column_cll_order3, &cfg_column_cll_order4, &cfg_column_cll_order5, &cfg_column_cll_order6, &cfg_column_cll_order7, &cfg_column_cll_order8 };

char *contact_list_columns[] =		{ &cfg_column_cl_order1, &cfg_column_cl_order2, &cfg_column_cl_order3, &cfg_column_cl_order4, &cfg_column_cl_order5, &cfg_column_cl_order6, &cfg_column_cl_order7, &cfg_column_cl_order8, &cfg_column_cl_order9,
									  &cfg_column_cl_order10, &cfg_column_cl_order11, &cfg_column_cl_order12, &cfg_column_cl_order13, &cfg_column_cl_order14, &cfg_column_cl_order15, &cfg_column_cl_order16, &cfg_column_cl_order17 };

char *ignore_list_columns[] =		{ &cfg_column_il_order1, &cfg_column_il_order2, &cfg_column_il_order3, &cfg_column_il_order4 };
char *ignore_cid_list_columns[] =	{ &cfg_column_icidl_order1, &cfg_column_icidl_order2, &cfg_column_icidl_order3, &cfg_column_icidl_order4, &cfg_column_icidl_order5, &cfg_column_icidl_order6, &cfg_column_icidl_order7 };

//

int *allow_list_columns_width[] =		{ &cfg_column_al_width1, &cfg_column_al_width2, &cfg_column_al_width3, &cfg_column_al_width4 };
int *allow_cid_list_columns_width[] =	{ &cfg_column_acidl_width1, &cfg_column_acidl_width2, &cfg_column_acidl_width3, &cfg_column_acidl_width4, &cfg_column_acidl_width5, &cfg_column_acidl_width6, &cfg_column_acidl_width7 };

int *call_log_columns_width[] =			{ &cfg_column_cll_width1, &cfg_column_cll_width2, &cfg_column_cll_width3, &cfg_column_cll_width4, &cfg_column_cll_width5, &cfg_column_cll_width6, &cfg_column_cll_width7, &cfg_column_cll_width8 };

int *contact_list_columns_width[] =		{ &cfg_column_cl_width1, &cfg_column_cl_width2, &cfg_column_cl_width3, &cfg_column_cl_width4, &cfg_column_cl_width5, &cfg_column_cl_width6, &cfg_column_cl_width7, &cfg_column_cl_width8, &cfg_column_cl_width9,
										  &cfg_column_cl_width10, &cfg_column_cl_width11, &cfg_column_cl_width12, &cfg_column_cl_width13, &cfg_column_cl_width14, &cfg_column_cl_width15, &cfg_column_cl_width16, &cfg_column_cl_width17 };

int *ignore_list_columns_width[] =		{ &cfg_column_il_width1, &cfg_column_il_width2, &cfg_column_il_width3, &cfg_column_il_width4 };
int *ignore_cid_list_columns_width[] =	{ &cfg_column_icidl_width1, &cfg_column_icidl_width2, &cfg_column_icidl_width3, &cfg_column_icidl_width4, &cfg_column_icidl_width5, &cfg_column_icidl_width6, &cfg_column_icidl_width7 };

#define ROTATE_LEFT( x, n ) ( ( ( x ) << ( n ) ) | ( ( x ) >> ( 8 - ( n ) ) ) )
#define ROTATE_RIGHT( x, n ) ( ( ( x ) >> ( n ) ) | ( ( x ) << ( 8 - ( n ) ) ) )

void encode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = 0; i < buffer_length; ++i )
	{
		*buffer ^= ( unsigned char )buffer_length;
		*buffer = ( *buffer + offset ) % 256;
		*buffer = ROTATE_LEFT( ( unsigned char )*buffer, offset % 8 );

		buffer++;
		--offset;
	}
}

void decode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = buffer_length; i > 0; --i )
	{
		*buffer = ROTATE_RIGHT( ( unsigned char )*buffer, offset % 8 );
		*buffer = ( *buffer - offset ) % 256;
		*buffer ^= ( unsigned char )buffer_length;

		buffer++;
		--offset;
	}
}

// Must use GlobalFree on this.
char *GlobalStrDupA( const char *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenA( _Str ) + sizeof( char );

	char *ret = ( char * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

// Must use GlobalFree on this.
wchar_t *GlobalStrDupW( const wchar_t *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenW( _Str ) * sizeof( wchar_t ) + sizeof( wchar_t );

	wchar_t *ret = ( wchar_t * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

int dllrbt_compare_ptr( void *a, void *b )
{
	if ( a == b )
	{
		return 0;
	}
	else if ( a > b )
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

int dllrbt_compare_w( void *a, void *b )
{
	return lstrcmpW( ( wchar_t * )a, ( wchar_t * )b );
}

int dllrbt_compare_guid( void *a, void *b )
{
	return _memcmp( a, b, sizeof( GUID ) );
}

int dllrbt_icid_compare( void *a, void *b )
{
	allow_ignore_cid_info *a1 = ( allow_ignore_cid_info * )a;
	allow_ignore_cid_info *b1 = ( allow_ignore_cid_info * )b;

	int ret = lstrcmpW( a1->caller_id, b1->caller_id );

	// See if the strings match.
	if ( ret == 0 )
	{
		// If they do, then check if the booleans differ.
		if ( ( a1->match_flag & 0x02 ) && !( b1->match_flag & 0x02 ) )
		{
			ret = 1;
		}
		else if ( !( a1->match_flag & 0x02 ) && ( b1->match_flag & 0x02 ) )
		{
			ret = -1;
		}
		else
		{
			if ( ( a1->match_flag & 0x01 ) && !( b1->match_flag & 0x01 ) )
			{
				ret = 1;
			}
			else if ( !( a1->match_flag & 0x01 ) && ( b1->match_flag & 0x01 ) )
			{
				ret = -1;
			}
			else
			{
				if ( ( a1->match_flag & 0x04 ) && !( b1->match_flag & 0x04 ) )
				{
					ret = 1;
				}
				else if ( !( a1->match_flag & 0x04 ) && ( b1->match_flag & 0x04 ) )
				{
					ret = -1;
				}
			}
		}
	}

	return ret;
}

char is_num_w( const wchar_t *str )
{
	if ( str == NULL )
	{
		return -1;
	}

	wchar_t *s = ( wchar_t * )str;

	char ret = 0;	// 0 = regular number, 1 = number with wildcard(s), -1 = not a number.

	if ( *s != NULL && *s == L'+' )
	{
		*s++;
	}

	while ( *s != NULL )
	{
		if ( *s == L'*' )
		{
			ret = 1;
		}
		else if ( !is_digit_w( *s ) )
		{
			return -1;
		}

		*s++;
	}

	return ret;
}

wchar_t *GetMonth( unsigned short month )
{
	if ( month > 12 || month < 1 )
	{
		return L"";
	}

	return month_string_table[ month - 1 ];
}

wchar_t *GetDay( unsigned short day )
{
	if ( day > 6 )
	{
		return L"";
	}

	return day_string_table[ day ];
}

void UnixTimeToSystemTime( DWORD t, SYSTEMTIME *st )
{
	FILETIME ft;
	LARGE_INTEGER li;
#ifdef _WIN64
	li.QuadPart = t * 10000000LL;
#else
	// Multipy our time and store the 64bit integer. It's located in edx:eax
	__asm
	{
		mov eax, t;
		mov ecx, 10000000;
		mul ecx;

		mov li.HighPart, edx;
		mov li.LowPart, eax;
	}
#endif
	li.QuadPart += 116444736000000000;

	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;

	FileTimeToSystemTime( &ft, st );
}

void GetCurrentCounterTime( ULARGE_INTEGER *li )
{
	if ( li != NULL )
	{
		SYSTEMTIME SystemTime;
		FILETIME FileTime;

		GetLocalTime( &SystemTime );
		SystemTimeToFileTime( &SystemTime, &FileTime );

		li->LowPart = FileTime.dwLowDateTime;
		li->HighPart = FileTime.dwHighDateTime;
	}
}

wchar_t *FormatTimestamp( ULARGE_INTEGER timestamp )
{
	SYSTEMTIME st;
	FILETIME ft;
	ft.dwLowDateTime = timestamp.LowPart;
	ft.dwHighDateTime = timestamp.HighPart;

	FileTimeToSystemTime( &ft, &st );

	#ifndef NTDLL_USE_STATIC_LIB
		//buffer_length = 64;	// Should be enough to hold most translated values.
		int buffer_length = __snwprintf( NULL, 0, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
	#else
		int buffer_length = _scwprintf( L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
	#endif

	wchar_t *formatted_timestamp = NULL;

	if ( buffer_length > 0 )
	{
		formatted_timestamp = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_length );

		__snwprintf( formatted_timestamp, buffer_length, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) );
	}

	return formatted_timestamp;
}

DWORD GetRemainingDropCallWaitTime( ULARGE_INTEGER start, ULARGE_INTEGER stop )
{
	stop.QuadPart -= start.QuadPart;
#ifdef _WIN64
	start.QuadPart = cfg_drop_call_wait * FILETIME_TICKS_PER_SECOND;
#else
	// Multiply the drop call wait time by the ticks/second.
	__asm
	{
		xor		eax, eax;						;// Zero out the register.
		mov		al, cfg_drop_call_wait			;// Load the value to multiply
		mov		ebx, FILETIME_TICKS_PER_SECOND	;// Load the value to multiply
		mul		ebx								;// Multiply the ints.
		mov		start.LowPart, eax				;// Store the low result.
		mov		start.HighPart, edx				;// Store the carry result.
	}
#endif
	// Sleep a little bit more if the audio playback was shorter than our drop call wait time.
	if ( stop.QuadPart < start.QuadPart )
	{
		start.QuadPart -= stop.QuadPart;
#ifdef _WIN64
		start.QuadPart /= FILETIME_TICKS_PER_MILLISECOND;
#else
		// Divide the 64bit value.
		__asm
		{
			xor edx, edx;				//; Zero out the register so we don't divide a full 64bit value.
			mov eax, start.HighPart;	//; We'll divide the high order bits first.
			mov ecx, FILETIME_TICKS_PER_MILLISECOND;
			div ecx;
			mov start.HighPart, eax;	//; Store the high order quotient.
			mov eax, start.LowPart;		//; Now we'll divide the low order bits.
			div ecx;
			mov start.LowPart, eax;		//; Store the low order quotient.
			//; Any remainder will be stored in edx. We're not interested in it though.
		}
#endif
		return ( DWORD )start.LowPart;
	}
	else
	{
		return 0;
	}
}

void free_displayinfo( display_info **di )
{
	// Free the strings in our info structure.
	GlobalFree( ( *di )->phone_number );
	GlobalFree( ( *di )->caller_id );
	GlobalFree( ( *di )->custom_caller_id );
	GlobalFree( ( *di )->w_phone_number );
	GlobalFree( ( *di )->w_time );
	// Then free the display_info structure.
	GlobalFree( *di );
	*di = NULL;
}

void free_contactinfo( contact_info **ci )
{
	char i;

	// Free the strings in our info structure.
	for ( i = 0; i < 16; ++i )
	{
		GlobalFree( ( *ci )->w_contact_info_values[ i ] );
	}

	GlobalFree( ( *ci )->picture_path );

	// Do not free ( *ci )->rti or its values.

	// Then free the contact_info structure.
	GlobalFree( *ci );
	*ci = NULL;
}

void free_allowignoreinfo( allow_ignore_info **aii )
{
	GlobalFree( ( *aii )->w_last_called );
	GlobalFree( ( *aii )->w_phone_number );
	GlobalFree( ( *aii )->phone_number );
	GlobalFree( *aii );
	*aii = NULL;
}

void free_allowignorecidinfo( allow_ignore_cid_info **aicidi )
{
	GlobalFree( ( *aicidi )->w_last_called );
	GlobalFree( ( *aicidi )->caller_id );
	GlobalFree( *aicidi );
	*aicidi = NULL;
}

void OffsetVirtualIndices( int *arr, char *column_arr[], unsigned char num_columns, unsigned char total_columns )
{
	for ( unsigned char i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] == -1 )	// See which columns are disabled.
		{
			for ( unsigned char j = 0; j < total_columns; ++j )
			{
				if ( arr[ j ] >= i )	// Increment each virtual column that comes after the disabled column.
				{
					arr[ j ]++;
				}
			}
		}
	}
}

int GetVirtualIndexFromColumnIndex( int column_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( count == column_index )
			{
				return i;
			}
			else
			{
				++count;
			}
		}
	}

	return -1;
}

int GetColumnIndexFromVirtualIndex( int virtual_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 && *column_arr[ i ] == virtual_index )
		{
			return count;
		}
		else if ( *column_arr[ i ] != -1 )
		{
			++count;
		}
	}

	return -1;
}

void UpdateColumnOrders()
{
	int arr[ 17 ];
	int offset = 0;

	_SendMessageW( g_hWnd_allow_list, LVM_GETCOLUMNORDERARRAY, g_total_columns1, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS1; ++i )
	{
		if ( *allow_list_columns[ i ] != -1 )
		{
			*allow_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_allow_cid_list, LVM_GETCOLUMNORDERARRAY, g_total_columns2, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS2; ++i )
	{
		if ( *allow_cid_list_columns[ i ] != -1 )
		{
			*allow_cid_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_call_log, LVM_GETCOLUMNORDERARRAY, g_total_columns3, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS3; ++i )
	{
		if ( *call_log_columns[ i ] != -1 )
		{
			*call_log_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_contact_list, LVM_GETCOLUMNORDERARRAY, g_total_columns4, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS4; ++i )
	{
		if ( *contact_list_columns[ i ] != -1 )
		{
			*contact_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_ignore_list, LVM_GETCOLUMNORDERARRAY, g_total_columns5, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS5; ++i )
	{
		if ( *ignore_list_columns[ i ] != -1 )
		{
			*ignore_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_ignore_cid_list, LVM_GETCOLUMNORDERARRAY, g_total_columns6, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS6; ++i )
	{
		if ( *ignore_cid_list_columns[ i ] != -1 )
		{
			*ignore_cid_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}
}

void SetDefaultColumnOrder( unsigned char list )
{
	switch ( list )
	{
		case 0:
		{
			cfg_column_al_order1 = 0;		// 0 # (always 0)
			cfg_column_al_order2 = 2;		// 1 Last Called
			cfg_column_al_order3 = 3;		// 2 Phone Number
			cfg_column_al_order4 = 1;		// 3 Total Calls
		}
		break;

		case 1:
		{
			cfg_column_acidl_order1 = 0;	// 0 # (always 0)
			cfg_column_acidl_order2 = 1;	// 1 Caller ID
			cfg_column_acidl_order3 = 3;	// 2 Last Called
			cfg_column_acidl_order4 = 4;	// 3 Match Case
			cfg_column_acidl_order5 = 5;	// 4 Match Whole Word
			cfg_column_acidl_order6 = 6;	// 5 Regular Expression
			cfg_column_acidl_order7 = 2;	// 6 Total Calls
		}
		break;

		case 2:
		{
			cfg_column_cll_order1 = 0;		// # (always 0)
			cfg_column_cll_order2 = -1;		// Allowed Caller ID
			cfg_column_cll_order3 = -1;		// Allowed Phone Number
			cfg_column_cll_order4 = 1;		// Caller ID
			cfg_column_cll_order5 = 3;		// Date and Time
			cfg_column_cll_order6 = -1;		// Ignored Caller ID
			cfg_column_cll_order7 = -1;		// Ignored Phone Number
			cfg_column_cll_order8 = 2;		// Phone Number
		}
		break;

		case 3:
		{
			cfg_column_cl_order1 = 0;		// 0 # (always 0)
			cfg_column_cl_order2 = 14;		// 1 Cell Phone
			cfg_column_cl_order3 = 6;		// 2 Company
			cfg_column_cl_order4 = 9;		// 3 Department
			cfg_column_cl_order5 = 10;		// 4 Email Address
			cfg_column_cl_order6 = 7;		// 5 Fax Number
			cfg_column_cl_order7 = 1;		// 6 First Name
			cfg_column_cl_order8 = 13;		// 7 Home Phone
			cfg_column_cl_order9 = 8;		// 8 Job Title
			cfg_column_cl_order10 = 2;		// 9 Last Name
			cfg_column_cl_order11 = 16;		// 10 Nickname
			cfg_column_cl_order12 = 3;		// 11 Office Phone
			cfg_column_cl_order13 = 11;		// 12 Other Phone
			cfg_column_cl_order14 = 12;		// 13 Profession
			cfg_column_cl_order15 = 5;		// 14 Title
			cfg_column_cl_order16 = 4;		// 15 Web Page
			cfg_column_cl_order17 = 15;		// 16 Work Phone
		}
		break;

		case 4:
		{
			cfg_column_il_order1 = 0;		// 0 # (always 0)
			cfg_column_il_order2 = 2;		// 1 Last Called
			cfg_column_il_order3 = 3;		// 2 Phone Number
			cfg_column_il_order4 = 1;		// 3 Total Calls
		}
		break;

		case 5:
		{
			cfg_column_icidl_order1 = 0;	// 0 # (always 0)
			cfg_column_icidl_order2 = 1;	// 1 Caller ID
			cfg_column_icidl_order3 = 3;	// 2 Last Called
			cfg_column_icidl_order4 = 4;	// 3 Match Case
			cfg_column_icidl_order5 = 5;	// 4 Match Whole Word
			cfg_column_icidl_order6 = 6;	// 5 Regular Expression
			cfg_column_icidl_order7 = 2;	// 6 Total Calls
		}
		break;
	}
}

void CheckColumnOrders( unsigned char list, char *column_arr[], unsigned char num_columns )
{
	// Make sure the first column is always 0 or -1.
	if ( *column_arr[ 0 ] > 0 || *column_arr[ 0 ] < -1 )
	{
		SetDefaultColumnOrder( list );
		return;
	}

	// Look for duplicates, or values that are out of range.
	unsigned char *is_set = ( unsigned char * )GlobalAlloc( GPTR, sizeof( unsigned char ) * num_columns );

	// Check ever other column.
	for ( int i = 1; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( *column_arr[ i ] < num_columns )
			{
				if ( is_set[ *column_arr[ i ] ] == 0 )
				{
					is_set[ *column_arr[ i ] ] = 1;
				}
				else	// Revert duplicate values.
				{
					SetDefaultColumnOrder( list );

					break;
				}
			}
			else	// Revert out of range values.
			{
				SetDefaultColumnOrder( list );

				break;
			}
		}
	}

	GlobalFree( is_set );
}

void CheckColumnWidths()
{
	if ( cfg_column_al_width1 < 0 || cfg_column_al_width1 > 2560 ) { cfg_column_al_width1 = 35; }
	if ( cfg_column_al_width2 < 0 || cfg_column_al_width2 > 2560 ) { cfg_column_al_width2 = 215; }
	if ( cfg_column_al_width3 < 0 || cfg_column_al_width3 > 2560 ) { cfg_column_al_width3 = 120; }
	if ( cfg_column_al_width4 < 0 || cfg_column_al_width4 > 2560 ) { cfg_column_al_width4 = 70; }

	if ( cfg_column_acidl_width1 < 0 || cfg_column_acidl_width1 > 2560 ) { cfg_column_acidl_width1 = 35; }
	if ( cfg_column_acidl_width2 < 0 || cfg_column_acidl_width2 > 2560 ) { cfg_column_acidl_width2 = 120; }
	if ( cfg_column_acidl_width3 < 0 || cfg_column_acidl_width3 > 2560 ) { cfg_column_acidl_width3 = 215; }
	if ( cfg_column_acidl_width4 < 0 || cfg_column_acidl_width4 > 2560 ) { cfg_column_acidl_width4 = 75; }
	if ( cfg_column_acidl_width5 < 0 || cfg_column_acidl_width5 > 2560 ) { cfg_column_acidl_width5 = 110; }
	if ( cfg_column_acidl_width6 < 0 || cfg_column_acidl_width6 > 2560 ) { cfg_column_acidl_width6 = 110; }
	if ( cfg_column_acidl_width7 < 0 || cfg_column_acidl_width7 > 2560 ) { cfg_column_acidl_width7 = 70; }

	if ( cfg_column_cll_width1 < 0 || cfg_column_cll_width1 > 2560 ) { cfg_column_cll_width1 = 35; }
	if ( cfg_column_cll_width2 < 0 || cfg_column_cll_width2 > 2560 ) { cfg_column_cll_width2 = 130; }
	if ( cfg_column_cll_width3 < 0 || cfg_column_cll_width3 > 2560 ) { cfg_column_cll_width3 = 130; }
	if ( cfg_column_cll_width4 < 0 || cfg_column_cll_width4 > 2560 ) { cfg_column_cll_width4 = 120; }
	if ( cfg_column_cll_width5 < 0 || cfg_column_cll_width5 > 2560 ) { cfg_column_cll_width5 = 215; }
	if ( cfg_column_cll_width6 < 0 || cfg_column_cll_width6 > 2560 ) { cfg_column_cll_width6 = 130; }
	if ( cfg_column_cll_width7 < 0 || cfg_column_cll_width7 > 2560 ) { cfg_column_cll_width7 = 130; }
	if ( cfg_column_cll_width8 < 0 || cfg_column_cll_width8 > 2560 ) { cfg_column_cll_width8 = 120; }

	if ( cfg_column_cl_width1 < 0 || cfg_column_cl_width1 > 2560 ) { cfg_column_cl_width1 = 35; }
	if ( cfg_column_cl_width2 < 0 || cfg_column_cl_width2 > 2560 ) { cfg_column_cl_width2 = 120; }
	if ( cfg_column_cl_width3 < 0 || cfg_column_cl_width3 > 2560 ) { cfg_column_cl_width3 = 100; }
	if ( cfg_column_cl_width4 < 0 || cfg_column_cl_width4 > 2560 ) { cfg_column_cl_width4 = 100; }
	if ( cfg_column_cl_width5 < 0 || cfg_column_cl_width5 > 2560 ) { cfg_column_cl_width5 = 150; }
	if ( cfg_column_cl_width6 < 0 || cfg_column_cl_width6 > 2560 ) { cfg_column_cl_width6 = 120; }
	if ( cfg_column_cl_width7 < 0 || cfg_column_cl_width7 > 2560 ) { cfg_column_cl_width7 = 100; }
	if ( cfg_column_cl_width8 < 0 || cfg_column_cl_width8 > 2560 ) { cfg_column_cl_width8 = 120; }
	if ( cfg_column_cl_width9 < 0 || cfg_column_cl_width9 > 2560 ) { cfg_column_cl_width9 = 100; }
	if ( cfg_column_cl_width10 < 0 || cfg_column_cl_width10 > 2560 ) { cfg_column_cl_width10 = 100; }
	if ( cfg_column_cl_width11 < 0 || cfg_column_cl_width11 > 2560 ) { cfg_column_cl_width11 = 100; }
	if ( cfg_column_cl_width12 < 0 || cfg_column_cl_width12 > 2560 ) { cfg_column_cl_width12 = 120; }
	if ( cfg_column_cl_width13 < 0 || cfg_column_cl_width13 > 2560 ) { cfg_column_cl_width13 = 120; }
	if ( cfg_column_cl_width14 < 0 || cfg_column_cl_width14 > 2560 ) { cfg_column_cl_width14 = 100; }
	if ( cfg_column_cl_width15 < 0 || cfg_column_cl_width15 > 2560 ) { cfg_column_cl_width15 = 70; }
	if ( cfg_column_cl_width16 < 0 || cfg_column_cl_width16 > 2560 ) { cfg_column_cl_width16 = 300; }
	if ( cfg_column_cl_width17 < 0 || cfg_column_cl_width17 > 2560 ) { cfg_column_cl_width17 = 120; }

	if ( cfg_column_il_width1 < 0 || cfg_column_il_width1 > 2560 ) { cfg_column_il_width1 = 35; }
	if ( cfg_column_il_width2 < 0 || cfg_column_il_width2 > 2560 ) { cfg_column_il_width2 = 215; }
	if ( cfg_column_il_width3 < 0 || cfg_column_il_width3 > 2560 ) { cfg_column_il_width3 = 120; }
	if ( cfg_column_il_width4 < 0 || cfg_column_il_width4 > 2560 ) { cfg_column_il_width4 = 70; }

	if ( cfg_column_icidl_width1 < 0 || cfg_column_icidl_width1 > 2560 ) { cfg_column_icidl_width1 = 35; }
	if ( cfg_column_icidl_width2 < 0 || cfg_column_icidl_width2 > 2560 ) { cfg_column_icidl_width2 = 120; }
	if ( cfg_column_icidl_width3 < 0 || cfg_column_icidl_width3 > 2560 ) { cfg_column_icidl_width3 = 215; }
	if ( cfg_column_icidl_width4 < 0 || cfg_column_icidl_width4 > 2560 ) { cfg_column_icidl_width4 = 75; }
	if ( cfg_column_icidl_width5 < 0 || cfg_column_icidl_width5 > 2560 ) { cfg_column_icidl_width5 = 110; }
	if ( cfg_column_icidl_width6 < 0 || cfg_column_icidl_width6 > 2560 ) { cfg_column_icidl_width6 = 110; }
	if ( cfg_column_icidl_width7 < 0 || cfg_column_icidl_width7 > 2560 ) { cfg_column_icidl_width7 = 70; }
}

wchar_t *GetFileNameW( wchar_t *path )
{
	if ( path == NULL )
	{
		return NULL;
	}

	unsigned short length = lstrlenW( path );

	while ( length != 0 && path[ --length ] != L'\\' );

	if ( path[ length ] == L'\\' )
	{
		++length;
	}
	return path + length;
}

HWND GetCurrentListView()
{
	HWND hWnd = NULL;

	int index = ( int )_SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );	// Get the selected tab
	if ( index != -1 )
	{
		TCITEM tie;
		_memzero( &tie, sizeof( TCITEM ) );
		tie.mask = TCIF_PARAM; // Get the lparam value
		_SendMessageW( g_hWnd_tab, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information

		if ( ( HWND )tie.lParam == g_hWnd_allow_tab || ( HWND )tie.lParam == g_hWnd_ignore_tab )
		{
			index = ( int )_SendMessageW( ( HWND )tie.lParam, TCM_GETCURSEL, 0, 0 );	// Get the selected tab
			if ( index != -1 )
			{
				_SendMessageW( ( HWND )tie.lParam, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
			}
		}

		hWnd = ( HWND )tie.lParam;
	}

	return hWnd;
}

wchar_t *GetSelectedColumnPhoneNumber( HWND hWnd, unsigned int column_id )
{
	wchar_t *phone_number = NULL;

	if ( hWnd != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		if ( column_id == MENU_PHONE_NUMBER_COL17 )
		{
			lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

				phone_number = ( ( display_info * )lvi.lParam )->phone_number;
			}
		}
		else if ( column_id == MENU_PHONE_NUMBER_COL21 ||
				  column_id == MENU_PHONE_NUMBER_COL25 ||
				  column_id == MENU_PHONE_NUMBER_COL27 ||
				  column_id == MENU_PHONE_NUMBER_COL211 ||
				  column_id == MENU_PHONE_NUMBER_COL212 || 
				  column_id == MENU_PHONE_NUMBER_COL216 )
		{
			lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

				switch ( column_id )
				{
					case MENU_PHONE_NUMBER_COL21: { phone_number = ( ( contact_info * )lvi.lParam )->cell_phone_number; } break;
					case MENU_PHONE_NUMBER_COL25: { phone_number = ( ( contact_info * )lvi.lParam )->fax_number; } break;
					case MENU_PHONE_NUMBER_COL27: { phone_number = ( ( contact_info * )lvi.lParam )->home_phone_number; } break;
					case MENU_PHONE_NUMBER_COL211: { phone_number = ( ( contact_info * )lvi.lParam )->office_phone_number; } break;
					case MENU_PHONE_NUMBER_COL212: { phone_number = ( ( contact_info * )lvi.lParam )->other_phone_number; } break;
					case MENU_PHONE_NUMBER_COL216: { phone_number = ( ( contact_info * )lvi.lParam )->work_phone_number; } break;
				}
			}
		}
		else if ( column_id == MENU_PHONE_NUMBER_COL32 )
		{
			lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

				phone_number = ( ( allow_ignore_info * )lvi.lParam )->phone_number;
			}
		}
	}

	return phone_number;
}

void CreatePopup( display_info *di )
{
	RECT wa;
	_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );	

	int left = 0;
	int top = 0;

	if ( cfg_popup_position == 0 )		// Top Left
	{
		left = wa.left;
		top = wa.top;
	}
	else if ( cfg_popup_position == 1 )	// Top Right
	{
		left = wa.right - cfg_popup_width;
		top = wa.top;
	}
	else if ( cfg_popup_position == 2 )	// Bottom Left
	{
		left = wa.left;
		top = wa.bottom - cfg_popup_height;
	}
	else					// Bottom Right
	{
		left = wa.right - cfg_popup_width;
		top = wa.bottom - cfg_popup_height;
	}

	HWND hWnd_popup = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"popup", NULL, WS_CLIPCHILDREN | WS_POPUP | ( cfg_popup_hide_border ? NULL : WS_THICKFRAME ), left, top, cfg_popup_width, cfg_popup_height, g_hWnd_main, NULL, NULL, NULL );

	_SetWindowLongPtrW( hWnd_popup, GWL_EXSTYLE, _GetWindowLongPtrW( hWnd_popup, GWL_EXSTYLE ) | WS_EX_LAYERED );
	_SetLayeredWindowAttributes( hWnd_popup, 0, cfg_popup_transparency, LWA_ALPHA );

	// ALL OF THIS IS FREED IN WM_DESTROY OF THE POPUP WINDOW.

	wchar_t *phone_line = GlobalStrDupW( di->w_phone_number );
	wchar_t *caller_id_line = GlobalStrDupW( di->caller_id );
	wchar_t *time_line = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 16 );

	SYSTEMTIME st;
	FILETIME ft;
	ft.dwLowDateTime = di->time.LowPart;
	ft.dwHighDateTime = di->time.HighPart;
	
	FileTimeToSystemTime( &ft, &st );
	if ( cfg_popup_time_format == 0 )	// 12 hour
	{
		__snwprintf( time_line, 16, L"%d:%02d %s", ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, ( st.wHour >= 12 ? L"PM" : L"AM" ) );
	}
	else	// 24 hour
	{
		__snwprintf( time_line, 16, L"%s%d:%02d", ( st.wHour > 9 ? L"" : L"0" ), st.wHour, st.wMinute );
	}

	SHARED_SETTINGS *shared_settings = ( SHARED_SETTINGS * )GlobalAlloc( GPTR, sizeof( SHARED_SETTINGS ) );
	shared_settings->popup_gradient = cfg_popup_gradient;
	shared_settings->popup_gradient_direction = cfg_popup_gradient_direction;
	shared_settings->popup_background_color1 = cfg_popup_background_color1;
	shared_settings->popup_background_color2 = cfg_popup_background_color2;
	shared_settings->popup_time = cfg_popup_time;

	if ( cfg_popup_show_contact_picture && di->ci != NULL )
	{
		shared_settings->contact_picture_info.picture_path = di->ci->picture_path;	// We don't want to free this.
	}

	// Use the default sound if the contact doesn't have a custom ringtone set.
	if ( cfg_popup_enable_ringtones )
	{
		if ( di->ci != NULL && di->ci->rti != NULL )
		{
			shared_settings->rti = di->ci->rti;
		}
		else
		{
			shared_settings->rti = default_ringtone;
		}
	}

	DoublyLinkedList *t_ll = NULL;

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
		ls->popup_info.font_face = NULL;
		ls->line_text = ( _abs( g_popup_info[ i ]->line_order ) == LINE_TIME ? time_line : ( _abs( g_popup_info[ i ]->line_order ) == LINE_CALLER_ID ? caller_id_line : phone_line ) );

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

		if ( t_ll == NULL )
		{
			t_ll = DLL_CreateNode( ( void * )ls );
		}
		else
		{
			DoublyLinkedList *ll = DLL_CreateNode( ( void * )ls );
			DLL_AddNode( &t_ll, ll, -1 );
		}
	}

	_SetWindowLongPtrW( hWnd_popup, 0, ( LONG_PTR )t_ll );

	_SendMessageW( hWnd_popup, WM_PROPAGATE, 0, 0 );
}

void Processing_Window( HWND hWnd, bool enable )
{
	if ( !enable )
	{
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, TRUE, 0 );	// SetCursor only works from the main thread. Set it to an arrow with hourglass.
		UpdateMenus( UM_DISABLE );									// Disable all processing menu items.
	}
	else
	{
		UpdateMenus( UM_ENABLE );									// Enable the appropriate menu items.
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, FALSE, 0 );	// Reset the cursor.
		_InvalidateRect( hWnd, NULL, FALSE );						// Refresh the number column values.
		_SetFocus( hWnd );											// Give focus back to the listview to allow shortcut keys.
	}
}

void kill_worker_thread()
{
	if ( in_worker_thread )
	{
		// This semaphore will be released when the thread gets killed.
		worker_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		kill_worker_thread_flag = true;	// Causes secondary threads to cease processing and release the semaphore.

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( worker_semaphore, 5000 );
		CloseHandle( worker_semaphore );
		worker_semaphore = NULL;
	}
}

// This will allow our main thread to continue while secondary threads finish their processing.
THREAD_RETURN cleanup( void *pArguments )
{
	kill_worker_thread();
	kill_telephony_thread();
	kill_update_check_thread();
	kill_ml_update_thread();
	kill_ml_worker_thread();
	kill_ringtone_update_thread();

	// DestroyWindow won't work on a window from a different thread. So we'll send a message to trigger it.
	_SendMessageW( g_hWnd_main, WM_DESTROY_ALT, 0, 0 );

	_ExitThread( 0 );
	return 0;
}

// Returns the first match after counting the number of total keyword matches.
allow_ignore_cid_info *find_caller_id_name_match( display_info *di, dllrbt_tree *list, unsigned char list_type )
{
	if ( di == NULL )
	{
		return NULL;
	}

	allow_ignore_cid_info *ret_aicidi = NULL;

	int caller_id_length = lstrlenW( di->caller_id );

	unsigned int *cid_match_count = ( list_type == LIST_TYPE_ALLOW ? &di->allow_cid_match_count : &di->ignore_cid_match_count );

	// Find a keyword that matches the value.
	node_type *node = dllrbt_get_head( list );
	while ( node != NULL )
	{
		allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )node->val;

		if ( aicidi->match_flag & 0x04 )
		{
			int error_code;
			size_t error_offset;

			if ( g_use_regular_expressions )
			{
				pcre2_code *regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )aicidi->caller_id, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );
				if ( regex_code != NULL )
				{
					pcre2_match_data *regex_match_data = _pcre2_match_data_create_from_pattern_16( regex_code, NULL );
					if ( regex_match_data != NULL )
					{
						if ( _pcre2_match_16( regex_code, ( PCRE2_SPTR16 )di->caller_id, caller_id_length, 0, 0, regex_match_data, NULL ) >= 0 )
						{
							++( *cid_match_count );

							aicidi->active = true;

							if ( ret_aicidi == NULL )
							{
								ret_aicidi = aicidi;
							}
						}

						_pcre2_match_data_free_16( regex_match_data );
					}

					_pcre2_code_free_16( regex_code );
				}
			}
		}
		else if ( ( aicidi->match_flag & 0x02 ) && ( aicidi->match_flag & 0x01 ) )
		{
			if ( lstrcmpW( di->caller_id, aicidi->caller_id ) == 0 )
			{
				++( *cid_match_count );

				aicidi->active = true;

				if ( ret_aicidi == NULL )
				{
					ret_aicidi = aicidi;
				}
			}
		}
		else if ( !( aicidi->match_flag & 0x02 ) && ( aicidi->match_flag & 0x01 ) )
		{
			if ( lstrcmpiW( di->caller_id, aicidi->caller_id ) == 0 )
			{
				++( *cid_match_count );

				aicidi->active = true;

				if ( ret_aicidi == NULL )
				{
					ret_aicidi = aicidi;
				}
			}
		}
		else if ( ( aicidi->match_flag & 0x02 ) && !( aicidi->match_flag & 0x01 ) )
		{
			if ( _StrStrW( di->caller_id, aicidi->caller_id ) != NULL )
			{
				++( *cid_match_count );

				aicidi->active = true;

				if ( ret_aicidi == NULL )
				{
					ret_aicidi = aicidi;
				}
			}
		}
		else if ( !( aicidi->match_flag & 0x02 ) && !( aicidi->match_flag & 0x01 ) )
		{
			if ( _StrStrIW( di->caller_id, aicidi->caller_id ) != NULL )
			{
				++( *cid_match_count );

				aicidi->active = true;

				if ( ret_aicidi == NULL )
				{
					ret_aicidi = aicidi;
				}
			}
		}

		node = node->next;
	}

	return ret_aicidi;
}

wchar_t *combine_names_w( wchar_t *first_name, wchar_t *last_name )
{
	int first_name_length = 0;
	int last_name_length = 0;
	int combined_name_length = 0;
	wchar_t *combined_name = NULL;

	if ( first_name == NULL && last_name == NULL )
	{
		return NULL;
	}
	else if ( first_name != NULL && last_name == NULL )
	{
		return GlobalStrDupW( first_name );
	}
	else if ( first_name == NULL && last_name != NULL )
	{
		return GlobalStrDupW( last_name );
	}

	first_name_length = lstrlenW( first_name );
	last_name_length = lstrlenW( last_name );

	combined_name_length = first_name_length + last_name_length + 1 + 1;
	combined_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * combined_name_length );
	_wmemcpy_s( combined_name, combined_name_length, first_name, first_name_length );
	combined_name[ first_name_length ] = L' ';
	_wmemcpy_s( combined_name + first_name_length + 1, combined_name_length - ( first_name_length + 1 ), last_name, last_name_length );
	combined_name[ combined_name_length - 1 ] = 0;	// Sanity.

	return combined_name;
}

void cleanup_custom_caller_id()
{
	if ( custom_cid_list != NULL )
	{
		for ( char i = 0; i < CID_LIST_COUNT; ++i )
		{
			// Free the values of the call_log.
			node_type *node = dllrbt_get_head( custom_cid_list[ i ] );
			while ( node != NULL )
			{
				// Free the linked list if there is one.
				DoublyLinkedList *ci_node = ( DoublyLinkedList * )node->val;
				while ( ci_node != NULL )
				{
					DoublyLinkedList *del_ci_node = ci_node;

					ci_node = ci_node->next;

					GlobalFree( del_ci_node );
				}

				node = node->next;
			}

			dllrbt_delete_recursively( custom_cid_list[ i ] );
			custom_cid_list[ i ] = NULL;
		}

		GlobalFree( custom_cid_list );
		custom_cid_list = NULL;
	}
}

wchar_t *get_custom_caller_id_w( wchar_t *phone_number, contact_info **ci )
{
	*ci = NULL;

	if ( phone_number != NULL && custom_cid_list != NULL )
	{
		DoublyLinkedList *dll = NULL;

		for ( char i = 0; i < CID_LIST_COUNT; ++i )
		{
			dll = ( DoublyLinkedList * )dllrbt_find( custom_cid_list[ i ], ( void * )phone_number, true );
			while ( dll != NULL )
			{
				*ci = ( contact_info * )dll->data;

				if ( *ci != NULL )
				{
					// If there are multiple people with the same office phone number, then use the business name to identify them.
					if ( ( i == 2 && dll->prev != NULL ) || i == 3 )
					{
						if ( ( *ci )->w_business_name != NULL && ( *ci )->w_business_name[ 0 ] != NULL )
						{
							return GlobalStrDupW( ( *ci )->w_business_name );
						}
					}
					else
					{
						// See if there's a first name available.
						if ( ( *ci )->w_first_name != NULL && ( *ci )->w_first_name[ 0 ] != NULL )
						{
							// See if there's a last name available.
							if ( ( *ci )->w_last_name != NULL && ( *ci )->w_last_name[ 0 ] != NULL )
							{
								return combine_names_w( ( *ci )->w_first_name, ( *ci )->w_last_name );
							}
							else	// If there's no last name, then use the first name.
							{
								return GlobalStrDupW( ( *ci )->w_first_name );
							}
						}
						else if ( ( *ci )->w_nickname != NULL && ( *ci )->w_nickname[ 0 ] != NULL )	// If there's no first name, then try the nickname.
						{
							return GlobalStrDupW( ( *ci )->w_nickname );
						}
					}
				}

				dll = dll->next;
			}
		}
	}

	return NULL;
}

void remove_custom_caller_id_w( contact_info *ci )
{
	if ( ci == NULL || custom_cid_list == NULL )
	{
		return;
	}

	wchar_t *phone_number = NULL;

	for ( char i = 0; i < CID_LIST_COUNT; ++i )
	{
		switch ( i )
		{
			case 0: { phone_number = ci->home_phone_number; } break;
			case 1: { phone_number = ci->cell_phone_number; } break;
			case 2: { phone_number = ci->office_phone_number; } break;
			case 3: { phone_number = ci->work_phone_number; } break;
			case 4: { phone_number = ci->other_phone_number; } break;
			case 5: { phone_number = ci->fax_number; } break;
		}

		// Skip blank phone numbers.
		if ( phone_number == NULL || ( phone_number != NULL && phone_number[ 0 ] == NULL ) )
		{
			continue;
		}

		dllrbt_iterator *itr = dllrbt_find( custom_cid_list[ i ], ( void * )phone_number, false );
		if ( itr != NULL )
		{
			// Head of the linked list.
			DoublyLinkedList *ll = ( DoublyLinkedList * )( ( node_type * )itr )->val;

			// Go through each linked list node and remove the one with ci.

			DoublyLinkedList *current_node = ll;
			while ( current_node != NULL )
			{
				if ( ( contact_info * )current_node->data == ci )
				{
					DLL_RemoveNode( &ll, current_node );
					GlobalFree( current_node );

					if ( ll != NULL && ll->data != NULL )
					{
						// Reset the head in the tree.
						( ( node_type * )itr )->val = ( void * )ll;

						switch ( i )
						{
							case 0: { ( ( node_type * )itr )->key = ( void * )( ( contact_info * )ll->data )->home_phone_number; } break;
							case 1: { ( ( node_type * )itr )->key = ( void * )( ( contact_info * )ll->data )->cell_phone_number; } break;
							case 2: { ( ( node_type * )itr )->key = ( void * )( ( contact_info * )ll->data )->office_phone_number; } break;
							case 3: { ( ( node_type * )itr )->key = ( void * )( ( contact_info * )ll->data )->work_phone_number; } break;
							case 4: { ( ( node_type * )itr )->key = ( void * )( ( contact_info * )ll->data )->other_phone_number; } break;
							case 5: { ( ( node_type * )itr )->key = ( void * )( ( contact_info * )ll->data )->fax_number; } break;
						}
					}

					break;
				}

				current_node = current_node->next;
			}

			// If the head of the linked list is NULL, then we can remove the linked list from the tree.
			if ( ll == NULL )
			{
				dllrbt_remove( custom_cid_list[ i ], itr );	// Remove the node from the tree. The tree will rebalance itself.
			}
		}
	}
}

void add_custom_caller_id_w( contact_info *ci )
{
	if ( ci == NULL )
	{
		return;
	}

	wchar_t *phone_number = NULL;

	if ( custom_cid_list == NULL )
	{
		custom_cid_list = ( dllrbt_tree ** )GlobalAlloc( GPTR, sizeof( dllrbt_tree * ) * CID_LIST_COUNT );
	}

	for ( char i = 0; i < CID_LIST_COUNT; ++i )
	{
		if ( custom_cid_list[ i ] == NULL )
		{
			 custom_cid_list[ i ] = dllrbt_create( dllrbt_compare_w );
		}

		switch ( i )
		{
			case 0: { phone_number = ci->home_phone_number; } break;
			case 1: { phone_number = ci->cell_phone_number; } break;
			case 2: { phone_number = ci->office_phone_number; } break;
			case 3: { phone_number = ci->work_phone_number; } break;
			case 4: { phone_number = ci->other_phone_number; } break;
			case 5: { phone_number = ci->fax_number; } break;
		}

		// Skip blank phone numbers.
		if ( phone_number == NULL || ( phone_number != NULL && phone_number[ 0 ] == NULL ) )
		{
			continue;
		}

		// Create the node to insert into a linked list.
		DoublyLinkedList *ci_node = DLL_CreateNode( ( void * )ci );

		// See if our tree has the phone number to add the node to.
		DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( custom_cid_list[ i ], ( void * )phone_number, true );
		if ( dll == NULL )
		{
			// If no phone number exits, insert the node into the tree.
			if ( dllrbt_insert( custom_cid_list[ i ], ( void * )phone_number, ( void * )ci_node ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( ci_node );	// This shouldn't happen.
			}
		}
		else	// If a phone number exits, insert the node into the linked list.
		{
			DLL_AddNode( &dll, ci_node, -1 );	// Insert at the end of the doubly linked list.
		}
	}
}

wchar_t *FormatPhoneNumberW( wchar_t *phone_number )
{
	wchar_t *val = NULL;

	if ( phone_number != NULL )
	{
		int val_length = lstrlenW( phone_number ) + 1;	// Include the NULL terminator.

		if ( is_num_w( phone_number ) >= 0 )
		{
			if ( val_length == 11 )	// 10 digit phone number + 1 NULL character. (555) 555-1234
			{
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( val_length + 4 ) );

				val[ 0 ] = L'(';
				_wmemcpy_s( val + 1, 14, phone_number, 3 );
				val[ 4 ] = L')';
				val[ 5 ] = L' ';
				_wmemcpy_s( val + 6, 9, phone_number + 3, 3 );
				val[ 9 ] = L'-';
				_wmemcpy_s( val + 10, 5, phone_number + 6, 4 );
				val[ 14 ] = 0;	// Sanity
			}
			else if ( val_length == 12 && phone_number[ 0 ] == L'1' )	// 11 digit phone number + 1 NULL character. 1-555-555-1234
			{
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( val_length + 3 ) );

				val[ 0 ] = phone_number[ 0 ];
				val[ 1 ] = L'-';
				_wmemcpy_s( val + 2, 13, phone_number + 1, 3 );
				val[ 5 ] = L'-';
				_wmemcpy_s( val + 6, 9, phone_number + 4, 3 );
				val[ 9 ] = L'-';
				_wmemcpy_s( val + 10, 5, phone_number + 7, 4 );
				val[ 14 ] = 0;	// Sanity
			}
		}
		
		if ( val == NULL )	// Number has some weird length, is unavailable, unknown, etc.
		{
			val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
			_wmemcpy_s( val, val_length, phone_number, val_length );
		}
	}

	return val;
}

// Allocates a new string if characters need escaping. Otherwise, it returns NULL.
char *escape_csv( const char *string )
{
	char *escaped_string = NULL;
	char *q = NULL;
	const char *p = NULL;
	int c = 0;

	if ( string == NULL )
	{
		return NULL;
	}

	// Get the character count and offset it for any quotes.
	for ( c = 0, p = string; *p != NULL; ++p ) 
	{
		if ( *p != '\"' )
		{
			++c;
		}
		else
		{
			c += 2;
		}
	}

	// If the string has no special characters to escape, then return NULL.
	if ( c <= ( p - string ) )
	{
		return NULL;
	}

	q = escaped_string = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( c + 1 ) );

	for ( p = string; *p != NULL; ++p ) 
	{
		if ( *p != '\"' )
		{
			*q = *p;
			++q;
		}
		else
		{
			*q++ = '\"';
			*q++ = '\"';
		}
	}

	*q = 0;	// Sanity.

	return escaped_string;
}
