// The contents of this file are subject to the Mozilla Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/ 
// 
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
// License for the specific language governing rights and limitations
// under the License. 
// 
// The Original Code is the Berkeley Open Infrastructure for Network Computing. 
// 
// The Initial Developer of the Original Code is the SETI@home project.
// Portions created by the SETI@home project are Copyright (C) 2002
// University of California at Berkeley. All Rights Reserved. 
// 
// Contributor(s):
//

/*		This Code Was Created By Jeff Molofee 2000
 *		A HUGE Thanks To Fredric Echols For Cleaning Up
 *		And Optimizing This Code, Making It More Flexible!
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit My Site At nehe.gamedev.net
 *		Adapted to BOINC by Eric Heien
 */

#include <afxwin.h>
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include <stdio.h>

#include "graphics_api.h"
#include "win_idle_tracker.h"

HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application
int			mouse_thresh = 3;
POINT		initCursorPos;
UINT		BOINC_GFX_MODE_MSG,gfx_timer;

#define GFX_TIMER_ID 1001

GLuint	main_font;			// Base Display List For The Font Set

extern bool using_opengl;
bool	keys[256];
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default
BOOL	win_loop_done=FALSE;			// Bool Variable To Exit Loop
int		counter,old_left,old_top,old_right,old_bottom,cur_gfx_mode,old_gfx_mode;
extern HANDLE hQuitEvent;

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc
DWORD WINAPI win_graphics_event_loop( LPVOID duff );
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool initially_visible);
void ChangeMode( int mode );
BOOL reg_win_class();
BOOL unreg_win_class();

GLvoid BuildFont(GLvoid)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	main_font = glGenLists(256);								// Storage For 96 Characters

	font = CreateFont(	-24,							// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_BOLD,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						"Courier New");					// Font Name

	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 0, 256, main_font);			// Builds 256 Characters
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font
}

GLvoid KillFont(GLvoid)									// Delete The Font List
{
	glDeleteLists(main_font, 256);						// Delete All 96 Characters
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	while(ShowCursor(true) < 0);						// Show Mouse Pointer
	active = false;

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}
}

void ChangeMode( int mode ) {
	HDC			screenDC=NULL;		// Screen Device Context
	RECT		WindowRect;			// Grabs Rectangle Upper Left / Lower Right Values
	bool		initially_visible = false;

	KillGLWindow();

	switch (mode) {
		case MODE_NO_GRAPHICS:
			if (fullscreen) while(ShowCursor(true) < 0);		// Show Mouse Pointer
			GetWindowRect( hWnd, &WindowRect );
			old_left = WindowRect.left;
			old_top = WindowRect.top;
			old_right = WindowRect.right;
			old_bottom = WindowRect.bottom;
			fullscreen = false;
			break;
		case MODE_WINDOW:
			if (fullscreen) while(ShowCursor(true) < 0);		// Show Mouse Pointer
			fullscreen = false;
			WindowRect.left = old_left;
			WindowRect.top = old_top;
			WindowRect.right = old_right;
			WindowRect.bottom = old_bottom;
			initially_visible = true;
			break;
		case MODE_FULLSCREEN:
			fullscreen = true;
			GetWindowRect( hWnd, &WindowRect );
			old_left = WindowRect.left;
			old_top = WindowRect.top;
			old_right = WindowRect.right;
			old_bottom = WindowRect.bottom;
			screenDC=GetDC(NULL);
			WindowRect.left = WindowRect.top = 0;
			WindowRect.right=GetDeviceCaps(screenDC, HORZRES);
			WindowRect.bottom=GetDeviceCaps(screenDC, VERTRES);
			ReleaseDC(NULL, screenDC);
			GetCursorPos(&initCursorPos);					// Store the current mouse pos
			while(ShowCursor(false) >= 0);
			initially_visible = true;
			break;
	}

	old_gfx_mode = cur_gfx_mode;
	cur_gfx_mode = mode;

	CreateGLWindow("BOINC App Window", WindowRect.right-WindowRect.left,
		WindowRect.bottom-WindowRect.top, 16, initially_visible);
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool initially_visible)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height
	HDC			screenDC=NULL;		// Screen Device Context

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_TOPMOST;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"BOINC_OpenGL",						// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	old_left = WindowRect.left;
	old_right = WindowRect.right;
	old_top = WindowRect.top;
	old_bottom = WindowRect.bottom;

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (initially_visible) {
		ShowWindow(hWnd,SW_SHOW);					// Show The Window
		active = true;
	} else {
		ShowWindow(hWnd,SW_HIDE);					// Hide The Window
		active = false;
	}

	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}
	BuildFont();										// Build The Font

	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		case WM_ACTIVATEAPP:						// Watch For App Activate Message
		{
			if (!HIWORD(wParam)) {					// Check Minimization State
				//active=TRUE;						// Program Is Active
			} else {
				//active=FALSE;						// Program Is No Longer Active
			}
			return 0;
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
			// If a key is pressed in full screen mode, go back to old mode
			if (fullscreen) {
				ChangeMode(old_gfx_mode);
			}
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;

		case WM_KEYUP:								// Has A Key Been Released?
			// If a key is pressed in full screen mode, go back to old mode
			if (fullscreen) {
				ChangeMode(old_gfx_mode);
			}
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MOUSEMOVE:
		{
			if (fullscreen) {
				counter--;
				if (counter<=0) {
					POINT pt;
					GetCursorPos(&pt);
					int dx=pt.x-initCursorPos.x; if (dx<0) dx=-dx;
					int dy=pt.y-initCursorPos.y; if (dy<0) dy=-dy;
				    if (dx>mouse_thresh || dy>mouse_thresh) {
						ChangeMode(old_gfx_mode);
				    }
				}
			}
			return 0;
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
			ChangeMode(MODE_NO_GRAPHICS);
			return 0;

		case WM_DESTROY:							// Did We Receive A Destroy Message?
			return 0;

		case WM_SIZE:								// Resize The OpenGL Window
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;

		// If we get a redraw request outside of our normal
		// redraw framework, just fill the window with black
		case WM_PAINT:
			PAINTSTRUCT ps;
		    HDC hdc;
			RECT winRect;

			hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &winRect);
            FillRect(hdc, &winRect, (HBRUSH) GetStockObject(BLACK_BRUSH));
            EndPaint(hWnd, &ps);
			return 0;
	}

	if (uMsg == BOINC_GFX_MODE_MSG) {
		if (lParam != cur_gfx_mode)
			ChangeMode(lParam);

		return 0;
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

DWORD WINAPI win_graphics_event_loop( LPVOID gi ) {
	MSG					msg;		// Windows Message Structure
	clock_t				next_redraw=0;
	GRAPHICS_INFO		gfx_info = *(GRAPHICS_INFO*)gi;

	fullscreen=FALSE;							// Windowed Mode

	// Register window class and graphics mode message
	reg_win_class();
	BOINC_GFX_MODE_MSG = RegisterWindowMessage( "BOINC_GFX_MODE" );

	// Create Our OpenGL Window
	if (!CreateGLWindow("BOINC App Window",gfx_info.xsize,
		gfx_info.ysize,16,false)) {
		return -1;					// Quit this thread if window was not created
	}

	// Initialize the graphics refresh timer
	gfx_timer = SetTimer(NULL, GFX_TIMER_ID, (int)(gfx_info.refresh_period*1000),
		(TIMERPROC)NULL);
	cur_gfx_mode = MODE_NO_GRAPHICS;
	using_opengl = true;

	while(!win_loop_done)					// Loop That Runs While done=FALSE
	{
		if (GetMessage(&msg,NULL,0,0)) {	// Is There A Message Waiting?
			if (msg.message==WM_TIMER) {
				if (active && (clock() > next_redraw)) {	// only draw if the window is visible and enough time has passed
					// Draw The Scene
					RECT win_rect;
					GetWindowRect(hWnd,&win_rect);
					app_render(win_rect.right-win_rect.left,win_rect.bottom-win_rect.top,
						time(NULL));
					SwapBuffers(hDC);        // This seems to take lots of CPU time
					next_redraw = clock()+(int)(gfx_info.refresh_period*CLOCKS_PER_SEC);
				}
			} else {
				TranslateMessage(&msg);			// Translate The Message
				DispatchMessage(&msg);			// Dispatch The Message
			}
		} else {
			win_loop_done = true;
		}
	}

	// Shutdown
	KillGLWindow();				// Kill The Window
	KillTimer(NULL, gfx_timer);	// Stop the graphics timer

	unreg_win_class();

	SetEvent(hQuitEvent);		// Signal to the worker thread that we're quitting
	return (msg.wParam);		// Exit The thread
}

BOOL VerifyPassword(HWND hwnd)
{ // Under NT, we return TRUE immediately. This lets the saver quit,
  // and the system manages passwords. Under '95, we call VerifyScreenSavePwd.
  // This checks the appropriate registry key and, if necessary,
  // pops up a verify dialog
  OSVERSIONINFO osv; osv.dwOSVersionInfoSize=sizeof(osv); GetVersionEx(&osv);
  if (osv.dwPlatformId==VER_PLATFORM_WIN32_NT) return TRUE;
  HINSTANCE hpwdcpl=::LoadLibrary("PASSWORD.CPL");
  if (hpwdcpl==NULL) {return TRUE;}
  typedef BOOL (WINAPI *VERIFYSCREENSAVEPWD)(HWND hwnd);
  VERIFYSCREENSAVEPWD VerifyScreenSavePwd;
  VerifyScreenSavePwd=
      (VERIFYSCREENSAVEPWD)GetProcAddress(hpwdcpl,"VerifyScreenSavePwd");
  if (VerifyScreenSavePwd==NULL)
  { 
    FreeLibrary(hpwdcpl);return TRUE;
  }
  BOOL bres=VerifyScreenSavePwd(hwnd); FreeLibrary(hpwdcpl);
  return bres;
}

BOOL reg_win_class() {
	WNDCLASS	wc;						// Windows Class Structure

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "BOINC_OpenGL";						// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}

	return TRUE;
}

BOOL unreg_win_class() {
	if (!UnregisterClass("BOINC_OpenGL",hInstance))		// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}

	return TRUE;
}

