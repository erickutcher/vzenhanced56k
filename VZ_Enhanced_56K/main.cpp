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
#include "wnd_proc.h"

#include "file_operations.h"
#include "string_tables.h"
#include "ssl_client.h"

#include "lite_tapi32.h"
#include "lite_gdiplus.h"
#include "lite_winmm.h"
#include "lite_shell32.h"
#include "lite_advapi32.h"
#include "lite_comdlg32.h"
#include "lite_gdi32.h"
#include "lite_comctl32.h"
#include "lite_ole32.h"

#include "telephony.h"
#include "waveout.h"

#ifdef USE_DEBUG_DIRECTORY
	#define BASE_DIRECTORY_FLAG CSIDL_APPDATA
#else
	#define BASE_DIRECTORY_FLAG CSIDL_LOCAL_APPDATA
#endif

SYSTEMTIME g_compile_time;

// We want to get these objects before the window is shown.

// Object variables
HWND g_hWnd_main = NULL;		// Handle to our main window.
HWND g_hWnd_message_log = NULL;	// Handle to our message log window.

HWND g_hWnd_active = NULL;		// Handle to the active window. Used to handle tab stops.

HFONT hFont = NULL;				// Handle to our font object.

int row_height = 0;

wchar_t *base_directory = NULL;
unsigned int base_directory_length = 0;

wchar_t *app_directory = NULL;
unsigned int app_directory_length = 0;

#ifndef NTDLL_USE_STATIC_LIB
int APIENTRY _WinMain()
#else
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif
{
	#ifndef NTDLL_USE_STATIC_LIB
		HINSTANCE hInstance = GetModuleHandle( NULL );
	#endif

	#ifndef USER32_USE_STATIC_LIB
		if ( !InitializeUser32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef NTDLL_USE_STATIC_LIB
		if ( !InitializeNTDLL() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef TAPI32_USE_STATIC_LIB
		if ( !InitializeTAPI32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef GDI32_USE_STATIC_LIB
		if ( !InitializeGDI32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef COMDLG32_USE_STATIC_LIB
		if ( !InitializeComDlg32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef SHELL32_USE_STATIC_LIB
		if ( !InitializeShell32() ){ goto UNLOAD_DLLS; }
	#endif

	_memzero( &g_compile_time, sizeof( SYSTEMTIME ) );
	if ( hInstance != NULL )
	{
		IMAGE_DOS_HEADER *idh = ( IMAGE_DOS_HEADER * )hInstance;
		IMAGE_NT_HEADERS *inth = ( IMAGE_NT_HEADERS * )( ( BYTE * )idh + idh->e_lfanew );

		UnixTimeToSystemTime( inth->FileHeader.TimeDateStamp, &g_compile_time );
	}

	unsigned char fail_type = 0;
	MSG msg;
	_memzero( &msg, sizeof( MSG ) );

	base_directory = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

	app_directory = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

	app_directory_length = GetModuleFileNameW( NULL, app_directory, MAX_PATH );

	while ( app_directory_length != 0 && app_directory[ --app_directory_length ] != L'\\' );

	// Get the new base directory if the user supplied a path.
	bool default_directory = true;
	int argCount = 0;
	LPWSTR *szArgList = _CommandLineToArgvW( GetCommandLineW(), &argCount );
	if ( szArgList != NULL )
	{
		// The first parameter is the path to the executable, second is our switch "-d", and third is the new base directory path.
		if ( argCount == 3 &&
			 szArgList[ 1 ][ 0 ] == L'-' && szArgList[ 1 ][ 1 ] == L'd' && szArgList[ 1 ][ 2 ] == 0 &&
			 GetFileAttributesW( szArgList[ 2 ] ) == FILE_ATTRIBUTE_DIRECTORY )
		{
			base_directory_length = lstrlenW( szArgList[ 2 ] );
			if ( base_directory_length >= MAX_PATH )
			{
				base_directory_length = MAX_PATH - 1;
			}
			_wmemcpy_s( base_directory, MAX_PATH, szArgList[ 2 ], base_directory_length );
			base_directory[ base_directory_length ] = 0;	// Sanity.

			default_directory = false;
		}
		else if ( argCount == 2 &&
				  szArgList[ 1 ][ 0 ] == L'-' && szArgList[ 1 ][ 1 ] == L'p' )	// Portable mode (use the application's current directory for our base directory).
		{
			base_directory_length = app_directory_length;
			_wmemcpy_s( base_directory, MAX_PATH, app_directory, base_directory_length );
			base_directory[ base_directory_length ] = 0;	// Sanity.

			default_directory = false;
		}

		// Free the parameter list.
		LocalFree( szArgList );
	}

	// Use our default directory if none was supplied.
	if ( default_directory )
	{
		_SHGetFolderPathW( NULL, BASE_DIRECTORY_FLAG, NULL, 0, base_directory );

		base_directory_length = lstrlenW( base_directory );
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\VZ Enhanced 56K\0", 17 );
		base_directory_length += 16;
		base_directory[ base_directory_length ] = 0;	// Sanity.

		// Check to see if the new path exists and create it if it doesn't.
		if ( GetFileAttributesW( base_directory ) == INVALID_FILE_ATTRIBUTES )
		{
			CreateDirectoryW( base_directory, NULL );
		}
	}

	// Blocks our reading thread and various GUI operations.
	InitializeCriticalSection( &pe_cs );

	// Blocks our telephony thread.
	InitializeCriticalSection( &tt_cs );

	// Allow a clean shutdown of the telephony thread if a call was answered.
	InitializeCriticalSection( &ac_cs );

	// Blocks our update check threads.
	InitializeCriticalSection( &cut_cs );

	// Blocks the CleanupConnection().
	InitializeCriticalSection( &cuc_cs );

	// Blocks our message log worker threads.
	InitializeCriticalSection( &ml_cs );

	// Blocks our message log update thread.
	InitializeCriticalSection( &ml_update_cs );

	// Blocks threads from adding/removing from the message log queue.
	InitializeCriticalSection( &ml_queue_cs );

	// Blocks our ringtone update thread.
	InitializeCriticalSection( &ringtone_update_cs );

	// Blocks threads from adding/removing from the ringtone queue.
	InitializeCriticalSection( &ringtone_queue_cs );

	// Blocks threads from auto saving our call log and lists.
	InitializeCriticalSection( &auto_save_cs );

	// Get the default message system font.
	NONCLIENTMETRICS ncm;
	_memzero( &ncm, sizeof( NONCLIENTMETRICS ) );
	ncm.cbSize = sizeof( NONCLIENTMETRICS );
	_SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &ncm, 0 );

	/*// Force the font to be 8 points.
	HDC hdc_screen = _GetDC( NULL );
	int logical_height = -MulDiv( 8, _GetDeviceCaps( hdc_screen, LOGPIXELSY ), 72 );
	_ReleaseDC( NULL, hdc_screen );

	ncm.lfMessageFont.lfHeight = logical_height;*/

	// Set our global font to the LOGFONT value obtained from the system.
	hFont = _CreateFontIndirectW( &ncm.lfMessageFont );

	// Get the row height for our listview control.
	TEXTMETRIC tm;
	HDC hDC = _GetDC( NULL );
	HFONT ohf = ( HFONT )_SelectObject( hDC, hFont );
	_GetTextMetricsW( hDC, &tm );
	_SelectObject( hDC, ohf );	// Reset old font.
	_ReleaseDC( NULL, hDC );

	row_height = tm.tmHeight + tm.tmExternalLeading + 5;

	int icon_height = _GetSystemMetrics( SM_CYSMICON ) + 2;
	if ( row_height < icon_height )
	{
		row_height = icon_height;
	}

	// Default position if no settings were saved.
	cfg_pos_x = ( ( _GetSystemMetrics( SM_CXSCREEN ) - MIN_WIDTH ) / 2 );
	cfg_pos_y = ( ( _GetSystemMetrics( SM_CYSCREEN ) - MIN_HEIGHT ) / 2 );

	_memzero( ignore_range_list, sizeof( ignore_range_list ) );

	_memzero( &cfg_modem_guid, sizeof( GUID ) );

	read_config();

	modem_list = dllrbt_create( dllrbt_compare_guid );

	recording_list = dllrbt_create( dllrbt_compare_w );

	LoadRecordings( recording_list );

	if ( cfg_recording != NULL )
	{
		wchar_t *file_name = GetFileNameW( cfg_recording );
		default_recording = ( ringtoneinfo * )dllrbt_find( recording_list, ( void * )file_name, true );
	}

	ringtone_list = dllrbt_create( dllrbt_compare_w );

	// Load our ringtones before our contact list.
	// The contact list will call the ringtone list when building itself.
	LoadRingtones( ringtone_list );

	if ( cfg_popup_ringtone != NULL )
	{
		wchar_t *file_name = GetFileNameW( cfg_popup_ringtone );
		default_ringtone = ( ringtoneinfo * )dllrbt_find( ringtone_list, ( void * )file_name, true );
	}

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_phone_numbers\0", 22 );
	base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

	ignore_list = dllrbt_create( dllrbt_compare_a );

	read_ignore_list( base_directory, ignore_list );

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_caller_id_names\0", 24 );
	base_directory[ base_directory_length + 23 ] = 0;	// Sanity.

	ignore_cid_list = dllrbt_create( dllrbt_icid_compare );

	read_ignore_cid_list( base_directory, ignore_cid_list );

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\contact_list\0", 14 );
	base_directory[ base_directory_length + 13 ] = 0;	// Sanity.

	contact_list = dllrbt_create( dllrbt_compare_ptr );

	read_contact_list( base_directory, contact_list );

	// Create a tree of linked lists. Each linked list contains a list of displayinfo structs that share the same "call from" phone number.
	call_log = dllrbt_create( dllrbt_compare_a );

	if ( cfg_popup_font_face1 == NULL )
	{
		cfg_popup_font_face1 = GlobalStrDupW( ncm.lfMessageFont.lfFaceName );
		cfg_popup_font_height1 = ncm.lfMessageFont.lfHeight;
		cfg_popup_font_weight1 = ncm.lfMessageFont.lfWeight;
		cfg_popup_font_italic1 = ncm.lfMessageFont.lfItalic;
		cfg_popup_font_underline1 = ncm.lfMessageFont.lfUnderline;
		cfg_popup_font_strikeout1 = ncm.lfMessageFont.lfStrikeOut;
	}

	if ( cfg_popup_font_face2 == NULL )
	{
		cfg_popup_font_face2 = GlobalStrDupW( ncm.lfMessageFont.lfFaceName );
		cfg_popup_font_height2 = ncm.lfMessageFont.lfHeight;
		cfg_popup_font_weight2 = ncm.lfMessageFont.lfWeight;
		cfg_popup_font_italic2 = ncm.lfMessageFont.lfItalic;
		cfg_popup_font_underline2 = ncm.lfMessageFont.lfUnderline;
		cfg_popup_font_strikeout2 = ncm.lfMessageFont.lfStrikeOut;
	}

	if ( cfg_popup_font_face3 == NULL )
	{
		cfg_popup_font_face3 = GlobalStrDupW( ncm.lfMessageFont.lfFaceName );
		cfg_popup_font_height3 = ncm.lfMessageFont.lfHeight;
		cfg_popup_font_weight3 = ncm.lfMessageFont.lfWeight;
		cfg_popup_font_italic3 = ncm.lfMessageFont.lfItalic;
		cfg_popup_font_underline3 = ncm.lfMessageFont.lfUnderline;
		cfg_popup_font_strikeout3 = ncm.lfMessageFont.lfStrikeOut;
	}

	// Initialize our window class.
	WNDCLASSEX wcex;
	_memzero( &wcex, sizeof( WNDCLASSEX ) );
	wcex.cbSize			= sizeof( WNDCLASSEX );
	wcex.style          = CS_VREDRAW | CS_HREDRAW;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = _LoadIconW( hInstance, MAKEINTRESOURCE( IDI_ICON ) );
	wcex.hCursor        = _LoadCursorW( NULL, IDC_ARROW );
	wcex.hbrBackground  = ( HBRUSH )( COLOR_WINDOW );
	wcex.lpszMenuName   = NULL;
	wcex.hIconSm        = NULL;


	wcex.lpfnWndProc    = MainWndProc;
	wcex.lpszClassName  = L"callerid";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = OptionsWndProc;
	wcex.lpszClassName  = L"options";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = ColumnsWndProc;
	wcex.lpszClassName  = L"columns";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = MessageLogWndProc;
	wcex.lpszClassName  = L"message_log";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = UpdateWndProc;
	wcex.lpszClassName  = L"update";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = ContactWndProc;
	wcex.lpszClassName  = L"contact";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = PhoneWndProc;
	wcex.lpszClassName  = L"phone";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = CIDWndProc;
	wcex.lpszClassName  = L"cid";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.cbWndExtra     = sizeof( LONG_PTR );
	wcex.lpfnWndProc    = PopupWndProc;
	wcex.lpszClassName  = L"popup";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.cbWndExtra		= 0;
	wcex.hIcon			= NULL;
	wcex.hIconSm		= NULL;
	wcex.hbrBackground  = ( HBRUSH )( COLOR_WINDOWFRAME );//( HBRUSH )( _GetStockObject( WHITE_BRUSH ) );
	
	wcex.lpfnWndProc    = CallLogColumnsWndProc;
	wcex.lpszClassName  = L"calllog_columns";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = ContactListColumnsWndProc;
	wcex.lpszClassName  = L"contactlist_columns";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = IgnoreListColumnsWndProc;
	wcex.lpszClassName  = L"ignorelist_columns";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = ConnectionTabWndProc;
	wcex.lpszClassName  = L"connection_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = ModemTabWndProc;
	wcex.lpszClassName  = L"modem_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = PopupTabWndProc;
	wcex.lpszClassName  = L"popup_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = GeneralTabWndProc;
	wcex.lpszClassName  = L"general_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	// Create this first so that the other windows and their function calls can start logging to it.
	g_hWnd_message_log = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"message_log", ST_Message_Log, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 600 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 480 ) / 2 ), 600, 480, NULL, NULL, NULL, NULL );

	if ( !g_hWnd_message_log )
	{
		fail_type = 2;
		goto CLEANUP;
	}

	g_hWnd_main = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"callerid", PROGRAM_CAPTION, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, cfg_pos_x, cfg_pos_y, cfg_width, cfg_height, NULL, NULL, NULL, NULL );

	if ( !g_hWnd_main )
	{
		fail_type = 2;
		goto CLEANUP;
	}

	bool telephony_started = InitializeTelephony();
	if ( telephony_started )
	{
		// Build our modem list, initialize all our TAPI values, and listen for incoming calls.
		CloseHandle( ( HANDLE )_CreateThread( NULL, 0, TelephonyPoll, ( void * )NULL, 0, NULL ) );
	}

	if ( !cfg_silent_startup )
	{
		_ShowWindow( g_hWnd_main, ( cfg_min_max == 1 ? SW_MINIMIZE : ( cfg_min_max == 2 ? SW_MAXIMIZE : SW_SHOWNORMAL ) ) );
	}

	// Main message loop:
	while ( _GetMessageW( &msg, NULL, 0, 0 ) > 0 )
	{
		if ( g_hWnd_active == NULL || !_IsDialogMessageW( g_hWnd_active, &msg ) )	// Checks tab stops.
		{
			_TranslateMessage( &msg );
			_DispatchMessageW( &msg );
		}
	}

	if ( telephony_started )
	{
		UnInitializeTelephony();
	}

CLEANUP:

	// Save before we exit.
	save_config();

	if ( ignore_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_phone_numbers\0", 22 );
		base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

		save_ignore_list( base_directory );
	}

	if ( ignore_cid_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore_caller_id_names\0", 24 );
		base_directory[ base_directory_length + 23 ] = 0;	// Sanity.

		save_ignore_cid_list( base_directory );
	}

	if ( contact_list_changed )
	{
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\contact_list\0", 14 );
		base_directory[ base_directory_length + 13 ] = 0;	// Sanity.

		save_contact_list( base_directory );
	}

	if ( cfg_recording != NULL )
	{
		GlobalFree( cfg_recording );
	}

	if ( cfg_popup_font_face1 != NULL )
	{
		GlobalFree( cfg_popup_font_face1 );
	}

	if ( cfg_popup_font_face2 != NULL )
	{
		GlobalFree( cfg_popup_font_face2 );
	}

	if ( cfg_popup_font_face3 != NULL )
	{
		GlobalFree( cfg_popup_font_face3 );
	}

	if ( cfg_popup_ringtone != NULL )
	{
		GlobalFree( cfg_popup_ringtone );
	}

	// Free the values of the modem_list.
	node_type *node = dllrbt_get_head( modem_list );
	while ( node != NULL )
	{
		modeminfo *mi = ( modeminfo * )node->val;

		if ( mi != NULL )
		{
			GlobalFree( mi->line_name );
			GlobalFree( mi );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( modem_list );
	modem_list = NULL;

	// Free the values of the recording_list.
	node = dllrbt_get_head( recording_list );
	while ( node != NULL )
	{
		ringtoneinfo *rti = ( ringtoneinfo * )node->val;

		if ( rti != NULL )
		{
			// ringtone_file points to the same memory as ringtone_path. So don't free it.
			GlobalFree( rti->ringtone_path );
			GlobalFree( rti );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( recording_list );
	recording_list = NULL;

	// Free the values of the ringtone_list.
	node = dllrbt_get_head( ringtone_list );
	while ( node != NULL )
	{
		ringtoneinfo *rti = ( ringtoneinfo * )node->val;

		if ( rti != NULL )
		{
			// ringtone_file points to the same memory as ringtone_path. So don't free it.
			GlobalFree( rti->ringtone_path );
			GlobalFree( rti );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( ringtone_list );
	ringtone_list = NULL;

	// Free the values of the ignore_list.
	node = dllrbt_get_head( ignore_list );
	while ( node != NULL )
	{
		ignoreinfo *ii = ( ignoreinfo * )node->val;

		if ( ii != NULL )
		{
			free_ignoreinfo( &ii );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( ignore_list );
	ignore_list = NULL;

	// Free the values of the ignore_cid_list.
	node = dllrbt_get_head( ignore_cid_list );
	while ( node != NULL )
	{
		ignorecidinfo *icidi = ( ignorecidinfo * )node->val;

		if ( icidi != NULL )
		{
			free_ignorecidinfo( &icidi );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( ignore_cid_list );
	ignore_cid_list = NULL;

	// Free the values of the contact_list.
	node = dllrbt_get_head( contact_list );
	while ( node != NULL )
	{
		contactinfo *ci = ( contactinfo * )node->val;

		if ( ci != NULL )
		{
			free_contactinfo( &ci );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( contact_list );
	contact_list = NULL;

	// Free the values of the call_log.
	node = dllrbt_get_head( call_log );
	while ( node != NULL )
	{
		// Free the linked list if there is one.
		DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
		while ( di_node != NULL )
		{
			DoublyLinkedList *del_di_node = di_node;

			di_node = di_node->next;

			// del_di_node->data contained our displayinfo structs which we freed in wnd_proc_main's WM_DESTROY_ALT.
			GlobalFree( del_di_node );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( call_log );
	call_log = NULL;

	for ( char i = 0; i < 16; ++i )
	{
		RangeDelete( &ignore_range_list[ i ] );
		ignore_range_list[ i ] = NULL;
	}

	cleanup_custom_caller_id();

	if ( base_directory != NULL )
	{
		GlobalFree( base_directory );
	}

	if ( app_directory != NULL )
	{
		GlobalFree( app_directory );
	}

	// Delete our font.
	_DeleteObject( hFont );

	// Delete our critical sections.
	DeleteCriticalSection( &pe_cs );	// User initiated actions
	DeleteCriticalSection( &tt_cs );	// Telephony poll
	DeleteCriticalSection( &ac_cs );	// Answered a call
	DeleteCriticalSection( &cut_cs );	// Update check
	DeleteCriticalSection( &cuc_cs );	// Cleanup connection
	DeleteCriticalSection( &ml_cs );	// Message log actions
	DeleteCriticalSection( &ml_update_cs );	// Message log updates
	DeleteCriticalSection( &ml_queue_cs );	// Message log queue operations
	DeleteCriticalSection( &ringtone_update_cs );	// Ringtone updates
	DeleteCriticalSection( &ringtone_queue_cs );	// Ringtone queue operations
	DeleteCriticalSection( &auto_save_cs );	// Auto save call log and lists

	if ( fail_type == 1 )
	{
		_MessageBoxA( NULL, "Call to _RegisterClassExW failed!", PROGRAM_CAPTION_A, MB_ICONWARNING );
	}
	else if ( fail_type == 2 )
	{
		_MessageBoxA( NULL, "Call to CreateWindow failed!", PROGRAM_CAPTION_A, MB_ICONWARNING );
	}

	// Delay loaded DLLs
	SSL_library_uninit();

	#ifndef WS2_32_USE_STATIC_LIB
		UnInitializeWS2_32();
	#else
		EndWS2_32();
	#endif
	#ifndef GDIPLUS_USE_STATIC_LIB
		UnInitializeGDIPlus();
	#else
		EndGDIPlus();
	#endif
	#ifndef WINMM_USE_STATIC_LIB
		UnInitializeWinMM();
	#endif
	#ifndef ADVAPI32_USE_STATIC_LIB
		UnInitializeAdvApi32();
	#endif
	#ifndef COMCTL32_USE_STATIC_LIB
		UnInitializeComCtl32();
	#endif
	#ifndef OLE32_USE_STATIC_LIB
		UnInitializeOle32();
	#endif

	// Start up loaded DLLs

UNLOAD_DLLS:

	#ifndef SHELL32_USE_STATIC_LIB
		UnInitializeShell32();
	#endif
	#ifndef COMDLG32_USE_STATIC_LIB
		UnInitializeComDlg32();
	#endif
	#ifndef GDI32_USE_STATIC_LIB
		UnInitializeGDI32();
	#endif
	#ifndef TAPI32_USE_STATIC_LIB
		UnInitializeTAPI32();
	#endif
	#ifndef NTDLL_USE_STATIC_LIB
		UnInitializeNTDLL();
	#endif
	#ifndef USER32_USE_STATIC_LIB
		UnInitializeUser32();
	#endif

	#ifndef NTDLL_USE_STATIC_LIB
		ExitProcess( ( UINT )msg.wParam );
	#endif
	return ( int )msg.wParam;
}
