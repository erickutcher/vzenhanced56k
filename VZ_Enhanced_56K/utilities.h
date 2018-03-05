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

#ifndef _UTILITIES_H
#define _UTILITIES_H

struct copyinfo
{
	unsigned short column;
	HWND hWnd;
};

struct removeinfo
{
	bool disable_critical_section;
	HWND hWnd;
};

char *GlobalStrDupA( const char *_Str );
wchar_t *GlobalStrDupW( const wchar_t *_Str );

void encode_cipher( char *buffer, int buffer_length );
void decode_cipher( char *buffer, int buffer_length );

void CreatePopup( displayinfo *fi );

void Processing_Window( HWND hWnd, bool enable );

THREAD_RETURN cleanup( void *pArguments );

void UpdateColumnOrders();
void CheckColumnOrders( unsigned char list, char *column_arr[], unsigned char num_columns );
void CheckColumnWidths();
void OffsetVirtualIndices( int *arr, char *column_arr[], unsigned char num_columns, unsigned char total_columns );
int GetColumnIndexFromVirtualIndex( int virtual_index, char *column_arr[], unsigned char num_columns );
int GetVirtualIndexFromColumnIndex( int column_index, char *column_arr[], unsigned char num_columns );

void free_displayinfo( displayinfo **di );
void free_contactinfo( contactinfo **ci );
void free_ignoreinfo( ignoreinfo **ii );
void free_ignorecidinfo( ignorecidinfo **icidi );

wchar_t *FormatPhoneNumber( char *phone_number );
void FormatDisplayInfo( displayinfo *di );

ignorecidinfo *find_ignore_caller_id_name_match( displayinfo *di );

void add_custom_caller_id( contactinfo *ci );
void remove_custom_caller_id( contactinfo *ci );
char *get_custom_caller_id( char *phone_number, contactinfo **ci );
void cleanup_custom_caller_id();

wchar_t *GetMonth( unsigned short month );
wchar_t *GetDay( unsigned short day );
void UnixTimeToSystemTime( DWORD t, SYSTEMTIME *st );
void GetCurrentCounterTime( ULARGE_INTEGER *li );
DWORD GetRemainingDropCallWaitTime( ULARGE_INTEGER start, ULARGE_INTEGER stop );
char *url_encode( char *str, unsigned int str_len, unsigned int *enc_len = 0 );
char is_num( const char *str );

int dllrbt_compare_ptr( void *a, void *b );
int dllrbt_compare_a( void *a, void *b );
int dllrbt_compare_w( void *a, void *b );
int dllrbt_compare_guid( void *a, void *b );
int dllrbt_icid_compare( void *a, void *b );

void kill_worker_thread();
void kill_telephony_thread();
void kill_update_check_thread();

char *GetFileExtension( char *path );
char *GetFileNameA( char *path );
wchar_t *GetFileNameW( wchar_t *path );
char *GetMIMEByFileName( char *filename );

HWND GetCurrentListView();
char *GetSelectedColumnPhoneNumber( unsigned int column_id );

char *escape_csv( const char *string );

//extern unsigned short bad_area_codes[];

#endif
