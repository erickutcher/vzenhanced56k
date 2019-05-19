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

#ifndef _MENUS_H
#define _MENUS_H

#define MENU_SAVE_CALL_LOG			1001
#define MENU_EXIT					1002
#define MENU_SEARCH					1003
#define MENU_OPTIONS				1004
#define MENU_ABOUT					1005
#define MENU_SELECT_ALL				1006
#define MENU_REMOVE_SEL				1007
#define MENU_RESTORE				1008
#define MENU_SELECT_COLUMNS			1009
#define MENU_MESSAGE_LOG			1010

#define MENU_IMPORT_LIST			1011
#define MENU_EXPORT_LIST			1012

#define MENU_ADD_CONTACT			1013
#define MENU_EDIT_CONTACT			1014

#define MENU_ADD_ALLOW_IGNORE_LIST	1015
#define MENU_EDIT_ALLOW_IGNORE_LIST	1016
#define MENU_ALLOW_LIST				1017
#define MENU_IGNORE_LIST			1018

#define MENU_ADD_ALLOW_IGNORE_CID_LIST	1019
#define MENU_EDIT_ALLOW_IGNORE_CID_LIST	1020
#define MENU_ALLOW_CID_LIST			1021
#define MENU_IGNORE_CID_LIST		1022

#define MENU_VIEW_ALLOW_LISTS		1023
#define MENU_VIEW_CALL_LOG			1024
#define MENU_VIEW_CONTACT_LIST		1026
#define MENU_VIEW_IGNORE_LISTS		1027

#define MENU_CLOSE_TAB				1028

#define MENU_COPY_SEL				1201

#define MENU_COPY_SEL_COL1			1202
#define MENU_COPY_SEL_COL2			1203
#define MENU_COPY_SEL_COL3			1204
#define MENU_COPY_SEL_COL4			1205
#define MENU_COPY_SEL_COL5			1206
#define MENU_COPY_SEL_COL6			1207
#define MENU_COPY_SEL_COL7			1208

#define MENU_COPY_SEL_COL21			1209
#define MENU_COPY_SEL_COL22			1210
#define MENU_COPY_SEL_COL23			1211
#define MENU_COPY_SEL_COL24			1212
#define MENU_COPY_SEL_COL25			1213
#define MENU_COPY_SEL_COL26			1214
#define MENU_COPY_SEL_COL27			1215
#define MENU_COPY_SEL_COL28			1216
#define MENU_COPY_SEL_COL29			1217
#define MENU_COPY_SEL_COL210		1218
#define MENU_COPY_SEL_COL211		1219
#define MENU_COPY_SEL_COL212		1220
#define MENU_COPY_SEL_COL213		1221
#define MENU_COPY_SEL_COL214		1222
#define MENU_COPY_SEL_COL215		1223
#define MENU_COPY_SEL_COL216		1224

#define MENU_COPY_SEL_COL31			1225
#define MENU_COPY_SEL_COL32			1226
#define MENU_COPY_SEL_COL33			1227

#define MENU_COPY_SEL_COL41			1228
#define MENU_COPY_SEL_COL42			1229
#define MENU_COPY_SEL_COL43			1230
#define MENU_COPY_SEL_COL44			1231
#define MENU_COPY_SEL_COL45			1232
#define MENU_COPY_SEL_COL46			1233

// A phone number column was right clicked.
#define MENU_PHONE_NUMBER_COL17		1401

#define MENU_PHONE_NUMBER_COL21		1402
#define MENU_PHONE_NUMBER_COL25		1403
#define MENU_PHONE_NUMBER_COL27		1404
#define MENU_PHONE_NUMBER_COL211	1405
#define MENU_PHONE_NUMBER_COL212	1406
#define MENU_PHONE_NUMBER_COL216	1407

#define MENU_PHONE_NUMBER_COL32		1408

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

#define MENU_COLUMN_1				3000
#define MENU_COLUMN_2				3001
#define MENU_COLUMN_3				3002
#define MENU_COLUMN_4				3003
#define MENU_COLUMN_5				3004
#define MENU_COLUMN_6				3005
#define MENU_COLUMN_7				3006
#define MENU_COLUMN_8				3007
#define MENU_COLUMN_9				3008
#define MENU_COLUMN_10				3009
#define MENU_COLUMN_11				3010
#define MENU_COLUMN_12				3011
#define MENU_COLUMN_13				3012
#define MENU_COLUMN_14				3013
#define MENU_COLUMN_15				3014
#define MENU_COLUMN_16				3015
#define MENU_COLUMN_17				3016

#define COLUMN_MENU_OFFSET			3000

#define UM_DISABLE			0
#define UM_ENABLE			1
#define UM_DISABLE_OVERRIDE	2

void CreateMenus();
void DestroyMenus();
void UpdateMenus( unsigned char action );
void ChangeMenuByHWND( HWND hWnd );

void UpdateColumns( HWND hWnd, unsigned int menu_id );

void HandleRightClick( HWND hWnd );

extern HMENU g_hMenu;					// Handle to our menu bar.
extern HMENU g_hMenuSub_tray;			// Handle to our tray menu.

extern char context_tab_index;

#endif
