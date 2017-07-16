/*
	VZ Enhanced 56K is a caller ID notifier that can block phone calls.
	Copyright (C) 2013-2017 Eric Kutcher

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

#ifndef _FILE_OPERATIONS_H
#define _FILE_OPERATIONS_H

#define MAGIC_ID_SETTINGS		"VZE\x00"
#define MAGIC_ID_IGNORE_PN		"VZE\x10"
#define MAGIC_ID_IGNORE_CNAM	"VZE\x11"
#define MAGIC_ID_FORWARD_PN		"VZE\x12"
#define MAGIC_ID_FORWARD_CNAM	"VZE\x13"
#define MAGIC_ID_CONTACT_LIST	"VZE\x14"
#define MAGIC_ID_CALL_LOG		"VZE\x20"

void LoadRecordings( dllrbt_tree *list );
void LoadRingtones( dllrbt_tree *list );

char read_config();
char save_config();

char read_ignore_list( wchar_t *file_path, dllrbt_tree *list );
char read_ignore_cid_list( wchar_t *file_path, dllrbt_tree *list );

char save_ignore_list( wchar_t *file_path );
char save_ignore_cid_list( wchar_t *file_path );

char read_call_log_history( wchar_t *file_path );
char save_call_log_history( wchar_t *file_path );

char save_call_log_csv_file( wchar_t *file_path );

char read_contact_list( wchar_t *file_path, dllrbt_tree *list );
char save_contact_list( wchar_t *file_path );

THREAD_RETURN AutoSave( void *pArguments );	// Saves our call log and lists.

extern CRITICAL_SECTION auto_save_cs;

#endif
