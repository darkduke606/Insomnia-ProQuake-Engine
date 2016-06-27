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
// vid.h -- video driver defs

#include <pspgum.h>

#define VID_CBITS	6
#define VID_GRADES	(1 << VID_CBITS)

// a pixel can be one, two, or four bytes
typedef byte pixel_t;

typedef struct vrect_s
{
	int			x,y,width,height;
	struct vrect_s	*pnext;
} vrect_t;

typedef struct
{
	pixel_t			*buffer;		// invisible buffer
	pixel_t			*colormap;		// 256 * VID_GRADES size
	unsigned short	*colormap16;		// 256 * VID_GRADES size
	int				fullbright;		// index of first fullbright color
	unsigned		rowbytes;		// may be > width if displayed in a window
	unsigned		width;
	unsigned		height;
	float			aspect;			// width / height -- < 0 is taller than wide
	int				numpages;
	int				recalc_refdef;		// if true, recalc vid-based stuff
	pixel_t			*conbuffer;
	int				conrowbytes;
	unsigned		conwidth;
	unsigned		conheight;
	int				maxwarpwidth;
	int				maxwarpheight;
	pixel_t			*direct;		// direct drawing to framebuffer, if not NULL
} viddef_t;

extern	viddef_t	vid;				// global video state
extern	unsigned short	d_8to16table[256];
extern	unsigned	d_8to24table[256];
extern void (*vid_menudrawfn)(void);
extern void (*vid_menukeyfn)(int key);

#ifdef SUPPORTS_GLVIDEO_MODESWITCH
extern void (*vid_menucmdfn)(void); //johnfitz
void VID_SyncCvars (void);
#endif

void	VID_SetPaletteOld (unsigned char *palette);

#if 1
// Sets the global palette (Quake)
void	VID_SetGlobalPalette (void);
// Sets colored light palette for lightmaps
void	VID_SetLightMapPalette (void);
// Sets texture-specific palette (i.e. Half-Life)
void	VID_SetPaletteToTexture (ScePspRGBA8888* palette);
#endif

// called at startup and after any gamma correction

void	VID_ShiftPaletteOld (unsigned char *palette);
// called for bonus and pain flashes, and for underwater color changes

void	VID_Init (unsigned char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	VID_Shutdown (void);
// Called at shutdown

void	VID_Update (vrect_t *rects);
// flushes the given rectangles from the view buffer to the screen

int VID_SetMode (int modenum, unsigned char *palette);
// sets the mode; only used by the Quake engine for resetting to mode 0 (the
// base mode) on memory allocation failures

void VID_HandlePause (qboolean pause);
// called only on Win32, when pause happens, so the mouse can be released

#ifdef SUPPORTS_ENHANCED_GAMMA
// by joe - gamma stuff
void VID_SetDeviceGammaRamp (unsigned short *ramps);
extern	qboolean vid_hwgamma_enabled;
#endif

#ifdef RELEASE_MOUSE_FULLSCREEN
// We will release the mouse if fullscreen under several circumstances
// but specifically NOT if connected to a server that isn't us
// In multiplayer you wouldn't want to release the mouse by going to console
// But we'll say it's ok if you went to the menu
#define MOUSE_RELEASE_GAME_SAFE  (cls.state != ca_connected || sv.active==true || key_dest == key_menu || cls.demoplayback)
//|| cls.demoplayback || key_dest == key_menu || sv.active)
#endif

#ifdef SUPPORTS_GLVIDEO_MODESWITCH
qboolean VID_WindowedSwapAvailable(void);
qboolean VID_isFullscreen(void);
void VID_Windowed(void);
void VID_Fullscreen(void);
#endif

#ifdef MACOSX
extern qboolean qMinimized;
#endif
