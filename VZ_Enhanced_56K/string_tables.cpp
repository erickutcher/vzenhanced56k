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

#include "string_tables.h"

// Ordered by month.
wchar_t *month_string_table[] =
{
	L"January",
	L"February",
	L"March",
	L"April",
	L"May",
	L"June",
	L"July",
	L"August",
	L"September",
	L"October",
	L"November",
	L"December"
};

// Ordered by day.
wchar_t *day_string_table[] =
{
	L"Sunday",
	L"Monday",
	L"Tuesday",
	L"Wednesday",
	L"Thursday",
	L"Friday",
	L"Saturday"
};

wchar_t *menu_string_table[] =
{
	L"&About",
	L"&Edit",
	L"&File",
	L"&Help",
	L"&Options...",
	L"&Save Call Log...",
	L"&Search...",
	L"&Tools",
	L"&View",
	L"Add Contact...",
	L"Add to Allow Caller ID Name List...",
	L"Add to Allow Phone Number List",
	L"Add to Allow Phone Number List...",
	L"Add to Ignore Caller ID Name List...",
	L"Add to Ignore Phone Number List",
	L"Add to Ignore Phone Number List...",
	L"Allow and Ignore Lists",
	L"Cancel Import",
	/*L"Cancel Update Download",*/
	L"Check for &Updates",
	L"Close Tab",
	L"Contacts",
	L"Copy Allow Caller ID Name State",
	L"Copy Allow Caller ID Name States",
	L"Copy Allow Phone Number State",
	L"Copy Allow Phone Number States",
	L"Copy Caller ID Name",
	L"Copy Caller ID Names",
	L"Copy Cell Phone Number",
	L"Copy Cell Phone Numbers",
	L"Copy Companies",
	L"Copy Company",
	L"Copy Date and Time",
	L"Copy Dates and Times",
	L"Copy Department",
	L"Copy Departments",
	L"Copy Email Address",
	L"Copy Email Addresses",
	L"Copy Fax Number",
	L"Copy Fax Numbers",
	L"Copy First Name",
	L"Copy First Names",
	L"Copy Home Phone Number",
	L"Copy Home Phone Numbers",
	L"Copy Ignore Caller ID Name State",
	L"Copy Ignore Caller ID Name States",
	L"Copy Ignore Phone Number State",
	L"Copy Ignore Phone Number States",
	L"Copy Job Title",
	L"Copy Job Titles",
	L"Copy Last Called",
	L"Copy Last Name",
	L"Copy Last Names",
	L"Copy Match Case State",
	L"Copy Match Case States",
	L"Copy Match Whole Word State",
	L"Copy Match Whole Word States",
	L"Copy Nickname",
	L"Copy Nicknames",
	L"Copy Office Phone Number",
	L"Copy Office Phone Numbers",
	L"Copy Other Phone Number",
	L"Copy Other Phone Numbers",
	L"Copy Phone Number",
	L"Copy Phone Numbers",
	L"Copy Profession",
	L"Copy Professions",
	L"Copy Reference",
	L"Copy References",
	L"Copy Regular Expression",
	L"Copy Regular Expressions",
	L"Copy Selected",
	L"Copy Sent to Phone Number",
	L"Copy Sent to Phone Numbers",
	L"Copy Title",
	L"Copy Titles",
	L"Copy Total Calls",
	L"Copy Web Page",
	L"Copy Web Pages",
	L"Copy Work Phone Number",
	L"Copy Work Phone Numbers",
	L"E&xit",
	L"Edit Allow Caller ID Name List Entry...",
	L"Edit Allow Phone Number List Entry...",
	L"Edit Contact...",
	L"Edit Ignore Caller ID Name List Entry...",
	L"Edit Ignore Phone Number List Entry...",
	L"Exit",
	L"Export...",
	L"Export Contacts...",
	L"Ignore Incoming Call",
	L"Import...",
	L"Import Contacts...",
	L"Open VZ Enhanced 56K",
	L"Open Web Page",
	L"Options...",
	L"Remove Contact",
	L"Remove from Allow Caller ID Name List",
	L"Remove from Allow Phone Number List",
	L"Remove from Ignore Caller ID Name List",
	L"Remove from Ignore Phone Number List",
	L"Remove Selected",
	L"Search with",
	L"Select All",
	L"Send Email...",
	L"Tabs",
	L"VZ Enhanced 56K &Home Page"
};

wchar_t *search_with_string_table[] =
{
	L"800Notes",
	L"Bing",
	L"Callerr",
	L"Google",
	L"Nomorobo",
	L"OkCaller",
	L"PhoneTray",
	L"WhitePages",
	L"WhoCallsMe",
	L"WhyCall.me"
};

wchar_t *common_message_string_table[] =
{
	/*L"A new version of VZ Enhanced 56K is available.\r\n\r\nWould you like to download it now?",*/
	L"Are you sure you want to remove the selected entries?",
	L"Are you sure you want to remove the selected entries from the contact list?",
	L"Are you sure you want to remove the selected entries from the allow caller ID name list?",
	L"Are you sure you want to remove the selected entries from the allow phone number list?",
	L"Are you sure you want to remove the selected entries from the ignore caller ID name list?",
	L"Are you sure you want to remove the selected entries from the ignore phone number list?",
	/*L"Area code is restricted.",*/
	L"Caller ID name is already in the allow caller ID name list.",
	L"Caller ID name is already in the ignore caller ID name list.",
	L"Contact is already in the contact list.",
	L"No voice / data modem was found on the system.",
	L"Phone number is already in the allow phone number list.",
	L"Phone number is already in the ignore phone number list.",
	L"Please enter a valid caller ID name.",
	L"Please enter a valid phone number.",
	L"Popup windows and ringtones must be enable to play per-contact ringtones.\r\n\r\nWould you like to enable these settings?",
	L"Ringtones must be enable to play per-contact ringtones.\r\n\r\nWould you like to enable this setting?",
	L"There was an error while saving the settings.",
	L"Selected caller ID name is allowed using multiple keywords.\r\n\r\nPlease remove it from the allow caller ID name list.",
	L"Selected phone number is allowed using wildcard digit(s).\r\n\r\nPlease remove it from the allow phone number list.",
	L"Selected caller ID name is ignored using multiple keywords.\r\n\r\nPlease remove it from the ignore caller ID name list.",
	L"Selected phone number is ignored using wildcard digit(s).\r\n\r\nPlease remove it from the ignore phone number list.",
	L"The download could not be completed.\r\n\r\nWould you like to visit the VZ Enhanced 56K home page instead?",
	L"The file(s) could not be imported because the format is incorrect.",
	/*L"The Message Log thread is still running.\r\n\r\nPlease wait for it to terminate, or restart the program.",*/
	L"The modem does not support the necessary features to log and ignore incoming calls.",
	L"The update check could not be completed.\r\n\r\nWould you like to visit the VZ Enhanced 56K home page instead?",
	L"Tree-View image list was not destroyed.",
	L"Voice playback is not supported.",
	/*L"VZ Enhanced 56K is up to date.",*/
	L"You must restart the program for this setting to take effect."
};

wchar_t *contact_string_table[] =
{
	L"Add Contact",
	L"Cell Phone:",
	L"Choose File...",
	L"Company:",
	L"Contact Picture",
	L"Contact Picture:",
	L"Department:",
	L"Email Address:",
	L"Fax:",
	L"First Name:",
	L"Home Phone:",
	L"Job Title:",
	L"Last Name:",
	L"Nickname:",
	L"Office Phone:",
	L"Other Phone:",
	L"Play",
	L"Profession:",
	L"Remove Picture",
	L"Ringtone:",
	L"Stop",
	L"Title:",
	L"Update Contact",
	L"Web Page:",
	L"Work Phone:"
};

wchar_t *update_string_table[] =
{
	L"A new version is available.",
	L"Checking for updates...",
	L"Checking For Updates...",
	L"Download Complete",
	L"Download Failed",
	L"Download Update",
	L"[DOWNLOAD URL]\r\n",
	L"Downloading the update...",
	L"Downloading Update...",
	L"Incomplete Download",
	L"[NOTES]\r\n",
	L"The download has completed.",
	L"The download has failed.",
	L"The update check has failed.",
	L"Up To Date",
	L"Update Check Failed",
	L"[VERSION]\r\n",
	L"VZ Enhanced 56K is up to date."
};

wchar_t *options_string_table[] =
{
	L"...",
	L"12 hour",
	L"24 hour",
	L"Add its phone number to the allow phone number list",
	L"Add its phone number to the ignore phone number list",
	L"Always on top",
	L"Apply",
	L"Background color:",
	L"Bottom Left",
	L"Bottom Right",
	L"Cancel",
	L"Center",
	L"Check for updates upon startup",
	L"Close to System Tray",
	L"Default modem:",
	L"Default recording:",
	L"Default ringtone:",
	L"Delay Time (seconds):",
	L"Drop answered calls after (seconds):",
	L"Enable Call Log history",
	L"Enable popup windows:",
	L"Enable ringtone(s)",
	L"Enable System Tray icon:",
	L"Font:",
	L"Font color:",
	L"Font shadow color:",
	L"General",
	L"Gradient background color:",
	L"Gradient direction:",
	L"Height (pixels):",
	L"Hide window border",
	L"Horizontal",
	L"Justify text:",
	L"Left",
	L"Log events to Message Log",
	L"Minimize to System Tray",
	L"Modem",
	L"Play recording when call is answered",
	L"Popup",
	L"Preview Popup",
	L"Recordings take priority if longer.",
	L"Repeat recording:",
	L"Retries:",
	L"Right",
	L"Screen Position:",
	L"Show contact picture",
	L"Show line:",
	L"Silent startup",
	L"Time format:",
	L"Top Left",
	L"Top Right",
	L"Transparency:",
	L"Vertical",
	L"When a call is allowed by its caller ID name:",
	L"When a call is ignored by its caller ID name:",
	L"Width (pixels):"
};

wchar_t *call_log_string_table[] =
{
	L"#",
	L"Allow Caller ID Name",
	L"Allow Phone Number",
	L"Caller ID Name",
	L"Date and Time",
	L"Ignore Caller ID Name",
	L"Ignore Phone Number",
	L"Phone Number"
};

wchar_t *contact_list_string_table[] =
{
	L"#",
	L"Cell Phone Number",
	L"Company",
	L"Department",
	L"Email Address",
	L"Fax Number",
	L"First Name",
	L"Home Phone Number",
	L"Job Title",
	L"Last Name",
	L"Nickname",
	L"Office Phone Number",
	L"Other Phone Number",
	L"Profession",
	L"Title",
	L"Web Page",
	L"Work Phone Number"
};

wchar_t *allow_ignore_list_string_table[] =
{
	L"#",
	L"Last Called",
	L"Phone Number",
	L"Total Calls"
};

wchar_t *allow_ignore_cid_list_string_table[] =
{
	L"#",
	L"Caller ID Name",
	L"Last Called",
	L"Match Case",
	L"Match Whole Word",
	L"Regular Expression",
	L"Total Calls"
};

wchar_t *message_log_list_string_table[] =
{
	L"#",
	L"Date and Time (UTC)",
	L"Level",
	L"Message"
};

wchar_t *search_string_table[] =
{
	L"Search All",
	L"Search for:",
	L"Search Next",
	L"Search Type"
};

wchar_t *common_string_table[] =
{
	L"\xABmultiple caller ID names\xBB",
	L"\xABmultiple phone numbers\xBB",
	L"Allow Caller ID Name",
	L"Allow Lists",
	L"Allow Phone Number",
	L"Call Log",
	L"Caller ID Name",
	L"Caller ID Names",
	L"Caller ID Name:",
	L"Clear Message Log",
	L"Close",
	L"Contact Information",
	L"Contact List",
	L"Entry #",
	L"Export",
	L"Ignore Caller ID Name",
	L"Ignore Lists",
	L"Ignore Phone Number",
	L"Import",
	L"Last Called",
	L"Match case",
	L"Match whole word",
	L"Message Log",
	L"No",
	L"OK",
	L"Options",
	L"Phone Number",
	L"Phone Numbers",
	L"Phone Number:",
	L"Regular expression",
	L"Save Call Log",
	L"Save Message Log...",
	L"Save Message Log",
	L"Search",
	L"Select Columns",
	L"Time",
	L"Total Calls",
	L"Update Phone Number",
	L"Update Caller ID Name",
	L"Yes"
};

wchar_t *connection_string_table[] =
{
	L"A connection could not be established.\r\n\r\nPlease check your network connection and try again.",
	/*L"A new version of the program is available.",*/
	L"Added contact.",
	/*L"Checking for updates.",*/
	L"Closed connection.",
	L"Deleted contact.",
	/*L"Downloaded update.",*/
	/*L"Downloading update.",*/
	L"File could not be saved.",
	L"Loaded contact list.",
	L"Received incoming caller ID information.",
	L"Reestablishing connection.",
	/*L"The download could not be completed.",*/
	/*L"The program is up to date.",*/
	/*L"The update check could not be completed.",*/
	L"Updated contact information."
};

wchar_t *tapi_string_table[] =
{
	L"Closing line device.",
	L"Detecting available modems.",
	L"Exiting telephony poll.",
	L"Failed to initialize telephony interface.",
	L"Initializing telephony interface.",
	L"Opening line device.",
	L"Shutting down telephony interface.",
	L"Waiting for incoming calls."
};

wchar_t *list_string_table[] =
{
	L"Added caller ID name to allow caller ID name list.",
	L"Added caller ID name(s) to allow caller ID name list.",
	L"Added caller ID name to ignore caller ID name list.",
	L"Added caller ID name(s) to ignore caller ID name list.",
	L"Added phone number to allow phone number list.",
	L"Added phone number(s) to allow phone number list.",
	L"Added phone number to ignore phone number list.",
	L"Added phone number(s) to ignore phone number list.",
	L"Automatic save has completed.",
	L"Exported allow caller ID name list.",
	L"Exported allow phone number list.",
	L"Exported call log history.",
	L"Exported contact list.",
	L"Exported ignore caller ID name list.",
	L"Exported ignore phone number list.",
	L"Imported allow caller ID name list.",
	L"Imported allow phone number list.",
	L"Imported call log history.",
	L"Imported contact list.",
	L"Imported ignore caller ID name list.",
	L"Imported ignore phone number list.",
	L"Loaded allow caller ID name list.",
	L"Loaded allow phone number list.",
	L"Loaded call log history.",
	L"Loaded ignore caller ID name list.",
	L"Loaded ignore phone number list.",
	L"Loaded recording directory.",
	L"Loaded ringtone directory.",
	L"Performing automatic save.",
	L"Removed call log entry / entries.",
	L"Removed caller ID name(s) from allow caller ID name list.",
	L"Removed phone number(s) from allow phone number list.",
	L"Removed caller ID name(s) from ignore caller ID name list.",
	L"Removed phone number(s) from ignore phone number list.",
	L"Updated allowed caller ID name.",
	L"Updated allowed caller ID name's call count.",
	L"Updated allowed phone number.",
	L"Updated allowed phone number's call count.",
	L"Updated ignored caller ID name.",
	L"Updated ignored caller ID name's call count.",
	L"Updated ignored phone number.",
	L"Updated ignored phone number's call count."
};
