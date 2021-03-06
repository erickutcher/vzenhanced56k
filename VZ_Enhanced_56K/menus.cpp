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
#include "string_tables.h"
#include "menus.h"

#include "connection.h"

HMENU g_hMenu = NULL;					// Handle to our menu bar.

HMENU g_hMenuSub_list_context = NULL;	// Handle to our context menu.
HMENU g_hMenuSub_contact_list_context = NULL;
HMENU g_hMenuSub_allow_ignore_list_context = NULL;
HMENU g_hMenuSub_allow_ignore_cid_list_context = NULL;

HMENU g_hMenuSub_tray = NULL;			// Handle to our tray menu.
HMENU g_hMenuSub_lists = NULL;			// Handle to our lists menu.
HMENU g_hMenuSub_search = NULL;			// Handle to our search sub menu.

HMENU g_hMenuSub_CLL_header_context;	// Handle to our Call Log List header context menu.
HMENU g_hMenuSub_CL_header_context;		// Handle to our Contact List header context menu.
HMENU g_hMenuSub_IL_header_context;		// Handle to our Ignore List header context menu.
HMENU g_hMenuSub_ICIDL_header_context;	// Handle to our Ignore Caller ID List header context menu.
HMENU g_hMenuSub_AL_header_context;		// Handle to our Allow List header context menu.
HMENU g_hMenuSub_ACIDL_header_context;	// Handle to our Allow Caller ID List header context menu.

HMENU g_hMenuSub_tabs_context = NULL;

char context_tab_index = -1;

bool column_l_menu_showing = false;		// Set if we right clicked a subitem and are displaying its copy menu item.
bool column_cl_menu_showing = false;
bool column_il_menu_showing = false;
bool column_il_cid_menu_showing = false;

bool l_phone_menu_showing = false;
bool cl_phone_menu_showing = false;
bool il_phone_menu_showing = false;

bool l_incoming_menu_showing = false;
bool cl_url_menu_showing = false;

void DestroyMenus()
{
	_DestroyMenu( g_hMenuSub_tray );
	_DestroyMenu( g_hMenuSub_lists );
	_DestroyMenu( g_hMenuSub_search );

	_DestroyMenu( g_hMenuSub_CLL_header_context );
	_DestroyMenu( g_hMenuSub_CL_header_context );
	_DestroyMenu( g_hMenuSub_IL_header_context );
	_DestroyMenu( g_hMenuSub_ICIDL_header_context );
	_DestroyMenu( g_hMenuSub_AL_header_context );
	_DestroyMenu( g_hMenuSub_ACIDL_header_context );

	_DestroyMenu( g_hMenuSub_tabs_context );

	// The current edit menu will automatically be destroyed when its associated window is destroyed.
	HMENU hMenuSub_edit = _GetSubMenu( g_hMenu, 1 );

	if ( hMenuSub_edit != g_hMenuSub_list_context )
	{
		_DestroyMenu( g_hMenuSub_list_context );
	}

	if ( hMenuSub_edit != g_hMenuSub_contact_list_context )
	{
		_DestroyMenu( g_hMenuSub_contact_list_context );
	}

	if ( hMenuSub_edit != g_hMenuSub_allow_ignore_list_context )
	{
		_DestroyMenu( g_hMenuSub_allow_ignore_list_context );
	}

	if ( hMenuSub_edit != g_hMenuSub_allow_ignore_cid_list_context )
	{
		_DestroyMenu( g_hMenuSub_allow_ignore_cid_list_context );
	}
}

void CreateMenus()
{
	// Create our menu objects.
	g_hMenu = _CreateMenu();

	HMENU hMenuSub_file = _CreatePopupMenu();
	HMENU hMenuSub_view = _CreatePopupMenu();
	HMENU hMenuSub_tools = _CreatePopupMenu();
	HMENU hMenuSub_help = _CreatePopupMenu();

	g_hMenuSub_lists = _CreatePopupMenu();
	HMENU g_hMenuSub_tabs = _CreatePopupMenu();
	HMENU g_hMenuSub_contacts = _CreatePopupMenu();
	g_hMenuSub_search = _CreatePopupMenu();

	g_hMenuSub_list_context = _CreatePopupMenu();
	g_hMenuSub_contact_list_context = _CreatePopupMenu();
	g_hMenuSub_allow_ignore_list_context = _CreatePopupMenu();
	g_hMenuSub_allow_ignore_cid_list_context = _CreatePopupMenu();
	g_hMenuSub_tray = _CreatePopupMenu();
	g_hMenuSub_tabs_context = _CreatePopupMenu();

	g_hMenuSub_CLL_header_context = _CreatePopupMenu();
	g_hMenuSub_CL_header_context = _CreatePopupMenu();
	g_hMenuSub_IL_header_context = _CreatePopupMenu();
	g_hMenuSub_ICIDL_header_context = _CreatePopupMenu();
	g_hMenuSub_AL_header_context = _CreatePopupMenu();
	g_hMenuSub_ACIDL_header_context = _CreatePopupMenu();


	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;


	// FILE MENU
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__Save_Call_Log___;
	mii.cch = 17;
	mii.wID = MENU_SAVE_CALL_LOG;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( hMenuSub_file, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_file, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Import___;
	mii.cch = 9;
	mii.wID = MENU_IMPORT_LIST;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 2, TRUE, &mii );

	mii.dwTypeData = ST_Export___;
	mii.cch = 9;
	mii.wID = MENU_EXPORT_LIST;
	_InsertMenuItemW( hMenuSub_file, 3, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_file, 4, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_E_xit;
	mii.cch = 5;
	mii.wID = MENU_EXIT;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 5, TRUE, &mii );

	
	// EDIT SUBMENU - SEARCH
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_800Notes;
	mii.cch = 8;
	mii.wID = MENU_SEARCH_WITH_1;
	_InsertMenuItemW( g_hMenuSub_search, 0, TRUE, &mii );

	mii.dwTypeData = ST_Bing;
	mii.cch = 4;
	mii.wID = MENU_SEARCH_WITH_2;
	_InsertMenuItemW( g_hMenuSub_search, 1, TRUE, &mii );

	mii.dwTypeData = ST_Callerr;
	mii.cch = 7;
	mii.wID = MENU_SEARCH_WITH_3;
	_InsertMenuItemW( g_hMenuSub_search, 2, TRUE, &mii );

	mii.dwTypeData = ST_Google;
	mii.cch = 6;
	mii.wID = MENU_SEARCH_WITH_4;
	_InsertMenuItemW( g_hMenuSub_search, 3, TRUE, &mii );

	mii.dwTypeData = ST_Nomorobo;
	mii.cch = 8;
	mii.wID = MENU_SEARCH_WITH_5;
	_InsertMenuItemW( g_hMenuSub_search, 5, TRUE, &mii );

	mii.dwTypeData = ST_OkCaller;
	mii.cch = 8;
	mii.wID = MENU_SEARCH_WITH_6;
	_InsertMenuItemW( g_hMenuSub_search, 6, TRUE, &mii );

	mii.dwTypeData = ST_PhoneTray;
	mii.cch = 9;
	mii.wID = MENU_SEARCH_WITH_7;
	_InsertMenuItemW( g_hMenuSub_search, 7, TRUE, &mii );

	mii.dwTypeData = ST_WhitePages;
	mii.cch = 10;
	mii.wID = MENU_SEARCH_WITH_8;
	_InsertMenuItemW( g_hMenuSub_search, 8, TRUE, &mii );

	mii.dwTypeData = ST_WhoCallsMe;
	mii.cch = 10;
	mii.wID = MENU_SEARCH_WITH_9;
	_InsertMenuItemW( g_hMenuSub_search, 9, TRUE, &mii );

	mii.dwTypeData = ST_WhyCall_me;
	mii.cch = 10;
	mii.wID = MENU_SEARCH_WITH_10;
	_InsertMenuItemW( g_hMenuSub_search, 10, TRUE, &mii );


	// EDIT MENUS
	mii.dwTypeData = ST_Add_to_Allow_Caller_ID_Name_List___;
	mii.cch = 35;
	mii.wID = MENU_ALLOW_CID_LIST;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( g_hMenuSub_lists, 0, TRUE, &mii );

	mii.dwTypeData = ST_Add_to_Allow_Phone_Number_List;
	mii.cch = 30;
	mii.wID = MENU_ALLOW_LIST;
	_InsertMenuItemW( g_hMenuSub_lists, 1, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_lists, 2, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Add_to_Ignore_Caller_ID_Name_List___;
	mii.cch = 36;
	mii.wID = MENU_IGNORE_CID_LIST;
	_InsertMenuItemW( g_hMenuSub_lists, 3, TRUE, &mii );

	mii.dwTypeData = ST_Add_to_Ignore_Phone_Number_List;
	mii.cch = 31;
	mii.wID = MENU_IGNORE_LIST;
	_InsertMenuItemW( g_hMenuSub_lists, 4, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Allow_and_Ignore_Lists;
	mii.cch = 22;
	mii.hSubMenu = g_hMenuSub_lists;
	_InsertMenuItemW( g_hMenuSub_list_context, 0, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_list_context, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Remove_Selected;
	mii.cch = 15;
	mii.wID = MENU_REMOVE_SEL;
	_InsertMenuItemW( g_hMenuSub_list_context, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_list_context, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Copy_Selected;
	mii.cch = 13;
	mii.wID = MENU_COPY_SEL;
	_InsertMenuItemW( g_hMenuSub_list_context, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_list_context, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Select_All;
	mii.cch = 10;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_list_context, 6, TRUE, &mii );



	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Add_Contact___;
	mii.cch = 14;
	mii.wID = MENU_ADD_CONTACT;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 0, TRUE, &mii );

	mii.dwTypeData = ST_Edit_Contact___;
	mii.cch = 15;
	mii.wID = MENU_EDIT_CONTACT;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 1, TRUE, &mii );

	mii.dwTypeData = ST_Remove_Contact;
	mii.cch = 14;
	mii.wID = MENU_REMOVE_SEL;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Copy_Selected;
	mii.cch = 13;
	mii.wID = MENU_COPY_SEL;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Select_All;
	mii.cch = 10;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 6, TRUE, &mii );



	mii.dwTypeData = ST_Add_to_Ignore_Phone_Number_List___;
	mii.cch = 34;
	mii.wID = MENU_ADD_ALLOW_IGNORE_LIST;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 0, TRUE, &mii );

	mii.dwTypeData = ST_Edit_Ignore_Phone_Number_List_Entry___;
	mii.cch = 38;
	mii.wID = MENU_EDIT_ALLOW_IGNORE_LIST;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 1, TRUE, &mii );

	mii.dwTypeData = ST_Remove_from_Ignore_Phone_Number_List;
	mii.cch = 36;
	mii.wID = MENU_REMOVE_SEL;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Copy_Selected;
	mii.cch = 13;
	mii.wID = MENU_COPY_SEL;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Select_All;
	mii.cch = 10;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 6, TRUE, &mii );



	mii.dwTypeData = ST_Add_to_Ignore_Caller_ID_Name_List___;
	mii.cch = 36;
	mii.wID = MENU_ADD_ALLOW_IGNORE_CID_LIST;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 0, TRUE, &mii );

	mii.dwTypeData = ST_Edit_Ignore_Caller_ID_Name_List_Entry___;
	mii.cch = 40;
	mii.wID = MENU_EDIT_ALLOW_IGNORE_CID_LIST;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 1, TRUE, &mii );

	mii.dwTypeData = ST_Remove_from_Ignore_Caller_ID_Name_List;
	mii.cch = 38;
	mii.wID = MENU_REMOVE_SEL;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Copy_Selected;
	mii.cch = 13;
	mii.wID = MENU_COPY_SEL;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Select_All;
	mii.cch = 10;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 6, TRUE, &mii );


	// VIEW SUBMENU - TABS
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Allow_Lists;
	mii.cch = 11;
	mii.wID = MENU_VIEW_ALLOW_LISTS;
	mii.fState = MFS_ENABLED | ( cfg_tab_order1 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 0, TRUE, &mii );

	mii.dwTypeData = ST_Call_Log;
	mii.cch = 8;
	mii.wID = MENU_VIEW_CALL_LOG;
	mii.fState = MFS_ENABLED | ( cfg_tab_order2 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 1, TRUE, &mii );

	mii.dwTypeData = ST_Contact_List;
	mii.cch = 12;
	mii.wID = MENU_VIEW_CONTACT_LIST;
	mii.fState = MFS_ENABLED | ( cfg_tab_order3 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 2, TRUE, &mii );

	mii.dwTypeData = ST_Ignore_Lists;
	mii.cch = 12;
	mii.wID = MENU_VIEW_IGNORE_LISTS;
	mii.fState = MFS_ENABLED | ( cfg_tab_order4 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 3, TRUE, &mii );


	// VIEW MENU
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Tabs;
	mii.cch = 4;
	mii.hSubMenu = g_hMenuSub_tabs;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_view, 0, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_view, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Message_Log;
	mii.cch = 11;
	mii.wID = MENU_MESSAGE_LOG;
	_InsertMenuItemW( hMenuSub_view, 2, TRUE, &mii );


	// TOOLS MENU
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__Search___;
	mii.cch = 10;
	mii.wID = MENU_SEARCH;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_tools, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__Options___;
	mii.cch = 11;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( hMenuSub_tools, 2, TRUE, &mii );


	// HELP MENU
	mii.dwTypeData = ST_VZ_Enhanced_56K__Home_Page;
	mii.cch = 26;
	mii.wID = MENU_VZ_ENHANCED_HOME_PAGE;
	_InsertMenuItemW( hMenuSub_help, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_help, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Check_for__Updates;
	mii.cch = 18;
	mii.wID = MENU_CHECK_FOR_UPDATES;
	_InsertMenuItemW( hMenuSub_help, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_help, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__About;
	mii.cch = 6;
	mii.wID = MENU_ABOUT;
	_InsertMenuItemW( hMenuSub_help, 4, TRUE, &mii );


	// MENU BAR
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = ST__File;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_file;
	_InsertMenuItemW( g_hMenu, 0, TRUE, &mii );

	mii.dwTypeData = ST__Edit;
	mii.cch = 5;
	mii.hSubMenu = g_hMenuSub_list_context;
	_InsertMenuItemW( g_hMenu, 1, TRUE, &mii );

	mii.dwTypeData = ST__View;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_view;
	_InsertMenuItemW( g_hMenu, 2, TRUE, &mii );

	mii.dwTypeData = ST__Tools;
	mii.cch = 6;
	mii.hSubMenu = hMenuSub_tools;
	_InsertMenuItemW( g_hMenu, 3, TRUE, &mii );

	mii.dwTypeData = ST__Help;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_help;
	_InsertMenuItemW( g_hMenu, 4, TRUE, &mii );


	// TRAY MENU (for right click)
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Open_VZ_Enhanced_56K;
	mii.cch = 20;
	mii.wID = MENU_RESTORE;
	mii.fState = MFS_DEFAULT | MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_tray, 0, TRUE, &mii );

	mii.fState = MFS_ENABLED;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Options___;
	mii.cch = 10;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( g_hMenuSub_tray, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Exit;
	mii.cch = 4;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( g_hMenuSub_tray, 4, TRUE, &mii );


	// TAB CONTEXT MENU (for right click)
	mii.dwTypeData = ST_Close_Tab;
	mii.cch = 9;
	mii.wID = MENU_CLOSE_TAB;
	_InsertMenuItemW( g_hMenuSub_tabs_context, 0, TRUE, &mii );

	// ALLOW LIST HEADER CONTEXT MENU (for right click)
	for ( char i = 0; i < NUM_COLUMNS1; ++i )
	{
		mii.dwTypeData = allow_ignore_list_string_table[ i ];
		mii.cch = lstrlenW( allow_ignore_list_string_table[ i ] );
		mii.wID = COLUMN_MENU_OFFSET + i;
		mii.fState = ( *allow_list_columns[ i ] != -1 ? MFS_CHECKED | ( g_total_columns1 > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
		_InsertMenuItemW( g_hMenuSub_AL_header_context, i, TRUE, &mii );
	}

	// ALLOW CALLER ID LIST HEADER CONTEXT MENU (for right click)
	for ( char i = 0; i < NUM_COLUMNS2; ++i )
	{
		mii.dwTypeData = allow_ignore_cid_list_string_table[ i ];
		mii.cch = lstrlenW( allow_ignore_cid_list_string_table[ i ] );
		mii.wID = COLUMN_MENU_OFFSET + i;
		mii.fState = ( *allow_cid_list_columns[ i ] != -1 ? MFS_CHECKED | ( g_total_columns2 > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
		_InsertMenuItemW( g_hMenuSub_ACIDL_header_context, i, TRUE, &mii );
	}

	// CALL LOG LIST HEADER CONTEXT MENU (for right click)
	for ( char i = 0; i < NUM_COLUMNS3; ++i )
	{
		mii.dwTypeData = call_log_string_table[ i ];
		mii.cch = lstrlenW( call_log_string_table[ i ] );
		mii.wID = COLUMN_MENU_OFFSET + i;
		mii.fState = ( *call_log_columns[ i ] != -1 ? MFS_CHECKED | ( g_total_columns3 > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
		_InsertMenuItemW( g_hMenuSub_CLL_header_context, i, TRUE, &mii );
	}

	// CONTACT LIST HEADER CONTEXT MENU (for right click)
	for ( char i = 0; i < NUM_COLUMNS4; ++i )
	{
		mii.dwTypeData = contact_list_string_table[ i ];
		mii.cch = lstrlenW( contact_list_string_table[ i ] );
		mii.wID = COLUMN_MENU_OFFSET + i;
		mii.fState = ( *contact_list_columns[ i ] != -1 ? MFS_CHECKED | ( g_total_columns4 > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
		_InsertMenuItemW( g_hMenuSub_CL_header_context, i, TRUE, &mii );
	}

	// IGNORE LIST HEADER CONTEXT MENU (for right click)
	for ( char i = 0; i < NUM_COLUMNS5; ++i )
	{
		mii.dwTypeData = allow_ignore_list_string_table[ i ];
		mii.cch = lstrlenW( allow_ignore_list_string_table[ i ] );
		mii.wID = COLUMN_MENU_OFFSET + i;
		mii.fState = ( *ignore_list_columns[ i ] != -1 ? MFS_CHECKED | ( g_total_columns5 > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
		_InsertMenuItemW( g_hMenuSub_IL_header_context, i, TRUE, &mii );
	}

	// IGNORE CALLER ID LIST HEADER CONTEXT MENU (for right click)
	for ( char i = 0; i < NUM_COLUMNS6; ++i )
	{
		mii.dwTypeData = allow_ignore_cid_list_string_table[ i ];
		mii.cch = lstrlenW( allow_ignore_cid_list_string_table[ i ] );
		mii.wID = COLUMN_MENU_OFFSET + i;
		mii.fState = ( *ignore_cid_list_columns[ i ] != -1 ? MFS_CHECKED | ( g_total_columns6 > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
		_InsertMenuItemW( g_hMenuSub_ICIDL_header_context, i, TRUE, &mii );
	}
}

void ChangeMenuByHWND( HWND hWnd )
{
	// SetMenuItemInfo returns 87 when I try to swap submenus. Don't know why.

	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = ST__Edit;
	mii.cch = 5;

	_RemoveMenu( g_hMenu, 1, MF_BYPOSITION );

	if ( hWnd == g_hWnd_call_log )
	{
		mii.hSubMenu = g_hMenuSub_list_context;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		mii.hSubMenu = g_hMenuSub_contact_list_context;
	}
	else if ( hWnd == g_hWnd_allow_list || hWnd == g_hWnd_ignore_list )
	{
		mii.hSubMenu = g_hMenuSub_allow_ignore_list_context;
	}
	else if ( hWnd == g_hWnd_allow_cid_list || hWnd == g_hWnd_ignore_cid_list )
	{
		mii.hSubMenu = g_hMenuSub_allow_ignore_cid_list_context;
	}

	_InsertMenuItemW( g_hMenu, 1, TRUE, &mii );

	_DrawMenuBar( g_hWnd_main );
}

void HandleRightClick( HWND hWnd )
{
	POINT p;
	_GetCursorPos( &p );

	bool show_search_menu = false;
	bool show_url_menu = false;
	unsigned char menu_offset = 0;

	if ( hWnd == g_hWnd_call_log )	// List item was clicked on.
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient( hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, call_log_columns, NUM_COLUMNS3 );

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;

				MENUITEMINFO mii2;
				_memcpy_s( &mii2, sizeof( MENUITEMINFO ), &mii, sizeof( MENUITEMINFO ) );

				if ( hti.iItem != -1 )
				{
					mii.fState = MFS_ENABLED;
					mii2.fState = MFS_ENABLED;
				}
				else
				{
					mii.fState = MFS_DISABLED;
					mii2.fState = MFS_DISABLED;
				}

				switch ( vindex )
				{
					case 1:
					{
						mii.wID = MENU_COPY_SEL_COL1;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Allow_Caller_ID_Name_States;
							mii.cch = 32;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Allow_Caller_ID_Name_State;
							mii.cch = 31;
						}
					}
					break;

					case 2:
					{
						mii.wID = MENU_COPY_SEL_COL2;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Allow_Phone_Number_States;
							mii.cch = 30;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Allow_Phone_Number_State;
							mii.cch = 29;
						}
					}
					break;

					case 3:
					{
						mii.wID = MENU_COPY_SEL_COL3;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Caller_ID_Names;
							mii.cch = 20;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Caller_ID_Name;
							mii.cch = 19;
						}
					}
					break;

					case 4:
					{
						mii.wID = MENU_COPY_SEL_COL4;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Dates_and_Times;
							mii.cch = 20;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Date_and_Time;
							mii.cch = 18;
						}
					}
					break;

					case 5:
					{
						mii.wID = MENU_COPY_SEL_COL5;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Ignore_Caller_ID_Name_States;
							mii.cch = 33;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Ignore_Caller_ID_Name_State;
							mii.cch = 32;
						}
					}
					break;

					case 6:
					{
						mii.wID = MENU_COPY_SEL_COL6;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Ignore_Phone_Number_States;
							mii.cch = 31;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Ignore_Phone_Number_State;
							mii.cch = 30;
						}
					}
					break;

					case 7:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL17;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL7;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Phone_Numbers;
							mii.cch = 18;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Phone_Number;
							mii.cch = 17;
						}
					}
					break;
				}

				unsigned char incoming_menu_offset = ( l_incoming_menu_showing ? 2 : 0 );
				menu_offset = 4 + incoming_menu_offset;

				if ( l_phone_menu_showing )
				{
					_DeleteMenu( g_hMenuSub_list_context, 1 + incoming_menu_offset, MF_BYPOSITION );	// Separator
					_RemoveMenu( g_hMenuSub_list_context, 0 + incoming_menu_offset, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

					l_phone_menu_showing = false;
				}

				if ( column_l_menu_showing )
				{
					_SetMenuItemInfoW( g_hMenuSub_list_context, menu_offset, TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_list_context, menu_offset, TRUE, &mii );

					column_l_menu_showing = true;
				}

				if ( show_search_menu )
				{
					mii2.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA | MIIM_SUBMENU;
					mii2.fType = MFT_STRING;
					mii2.dwTypeData = ST_Search_with;
					mii2.cch = 11;
					mii2.dwItemData = mii2.wID;
					mii2.wID = MENU_SEARCH_WITH;
					mii2.hSubMenu = g_hMenuSub_search;
					_InsertMenuItemW( g_hMenuSub_list_context, 0 + incoming_menu_offset, TRUE, &mii2 );

					mii2.fMask = MIIM_TYPE;
					mii2.fType = MFT_SEPARATOR;
					_InsertMenuItemW( g_hMenuSub_list_context, 1 + incoming_menu_offset, TRUE, &mii2 );

					l_phone_menu_showing = true;
				}
			}
		}
		else
		{
			if ( l_incoming_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

				l_incoming_menu_showing = false;
			}

			if ( l_phone_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
				_RemoveMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

				l_phone_menu_showing = false;
			}

			if ( column_l_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_list_context, 4, MF_BYPOSITION );

				column_l_menu_showing = false;
			}
		}

		// Display the ignore incoming call menu items for 30 seconds.

		// Retrieve the lParam value from the selected listview item.
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

			display_info *di = ( display_info * )lvi.lParam;

			if ( di != NULL )
			{
				if ( di->process_incoming )
				{
					SYSTEMTIME SystemTime;
					GetLocalTime( &SystemTime );

					FILETIME FileTime;
					SystemTimeToFileTime( &SystemTime, &FileTime );

					//__int64 current_time = 0;
					//_memcpy_s( ( void * )&current_time, sizeof( __int64 ), ( void * )&FileTime, sizeof( __int64 ) );
					ULARGE_INTEGER li;
					li.LowPart = FileTime.dwLowDateTime;
					li.HighPart = FileTime.dwHighDateTime;

					// See if the elapsed time is less than 30 seconds.
					if ( ( li.QuadPart - di->time.QuadPart ) <= ( 30 * FILETIME_TICKS_PER_SECOND ) )
					{
						if ( !l_incoming_menu_showing )
						{
							MENUITEMINFO mii3;
							_memzero( &mii3, sizeof( MENUITEMINFO ) );
							mii3.cbSize = sizeof( MENUITEMINFO );
							mii3.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
							mii3.fType = MFT_STRING;
							mii3.fState = MFS_ENABLED;

							mii3.wID = MENU_INCOMING_IGNORE;
							mii3.dwTypeData = ST_Ignore_Incoming_Call;
							mii3.cch = 20;
							_InsertMenuItemW( g_hMenuSub_list_context, 0, TRUE, &mii3 );

							mii3.fType = MFT_SEPARATOR;
							_InsertMenuItemW( g_hMenuSub_list_context, 1, TRUE, &mii3 );

							l_incoming_menu_showing = true;
						}
					}
					else	// If it's greater than 30 seconds, then remove the menus and disable them from showing again.
					{
						if ( l_incoming_menu_showing )
						{
							_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
							_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

							l_incoming_menu_showing = false;
						}

						di->process_incoming = false;
					}
				}
				else if ( !di->process_incoming && l_incoming_menu_showing )
				{
					_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
					_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

					l_incoming_menu_showing = false;
				}
			}
		}

		// Show our edit context menu as a popup.
		_TrackPopupMenu( g_hMenuSub_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient( hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, contact_list_columns, NUM_COLUMNS4 );

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;

				MENUITEMINFO mii2;
				_memcpy_s( &mii2, sizeof( MENUITEMINFO ), &mii, sizeof( MENUITEMINFO ) );

				if ( hti.iItem != -1 )
				{
					mii.fState = MFS_ENABLED;
					mii2.fState = MFS_ENABLED;
				}
				else
				{
					mii.fState = MFS_DISABLED;
					mii2.fState = MFS_DISABLED;
				}

				switch ( vindex )
				{
					case 1:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL21;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL21;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Cell_Phone_Numbers;
							mii.cch = 23;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Cell_Phone_Number;
							mii.cch = 22;
						}
					}
					break;

					case 2:
					{
						mii.wID = MENU_COPY_SEL_COL22;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Companies;
							mii.cch = 14;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Company;
							mii.cch = 12;
						}
					}
					break;

					case 3:
					{
						mii.wID = MENU_COPY_SEL_COL23;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Departments;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Department;
							mii.cch = 15;
						}
					}
					break;

					case 4:
					{
						mii2.fState = MFS_ENABLED;
						mii2.wID = MENU_OPEN_EMAIL_ADDRESS;
						mii2.dwTypeData = ST_Send_Email___;
						mii2.cch = 13;

						show_url_menu = true;

						mii.wID = MENU_COPY_SEL_COL24;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Email_Addresses;
							mii.cch = 20;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Email_Address;
							mii.cch = 18;
						}
					}
					break;

					case 5:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL25;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL25;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Fax_Numbers;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Fax_Number;
							mii.cch = 15;
						}
					}
					break;

					case 6:
					{
						mii.wID = MENU_COPY_SEL_COL26;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_First_Names;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_First_Name;
							mii.cch = 15;
						}
					}
					break;

					case 7:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL27;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL27;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Home_Phone_Numbers;
							mii.cch = 23;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Home_Phone_Number;
							mii.cch = 22;
						}
					}
					break;

					case 8:
					{
						mii.wID = MENU_COPY_SEL_COL28;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Job_Titles;
							mii.cch = 15;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Job_Title;
							mii.cch = 14;
						}
					}
					break;

					case 9:
					{
						mii.wID = MENU_COPY_SEL_COL29;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Last_Names;
							mii.cch = 15;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Last_Name;
							mii.cch = 14;
						}
					}
					break;

					case 10:
					{
						mii.wID = MENU_COPY_SEL_COL210;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Nicknames;
							mii.cch = 14;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Nickname;
							mii.cch = 13;
						}
					}
					break;

					case 11:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL211;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL211;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Office_Phone_Numbers;
							mii.cch = 25;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Office_Phone_Number;
							mii.cch = 24;
						}
					}
					break;

					case 12:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL212;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL212;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Other_Phone_Numbers;
							mii.cch = 24;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Other_Phone_Number;
							mii.cch = 23;
						}
					}
					break;

					case 13:
					{
						mii.wID = MENU_COPY_SEL_COL213;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Professions;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Profession;
							mii.cch = 15;
						}
					}
					break;

					case 14:
					{
						mii.wID = MENU_COPY_SEL_COL214;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Titles;
							mii.cch = 11;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Title;
							mii.cch = 10;
						}
					}
					break;

					case 15:
					{
						mii2.fState = MFS_ENABLED;
						mii2.wID = MENU_OPEN_WEB_PAGE;
						mii2.dwTypeData = ST_Open_Web_Page;
						mii2.cch = 13;

						show_url_menu = true;

						mii.wID = MENU_COPY_SEL_COL215;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Web_Pages;
							mii.cch = 14;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Web_Page;
							mii.cch = 13;
						}
					}
					break;

					case 16:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL216;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL216;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Work_Phone_Numbers;
							mii.cch = 23;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Work_Phone_Number;
							mii.cch = 22;
						}
					}
					break;
				}

				menu_offset = 4;

				if ( cl_phone_menu_showing )
				{
					_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
					_RemoveMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

					cl_phone_menu_showing = false;
				}
				
				if ( cl_url_menu_showing )
				{
					_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
					_DeleteMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );

					cl_url_menu_showing = false;
				}

				if ( column_cl_menu_showing )
				{
					_SetMenuItemInfoW( g_hMenuSub_contact_list_context, menu_offset, TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_contact_list_context, menu_offset, TRUE, &mii );

					column_cl_menu_showing = true;
				}

				if ( show_search_menu )
				{
					mii2.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA | MIIM_SUBMENU;
					mii2.fType = MFT_STRING;
					mii2.dwTypeData = ST_Search_with;
					mii2.cch = 11;
					mii2.dwItemData = mii2.wID;
					mii2.wID = MENU_SEARCH_WITH;
					mii2.hSubMenu = g_hMenuSub_search;
					_InsertMenuItemW( g_hMenuSub_contact_list_context, 0, TRUE, &mii2 );

					mii2.fMask = MIIM_TYPE;
					mii2.fType = MFT_SEPARATOR;
					_InsertMenuItemW( g_hMenuSub_contact_list_context, 1, TRUE, &mii2 );

					cl_phone_menu_showing = true;
				}

				if ( show_url_menu )
				{
					_InsertMenuItemW( g_hMenuSub_contact_list_context, 0, TRUE, &mii2 );

					mii2.fType = MFT_SEPARATOR;
					_InsertMenuItemW( g_hMenuSub_contact_list_context, 1, TRUE, &mii2 );

					cl_url_menu_showing = true;
				}
			}
		}
		else
		{
			if ( cl_phone_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
				_RemoveMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

				cl_phone_menu_showing = false;
			}

			if ( cl_url_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );

				cl_url_menu_showing = false;
			}

			if ( column_cl_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_contact_list_context, 4, MF_BYPOSITION );

				column_cl_menu_showing = false;
			}
		}

		// Show our edit context menu as a popup.
		_TrackPopupMenu( g_hMenuSub_contact_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_allow_list || hWnd == g_hWnd_ignore_list )
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient( hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex;
				if ( hWnd == g_hWnd_allow_list )
				{
					vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, allow_list_columns, NUM_COLUMNS1 );
				}
				else
				{
					vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, ignore_list_columns, NUM_COLUMNS5 );
				}

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;

				MENUITEMINFO mii2;
				_memcpy_s( &mii2, sizeof( MENUITEMINFO ), &mii, sizeof( MENUITEMINFO ) );

				if ( hti.iItem != -1 )
				{
					mii.fState = MFS_ENABLED;
					mii2.fState = MFS_ENABLED;
				}
				else
				{
					mii.fState = MFS_DISABLED;
					mii2.fState = MFS_DISABLED;
				}

				switch ( vindex )
				{
					case 1:
					{
						mii.wID = MENU_COPY_SEL_COL31;

						mii.dwTypeData = ST_Copy_Last_Called;
						mii.cch = 16;
					}
					break;

					case 2:
					{
						mii2.wID = MENU_PHONE_NUMBER_COL32;

						show_search_menu = true;

						mii.wID = MENU_COPY_SEL_COL32;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Phone_Numbers;
							mii.cch = 18;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Phone_Number;
							mii.cch = 17;
						}
					}
					break;

					case 3:
					{
						mii.wID = MENU_COPY_SEL_COL33;

						mii.dwTypeData = ST_Copy_Total_Calls;
						mii.cch = 16;
					}
					break;
				}

				menu_offset = 4;

				if ( il_phone_menu_showing )
				{
					_DeleteMenu( g_hMenuSub_allow_ignore_list_context, 1, MF_BYPOSITION );	// Separator
					_RemoveMenu( g_hMenuSub_allow_ignore_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

					il_phone_menu_showing = false;
				}

				if ( column_il_menu_showing )
				{
					_SetMenuItemInfoW( g_hMenuSub_allow_ignore_list_context, menu_offset, TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, menu_offset, TRUE, &mii );

					column_il_menu_showing = true;
				}

				if ( show_search_menu )
				{
					mii2.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA | MIIM_SUBMENU;
					mii2.fType = MFT_STRING;
					mii2.dwTypeData = ST_Search_with;
					mii2.cch = 11;
					mii2.dwItemData = mii2.wID;
					mii2.wID = MENU_SEARCH_WITH;
					mii2.hSubMenu = g_hMenuSub_search;
					_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 0, TRUE, &mii2 );

					mii2.fMask = MIIM_TYPE;
					mii2.fType = MFT_SEPARATOR;
					_InsertMenuItemW( g_hMenuSub_allow_ignore_list_context, 1, TRUE, &mii2 );

					il_phone_menu_showing = true;
				}
			}
		}
		else
		{
			if ( il_phone_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_allow_ignore_list_context, 1, MF_BYPOSITION );	// Separator
				_RemoveMenu( g_hMenuSub_allow_ignore_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

				il_phone_menu_showing = false;
			}

			if ( column_il_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_allow_ignore_list_context, 4, MF_BYPOSITION );

				column_il_menu_showing = false;
			}
		}

		_TrackPopupMenu( g_hMenuSub_allow_ignore_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_allow_cid_list || hWnd == g_hWnd_ignore_cid_list )
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient( hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex;
				if ( hWnd == g_hWnd_allow_cid_list )
				{
					vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, allow_cid_list_columns, NUM_COLUMNS2 );
				}
				else
				{
					vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, ignore_cid_list_columns, NUM_COLUMNS6 );
				}

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;
				mii.fState = ( hti.iItem != -1 ? MFS_ENABLED : MFS_DISABLED );

				switch ( vindex )
				{
					case 1:
					{
						mii.wID = MENU_COPY_SEL_COL41;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Caller_ID_Names;
							mii.cch = 20;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Caller_ID_Name;
							mii.cch = 19;
						}
					}
					break;

					case 2:
					{
						mii.wID = MENU_COPY_SEL_COL42;

						mii.dwTypeData = ST_Copy_Last_Called;
						mii.cch = 16;
					}
					break;

					case 3:
					{
						mii.wID = MENU_COPY_SEL_COL43;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Match_Case_States;
							mii.cch = 22;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Match_Case_State;
							mii.cch = 21;
						}
					}
					break;

					case 4:
					{
						mii.wID = MENU_COPY_SEL_COL44;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Match_Whole_Word_States;
							mii.cch = 28;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Match_Whole_Word_State;
							mii.cch = 27;
						}
					}
					break;

					case 5:
					{
						mii.wID = MENU_COPY_SEL_COL45;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Regular_Expressions;
							mii.cch = 24;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Regular_Expression;
							mii.cch = 23;
						}
					}
					break;

					case 6:
					{
						mii.wID = MENU_COPY_SEL_COL46;

						mii.dwTypeData = ST_Copy_Total_Calls;
						mii.cch = 16;
					}
					break;
				}

				if ( column_il_cid_menu_showing )
				{
					_SetMenuItemInfoW( g_hMenuSub_allow_ignore_cid_list_context, 4, TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_allow_ignore_cid_list_context, 4, TRUE, &mii );

					column_il_cid_menu_showing = true;
				}
			}
		}
		else
		{
			if ( column_il_cid_menu_showing )
			{
				_DeleteMenu( g_hMenuSub_allow_ignore_cid_list_context, 4, MF_BYPOSITION );

				column_il_cid_menu_showing = false;
			}
		}

		_TrackPopupMenu( g_hMenuSub_allow_ignore_cid_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_tab )
	{
		TCHITTESTINFO tcht;
		tcht.flags = TCHT_ONITEM;
		tcht.pt.x = p.x;
		tcht.pt.y = p.y;
		_ScreenToClient( hWnd, &tcht.pt );

		int index = ( int )_SendMessageW( hWnd, TCM_HITTEST, 0, ( LPARAM )&tcht );

		if ( index >= 0 && index < g_total_tabs )
		{
			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM;
			_SendMessageW( hWnd, TCM_GETITEM, index, ( LPARAM )&ti );	// Insert a new tab at the end.

			if ( ( HWND )ti.lParam == g_hWnd_allow_tab )
			{
				context_tab_index = 0;
			}
			else if ( ( HWND )ti.lParam == g_hWnd_call_log )
			{
				context_tab_index = 1;
			}
			else if ( ( HWND )ti.lParam == g_hWnd_contact_list )
			{
				context_tab_index = 2;
			}
			else if ( ( HWND )ti.lParam == g_hWnd_ignore_tab )
			{
				context_tab_index = 3;
			}
			else
			{
				context_tab_index = -1;
			}

			_TrackPopupMenu( g_hMenuSub_tabs_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
	}
	else	// Header control was clicked on.
	{
		HWND hWnd_parent = _GetParent( hWnd );

		// Show our columns context menu as a popup.
		if ( hWnd_parent == g_hWnd_call_log )
		{
			_TrackPopupMenu( g_hMenuSub_CLL_header_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
		else if ( hWnd_parent == g_hWnd_contact_list )
		{
			_TrackPopupMenu( g_hMenuSub_CL_header_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
		else if ( hWnd_parent == g_hWnd_ignore_list )
		{
			_TrackPopupMenu( g_hMenuSub_IL_header_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
		else if ( hWnd_parent == g_hWnd_ignore_cid_list )
		{
			_TrackPopupMenu( g_hMenuSub_ICIDL_header_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
		else if ( hWnd_parent == g_hWnd_allow_list )
		{
			_TrackPopupMenu( g_hMenuSub_AL_header_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
		else if ( hWnd_parent == g_hWnd_allow_cid_list )
		{
			_TrackPopupMenu( g_hMenuSub_ACIDL_header_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
	}
}

void UpdateColumns( HWND hWnd, unsigned int menu_id )
{
	int arr[ 17 ];
	int offset = 0;
	int index = 0;
	unsigned char menu_index = menu_id - COLUMN_MENU_OFFSET;

	char **columns;
	unsigned char *total_columns;
	unsigned char num_columns;
	HMENU menu;
	wchar_t **column_name;
	int **columns_width;

	if ( hWnd == g_hWnd_allow_list )
	{
		columns = allow_list_columns;
		total_columns = &g_total_columns1;
		num_columns = NUM_COLUMNS1;
		menu = g_hMenuSub_AL_header_context;
		column_name = allow_ignore_list_string_table;
		columns_width = allow_list_columns_width;
	}
	else if ( hWnd == g_hWnd_allow_cid_list )
	{
		columns = allow_cid_list_columns;
		total_columns = &g_total_columns2;
		num_columns = NUM_COLUMNS2;
		menu = g_hMenuSub_ACIDL_header_context;
		column_name = allow_ignore_cid_list_string_table;
		columns_width = allow_cid_list_columns_width;
	}
	else if ( hWnd == g_hWnd_call_log )
	{
		columns = call_log_columns;
		total_columns = &g_total_columns3;
		num_columns = NUM_COLUMNS3;
		menu = g_hMenuSub_CLL_header_context;
		column_name = call_log_string_table;
		columns_width = call_log_columns_width;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		columns = contact_list_columns;
		total_columns = &g_total_columns4;
		num_columns = NUM_COLUMNS4;
		menu = g_hMenuSub_CL_header_context;
		column_name = contact_list_string_table;
		columns_width = contact_list_columns_width;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		columns = ignore_list_columns;
		total_columns = &g_total_columns5;
		num_columns = NUM_COLUMNS5;
		menu = g_hMenuSub_IL_header_context;
		column_name = allow_ignore_list_string_table;
		columns_width = ignore_list_columns_width;
	}
	else if ( hWnd == g_hWnd_ignore_cid_list )
	{
		columns = ignore_cid_list_columns;
		total_columns = &g_total_columns6;
		num_columns = NUM_COLUMNS6;
		menu = g_hMenuSub_ICIDL_header_context;
		column_name = allow_ignore_cid_list_string_table;
		columns_width = ignore_cid_list_columns_width;
	}

	if ( menu_index >= 0 && menu_index <= num_columns )
	{
		_SendMessageW( hWnd, LVM_GETCOLUMNORDERARRAY, *total_columns, ( LPARAM )arr );
		for ( int i = 0; i < num_columns; ++i )
		{
			if ( *columns[ i ] != -1 )
			{
				*columns[ i ] = ( char )arr[ offset++ ];
			}
		}

		if ( *columns[ menu_index ] != -1 )
		{
			_CheckMenuItem( menu, menu_id, MF_UNCHECKED );

			--( *total_columns );

			if ( menu_index == 0 )
			{
				index = 0;
			}
			else if	( ( hWnd == g_hWnd_allow_list && menu_index == ( NUM_COLUMNS1 - 1 ) ) ||
					  ( hWnd == g_hWnd_allow_cid_list && menu_index == ( NUM_COLUMNS2 - 1 ) ) ||
					  ( hWnd == g_hWnd_call_log && menu_index == ( NUM_COLUMNS3 - 1 ) ) ||
					  ( hWnd == g_hWnd_contact_list && menu_index == ( NUM_COLUMNS4 - 1 ) ) ||
					  ( hWnd == g_hWnd_ignore_list && menu_index == ( NUM_COLUMNS5 - 1 ) ) ||
					  ( hWnd == g_hWnd_ignore_cid_list && menu_index == ( NUM_COLUMNS6 - 1 ) ) )
			{
				index = *total_columns;
			}
			else
			{
				index = GetColumnIndexFromVirtualIndex( *columns[ menu_index ], columns, num_columns );
			}

			*columns[ menu_index ] = -1;
			_SendMessageW( hWnd, LVM_DELETECOLUMN, index, 0 );
			
			if ( *total_columns == 1 )
			{
				// Find the remaining menu item and disable it.
				for ( int j = 0; j < num_columns; ++j )
				{
					if ( *columns[ j ] != -1 )
					{
						_EnableMenuItem( menu, COLUMN_MENU_OFFSET + j, MF_DISABLED );

						break;
					}
				}
			}
		}
		else
		{
			if ( *total_columns == 1 )
			{
				// Find the remaining menu item and disable it.
				for ( int j = 0; j < num_columns; ++j )
				{
					if ( *columns[ j ] != -1 )
					{
						_EnableMenuItem( menu, COLUMN_MENU_OFFSET + j, MF_ENABLED );

						break;
					}
				}
			}

			_CheckMenuItem( menu, menu_id, MF_CHECKED );

			if ( menu_index == 0 )
			{
				*columns[ menu_index ] = 0;

				// Update the virtual indices.
				for ( int j = 1; j < num_columns; ++j )
				{
					if ( *columns[ j ] != -1 )
					{
						++( *( columns[ j ] ) );
					}
				}
			}
			else if	( ( hWnd == g_hWnd_allow_list && menu_index == ( NUM_COLUMNS1 - 1 ) ) ||
					  ( hWnd == g_hWnd_allow_cid_list && menu_index == ( NUM_COLUMNS2 - 1 ) ) ||
					  ( hWnd == g_hWnd_call_log && menu_index == ( NUM_COLUMNS3 - 1 ) ) ||
					  ( hWnd == g_hWnd_contact_list && menu_index == ( NUM_COLUMNS4 - 1 ) ) ||
					  ( hWnd == g_hWnd_ignore_list && menu_index == ( NUM_COLUMNS5 - 1 ) ) ||
					  ( hWnd == g_hWnd_ignore_cid_list && menu_index == ( NUM_COLUMNS6 - 1 ) ) )
			{
				*columns[ menu_index ] = *total_columns; index = *total_columns;
			}
			else
			{
				*columns[ menu_index ] = *total_columns;
				index = GetColumnIndexFromVirtualIndex( *columns[ menu_index ], columns, num_columns );
			}

			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER;
			lvc.iOrder = *columns[ menu_index ];
			lvc.pszText = column_name[ menu_index ];
			lvc.cx = *columns_width[ menu_index ];
			_SendMessageW( hWnd, LVM_INSERTCOLUMN, index, ( LPARAM )&lvc );

			++( *total_columns );
		}
	}
}

// Enable/Disable the appropriate menu items depending on how many items exist as well as selected.
void UpdateMenus( unsigned char action )
{
	// Enable Menus based on which tab is selected.
	HWND hWnd = GetCurrentListView();

	int item_count = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

	if ( hWnd == g_hWnd_call_log )
	{
		// Ignore incoming call.
		if ( l_incoming_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
			_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

			l_incoming_menu_showing = false;
		}

		// Search phone number.
		if ( l_phone_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
			_RemoveMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

			l_phone_menu_showing = false;
		}

		// Copy column entry.
		if ( column_l_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_list_context, 4, MF_BYPOSITION );

			column_l_menu_showing = false;
		}

		// Get the first selected item if it exists.
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		MENUITEMINFO mii;
		_memzero( &mii, sizeof( MENUITEMINFO ) );
		mii.cbSize = sizeof( MENUITEMINFO );
		mii.fMask = MIIM_TYPE;
		mii.fType = MFT_STRING;

		lvi.iItem = ( int )_SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

			display_info *di = ( display_info * )lvi.lParam;

			if ( di != NULL )
			{
				if ( di->allow_cid_match_count > 0 )
				{
					mii.dwTypeData = ST_Remove_from_Allow_Caller_ID_Name_List;
					mii.cch = 37;
					_SetMenuItemInfoW( g_hMenuSub_lists, 0, TRUE, &mii );
				}
				else
				{
					mii.dwTypeData = ST_Add_to_Allow_Caller_ID_Name_List___;
					mii.cch = 35;
					_SetMenuItemInfoW( g_hMenuSub_lists, 0, TRUE, &mii );
				}

				if ( di->allow_phone_number )
				{
					mii.dwTypeData = ST_Remove_from_Allow_Phone_Number_List;
					mii.cch = 35;
					_SetMenuItemInfoW( g_hMenuSub_lists, 1, TRUE, &mii );
				}
				else
				{
					mii.dwTypeData = ST_Add_to_Allow_Phone_Number_List;
					mii.cch = 30;
					_SetMenuItemInfoW( g_hMenuSub_lists, 1, TRUE, &mii );
				}

				if ( di->ignore_cid_match_count > 0 )
				{
					mii.dwTypeData = ST_Remove_from_Ignore_Caller_ID_Name_List;
					mii.cch = 38;
					_SetMenuItemInfoW( g_hMenuSub_lists, 3, TRUE, &mii );
				}
				else
				{
					mii.dwTypeData = ST_Add_to_Ignore_Caller_ID_Name_List___;
					mii.cch = 36;
					_SetMenuItemInfoW( g_hMenuSub_lists, 3, TRUE, &mii );
				}

				if ( di->ignore_phone_number )
				{
					mii.dwTypeData = ST_Remove_from_Ignore_Phone_Number_List;
					mii.cch = 36;
					_SetMenuItemInfoW( g_hMenuSub_lists, 4, TRUE, &mii );
				}
				else
				{
					mii.dwTypeData = ST_Add_to_Ignore_Phone_Number_List;
					mii.cch = 31;
					_SetMenuItemInfoW( g_hMenuSub_lists, 4, TRUE, &mii );
				}
			}
		}
		else
		{
			mii.dwTypeData = ST_Add_to_Allow_Caller_ID_Name_List___;
			mii.cch = 35;
			_SetMenuItemInfoW( g_hMenuSub_lists, 0, TRUE, &mii );

			mii.dwTypeData = ST_Add_to_Allow_Phone_Number_List;
			mii.cch = 30;
			_SetMenuItemInfoW( g_hMenuSub_lists, 1, TRUE, &mii );

			mii.dwTypeData = ST_Add_to_Ignore_Caller_ID_Name_List___;
			mii.cch = 36;
			_SetMenuItemInfoW( g_hMenuSub_lists, 3, TRUE, &mii );

			mii.dwTypeData = ST_Add_to_Ignore_Phone_Number_List;
			mii.cch = 31;
			_SetMenuItemInfoW( g_hMenuSub_lists, 4, TRUE, &mii );
		}

		if ( action == UM_ENABLE && sel_count > 0 )
		{
			_EnableMenuItem( g_hMenuSub_lists, MENU_ALLOW_CID_LIST, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_lists, MENU_ALLOW_LIST, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_lists, MENU_IGNORE_CID_LIST, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_lists, MENU_IGNORE_LIST, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_list_context, MENU_COPY_SEL, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_list_context, MENU_REMOVE_SEL, MF_ENABLED );
		}
		else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_lists, MENU_ALLOW_CID_LIST, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_lists, MENU_ALLOW_LIST, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_lists, MENU_IGNORE_CID_LIST, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_lists, MENU_IGNORE_LIST, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_list_context, MENU_COPY_SEL, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_list_context, MENU_REMOVE_SEL, MF_DISABLED );
		}

		if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_list_context, MENU_SELECT_ALL, MF_DISABLED );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_list_context, MENU_SELECT_ALL, MF_ENABLED );
		}
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		// Search phone number.
		if ( cl_phone_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
			_RemoveMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

			cl_phone_menu_showing = false;
		}

		// Open webpage.
		if ( cl_url_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
			_DeleteMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );

			cl_url_menu_showing = false;
		}

		// Copy column entry.
		if ( column_cl_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_contact_list_context, 4, MF_BYPOSITION );

			column_cl_menu_showing = false;
		}

		if ( action == UM_ENABLE && sel_count > 0 )
		{
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_EDIT_CONTACT, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_REMOVE_SEL, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_COPY_SEL, MF_ENABLED );
		}
		else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_EDIT_CONTACT, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_REMOVE_SEL, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_COPY_SEL, MF_DISABLED );
		}

		if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_SELECT_ALL, MF_DISABLED );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_SELECT_ALL, MF_ENABLED );
		}
	}
	else if ( hWnd == g_hWnd_allow_list || hWnd == g_hWnd_ignore_list )
	{
		// Search phone number.
		if ( il_phone_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_allow_ignore_list_context, 1, MF_BYPOSITION );	// Separator
			_RemoveMenu( g_hMenuSub_allow_ignore_list_context, 0, MF_BYPOSITION );	// Remove instead to save the sub menu handle.

			il_phone_menu_showing = false;
		}

		// Copy column entry.
		if ( column_il_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_allow_ignore_list_context, 4, MF_BYPOSITION );

			column_il_menu_showing = false;
		}

		MENUITEMINFO mii;
		_memzero( &mii, sizeof( MENUITEMINFO ) );
		mii.cbSize = sizeof( MENUITEMINFO );
		mii.fMask = MIIM_TYPE;
		mii.fType = MFT_STRING;

		if ( hWnd == g_hWnd_allow_list )
		{
			mii.dwTypeData = ST_Add_to_Allow_Phone_Number_List___;
			mii.cch = 33;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_list_context, 0, TRUE, &mii );

			mii.dwTypeData = ST_Edit_Allow_Phone_Number_List_Entry___;
			mii.cch = 37;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_list_context, 1, TRUE, &mii );

			mii.dwTypeData = ST_Remove_from_Allow_Phone_Number_List;
			mii.cch = 35;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_list_context, 2, TRUE, &mii );
		}
		else
		{
			mii.dwTypeData = ST_Add_to_Ignore_Phone_Number_List___;
			mii.cch = 34;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_list_context, 0, TRUE, &mii );

			mii.dwTypeData = ST_Edit_Ignore_Phone_Number_List_Entry___;
			mii.cch = 38;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_list_context, 1, TRUE, &mii );

			mii.dwTypeData = ST_Remove_from_Ignore_Phone_Number_List;
			mii.cch = 36;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_list_context, 2, TRUE, &mii );
		}

		if ( action == UM_ENABLE && sel_count > 0 )
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_EDIT_ALLOW_IGNORE_LIST, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_REMOVE_SEL, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_COPY_SEL, MF_ENABLED );
		}
		else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_EDIT_ALLOW_IGNORE_LIST, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_REMOVE_SEL, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_COPY_SEL, MF_DISABLED );
		}

		if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_SELECT_ALL, MF_DISABLED );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_list_context, MENU_SELECT_ALL, MF_ENABLED );
		}
	}
	else if ( hWnd == g_hWnd_allow_cid_list || hWnd == g_hWnd_ignore_cid_list )
	{
		// Copy column entry.
		if ( column_il_cid_menu_showing )
		{
			_DeleteMenu( g_hMenuSub_allow_ignore_cid_list_context, 4, MF_BYPOSITION );

			column_il_cid_menu_showing = false;
		}

		MENUITEMINFO mii;
		_memzero( &mii, sizeof( MENUITEMINFO ) );
		mii.cbSize = sizeof( MENUITEMINFO );
		mii.fMask = MIIM_TYPE;
		mii.fType = MFT_STRING;

		if ( hWnd == g_hWnd_allow_cid_list )
		{
			mii.dwTypeData = ST_Add_to_Allow_Caller_ID_Name_List___;
			mii.cch = 35;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_cid_list_context, 0, TRUE, &mii );

			mii.dwTypeData = ST_Edit_Allow_Caller_ID_Name_List_Entry___;
			mii.cch = 39;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_cid_list_context, 1, TRUE, &mii );

			mii.dwTypeData = ST_Remove_from_Allow_Caller_ID_Name_List;
			mii.cch = 37;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_cid_list_context, 2, TRUE, &mii );
		}
		else
		{
			mii.dwTypeData = ST_Add_to_Ignore_Caller_ID_Name_List___;
			mii.cch = 36;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_cid_list_context, 0, TRUE, &mii );

			mii.dwTypeData = ST_Edit_Ignore_Caller_ID_Name_List_Entry___;
			mii.cch = 40;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_cid_list_context, 1, TRUE, &mii );

			mii.dwTypeData = ST_Remove_from_Ignore_Caller_ID_Name_List;
			mii.cch = 38;
			_SetMenuItemInfoW( g_hMenuSub_allow_ignore_cid_list_context, 2, TRUE, &mii );
		}

		if ( action == UM_ENABLE && sel_count > 0 )
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_EDIT_ALLOW_IGNORE_CID_LIST, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_REMOVE_SEL, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_COPY_SEL, MF_ENABLED );
		}
		else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_EDIT_ALLOW_IGNORE_CID_LIST, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_REMOVE_SEL, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_COPY_SEL, MF_DISABLED );
		}

		if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_SELECT_ALL, MF_DISABLED );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_allow_ignore_cid_list_context, MENU_SELECT_ALL, MF_ENABLED );
		}
	}
}
