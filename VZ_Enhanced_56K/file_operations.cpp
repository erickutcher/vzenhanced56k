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

#include "file_operations.h"
#include "utilities.h"
#include "message_log_utilities.h"
#include "string_tables.h"

RANGE *ignore_range_list[ 16 ];

dllrbt_tree *modem_list = NULL;
modeminfo *default_modem = NULL;

dllrbt_tree *recording_list = NULL;
ringtoneinfo *default_recording = NULL;

dllrbt_tree *ringtone_list = NULL;
ringtoneinfo *default_ringtone = NULL;

dllrbt_tree *ignore_list = NULL;
bool ignore_list_changed = false;

dllrbt_tree *ignore_cid_list = NULL;
bool ignore_cid_list_changed = false;

dllrbt_tree *contact_list = NULL;
bool contact_list_changed = false;

CRITICAL_SECTION auto_save_cs;

void LoadRecordings( dllrbt_tree *list )
{
	_wmemcpy_s( app_directory + app_directory_length, MAX_PATH - app_directory_length, L"\\recordings\\*\0", 14 );
	app_directory[ app_directory_length + 13 ] = 0;	// Sanity.

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFileEx( ( LPCWSTR )app_directory, FindExInfoStandard, &FindFileData, FindExSearchNameMatch, NULL, 0 );
	if ( hFind != INVALID_HANDLE_VALUE ) 
	{
		do
		{
			// See if the file is a directory.
			if ( ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
			{
				int extension_offset = 0;
				int file_name_length = 0;

				file_name_length = extension_offset = lstrlenW( FindFileData.cFileName );

				// Find the start of the file extension.
				while ( extension_offset != 0 && FindFileData.cFileName[ --extension_offset ] != L'.' );

				// Load only wav, mp3, and midi files.
				if ( ( ( file_name_length - extension_offset ) == 4 &&
						 _StrCmpNIW( FindFileData.cFileName + ( extension_offset + 1 ), L"wav", 3 ) == 0 ) ||
					 ( ( file_name_length - extension_offset ) == 5 &&
						 _StrCmpNIW( FindFileData.cFileName + ( extension_offset + 1 ), L"wave", 4 ) == 0 ) )
				{
					int w_file_path_length = app_directory_length + file_name_length + 13;

					ringtoneinfo *rti = ( ringtoneinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ringtoneinfo ) );

					rti->ringtone_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * w_file_path_length );
					_wmemcpy_s( rti->ringtone_path, w_file_path_length, app_directory, app_directory_length + 12 );
					_wmemcpy_s( rti->ringtone_path + ( app_directory_length + 12 ), w_file_path_length - ( app_directory_length + 12 ), FindFileData.cFileName, file_name_length + 1 );
					rti->ringtone_path[ app_directory_length + file_name_length + 12 ] = 0;

					rti->ringtone_file = rti->ringtone_path + ( app_directory_length + 12 );

					if ( dllrbt_insert( list, ( void * )rti->ringtone_file, ( void * )rti ) != DLLRBT_STATUS_OK )
					{
						// ringtone_file points to the same memory as ringtone_path. So don't free it.
						GlobalFree( rti->ringtone_path );
						GlobalFree( rti );
					}
				}
			}
		}
		while ( FindNextFile( hFind, &FindFileData ) != 0 );	// Go to the next file.

		FindClose( hFind );	// Close the find file handle.

		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Loaded_recording_directory )
	}
}

void LoadRingtones( dllrbt_tree *list )
{
	_wmemcpy_s( app_directory + app_directory_length, MAX_PATH - app_directory_length, L"\\ringtones\\*\0", 13 );
	app_directory[ app_directory_length + 12 ] = 0;	// Sanity.

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFileEx( ( LPCWSTR )app_directory, FindExInfoStandard, &FindFileData, FindExSearchNameMatch, NULL, 0 );
	if ( hFind != INVALID_HANDLE_VALUE ) 
	{
		do
		{
			// See if the file is a directory.
			if ( ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
			{
				int extension_offset = 0;
				int file_name_length = 0;

				file_name_length = extension_offset = lstrlenW( FindFileData.cFileName );

				// Find the start of the file extension.
				while ( extension_offset != 0 && FindFileData.cFileName[ --extension_offset ] != L'.' );

				// Load only wav, mp3, and midi files.
				if ( ( ( file_name_length - extension_offset ) == 4 &&
					   ( _StrCmpNIW( FindFileData.cFileName + ( extension_offset + 1 ), L"wav", 3 ) == 0 ||
						 _StrCmpNIW( FindFileData.cFileName + ( extension_offset + 1 ), L"mp3", 3 ) == 0 ||
						 _StrCmpNIW( FindFileData.cFileName + ( extension_offset + 1 ), L"mid", 3 ) == 0 ) ) ||
					 ( ( file_name_length - extension_offset ) == 5 &&
					   ( _StrCmpNIW( FindFileData.cFileName + ( extension_offset + 1 ), L"wave", 4 ) == 0 ||
						 _StrCmpNIW( FindFileData.cFileName + ( extension_offset + 1 ), L"midi", 4 ) == 0 ) ) )
				{
					int w_file_path_length = app_directory_length + file_name_length + 12;

					ringtoneinfo *rti = ( ringtoneinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ringtoneinfo ) );

					rti->ringtone_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * w_file_path_length );
					_wmemcpy_s( rti->ringtone_path, w_file_path_length, app_directory, app_directory_length + 11 );
					_wmemcpy_s( rti->ringtone_path + ( app_directory_length + 11 ), w_file_path_length - ( app_directory_length + 11 ), FindFileData.cFileName, file_name_length + 1 );
					rti->ringtone_path[ app_directory_length + file_name_length + 11 ] = 0;

					rti->ringtone_file = rti->ringtone_path + ( app_directory_length + 11 );

					if ( dllrbt_insert( list, ( void * )rti->ringtone_file, ( void * )rti ) != DLLRBT_STATUS_OK )
					{
						// ringtone_file points to the same memory as ringtone_path. So don't free it.
						GlobalFree( rti->ringtone_path );
						GlobalFree( rti );
					}
				}
			}
		}
		while ( FindNextFile( hFind, &FindFileData ) != 0 );	// Go to the next file.

		FindClose( hFind );	// Close the find file handle.

		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Loaded_ringtone_directory )
	}
}

char read_config()
{
	char status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\vz_enhanced_56k_settings\0", 26 );
	base_directory[ base_directory_length + 25 ] = 0;	// Sanity.

	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, pos = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

		int reserved = 1024 - 304;	// There are currently 304 bytes used for settings (not including the strings).

		// Our config file is going to be small. If it's something else, we're not going to read it.
		// Add 5 for the strings.
		if ( fz >= ( 1024 + 5 ) && fz < 10240 )
		{
			char *cfg_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 1 );

			ReadFile( hFile_cfg, cfg_buf, sizeof( char ) * fz, &read, NULL );

			cfg_buf[ fz ] = 0;	// Guarantee a NULL terminated buffer.

			// Read the config. It must be in the order specified below.
			if ( read == fz && _memcmp( cfg_buf, MAGIC_ID_SETTINGS, 4 ) == 0 )
			{
				char *next = cfg_buf + 4;

				_memcpy_s( &cfg_connection_auto_login, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_connection_reconnect, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_connection_retries, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_connection_timeout, sizeof( unsigned short ), next, sizeof( unsigned short ) );
				next += sizeof( unsigned short );
				_memcpy_s( &cfg_connection_ssl_version, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_check_for_updates, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_modem_guid, sizeof( GUID ), next, sizeof( GUID ) );
				next += sizeof( GUID );

				_memcpy_s( &cfg_popup_enable_recording, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_repeat_recording, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_drop_call_wait, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				_memcpy_s( &cfg_pos_x, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_pos_y, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_width, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_height, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_tab_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_tab_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_tab_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width5, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width6, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column2_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width5, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width6, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width7, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width8, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column2_width9, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width10, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width11, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width12, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width13, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width14, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width15, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width16, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width17, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column3_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column3_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column3_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column4_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column4_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column4_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column4_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column4_width5, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order5, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order6, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column2_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order5, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order6, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order7, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order8, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column2_order9, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order10, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order11, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order12, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order13, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order14, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order15, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order16, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order17, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column3_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column3_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column3_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column4_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column4_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column4_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column4_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column4_order5, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_tray_icon, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_close_to_tray, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_minimize_to_tray, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_silent_startup, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_always_on_top, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_enable_call_log_history, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_enable_message_log, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_enable_popups, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_popup_hide_border, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_width, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_popup_height, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_popup_position, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_show_contact_picture, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_popup_time, sizeof( unsigned short ), next, sizeof( unsigned short ) );
				next += sizeof( unsigned short );
				_memcpy_s( &cfg_popup_transparency, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				_memcpy_s( &cfg_popup_gradient, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_gradient_direction, sizeof( bool ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_background_color1, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_background_color2, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );

				_memcpy_s( &cfg_popup_font_color1, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_color2, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_color3, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_height1, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_height2, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_height3, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_weight1, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_weight2, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_weight3, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_italic1, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_italic2, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_italic3, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_underline1, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_underline2, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_underline3, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_strikeout1, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_strikeout2, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_strikeout3, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_shadow1, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_font_shadow2, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_font_shadow3, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_font_shadow_color1, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_shadow_color2, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_shadow_color3, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_justify_line1, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_justify_line2, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_justify_line3, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_line_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_popup_line_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_popup_line_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_popup_time_format, sizeof( bool ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_enable_ringtones, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				next += reserved;	// Skip past reserved bytes.

				int string_length = 0;
				int cfg_val_length = 0;

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read sound.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_recording = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_recording, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read font face.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_font_face1 = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_font_face1, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read font face.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_font_face2 = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_font_face2, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read font face.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_font_face3 = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_font_face3, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read sound.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_ringtone = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_ringtone, cfg_val_length );

					next += string_length;
				}

				// Set the default values for bad configuration values.

				if ( !cfg_tray_icon )
				{
					cfg_silent_startup = false;
				}

				// These should never be negative.
				if ( cfg_popup_time > 300 ) { cfg_popup_time = 10; }
				if ( cfg_connection_retries > 10 ) { cfg_connection_retries = 3; }
				if ( cfg_connection_timeout > 300 || ( cfg_connection_timeout > 0 && cfg_connection_timeout < 60 ) ) { cfg_connection_timeout = 60; }
				if ( cfg_repeat_recording == 0 || cfg_repeat_recording > 10 ) { cfg_repeat_recording = 1; }
				if ( cfg_drop_call_wait == 0 || cfg_drop_call_wait > 10 ) { cfg_connection_timeout = 4; }

				CheckColumnOrders( 0, call_log_columns, NUM_COLUMNS1 );
				CheckColumnOrders( 1, contact_list_columns, NUM_COLUMNS2 );
				CheckColumnOrders( 2, ignore_list_columns, NUM_COLUMNS3 );
				CheckColumnOrders( 3, ignore_cid_list_columns, NUM_COLUMNS4 );

				// Revert column widths if they exceed our limits.
				CheckColumnWidths();
			}
			else
			{
				status = -2;	// Bad file format.
			}

			GlobalFree( cfg_buf );
		}
		else
		{
			status = -3;	// Incorrect file size.
		}

		CloseHandle( hFile_cfg );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char save_config()
{
	char status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\vz_enhanced_56k_settings\0", 26 );
	base_directory[ base_directory_length + 25 ] = 0;	// Sanity.

	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		int reserved = 1024 - 304; // There are currently 304 bytes used for settings (not including the strings).
		int size = ( sizeof( int ) * 51 ) + ( sizeof( bool ) * 19 ) + ( sizeof( char ) * 61 ) + ( sizeof( unsigned short ) * 2 ) + sizeof( GUID ) + reserved;
		int pos = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_SETTINGS, sizeof( char ) * 4 );	// Magic identifier for the main program's settings.
		pos += ( sizeof( char ) * 4 );

		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_auto_login, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_reconnect, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_retries, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_timeout, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_ssl_version, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_check_for_updates, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_modem_guid, sizeof( GUID ) );
		pos += sizeof( GUID );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_enable_recording, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_repeat_recording, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_drop_call_wait, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_pos_x, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_pos_y, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_width, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_height, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_tab_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_tab_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_tab_order3, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width4, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width5, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width6, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width4, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width5, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width6, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width7, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width8, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width9, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width10, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width11, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width12, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width13, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width14, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width15, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width16, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width17, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_width3, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width4, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width5, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order4, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order5, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order6, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order4, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order5, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order6, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order7, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order8, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order9, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order10, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order11, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order12, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order13, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order14, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order15, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order16, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order17, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_order3, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order4, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order5, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_tray_icon, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_close_to_tray, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_minimize_to_tray, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_silent_startup, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_always_on_top, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_call_log_history, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_message_log, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_popups, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_hide_border, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_width, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_height, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_position, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_show_contact_picture, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_time, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_transparency, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_gradient, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_gradient_direction, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_background_color1, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_background_color2, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_color1, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_color2, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_color3, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_height1, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_height2, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_height3, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_weight1, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_weight2, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_weight3, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_italic1, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_italic2, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_italic3, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_underline1, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_underline2, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_underline3, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_strikeout1, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_strikeout2, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_strikeout3, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow1, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow2, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow3, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow_color1, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow_color2, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow_color3, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_justify_line1, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_justify_line2, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_justify_line3, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_line_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_line_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_line_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_time_format, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_enable_ringtones, sizeof( bool ) );
		pos += sizeof( bool );

		// Write Reserved bytes.
		_memzero( write_buf + pos, size - pos );

		DWORD write = 0;
		WriteFile( hFile_cfg, write_buf, size, &write, NULL );

		GlobalFree( write_buf );

		int cfg_val_length = 0;
		char *utf8_cfg_val = NULL;

		if ( cfg_recording != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_recording, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_recording, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_popup_font_face1 != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face1, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face1, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_popup_font_face2 != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face2, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face2, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_popup_font_face3 != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face3, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face3, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_popup_ringtone != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_ringtone, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_ringtone, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		CloseHandle( hFile_cfg );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char read_ignore_list( wchar_t *file_path, dllrbt_tree *list )
{
	char status = 0;

	HANDLE hFile_ignore = hFile_ignore = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;
		bool skip_next_newline = false;

		char magic_identifier[ 4 ];
		ReadFile( hFile_ignore, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_IGNORE_PN, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_ignore, NULL ) - 4;

			char *ignore_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 28 bytes.

			while ( total_read < fz )
			{
				ReadFile( hFile_ignore, ignore_buf, sizeof( char ) * 32768, &read, NULL );
				ignore_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				total_read += read;

				char *p = ignore_buf;
				char *s = ignore_buf;

				while ( p != NULL && *s != NULL )
				{
					// If we read a partial number and it was too long.
					if ( skip_next_newline )
					{
						skip_next_newline = false;

						// Then we skip the remainder of the number.
						p = _StrChrA( s, '\x1e' );

						// If it was the last number, then we're done.
						if ( p == NULL )
						{
							continue;
						}

						// Move to the next value.
						++p;
						s = p;
					}

					// Find the newline token.
					p = _StrChrA( s, '\x1e' );

					// If we found one, copy the number.
					if ( p != NULL || ( total_read >= fz ) )
					{
						char *tp = p;

						if ( p != NULL )
						{
							*p = 0;
							++p;
						}
						else	// Handle an EOF.
						{
							tp = ( ignore_buf + read );
						}

						char *t = _StrChrA( s, '\x1f' );	// Find the ignore count.

						DWORD length1 = 0;
						DWORD length2 = 0;

						if ( t == NULL )
						{
							length1 = ( tp - s );
						}
						else
						{
							length1 = ( t - s );
							++t;
							length2 = ( ( tp - t ) > 10 ? 10 : ( tp - t ) );
						}

						// Make sure the number is at most 15 + 1 digits.
						if ( ( length1 <= 16 && length1 > 0 ) && ( length2 <= 10 && length2 >= 0 ) )
						{
							ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreinfo ) );

							ii->c_phone_number = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
							_memcpy_s( ii->c_phone_number, length1 + 1, s, length1 );
							*( ii->c_phone_number + length1 ) = 0;	// Sanity

							ii->phone_number = FormatPhoneNumber( ii->c_phone_number );

							if ( length2 == 0 )
							{
								ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
								*ii->c_total_calls = '0';
								*( ii->c_total_calls + 1 ) = 0;	// Sanity

								ii->count = 0;
							}
							else
							{
								ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
								_memcpy_s( ii->c_total_calls, length2 + 1, t, length2 );
								*( ii->c_total_calls + length2 ) = 0;	// Sanity

								ii->count = _strtoul( ii->c_total_calls, NULL, 10 );
							}

							int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
							wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
							MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

							ii->total_calls = val;

							ii->state = 0;

							if ( dllrbt_insert( list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
							{
								free_ignoreinfo( &ii );
							}
							else if ( is_num( ii->c_phone_number ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
							{
								RangeAdd( &ignore_range_list[ length1 - 1 ], ii->c_phone_number, length1 );
							}
						}

						// Move to the next value.
						s = p;
					}
					else if ( total_read < fz )	// Reached the end of what we've read.
					{
						// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid number.
						DWORD offset = ( ( ignore_buf + read ) - s );

						char *t = _StrChrA( s, '\x1f' );	// Find the ignore count.

						DWORD length1 = 0;
						DWORD length2 = 0;

						if ( t == NULL )
						{
							length1 = offset;
						}
						else
						{
							length1 = ( t - s );
							++t;
							length2 = ( ( ( ignore_buf + read ) - t ) > 10 ? 10 : ( ( ignore_buf + read ) - t ) );
						}

						// Max recommended number length is 15 + 1.
						// We can only offset back less than what we've read, and no more than 27 digits (bytes). 15 + 1 + 1 + 10
						// This means read, and our buffer size should be greater than 27.
						// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 28.
						if ( offset < read && length1 <= 16 && length2 <= 10 )
						{
							total_read -= offset;
							SetFilePointer( hFile_ignore, total_read, NULL, FILE_BEGIN );
						}
						else	// The length of the next number is too long.
						{
							skip_next_newline = true;
						}
					}
				}
			}

			GlobalFree( ignore_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_ignore );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char read_ignore_cid_list( wchar_t *file_path, dllrbt_tree *list )
{
	char status = 0;

	HANDLE hFile_ignore = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;
		bool skip_next_newline = false;

		char magic_identifier[ 4 ];
		ReadFile( hFile_ignore, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_IGNORE_CNAM, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_ignore, NULL ) - 4;

			char *ignore_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 29 bytes.

			while ( total_read < fz )
			{
				ReadFile( hFile_ignore, ignore_buf, sizeof( char ) * 32768, &read, NULL );
				ignore_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				total_read += read;

				char *p = ignore_buf;
				char *s = ignore_buf;

				while ( p != NULL && *s != NULL )
				{
					// If we read a partial number and it was too long.
					if ( skip_next_newline )
					{
						skip_next_newline = false;

						// Then we skip the remainder of the number.
						p = _StrChrA( s, '\x1e' );

						// If it was the last number, then we're done.
						if ( p == NULL )
						{
							continue;
						}

						// Move to the next value.
						++p;
						s = p;
					}

					// Find the newline token.
					p = _StrChrA( s, '\x1e' );

					// If we found one, copy the number.
					if ( p != NULL || ( total_read >= fz ) )
					{
						char *tp = p;

						if ( p != NULL )
						{
							*p = 0;
							++p;
						}
						else	// Handle an EOF.
						{
							tp = ( ignore_buf + read );
						}

						char *t = _StrChrA( s, '\x1f' );	// Find the match values.

						if ( t != NULL && ( ( tp - t ) >= 2 ) )
						{
							DWORD length1 = ( t - s );
							DWORD length2 = 0;

							bool match_case = ( t[ 1 ] & 0x02 ? true : false );
							bool match_whole_word = ( t[ 1 ] & 0x01 ? true : false );

							t = _StrChrA( t + 2, '\x1f' );

							if ( t != NULL )
							{
								++t;
								length2 = ( ( tp - t ) > 10 ? 10 : ( tp - t ) );
							}

							// Make sure the number is at most 15 digits.
							if ( ( length1 <= 15 && length1 > 0 ) && ( length2 <= 10 && length2 >= 0 ) )
							{
								ignorecidinfo *icidi = ( ignorecidinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidinfo ) );

								icidi->c_caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
								_memcpy_s( icidi->c_caller_id, length1 + 1, s, length1 );
								*( icidi->c_caller_id + length1 ) = 0;	// Sanity

								int val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
								wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
								MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, val, val_length );

								icidi->caller_id = val;

								if ( length2 == 0 )
								{
									icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
									*icidi->c_total_calls = '0';
									*( icidi->c_total_calls + 1 ) = 0;	// Sanity

									icidi->count = 0;
								}
								else
								{
									icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
									_memcpy_s( icidi->c_total_calls, length2 + 1, t, length2 );
									*( icidi->c_total_calls + length2 ) = 0;	// Sanity

									icidi->count = _strtoul( icidi->c_total_calls, NULL, 10 );
								}

								val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
								val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
								MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, val, val_length );

								icidi->total_calls = val;

								icidi->match_case = match_case;
								icidi->match_whole_word = match_whole_word;

								if ( !icidi->match_case )
								{
									icidi->c_match_case = GlobalStrDupA( "No" );
									icidi->w_match_case = GlobalStrDupW( ST_No );
								}
								else
								{
									icidi->c_match_case = GlobalStrDupA( "Yes" );
									icidi->w_match_case = GlobalStrDupW( ST_Yes );
								}

								if ( !icidi->match_whole_word )
								{
									icidi->c_match_whole_word = GlobalStrDupA( "No" );
									icidi->w_match_whole_word = GlobalStrDupW( ST_No );
								}
								else
								{
									icidi->c_match_whole_word = GlobalStrDupA( "Yes" );
									icidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
								}

								icidi->state = 0;
								icidi->active = false;

								if ( dllrbt_insert( list, ( void * )icidi, ( void * )icidi ) != DLLRBT_STATUS_OK )
								{
									free_ignorecidinfo( &icidi );
								}
							}
						}

						// Move to the next value.
						s = p;
					}
					else if ( total_read < fz )	// Reached the end of what we've read.
					{
						// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid caller ID.
						DWORD offset = ( ( ignore_buf + read ) - s );

						char *t = _StrChrA( s, '\x1f' );	// Find the match values.

						DWORD length1 = 0;
						DWORD length2 = 0;
						DWORD length3 = 0;

						if ( t == NULL )
						{
							length1 = offset;
						}
						else
						{
							length1 = ( t - s );

							s = t + 1;

							t = _StrChrA( s, '\x1f' );	// Find the total count.

							if ( t == NULL )
							{
								length2 = ( ( ignore_buf + read ) - s );
							}
							else
							{
								length2 = ( t - s );
								++t;
								length3 = ( ( ( ignore_buf + read ) - t ) > 10 ? 10 : ( ( ignore_buf + read ) - t ) );
							}
						}

						// Max recommended caller ID length is 15.
						// We can only offset back less than what we've read, and no more than 28 bytes. 15 + 1 + 1 + 1 + 10.
						// This means read, and our buffer size should be greater than 28.
						// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 29.
						if ( offset < read && length1 <= 15 && length2 <= 2 && length3 <= 10 )
						{
							total_read -= offset;
							SetFilePointer( hFile_ignore, total_read, NULL, FILE_BEGIN );
						}
						else	// The length of the next caller ID is too long.
						{
							skip_next_newline = true;
						}
					}
				}
			}

			GlobalFree( ignore_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_ignore );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char save_ignore_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_ignore = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 28 bytes.

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_IGNORE_PN, sizeof( char ) * 4 );	// Magic identifier for the ignore phone number list.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( ignore_list );
		while ( node != NULL )
		{
			ignoreinfo *ii = ( ignoreinfo * )node->val;

			if ( ii != NULL && ii->c_phone_number != NULL && ii->c_total_calls != NULL )
			{
				int phone_number_length = lstrlenA( ii->c_phone_number );
				int total_calls_length = lstrlenA( ii->c_total_calls );

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + phone_number_length + total_calls_length + 2 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore numbers that are greater than 15 + 1 digits (bytes).
				if ( phone_number_length + total_calls_length + 2 <= 28 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, ii->c_phone_number, phone_number_length );
					pos += phone_number_length;
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, ii->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\x1e';
				}
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_ignore );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char save_ignore_cid_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_ignore = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 29 bytes.

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_IGNORE_CNAM, sizeof( char ) * 4 );	// Magic identifier for the ignore caller ID name list.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( ignore_cid_list );
		while ( node != NULL )
		{
			ignorecidinfo *icidi = ( ignorecidinfo * )node->val;

			if ( icidi != NULL && icidi->c_caller_id != NULL && icidi->c_total_calls != NULL )
			{
				int caller_id_length = lstrlenA( icidi->c_caller_id );
				int total_calls_length = lstrlenA( icidi->c_total_calls );

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + caller_id_length + total_calls_length + 1 + 3 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore caller ID names that are greater than 15 digits (bytes).
				if ( caller_id_length + total_calls_length + 1 + 3 <= 29 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, icidi->c_caller_id, caller_id_length );
					pos += caller_id_length;
					write_buf[ pos++ ] = '\x1f';
					write_buf[ pos++ ] = 0x80 | ( icidi->match_case ? 0x02 : 0x00 ) | ( icidi->match_whole_word ? 0x01 : 0x00 );
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, icidi->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\x1e';
				}
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_ignore );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char read_call_log_history( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_read = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		bool ignored = false;
		ULONGLONG time = 0;
		char *caller_id = NULL;
		char *call_from = NULL;

		char range_number[ 32 ];

		char magic_identifier[ 4 ];
		ReadFile( hFile_read, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_CALL_LOG, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

			char *history_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

			while ( total_read < fz )
			{
				// Stop processing and exit the function.
				/*if ( kill_worker_thread_flag )
				{
					break;
				}*/

				ReadFile( hFile_read, history_buf, sizeof( char ) * 524288, &read, NULL );

				history_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				// Make sure that we have at least part of the entry. 2 = 2 NULL strings. This is the minimum size an entry could be.
				if ( read < ( sizeof( bool ) + sizeof( ULONGLONG ) + 2 ) )
				{
					break;
				}

				total_read += read;

				// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
				// If it's larger than our buffer, then the file is probably invalid/corrupt.
				if ( total_read == last_total )
				{
					break;
				}

				last_total = total_read;

				p = history_buf;
				offset = last_entry = 0;

				while ( offset < read )
				{
					// Stop processing and exit the function.
					/*if ( kill_worker_thread_flag )
					{
						break;
					}*/

					caller_id = NULL;
					call_from = NULL;

					// Ignored state
					offset += sizeof( bool );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &ignored, sizeof( bool ), p, sizeof( bool ) );
					p += sizeof( bool );

					// Call time
					offset += sizeof( ULONGLONG );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &time, sizeof( ULONGLONG ), p, sizeof( ULONGLONG ) );
					p += sizeof( ULONGLONG );

					// Caller ID
					int string_length = lstrlenA( p ) + 1;

					offset += string_length;
					if ( offset >= read ) { goto CLEANUP; }

					caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
					_memcpy_s( caller_id, string_length, p, string_length );
					*( caller_id + ( string_length - 1 ) ) = 0;	// Sanity

					p += string_length;

					// Incoming number
					string_length = lstrlenA( p );

					offset += ( string_length + 1 );
					if ( offset > read ) { goto CLEANUP; }	// Offset must equal read for the last string in the file. It's truncated/corrupt if it doesn't.

					int call_from_length = min( string_length, 16 );
					int adjusted_size = call_from_length + 1;

					call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * adjusted_size );
					_memcpy_s( call_from, adjusted_size, p, adjusted_size );
					*( call_from + ( adjusted_size - 1 ) ) = 0;	// Sanity

					p += ( string_length + 1 );

					last_entry = offset;	// This value is the ending offset of the last valid entry.

					displayinfo *di = ( displayinfo * )GlobalAlloc( GPTR, sizeof( displayinfo ) );

					di->ci.call_from = call_from;
					di->ci.caller_id = caller_id;
					di->ci.ignored = ignored;
					di->time.QuadPart = time;

					// Create the node to insert into a linked list.
					DoublyLinkedList *di_node = DLL_CreateNode( ( void * )di );

					// See if our tree has the phone number to add the node to.
					DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
					if ( dll == NULL )
					{
						// If no phone number exits, insert the node into the tree.
						if ( dllrbt_insert( call_log, ( void * )di->ci.call_from, ( void * )di_node ) != DLLRBT_STATUS_OK )
						{
							GlobalFree( di_node );	// This shouldn't happen.
						}
					}
					else	// If a phone number exits, insert the node into the linked list.
					{
						DLL_AddNode( &dll, di_node, -1 );	// Insert at the end of the doubly linked list.
					}

					// Search the ignore_list for a match.
					ignoreinfo *ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )di->ci.call_from, true );

					// Try searching the range list.
					if ( ii == NULL )
					{
						_memzero( range_number, 32 );

						int range_index = call_from_length;
						range_index = ( range_index > 0 ? range_index - 1 : 0 );

						if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) )
						{
							ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )range_number, true );
						}
					}

					if ( ii != NULL )
					{
						di->ignore_phone_number = true;
					}

					// Search for the first ignore caller ID list match. di->ignore_cid_match_count will be updated here for all keyword matches.
					find_ignore_caller_id_name_match( di );

					_SendNotifyMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 1 ), ( LPARAM )di );	// Add entry to listview and don't show popup.

					continue;

	CLEANUP:
					GlobalFree( caller_id );
					GlobalFree( call_from );

					// Go back to the last valid entry.
					if ( total_read < fz )
					{
						total_read -= ( read - last_entry );
						SetFilePointer( hFile_read, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
					}

					break;
				}
			}

			GlobalFree( history_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_read );	
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}


char save_call_log_history( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_call_log = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_call_log != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_CALL_LOG, sizeof( char ) * 4 );	// Magic identifier for the call log history.
		pos += ( sizeof( char ) * 4 );

		int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );

		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		for ( lvi.iItem = 0; lvi.iItem < item_count; ++lvi.iItem )
		{
			_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

			displayinfo *di = ( displayinfo * )lvi.lParam;

			// lstrlen is safe for NULL values.
			int caller_id_length = lstrlenA( di->ci.caller_id ) + 1;
			int phone_number_length1 = lstrlenA( di->ci.call_from ) + 1;

			// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
			if ( ( signed )( pos + phone_number_length1 + caller_id_length + sizeof( ULONGLONG ) + sizeof( bool ) ) > size )
			{
				// Dump the buffer.
				WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
				pos = 0;
			}

			_memcpy_s( write_buf + pos, size - pos, &di->ci.ignored, sizeof( bool ) );
			pos += sizeof( bool );

			_memcpy_s( write_buf + pos, size - pos, &di->time.QuadPart, sizeof( ULONGLONG ) );
			pos += sizeof( ULONGLONG );

			_memcpy_s( write_buf + pos, size - pos, di->ci.caller_id, caller_id_length );
			pos += caller_id_length;

			_memcpy_s( write_buf + pos, size - pos, di->ci.call_from, phone_number_length1 );
			pos += phone_number_length1;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_call_log );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char save_call_log_csv_file( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_call_log = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_call_log != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;
		char unix_timestamp[ 21 ];
		_memzero( unix_timestamp, 21 );

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		// Write the UTF-8 BOM and CSV column titles.
		WriteFile( hFile_call_log, "\xEF\xBB\xBF\"Caller ID Name\",\"Phone Number\",\"Date and Time\",\"Unix Timestamp\",\"Ignore Phone Number\",\"Ignore Caller ID Name\"", 113, &write, NULL );

		node_type *node = dllrbt_get_head( call_log );
		while ( node != NULL )
		{
			// Stop processing and exit the function.
			/*if ( kill_worker_thread_flag )
			{
				break;
			}*/

			DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
			while ( di_node != NULL )
			{
				// Stop processing and exit the function.
				/*if ( kill_worker_thread_flag )
				{
					break;
				}*/

				displayinfo *di = ( displayinfo * )di_node->data;

				if ( di != NULL )
				{
					// lstrlen is safe for NULL values.
					int phone_number_length1 = lstrlenA( di->ci.call_from );

					wchar_t *w_ignore = ( di->ignore_phone_number ? ST_Yes : ST_No );
					wchar_t *w_ignore_cid = ( di->ignore_cid_match_count > 0 ? ST_Yes : ST_No );

					int ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, NULL, 0, NULL, NULL );
					char *utf8_ignore = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ignore_length ); // Size includes the null character.
					ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, utf8_ignore, ignore_length, NULL, NULL ) - 1;

					int ignore_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore_cid, -1, NULL, 0, NULL, NULL );
					char *utf8_ignore_cid = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ignore_cid_length ); // Size includes the null character.
					ignore_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore_cid, -1, utf8_ignore_cid, ignore_cid_length, NULL, NULL ) - 1;

					char *escaped_caller_id = escape_csv( di->ci.caller_id );

					int caller_id_length = lstrlenA( ( escaped_caller_id != NULL ? escaped_caller_id : di->ci.caller_id ) );

					int time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, NULL, 0, NULL, NULL );
					char *utf8_time = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * time_length ); // Size includes the null character.
					time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, utf8_time, time_length, NULL, NULL ) - 1;

					// Convert the time into a 32bit Unix timestamp.
					ULARGE_INTEGER date;
					date.HighPart = di->time.HighPart;
					date.LowPart = di->time.LowPart;

					date.QuadPart -= ( 11644473600000 * 10000 );

					// Divide the 64bit value.
					__asm
					{
						xor edx, edx;				//; Zero out the register so we don't divide a full 64bit value.
						mov eax, date.HighPart;		//; We'll divide the high order bits first.
						mov ecx, FILETIME_TICKS_PER_SECOND;
						div ecx;
						mov date.HighPart, eax;		//; Store the high order quotient.
						mov eax, date.LowPart;		//; Now we'll divide the low order bits.
						div ecx;
						mov date.LowPart, eax;		//; Store the low order quotient.
						//; Any remainder will be stored in edx. We're not interested in it though.
					}

					int timestamp_length = __snprintf( unix_timestamp, 21, "%llu", date.QuadPart );

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( pos + phone_number_length1 + caller_id_length + time_length + timestamp_length + ignore_length + ignore_cid_length + 11 > size )
					{
						// Dump the buffer.
						WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
						pos = 0;
					}

					// Add to the buffer.
					write_buf[ pos++ ] = '\r';
					write_buf[ pos++ ] = '\n';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, ( escaped_caller_id != NULL ? escaped_caller_id : di->ci.caller_id ), caller_id_length );
					pos += caller_id_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, di->ci.call_from, phone_number_length1 );
					pos += phone_number_length1;
					write_buf[ pos++ ] = ',';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_time, time_length );
					pos += time_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, unix_timestamp, timestamp_length );
					pos += timestamp_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_ignore, ignore_length );
					pos += ignore_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_ignore_cid, ignore_cid_length );
					pos += ignore_cid_length;

					GlobalFree( utf8_ignore );
					GlobalFree( utf8_ignore_cid );
					GlobalFree( utf8_time );
					GlobalFree( escaped_caller_id );
				}

				di_node = di_node->next;
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_call_log );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char *GetContactEntry( char *entry_start, char *record_end, char **c_entry_value, wchar_t **w_entry_value, bool is_phone_number )
{
	DWORD entry_length = 0;
	char *new_entry_start = NULL;

	if ( entry_start == NULL || record_end == NULL )
	{
		return NULL;
	}

	char *entry_end = _StrChrA( entry_start, '\x1f' );	// Find the end of the entry.
	if ( entry_end == NULL )
	{
		entry_length = ( record_end - entry_start );
	}
	else
	{
		entry_length = ( entry_end - entry_start );
		new_entry_start = entry_end + 1;
	}

	if ( c_entry_value != NULL )
	{
		*c_entry_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( entry_length + 1 ) );
		_memcpy_s( *c_entry_value, entry_length + 1, entry_start, entry_length );
		*( *c_entry_value + entry_length ) = 0;	// Sanity
	}

	if ( w_entry_value != NULL )
	{
		if ( is_phone_number )
		{
			*w_entry_value = FormatPhoneNumber( *c_entry_value );
		}
		else
		{
			int val_length = MultiByteToWideChar( CP_UTF8, 0, *c_entry_value, -1, NULL, 0 );	// Include the NULL terminator.
			*w_entry_value = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
			MultiByteToWideChar( CP_UTF8, 0, *c_entry_value, -1, *w_entry_value, val_length );
		}
	}

	return new_entry_start;
}

char read_contact_list( wchar_t *file_path, dllrbt_tree *list )
{
	char status = 0;

	HANDLE hFile_contact = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_contact != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, last_total = 0;

		char magic_identifier[ 4 ];
		ReadFile( hFile_contact, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_CONTACT_LIST, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_contact, NULL ) - 4;

			char *contact_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be sufficiently large enough to read in the values.

			while ( total_read < fz )
			{
				ReadFile( hFile_contact, contact_buf, sizeof( char ) * 32768, &read, NULL );
				contact_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				total_read += read;

				// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
				// If it's larger than our buffer, then the file is probably invalid/corrupt.
				if ( total_read == last_total )
				{
					break;
				}

				last_total = total_read;

				char *p = contact_buf;
				char *s = contact_buf;

				while ( p != NULL && *s != NULL )
				{
					// Find the newline token.
					p = _StrChrA( s, '\x1e' );

					// If we found one, copy the values.
					if ( p != NULL || ( total_read >= fz ) )
					{
						char *tp = p;

						if ( p != NULL )
						{
							*p = 0;
							++p;
						}
						else	// Handle an EOF.
						{
							tp = ( contact_buf + read );
						}

						contactinfo *ci = ( contactinfo * )GlobalAlloc( GPTR, sizeof( contactinfo ) );

						s = GetContactEntry( s, tp, &ci->contact.cell_phone_number, &ci->cell_phone_number, true );
						s = GetContactEntry( s, tp, &ci->contact.business_name, &ci->business_name, false );
						s = GetContactEntry( s, tp, &ci->contact.department, &ci->department, false );
						s = GetContactEntry( s, tp, &ci->contact.email_address, &ci->email_address, false );
						s = GetContactEntry( s, tp, &ci->contact.fax_number, &ci->fax_number, true );
						s = GetContactEntry( s, tp, &ci->contact.first_name, &ci->first_name, false );
						s = GetContactEntry( s, tp, &ci->contact.home_phone_number, &ci->home_phone_number, true );
						s = GetContactEntry( s, tp, &ci->contact.designation, &ci->designation, false );
						s = GetContactEntry( s, tp, &ci->contact.last_name, &ci->last_name, false );
						s = GetContactEntry( s, tp, &ci->contact.nickname, &ci->nickname, false );
						s = GetContactEntry( s, tp, &ci->contact.office_phone_number, &ci->office_phone_number, true );
						s = GetContactEntry( s, tp, &ci->contact.other_phone_number, &ci->other_phone_number, true );
						s = GetContactEntry( s, tp, &ci->contact.category, &ci->category, false );
						s = GetContactEntry( s, tp, &ci->contact.title, &ci->title, false );
						s = GetContactEntry( s, tp, &ci->contact.web_page, &ci->web_page, false );
						s = GetContactEntry( s, tp, &ci->contact.work_phone_number, &ci->work_phone_number, true );

						char *c_ringtone_file_name = NULL;
						wchar_t *w_ringtone_file_name = NULL;
						s = GetContactEntry( s, tp, &c_ringtone_file_name, &w_ringtone_file_name, false );

						ci->ringtone_info = ( ringtoneinfo * )dllrbt_find( ringtone_list, ( void * )w_ringtone_file_name, true );

						if ( c_ringtone_file_name != NULL )
						{
							GlobalFree( c_ringtone_file_name );
						}

						if ( w_ringtone_file_name != NULL )
						{
							GlobalFree( w_ringtone_file_name );
						}

						char *c_picture_path = NULL;
						s = GetContactEntry( s, tp, &c_picture_path, &ci->picture_path, false );

						if ( c_picture_path != NULL )
						{
							GlobalFree( c_picture_path );
						}

						if ( dllrbt_insert( list, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
						{
							free_contactinfo( &ci );
						}
						else
						{
							add_custom_caller_id( ci );
						}

						// Move to the next value.
						s = p;
					}
					else if ( total_read < fz )	// Reached the end of what we've read.
					{
						total_read -= ( ( contact_buf + read ) - s );
						SetFilePointer( hFile_contact, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
					}
				}
			}

			GlobalFree( contact_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_contact );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char save_contact_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_contact = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_contact != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 28 bytes.

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_CONTACT_LIST, sizeof( char ) * 4 );	// Magic identifier for the ignore phone number list.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( contact_list );
		while ( node != NULL )
		{
			int cell_phone_number_length = 0;
			int business_name_length = 0;
			int department_length = 0;
			int email_address_length = 0;
			int fax_number_length = 0;
			int first_name_length = 0;
			int home_phone_number_length = 0;
			int designation_length = 0;
			int last_name_length = 0;
			int nickname_length = 0;
			int office_phone_number_length = 0;
			int other_phone_number_length = 0;
			int category_length = 0;
			int title_length = 0;
			int web_page_length = 0;
			int work_phone_number_length = 0;

			int ringtone_file_name_length = 0;
			int picture_path_length = 0;

			char *ringtone_file_name = NULL;
			char *picture_path = NULL;

			contactinfo *ci = ( contactinfo * )node->val;

			if ( ci != NULL )
			{
				if ( ci->contact.cell_phone_number != NULL ) { cell_phone_number_length = lstrlenA( ci->contact.cell_phone_number ); }
				if ( ci->contact.business_name != NULL ) { business_name_length = lstrlenA( ci->contact.business_name ); }
				if ( ci->contact.department != NULL ) { department_length = lstrlenA( ci->contact.department ); }
				if ( ci->contact.email_address != NULL ) { email_address_length = lstrlenA( ci->contact.email_address ); }
				if ( ci->contact.fax_number != NULL ) { fax_number_length = lstrlenA( ci->contact.fax_number ); }
				if ( ci->contact.first_name != NULL ) { first_name_length = lstrlenA( ci->contact.first_name ); }
				if ( ci->contact.home_phone_number != NULL ) { home_phone_number_length = lstrlenA( ci->contact.home_phone_number ); }
				if ( ci->contact.designation != NULL ) { designation_length = lstrlenA( ci->contact.designation ); }
				if ( ci->contact.last_name != NULL ) { last_name_length = lstrlenA( ci->contact.last_name ); }
				if ( ci->contact.nickname != NULL ) { nickname_length = lstrlenA( ci->contact.nickname ); }
				if ( ci->contact.office_phone_number != NULL ) { office_phone_number_length = lstrlenA( ci->contact.office_phone_number ); }
				if ( ci->contact.other_phone_number != NULL ) { other_phone_number_length = lstrlenA( ci->contact.other_phone_number ); }
				if ( ci->contact.category != NULL ) { category_length = lstrlenA( ci->contact.category ); }
				if ( ci->contact.title != NULL ) { title_length = lstrlenA( ci->contact.title ); }
				if ( ci->contact.web_page != NULL ) { web_page_length = lstrlenA( ci->contact.web_page ); }
				if ( ci->contact.work_phone_number != NULL ) { work_phone_number_length = lstrlenA( ci->contact.work_phone_number ); }

				if ( ci->ringtone_info != NULL && ci->ringtone_info->ringtone_file != NULL )
				{
					ringtone_file_name_length = WideCharToMultiByte( CP_UTF8, 0, ci->ringtone_info->ringtone_file, -1, NULL, 0, NULL, NULL );
					ringtone_file_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ringtone_file_name_length ); // Size includes the null character.
					ringtone_file_name_length = WideCharToMultiByte( CP_UTF8, 0, ci->ringtone_info->ringtone_file, -1, ringtone_file_name, ringtone_file_name_length, NULL, NULL ) - 1;
				}

				if ( ci->picture_path != NULL )
				{
					picture_path_length = WideCharToMultiByte( CP_UTF8, 0, ci->picture_path, -1, NULL, 0, NULL, NULL );
					picture_path = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * picture_path_length ); // Size includes the null character.
					picture_path_length = WideCharToMultiByte( CP_UTF8, 0, ci->picture_path, -1, picture_path, picture_path_length, NULL, NULL ) - 1;
				}

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + cell_phone_number_length +
						   business_name_length +
						   department_length +
						   email_address_length +
						   fax_number_length +
						   first_name_length +
						   home_phone_number_length +
						   designation_length +
						   last_name_length +
						   nickname_length +
						   office_phone_number_length +
						   other_phone_number_length +
						   category_length +
						   title_length +
						   web_page_length +
						   work_phone_number_length +
						   ringtone_file_name_length +
						   picture_path_length +
						   18 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_contact, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Add to the buffer.
				_memcpy_s( write_buf + pos, size - pos, ci->contact.cell_phone_number, cell_phone_number_length );
				pos += cell_phone_number_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.business_name, business_name_length );
				pos += business_name_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.department, department_length );
				pos += department_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.email_address, email_address_length );
				pos += email_address_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.fax_number, fax_number_length );
				pos += fax_number_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.first_name, first_name_length );
				pos += first_name_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.home_phone_number, home_phone_number_length );
				pos += home_phone_number_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.designation, designation_length );
				pos += designation_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.last_name, last_name_length );
				pos += last_name_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.nickname, nickname_length );
				pos += nickname_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.office_phone_number, office_phone_number_length );
				pos += office_phone_number_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.other_phone_number, other_phone_number_length );
				pos += other_phone_number_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.category, category_length );
				pos += category_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.title, title_length );
				pos += title_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.web_page, web_page_length );
				pos += web_page_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ci->contact.work_phone_number, work_phone_number_length );
				pos += work_phone_number_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, ringtone_file_name, ringtone_file_name_length );
				pos += ringtone_file_name_length;
				write_buf[ pos++ ] = '\x1f';

				_memcpy_s( write_buf + pos, size - pos, picture_path, picture_path_length );
				pos += picture_path_length;
				write_buf[ pos++ ] = '\x1e';
			}

			if ( ringtone_file_name != NULL )
			{
				GlobalFree( ringtone_file_name );
			}

			if ( picture_path != NULL )
			{
				GlobalFree( picture_path );
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_contact, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_contact );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

THREAD_RETURN AutoSave( void *pArguments )	// Saves our call log and lists.
{
	unsigned char return_type = ( unsigned char )pArguments;

	EnterCriticalSection( &auto_save_cs );

	if ( return_type == 0 )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Performing_automatic_save )
	}

	if ( ignore_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_phone_numbers\0", 22 );
		base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

		save_ignore_list( base_directory );
		ignore_list_changed = false;
	}

	if ( ignore_cid_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_caller_id_names\0", 24 );
		base_directory[ base_directory_length + 23 ] = 0;	// Sanity.

		save_ignore_cid_list( base_directory );
		ignore_cid_list_changed = false;
	}

	if ( contact_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\contact_list\0", 14 );
		base_directory[ base_directory_length + 13 ] = 0;	// Sanity.

		save_contact_list( base_directory );
		contact_list_changed = false;
	}

	if ( cfg_enable_call_log_history && call_log_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\call_log_history\0", 18 );
		base_directory[ base_directory_length + 17 ] = 0;	// Sanity.

		save_call_log_history( base_directory );
		call_log_changed = false;
	}

	if ( return_type == 0 )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Automatic_save_has_completed )
	}

	LeaveCriticalSection( &auto_save_cs );

	// If we're calling AutoSave from the main window thread (in WM_ENDSESSION), then we don't want to ExitThread since we're not spawning it as a worker thread.
	if ( return_type == 0 )
	{
		_ExitThread( 0 );
	}

	return 0;
}
