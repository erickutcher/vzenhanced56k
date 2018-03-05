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

#include "lite_dlls.h"
#include "lite_gdiplus.h"
#include "lite_ntdll.h"
#include "lite_gdi32.h"
#include "lite_user32.h"

unsigned char gdiplus_state = GDIPLUS_STATE_SHUTDOWN;

ULONG_PTR gdiplusToken = NULL;
GdiplusStartupInput gdiplusStartupInput;

#ifndef GDIPLUS_USE_STATIC_LIB

	pGdiplusStartup			_GdiplusStartup;
	pGdiplusShutdown		_GdiplusShutdown;
	pGdipLoadImageFromFile	_GdipLoadImageFromFile;
	pGdipGetImageWidth		_GdipGetImageWidth;
	pGdipGetImageHeight		_GdipGetImageHeight;
	pGdipDisposeImage		_GdipDisposeImage;
	pGdipCreateFromHDC		_GdipCreateFromHDC;
	pGdipDrawImageRectI		_GdipDrawImageRectI;
	pGdipDeleteGraphics		_GdipDeleteGraphics;

	HMODULE hModule_gdip = NULL;

	bool InitializeGDIPlus()
	{
		if ( gdiplus_state != GDIPLUS_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_gdip = LoadLibraryDEMW( L"gdiplus.dll" );

		if ( hModule_gdip == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdiplusStartup, "GdiplusStartup" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdiplusShutdown, "GdiplusShutdown" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdipLoadImageFromFile, "GdipLoadImageFromFile" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdipGetImageWidth, "GdipGetImageWidth" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdipGetImageHeight, "GdipGetImageHeight" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdipDisposeImage, "GdipDisposeImage" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdipCreateFromHDC, "GdipCreateFromHDC" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdipDrawImageRectI, "GdipDrawImageRectI" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdip, ( void ** )&_GdipDeleteGraphics, "GdipDeleteGraphics" ) )

		StartGDIPlus();

		//gdiplus_state = GDIPLUS_STATE_RUNNING;

		return true;
	}

	bool UnInitializeGDIPlus()
	{
		if ( gdiplus_state != GDIPLUS_STATE_SHUTDOWN )
		{
			EndGDIPlus();

			return ( FreeLibrary( hModule_gdip ) == FALSE ? false : true );
		}

		return true;
	}

#endif


void StartGDIPlus()
{
	if ( gdiplus_state != GDIPLUS_STATE_RUNNING )
	{
		if ( gdiplusToken == NULL )
		{
			gdiplusStartupInput.GdiplusVersion = 1;
			gdiplusStartupInput.DebugEventCallback = NULL;
			gdiplusStartupInput.SuppressBackgroundThread = FALSE;
			gdiplusStartupInput.SuppressExternalCodecs = FALSE;

			_GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );
		}
	}

	gdiplus_state = GDIPLUS_STATE_RUNNING;
}

void EndGDIPlus()
{
	if ( gdiplus_state != GDIPLUS_STATE_SHUTDOWN )
	{
		if ( gdiplusToken != NULL )
		{
			_GdiplusShutdown( gdiplusToken );
		}
	}

	gdiplus_state = GDIPLUS_STATE_SHUTDOWN;
}

HBITMAP ImageToBitmap( wchar_t *file_path, unsigned int &height, unsigned int &width, unsigned int fixed_height )
{
	GpImage *image = NULL;
	GpGraphics *graphics = NULL;
	HBITMAP hbm = NULL;

	_GdipLoadImageFromFile( file_path, &image );

	_GdipGetImageHeight( image, &height );
	_GdipGetImageWidth( image, &width );

	float w = ( float )width;
	float h = ( float )height;

	if ( width > fixed_height || height > fixed_height )
	{
		float scale = 0.0f;

		if ( width > height )
		{
			scale = ( float )fixed_height / w;
		}
		else
		{
			scale = ( float )fixed_height / h;
		}

		#ifndef NTDLL_USE_STATIC_LIB

			// Seems like a cheap way to get an integer out of a floating point multiplication.
			w *= scale;
			h *= scale;

			int nw = 0;
			int nh = 0;

			__asm
			{
				fld w;		//; Load the floating point width onto the FPU stack.
				fistp nw;	//; Convert the floating point width into an integer, store it in an integer, and then pop it off the stack.

				fld h;
				fistp nh;
			}

			width = nw;
			height = nh;

		#else

			width = ( int )( w * scale );
			height = ( int )( h * scale );

		#endif
	}

	HDC hDC = _GetDC( NULL );
	HDC hdcMem = _CreateCompatibleDC( hDC );

	// GdipDrawImageRectI draws to hbm. Return this value.
	hbm = _CreateCompatibleBitmap( hDC, width, height );

	_SelectObject( hdcMem, hbm );

	_PatBlt( hdcMem, 0, 0, width, height, WHITENESS );	// Set the background to white.

	_GdipCreateFromHDC( hdcMem, &graphics );	// Sets our graphics object to use the hbm bitmap.

	_GdipDrawImageRectI( graphics, image, 0, 0, width, height );	// Resizes image to height and width and then draws it to graphics.

	// Clean up our objects.
	_GdipDisposeImage( image );
	_GdipDeleteGraphics( graphics );

	_DeleteDC( hdcMem );
	_ReleaseDC( NULL, hDC );

	return hbm;
}
/*
HBITMAP ImageToBitmap( wchar_t *file_path, unsigned int &height, unsigned int &width )
{
	GpImage *image = NULL;
	GpGraphics *graphics = NULL;
	HBITMAP hbm = NULL;

	_GdipLoadImageFromFile( file_path, &image );

	_GdipGetImageHeight( image, &height );
	_GdipGetImageWidth( image, &width );

	float w = ( float )width;
	float h = ( float )height;

	if ( width > 96 || height > 96 )
	{
		float scale = 0.0f;

		if ( width > height )
		{
			scale = 96.0f / w;
		}
		else
		{
			scale = 96.0f / h;
		}

		#ifndef NTDLL_USE_STATIC_LIB

			// Seems like a cheap way to get an integer out of a floating point multiplication.
			w *= scale;
			h *= scale;

			int nw = 0;
			int nh = 0;

			__asm
			{
				fld w;		//; Load the floating point width onto the FPU stack.
				fistp nw;	//; Convert the floating point width into an integer, store it in an integer, and then pop it off the stack.

				fld h;
				fistp nh;
			}

			width = nw;
			height = nh;

		#else

			width = ( int )( w * scale );
			height = ( int )( h * scale );

		#endif
	}

	HDC hDC = _GetDC( NULL );
	HDC hdcMem = _CreateCompatibleDC( hDC );

	// GdipDrawImageRectI draws to hbm. Return this value.
	hbm = _CreateCompatibleBitmap( hDC, width, height );

	_SelectObject( hdcMem, hbm );

	_PatBlt( hdcMem, 0, 0, width, height, WHITENESS );	// Set the background to white.

	_GdipCreateFromHDC( hdcMem, &graphics );	// Sets our graphics object to use the hbm bitmap.

	_GdipDrawImageRectI( graphics, image, 0, 0, width, height );	// Resizes image to height and width and then draws it to graphics.

	// Clean up our objects.
	_GdipDisposeImage( image );
	_GdipDeleteGraphics( graphics );

	_DeleteDC( hdcMem );
	_ReleaseDC( NULL, hDC );

	return hbm;
}
*/
