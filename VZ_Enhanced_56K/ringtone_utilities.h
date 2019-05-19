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

#ifndef _RINGTONE_UTILITIES_H
#define _RINGTONE_UTILITIES_H

#define MAX_RINGTONE_COUNT		100
#define PROCESS_RINGTONE_COUNT	32

#define RINGTONE_PLAY	1
#define RINGTONE_CLOSE	2

struct RINGTONE_INFO
{
	wchar_t *file_path;
	wchar_t *alias;
	unsigned char state;
};

THREAD_RETURN RingtoneManager( void *pArguments );

void cleanup_ringtone_queue();

void kill_ringtone_update_thread();

void HandleRingtone( unsigned char state, wchar_t *file_path, wchar_t *alias );

extern HANDLE ringtone_update_trigger_semaphore;

#endif
