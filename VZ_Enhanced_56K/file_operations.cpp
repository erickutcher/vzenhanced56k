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

#include "file_operations.h"
#include "utilities.h"
#include "message_log_utilities.h"
#include "string_tables.h"

RANGE *allow_range_list[ 16 ];
RANGE *ignore_range_list[ 16 ];

dllrbt_tree *modem_list = NULL;
modem_info *default_modem = NULL;

dllrbt_tree *recording_list = NULL;
ringtone_info *default_recording = NULL;

dllrbt_tree *ringtone_list = NULL;
ringtone_info *default_ringtone = NULL;

dllrbt_tree *allow_list = NULL;
bool allow_list_changed = false;

dllrbt_tree *allow_cid_list = NULL;
bool allow_cid_list_changed = false;

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
			if ( !( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
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

					ringtone_info *rti = ( ringtone_info * )GlobalAlloc( GMEM_FIXED, sizeof( ringtone_info ) );

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
			if ( !( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
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

					ringtone_info *rti = ( ringtone_info * )GlobalAlloc( GMEM_FIXED, sizeof( ringtone_info ) );

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

		int reserved = 1024 - 370;	// There are currently 370 bytes used for settings (not including the strings).

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

				_memcpy_s( &cfg_min_max, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				for ( char i = 0; i < NUM_TABS; ++i )
				{
					_memcpy_s( tab_order[ i ], sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

				for ( char i = 0; i < NUM_COLUMNS1; ++i )
				{
					_memcpy_s( allow_list_columns_width[ i ], sizeof( int ), next, sizeof( int ) );
					next += sizeof( int );

					_memcpy_s( allow_list_columns[ i ], sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

				for ( char i = 0; i < NUM_COLUMNS2; ++i )
				{
					_memcpy_s( allow_cid_list_columns_width[ i ], sizeof( int ), next, sizeof( int ) );
					next += sizeof( int );

					_memcpy_s( allow_cid_list_columns[ i ], sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

				for ( char i = 0; i < NUM_COLUMNS3; ++i )
				{
					_memcpy_s( call_log_columns_width[ i ], sizeof( int ), next, sizeof( int ) );
					next += sizeof( int );

					_memcpy_s( call_log_columns[ i ], sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

				for ( char i = 0; i < NUM_COLUMNS4; ++i )
				{
					_memcpy_s( contact_list_columns_width[ i ], sizeof( int ), next, sizeof( int ) );
					next += sizeof( int );

					_memcpy_s( contact_list_columns[ i ], sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

				for ( char i = 0; i < NUM_COLUMNS5; ++i )
				{
					_memcpy_s( ignore_list_columns_width[ i ], sizeof( int ), next, sizeof( int ) );
					next += sizeof( int );

					_memcpy_s( ignore_list_columns[ i ], sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

				for ( char i = 0; i < NUM_COLUMNS6; ++i )
				{
					_memcpy_s( ignore_cid_list_columns_width[ i ], sizeof( int ), next, sizeof( int ) );
					next += sizeof( int );

					_memcpy_s( ignore_cid_list_columns[ i ], sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

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
				_memcpy_s( &cfg_popup_gradient_direction, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_background_color1, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_background_color2, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );

				_memcpy_s( &cfg_popup_time_format, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_enable_ringtones, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				for ( char i = 0; i < 3; ++i )
				{
					_memcpy_s( &g_popup_info[ i ]->font_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
					next += sizeof( COLORREF );
					_memcpy_s( &g_popup_info[ i ]->font_height, sizeof( LONG ), next, sizeof( LONG ) );
					next += sizeof( LONG );
					_memcpy_s( &g_popup_info[ i ]->font_weight, sizeof( LONG ), next, sizeof( LONG ) );
					next += sizeof( LONG );
					_memcpy_s( &g_popup_info[ i ]->font_italic, sizeof( BYTE ), next, sizeof( BYTE ) );
					next += sizeof( BYTE );
					_memcpy_s( &g_popup_info[ i ]->font_underline, sizeof( BYTE ), next, sizeof( BYTE ) );
					next += sizeof( BYTE );
					_memcpy_s( &g_popup_info[ i ]->font_strikeout, sizeof( BYTE ), next, sizeof( BYTE ) );
					next += sizeof( BYTE );
					_memcpy_s( &g_popup_info[ i ]->font_shadow, sizeof( bool ), next, sizeof( bool ) );
					next += sizeof( bool );
					_memcpy_s( &g_popup_info[ i ]->font_shadow_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
					next += sizeof( COLORREF );
					_memcpy_s( &g_popup_info[ i ]->justify_line, sizeof( unsigned char ), next, sizeof( unsigned char ) );
					next += sizeof( unsigned char );
					_memcpy_s( &g_popup_info[ i ]->line_order, sizeof( char ), next, sizeof( char ) );
					next += sizeof( char );
				}

				//

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

				for ( char i = 0; i < 3; ++i )
				{
					if ( ( DWORD )( next - cfg_buf ) < read )
					{
						string_length = lstrlenA( next ) + 1;

						// Read font face.
						cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
						g_popup_info[ i ]->font_face = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
						MultiByteToWideChar( CP_UTF8, 0, next, string_length, g_popup_info[ i ]->font_face, cfg_val_length );

						next += string_length;
					}
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
				if ( cfg_repeat_recording == 0 || cfg_repeat_recording > 10 ) { cfg_repeat_recording = 1; }
				if ( cfg_drop_call_wait == 0 || cfg_drop_call_wait > 10 ) { cfg_drop_call_wait = 4; }

				CheckColumnOrders( 0, allow_list_columns, NUM_COLUMNS1 );
				CheckColumnOrders( 1, allow_cid_list_columns, NUM_COLUMNS2 );
				CheckColumnOrders( 2, call_log_columns, NUM_COLUMNS3 );
				CheckColumnOrders( 3, contact_list_columns, NUM_COLUMNS4 );
				CheckColumnOrders( 4, ignore_list_columns, NUM_COLUMNS5 );
				CheckColumnOrders( 5, ignore_cid_list_columns, NUM_COLUMNS6 );

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
		int reserved = 1024 - 370; // There are currently 370 bytes used for settings (not including the strings).
		int size = ( sizeof( int ) * 65 ) + ( sizeof( bool ) * 17 ) + ( sizeof( char ) * 75 ) + sizeof( unsigned short ) + sizeof( GUID ) + reserved;
		int pos = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_SETTINGS, sizeof( char ) * 4 );	// Magic identifier for the main program's settings.
		pos += ( sizeof( char ) * 4 );

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

		_memcpy_s( write_buf + pos, size - pos, &cfg_min_max, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		for ( char i = 0; i < NUM_TABS; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, tab_order[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

		for ( char i = 0; i < NUM_COLUMNS1; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, allow_list_columns_width[ i ], sizeof( int ) );
			pos += sizeof( int );

			_memcpy_s( write_buf + pos, size - pos, allow_list_columns[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

		for ( char i = 0; i < NUM_COLUMNS2; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, allow_cid_list_columns_width[ i ], sizeof( int ) );
			pos += sizeof( int );

			_memcpy_s( write_buf + pos, size - pos, allow_cid_list_columns[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

		for ( char i = 0; i < NUM_COLUMNS3; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, call_log_columns_width[ i ], sizeof( int ) );
			pos += sizeof( int );

			_memcpy_s( write_buf + pos, size - pos, call_log_columns[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

		for ( char i = 0; i < NUM_COLUMNS4; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, contact_list_columns_width[ i ], sizeof( int ) );
			pos += sizeof( int );

			_memcpy_s( write_buf + pos, size - pos, contact_list_columns[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

		for ( char i = 0; i < NUM_COLUMNS5; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, ignore_list_columns_width[ i ], sizeof( int ) );
			pos += sizeof( int );

			_memcpy_s( write_buf + pos, size - pos, ignore_list_columns[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

		for ( char i = 0; i < NUM_COLUMNS6; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, ignore_cid_list_columns_width[ i ], sizeof( int ) );
			pos += sizeof( int );

			_memcpy_s( write_buf + pos, size - pos, ignore_cid_list_columns[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

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

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_time_format, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_enable_ringtones, sizeof( bool ) );
		pos += sizeof( bool );

		for ( char i = 0; i < 3; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_color, sizeof( COLORREF ) );
			pos += sizeof( COLORREF );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_height, sizeof( LONG ) );
			pos += sizeof( LONG );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_weight, sizeof( LONG ) );
			pos += sizeof( LONG );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_italic, sizeof( BYTE ) );
			pos += sizeof( BYTE );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_underline, sizeof( BYTE ) );
			pos += sizeof( BYTE );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_strikeout, sizeof( BYTE ) );
			pos += sizeof( BYTE );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_shadow, sizeof( bool ) );
			pos += sizeof( bool );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->font_shadow_color, sizeof( COLORREF ) );
			pos += sizeof( COLORREF );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->justify_line, sizeof( unsigned char ) );
			pos += sizeof( unsigned char );
			_memcpy_s( write_buf + pos, size - pos, &g_popup_info[ i ]->line_order, sizeof( char ) );
			pos += sizeof( char );
		}

		//

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

		for ( char i = 0; i < 3; ++i )
		{
			if ( g_popup_info[ i ]->font_face != NULL )
			{
				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, g_popup_info[ i ]->font_face, -1, NULL, 0, NULL, NULL );
				utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, g_popup_info[ i ]->font_face, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

				WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

				GlobalFree( utf8_cfg_val );
			}
			else
			{
				WriteFile( hFile_cfg, "\0", 1, &write, NULL );
			}
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

void read_old_ignore_list( HANDLE hFile, dllrbt_tree *list )
{
	DWORD read = 0, total_read = 0;
	bool skip_next_newline = false;

	DWORD fz = GetFileSize( hFile, NULL ) - 4;

	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 28 bytes.

	while ( total_read < fz )
	{
		ReadFile( hFile, buf, sizeof( char ) * 32768, &read, NULL );
		buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

		total_read += read;

		char *p = buf;
		char *s = buf;

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
					tp = ( buf + read );
				}

				char *t = _StrChrA( s, '\x1f' );	// Find the ignore count.

				DWORD length1 = 0;
				DWORD length2 = 0;

				if ( t == NULL )
				{
					length1 = ( DWORD )( tp - s );
				}
				else
				{
					length1 = ( DWORD )( t - s );
					++t;
					length2 = ( DWORD )( ( tp - t ) > 10 ? 10 : ( tp - t ) );
				}

				// Make sure the number is at most 15 + 1 digits.
				if ( ( length1 <= 16 && length1 > 0 ) && ( length2 <= 10 && length2 >= 0 ) )
				{
					allow_ignore_info *aii = ( allow_ignore_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_info ) );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, s, length1, NULL, 0 );	// Include the NULL terminator.
					aii->phone_number = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( val_length + 1 ) );
					MultiByteToWideChar( CP_UTF8, 0, s, length1, aii->phone_number, val_length );
					aii->phone_number[ val_length ] = 0;	// Sanity.

					aii->w_phone_number = FormatPhoneNumberW( aii->phone_number );

					if ( length2 == 0 )
					{
						aii->count = 0;
					}
					else
					{
						char tmp = t[ length2 ];
						t[ length2 ] = 0;
						aii->count = _strtoul( t, NULL, 10 );
						t[ length2 ] = tmp;
					}

					aii->state = 0;

					if ( dllrbt_insert( list, ( void * )aii->phone_number, ( void * )aii ) != DLLRBT_STATUS_OK )
					{
						free_allowignoreinfo( &aii );
					}
					else if ( is_num_w( aii->phone_number ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
					{
						RangeAdd( &ignore_range_list[ length1 - 1 ], aii->phone_number, length1 );
					}
				}

				// Move to the next value.
				s = p;
			}
			else if ( total_read < fz )	// Reached the end of what we've read.
			{
				// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid number.
				DWORD offset = ( DWORD )( ( buf + read ) - s );

				char *t = _StrChrA( s, '\x1f' );	// Find the ignore count.

				DWORD length1 = 0;
				DWORD length2 = 0;

				if ( t == NULL )
				{
					length1 = offset;
				}
				else
				{
					length1 = ( DWORD )( t - s );
					++t;
					length2 = ( DWORD )( ( ( buf + read ) - t ) > 10 ? 10 : ( ( buf + read ) - t ) );
				}

				// Max recommended number length is 15 + 1.
				// We can only offset back less than what we've read, and no more than 27 digits (bytes). 15 + 1 + 1 + 10
				// This means read, and our buffer size should be greater than 27.
				// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 28.
				if ( offset < read && length1 <= 16 && length2 <= 10 )
				{
					total_read -= offset;
					SetFilePointer( hFile, total_read, NULL, FILE_BEGIN );
				}
				else	// The length of the next number is too long.
				{
					skip_next_newline = true;
				}
			}
		}
	}

	GlobalFree( buf );
}

char read_allow_ignore_list( wchar_t *file_path, dllrbt_tree *list, unsigned char list_type )
{
	char ret_status = 0;

	HANDLE hFile = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		ULARGE_INTEGER	last_called;
		unsigned int	call_total;
		wchar_t			*phone_number;

		char magic_identifier[ 4 ];
		ReadFile( hFile, magic_identifier, sizeof( char ) * 4, &read, NULL );

		char version = -1;

		if ( read == 4 )
		{
			if ( list_type == LIST_TYPE_ALLOW )
			{
				if ( _memcmp( magic_identifier, MAGIC_ID_ALLOW_PN, 4 ) == 0 )
				{
					version = 1;
				}
			}
			else if ( list_type == LIST_TYPE_IGNORE )
			{
				if ( _memcmp( magic_identifier, MAGIC_ID_IGNORE_PN, 4 ) == 0 )
				{
					version = 1;
				}
				else if ( _memcmp( magic_identifier, OLD_MAGIC_ID_IGNORE_PN, 4 ) == 0 )
				{
					version = 0;
				}
			}
		}

		if ( version == 1 )
		{
			DWORD fz = GetFileSize( hFile, NULL ) - 4;

			char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

			while ( total_read < fz )
			{
				ReadFile( hFile, buf, sizeof( char ) * 524288, &read, NULL );

				buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				// Make sure that we have at least part of the entry. This is the minimum size an entry could be.
				// Include last called, call total, and phone number NULL terminator.
				if ( read < ( sizeof( ULONGLONG ) + sizeof( unsigned int ) + sizeof( char ) ) )
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

				p = buf;
				offset = last_entry = 0;

				while ( offset < read )
				{
					phone_number = NULL;

					// Call Total
					offset += sizeof( unsigned int );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &call_total, sizeof( unsigned int ), p, sizeof( unsigned int ) );
					p += sizeof( unsigned int );

					// Last Called.
					offset += sizeof( ULONGLONG );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &last_called.QuadPart, sizeof( ULONGLONG ), p, sizeof( ULONGLONG ) );
					p += sizeof( ULONGLONG );

					// Phone Number
					int phone_number_length = lstrlenW( ( wchar_t * )p ) + 1;

					// Let's not allocate an empty string.
					if ( phone_number_length > 1 )
					{
						offset += ( phone_number_length * sizeof( wchar_t ) );
						if ( offset > read ) { goto CLEANUP; }

						phone_number = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * phone_number_length );
						_memcpy_s( phone_number, sizeof( wchar_t ) * phone_number_length, p, sizeof( wchar_t ) * phone_number_length );
						*( phone_number + ( phone_number_length - 1 ) ) = 0;	// Sanity
					}

					p += ( phone_number_length * sizeof( wchar_t ) );

					last_entry = offset;	// This value is the ending offset of the last valid entry.

					// Create our list entry.

					allow_ignore_info *aii = ( allow_ignore_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_info ) );

					aii->last_called.QuadPart = last_called.QuadPart;

					if ( last_called.QuadPart > 0 )
					{
						aii->w_last_called = FormatTimestamp( aii->last_called );
					}

					aii->phone_number = phone_number;

					aii->w_phone_number = FormatPhoneNumberW( aii->phone_number );

					aii->count = call_total;

					if ( dllrbt_insert( list, ( void * )aii->phone_number, ( void * )aii ) != DLLRBT_STATUS_OK )
					{
						free_allowignoreinfo( &aii );
					}
					else if ( is_num_w( aii->phone_number ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
					{
						if ( phone_number_length > 1 )
						{
							if ( list_type == LIST_TYPE_ALLOW )
							{
								RangeAdd( &allow_range_list[ phone_number_length - 2 ], aii->phone_number, phone_number_length - 1 );
							}
							else
							{
								RangeAdd( &ignore_range_list[ phone_number_length - 2 ], aii->phone_number, phone_number_length - 1 );
							}
						}
					}

					continue;

	CLEANUP:
					GlobalFree( phone_number );

					// Go back to the last valid entry.
					if ( total_read < fz )
					{
						total_read -= ( read - last_entry );
						SetFilePointer( hFile, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
					}

					break;
				}
			}

			GlobalFree( buf );
		}
		else if ( version == 0 )
		{
			read_old_ignore_list( hFile, list );
		}
		else
		{
			ret_status = -2;	// Bad file format.
		}

		CloseHandle( hFile );	
	}
	else
	{
		ret_status = -1;	// Can't open file for reading.
	}

	return ret_status;
}

void read_old_ignore_cid_list( HANDLE hFile, dllrbt_tree *list )
{
	DWORD read = 0, total_read = 0;
	bool skip_next_newline = false;

	DWORD fz = GetFileSize( hFile, NULL ) - 4;

	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 29 bytes.

	while ( total_read < fz )
	{
		ReadFile( hFile, buf, sizeof( char ) * 32768, &read, NULL );
		buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

		total_read += read;

		char *p = buf;
		char *s = buf;

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
					tp = ( buf + read );
				}

				char *t = _StrChrA( s, '\x1f' );	// Find the match values.

				if ( t != NULL && ( ( tp - t ) >= 2 ) )
				{
					DWORD length1 = ( DWORD )( t - s );
					DWORD length2 = 0;

					/*bool regular_expression = ( t[ 1 ] & 0x04 ? true : false );
					bool match_case = ( t[ 1 ] & 0x02 ? true : false );
					bool match_whole_word = ( t[ 1 ] & 0x01 ? true : false );*/
					unsigned char match_flag = t[ 1 ];

					t = _StrChrA( t + 2, '\x1f' );

					if ( t != NULL )
					{
						++t;
						length2 = ( DWORD )( ( tp - t ) > 10 ? 10 : ( tp - t ) );
					}

					// Make sure the number is at most 15 digits.
					if ( ( length1 <= 15 && length1 > 0 ) && ( length2 <= 10 && length2 >= 0 ) )
					{
						allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_info ) );

						int val_length = MultiByteToWideChar( CP_UTF8, 0, s, length1, NULL, 0 ) + 1;	// Include the NULL terminator.
						aicidi->caller_id = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, s, length1, aicidi->caller_id, val_length );
						aicidi->caller_id[ val_length - 1 ] = 0;	// Sanity.

						if ( length2 == 0 )
						{
							aicidi->count = 0;
						}
						else
						{
							char tmp = t[ length2 ];
							t[ length2 ] = 0;
							aicidi->count = _strtoul( t, NULL, 10 );
							t[ length2 ] = tmp;
						}

						aicidi->match_flag = match_flag;

						aicidi->state = 0;
						aicidi->active = false;

						if ( dllrbt_insert( list, ( void * )aicidi, ( void * )aicidi ) != DLLRBT_STATUS_OK )
						{
							free_allowignorecidinfo( &aicidi );
						}
					}
				}

				// Move to the next value.
				s = p;
			}
			else if ( total_read < fz )	// Reached the end of what we've read.
			{
				// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid caller ID.
				DWORD offset = ( DWORD )( ( buf + read ) - s );

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
					length1 = ( DWORD )( t - s );

					s = t + 1;

					t = _StrChrA( s, '\x1f' );	// Find the total count.

					if ( t == NULL )
					{
						length2 = ( DWORD )( ( buf + read ) - s );
					}
					else
					{
						length2 = ( DWORD )( t - s );
						++t;
						length3 = ( DWORD )( ( ( buf + read ) - t ) > 10 ? 10 : ( ( buf + read ) - t ) );
					}
				}

				// Max recommended caller ID length is 15.
				// We can only offset back less than what we've read, and no more than 28 bytes. 15 + 1 + 1 + 1 + 10.
				// This means read, and our buffer size should be greater than 28.
				// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 29.
				if ( offset < read && length1 <= 15 && length2 <= 2 && length3 <= 10 )
				{
					total_read -= offset;
					SetFilePointer( hFile, total_read, NULL, FILE_BEGIN );
				}
				else	// The length of the next caller ID is too long.
				{
					skip_next_newline = true;
				}
			}
		}
	}
}

char read_allow_ignore_cid_list( wchar_t *file_path, dllrbt_tree *list, unsigned char list_type )
{
	char ret_status = 0;

	HANDLE hFile = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		ULARGE_INTEGER	last_called;
		unsigned int	call_total;
		unsigned char	match_flag;
		wchar_t			*search_string;

		char magic_identifier[ 4 ];
		ReadFile( hFile, magic_identifier, sizeof( char ) * 4, &read, NULL );

		char version = -1;

		if ( read == 4 )
		{
			if ( list_type == LIST_TYPE_ALLOW )
			{
				if ( _memcmp( magic_identifier, MAGIC_ID_ALLOW_CNAM, 4 ) == 0 )
				{
					version = 1;
				}
			}
			else if ( list_type == LIST_TYPE_IGNORE )
			{
				if ( _memcmp( magic_identifier, MAGIC_ID_IGNORE_CNAM, 4 ) == 0 )
				{
					version = 1;
				}
				else if ( _memcmp( magic_identifier, OLD_MAGIC_ID_IGNORE_CNAM, 4 ) == 0 )
				{
					version = 0;
				}
			}
		}

		if ( version == 1 )
		{
			DWORD fz = GetFileSize( hFile, NULL ) - 4;

			char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

			while ( total_read < fz )
			{
				ReadFile( hFile, buf, sizeof( char ) * 524288, &read, NULL );

				buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				// Make sure that we have at least part of the entry. This is the minimum size an entry could be.
				// Include last called, call total, match flag, and search string NULL terminator.
				if ( read < ( sizeof( ULONGLONG ) + sizeof( unsigned int ) + sizeof( unsigned char ) + sizeof( char ) ) )
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

				p = buf;
				offset = last_entry = 0;

				while ( offset < read )
				{
					search_string = NULL;

					// Match Flag
					offset += sizeof( unsigned char );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &match_flag, sizeof( unsigned char ), p, sizeof( unsigned char ) );
					p += sizeof( unsigned char );

					// Call Total
					offset += sizeof( unsigned int );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &call_total, sizeof( unsigned int ), p, sizeof( unsigned int ) );
					p += sizeof( unsigned int );

					// Last Called.
					offset += sizeof( ULONGLONG );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &last_called.QuadPart, sizeof( ULONGLONG ), p, sizeof( ULONGLONG ) );
					p += sizeof( ULONGLONG );

					// Search String
					int string_length = lstrlenW( ( wchar_t * )p ) + 1;

					offset += ( string_length * sizeof( wchar_t ) );
					if ( offset > read ) { goto CLEANUP; }

					// Let's not allocate an empty string.
					if ( string_length > 1 )
					{
						search_string = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
						_memcpy_s( search_string, sizeof( wchar_t ) * string_length, p, sizeof( wchar_t ) * string_length );
						*( search_string + ( string_length - 1 ) ) = 0;	// Sanity
					}

					p += ( string_length * sizeof( wchar_t ) );

					last_entry = offset;	// This value is the ending offset of the last valid entry.

					// Create our list entry.

					allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )GlobalAlloc( GPTR, sizeof( allow_ignore_cid_info ) );

					aicidi->last_called.QuadPart = last_called.QuadPart;

					if ( last_called.QuadPart > 0 )
					{
						aicidi->w_last_called = FormatTimestamp( aicidi->last_called );
					}

					aicidi->caller_id = search_string;

					aicidi->count = call_total;

					aicidi->match_flag = match_flag;

					if ( dllrbt_insert( list, ( void * )aicidi, ( void * )aicidi ) != DLLRBT_STATUS_OK )
					{
						free_allowignorecidinfo( &aicidi );
					}

					continue;

	CLEANUP:
					GlobalFree( search_string );

					// Go back to the last valid entry.
					if ( total_read < fz )
					{
						total_read -= ( read - last_entry );
						SetFilePointer( hFile, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
					}

					break;
				}
			}

			GlobalFree( buf );
		}
		else if ( version == 0 )
		{
			read_old_ignore_cid_list( hFile, list );
		}
		else
		{
			ret_status = -2;	// Bad file format.
		}

		CloseHandle( hFile );	
	}
	else
	{
		ret_status = -1;	// Can't open file for reading.
	}

	return ret_status;
}

char save_allow_ignore_list( wchar_t *file_path, dllrbt_tree *list, unsigned char list_type )
{
	char ret_status = 0;

	HANDLE hFile = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, ( list_type == LIST_TYPE_ALLOW ? MAGIC_ID_ALLOW_PN : MAGIC_ID_IGNORE_PN ), sizeof( char ) * 4 );	// Magic identifier for the call log history.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( list );
		while ( node != NULL )
		{
			allow_ignore_info *aii = ( allow_ignore_info * )node->val;

			if ( aii != NULL )
			{
				int phone_number_length = ( lstrlenW( aii->phone_number ) + 1 ) * sizeof( wchar_t );

				// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
				if ( ( signed )( pos + phone_number_length + sizeof( ULONGLONG ) + sizeof( unsigned int ) ) > size )
				{
					// Dump the buffer.
					WriteFile( hFile, buf, pos, &write, NULL );
					pos = 0;
				}

				_memcpy_s( buf + pos, size - pos, &aii->count, sizeof( unsigned int ) );
				pos += sizeof( unsigned int );

				_memcpy_s( buf + pos, size - pos, &aii->last_called, sizeof( ULONGLONG ) );
				pos += sizeof( ULONGLONG );

				_memcpy_s( buf + pos, size - pos, aii->phone_number, phone_number_length );
				pos += phone_number_length;
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
		}

		GlobalFree( buf );

		CloseHandle( hFile );
	}
	else
	{
		ret_status = -1;	// Can't open file for writing.
	}

	return ret_status;
}

char save_allow_ignore_cid_list( wchar_t *file_path, dllrbt_tree *list, unsigned char list_type )
{
	char ret_status = 0;

	HANDLE hFile = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, ( list_type == LIST_TYPE_ALLOW ? MAGIC_ID_ALLOW_CNAM : MAGIC_ID_IGNORE_CNAM ), sizeof( char ) * 4 );	// Magic identifier for the call log history.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( list );
		while ( node != NULL )
		{
			allow_ignore_cid_info *aicidi = ( allow_ignore_cid_info * )node->val;

			if ( aicidi != NULL )
			{
				int caller_id_length = ( lstrlenW( aicidi->caller_id ) + 1 ) * sizeof( wchar_t );

				// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
				if ( ( signed )( pos + caller_id_length + sizeof( ULONGLONG ) + sizeof( unsigned int ) + sizeof( unsigned char ) ) > size )
				{
					// Dump the buffer.
					WriteFile( hFile, buf, pos, &write, NULL );
					pos = 0;
				}

				_memcpy_s( buf + pos, size - pos, &aicidi->match_flag, sizeof( unsigned char ) );
				pos += sizeof( unsigned char );

				_memcpy_s( buf + pos, size - pos, &aicidi->count, sizeof( unsigned int ) );
				pos += sizeof( unsigned int );

				_memcpy_s( buf + pos, size - pos, &aicidi->last_called, sizeof( ULONGLONG ) );
				pos += sizeof( ULONGLONG );

				_memcpy_s( buf + pos, size - pos, aicidi->caller_id, caller_id_length );
				pos += caller_id_length;
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
		}

		GlobalFree( buf );

		CloseHandle( hFile );
	}
	else
	{
		ret_status = -1;	// Can't open file for writing.
	}

	return ret_status;
}

char read_call_log_history( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		bool ignored = false;
		ULONGLONG time = 0;
		wchar_t *caller_id = NULL;
		wchar_t *custom_caller_id = NULL;
		wchar_t *phone_number = NULL;

		wchar_t range_number[ 32 ];

		char magic_identifier[ 4 ];
		ReadFile( hFile, magic_identifier, sizeof( char ) * 4, &read, NULL );

		char version = -1;

		if ( read == 4 )
		{
			if ( _memcmp( magic_identifier, MAGIC_ID_CALL_LOG, 4 ) == 0 )
			{
				version = 1;
			}
			else if ( _memcmp( magic_identifier, OLD_MAGIC_ID_CALL_LOG, 4 ) == 0 )
			{
				version = 0;
			}
		}

		if ( version != -1 )
		{
			DWORD fz = GetFileSize( hFile, NULL ) - 4;

			char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

			while ( total_read < fz )
			{
				ReadFile( hFile, buf, sizeof( char ) * 524288, &read, NULL );

				buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

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

				p = buf;
				offset = last_entry = 0;

				while ( offset < read )
				{
					caller_id = NULL;
					custom_caller_id = NULL;
					phone_number = NULL;

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

					int phone_number_length;

					if ( version == 0 )
					{
						// Caller ID
						int string_length = lstrlenA( p );

						offset += ( string_length + 1 );
						if ( offset >= read ) { goto CLEANUP; }

						int val_length = MultiByteToWideChar( CP_UTF8, 0, p, string_length, NULL, 0 ) + 1;	// Include the NULL terminator.
						caller_id = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, p, string_length, caller_id, val_length );
						caller_id[ val_length - 1 ] = 0;	// Sanity.

						p += ( string_length + 1 );

						// Incoming number
						string_length = lstrlenA( p );

						offset += ( string_length + 1 );
						if ( offset > read ) { goto CLEANUP; }	// Offset must equal read for the last string in the file. It's truncated/corrupt if it doesn't.

						phone_number_length = min( string_length, 16 );
						int adjusted_size = phone_number_length + 1;

						val_length = MultiByteToWideChar( CP_UTF8, 0, p, phone_number_length, NULL, 0 ) + 1;	// Include the NULL terminator.
						phone_number = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, p, phone_number_length, phone_number, val_length );
						phone_number[ val_length - 1 ] = 0;	// Sanity.

						p += ( string_length + 1 );
					}
					else
					{
						// Caller ID
						int string_length = lstrlenW( ( wchar_t * )p ) + 1;

						offset += ( string_length * sizeof( wchar_t ) );
						if ( offset >= read ) { goto CLEANUP; }

						caller_id = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
						_memcpy_s( caller_id, string_length * sizeof( wchar_t ), p, string_length * sizeof( wchar_t ) );
						*( caller_id + ( string_length - 1 ) ) = 0;	// Sanity

						p += ( string_length * sizeof( wchar_t ) );

						// Custom Caller ID
						string_length = lstrlenW( ( wchar_t * )p ) + 1;

						offset += ( string_length * sizeof( wchar_t ) );
						if ( offset >= read ) { goto CLEANUP; }

						// Let's not allocate an empty string.
						if ( string_length > 1 )
						{
							custom_caller_id = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_memcpy_s( custom_caller_id, string_length * sizeof( wchar_t ), p, string_length * sizeof( wchar_t ) );
							*( custom_caller_id + ( string_length - 1 ) ) = 0;	// Sanity
						}

						p += ( string_length * sizeof( wchar_t ) );

						// Incoming number
						string_length = lstrlenW( ( wchar_t * )p );

						offset += ( ( string_length + 1 ) * sizeof( wchar_t ) );
						if ( offset > read ) { goto CLEANUP; }	// Offset must equal read for the last string in the file. It's truncated/corrupt if it doesn't.

						phone_number_length = min( string_length, 16 );
						int adjusted_size = phone_number_length + 1;

						phone_number = ( wchar_t * )GlobalAlloc( GMEM_FIXED, adjusted_size * sizeof( wchar_t ) );
						_memcpy_s( phone_number, adjusted_size * sizeof( wchar_t ), p, adjusted_size * sizeof( wchar_t ) );
						*( phone_number + ( adjusted_size - 1 ) ) = 0;	// Sanity

						p += ( ( string_length + 1 ) * sizeof( wchar_t ) );
					}

					last_entry = offset;	// This value is the ending offset of the last valid entry.

					display_info *di = ( display_info * )GlobalAlloc( GPTR, sizeof( display_info ) );

					di->phone_number = phone_number;
					di->caller_id = caller_id;
					di->custom_caller_id = custom_caller_id;
					di->ignored = ignored;
					di->time.QuadPart = time;

					// Create the node to insert into a linked list.
					DoublyLinkedList *di_node = DLL_CreateNode( ( void * )di );

					// See if our tree has the phone number to add the node to.
					DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->phone_number, true );
					if ( dll == NULL )
					{
						// If no phone number exits, insert the node into the tree.
						if ( dllrbt_insert( call_log, ( void * )di->phone_number, ( void * )di_node ) != DLLRBT_STATUS_OK )
						{
							GlobalFree( di_node );	// This shouldn't happen.
						}
					}
					else	// If a phone number exits, insert the node into the linked list.
					{
						DLL_AddNode( &dll, di_node, -1 );	// Insert at the end of the doubly linked list.
					}

					// Search the allow_list for a match.
					allow_ignore_info *aii = ( allow_ignore_info * )dllrbt_find( allow_list, ( void * )di->phone_number, true );

					// Try searching the range list.
					if ( aii == NULL )
					{
						_memzero( range_number, 32 * sizeof( wchar_t ) );

						int range_index = phone_number_length;
						range_index = ( range_index > 0 ? range_index - 1 : 0 );

						if ( RangeSearch( &allow_range_list[ range_index ], di->phone_number, range_number ) )
						{
							aii = ( allow_ignore_info * )dllrbt_find( allow_list, ( void * )range_number, true );
						}
					}

					if ( aii != NULL )
					{
						di->allow_phone_number = true;
					}

					// Search the ignore_list for a match.
					aii = ( allow_ignore_info * )dllrbt_find( ignore_list, ( void * )di->phone_number, true );

					// Try searching the range list.
					if ( aii == NULL )
					{
						_memzero( range_number, 32 * sizeof( wchar_t ) );

						int range_index = phone_number_length;
						range_index = ( range_index > 0 ? range_index - 1 : 0 );

						if ( RangeSearch( &ignore_range_list[ range_index ], di->phone_number, range_number ) )
						{
							aii = ( allow_ignore_info * )dllrbt_find( ignore_list, ( void * )range_number, true );
						}
					}

					if ( aii != NULL )
					{
						di->ignore_phone_number = true;
					}

					// Search for the first allow/ignore caller ID list match. di->allow/ignore_cid_match_count will be updated here for all keyword matches.
					find_caller_id_name_match( di, allow_cid_list, LIST_TYPE_ALLOW );
					find_caller_id_name_match( di, ignore_cid_list, LIST_TYPE_IGNORE );

					di->w_phone_number = FormatPhoneNumberW( di->phone_number );

					if ( di->time.QuadPart > 0 )
					{
						di->w_time = FormatTimestamp( di->time );
					}

					_SendNotifyMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 1 ), ( LPARAM )di );	// Add entry to listview and don't show popup.

					continue;

	CLEANUP:
					GlobalFree( custom_caller_id );
					GlobalFree( caller_id );
					GlobalFree( phone_number );

					// Go back to the last valid entry.
					if ( total_read < fz )
					{
						total_read -= ( read - last_entry );
						SetFilePointer( hFile, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
					}

					break;
				}
			}

			GlobalFree( buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile );	
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

		int item_count = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );

		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		for ( lvi.iItem = 0; lvi.iItem < item_count; ++lvi.iItem )
		{
			_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

			display_info *di = ( display_info * )lvi.lParam;

			// lstrlen is safe for NULL values.
			int caller_id_length = ( lstrlenW( di->caller_id ) + 1 ) * sizeof( wchar_t );
			int custom_caller_id_length = ( lstrlenW( di->custom_caller_id ) + 1 ) * sizeof( wchar_t );
			int phone_number_length = ( lstrlenW( di->phone_number ) + 1 ) * sizeof( wchar_t );

			// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
			if ( ( signed )( pos + phone_number_length + caller_id_length + custom_caller_id_length + sizeof( ULONGLONG ) + sizeof( bool ) ) > size )
			{
				// Dump the buffer.
				WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
				pos = 0;
			}

			_memcpy_s( write_buf + pos, size - pos, &di->ignored, sizeof( bool ) );
			pos += sizeof( bool );

			_memcpy_s( write_buf + pos, size - pos, &di->time.QuadPart, sizeof( ULONGLONG ) );
			pos += sizeof( ULONGLONG );

			_memcpy_s( write_buf + pos, size - pos, di->caller_id, caller_id_length );
			pos += caller_id_length;

			_memcpy_s( write_buf + pos, size - pos, SAFESTRW( di->custom_caller_id ), custom_caller_id_length );
			pos += custom_caller_id_length;

			_memcpy_s( write_buf + pos, size - pos, di->phone_number, phone_number_length );
			pos += phone_number_length;
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
		WriteFile( hFile_call_log, "\xEF\xBB\xBF\"Caller ID Name\",\"Phone Number\",\"Date and Time\",\"Unix Timestamp\",\"Allow Phone Number\",\"Allow Caller ID Name\",\"Ignore Phone Number\",\"Ignore Caller ID Name\"", 157, &write, NULL );

		node_type *node = dllrbt_get_head( call_log );
		while ( node != NULL )
		{
			DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
			while ( di_node != NULL )
			{
				display_info *di = ( display_info * )di_node->data;

				if ( di != NULL )
				{
					// lstrlen is safe for NULL values.

					int phone_number_length = WideCharToMultiByte( CP_UTF8, 0, di->phone_number, -1, NULL, 0, NULL, NULL );
					char *utf8_phone_number = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * phone_number_length ); // Size includes the null character.
					phone_number_length = WideCharToMultiByte( CP_UTF8, 0, di->phone_number, -1, utf8_phone_number, phone_number_length, NULL, NULL ) - 1;

					wchar_t *w_allow = ( di->allow_phone_number ? ST_Yes : ST_No );
					wchar_t *w_allow_cid = ( di->allow_cid_match_count > 0 ? ST_Yes : ST_No );

					int allow_length = WideCharToMultiByte( CP_UTF8, 0, w_allow, -1, NULL, 0, NULL, NULL );
					char *utf8_allow = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * allow_length ); // Size includes the null character.
					allow_length = WideCharToMultiByte( CP_UTF8, 0, w_allow, -1, utf8_allow, allow_length, NULL, NULL ) - 1;

					int allow_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_allow_cid, -1, NULL, 0, NULL, NULL );
					char *utf8_allow_cid = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * allow_cid_length ); // Size includes the null character.
					allow_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_allow_cid, -1, utf8_allow_cid, allow_cid_length, NULL, NULL ) - 1;

					wchar_t *w_ignore = ( di->ignore_phone_number ? ST_Yes : ST_No );
					wchar_t *w_ignore_cid = ( di->ignore_cid_match_count > 0 ? ST_Yes : ST_No );

					int ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, NULL, 0, NULL, NULL );
					char *utf8_ignore = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ignore_length ); // Size includes the null character.
					ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, utf8_ignore, ignore_length, NULL, NULL ) - 1;

					int ignore_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore_cid, -1, NULL, 0, NULL, NULL );
					char *utf8_ignore_cid = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ignore_cid_length ); // Size includes the null character.
					ignore_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore_cid, -1, utf8_ignore_cid, ignore_cid_length, NULL, NULL ) - 1;

					int custom_caller_id_length = 0;
					int caller_id_length = WideCharToMultiByte( CP_UTF8, 0, di->caller_id, -1, NULL, 0, NULL, NULL );
					if ( di->custom_caller_id != NULL )
					{
						++caller_id_length;
						custom_caller_id_length = WideCharToMultiByte( CP_UTF8, 0, di->custom_caller_id, -1, NULL, 0, NULL, NULL ) + 1;
					}

					char *utf8_caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( caller_id_length + custom_caller_id_length ) ); // Size includes the null character.
					caller_id_length = WideCharToMultiByte( CP_UTF8, 0, di->caller_id, -1, utf8_caller_id + custom_caller_id_length, caller_id_length, NULL, NULL ) - 1;
					if ( di->custom_caller_id != NULL )
					{
						custom_caller_id_length = WideCharToMultiByte( CP_UTF8, 0, di->custom_caller_id, -1, utf8_caller_id, custom_caller_id_length, NULL, NULL ) - 1;
						utf8_caller_id[ custom_caller_id_length++ ] = L' ';
						utf8_caller_id[ custom_caller_id_length++ ] = L'(';
						caller_id_length += custom_caller_id_length;
						utf8_caller_id[ caller_id_length++ ] = L')';
						utf8_caller_id[ caller_id_length ] = 0;	// Sanity.
					}

					char *escaped_caller_id = escape_csv( utf8_caller_id );
					if ( escaped_caller_id != NULL )
					{
						caller_id_length = lstrlenA( escaped_caller_id );
					}

					int time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, NULL, 0, NULL, NULL );
					char *utf8_time = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * time_length ); // Size includes the null character.
					time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, utf8_time, time_length, NULL, NULL ) - 1;

					// Convert the time into a 32bit Unix timestamp.
					ULARGE_INTEGER date;
					date.HighPart = di->time.HighPart;
					date.LowPart = di->time.LowPart;

					date.QuadPart -= ( 11644473600000 * 10000 );
#ifdef _WIN64
					date.QuadPart /= FILETIME_TICKS_PER_SECOND;
#else
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
#endif
					int timestamp_length = __snprintf( unix_timestamp, 21, "%llu", date.QuadPart );

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( pos + phone_number_length + caller_id_length + time_length + timestamp_length + allow_length + allow_cid_length + ignore_length + ignore_cid_length + 13 > size )
					{
						// Dump the buffer.
						WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
						pos = 0;
					}

					// Add to the buffer.
					write_buf[ pos++ ] = '\r';
					write_buf[ pos++ ] = '\n';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, ( escaped_caller_id != NULL ? escaped_caller_id : utf8_caller_id ), caller_id_length );
					pos += caller_id_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_phone_number, phone_number_length );
					pos += phone_number_length;
					write_buf[ pos++ ] = ',';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_time, time_length );
					pos += time_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, unix_timestamp, timestamp_length );
					pos += timestamp_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_allow, allow_length );
					pos += allow_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_allow_cid, allow_cid_length );
					pos += allow_cid_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_ignore, ignore_length );
					pos += ignore_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_ignore_cid, ignore_cid_length );
					pos += ignore_cid_length;

					GlobalFree( utf8_allow );
					GlobalFree( utf8_allow_cid );
					GlobalFree( utf8_ignore );
					GlobalFree( utf8_ignore_cid );
					GlobalFree( utf8_time );
					GlobalFree( utf8_phone_number );
					GlobalFree( utf8_caller_id );
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

char *GetContactEntry( char *entry_start, char *record_end, wchar_t **w_entry_value )
{
	DWORD entry_length = 0;
	char *new_entry_start = NULL;

	if ( entry_start == NULL || record_end == NULL || w_entry_value == NULL )
	{
		return NULL;
	}

	char *entry_end = _StrChrA( entry_start, '\x1f' );	// Find the end of the entry.
	if ( entry_end == NULL )
	{
		entry_length = ( DWORD )( record_end - entry_start );
	}
	else
	{
		entry_length = ( DWORD )( entry_end - entry_start );
		new_entry_start = entry_end + 1;
	}

	if ( entry_length > 0 )
	{
		int val_length = MultiByteToWideChar( CP_UTF8, 0, entry_start, entry_length, NULL, 0 ) + 1;	// Include the NULL terminator.
		*w_entry_value = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
		MultiByteToWideChar( CP_UTF8, 0, entry_start, entry_length, *w_entry_value, val_length );
		*( *w_entry_value + ( val_length - 1 ) ) = 0;	// Sanity.
	}

	return new_entry_start;
}

void read_old_contact_list( HANDLE hFile, dllrbt_tree *list )
{
	DWORD read = 0, total_read = 0, last_total = 0;

	DWORD fz = GetFileSize( hFile, NULL ) - 4;

	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be sufficiently large enough to read in the values.

	while ( total_read < fz )
	{
		ReadFile( hFile, buf, sizeof( char ) * 32768, &read, NULL );
		buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

		total_read += read;

		// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
		// If it's larger than our buffer, then the file is probably invalid/corrupt.
		if ( total_read == last_total )
		{
			break;
		}

		last_total = total_read;

		char *p = buf;
		char *s = buf;

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
					tp = ( buf + read );
				}

				contact_info *ci = ( contact_info * )GlobalAlloc( GPTR, sizeof( contact_info ) );

				for ( char i = 0, j = 0; i < 16; ++i )
				{
					switch ( i )
					{
						case 0:
						case 4:
						case 6:
						case 10:
						case 11:
						case 15:
						{
							s = GetContactEntry( s, tp, &ci->w_contact_info_phone_numbers[ j ] );
							ci->w_contact_info_values[ i ] = FormatPhoneNumberW( ci->w_contact_info_phone_numbers[ j ] );
							++j;
						}
						break;

						default:
						{
							s = GetContactEntry( s, tp, &ci->w_contact_info_values[ i ] );
						}
						break;
					}
				}

				wchar_t *w_ringtone_file_name = NULL;
				s = GetContactEntry( s, tp, &w_ringtone_file_name );

				ci->rti = ( ringtone_info * )dllrbt_find( ringtone_list, ( void * )w_ringtone_file_name, true );

				if ( w_ringtone_file_name != NULL )
				{
					GlobalFree( w_ringtone_file_name );
				}

				s = GetContactEntry( s, tp, &ci->picture_path );

				if ( dllrbt_insert( list, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
				{
					free_contactinfo( &ci );
				}
				else
				{
					add_custom_caller_id_w( ci );
				}

				// Move to the next value.
				s = p;
			}
			else if ( total_read < fz )	// Reached the end of what we've read.
			{
				total_read -= ( DWORD )( ( buf + read ) - s );
				SetFilePointer( hFile, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
			}
		}
	}

	GlobalFree( buf );
}

char read_contact_list( wchar_t *file_path, dllrbt_tree *list )
{
	char ret_status = 0;

	HANDLE hFile = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		wchar_t *w_contact_info_values[ 18 ];
		wchar_t *w_contact_info_phone_numbers[ 6 ];

		char magic_identifier[ 4 ];
		ReadFile( hFile, magic_identifier, sizeof( char ) * 4, &read, NULL );

		char version = -1;

		if ( read == 4 )
		{
			if ( _memcmp( magic_identifier, MAGIC_ID_CONTACT_LIST, 4 ) == 0 )
			{
				version = 1;
			}
			else if ( _memcmp( magic_identifier, OLD_MAGIC_ID_CONTACT_LIST, 4 ) == 0 )
			{
				version = 0;
			}
		}

		if ( version == 1 )
		{
			DWORD fz = GetFileSize( hFile, NULL ) - 4;

			char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

			while ( total_read < fz )
			{
				ReadFile( hFile, buf, sizeof( char ) * 524288, &read, NULL );

				buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				// Make sure that we have at least part of the entry. This is the minimum size an entry could be.
				// Include wide char NULL terminators.
				if ( read < ( sizeof( wchar_t ) * 16 ) )
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

				p = buf;
				offset = last_entry = 0;

				while ( offset < read )
				{
					_memzero( w_contact_info_values, sizeof( wchar_t * ) * 18 );
					_memzero( w_contact_info_phone_numbers, sizeof( wchar_t * ) * 6 );

					for ( char i = 0, j = 0; i < 18; ++i )
					{
						int string_length = lstrlenW( ( wchar_t * )p ) + 1;

						offset += ( string_length * sizeof( wchar_t ) );
						if ( offset > read ) { goto CLEANUP; }

						switch ( i )
						{
							case 0:
							case 4:
							case 6:
							case 10:
							case 11:
							case 15:
							{
								if ( string_length > 1 )
								{
									w_contact_info_phone_numbers[ j ] = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
									_wmemcpy_s( w_contact_info_phone_numbers[ j ], string_length, p, string_length );
									*( w_contact_info_phone_numbers[ j ] + ( string_length - 1 ) ) = 0;	// Sanity

									w_contact_info_values[ i ] = FormatPhoneNumberW( w_contact_info_phone_numbers[ j ] );
								}

								++j;
							}
							break;

							default:
							{
								if ( string_length > 1 )
								{
									w_contact_info_values[ i ] = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
									_wmemcpy_s( w_contact_info_values[ i ], string_length, p, string_length );
									*( w_contact_info_values[ i ] + ( string_length - 1 ) ) = 0;	// Sanity
								}
							}
							break;
						}

						p += ( string_length * sizeof( wchar_t ) );
					}

					last_entry = offset;	// This value is the ending offset of the last valid entry.

					// Create our list entry.

					contact_info *ci = ( contact_info * )GlobalAlloc( GPTR, sizeof( contact_info ) );

					for ( char i = 0; i < 16; ++i )
					{
						ci->w_contact_info_values[ i ] = w_contact_info_values[ i ];
					}

					for ( char i = 0; i < 6; ++i )
					{
						ci->w_contact_info_phone_numbers[ i ] = w_contact_info_phone_numbers[ i ];
					}

					if ( w_contact_info_values[ 16 ] != NULL )
					{
						ci->rti = ( ringtone_info * )dllrbt_find( ringtone_list, ( void * )w_contact_info_values[ 16 ], true );

						GlobalFree( w_contact_info_values[ 16 ] );
					}

					ci->picture_path = w_contact_info_values[ 17 ];

					if ( dllrbt_insert( list, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
					{
						free_contactinfo( &ci );
					}
					else
					{
						add_custom_caller_id_w( ci );
					}

					continue;

	CLEANUP:
					for ( char i = 0; i < 18; ++i )
					{
						GlobalFree( w_contact_info_values[ i ] );
					}

					for ( char i = 0; i < 6; ++i )
					{
						GlobalFree( w_contact_info_phone_numbers[ i ] );
					}

					// Go back to the last valid entry.
					if ( total_read < fz )
					{
						total_read -= ( read - last_entry );
						SetFilePointer( hFile, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
					}

					break;
				}
			}

			GlobalFree( buf );
		}
		else if ( version == 0 )
		{
			read_old_contact_list( hFile, list );
		}
		else
		{
			ret_status = -2;	// Bad file format.
		}

		CloseHandle( hFile );	
	}
	else
	{
		ret_status = -1;	// Can't open file for reading.
	}

	return ret_status;
}

char save_contact_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		int w_contact_info_values_lengths[ 18 ];

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 28 bytes.

		_memcpy_s( buf + pos, size - pos, MAGIC_ID_CONTACT_LIST, sizeof( char ) * 4 );	// Magic identifier for the ignore phone number list.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( contact_list );
		while ( node != NULL )
		{
			contact_info *ci = ( contact_info * )node->val;

			if ( ci != NULL )
			{
				_memzero( w_contact_info_values_lengths, sizeof( int ) * 18 );
				int write_length = 0;

				for ( char i = 0, j = 0; i < 16; ++i )
				{
					switch ( i )
					{
						case 0:
						case 4:
						case 6:
						case 10:
						case 11:
						case 15: { w_contact_info_values_lengths[ i ] = lstrlenW( ci->w_contact_info_phone_numbers[ j ] ) + 1; ++j; } break;
						default: { w_contact_info_values_lengths[ i ] = lstrlenW( ci->w_contact_info_values[ i ] ) + 1; } break;
					}

					write_length += w_contact_info_values_lengths[ i ];
				}

				if ( ci->rti != NULL )
				{
					w_contact_info_values_lengths[ 16 ] = lstrlenW( ci->rti->ringtone_file ) + 1;
				}
				else
				{
					w_contact_info_values_lengths[ 16 ] = 1;	// NULL terminator.
				}
				write_length += w_contact_info_values_lengths[ 16 ];

				w_contact_info_values_lengths[ 17 ] = lstrlenW( ci->picture_path ) + 1;
				write_length += w_contact_info_values_lengths[ 17 ];

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + write_length > size )
				{
					// Dump the buffer.
					WriteFile( hFile, buf, pos, &write, NULL );
					pos = 0;
				}

				// Add to the buffer.
				for ( char i = 0, j = 0; i < 16; ++i )
				{
					switch ( i )
					{
						case 0:
						case 4:
						case 6:
						case 10:
						case 11:
						case 15: { _memcpy_s( buf + pos, size - pos, SAFESTRW( ci->w_contact_info_phone_numbers[ j ] ), w_contact_info_values_lengths[ i ] * sizeof( wchar_t ) ); ++j; } break;
						default: { _memcpy_s( buf + pos, size - pos, SAFESTRW( ci->w_contact_info_values[ i ] ), w_contact_info_values_lengths[ i ] * sizeof( wchar_t ) ); } break;
					}

					pos += ( w_contact_info_values_lengths[ i ] * sizeof( wchar_t ) );
				}

				if ( ci->rti != NULL )
				{
					_memcpy_s( buf + pos, size - pos, SAFESTRW( ci->rti->ringtone_file ), w_contact_info_values_lengths[ 16 ] * sizeof( wchar_t ) );
					pos += ( w_contact_info_values_lengths[ 16 ] * sizeof( wchar_t ) );
				}
				else
				{
					_memcpy_s( buf + pos, size - pos, L"", sizeof( wchar_t ) );
					pos += sizeof( wchar_t );
				}

				_memcpy_s( buf + pos, size - pos, SAFESTRW( ci->picture_path ), w_contact_info_values_lengths[ 17 ] * sizeof( wchar_t ) );
				pos += ( w_contact_info_values_lengths[ 17 ] * sizeof( wchar_t ) );
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
		}

		GlobalFree( buf );

		CloseHandle( hFile );
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

	if ( allow_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\allow_phone_numbers\0", 21 );
		base_directory[ base_directory_length + 20 ] = 0;	// Sanity.

		save_allow_ignore_list( base_directory, allow_list, LIST_TYPE_ALLOW );
		allow_list_changed = false;
	}

	if ( allow_cid_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\allow_caller_id_names\0", 23 );
		base_directory[ base_directory_length + 22 ] = 0;	// Sanity.

		save_allow_ignore_cid_list( base_directory, allow_cid_list, LIST_TYPE_ALLOW );
		allow_cid_list_changed = false;
	}

	if ( ignore_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_phone_numbers\0", 22 );
		base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

		save_allow_ignore_list( base_directory, ignore_list, LIST_TYPE_IGNORE );
		ignore_list_changed = false;
	}

	if ( ignore_cid_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_caller_id_names\0", 24 );
		base_directory[ base_directory_length + 23 ] = 0;	// Sanity.

		save_allow_ignore_cid_list( base_directory, ignore_cid_list, LIST_TYPE_IGNORE );
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
