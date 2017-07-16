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

#ifndef _GDIPLUSLITE_H
#define _GDIPLUSLITE_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
using namespace Gdiplus::DllExports;
#include <gdiplusflat.h>

//#define GDIPLUS_USE_STATIC_LIB

#define GDIPLUS_STATE_SHUTDOWN		0
#define GDIPLUS_STATE_RUNNING		1

#ifdef GDIPLUS_USE_STATIC_LIB

	//__pragma( comment( lib, "gdiplus.lib" ) )

	#define GpImage					GpImage
	#define GpGraphics				GpGraphics

	#define _GdiplusStartup			GdiplusStartup
	#define _GdiplusShutdown		GdiplusShutdown
	#define _GdipLoadImageFromFile	GdipLoadImageFromFile
	#define _GdipGetImageWidth		GdipGetImageWidth
	#define _GdipGetImageHeight		GdipGetImageHeight
	#define _GdipDisposeImage		GdipDisposeImage
	#define _GdipCreateFromHDC		GdipCreateFromHDC
	#define _GdipDrawImageRectI		GdipDrawImageRectI
	#define _GdipDeleteGraphics		GdipDeleteGraphics

#else

	typedef Status ( WINGDIPAPI *pGdiplusStartup )( ULONG_PTR *token, const GdiplusStartupInput *input, GdiplusStartupOutput *output );
	typedef void ( WINGDIPAPI *pGdiplusShutdown )( ULONG_PTR token );

	typedef GpStatus ( WINGDIPAPI *pGdipLoadImageFromFile )( WCHAR *file, GpImage **image );
	typedef GpStatus ( WINGDIPAPI *pGdipGetImageWidth )( GpImage *image, UINT *width );
	typedef GpStatus ( WINGDIPAPI *pGdipGetImageHeight )( GpImage *image, UINT *height );
	typedef GpStatus ( WINGDIPAPI *pGdipDisposeImage )( GpImage *image );

	typedef GpStatus ( WINGDIPAPI *pGdipCreateFromHDC )( HDC hdc, GpGraphics **graphics );
	typedef GpStatus ( WINGDIPAPI *pGdipDrawImageRectI )( GpGraphics *graphics, GpImage *image, INT x, INT y, INT width, INT height );
	typedef GpStatus ( WINGDIPAPI *pGdipDeleteGraphics )( GpGraphics *graphics );

	extern pGdiplusStartup			_GdiplusStartup;
	extern pGdiplusShutdown			_GdiplusShutdown;
	extern pGdipLoadImageFromFile	_GdipLoadImageFromFile;
	extern pGdipGetImageWidth		_GdipGetImageWidth;
	extern pGdipGetImageHeight		_GdipGetImageHeight;
	extern pGdipDisposeImage		_GdipDisposeImage;
	extern pGdipCreateFromHDC		_GdipCreateFromHDC;
	extern pGdipDrawImageRectI		_GdipDrawImageRectI;
	extern pGdipDeleteGraphics		_GdipDeleteGraphics;

	bool InitializeGDIPlus();
	bool UnInitializeGDIPlus();

#endif

void StartGDIPlus();
void EndGDIPlus();

HBITMAP ImageToBitmap( wchar_t *file_path, unsigned int &height, unsigned int &width, unsigned int fixed_height );
//HBITMAP ImageToBitmap( wchar_t *file_path, unsigned int &height, unsigned int &width );

extern unsigned char gdiplus_state;

#endif
