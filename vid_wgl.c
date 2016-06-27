/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// vid_wgl.c -- NT GL vid component

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include <commctrl.h>

// === end includes

#define MAX_MODE_LIST		600
#define VID_ROW_SIZE	3
#define WARP_WIDTH		320
#define WARP_HEIGHT		200
#define MAXWIDTH		10000
#define MAXHEIGHT		10000
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

// === end defines

typedef struct {
	modestate_t	type;
	int			width;
	int			height;
	int			modenum;
	int			dib;
	int			fullscreen;
	int			bpp;
	int			refreshrate; //johnfitz
	int			halfscreen;
	char		modedesc[17];
} vmode_t;

typedef struct {
	int			width;
	int			height;
} lmode_t;

lmode_t	lowresmodes[] = {
	{320, 200},
	{320, 240},
	{400, 300},
	{512, 384},
};


qboolean		DDActive;


static vmode_t	modelist[MAX_MODE_LIST];
static int		nummodes;
static vmode_t	*pcurrentmode;
static vmode_t	badmode;

static DEVMODE	gdevmode;
static qboolean	vid_initialized = false;
static qboolean	windowed, leavecurrentmode;
static qboolean vid_canalttab = false;
static qboolean vid_wassuspended = false;
static int		windowed_mouse;
extern qboolean	flex_mouseactive;  // from in_win.c
static HICON	hIcon;



int			DIBWidth, DIBHeight;
RECT		WindowRect;
DWORD		WindowStyle, ExWindowStyle;

HWND	mainwindow, dibwindow;

int			vid_modenum = NO_MODE;
int			vid_realmode;
int			vid_default = MODE_WINDOWED;

#ifdef SUPPORTS_GLVIDEO_MODESWITCH
qboolean	video_options_disabled = false; // Baker 3.93: D3D version loses surface with this, investigate later
#else
qboolean	video_options_disabled = true;
#endif

int			desktop_bpp;  // query this @ startup and when entering the video menu
qboolean	vid_fullscreen_only = false; // Baker 3.99h: This is to allow partial video mode switching if -bpp != desktop_bpp, only available if -window isn't used

static int	windowed_default;
unsigned char	vid_curpal[256*3];
qboolean fullsbardraw = false;

// Baker: begin hwgamma support


unsigned short	*currentgammaramp = NULL;
static unsigned short systemgammaramp[3][256];

qboolean	vid_gammaworks = false;
qboolean	vid_hwgamma_enabled = false;
qboolean	customgamma = false;
void VID_SetDeviceGammaRamp (unsigned short *ramps);
void VID_Gamma_Shutdown (void);
void RestoreHWGamma (void);
cvar_t		vid_hwgammacontrol		= {"vid_hwgammacontrol", "2"};//R00k changed to 2 to support windowed modes!


#ifdef SUPPORTS_ENHANCED_GAMMA
	qboolean using_hwgamma=true; // GLquake
#endif
// Baker end hwgamma support

HDC		maindc;

glvert_t glv;

// ProQuake will now permanently using gl_ztrick 0 as default
// First, d3dquake must have this = 0; second, Intel display adapters hate it
cvar_t	gl_ztrick = {"gl_ztrick","0"};

modestate_t	modestate = MS_UNINIT;

int			window_center_x, window_center_y, window_x, window_y, window_width, window_height;
RECT		window_rect;

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow);

viddef_t	vid;				// global video state


void VID_Menu_Init (void); //johnfitz
void VID_Menu_f (void); //johnfitz
void VID_MenuDraw (void);
void VID_MenuKey (int key);

LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AppActivate(BOOL fActive, BOOL minimize);
char *VID_GetModeDescription (int mode);

void VID_UpdateWindowStatus (void);
//void GL_Init (void);



typedef void (APIENTRY *lp3DFXFUNC) (int, int, int, int, int, const void*);
lp3DFXFUNC glColorTableEXT;
qboolean is8bit = false;




qboolean	vid_locked = false; //johnfitz
int			vid_current_bpp;//R00k

extern	cvar_t	cl_confirmquit; // Baker 3.60



// CVARS BEGIN HERE
//

cvar_t		vid_fullscreen = {"vid_fullscreen", "1", true};
cvar_t		vid_width = {"vid_width", "640", true};
cvar_t		vid_height = {"vid_height", "480", true};

int			vid_bpp;
cvar_t		vid_refreshrate = {"vid_refreshrate", "60", true};
cvar_t		vid_vsync = {"vid_vsync", "1", true};

static 		qboolean update_vsync = false;
//johnfitz



cvar_t		vid_mode = {"vid_mode","0", false};
// Note that 0 is MODE_WINDOWED
cvar_t		_vid_default_mode = {"_vid_default_mode","0", true};
// Note that 3 is MODE_FULLSCREEN_DEFAULT
cvar_t		_vid_default_mode_win = {"_vid_default_mode_win","3", true};
cvar_t		vid_wait = {"vid_wait","0"};
cvar_t		vid_nopageflip = {"vid_nopageflip","0", true};
cvar_t		_vid_wait_override = {"_vid_wait_override", "0", true};
cvar_t		vid_config_x = {"vid_config_x","800", true};
cvar_t		vid_config_y = {"vid_config_y","600", true};
cvar_t		vid_stretch_by_2 = {"vid_stretch_by_2","1", true};
cvar_t		_windowed_mouse = {"_windowed_mouse","1", true};
cvar_t		vid_fullscreen_mode = {"vid_fullscreen_mode","3", true};
cvar_t		vid_windowed_mode = {"vid_windowed_mode","0", true};


typedef BOOL (APIENTRY *SWAPINTERVALFUNCPTR)(int);
SWAPINTERVALFUNCPTR wglSwapIntervalEXT = NULL;


void VID_Vsync_f (void);

void CheckVsyncControlExtensions (void)
{
	extern cvar_t vid_vsync;
	// Baker: Keep the cvar even though it won't work in D3DQuake
	Cvar_RegisterVariable (&vid_vsync, VID_Vsync_f);

	if (COM_CheckParm("-noswapctrl"))
	{
		Con_Warning ("Vertical sync disabled at command line\n");
		return;
	}

#ifdef DX8QUAKE_VSYNC_COMMANDLINE_PARAM
	if (COM_CheckParm("-vsync")) {
		Con_SafePrintf ("\x02Note:");
		Con_Printf (" Vertical sync in fullscreen is on (-vsync param)\n");
		return;
	} else {
		Con_Warning ("Vertical sync in fullscreen is off\n");
		Con_Printf ("Use -vsync in command line to enable vertical sync\n");
		return;
	}
#endif


	if (!CheckExtension("WGL_EXT_swap_control") && !CheckExtension("GL_WIN_swap_hint")) {
		Con_Warning ("Vertical sync not supported (extension not found)\n");
		return;
	}

	if (!(wglSwapIntervalEXT = (void *)wglGetProcAddress("wglSwapIntervalEXT"))) {
				Con_Warning ("vertical sync not supported (wglSwapIntervalEXT failed)\n");
		return;
	}

	Con_Printf("Vsync control extensions found\n");
}


//==========================================================================
// direct draw software compatability stuff

void VID_HandlePause (qboolean pause) {
	// Baker: Leave this comment here
}

void VID_ForceLockState (int lk) {
}

void VID_LockBuffer (void) {
	// Baker: Leave this comment here
}

void VID_UnlockBuffer (void) {
	// Baker: Leave this comment here
}

int VID_ForceUnlockedAndReturnState (void)
{
	return 0;
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height) {
	// Baker: Leave this comment here
}

void D_EndDirectRect (int x, int y, int width, int height) {
	// Baker: Leave this comment here
}

/*
================
VID_UpdateWindowStatus
================
*/
void VID_UpdateWindowStatus (void) {

	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	IN_UpdateClipCursor ();
}


/*
================
CenterWindow
================
*/
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify) {
    int     CenterX, CenterY;

	CenterX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	CenterY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	if (CenterX > CenterY*2)
		CenterX >>= 1;	// dual screens
	CenterX = (CenterX < 0) ? 0: CenterX;
	CenterY = (CenterY < 0) ? 0: CenterY;
	SetWindowPos (hWndCenter, NULL, CenterX, CenterY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);
}

void VID_Consize_f();

/*
================
VID_SetWindowedMode
================
*/
qboolean VID_SetWindowedMode (int modenum) {
	HDC				hdc;
	int				lastmodestate, width, height;
	RECT			rect;

	lastmodestate = modestate;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = modelist[modenum].width;
	WindowRect.bottom = modelist[modenum].height;

	DIBWidth = modelist[modenum].width;
	DIBHeight = modelist[modenum].height;
	// Baker 3.60: full-screeen-windowed from aguirRe's engine
	if (DIBWidth == GetSystemMetrics(SM_CXSCREEN) && DIBHeight == GetSystemMetrics(SM_CYSCREEN))
		WindowStyle = WS_POPUP; // Window covers entire screen; no caption, borders etc
	else
		WindowStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	ExWindowStyle = 0;

	rect = WindowRect;
	AdjustWindowRectEx(&rect, WindowStyle, FALSE, 0);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	// Create the DIB window
	dibwindow = CreateWindowEx (
		 ExWindowStyle,
		 TEXT(ENGINE_NAME), // "WinQuake",
		 va("%s %s %s",ENGINE_NAME, RENDERER_NAME, ENGINE_VERSION), // D3D diff 3 of 14
		 WindowStyle,
		 rect.left, rect.top,
		 width,
		 height,
		 NULL,
		 NULL,
		 global_hInstance,
		 NULL);

	if (!dibwindow)
		Sys_Error ("Couldn't create DIB window");

	// Center and show the DIB window
	CenterWindow(dibwindow, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, false);

	ShowWindow (dibwindow, SW_SHOWDEFAULT);
	UpdateWindow (dibwindow);

	modestate = MS_WINDOWED;

// because we have set the background brush for the window to NULL
// (to avoid flickering when re-sizing the window on the desktop),
// we clear the window to black when created, otherwise it will be
// empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);

	//johnfitz -- stuff
	vid.width = modelist[modenum].width;
	vid.height = modelist[modenum].height;
	//johnfitz

	VID_Consize_f();

	vid.numpages = 2;

	mainwindow = dibwindow;

	SendMessage (mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessage (mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	return true;
}

/*
================
VID_SetFullDIBMode
================
*/
qboolean VID_SetFullDIBMode (int modenum) {
	HDC				hdc;
	int				lastmodestate;
	int				width, height;
	RECT			rect;

	if (!leavecurrentmode) {
		gdevmode.dmFields = DM_BITSPERPEL |
							DM_PELSWIDTH |
							DM_PELSHEIGHT |
							DM_DISPLAYFREQUENCY; //johnfitz -- refreshrate
		gdevmode.dmBitsPerPel = modelist[modenum].bpp;
		gdevmode.dmPelsWidth = modelist[modenum].width << modelist[modenum].halfscreen;
		gdevmode.dmPelsHeight = modelist[modenum].height;
		gdevmode.dmDisplayFrequency = modelist[modenum].refreshrate; //johnfitz -- refreshrate
		gdevmode.dmSize = sizeof (gdevmode);

		if (ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			Sys_Error ("Couldn't set fullscreen DIB mode");
	}

	lastmodestate = modestate;
	modestate = MS_FULLDIB;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = modelist[modenum].width;
	WindowRect.bottom = modelist[modenum].height;

	DIBWidth = modelist[modenum].width;
	DIBHeight = modelist[modenum].height;

	WindowStyle = WS_POPUP;
	ExWindowStyle = 0;

	rect = WindowRect;
	AdjustWindowRectEx(&rect, WindowStyle, FALSE, 0);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	// Create the DIB window
	dibwindow = CreateWindowEx (
		 ExWindowStyle,
		 TEXT(ENGINE_NAME),	// "WinQuake" in the past, now "ProQuake"
		 va("%s %s %s",ENGINE_NAME, RENDERER_NAME, ENGINE_VERSION), // D3D diff 4 of 14
		 WindowStyle,
		 rect.left, rect.top,
		 width,
		 height,
		 NULL,
		 NULL,
		 global_hInstance,
		 NULL);

	if (!dibwindow)
		Sys_Error ("Couldn't create DIB window");

	ShowWindow (dibwindow, SW_SHOWDEFAULT);
	UpdateWindow (dibwindow);

	// Because we have set the background brush for the window to NULL (to avoid flickering when re-sizing the window on the desktop),
	// we clear the window to black when created, otherwise it will be  empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);

	vid.width = modelist[modenum].width;
	vid.height = modelist[modenum].height;

	VID_Consize_f();

	vid.numpages = 2;

// needed because we're not getting WM_MOVE messages fullscreen on NT
	window_x = window_y = 0;
	mainwindow = dibwindow;

	SendMessage (mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessage (mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	return true;
}

/*
================
VID_SetMode
================
*/
extern void IN_StartupMouse (void);
int VID_SetMode (int modenum, unsigned char *palette) {
	int				original_mode, temp;
	qboolean		stat;
    MSG				msg;

	if ((windowed && modenum) || (!windowed && modenum < 1) || (!windowed && modenum >= nummodes))
		Sys_Error ("Bad video mode");

// so Con_Printfs don't mess us up by forcing vid and snd updates
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	S_BlockSound ();
	S_ClearBuffer ();
	CDAudio_Pause ();

	if (vid_modenum == NO_MODE)
		original_mode = windowed_default;
	else
		original_mode = vid_modenum;

	// Set either the fullscreen or windowed mode
	if (modelist[modenum].type == MS_WINDOWED) {
		if (_windowed_mouse.value && key_dest == key_game) {
			stat = VID_SetWindowedMode(modenum);
			IN_ActivateMouse ();
			IN_HideMouse ();
		} else {
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			stat = VID_SetWindowedMode(modenum);
		}
	} else if (modelist[modenum].type == MS_FULLDIB) {
		stat = VID_SetFullDIBMode(modenum);
		IN_ActivateMouse ();
		IN_HideMouse ();
	} else {
		Sys_Error ("VID_SetMode: Bad mode type in modelist");
	}

	window_width = DIBWidth;
	window_height = DIBHeight;
	VID_UpdateWindowStatus ();

	S_UnblockSound ();
	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

	if (!stat) {
		Sys_Error ("Couldn't set video mode");
	}

	// now we try to make sure we get the focus on the mode switch, because sometimes in some systems we don't.
	// We grab the foreground, then finish setting up, pump all our messages, and sleep for a little while
	// to let messages finish bouncing around the system, then we put ourselves at the top of the z order,
	// then grab the foreground again. Who knows if it helps, but it probably doesn't hurt

	SetForegroundWindow (mainwindow);
	VID_SetPaletteOld (palette);
	vid_modenum = modenum;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sleep (100);

	SetWindowPos (mainwindow, HWND_TOP, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS);

	SetForegroundWindow (mainwindow);

// fix the leftover Alt from any Alt-Tab or the like that switched us away
	Key_ClearAllStates ();

	if (!msg_suppress_1)
		Con_SafePrintf ("Video mode %s initialized.\n", VID_GetModeDescription (vid_modenum));

	VID_SetPaletteOld (palette);

	vid.recalc_refdef = 1;

	//R00k mouse died on mode change
	IN_StartupMouse ();

	return true;
}

/*
===============
VID_Vsync_f -- johnfitz
===============
*/
void VID_Vsync_f (void) {
	update_vsync = true;
	//return false;
}

/*
===================
VID_Restart -- johnfitz -- change video modes on the fly
===================
*/
void VID_SyncCvars (void);
// D3D diff 10 of 14
#ifdef D3D_WORKAROUND
void d3dInitSetForce16BitTextures(int force16bitTextures);
void d3dSetMode(int fullscreen, int width, int height, int bpp, int zbpp);
#endif

#ifdef DX8QUAKE
void D3D_WrapResetMode (int newmodenum, qboolean newmode_is_windowed) {
	// Baker: wrap the reset mode with all the stuff we need
	int temp;

	Key_ClearAllStates ();
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	S_BlockSound ();
	S_ClearBuffer ();
	CDAudio_Pause ();

//	ShowWindow (dibwindow, SW_SHOWDEFAULT);
//	UpdateWindow (dibwindow);

//	modestate = MS_WINDOWED;
//	DIBWidth =  modelist[newmodenum].width;
//	DIBHeight = modelist[newmodenum].height;

	// Baker: since we aren't actually do a mode change, but a resize
	//        let's do this here to be safe
//	IN_DeactivateMouse ();
//	IN_ShowMouse ();

	// Set either the fullscreen or windowed mode
	if (modelist[newmodenum].type == MS_WINDOWED) {
		if (_windowed_mouse.value && key_dest == key_game) {
			D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
			vid_modenum = newmodenum;
			modestate = MS_WINDOWED;
			IN_ActivateMouse ();
			IN_HideMouse ();
		} else {
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
			modestate = MS_WINDOWED;
		}
	} else if (modelist[newmodenum].type == MS_FULLDIB) {
		// and reset the mode
		D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
		modestate = MS_FULLDIB;
		IN_ActivateMouse ();
		IN_HideMouse ();
	} else {
		Sys_Error ("VID_SetMode: Bad mode type in modelist");
	}

	// now fill in all the ugly globals that Quake stores the same data in over and over again
	// (this will be different for different engines)


	DIBWidth = vid.width = window_width = WindowRect.right = modelist[newmodenum].width;
	DIBHeight = vid.height = window_height = WindowRect.bottom = modelist[newmodenum].height;
	VID_Consize_f();



	// these also needed
	VID_UpdateWindowStatus ();

	VID_SetPaletteOld (host_basepal);
	Key_ClearAllStates ();
	S_UnblockSound ();
	CDAudio_Resume ();

	scr_disabled_for_loading = temp;
	vid.recalc_refdef = 1;
	IN_StartupMouse();
}
#endif



#if !defined(D3DQ_CANNOT_DO_THIS)
BOOL bSetupPixelFormat(HDC hDC);
#endif
void VID_Restart_f (void)
{
	HDC			hdc;
	HGLRC		hrc;
	int			i;
	qboolean	mode_changed = false;
	vmode_t		oldmode;

	if (vid_locked)
		return;

//
// check cvars against current mode
//
	if (vid_fullscreen.value || vid_fullscreen_only)
	{
		if (modelist[vid_default].type == MS_WINDOWED)
			mode_changed = true;
		else if (modelist[vid_default].refreshrate != (int)vid_refreshrate.value)
			mode_changed = true;
	}
	else
		if (modelist[vid_default].type != MS_WINDOWED)
			mode_changed = true;

	if (modelist[vid_default].width != (int)vid_width.value ||
		modelist[vid_default].height != (int)vid_height.value)
		mode_changed = true;

	if (mode_changed)
	{
//
// decide which mode to set
//
		oldmode = modelist[vid_default];

		if (vid_fullscreen.value || vid_fullscreen_only)
		{
			for (i=1; i<nummodes; i++)
			{
				if (modelist[i].width == (int)vid_width.value &&
					modelist[i].height == (int)vid_height.value &&
					modelist[i].bpp == (int)vid_bpp &&
					modelist[i].refreshrate == (int)vid_refreshrate.value)
				{
					break;
				}
			}

			if (i == nummodes)
			{
				Con_Printf ("%dx%dx%d %dHz is not a valid fullscreen mode\n",
							(int)vid_width.value,
							(int)vid_height.value,
							(int)vid_bpp,
							(int)vid_refreshrate.value);
				return;
			}

			windowed = false;
			vid_default = i;
		}
		else //not fullscreen
		{
			hdc = GetDC (NULL);
			if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
			{
				Con_Printf ("Can't run windowed on non-RGB desktop\n");
				ReleaseDC (NULL, hdc);
				return;
			}
			ReleaseDC (NULL, hdc);

			if (vid_width.value < 320)
			{
				Con_Printf ("Window width can't be less than 320\n");
				return;
			}

			if (vid_height.value < 200)
			{
				Con_Printf ("Window height can't be less than 200\n");
				return;
			}

			modelist[0].width = (int)vid_width.value;
			modelist[0].height = (int)vid_height.value;
			snprintf (modelist[0].modedesc, sizeof(modelist[0].modedesc), "%dx%dx%d %dHz",
					 modelist[0].width,
					 modelist[0].height,
					 modelist[0].bpp,
					 modelist[0].refreshrate);

			windowed = true;
			vid_default = 0;
		}
//
// destroy current window
//

		// Baker: restore gamma after Window is destroyed
		// to avoid Windows desktop looking distorted due
		// during switch
#ifdef SUPPORTS_ENHANCED_GAMMA
		if (using_hwgamma && vid_hwgamma_enabled)
			RestoreHWGamma ();
#endif

#ifdef DX8QUAKE

		// instead of destroying the window, context, etc we just need to resize the window and reset the device for D3D
		// we need this too
		vid_canalttab = false;

		D3D_WrapResetMode (vid_default, windowed);

#else
		hrc = wglGetCurrentContext();
		hdc = wglGetCurrentDC();
		wglMakeCurrent(NULL, NULL);

		vid_canalttab = false;

		if (hdc && dibwindow)
			ReleaseDC (dibwindow, hdc);
		if (modestate == MS_FULLDIB)
			ChangeDisplaySettings (NULL, 0);
		if (maindc && dibwindow)
			ReleaseDC (dibwindow, maindc);
		maindc = NULL;
		if (dibwindow)
			DestroyWindow (dibwindow);
//
// set new mode
//
		VID_SetMode (vid_default, host_basepal);

		maindc = GetDC(mainwindow);
#ifdef D3DQ_WORKAROUND
	{
		int zbpp = vid_bpp > 16 ? 24 : 16;
//		if (COM_CheckParm("-zbpp"))
//		{
//			zbpp = atoi(com_argv[COM_CheckParm("-zbpp")+1]);
//		}
		d3dSetMode(!windowed, modelist[0].width, modelist[0].height, vid_bpp, zbpp);
	}
#else
	bSetupPixelFormat(maindc);
#endif

		// if bpp changes, recreate render context and reload textures
		if (modelist[vid_default].bpp != oldmode.bpp)
		{
			wglDeleteContext (hrc);
			hrc = wglCreateContext (maindc);
			if (!wglMakeCurrent (maindc, hrc))
				Sys_Error ("VID_Restart: wglMakeCurrent failed");
			//Bakertest: TexMgr_ReloadImages ();
			GL_SetupState ();
		}
		else
			if (!wglMakeCurrent (maindc, hrc))
#if 1
			{
				char szBuf[80];
				LPVOID lpMsgBuf;
				DWORD dw = GetLastError();
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
				snprintf(szBuf, sizeof(szBuf), "VID_Restart: wglMakeCurrent failed with error %d: %s", dw, lpMsgBuf);
 				Sys_Error (szBuf);
			}
#else
				Sys_Error ("VID_Restart: wglMakeCurrent failed");
#endif

#endif // !DX8QUAKE
		vid_canalttab = true;

		// Baker: Now that we have created the new window, restore it
#ifdef SUPPORTS_ENHANCED_GAMMA
		if (using_hwgamma && vid_hwgamma_enabled)
			VID_SetDeviceGammaRamp (currentgammaramp);
#endif

		//swapcontrol settings were lost when previous window was destroyed
		VID_Vsync_f ();


		//conwidth and conheight need to be recalculated

		//vid.conwidth = (vid_conwidth.value > 0) ? (int)vid_conwidth.value : vid.width;
		//vid.conwidth = CLAMP (320, vid.conwidth, vid.width);
		//vid.conwidth &= 0xFFFFFFF8;
		//vid.conheight = vid.conwidth * vid.height / vid.width;


//		VID_Consize_f();

	}

//
// keep cvars in line with actual mode
//

	VID_SyncCvars ();
	VID_Consize_f ();

}

void VID_Fullscreen(void) {
	// Only do if video mode switching is enabled?  Yes

	if (!host_initialized)
		return;

	if (modestate == MS_FULLDIB  || vid_fullscreen_only) {
		Con_DPrintf("VID_Fullscreen: Already fullscreen\n");
		return;
	}

	if (vid_locked) {
		Con_Printf("VID_Fullscreen: Video mode switching is locked\n");
		return;
	}

	Cvar_SetValueByRef (&vid_fullscreen, 1);
	VID_Restart_f();
}

void VID_Windowed(void) {
	// Only do if video mode switching is enabled?  Yes

	if (!host_initialized)
		return;

	if (modestate == MS_WINDOWED) {
		Con_DPrintf("VID_Windowed: Already windowed\n");
		return;
	}

	if (vid_locked) {
		Con_Printf("VID_Windowed: Video mode switching is locked\n");
		return;
	}

	//MessageBox(NULL,"Stage1","Stage1",MB_OK);
	Cvar_SetValueByRef (&vid_fullscreen, 0);
	VID_Restart_f();
	//MessageBox(NULL,"Stage2","Stage2",MB_OK);
}

qboolean VID_CanSwitchedToWindowed(void) {

	if (!host_initialized)
		return 0; // can't

	if (modestate == MS_WINDOWED)
		return 0; // can't; already are

	if (vid_locked)
		return 0; // can't switch modes

	if (vid_fullscreen_only)
		return 0; // can't switch to windowed mode

	return 1; // can and we aren't in it already
}

qboolean VID_WindowedSwapAvailable(void) {
	if (!host_initialized)
		return 0; // can't

	if (vid_locked)
		return 0; // can't switch modes

	if (vid_fullscreen_only)
		return 0; // can't switch to/from windowed mode

	return 1; //switchable
}

qboolean VID_isFullscreen(void) {
	if (modestate == MS_WINDOWED)
		return 0;

	return 1; // modestate == MS_FULLDIB

}

/*
================
VID_Test -- johnfitz -- like vid_restart, but asks for confirmation after switching modes
================
*/
void VID_Test_f (void)
{
	vmode_t oldmode;
	qboolean	mode_changed = false;

	if (vid_locked)
		return;
//
// check cvars against current mode
//
	if (vid_fullscreen.value || vid_fullscreen_only)
	{
		if (modelist[vid_default].type == MS_WINDOWED)
			mode_changed = true;
/*		else if (modelist[vid_default].bpp != (int)vid_bpp.value)
			mode_changed = true; */
		else if (modelist[vid_default].refreshrate != (int)vid_refreshrate.value)
			mode_changed = true;
	}
	else
		if (modelist[vid_default].type != MS_WINDOWED)
			mode_changed = true;

	if (modelist[vid_default].width != (int)vid_width.value ||
		modelist[vid_default].height != (int)vid_height.value)
		mode_changed = true;

	if (!mode_changed)
		return;
//
// now try the switch
//
	oldmode = modelist[vid_default];

	VID_Restart_f ();

	//pop up confirmation dialoge
	if (!SCR_ModalMessage("Would you like to keep this\nvideo mode? (y/n)\n", 5.0f))
	{
		//revert cvars and mode
		Cvar_SetStringByRef (&vid_width, va("%i", oldmode.width));
		Cvar_SetStringByRef (&vid_height, va("%i", oldmode.height));
//		Cvar_SetStringByRef (&vid_bpp", va("%i", oldmode.bpp));
		Cvar_SetStringByRef (&vid_refreshrate, va("%i", oldmode.refreshrate));
		Cvar_SetStringByRef (&vid_fullscreen, ((vid_fullscreen_only) ? "1" : (oldmode.type == MS_WINDOWED) ? "0" : "1"));
		VID_Restart_f ();
	}
}


/*
================
VID_Unlock -- johnfitz
================
*/
void VID_Unlock_f (void)
{
	vid_locked = false;

	//sync up cvars in case they were changed during the lock
	Cvar_SetStringByRef (&vid_width, va("%i", modelist[vid_default].width));
	Cvar_SetStringByRef (&vid_height, va("%i", modelist[vid_default].height));
	Cvar_SetStringByRef (&vid_refreshrate, va("%i", modelist[vid_default].refreshrate));
	Cvar_SetStringByRef (&vid_fullscreen, (vid_fullscreen_only) ? "1" : ((windowed) ? "0" : "1"));
}








/*
=================
GL_BeginRendering -- sets values of glx, gly, glwidth, glheight
=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height) {
	*x = *y = 0;
	*width = WindowRect.right - WindowRect.left;
	*height = WindowRect.bottom - WindowRect.top;
}

// D3D diff 5 of 14
#ifdef D3DQ_WORKAROUND
// Call our swap buffer instead of the standard one
void FakeSwapBuffers();
#endif

/*
=================
GL_EndRendering
=================
*/
void GL_EndRendering (void) {
	// Baker: hw gamma support
#ifdef SUPPORTS_ENHANCED_GAMMA
	if (using_hwgamma) {
		static qboolean	old_hwgamma_enabled;

		vid_hwgamma_enabled = vid_hwgammacontrol.value && vid_gammaworks && ActiveApp && !Minimized;
		vid_hwgamma_enabled = vid_hwgamma_enabled && (modestate == MS_FULLDIB || vid_hwgammacontrol.value == 2);
		if (vid_hwgamma_enabled != old_hwgamma_enabled) {
			old_hwgamma_enabled = vid_hwgamma_enabled;
			if (vid_hwgamma_enabled && currentgammaramp)
				VID_SetDeviceGammaRamp (currentgammaramp);
			else
				RestoreHWGamma ();
		}
	}
	// Baker end hwgamma support
#endif

	if (!scr_skipupdate || block_drawing) {

		if (wglSwapIntervalEXT && update_vsync && vid_vsync.string[0])
			wglSwapIntervalEXT (vid_vsync.value ? 1 : 0);
		update_vsync = false;
#if defined(D3DQ_WORKAROUND)  || defined(DX8QUAKE
	FakeSwapBuffers();
#else
		SwapBuffers (maindc);
#endif
	}

// handle the mouse state when windowed if that's changed
	if (modestate == MS_WINDOWED) {
		if (!_windowed_mouse.value) {
			if (windowed_mouse)	{
				IN_DeactivateMouse ();
				IN_ShowMouse ();
				windowed_mouse = false;
			}
		} else {
			windowed_mouse = true;
			if (key_dest == key_game && !flex_mouseactive && ActiveApp) {
				IN_ActivateMouse ();
				IN_HideMouse ();
			} else if (flex_mouseactive && key_dest != key_game) {
				IN_DeactivateMouse ();
				IN_ShowMouse ();
			}
		}
	}
#ifdef RELEASE_MOUSE_FULLSCREEN // Baker release mouse even when fullscreen
	else
	{
			windowed_mouse = true;
			if (key_dest == key_game && !flex_mouseactive && ActiveApp) {
				IN_ActivateMouse ();
				IN_HideMouse ();
			} else if (flex_mouseactive && key_dest != key_game) {
				if (MOUSE_RELEASE_GAME_SAFE) {
					IN_DeactivateMouse ();
					IN_ShowMouse ();
	//				Con_Printf("Debug: Mouse shown\n", flex_mouseactive);
				}
			}
	}
#endif


	if (fullsbardraw)
		Sbar_Changed(); // Baker: WTF

}

void VID_SetDefaultMode (void) {
	IN_DeactivateMouse ();
}

void	VID_Shutdown (void) {
   	HGLRC hRC;
   	HDC	  hDC;

	if (vid_initialized) {
#ifdef SUPPORTS_ENHANCED_GAMMA
		if (using_hwgamma)
			RestoreHWGamma ();
#endif
		vid_canalttab = false;
		hRC = wglGetCurrentContext();
    	hDC = wglGetCurrentDC();

    	wglMakeCurrent(NULL, NULL);

    	if (hRC)
    	    wglDeleteContext(hRC);

		// Baker hwgamma support
#ifdef SUPPORTS_ENHANCED_GAMMA
		if (using_hwgamma)
			VID_Gamma_Shutdown (); //johnfitz
		// Baker end hwgamma support
#endif

		if (hDC && dibwindow)
			ReleaseDC(dibwindow, hDC);

		if (modestate == MS_FULLDIB)
			ChangeDisplaySettings (NULL, 0);

		if (maindc && dibwindow)
			ReleaseDC (dibwindow, maindc);

		AppActivate(false, false);
	}
}



// Baker begin hwgamma support
void VID_ShiftPalette (unsigned char *palette) {}

// Note: ramps must point to a static array
void VID_SetDeviceGammaRamp (unsigned short *ramps) {
	if (vid_gammaworks) {
		currentgammaramp = ramps;
		if (vid_hwgamma_enabled) {
			SetDeviceGammaRamp (maindc, ramps);
			customgamma = true;
		}
	}
}

void InitHWGamma (void) {
#if !defined(SUPPORTS_ENHANCED_GAMMA)
//	Con_Warning ("Hardware gamma not supported in this engine build\n"); // No warning, it is just normal
	return;
#endif

#ifdef SUPPORTS_ENHANCED_GAMMA
	if (!using_hwgamma) {
//		Con_Printf ("Note: Hardware gamma unavailable due to -gamma parameter\n"); // No warning, unnecessary
		return;
	}

	if (COM_CheckParm("-nohwgamma")) {
		Con_Warning ("Hardware gamma disabled at command line\n");
		return;
	}

	if (!GetDeviceGammaRamp (maindc, systemgammaramp)) {
		Con_Warning ("Hardware gamma not available (GetDeviceGammaRamp failed)\n");
		return;
	}

	Con_Success ("Hardware gamma enabled\n");
	vid_gammaworks = true;
#endif
}

void RestoreHWGamma (void) {
	if (vid_gammaworks && customgamma) {
		customgamma = false;
		SetDeviceGammaRamp (maindc, systemgammaramp);
	}
}

/*
================
VID_Gamma_Restore -- restore system gamma
================
*/
void VID_Gamma_Restore (void)
{
	if (maindc)
	{
		if (vid_gammaworks)
			if (SetDeviceGammaRamp(maindc, systemgammaramp) == false)
				Con_Printf ("VID_Gamma_Restore: failed on SetDeviceGammaRamp\n");
	}
}

/*
================
VID_Gamma_Shutdown -- called on exit
================
*/
void VID_Gamma_Shutdown (void)
{
	VID_Gamma_Restore ();
}




// Baker end hwgamma support
// D3D diff 8 of 14
#if !defined(D3DQ_CANNOT_DO_THIS)

BOOL bSetupPixelFormat(HDC hDC) {
    static PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
	1,				// version number
	PFD_DRAW_TO_WINDOW |	// support window
	PFD_SUPPORT_OPENGL |	// support OpenGL
	PFD_DOUBLEBUFFER,		// double buffered
	PFD_TYPE_RGBA,			// RGBA type
	24,				// 24-bit color depth
	0, 0, 0, 0, 0, 0,		// color bits ignored
	0,				// no alpha buffer
	0,				// shift bit ignored
	0,				// no accumulation buffer
	0, 0, 0, 0, 			// accum bits ignored
	32,				// 32-bit z-buffer
	0,				// no stencil buffer
	0,				// no auxiliary buffer
	PFD_MAIN_PLANE,			// main layer
	0,				// reserved
	0, 0, 0				// layer masks ignored
    };
    int pixelformat;

    if ((pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0 ) {
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (SetPixelFormat(hDC, pixelformat, &pfd) == FALSE) {
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    return TRUE;
}
// D3D diff 9 of 14
#endif

/*
===================================================================

MAIN WINDOW

===================================================================
*/


static float alttab_out_sbar_value=-1;
void AppActivate(BOOL fActive, BOOL minimize) {
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
	static BOOL	sound_active;

	ActiveApp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active) {
		S_BlockSound ();
		S_ClearBuffer ();
#ifdef BUILD_MP3_VERSION
		// Need to pause CD music here if is playing
		if (sound_started) {
//			Cbuf_InsertText ("cd pause\n");
//			Cbuf_Execute ();
			CDAudio_Pause ();
		}
#endif
		sound_active = false;
	} else if (ActiveApp && !sound_active) {
		S_UnblockSound ();
#ifdef BUILD_MP3_VERSION
		// Need to unpause CD music here if was playing
		if (sound_started) {
//			Cbuf_InsertText ("cd resume\n");
//			Cbuf_Execute ();
			CDAudio_Resume ();
		}
#endif
		sound_active = true;
	}

	if (fActive) {
		if (modestate == MS_FULLDIB) {

			// Has been activated
			// Baker 3.99k hack
			if (host_initialized && cl_sbar.value == 1 && alttab_out_sbar_value != -1 && cl_sbar.value != alttab_out_sbar_value) {
				// Restore sbar to proper value from 1
				Cvar_SetValueByRef (&cl_sbar, alttab_out_sbar_value);
				alttab_out_sbar_value = -1; // Reset this in case something weird can happen
				Sbar_Changed ();
				Con_DPrintf ("Transparent sbar restored\n");
			}

			IN_ActivateMouse ();
			IN_HideMouse ();

			if (vid_canalttab && vid_wassuspended) {
				vid_wassuspended = false;
				ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN);
				ShowWindow(mainwindow, SW_SHOWNORMAL);

				// Fix for alt-tab bug in NVidia drivers
                MoveWindow (mainwindow, 0, 0, gdevmode.dmPelsWidth, gdevmode.dmPelsHeight, false);

				// scr_fullupdate = 0;
				Sbar_Changed ();
			}
		} else if (modestate == MS_WINDOWED && Minimized) {
			ShowWindow (mainwindow, SW_RESTORE);
		} else if ((modestate == MS_WINDOWED) && _windowed_mouse.value && key_dest == key_game) {
			IN_ActivateMouse ();
			IN_HideMouse ();
		}

#ifdef SUPPORTS_ENHANCED_GAMMA
		if (using_hwgamma && vid_hwgamma_enabled)
			if (vid_canalttab && /* !Minimized &&*/ currentgammaramp)
				VID_SetDeviceGammaRamp (currentgammaramp);
#endif
	}

	if (!fActive) {
#ifdef SUPPORTS_ENHANCED_GAMMA
		if (using_hwgamma && vid_hwgamma_enabled)
			RestoreHWGamma ();
#endif
		if (modestate == MS_FULLDIB) {

			// Baker hack: with cl_sbar < 1, save it and set it to 1 and restore it later
			if (host_initialized && cl_sbar.value < 1) {
				// Restore sbar to proper value from 1
				alttab_out_sbar_value =  cl_sbar.value;
				Cvar_SetValueByRef (&cl_sbar, 1);
				Sbar_Changed ();
				Con_DPrintf ("Transparent state stored\n");
			}


			IN_DeactivateMouse ();
			IN_ShowMouse ();

			if (vid_canalttab) {
				ChangeDisplaySettings (NULL, 0);
				vid_wassuspended = true;
			}
		} else if ((modestate == MS_WINDOWED) && _windowed_mouse.value) {
			IN_DeactivateMouse ();
			IN_ShowMouse ();
		}
	}
}

#ifdef BUILD_MP3_VERSION
#ifndef WM_GRAPHNOTIFY
#define WM_GRAPHNOTIFY  WM_USER + 13
#endif
#endif

LONG CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int IN_MapKey (int key);
extern int 	key_special_dest;
int 		extmousex, extmousey; // Baker: for tracking Windowed mouse coordinates
/* main window procedure */
LONG WINAPI MainWndProc (HWND    hWnd, UINT    uMsg, WPARAM  wParam, LPARAM  lParam) {
    LONG    lRet = 1;
	int		fActive, fMinimized, temp;
	extern unsigned int uiWheelMessage;
	char	state[256];
	char	asciichar[4];
	int		vkey;
	int		charlength;
	qboolean down = false;

	if ( uMsg == uiWheelMessage ) {
		uMsg = WM_MOUSEWHEEL;
		wParam <<= 16; // Baker 3.80x - Added this, purpose unknown
	}

    switch (uMsg) {
		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper Event.
		case WM_MOUSEWHEEL:
			if ((short) HIWORD(wParam) > 0) {
				Key_Event(K_MWHEELUP, 0, true);
				Key_Event(K_MWHEELUP, 0, false);
			} else {
				Key_Event(K_MWHEELDOWN, 0, true);
				Key_Event(K_MWHEELDOWN, 0, false);
			}
			break;

		case WM_KILLFOCUS:
			if (modestate == MS_FULLDIB)
				ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);
			break;

		case WM_CREATE:
			break;

		case WM_MOVE:

			window_x = (int) LOWORD(lParam);
			window_y = (int) HIWORD(lParam);
			VID_UpdateWindowStatus ();
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			down=true;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			// Baker 3.703 - old way
			// Key_Event (IN_MapKey(lParam), false);
			vkey = IN_MapKey(lParam);
				GetKeyboardState (state);
				// alt/ctrl/shift tend to produce funky ToAscii values,
				// and if it's not a single character we don't know care about it
				charlength = ToAscii (wParam, lParam >> 16, state, (unsigned short *)asciichar, 0);
				if (vkey == K_ALT || vkey == K_CTRL || vkey == K_SHIFT || charlength == 0)
					asciichar[0] = 0;
				else if( charlength == 2 ) {
					asciichar[0] = asciichar[1];
				}

				Key_Event (vkey, asciichar[0], down);
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONUP:
			// Mouse isn't active + special destination
			// means Quake doesn't control mouse
			if (key_special_dest && !flex_mouseactive) {
				extmousex = Q_rint((float)LOWORD(lParam)*((float)vid.width/(float)glwidth)); //Con_Printf("Mouse click x/y %d/%d\n", extmousex, extmousey);
				extmousey = Q_rint((float)HIWORD(lParam)*((float)vid.height/(float)glheight));
				Key_Event (K_MOUSECLICK_BUTTON1, 0, false);
				break;
			}
		case WM_RBUTTONUP:
			// Mouse isn't active + special destination
			// means Quake doesn't control mouse
			if (key_special_dest && !flex_mouseactive) {
				Key_Event (K_MOUSECLICK_BUTTON2, 0, false);
				break;
			}

// Since we are not trapping button downs for special destination
// like namemaker or customize controls, we need the down event
// captures to be below the above code so it doesn't filter into it
// The code below is safe due to the "& MK_xBUTTON" checks
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON) {
				if (key_special_dest && !flex_mouseactive) {
					Key_Event (K_MOUSECLICK_BUTTON3, 0, false);
					break; // Get out
				}
				temp |= 4;
			}

			if (wParam & MK_XBUTTON1) {
				if (key_special_dest && !flex_mouseactive) {
					Key_Event (K_MOUSECLICK_BUTTON4, 0, false);
					break; // Get out
				}
				temp |= 8;
			}

			if (wParam & MK_XBUTTON2) {
				if (key_special_dest && !flex_mouseactive) {
					Key_Event (K_MOUSECLICK_BUTTON5, 0, false);
					break; // Get out
				}
				temp |= 16;
			}

			IN_MouseEvent (temp);

			break;


    	case WM_SIZE:
            break;

   	    case WM_CLOSE:

			if (!cl_confirmquit.value || MessageBox(mainwindow, "Are you sure you want to quit?", "Confirm Exit", MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
				Sys_Quit ();

	        break;

		case WM_ACTIVATE:

			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);

			AppActivate(!(fActive == WA_INACTIVE), fMinimized);

		// fix the leftover Alt from any Alt-Tab or the like that switched us away
			Key_ClearAllStates ();

			break;

   	    case WM_DESTROY:

			if (dibwindow)
				DestroyWindow (dibwindow);

            PostQuitMessage (0);

        break;
#ifdef BUILD_MP3_VERSION
		case WM_GRAPHNOTIFY:
#else
		case MM_MCINOTIFY:
#endif
            lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);

			break;

    	default:
            // pass all unhandled messages to DefWindowProc
            lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);

        break;
    }

    // return 1 if handled message, 0 if not
    return lRet;
}

//==========================================================================
//
//  COMMANDS
//
//==========================================================================

/*
=================
VID_NumModes
=================
*/
int VID_NumModes (void) {
	return nummodes;
}


/*
=================
VID_GetModePtr
=================
*/
vmode_t *VID_GetModePtr (int modenum) {

	if ((modenum >= 0) && (modenum < nummodes))
		return &modelist[modenum];

	return &badmode;
}


/*
=================
VID_GetModeDescription
=================
*/
char *VID_GetModeDescription (int mode) {
	char		*pinfo;
	vmode_t		*pv;
	static char	temp[100];

	if ((mode < 0) || (mode >= nummodes))
		return NULL;

	if (!leavecurrentmode) {
		pv = VID_GetModePtr (mode);
		pinfo = pv->modedesc;
	} else {
		snprintf (temp, sizeof(temp), "Desktop resolution (%ix%ix%i)", //johnfitz -- added bpp
				 modelist[MODE_FULLSCREEN_DEFAULT].width,
				 modelist[MODE_FULLSCREEN_DEFAULT].height,
				 modelist[MODE_FULLSCREEN_DEFAULT].bpp); //johnfitz -- added bpp
		pinfo = temp;
	}

	return pinfo;
}

// KJB: Added this to return the mode driver name in description for console
/*
=================
VID_GetExtModeDescription
=================
*/
char *VID_GetExtModeDescription (int mode) {
	static char	pinfo[40];
	vmode_t		*pv;

	if ((mode < 0) || (mode >= nummodes))
		return NULL;

	pv = VID_GetModePtr (mode);
	if (modelist[mode].type == MS_FULLDIB) {
		if (!leavecurrentmode)
		{
			snprintf(pinfo, sizeof(pinfo), "%s fullscreen", pv->modedesc);
		}
		else
		{
			snprintf (pinfo, sizeof(pinfo), "Desktop resolution (%ix%ix%i)", //johnfitz -- added bpp
					 modelist[MODE_FULLSCREEN_DEFAULT].width,
					 modelist[MODE_FULLSCREEN_DEFAULT].height,
					 modelist[MODE_FULLSCREEN_DEFAULT].bpp); //johnfitz -- added bpp
		}
	} else {
		if (modestate == MS_WINDOWED)
			snprintf(pinfo, sizeof(pinfo), "%s windowed", pv->modedesc);
		else
			snprintf(pinfo, sizeof(pinfo), "windowed");
	}

	return pinfo;
}

/*
=================
VID_DescribeCurrentMode_f
=================
*/
void VID_DescribeCurrentMode_f (void) {
	Con_Printf ("%s\n", VID_GetExtModeDescription (vid_modenum));
}

/*
=================
VID_DescribeModes_f -- johnfitz -- changed formatting, and added refresh rates after each mode.
=================
*/
void VID_DescribeModes_f (void) {
	int			i, lnummodes, t;
//	char		*pinfo;
	vmode_t		*pv;
	int			lastwidth=0, lastheight=0, lastbpp=0, count=0;

	lnummodes = VID_NumModes ();

	t = leavecurrentmode;
	leavecurrentmode = 0;

	for (i=1 ; i<lnummodes ; i++) {
		pv = VID_GetModePtr (i);
		if (lastwidth == pv->width && lastheight == pv->height && lastbpp == pv->bpp)
		{
			Con_SafePrintf (",%i", pv->refreshrate);
		}
		else
		{
			if (count>0)
				Con_SafePrintf ("\n");
			Con_SafePrintf ("   %4i x %4i x %i : %i", pv->width, pv->height, pv->bpp, pv->refreshrate);
			lastwidth = pv->width;
			lastheight = pv->height;
			lastbpp = pv->bpp;
			count++;
	}
	}
	Con_Printf ("\n%i modes\n", count);

	leavecurrentmode = t;
}

//==========================================================================
//
//  INIT
//
//==========================================================================

/*
=================
VID_InitDIB
=================
*/
void VID_InitDIB (HINSTANCE hInstance) {
	DEVMODE			devmode; //johnfitz
	WNDCLASS		wc;
	HDC				hdc;

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = ENGINE_NAME; //"WinQuake";

    if (!RegisterClass (&wc) )
		Sys_Error ("Couldn't register window class");

	modelist[0].type = MS_WINDOWED;

	if (COM_CheckParm("-width"))
		modelist[0].width = atoi(com_argv[COM_CheckParm("-width")+1]);
	else if (COM_CheckParm("-fullwindow"))
		modelist[0].width = GetSystemMetrics(SM_CXSCREEN); // Baker 3.60 - fullwindow mode
	else
		modelist[0].width = 640;

	if (modelist[0].width < 320)
		modelist[0].width = 320;

	if (COM_CheckParm("-height"))
		modelist[0].height= atoi(com_argv[COM_CheckParm("-height")+1]);
	else if (COM_CheckParm("-fullwindow"))
		modelist[0].height = GetSystemMetrics(SM_CYSCREEN); // Baker 3.60 - fullwindow mode
	else
		modelist[0].height = modelist[0].width * 240/320;

	if (modelist[0].height < 200) //johnfitz -- was 240
		modelist[0].height = 200; //johnfitz -- was 240

	//johnfitz -- get desktop bit depth
	hdc = GetDC(NULL);
	modelist[0].bpp = desktop_bpp = GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(NULL, hdc);
	//johnfitz

	//johnfitz -- get refreshrate
	if (EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode))
		modelist[0].refreshrate = devmode.dmDisplayFrequency;
	//johnfitz

	snprintf (modelist[0].modedesc, sizeof(modelist[0].modedesc), "%dx%dx%d %dHz", //johnfitz -- added bpp, refreshrate
			 modelist[0].width,
			 modelist[0].height,
			 modelist[0].bpp, //johnfitz -- added bpp
			 modelist[0].refreshrate); //johnfitz -- added refreshrate

	modelist[0].modenum = MODE_WINDOWED;
	modelist[0].dib = 1;
	modelist[0].fullscreen = 0;
	modelist[0].halfscreen = 0;
//	modelist[0].bpp = 0;
	nummodes = 1;
}

/*
=================
VID_InitFullDIB
=================
*/
void VID_InitFullDIB (HINSTANCE hInstance) {
	DEVMODE	devmode;
	int		i, modenum, originalnummodes, existingmode, numlowresmodes;
	int		j, bpp, done;
	BOOL	stat;

// enumerate >8 bpp modes
	originalnummodes = nummodes;
	modenum = 0;

	do {
		stat = EnumDisplaySettings (NULL, modenum, &devmode);

		if ((devmode.dmBitsPerPel >= 15) &&
			(devmode.dmPelsWidth <= MAXWIDTH) &&
			(devmode.dmPelsHeight <= MAXHEIGHT) &&
			(nummodes < MAX_MODE_LIST))
		{
			devmode.dmFields = DM_BITSPERPEL |
							   DM_PELSWIDTH |
							   DM_PELSHEIGHT |
							   DM_DISPLAYFREQUENCY; //johnfitz -- refreshrate

			if (ChangeDisplaySettings (&devmode, CDS_TEST | CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
				modelist[nummodes].type = MS_FULLDIB;
				modelist[nummodes].width = devmode.dmPelsWidth;
				modelist[nummodes].height = devmode.dmPelsHeight;
				modelist[nummodes].modenum = 0;
				modelist[nummodes].halfscreen = 0;
				modelist[nummodes].dib = 1;
				modelist[nummodes].fullscreen = 1;
				modelist[nummodes].bpp = devmode.dmBitsPerPel;
				modelist[nummodes].refreshrate = devmode.dmDisplayFrequency; //johnfitz -- refreshrate
				snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d %dHz", //johnfitz -- refreshrate
						 devmode.dmPelsWidth,
						 devmode.dmPelsHeight,
						 devmode.dmBitsPerPel,
						 devmode.dmDisplayFrequency); //johnfitz -- refreshrate

			// if the width is more than twice the height, reduce it by half because this
			// is probably a dual-screen monitor
				if (!COM_CheckParm("-noadjustaspect"))
				{
					if (modelist[nummodes].width > (modelist[nummodes].height << 1))
					{
						modelist[nummodes].width >>= 1;
						modelist[nummodes].halfscreen = 1;
						snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d %dHz", //johnfitz -- refreshrate
								 modelist[nummodes].width,
								 modelist[nummodes].height,
								 modelist[nummodes].bpp,
								 modelist[nummodes].refreshrate); //johnfitz -- refreshrate
					}
				}

				for (i=originalnummodes, existingmode = 0 ; i<nummodes ; i++) {
					if ((modelist[nummodes].width == modelist[i].width)   &&
						(modelist[nummodes].height == modelist[i].height) &&
						(modelist[nummodes].bpp == modelist[i].bpp) &&
						(modelist[nummodes].refreshrate == modelist[i].refreshrate)) //johnfitz -- refreshrate
					{
						existingmode = 1;
						break;
					}
				}

				if (!existingmode)
					nummodes++;
			}
		}

		modenum++;
	} while (stat);

// see if there are any low-res modes that aren't being reported
	numlowresmodes = sizeof(lowresmodes) / sizeof(lowresmodes[0]);
	bpp = 16;
	done = 0;

	do {
		for (j=0 ; (j<numlowresmodes) && nummodes < MAX_MODE_LIST ; j++) {
			devmode.dmBitsPerPel = bpp;
			devmode.dmPelsWidth = lowresmodes[j].width;
			devmode.dmPelsHeight = lowresmodes[j].height;
			devmode.dmFields = DM_BITSPERPEL |
							   DM_PELSWIDTH |
							   DM_PELSHEIGHT |
							   DM_DISPLAYFREQUENCY; //johnfitz -- refreshrate;

			if (ChangeDisplaySettings (&devmode, CDS_TEST | CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
			{
				modelist[nummodes].type = MS_FULLDIB;
				modelist[nummodes].width = devmode.dmPelsWidth;
				modelist[nummodes].height = devmode.dmPelsHeight;
				modelist[nummodes].modenum = 0;
				modelist[nummodes].halfscreen = 0;
				modelist[nummodes].dib = 1;
				modelist[nummodes].fullscreen = 1;
				modelist[nummodes].bpp = devmode.dmBitsPerPel;
				modelist[nummodes].refreshrate = devmode.dmDisplayFrequency; //johnfitz -- refreshrate
				snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d %dHz", //johnfitz -- refreshrate
						 devmode.dmPelsWidth,
						 devmode.dmPelsHeight,
						 devmode.dmBitsPerPel,
						 devmode.dmDisplayFrequency); //johnfitz -- refreshrate

				for (i=originalnummodes, existingmode = 0 ; i<nummodes ; i++)
				{
					if ((modelist[nummodes].width == modelist[i].width)   &&
						(modelist[nummodes].height == modelist[i].height) &&
						(modelist[nummodes].bpp == modelist[i].bpp) &&
						(modelist[nummodes].refreshrate == modelist[i].refreshrate)) //johnfitz -- refreshrate
					{
						existingmode = 1;
						break;
					}
				}

				if (!existingmode)  {
					nummodes++;
				}
			}
		}

		switch (bpp) {
			case 16:
				bpp = 32;
				break;

			case 32:
				bpp = 24;
				break;

			case 24:
				done = 1;
				break;
		}
	} while (!done);

	if (nummodes == originalnummodes) {
		Con_SafePrintf ("No fullscreen DIB modes found\n");
	}
}

qboolean VID_Is8bit() {
	return is8bit;
}

#define GL_SHARED_TEXTURE_PALETTE_EXT 0x81FB

void VID_Init8bitPalette()
{

#if !defined(DX8QUAKE_NO_8BIT)
	// Check for 8bit Extensions and initialize them.
	int i;
	char thePalette[256*3];
	char *oldPalette, *newPalette;

#if 0
	// Baker: this "old way with -no8bit"

	glColorTableEXT = (void *)wglGetProcAddress("glColorTableEXT");
    if (!glColorTableEXT || strstr(gl_extensions, "GL_EXT_shared_texture_palette") ||
		COM_CheckParm("-no8bit"))
		return;
#endif

	//Baker: New way

	glColorTableEXT = (void *)wglGetProcAddress("glColorTableEXT");
    if (!glColorTableEXT || strstr(gl_extensions, "GL_EXT_shared_texture_palette") ||
		!COM_CheckParm("-8bit"))
		return;

	// Baker: end new way

	Con_SafePrintf("8-bit GL extensions enabled.\n");
    glEnable( GL_SHARED_TEXTURE_PALETTE_EXT );
	oldPalette = (char *) d_8to24table; //d_8to24table3dfx;
	newPalette = thePalette;
	for (i=0;i<256;i++) {
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		*newPalette++ = *oldPalette++;
		oldPalette++;
	}
	glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE,
		(void *) thePalette);
	is8bit = TRUE;
#endif
}


/*
===================
VID_Init
===================
*/
extern	cvar_t vid_consize;
void	VID_Init (unsigned char *palette) {
	int		i, existingmode;
	int		basenummodes, width, height, bpp, findbpp, done;
	HGLRC	baseRC; //johnfitz -- moved here from global scope, since it was only used in this
	HDC		hdc;
	DEVMODE	devmode;
	extern	cvar_t gl_clear;
	extern	void GL_PrintExtensions_f (void);

	memset(&devmode, 0, sizeof(devmode));

	Cvar_RegisterVariable (&vid_fullscreen, NULL); //johnfitz
	Cvar_RegisterVariable (&vid_width, NULL); //johnfitz
	Cvar_RegisterVariable (&vid_height, NULL); //johnfitz
	Cvar_RegisterVariable (&vid_consize, VID_Consize_f); //Baker 3.97: this supercedes vid_conwidth/vid_conheight cvars
	Cvar_RegisterVariable (&vid_refreshrate, NULL); //johnfitz
	Cvar_RegisterVariable (&_windowed_mouse, NULL);
	Cvar_RegisterVariable (&vid_fullscreen_mode, NULL);
	Cvar_RegisterVariable (&vid_windowed_mode, NULL);

	Cvar_RegisterVariable (&gl_clear, NULL); // Baker: cvar needs registered here so we can set it
	Cvar_RegisterVariable (&gl_ztrick, NULL);

	// Baker hwgamma support
	Cvar_RegisterVariable (&vid_hwgammacontrol, NULL);
	// Baker end hwgamma support

#ifdef SUPPORTS_GLVIDEO_MODESWITCH // D3D DOES NOT SUPPORT THIS
	Cmd_AddCommand ("vid_unlock", VID_Unlock_f); //johnfitz
	Cmd_AddCommand ("vid_restart", VID_Restart_f); //johnfitz
	Cmd_AddCommand ("vid_test", VID_Test_f); //johnfitz
#endif // D3D dies on video mode change

	Cmd_AddCommand ("gl_print_extensions", GL_PrintExtensions_f);
	Cmd_AddCommand ("vid_describecurrentmode", VID_DescribeCurrentMode_f);
	Cmd_AddCommand ("vid_describemodes", VID_DescribeModes_f);

	hIcon = LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON2));

	// Baker: This doesn't seem to be in too many engines
	// Plus it requires a link to a dll.  What purpose does this serve?
	// This originated, I believe, in FitzQuake and I think it can be
	// found in DarkPlaces, but not most engines.
	// Maybe there is a gain from it (file dialog capability?), but not sure what it is right now.
	// But whatever it is, it doesn't seem to be leveraged.
//	InitCommonControls();

	VID_InitDIB (global_hInstance);
	basenummodes = nummodes = 1;

	VID_InitFullDIB (global_hInstance);

#ifdef SUPPORTS_ENHANCED_GAMMA
	if (COM_CheckParm("-gamma")) // Baker hwgamma support
		using_hwgamma = false;
#endif

// D3D diff 11 of 14
#ifdef D3DQ_WORKAROUND
	bpp = 16;
	if (COM_CheckParm("-bpp"))
	{
		bpp = atoi(com_argv[COM_CheckParm("-bpp")+1]);
	}
#endif

	if (COM_CheckParm("-window") || COM_CheckParm("-fullwindow")) {
		hdc = GetDC (NULL);

		if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
			Sys_Error ("Can't run in non-RGB mode");

		ReleaseDC (NULL, hdc);

		windowed = true;
		video_options_disabled = true;
		vid_default = MODE_WINDOWED;
	} else {
		if (nummodes == 1)
			Sys_Error ("No RGB fullscreen modes available");

		windowed = false;

		if (COM_CheckParm("-mode"))
		{
			vid_default = atoi(com_argv[COM_CheckParm("-mode")+1]);
		}
		else
		{
			if (COM_CheckParm("-current")) {
				modelist[MODE_FULLSCREEN_DEFAULT].width = GetSystemMetrics (SM_CXSCREEN);
				modelist[MODE_FULLSCREEN_DEFAULT].height = GetSystemMetrics (SM_CYSCREEN);
				vid_default = MODE_FULLSCREEN_DEFAULT;
				leavecurrentmode = 1;
			} else {
				if (COM_CheckParm("-width"))
					width = atoi(com_argv[COM_CheckParm("-width")+1]);
				else
					width = 640;


				if (COM_CheckParm("-bpp")) {
					bpp = atoi(com_argv[COM_CheckParm("-bpp")+1]);
					findbpp = 0;
				} else {
					bpp = desktop_bpp;
					findbpp = 1;
				}

				if (COM_CheckParm("-height"))
					height = atoi(com_argv[COM_CheckParm("-height")+1]);

			// if they want to force it, add the specified mode to the list
				if (COM_CheckParm("-force") && nummodes < MAX_MODE_LIST) {
					modelist[nummodes].type = MS_FULLDIB;
					modelist[nummodes].width = width;
					modelist[nummodes].height = height;
					modelist[nummodes].modenum = 0;
					modelist[nummodes].halfscreen = 0;
					modelist[nummodes].dib = 1;
					modelist[nummodes].fullscreen = 1;
					modelist[nummodes].bpp = bpp;
					snprintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d %dHz", //johnfitz -- refreshrate
							 devmode.dmPelsWidth,
							 devmode.dmPelsHeight,
							 devmode.dmBitsPerPel,
							 devmode.dmDisplayFrequency); //johnfitz -- refreshrate

					for (i=nummodes, existingmode = 0 ; i<nummodes ; i++) {
						if ((modelist[nummodes].width == modelist[i].width)   &&
							(modelist[nummodes].height == modelist[i].height) &&
							(modelist[nummodes].bpp == modelist[i].bpp) &&
							(modelist[nummodes].refreshrate == modelist[i].refreshrate)) //johnfitz -- refreshrate
						{
							existingmode = 1;
							break;
						}
					}

					if (!existingmode) {
						nummodes++;
					}
				}

				done = 0;

				do {
					if (COM_CheckParm("-height")) {
						height = atoi(com_argv[COM_CheckParm("-height")+1]);

						for (i=1, vid_default=0 ; i<nummodes ; i++) {
							if ((modelist[i].width == width) && (modelist[i].height == height) && (modelist[i].bpp == bpp)) {
								vid_default = i;
								done = 1;
								break;
							}
						}
					} else {
						for (i=1, vid_default=0 ; i<nummodes ; i++) {
							if ((modelist[i].width == width) && (modelist[i].bpp == bpp)) {
								vid_default = i;
								done = 1;
								break;
							}
						}
					}

					if (!done) {
						if (findbpp) {
							switch (bpp) {
							case 15:
								bpp = 16;
								break;

							case 16:
								bpp = 32;
								break;

							case 32:
								bpp = 24;
								break;

							case 24:
								done = 1;
								break;
							}
						} else {
							done = 1;
						}
					}
				} while (!done);

				vid_bpp = bpp;

				if (!vid_default) {
					Sys_Error ("Specified video mode not available");
				}
			}
		}
	}

	vid_initialized = true;

// D3D diff 12 of 14
#ifdef D3DQ_WORKAROUND
	if ((i = COM_CheckParm("-force16bittextures")) != 0)
		d3dInitSetForce16BitTextures(1);

#endif

	if ((i = COM_CheckParm("-conwidth")) != 0) {
		// Conwidth specified, we check conheight too then
		vid.conwidth = atoi(com_argv[i+1]);

		if ((i = COM_CheckParm("-conheight")) != 0)
			vid.conheight = atoi(com_argv[i+1]);
	} else {
		// Conwidth not specified, default one
		if (modelist[i].width >= 1024) {
			vid.conwidth = modelist[vid_default].width / 2;
			vid.conheight = modelist[vid_default].height / 2;
		} else {
			vid.conwidth = modelist[vid_default].width;
			vid.conheight = modelist[vid_default].height;
		}
	}

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	if (vid.conheight < 200)
		vid.conheight = 200;

	vid.conwidth &= 0xfff8; // make it a multiple of eight
	vid.conheight &= 0xfff8; // make it a multiple of eight

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
#if !defined(DX8QUAKE_NO_DIALOGS) // No dialog for DX8Quake
	DestroyWindow (hwnd_dialog);
#endif

	Check_GammaOld (palette);
	VID_SetPaletteOld (palette);
	VID_SetMode (vid_default, palette);

    maindc = GetDC(mainwindow);
   // D3D diff 13 of 14
#ifdef D3DQ_WORKAROUND
	{
		int zbpp = bpp > 16 ? 24 : 16;
		if (COM_CheckParm("-zbpp"))
		{
			zbpp = atoi(com_argv[COM_CheckParm("-zbpp")+1]);
		}
		d3dSetMode(!windowed, modelist[0].width, modelist[0].height, bpp, zbpp);
	}
#else
	bSetupPixelFormat(maindc);
	// D3D diff 14 of 14
#endif

	InitHWGamma ();

	// baker end hwgamma support
    baseRC = wglCreateContext( maindc );
	if (!baseRC)
		Sys_Error ("Could not initialize GL (wglCreateContext failed).\n\nMake sure you are in 65535 color mode, and try running -window.");
    if (!wglMakeCurrent( maindc, baseRC ))
		Sys_Error ("VID_Init: wglMakeCurrent failed");

	GL_Init ();
	CheckVsyncControlExtensions ();

	// Baker 3.95: With .ms2 meshing gone, this is unnecessary, right?
//	snprintf (gldir, sizeof(gldir), "%s/glquake", com_gamedir);
//	Sys_mkdir (gldir);

	vid_realmode = vid_modenum;

#if !defined(DX8QUAKE_NO_8BIT)  // DX8QUAKE NO 8 BIT
	// Check for 3DFX Extensions and initialize them.
	VID_Init8bitPalette();
#endif

#ifdef SUPPORTS_GLVIDEO_MODESWITCH // Right?
	vid_menucmdfn = VID_Menu_f; //johnfitz
	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;
#endif

	strcpy (badmode.modedesc, "Bad mode");
	vid_canalttab = true;

	if (COM_CheckParm("-fullsbar"))
		fullsbardraw = true;

	VID_Menu_Init(); //johnfitz

#ifdef D3DQ_WORKAROUND
	vid_locked = true; // D3DQuake can't switch resolution due to DDE_LOSTSURFACE or whatever
#else
	//johnfitz -- command line vid settings should override config file settings.
	//so we have to lock the vid mode from now until after all config files are read.
	if (COM_CheckParm("-width") || COM_CheckParm("-height") /*|| COM_CheckParm("-bpp")*/ || COM_CheckParm("-window")) {
		// Baker 3.99h: I'd like to allow video mode changing with -bpp != desktop
		vid_locked = true;
	} else {
		// Baker: 3.99h if vid is not -window and bpp != desktop_bpp then vid_windowtypelocked = true;
		if (vid_bpp != desktop_bpp)
			vid_fullscreen_only = true;
	}

	//johnfitz
#endif

}

/*
================
VID_SyncCvars -- johnfitz -- set vid cvars to match current video mode
================
*/
extern qboolean vid_consize_ignore_callback;
void VID_SyncCvars (void)
{
	Cvar_SetStringByRef (&vid_width, va("%i", modelist[vid_default].width));
	Cvar_SetStringByRef (&vid_height, va("%i", modelist[vid_default].height));
	Cvar_SetStringByRef (&vid_refreshrate, va("%i", modelist[vid_default].refreshrate));
	Cvar_SetStringByRef (&vid_fullscreen, (vid_fullscreen_only) ? "1" : ((windowed) ? "0" : "1"));
}

// Baker 3.97: new scheme supercedes these

/*
==================
VID_Consize_f -- Baker -- called when vid_consize changes
==================
*/
extern qpic_t *conback;
//qboolean vid_smoothfont = false;
extern qboolean smoothfont_init;

void VID_Consize_f(void) {

	float startwidth;
	float startheight;
	float desiredwidth;
	int contype = vid_consize.value;
	int exception = 0;

	startwidth = vid.width = modelist[vid_default].width;
	startheight = vid.height = modelist[vid_default].height;

//	Con_Printf("Entering ...\n");
//	Con_Printf("vid.width is %d and vid.height is %d\n", vid.width, vid.height);
//	Con_Printf("vid.conwidth is %d and vid.conheight is %d\n", vid.conwidth, vid.conheight);

	// Baker 3.97
	// We need to appropriately set vid.width, vid.height, vid.smoothfont (?)

//	vid_smoothfont = false; // Assume it is unnecessary

	if (contype == -1) {
		// Automatic consize to avoid microscopic text
		if (vid.width>=1024)
			contype = 1;
		else
			contype = 0;
	}

	switch (contype) {

		case 0: // consize is width

			desiredwidth = vid.width;
			break;

		case 1: // consize is 50% width (if possible)

			// if resolution is < 640, must use the resolution itself.
			if (vid.width < 640) {
				exception = 1; // Notify later about console resolution unavailable
				desiredwidth = vid.width;
				break;
			}

			desiredwidth = (int)(vid.width/2);
			break;

		case 3:
			desiredwidth = 320;
			break;

		default:
			// If vid.width is under 640, must use 320?
			if (vid.width < 640) {
				exception = 2; // Notify later about console resolution unavailable
				desiredwidth = vid.width;
				break;
			}
			desiredwidth = 640;
			break;
	}

	vid.conwidth = CLAMP (320, desiredwidth, vid.width);
	vid.conwidth &= 0xFFFFFFF8;                      // But this must be a multiple of 8
	vid.conheight = vid.conwidth * vid.height / vid.width;  // Now set height using proper aspect ratio
	vid.conheight &= 0xFFFFFFF8;					  // This too must be a multiple of 8

	conback->width = vid.width = vid.conwidth; // = vid.width;
	conback->height = vid.height = vid.conheight; // = vid.height;

	//  Determine if smooth font is needed

	if ((int)(startwidth / vid.conwidth) == ((startwidth + 0.0f) / (vid.conwidth + 0.0f)) /*&& (int)(startheight / vid.conheight) == ((startheight + 0.0f) / (vid.conheight + 0.0f))*/) {
		SmoothFontSet (false);
	} else {
		SmoothFontSet (true);
	}

	// Print messages AFTER console resizing to ensure they print right
	if (exception) {
		if (exception == 1)
			Con_Printf ("VID_Consize_f: 50%% console size unavailable, using 100%% for this resolution.\n");
		else
			Con_Printf ("VID_Consize_f: 640 console size unavailable, using 100%% for this resolution.\n");
	}

	vid.recalc_refdef = 1;

}


//==========================================================================
//
//  NEW VIDEO MENU -- johnfitz
//
//==========================================================================

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, char *str);
extern void M_PrintWhite (int cx, int cy, char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);
extern void M_DrawCheckbox (int x, int y, int on);

extern qboolean	m_entersound;

// Baker - mirror of menu
/* Baker moved to menu.h
enum {
	m_none,
	m_main,
	m_singleplayer,
	m_load,
	m_save,
	m_multiplayer,
	m_setup,
	m_net,
	m_options,
	m_video,
	m_keys,
	m_help,
	m_quit,
	m_serialconfig,
	m_modemconfig,
	m_lanconfig,
	m_gameoptions,
	m_search,
	m_slist,
	m_preferences
} menu_state;
*/

#define VIDEO_OPTIONS_ITEMS 6
int		video_cursor_table[] = {48, 56, 64, 72, 88, 96};
int		video_options_cursor = 0;

typedef struct {int width,height;} vid_menu_mode;

//TODO: replace these fixed-length arrays with hunk_allocated buffers

vid_menu_mode vid_menu_modes[MAX_MODE_LIST];
int vid_menu_nummodes=0;

//int vid_menu_bpps[4];
//int vid_menu_numbpps=0;

int vid_menu_rates[20];
int vid_menu_numrates=0;

/*
================
VID_Menu_Init
================
*/
void VID_Menu_Init (void)
{
	int i,j,h,w;

	for (i=1;i<nummodes;i++) //start i at mode 1 because 0 is windowed mode
	{
		w = modelist[i].width;
		h = modelist[i].height;

		for (j=0;j<vid_menu_nummodes;j++)
{
			if (vid_menu_modes[j].width == w &&
				vid_menu_modes[j].height == h)
				break;
		}

		if (j==vid_menu_nummodes)
		{
			vid_menu_modes[j].width = w;
			vid_menu_modes[j].height = h;
			vid_menu_nummodes++;
		}
	}
}


/*
================
VID_Menu_RebuildRateList

regenerates rate list based on current vid_width, vid_height and vid_bpp
================
*/
void VID_Menu_RebuildRateList (void)
{
	int i,j,r;

	vid_menu_numrates=0;

	for (i=1;i<nummodes;i++) //start i at mode 1 because 0 is windowed mode
	{
		//rate list is limited to rates available with current width/height/bpp
		if (modelist[i].width != vid_width.value ||
			modelist[i].height != vid_height.value /*||
			modelist[i].bpp != vid_bpp.value*/)
			continue;

		r = modelist[i].refreshrate;

		for (j=0;j<vid_menu_numrates;j++)
		{
			if (vid_menu_rates[j] == r)
				break;
		}

		if (j==vid_menu_numrates)
		{
			vid_menu_rates[j] = r;
			vid_menu_numrates++;
		}
	}

	//if vid_refreshrate is not in the new list, change vid_refreshrate
	for (i=0;i<vid_menu_numrates;i++)
		if (vid_menu_rates[i] == (int)(vid_refreshrate.value))
			break;

	if (i==vid_menu_numrates)
		Cvar_SetStringByRef (&vid_refreshrate, va("%i",vid_menu_rates[0]));
	}

/*
================
VID_Menu_ChooseNextMode

chooses next resolution in order, then updates vid_width and
vid_height cvars, then updates bpp and refreshrate lists
================
*/
void VID_Menu_ChooseNextMode (int dir)
{
	int i;

	for (i=0;i<vid_menu_nummodes;i++)
	{
		if (vid_menu_modes[i].width == vid_width.value &&
			vid_menu_modes[i].height == vid_height.value)
			break;
	}

	if (i==vid_menu_nummodes) //can't find it in list, so it must be a custom windowed res
	{
		i = 0;
	}
			else
	{
		i+=dir;
		if (i>=vid_menu_nummodes)
			i = 0;
		else if (i<0)
			i = vid_menu_nummodes-1;
	}

	Cvar_SetStringByRef (&vid_width, va("%i",vid_menu_modes[i].width));
	Cvar_SetStringByRef (&vid_height, va("%i",vid_menu_modes[i].height));
	VID_Menu_RebuildRateList ();
}

/*
================
VID_Menu_ChooseNextRate

chooses next refresh rate in order, then updates vid_refreshrate cvar
================
*/
void VID_Menu_ChooseNextRate (int dir)
{
	int i;

	for (i=0;i<vid_menu_numrates;i++)
	{
		if (vid_menu_rates[i] == vid_refreshrate.value)
			break;
	}

	if (i==vid_menu_numrates) //can't find it in list
	{
		i = 0;
	}
	else
	{
		i+=dir;
		if (i>=vid_menu_numrates)
			i = 0;
		else if (i<0)
			i = vid_menu_numrates-1;
}

	Cvar_SetStringByRef (&vid_refreshrate, va("%i",vid_menu_rates[i]));
}

/*
================
VID_MenuKey
================
*/
void VID_MenuKey (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		VID_SyncCvars (); //sync cvars before leaving menu. FIXME: there are other ways to leave menu
		S_LocalSound ("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		video_options_cursor--;
		if (video_options_cursor < 0)
			video_options_cursor = VIDEO_OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		video_options_cursor++;
		if (video_options_cursor >= VIDEO_OPTIONS_ITEMS)
			video_options_cursor = 0;
		break;

	case K_LEFTARROW:
		S_LocalSound ("misc/menu3.wav");
		switch (video_options_cursor)
		{
		case 0:
			VID_Menu_ChooseNextMode (-1);
			break;
		case 1:
			//VID_Menu_ChooseNextBpp (-1);
			break;
		case 2:
			VID_Menu_ChooseNextRate (-1);
			break;
		case 3:
			Cbuf_AddText ("toggle vid_fullscreen\n");
			break;
		case 4:
		case 5:
	default:
		break;
	}
		break;

	case K_RIGHTARROW:
		S_LocalSound ("misc/menu3.wav");
		switch (video_options_cursor)
		{
		case 0:
			VID_Menu_ChooseNextMode (1);
			break;
		case 1:
			//VID_Menu_ChooseNextBpp (1);
			break;
		case 2:
			VID_Menu_ChooseNextRate (1);
			break;
		case 3:
			if (vid_bpp == desktop_bpp)
				Cbuf_AddText ("toggle vid_fullscreen\n");
			break;
		case 4:
		case 5:
		default:
			break;
		}
		break;

	case K_ENTER:
		m_entersound = true;
		switch (video_options_cursor)
		{
		case 0:
			VID_Menu_ChooseNextMode (1);
			break;
		case 1:
			SCR_ModalMessage("Colordepth (bits per pixel) must be set\nusing -bpp 16 or -bpp 32 from the\ncommand line.\n\nPress Y or N to continue.",0.0f);
			break;
		case 2:
			VID_Menu_ChooseNextRate (1);
			break;
		case 3:
			if (vid_bpp == desktop_bpp)
				Cbuf_AddText ("toggle vid_fullscreen\n");
			else
				SCR_ModalMessage("Changing between fullscreen and\nwindowed mode is not available\nbecause your color depth does\nnot match the desktop.\n\nRemove -bpp from your command line\nto have this available.\n\nPress Y or N to continue.",0.0f);

			break;

		case 4:
			Cbuf_AddText ("vid_test\n");
			break;
		case 5:
			Cbuf_AddText ("vid_restart\n");
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	int i = 0;
	MYPICT *p;
	char *title;
	int  aspectratio1;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp"));

	//p = Draw_CachePic ("gfx/vidmodes.lmp");
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	// title
	title = "Video Options";
	M_PrintWhite ((320-8*strlen(title))/2, 32, title);

	// options
	M_Print (16, video_cursor_table[i], "        Video mode");
	M_Print (184, video_cursor_table[i], va("%ix%i", (int)vid_width.value, (int)vid_height.value));

	// Baker: show aspect ratio
	aspectratio1 = (int)((vid_width.value/vid_height.value) * 100.0f);

	if (aspectratio1 == 133) // 1.33333
		M_PrintWhite (264, video_cursor_table[i], "4:3");
	else if (aspectratio1 == 125) // 1.25
		M_PrintWhite (264, video_cursor_table[i], "5:4");
	else if (aspectratio1 == 160) // 1.6
		M_PrintWhite (264, video_cursor_table[i], "8:5");
	else if (aspectratio1 == 166) // 1.6666
		M_PrintWhite (264, video_cursor_table[i], "5:3");
	else if (aspectratio1 == 155) // 1.5555
		M_PrintWhite (264, video_cursor_table[i], "14:9");
	else if (aspectratio1 == 177) // 1.7777
		M_PrintWhite (264, video_cursor_table[i], "16:9");
	// Baker ... else just don't print one

	i++;

	M_Print (16, video_cursor_table[i], "       Color depth");
	M_Print (184, video_cursor_table[i], va("%i [locked]", (int)vid_bpp));
	i++;

	M_Print (16, video_cursor_table[i], "      Refresh rate");
	M_Print (184, video_cursor_table[i], va("%i Hz", (int)vid_refreshrate.value));
	i++;

	M_Print (16, video_cursor_table[i], "        Fullscreen");

	if (vid_bpp == desktop_bpp)
		M_DrawCheckbox (184, video_cursor_table[i], (int)vid_fullscreen.value);
	else
		M_Print (184, video_cursor_table[i], va("%s [locked]", (int)vid_fullscreen.value ? "on" : "off"));

	i++;

	M_Print (16, video_cursor_table[i], "      Test changes");
	i++;

	M_Print (16, video_cursor_table[i], "     Apply changes");

	// cursor
	M_DrawCharacter (168, video_cursor_table[video_options_cursor], 12+((int)(realtime*4)&1));

	// notes          "345678901234567890123456789012345678"
//	M_Print (16, 172, "Windowed modes always use the desk- ");
//	M_Print (16, 180, "top color depth, and can never be   ");
//	M_Print (16, 188, "larger than the desktop resolution. ");
}

/*
================
VID_Menu_f
================
*/
void VID_Menu_f (void)
{
	key_dest = key_menu;
	menu_state = m_videomodes;
	m_entersound = true;

	//set all the cvars to match the current mode when entering the menu
	VID_SyncCvars ();

	//set up bpp and rate lists based on current cvars
	VID_Menu_RebuildRateList ();
}