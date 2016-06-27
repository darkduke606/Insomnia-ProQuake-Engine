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
// r_screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
//#include "r_local.h"
#ifdef SUPPORTS_AVI_CAPTURE
#include "movie.h"
#endif

// only the refresh window will be updated unless these variables are flagged
int			scr_copytop;
int			scr_copyeverything;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

float		oldscreensize, oldfov;
cvar_t		scr_viewsize = {"viewsize","100", true};
cvar_t		scr_fov = {"fov","90", true, 6};	// 10 - 170  Baker 3.60 - Save to config
cvar_t		default_fov = {"default_fov","0", true};	// Baker 3.85 - Default_fov from FuhQuake
cvar_t		scr_conspeed = {"scr_conspeed","99999", true}; // Baker 3.60 - Save to config
cvar_t		scr_centertime = {"scr_centertime","2"};
cvar_t		scr_showram = {"showram","1"};
cvar_t		scr_showturtle = {"showturtle","0"};
cvar_t		scr_showpause = {"showpause","1"};
cvar_t		scr_printspeed = {"scr_printspeed","8"};

cvar_t      pq_drawfps = {"pq_drawfps", "0", true}; // JPG - draw frames per second
cvar_t      show_speed = {"show_speed", "0", false}; // JPG - draw frames per second

qboolean	scr_initialized;		// ready to draw

qpic_t		*scr_ram;
qpic_t		*scr_net;
qpic_t		*scr_turtle;

int			scr_fullupdate;

int			clearconsole;
int			clearnotify;

viddef_t	vid;				// global video state

vrect_t		*pconupdate;
vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;
qboolean	scr_skipupdate;

qboolean	block_drawing;

void SCR_ScreenShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}

void SCR_EraseCenterString (void)
{
	int		y;

	if (scr_erase_center++ > vid.numpages)
	{
		scr_erase_lines = 0;
		return;
	}

	if (scr_center_lines <= 4)
		y = vid.height*0.35;
	else
		y = 48;

	scr_copytop = 1;
	Draw_TileClear (0, y,vid.width, 8*scr_erase_lines);
}

void SCR_DrawCenterString (void)
{
	char	*start;
	int	l, j, x, y, remaining;

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = vid.height*0.35;
	else
		y = 48;

	do {
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (vid.width - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);
			if (!remaining--)
				return;
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

extern cvar_t cl_scoreboard_clean;
extern qboolean sb_showscores;
void SCR_CheckDrawCenterString (void)
{
	scr_copytop = 1;
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;

	if (key_dest != key_game)
		return;

	if (sb_showscores && cl_scoreboard_clean.value)
		return;


	SCR_DrawCenterString ();
}

//=============================================================================

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
        float   a, x;

        if (fov_x < 1 || fov_x > 179)
                Sys_Error ("Bad fov: %f", fov_x);

        x = width/tan(fov_x/360*M_PI);
        a = atan (height/x);
        a = a*360/M_PI;
        return a;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	vrect_t		vrect;
	float		size;

	scr_fullupdate = 0;		// force a background redraw
	vid.recalc_refdef = 0;

// force the status bar to redraw
	Sbar_Changed ();

//========================================

// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetValueByRef (&scr_viewsize, 30);
	if (scr_viewsize.value > 120)
		Cvar_SetValueByRef (&scr_viewsize, 120);

// bound field of view
	if (scr_fov.value < 10)
		Cvar_SetValueByRef (&scr_fov, 10);
	if (scr_fov.value > 170)
		Cvar_SetValueByRef (&scr_fov, 170);

	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

// intermission is always full screen
	if (cl.intermission)
		size = 120;
	else
		size = scr_viewsize.value;

	if (size >= 120)
		sb_lines = 0;		// no status bar at all
	else if (size >= 110)
		sb_lines = 24;		// no inventory
	else
		sb_lines = 24+16+8;

// these calculations mirror those in R_Init() for r_refdef, but take no
// account of water warping
	vrect.x = 0;
	vrect.y = 0;
	vrect.width = vid.width;
	vrect.height = vid.height;

	R_SetVrect (&vrect, &scr_vrect, sb_lines);

// guard against going from one mode to another that's less than half the
// vertical resolution
	if (scr_con_current > vid.height)
		scr_con_current = vid.height;

// notify the refresh of the change
	R_ViewChanged (&vrect, sb_lines, vid.aspect);
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValueByRef (&scr_viewsize,scr_viewsize.value+10);
	vid.recalc_refdef = 1;
}

/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValueByRef (&scr_viewsize,scr_viewsize.value-10);
	vid.recalc_refdef = 1;
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void CL_Default_fov_f(void);
void CL_Fov_f(void);
void SCR_Init (void)
{
	Cvar_RegisterVariable (&default_fov, CL_Default_fov_f);
	Cvar_RegisterVariable (&scr_fov, CL_Fov_f);

	Cvar_RegisterVariable (&scr_viewsize, NULL);
	Cvar_RegisterVariable (&scr_conspeed, NULL);
	Cvar_RegisterVariable (&scr_showram, NULL);
	Cvar_RegisterVariable (&scr_showturtle, NULL);
	Cvar_RegisterVariable (&scr_showpause, NULL);
	Cvar_RegisterVariable (&scr_centertime, NULL);
	Cvar_RegisterVariable (&scr_printspeed, NULL);

	Cvar_RegisterVariable (&pq_drawfps, NULL); // JPG - draw frames per second
	Cvar_RegisterVariable (&show_speed, NULL); // Baker 3.67

// register our commands

	Cmd_AddCommand ("screenshot",SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	scr_ram = Draw_PicFromWad ("ram");
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");

#ifdef SUPPORTS_AVI_CAPTURE
	Movie_Init ();
#endif

	scr_initialized = true;
}


/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
	if (!scr_showram.value)
		return;

	if (!r_cache_thrash)
		return;

	Draw_Pic (scr_vrect.x+32, scr_vrect.y, scr_ram);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;

	if (!scr_showturtle.value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;

	if (cls.demoplayback)
		return;

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

/* JPG - draw frames per second
==============
SCR_DrawFPS
==============
*/
void SCR_DrawFPS (void)
{
	int x;
	static double last_realtime = 0.0;
	static int last_framecount = 0;
	static int fps = 0;
	char buff[10];
	char *ch;

	if (realtime - last_realtime > 1.0)
	{
		fps = (host_framecount - last_framecount) / (realtime - last_realtime) + 0.5;
		last_framecount = host_framecount;
		last_realtime = realtime;
	}

	if (!pq_drawfps.value)
		return;

	snprintf (buff, sizeof(buff), "%3d", fps);
	x = vid.width - 48;

	ch = buff;

	while (*ch)
	{
		Draw_Character(x, (!show_speed.value ? 8 : 16), *ch);
		x += 8;
		ch++;
	}
}

#ifdef HTTP_DOWNLOAD
/*
==============
SCR_DrawWebPercent
==============
*/
void SCR_DrawWebPercent (void)
{
	int x;
	char buff[20];
	char *ch;

	snprintf (buff, sizeof(buff), "download: %2.1f%%", (float)(cls.download.percent*100));
	x = vid.width - (16*8); //64; // 16 x 3 = 48 ... we need 16 x 4 = 64

	Draw_Fill (0, 20, vid.width, 2, 0);
	Draw_Fill (0, 0, vid.width, 20, 98);
	Draw_Fill (0, 8, (int)((vid.width - (18*8)) * cls.download.percent), 8, 8);

	ch = buff;

	while (*ch)
	{
		Draw_Character(x, 8, (*ch)+128);
		x += 8;
		ch++;
	}
}
#endif

/*
==============
SCR_DrawSpeed - Baker 3.67 from JoeQuake
==============
*/
void SCR_DrawSpeed (void)
{
	int		x;
	char buff[10];
	char *ch;
	float		speed, vspeed;
	vec3_t		vel;
	static	float	maxspeed = 0, display_speed = -1;
	static	double	lastrealtime = 0;

	if (!show_speed.value)
		return;

	if (lastrealtime > realtime)
	{
		lastrealtime = 0;
		display_speed = -1;
		maxspeed = 0;
	}

	VectorCopy (cl.velocity, vel);
	vspeed = vel[2];
	vel[2] = 0;
	speed = VectorLength (vel);

	if (speed > maxspeed)
		maxspeed = speed;

	if (display_speed >= 0)
	{
		snprintf(buff, sizeof(buff), "%3d", (int)display_speed);
	ch = buff;
	x = vid.width - 48;
	while (*ch)
	{
			Draw_Character(x, 8, (*ch)+128);
		x += 8;
		ch++;
	}
}

	if (realtime - lastrealtime >= 0.1)
	{
		lastrealtime = realtime;
		display_speed = maxspeed;
		maxspeed = 0;
	}
}


/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	qpic_t	*pic;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	if (!cl.paused)
		return;

	pic = Draw_CachePic ("gfx/pause.lmp");
	Draw_Pic ( (vid.width - pic->width)/2, (vid.height - 48 - pic->height)/2, pic);
}


/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	qpic_t	*pic;

	if (!scr_drawloading)
		return;

	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_Pic ( (vid.width - pic->width)/2, (vid.height - 48 - pic->height)/2, pic);
}


//=============================================================================

/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	//johnfitz -- let's hack away the problem of slow console when host_timescale is <0
	extern cvar_t host_timescale;
	float timescale;
	//johnfitz

	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = vid.height;		// full screen
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
		scr_conlines = vid.height/2;	// half screen
	else
		scr_conlines = 0;				// none visible

	timescale = (host_timescale.value > 0) ? host_timescale.value : 1; //johnfitz -- timescale

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed.value*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed.value*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
	{
		scr_copytop = 1;
		Draw_TileClear (0,(int)scr_con_current,vid.width, vid.height - (int)scr_con_current);
		Sbar_Changed ();
	}
	else if (clearnotify++ < vid.numpages)
	{
		scr_copytop = 1;
		Draw_TileClear (0,0,vid.width, con_notifylines);
	}
	else
	{
		con_notifylines = 0;
}
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		scr_copyeverything = 1;
		Con_DrawConsole (scr_con_current, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/


typedef struct
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
} pcx_t;

#ifdef SUPPORTS_SW_SKYBOX
/*
============
LoadPCX
============
*/
void LoadPCX (char *filename, byte **pic, int *width, int *height)
{
	pcx_t	*pcx;
	byte	*pcxbuf, *out, *pix;
	int		x, y;
	int		dataByte, runLength;

	*pic = NULL;
	pcxbuf = COM_LoadTempFile (filename);
	if (!pcxbuf)
		return;

//
// parse the PCX file
//
	pcx = (pcx_t *)pcxbuf;
	pcx->xmax = LittleShort (pcx->xmax);
	pcx->xmin = LittleShort (pcx->xmin);
	pcx->ymax = LittleShort (pcx->ymax);
	pcx->ymin = LittleShort (pcx->ymin);
	pcx->hres = LittleShort (pcx->hres);
	pcx->vres = LittleShort (pcx->vres);
	pcx->bytes_per_line = LittleShort (pcx->bytes_per_line);
	pcx->palette_type = LittleShort (pcx->palette_type);

	pix = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
/*		|| pcx->ymax >= 480*/)
	{
		Con_Printf ("Bad pcx file\n");
		return;
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	*pic = out = Q_malloc ((pcx->xmax+1) * (pcx->ymax+1));

	for (y=0 ; y<=pcx->ymax ; y++, out += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *pix++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *pix++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				out[x++] = dataByte;
		}
	}
}
// Manoel Kasimier - skyboxes - end
#endif

#if !defined(MACOSX)

/*
==============
WritePCXfile
==============
*/
void WritePCXfile (char *filename, byte *data, int width, int height,
	int rowbytes, byte *palette)
{
	int		i, j, length;
	pcx_t	*pcx;
	byte		*pack;

	pcx = Hunk_TempAlloc (width*height*2+1000);
	if (pcx == NULL)
	{
		Con_Printf("SCR_ScreenShot_f: not enough memory\n");
		return;
	}

	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;			// 256 color
 	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;		// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = LittleShort((short)(width-1));
	pcx->ymax = LittleShort((short)(height-1));
	pcx->hres = LittleShort((short)width);
	pcx->vres = LittleShort((short)height);
	memset (pcx->palette,0,sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = LittleShort((short)width);
	pcx->palette_type = LittleShort(2);		// not a grey scale
	memset (pcx->filler,0,sizeof(pcx->filler));

// pack the image
	pack = &pcx->data;

	for (i=0 ; i<height ; i++)
	{
		for (j=0 ; j<width ; j++)
		{
			if ( (*data & 0xc0) != 0xc0)
				*pack++ = *data++;
			else
			{
				*pack++ = 0xc1;
				*pack++ = *data++;
			}
		}

		data += rowbytes - width;
	}

// write the palette
	*pack++ = 0x0c;	// palette ID byte
	for (i=0 ; i<768 ; i++)
		*pack++ = *palette++;

// write output file
	length = pack - (byte *)pcx;
	COM_WriteFile (filename, pcx, length);
}

#else

/*
==============
WritePNGFile
==============
*/
qboolean WritePNGFile (char *theFileName, byte *theData, int theWidth, int theHeight, int theRowBytes, byte *thePalette)
{
    extern qboolean	VID_Screenshot (char *, unsigned char *, unsigned int, unsigned int, unsigned int);
    unsigned int	mySize = theWidth * theHeight,
                        myColorIndex,
                        i, j;
    unsigned char	*myRawRGBData = NULL,
                        *myRGBPixel = NULL;

    // get some temp memory for the RGB data:
    myRGBPixel = myRawRGBData = Hunk_TempAlloc (mySize * 3);
    if (myRawRGBData == NULL)
    {
        Con_Printf ("SCR_ScreenShot_f: not enough memory\n");
        return (false);
    }

    // convert the indexed color data to RGB raw data:
    for (i = 0; i < theHeight; i++)
    {
        for (j = 0; j < theWidth; j++)
        {
            myColorIndex = *(theData++) * 3;
            *(myRGBPixel++) = thePalette[myColorIndex++];
            *(myRGBPixel++) = thePalette[myColorIndex++];
            *(myRGBPixel++) = thePalette[myColorIndex];
        }
        theData += theRowBytes - theWidth;
    }

    // finally write the PNG file:
    return (VID_Screenshot (theFileName, myRawRGBData, theWidth, theHeight, theWidth * 3));
}

#endif /* !MACOSX  */

/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (void)
{
	int     i;
	char		pcxname[80];
	char		checkname[MAX_OSPATH];

// find a file name to save it to

#ifdef MACOSX
        qboolean	success = false;

	strcpy(pcxname,"quake00.png");
#else
	strcpy(pcxname,"quake00.pcx");
#endif /* MACOSX */

	for (i=0 ; i<=99 ; i++)
	{
		pcxname[5] = i/10 + '0';
		pcxname[6] = i%10 + '0';
		snprintf (checkname, sizeof(checkname), "%s/%s", com_gamedir, pcxname);
		if (Sys_FileTime(checkname) == -1)
			break;	// file doesn't exist
	}
	if (i==100)
	{
#ifdef MACOSX
		Con_Printf ("SCR_ScreenShot_f: Couldn't create a PNG file\n");
#else
		Con_Printf ("SCR_ScreenShot_f: Couldn't create a PCX file\n");
#endif /* MACOSX */
		return;
 	}

// save the pcx file

	D_EnableBackBufferAccess ();	// enable direct drawing of console to back buffer

#ifdef MACOSX
        success = WritePNGFile (checkname, vid.buffer, vid.width, vid.height, vid.rowbytes, host_basepal);
	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in for linear writes all the time
        if (success == true)
            Con_Printf ("Wrote %s\n", pcxname);
#else
	WritePCXfile (pcxname, vid.buffer, vid.width, vid.height, vid.rowbytes, host_basepal);

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in for linear writes all the time

	Con_Printf ("Wrote %s\n", pcxname);
#endif /* MACOSX */

}

//=============================================================================


/*
===============
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;

// redraw with no console and the loading plaque
	Con_ClearNotify ();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	scr_fullupdate = 0;
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
	scr_fullupdate = 0;
}

/*
===============
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	scr_fullupdate = 0;
	Con_ClearNotify ();
}

//=============================================================================

char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	char	*start;
	int	l, j, x, y;

	start = scr_notifystring;

	y = vid.height*0.35;

	do {
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (vid.width - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage (char *text, float timeout) //johnfitz -- timeout
{
	double time1, time2; //johnfitz -- timeout

#ifdef FLASH
	return true;	//For Flash we receive key messages via the Main.as file, between calls to the Alchemy C source.
					//We therefore cant check what the user response was, so we just assume that it was 'yes'.
#endif

	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;

// draw a fresh screen
	scr_fullupdate = 0;
	scr_drawdialog = true;
	SCR_UpdateScreen ();
#ifdef MACOSX_QUESTIONABLE_VALUE
        scr_drawdialog = false;
#endif /* !MACOSX  */

	S_ClearBuffer ();		// so dma doesn't loop current sound

	time1 = Sys_DoubleTime () + timeout; //johnfitz -- timeout
	time2 = 0.0f; //johnfitz -- timeout

	do 	{
		key_count = -1;		// wait for a key down and up
		Sys_SendKeyEvents ();
		if (timeout) time2 = Sys_DoubleTime (); //johnfitz -- zero timeout means wait forever.
	} while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE  && time2 <= time1);

#ifdef MACOSX_QUESTIONABLE_VALUE
        scr_drawdialog = false;
#endif /* MACOSX */
	scr_fullupdate = 0;
	SCR_UpdateScreen ();
	//johnfitz -- timeout
	if (time2 > time1)
		return false;
	//johnfitz

	return key_lastpress == 'y';
}

//=============================================================================

/*
===============
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole (void)
{
	int		i;

	scr_centertime_off = 0;

	for (i=0 ; i<20 && scr_conlines != scr_con_current ; i++)
		SCR_UpdateScreen ();

	cl.cshifts[0].percent = 0;		// no area contents palette on next frame
	VID_SetPaletteOld (host_basepal);
}

void Mat_Update (void);	// JPG


/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	static float	oldscr_viewsize;
	static float	oldlcd_x;
	vrect_t		vrect;

	if (cls.state == ca_dedicated)
		return;				// stdout only


	if (scr_skipupdate || block_drawing)
		return;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
			scr_disabled_for_loading = false;  // Con_Printf ("load failed.\n");
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

#ifdef _WIN32
	{	// don't suck up any cpu if minimized
		extern	int	Minimized;

		if (Minimized)
			return;
	}
#endif

#ifdef _WIN32
	{	// don't suck up any cpu if minimized
		extern	int	Minimized;

		if (Minimized)
			return;
	}
#endif

#ifdef MACOSX
	if (qMinimized) {
			Sys_Sleep ();
			return;
	}
#endif

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (scr_viewsize.value != oldscr_viewsize)
	{
		oldscr_viewsize = scr_viewsize.value;
		vid.recalc_refdef = 1;
	}

// check for vid changes
	if (oldfov != scr_fov.value)
	{
		oldfov = scr_fov.value;
		vid.recalc_refdef = true;
	}

	if (oldlcd_x != lcd_x.value)
	{
		oldlcd_x = lcd_x.value;
		vid.recalc_refdef = true;
	}

	if (oldscreensize != scr_viewsize.value)
	{
		oldscreensize = scr_viewsize.value;
		vid.recalc_refdef = true;
	}

	if (vid.recalc_refdef)
	{
	// something changed, so reorder the screen
		SCR_CalcRefdef ();
	}

// do 3D refresh drawing, and then update the screen
	D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly

	if (scr_fullupdate++ < vid.numpages)
	{	// clear the entire screen
		scr_copyeverything = 1;
		Draw_TileClear (0,0,vid.width,vid.height);
		Sbar_Changed ();
	}

	pconupdate = NULL;

	SCR_SetUpToDrawConsole ();
	SCR_EraseCenterString ();

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in for linear writes all the time

	VID_LockBuffer ();
	V_RenderView ();
	VID_UnlockBuffer ();

	D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly

	if (scr_drawdialog) //new game confirm
	{
		Sbar_Draw ();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
		scr_copyeverything = true;
	}
	else if (scr_drawloading) //loading
	{
		SCR_DrawLoading ();
		Sbar_Draw ();
	}
	else if (cl.intermission == 1 && key_dest == key_game) //end of level
	{
		Sbar_IntermissionOverlay ();
	}
	else if (cl.intermission == 2 && key_dest == key_game) //end of episode
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	}
	else if (cl.intermission == 3 && key_dest == key_game)
	{
		SCR_CheckDrawCenterString ();
	}
	else
	{
		SCR_DrawRam ();
		SCR_DrawNet ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();

		if (cls.state == ca_connected) {
			void Draw_Crosshair (void);
			Draw_Crosshair ();
#ifdef SUPPORTS_AUTOID_SOFTWARE
			R_DrawNameTags();
#endif
		SCR_DrawFPS (); // JPG - draw FPS
		SCR_DrawSpeed (); // Baker 3.67 - Drawspeed
		SCR_CheckDrawCenterString ();
		SCR_DrawVolume (); // Baker 3.60 - JoeQuake 0.15
		Sbar_Draw ();
		}

		if (mod_conhide==false || (key_dest == key_console || key_dest == key_message))
			SCR_DrawConsole ();

#ifdef HTTP_DOWNLOAD
		if (cls.download.web)
			SCR_DrawWebPercent ();
#endif

		M_Draw ();
		Mat_Update ();	// JPG
	}

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in for linear writes all the time

#if 0
	if (pconupdate) {
		D_UpdateRects (pconupdate);
	}
#endif

	V_UpdatePalette_Software ();

// update one of three areas
	if (scr_copyeverything)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height;
		vrect.pnext = 0;

		VID_Update (&vrect);
	}
	else if (scr_copytop)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height - sb_lines;
		vrect.pnext = 0;

		VID_Update (&vrect);
	}
	else
	{
		vrect.x = scr_vrect.x;
		vrect.y = scr_vrect.y;
		vrect.width = scr_vrect.width;
		vrect.height = scr_vrect.height;
		vrect.pnext = 0;

		VID_Update (&vrect);
	}

#ifdef SUPPORTS_AVI_CAPTURE
	Movie_UpdateScreen ();
#endif
}

/*
==================
SCR_UpdateWholeScreen
==================
*/
void SCR_UpdateWholeScreen (void)
{
	scr_fullupdate = 0;
	SCR_UpdateScreen ();
}
