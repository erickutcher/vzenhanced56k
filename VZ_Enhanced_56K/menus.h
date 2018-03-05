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

#ifndef _MENUS_H
#define _MENUS_H

#define MENU_SAVE_CALL_LOG			1001
#define MENU_EXIT					1002
#define MENU_OPTIONS				1003
#define MENU_ABOUT					1004
#define MENU_SELECT_ALL				1005
#define MENU_REMOVE_SEL				1006
#define MENU_RESTORE				1007
#define MENU_SELECT_COLUMNS			1008
#define MENU_MESSAGE_LOG			1009

#define MENU_IMPORT_LIST			1010
#define MENU_EXPORT_LIST			1011

#define MENU_ADD_CONTACT			1012
#define MENU_EDIT_CONTACT			1013

#define MENU_ADD_IGNORE_LIST		1014
#define MENU_IGNORE_LIST			1015

#define MENU_ADD_IGNORE_CID_LIST	1016
#define MENU_EDIT_IGNORE_CID_LIST	1017
#define MENU_IGNORE_CID_LIST		1018

#define MENU_VIEW_CALL_LOG			1019
#define MENU_VIEW_CONTACT_LIST		1020
#define MENU_VIEW_IGNORE_LISTS		1021

#define MENU_CLOSE_TAB				1022

#define MENU_COPY_SEL				1201

#define MENU_COPY_SEL_COL1			1202
#define MENU_COPY_SEL_COL2			1203
#define MENU_COPY_SEL_COL3			1204
#define MENU_COPY_SEL_COL4			1205
#define MENU_COPY_SEL_COL5			1206

#define MENU_COPY_SEL_COL21			1207
#define MENU_COPY_SEL_COL22			1208
#define MENU_COPY_SEL_COL23			1209
#define MENU_COPY_SEL_COL24			1210
#define MENU_COPY_SEL_COL25			1211
#define MENU_COPY_SEL_COL26			1212
#define MENU_COPY_SEL_COL27			1213
#define MENU_COPY_SEL_COL28			1214
#define MENU_COPY_SEL_COL29			1215
#define MENU_COPY_SEL_COL210		1216
#define MENU_COPY_SEL_COL211		1217
#define MENU_COPY_SEL_COL212		1218
#define MENU_COPY_SEL_COL213		1219
#define MENU_COPY_SEL_COL214		1220
#define MENU_COPY_SEL_COL215		1221
#define MENU_COPY_SEL_COL216		1222

#define MENU_COPY_SEL_COL31			1223
#define MENU_COPY_SEL_COL32			1224

#define MENU_COPY_SEL_COL41			1225
#define MENU_COPY_SEL_COL42			1226
#define MENU_COPY_SEL_COL43			1227
#define MENU_COPY_SEL_COL44			1228

// A phone number column was right clicked.
#define MENU_PHONE_NUMBER_COL15		1401
#define MENU_PHONE_NUMBER_COL18		1402
#define MENU_PHONE_NUMBER_COL110	1403

#define MENU_PHONE_NUMBER_COL21		1404
#define MENU_PHONE_NUMBER_COL25		1405
#define MENU_PHONE_NUMBER_COL27		1406
#define MENU_PHONE_NUMBER_COL211	1407
#define MENU_PHONE_NUMBER_COL212	1408
#define MENU_PHONE_NUMBER_COL216	1409

#define MENU_PHONE_NUMBER_COL31		1410

#define MENU_INCOMING_IGNORE		1501

#define MENU_OPEN_WEB_PAGE			1502
#define MENU_OPEN_EMAIL_ADDRESS		1503

#define MENU_VZ_ENHANCED_HOME_PAGE	1504
#define MENU_CHECK_FOR_UPDATES		1505

#define MENU_SEARCH_WITH			2000
#define MENU_SEARCH_WITH_1			2001
#define MENU_SEARCH_WITH_2			2002
#define MENU_SEARCH_WITH_3			2003
#define MENU_SEARCH_WITH_4			2004
#define MENU_SEARCH_WITH_5			2005
#define MENU_SEARCH_WITH_6			2006
#define MENU_SEARCH_WITH_7			2007
#define MENU_SEARCH_WITH_8			2008
#define MENU_SEARCH_WITH_9			2009
#define MENU_SEARCH_WITH_10			2010

#define UM_DISABLE			0
#define UM_ENABLE			1
#define UM_DISABLE_OVERRIDE	2

void CreateMenus();
void DestroyMenus();
void UpdateMenus( unsigned char action );
void ChangeMenuByHWND( HWND hWnd );

void HandleRightClick( HWND hWnd );

extern HMENU g_hMenu;					// Handle to our menu bar.
extern HMENU g_hMenuSub_tray;			// Handle to our tray menu.

extern char context_tab_index;

#endif
