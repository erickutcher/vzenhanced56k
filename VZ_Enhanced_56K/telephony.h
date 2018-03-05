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

#ifndef _TELEPHONY_H
#define _TELEPHONY_H

#include "lite_tapi32.h"

struct incomingcallinfo
{
	HCALL incoming_call;
	unsigned char state;
};

bool InitializeTelephony();
void UnInitializeTelephony();

THREAD_RETURN TelephonyPoll( void *pArguments );

void IgnoreIncomingCall( HCALL call );

#endif
