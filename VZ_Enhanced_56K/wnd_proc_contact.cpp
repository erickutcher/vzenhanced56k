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
#include "connection.h"
#include "utilities.h"
#include "ringtone_utilities.h"
#include "string_tables.h"
#include "list_operations.h"

#include "lite_comdlg32.h"
#include "lite_gdi32.h"
#include "lite_gdiplus.h"

#define BTN_EXIT_INFO		1000
#define BTN_UPDATE_PICTURE	1001
#define BTN_REMOVE_PICTURE	1002

#define CB_RINGTONES		1003
#define BTN_PLAY_RINGTONE	1004
#define BTN_STOP_RINGTONE	1005

HWND g_hWnd_contact = NULL;

HWND g_hWnd_title = NULL;
HWND g_hWnd_first_name = NULL;
HWND g_hWnd_last_name = NULL;
HWND g_hWnd_nickname = NULL;

HWND g_hWnd_company = NULL;
HWND g_hWnd_job_title = NULL;
HWND g_hWnd_department = NULL;
HWND g_hWnd_profession = NULL;

HWND g_hWnd_home_phone_number = NULL;
HWND g_hWnd_cell_phone_number = NULL;
HWND g_hWnd_office_phone_number = NULL;
HWND g_hWnd_other_phone_number = NULL;

HWND g_hWnd_work_phone_number = NULL;
HWND g_hWnd_fax_number = NULL;

HWND g_hWnd_email_address = NULL;
HWND g_hWnd_web_page = NULL;

HWND g_hWnd_ringtones = NULL;
HWND g_hWnd_play_ringtone = NULL;
HWND g_hWnd_stop_ringtone = NULL;

HWND g_hWnd_remove_picture = NULL;

HWND g_hWnd_contact_action = NULL;

HWND g_hWnd_static_picture = NULL;

contact_info *edit_ci = NULL;

HWND window_fields[ 16 ];

unsigned int g_picture_height = 0;
unsigned int g_picture_width = 0;
HBITMAP g_hbm_picture = NULL;

wchar_t *g_picture_file_path = NULL;
bool remove_picture = false;

bool contact_ringtone_played = false;

LRESULT CALLBACK ContactWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			//HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static1 = _CreateWindowW( WC_STATIC, ST_Title_, WS_CHILD | WS_VISIBLE, 20, 22, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_title = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 20, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static2 = _CreateWindowW( WC_STATIC, ST_First_Name_, WS_CHILD | WS_VISIBLE, 20, 50, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_first_name = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 48, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static3 = _CreateWindowW( WC_STATIC, ST_Last_Name_, WS_CHILD | WS_VISIBLE, 20, 78, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_last_name = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 76, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static4 = _CreateWindowW( WC_STATIC, ST_Nickname_, WS_CHILD | WS_VISIBLE, 20, 106, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_nickname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 104, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static5 = _CreateWindowW( WC_STATIC, ST_Company_, WS_CHILD | WS_VISIBLE, 20, 134, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_company = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 132, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static6 = _CreateWindowW( WC_STATIC, ST_Job_Title_, WS_CHILD | WS_VISIBLE, 20, 162, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_job_title = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 160, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static7 = _CreateWindowW( WC_STATIC, ST_Department_, WS_CHILD | WS_VISIBLE, 20, 190, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_department = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 188, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static8 = _CreateWindowW( WC_STATIC, ST_Profession_, WS_CHILD | WS_VISIBLE, 20, 218, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_profession = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 216, 105, 23, hWnd, NULL, NULL, NULL );



			HWND g_hWnd_static9 = _CreateWindowW( WC_STATIC, ST_Home_Phone_, WS_CHILD | WS_VISIBLE, 220, 22, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_home_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 20, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static10 = _CreateWindowW( WC_STATIC, ST_Cell_Phone_, WS_CHILD | WS_VISIBLE, 220, 50, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_cell_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 48, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static11 = _CreateWindowW( WC_STATIC, ST_Office_Phone_, WS_CHILD | WS_VISIBLE, 220, 78, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_office_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 76, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static12 = _CreateWindowW( WC_STATIC, ST_Other_Phone_, WS_CHILD | WS_VISIBLE, 220, 106, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_other_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 104, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static13 = _CreateWindowW( WC_STATIC, ST_Work_Phone_, WS_CHILD | WS_VISIBLE, 220, 134, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_work_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 132, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static14 = _CreateWindowW( WC_STATIC, ST_Fax_, WS_CHILD | WS_VISIBLE, 220, 162, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_fax_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 160, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static15 = _CreateWindowW( WC_STATIC, ST_Email_Address_, WS_CHILD | WS_VISIBLE, 220, 190, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_email_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 188, 105, 23, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static16 = _CreateWindowW( WC_STATIC, ST_Web_Page_, WS_CHILD | WS_VISIBLE, 220, 218, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_web_page = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 216, 105, 23, hWnd, NULL, NULL, NULL );


			HWND g_hWnd_static17 = _CreateWindowW( WC_STATIC, ST_Ringtone_, WS_CHILD | WS_VISIBLE, 20, 246, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_ringtones = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, L"", CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 100, 246, 190, 23, hWnd, ( HMENU )CB_RINGTONES, NULL, NULL );
			_SendMessageW( g_hWnd_ringtones, CB_ADDSTRING, 0, ( LPARAM )L"" );
			_SendMessageW( g_hWnd_ringtones, CB_SETCURSEL, 0, 0 );
			g_hWnd_play_ringtone = _CreateWindowW( WC_BUTTON, ST_Play, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 246, 50, 23, hWnd, ( HMENU )BTN_PLAY_RINGTONE, NULL, NULL );
			g_hWnd_stop_ringtone = _CreateWindowW( WC_BUTTON, ST_Stop, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 355, 246, 50, 23, hWnd, ( HMENU )BTN_STOP_RINGTONE, NULL, NULL );

			HWND g_hWnd_static18 = _CreateWindowW( WC_STATIC, ST_Contact_Picture_, WS_CHILD | WS_VISIBLE, 420, 20, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_static_picture = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 420, 35, 100, 100, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_update_picture = _CreateWindowW( WC_BUTTON, ST_Choose_File___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 420, 145, 100, 23, hWnd, ( HMENU )BTN_UPDATE_PICTURE, NULL, NULL );
			g_hWnd_remove_picture = _CreateWindowW( WC_BUTTON, ST_Remove_Picture, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 420, 173, 100, 23, hWnd, ( HMENU )BTN_REMOVE_PICTURE, NULL, NULL );

			g_hWnd_contact_action = _CreateWindowW( WC_BUTTON, ST_Add_Contact, WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) ) / 2, rc.bottom - 32, _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ), 23, hWnd, ( HMENU )BTN_EXIT_INFO, NULL, NULL );

			_SendMessageW( g_hWnd_home_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_cell_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_office_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_other_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_work_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_fax_number, EM_LIMITTEXT, 15, 0 );

			_SendMessageW( g_hWnd_title, EM_LIMITTEXT, 64, 0 );
			_SendMessageW( g_hWnd_first_name, EM_LIMITTEXT, 128, 0 );
			_SendMessageW( g_hWnd_last_name, EM_LIMITTEXT, 128, 0 );
			_SendMessageW( g_hWnd_nickname, EM_LIMITTEXT, 128, 0 );
			_SendMessageW( g_hWnd_company, EM_LIMITTEXT, 256, 0 );
			_SendMessageW( g_hWnd_job_title, EM_LIMITTEXT, 64, 0 );
			_SendMessageW( g_hWnd_department, EM_LIMITTEXT, 256, 0 );
			_SendMessageW( g_hWnd_profession, EM_LIMITTEXT, 256, 0 );
			_SendMessageW( g_hWnd_email_address, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_web_page, EM_LIMITTEXT, 2083, 0 );


			_SendMessageW( g_hWnd_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_title, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_first_name, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static3, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_last_name, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static4, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_nickname, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static5, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_company, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static6, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_job_title, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static7, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_department, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static8, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_profession, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static9, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_home_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static10, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_cell_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static11, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_office_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static12, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_other_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static13, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_work_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static14, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_fax_number, WM_SETFONT, ( WPARAM )hFont, 0 );


			_SendMessageW( g_hWnd_static15, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_email_address, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static16, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_web_page, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static17, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ringtones, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_play_ringtone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_stop_ringtone, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static18, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_update_picture, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_remove_picture, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_contact_action, WM_SETFONT, ( WPARAM )hFont, 0 );


			window_fields[ 0 ] = g_hWnd_cell_phone_number;
			window_fields[ 1 ] = g_hWnd_company;
			window_fields[ 2 ] = g_hWnd_department;
			window_fields[ 3 ] = g_hWnd_email_address;
			window_fields[ 4 ] = g_hWnd_fax_number;
			window_fields[ 5 ] = g_hWnd_first_name;
			window_fields[ 6 ] = g_hWnd_home_phone_number;
			window_fields[ 7 ] = g_hWnd_job_title;
			window_fields[ 8 ] = g_hWnd_last_name;
			window_fields[ 9 ] = g_hWnd_nickname;
			window_fields[ 10 ] = g_hWnd_office_phone_number;
			window_fields[ 11 ] = g_hWnd_other_phone_number;
			window_fields[ 12 ] = g_hWnd_profession;
			window_fields[ 13 ] = g_hWnd_title;
			window_fields[ 14 ] = g_hWnd_web_page;
			window_fields[ 15 ] = g_hWnd_work_phone_number;


			node_type *node = dllrbt_get_head( ringtone_list );
			while ( node != NULL )
			{
				ringtone_info *rti = ( ringtone_info * )node->val;

				if ( rti != NULL )
				{
					int add_index = ( int )_SendMessageW( g_hWnd_ringtones, CB_ADDSTRING, 0, ( LPARAM )rti->ringtone_file );

					_SendMessageW( g_hWnd_ringtones, CB_SETITEMDATA, add_index, ( LPARAM )rti );
				}

				node = node->next;
			}	

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc, frame_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_3DFACE ) );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			frame_rc = client_rc;
			frame_rc.left += 10;
			frame_rc.right -= 10;
			frame_rc.top += 10;
			frame_rc.bottom -= 40;

			// Fill the frame.
			color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
			_FillRect( hdcMem, &frame_rc, color );
			_DeleteObject( color );

			// Draw the frame's border.
			_DrawEdge( hdcMem, &frame_rc, EDGE_ETCHED, BF_RECT );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_STATIC && dis->hwndItem == g_hWnd_static_picture )
			{
				// Create and save a bitmap in memory to paint to.
				HDC hdcMem = _CreateCompatibleDC( dis->hDC );
				HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );
				_DeleteObject( hbm );

				HBRUSH hBrush = _GetSysColorBrush( COLOR_WINDOW );//( HBRUSH )_GetStockObject( WHITE_BRUSH );//GetClassLong( _GetParent( hWnd ), GCL_HBRBACKGROUND );
				_FillRect( hdcMem, &dis->rcItem, hBrush );

				// Load the image and center it in the frame bitmap.
				HDC hdcMem2 = _CreateCompatibleDC( dis->hDC );
				ohbm = ( HBITMAP )_SelectObject( hdcMem2, g_hbm_picture );
				_DeleteObject( ohbm );
				_BitBlt( hdcMem, ( ( dis->rcItem.right - dis->rcItem.left ) - g_picture_width ) / 2, ( ( dis->rcItem.bottom - dis->rcItem.top ) - g_picture_height ) / 2, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top, hdcMem2, 0, 0, SRCCOPY );
				_DeleteDC( hdcMem2 );

				// Draw everything to the device.
				_BitBlt( dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top, hdcMem, 0, 0, SRCCOPY );
	
				// Delete our back buffer.
				_DeleteDC( hdcMem );
			}
			return TRUE;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_EXIT_INFO:
				{
					bool info_changed = false;	// If this is true, then we'll update the contact.

					contact_info *ci = ( contact_info * )GlobalAlloc( GPTR, sizeof( contact_info ) );

					int cfg_val_length = 0;
					char *utf8_val = NULL;

					for ( char i = 0, j = 0; i < 16; ++i )
					{
						unsigned int text_length = ( unsigned int )_SendMessageW( window_fields[ i ], WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.

						wchar_t *text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * text_length );
						_SendMessageW( window_fields[ i ], WM_GETTEXT, text_length, ( LPARAM )text_val );

						switch ( i )
						{
							case 0:
							case 4:
							case 6:
							case 10:
							case 11:
							case 15:
							{
								if ( edit_ci == NULL ||
								   ( edit_ci != NULL && ( edit_ci->w_contact_info_phone_numbers[ j ] == NULL ||
														( edit_ci->w_contact_info_phone_numbers[ j ] != NULL && lstrcmpW( text_val, edit_ci->w_contact_info_phone_numbers[ j ] ) != 0 ) ) ) )
								{
									ci->w_contact_info_phone_numbers[ j ] = text_val;

									info_changed = true;
								}
								else
								{
									GlobalFree( text_val );
								}

								++j;
							}
							break;

							default:
							{
								if ( edit_ci == NULL ||
								   ( edit_ci != NULL && ( edit_ci->w_contact_info_values[ i ] == NULL ||
														( edit_ci->w_contact_info_values[ i ] != NULL && lstrcmpW( text_val, edit_ci->w_contact_info_values[ i ] ) != 0 ) ) ) )
								{
									ci->w_contact_info_values[ i ] = text_val;

									info_changed = true;
								}
								else
								{
									GlobalFree( text_val );
								}
							}
							break;

						}
					}

					int current_ringtone_index = ( int )_SendMessageW( g_hWnd_ringtones, CB_GETCURSEL, 0, 0 );
					if ( current_ringtone_index != CB_ERR )
					{
						ringtone_info *rti = NULL;

						if ( current_ringtone_index != 0 )
						{
							rti = ( ringtone_info * )_SendMessageW( g_hWnd_ringtones, CB_GETITEMDATA, current_ringtone_index, 0 );
							if ( rti == ( ringtone_info * )CB_ERR )
							{
								rti = NULL;
							}
						}

						//if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->rti == NULL || ( edit_ci->rti != NULL && edit_ci->rti->ringtone_path == NULL ) ) ) )
						if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->rti != rti ) ) )
						{
							ci->rti = rti;

							info_changed = true;

							// Don't handle the empty ringtone.
							if ( current_ringtone_index != 0 )
							{
								// If our ringtone setting is disabled, then ask if the user wants to enable it.
								if ( !cfg_popup_enable_ringtones )
								{
									// Also check if the popup windows setting is disabled and ask the user if they want to enable that as well.
									if ( !cfg_enable_popups )
									{
										if ( _MessageBoxW( hWnd, ST_PROMPT_Popup_windows_and_ringtones, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION | MB_YESNO ) == IDYES )
										{
											cfg_enable_popups = true;

											cfg_popup_enable_ringtones = true;
										}
									}
									else
									{
										if ( _MessageBoxW( hWnd, ST_PROMPT_Ringtones, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION | MB_YESNO ) == IDYES )
										{
											cfg_popup_enable_ringtones = true;
										}
									}
								}
							}
						}
					}

					bool picture_changed = false;

					// First see if we selected a picture.
					if ( g_picture_file_path != NULL )
					{
						// Then see if the contact has a picture set, and if it does, see if it's the same as the one we've selected.
						if ( edit_ci != NULL && ( edit_ci->picture_path != NULL && lstrcmpW( g_picture_file_path, edit_ci->picture_path ) == 0 ) )
						{
							// If the picture paths are the same, then free the selected one.
							GlobalFree( g_picture_file_path );

							g_picture_file_path = NULL;
						}
						else	// If we're updating (and the paths differ), or we're adding a new contact.
						{
							// Set the contact's picture path.
							ci->picture_path = GlobalStrDupW( g_picture_file_path );

							picture_changed = true;
						}
					}

					if ( edit_ci != NULL )	// Update contact.
					{
						if ( info_changed || picture_changed || remove_picture )
						{
							contact_update_info *cui = ( contact_update_info * )GlobalAlloc( GMEM_FIXED, sizeof( contact_update_info ) );
							cui->action = 3;	// Add = 0, 1 = Remove, 2 = Add all tree items, 3 = Update
							cui->old_ci = edit_ci;
							cui->new_ci = ci;
							cui->remove_picture = remove_picture;

							// Update this contact.
							HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )cui, 0, NULL );
							if ( thread != NULL )
							{
								CloseHandle( thread );
							}
							else
							{
								free_contactinfo( &ci );
								GlobalFree( cui );
							}
						}
						else
						{
							free_contactinfo( &ci );
						}
					}
					else	// Add contact.
					{
						// Freed in the update_contact_list thread.
						contact_update_info *cui = ( contact_update_info * )GlobalAlloc( GMEM_FIXED, sizeof( contact_update_info ) );
						cui->action = 0;	// Add = 0, 1 = Remove, 2 = Add all tree items, 3 = Update
						cui->old_ci = ci;
						cui->new_ci = NULL;
						cui->remove_picture = false;

						// Add the contact to the listview. The only values this creates are the wide character phone values. Everything else will have been set.
						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )cui, 0, NULL );
						if ( thread != NULL )
						{
							CloseHandle( thread );
						}
						else
						{
							free_contactinfo( &ci );
							GlobalFree( cui );
						}
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case CB_RINGTONES:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						if ( _SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 ) == 0 )
						{
							_EnableWindow( g_hWnd_play_ringtone, FALSE );
							_EnableWindow( g_hWnd_stop_ringtone, FALSE );
						}
						else
						{
							_EnableWindow( g_hWnd_play_ringtone, TRUE );
							_EnableWindow( g_hWnd_stop_ringtone, TRUE );
						}

						if ( contact_ringtone_played )
						{
							HandleRingtone( RINGTONE_CLOSE, NULL, L"contact" );

							contact_ringtone_played = false;
						}
					}
				}
				break;

				case BTN_PLAY_RINGTONE:
				{
					if ( contact_ringtone_played )
					{
						HandleRingtone( RINGTONE_CLOSE, NULL, L"contact" );

						contact_ringtone_played = false;
					}

					int current_ringtone_index = ( int )_SendMessageW( g_hWnd_ringtones, CB_GETCURSEL, 0, 0 );
					if ( current_ringtone_index > 0 )
					{
						ringtone_info *rti = ( ringtone_info * )_SendMessageW( g_hWnd_ringtones, CB_GETITEMDATA, current_ringtone_index, 0 );
						if ( rti != NULL && rti != ( ringtone_info * )CB_ERR )
						{
							HandleRingtone( RINGTONE_PLAY, rti->ringtone_path, L"contact" );

							contact_ringtone_played = true;
						}
					}
				}
				break;

				case BTN_STOP_RINGTONE:
				{
					if ( contact_ringtone_played )
					{
						HandleRingtone( RINGTONE_CLOSE, NULL, L"contact" );

						contact_ringtone_played = false;
					}
				}
				break;

				case BTN_REMOVE_PICTURE:
				{
					if ( edit_ci != NULL )	// Update contact.
					{
						// Only set it to remove if the contact has a picture set.
						if ( edit_ci->picture_path != NULL )
						{
							remove_picture = true;
						}

						if ( g_hbm_picture != NULL )
						{
							_DeleteObject( g_hbm_picture );
							g_hbm_picture = NULL;

							_InvalidateRect( g_hWnd_static_picture, NULL, TRUE );
						}

						// Disable the remove button.
						_EnableWindow( g_hWnd_remove_picture, FALSE );
					}
				}
				break;

				case BTN_UPDATE_PICTURE:
				{
					wchar_t *picture_file_path = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

					if ( edit_ci != NULL && edit_ci->picture_path != NULL )
					{
						_wcsncpy_s( picture_file_path, MAX_PATH, edit_ci->picture_path, MAX_PATH );
						picture_file_path[ MAX_PATH - 1 ] = 0;	// Sanity.
					}

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"Bitmap Files (*.bmp;*.dib)\0*.bmp;*.dib\0" \
									  L"JPEG (*.jpg;*.jpeg;*.jpe;*.jfif)\0*.jpg;*.jpeg;*.jpe;*.jfif\0" \
									  L"GIF (*.gif)\0*.gif\0" \
									  L"PNG (*.png)\0*.png\0" \
									  L"All Files (*.*)\0*.*\0";
					ofn.nFilterIndex = 2;	// Default filter is for JPEGs.
					ofn.lpstrTitle = ST_Contact_Picture;
					ofn.lpstrFile = picture_file_path;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;

					if ( _GetOpenFileNameW( &ofn ) )
					{
						// Update the selected picture path if it's already set.
						if ( g_picture_file_path != NULL )
						{
							GlobalFree( g_picture_file_path );
						}

						g_picture_file_path = picture_file_path;

						remove_picture = false;	// We have a new picture, so don't remove it.

						if ( g_hbm_picture != NULL )
						{
							_DeleteObject( g_hbm_picture );
							g_hbm_picture = NULL;
						}

						g_picture_height = 0;
						g_picture_width = 0;

						bool convert = true;
						if ( gdiplus_state == GDIPLUS_STATE_SHUTDOWN )
						{
							#ifndef GDIPLUS_USE_STATIC_LIB
								convert = InitializeGDIPlus();
							#else
								StartGDIPlus();
							#endif
						}

						if ( convert )
						{
							g_hbm_picture = ImageToBitmap( g_picture_file_path, g_picture_height, g_picture_width, 96 );
						}

						_InvalidateRect( g_hWnd_static_picture, NULL, TRUE );

						_EnableWindow( g_hWnd_remove_picture, TRUE );
					}
					else
					{
						GlobalFree( picture_file_path );
						picture_file_path = NULL;
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( LOWORD( wParam ) == CW_MODIFY )	// Add/Update contact.
			{
				if ( contact_ringtone_played )
				{
					HandleRingtone( RINGTONE_CLOSE, NULL, L"contact" );

					contact_ringtone_played = false;
				}

				// If we have a window open already and want to switch (add/update)
				// Then clean up and reset any variables that have been set.

				if ( g_hbm_picture != NULL )
				{
					_DeleteObject( g_hbm_picture );
					g_hbm_picture = NULL;
				}

				g_picture_height = 0;
				g_picture_width = 0;

				if ( g_picture_file_path != NULL )
				{
					GlobalFree( g_picture_file_path );
					g_picture_file_path = NULL;
				}

				remove_picture = false;

				edit_ci = ( contact_info * )lParam;

				if ( edit_ci != NULL )
				{
					if ( edit_ci->picture_path != NULL )
					{
						bool convert = true;
						if ( gdiplus_state == GDIPLUS_STATE_SHUTDOWN )
						{
							#ifndef GDIPLUS_USE_STATIC_LIB
								convert = InitializeGDIPlus();
							#else
								StartGDIPlus();
							#endif
						}

						if ( convert )
						{
							g_hbm_picture = ImageToBitmap( edit_ci->picture_path, g_picture_height, g_picture_width, 96 );
						}

						_EnableWindow( g_hWnd_remove_picture, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_remove_picture, FALSE );
					}

					int set_index = 0;

					// Run through our list of ringtones and find a matching file name.
					if ( edit_ci->rti != NULL && edit_ci->rti->ringtone_file != NULL )
					{
						int item_count = ( int )_SendMessageW( g_hWnd_ringtones, CB_GETCOUNT, 0, 0 );
						for ( int i = 1; i < item_count; ++i )
						{
							ringtone_info *rti = ( ringtone_info * )_SendMessageW( g_hWnd_ringtones, CB_GETITEMDATA, i, 0 );
							if ( rti != NULL && rti != ( ringtone_info * )CB_ERR )
							{
								if ( rti->ringtone_file != NULL && lstrcmpW( rti->ringtone_file, edit_ci->rti->ringtone_file ) == 0 )
								{
									set_index = i;

									break;
								}
							}
						}
					}

					_SendMessageW( g_hWnd_ringtones, CB_SETCURSEL, set_index, 0 );

					if ( set_index == 0 )
					{
						_EnableWindow( g_hWnd_play_ringtone, FALSE );
						_EnableWindow( g_hWnd_stop_ringtone, FALSE );
					}
					else
					{
						_EnableWindow( g_hWnd_play_ringtone, TRUE );
						_EnableWindow( g_hWnd_stop_ringtone, TRUE );
					}

					for ( char i = 0, j = 0; i < 16; ++i )
					{
						switch ( i )
						{
							case 0:
							case 4:
							case 6:
							case 10:
							case 11:
							case 15: { _SendMessageW( window_fields[ i ], WM_SETTEXT, 0, ( LPARAM )edit_ci->w_contact_info_phone_numbers[ j ] ); ++j; } break;
							default: { _SendMessageW( window_fields[ i ], WM_SETTEXT, 0, ( LPARAM )edit_ci->w_contact_info_values[ i ] ); } break;
						}
					}

					_SendMessageW( g_hWnd_contact_action, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact );

					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact );
				}
				else	// Add contact.
				{
					edit_ci = NULL;

					_EnableWindow( g_hWnd_remove_picture, FALSE );

					_SendMessageW( g_hWnd_ringtones, CB_SETCURSEL, 0, 0 );
					_EnableWindow( g_hWnd_play_ringtone, FALSE );
					_EnableWindow( g_hWnd_stop_ringtone, FALSE );
					
					for ( char i = 0, j = 0; i < 16; ++i )
					{
						_SendMessageW( window_fields[ i ], WM_SETTEXT, 0, 0 );
					}

					_SendMessageW( g_hWnd_contact_action, WM_SETTEXT, 0, ( LPARAM )ST_Add_Contact );

					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Add_Contact );
				}

				_InvalidateRect( g_hWnd_static_picture, NULL, TRUE );

				_ShowWindow( hWnd, SW_SHOWNORMAL );
			}
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
			remove_picture = false;

			if ( g_hbm_picture != NULL )
			{
				_DeleteObject( g_hbm_picture );
				g_hbm_picture = NULL;
			}

			g_picture_height = 0;
			g_picture_width = 0;

			if ( g_picture_file_path != NULL )
			{
				GlobalFree( g_picture_file_path );
				g_picture_file_path = NULL;
			}

			g_hWnd_contact = NULL;

			if ( contact_ringtone_played )
			{
				HandleRingtone( RINGTONE_CLOSE, NULL, L"contact" );

				contact_ringtone_played = false;
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
