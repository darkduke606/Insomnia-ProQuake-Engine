/*
Copyright (C) 1996-1997  Id Software, Inc.
Copyright (C) 1999-2000  contributors of the QuakeForge project
Copyright (C) 1999-2000  Nelson Rush.
Copyright (C) 2000       Marcus Sundberg [mackan@stacken.kth.se]
Please see the file "AUTHORS" for a list of contributors

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

// JPG 1.05 - completely new file supplied by CSR because the old one was badly broken.
// If you have problems with this file, the original version is in gl_vidlinuxglx.c.bak.

#include "quakedef.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>

#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif
#ifndef RTLD_LAZY
# ifdef DL_LAZY
#  define RTLD_LAZY	DL_LAZY
# else
#  define RTLD_LAZY	0
# endif
#endif

#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>

# include <X11/extensions/xf86dga.h>
# include <X11/extensions/xf86vmode.h>

#ifdef XMESA
# include <GL/xmesa.h>
#endif

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

static Display		*dpy = NULL;
static int		screen;
static Window		win;
static Cursor		cursor = None;
static GLXContext	ctx = NULL;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask)
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | \
	StructureNotifyMask)

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];
unsigned char	d_15to8table[65536];

cvar_t	_windowed_mouse = {"_windowed_mouse", "1", true};
cvar_t	m_filter = {"m_filter", "1"};
cvar_t	vid_mode = {"vid_mode", "0", false};
cvar_t  vid_glx_fullscreen = {"vid_glx_fullscreen", "1", false};

static int	fullscreen = 0;

static float	mouse_x, mouse_y;
static float	old_mouse_x, old_mouse_y;
static int	mouse_grabbed = 0;
#define	mouse_shouldgrab ((int)vid_glx_fullscreen.value ||(int)_windowed_mouse.value)

static int	nummodes;
static XF86VidModeModeInfo **vidmodes;
static int	hasdgavideo = 0, hasvidmode = 0;
static int	dgamouse = 0;
static cvar_t	vid_dga_mouseaccel = {"vid_dga_mouseaccel", "1", true};

#ifdef XMESA
static int	xmesafullscreen = 0;
#endif

#ifdef HAVE_DLOPEN
static void	*dlhand = NULL;
#endif
static int	hasdga = 0;
static GLboolean (*QF_XMesaSetFXmode)(GLint mode) = NULL;


static int scr_width, scr_height;

//#if defined(XMESA) || defined(HAS_DGA)
int VID_options_items = 2;
#if 0
//#else
int VID_options_items = 1;
#endif

/*-----------------------------------------------------------------------*/

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

cvar_t	gl_ztrick = {"gl_ztrick", "0", true};

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

static float vid_gamma = 1.0;

qboolean is8bit = false;
qboolean isPermedia = false;
qboolean gl_mtexable = false;

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

/*
  VID_CheckDGA

  Check for the presence of the XFree86-DGA X server extension
*/
int
VID_CheckDGA(Display *dpy, int *maj_ver, int *min_ver, int *hasvideo)
{
	int event_base, error_base, dgafeat, dummy;

	if (! XF86DGAQueryExtension(dpy, &event_base, &error_base)) {
		return 0;
	}

	if (maj_ver == NULL) maj_ver = &dummy;
	if (min_ver == NULL) min_ver = &dummy;

	if (! XF86DGAQueryVersion(dpy, maj_ver, min_ver)) {
		return 0;
	}
	if (! XF86DGAQueryDirectVideo(dpy, DefaultScreen(dpy), &dgafeat)) {
		*hasvideo = 0;
	} else {
		*hasvideo = (dgafeat & XF86DGADirectPresent);
	}

	return 1;
}


/*
  VID_CheckVMode

  Check for the presence of the XFree86-VidMode X server extension
*/
int
VID_CheckVMode(Display *dpy, int *maj_ver, int *min_ver)
{
	int event_base, error_base;
	int dummy;

	if (! XF86VidModeQueryExtension(dpy, &event_base, &error_base)) {
		return 0;
	}

	if (maj_ver == NULL) maj_ver = &dummy;
	if (min_ver == NULL) min_ver = &dummy;

	if (! XF86VidModeQueryVersion(dpy, maj_ver, min_ver)) {
		return 0;
	}

	return 1;
}

static void
blank_cursor(void)
{
	Pixmap blank;
	XColor dummy;
	char data[1] = {0};

	blank = XCreateBitmapFromData(dpy, win, data, 1, 1);
	if (blank == None) {
		fprintf(stderr,"Could not create cursor: Out of memory.\n");
	} else {
		cursor = XCreatePixmapCursor(dpy, blank, blank,
					     &dummy, &dummy, 0, 0);
		XFreePixmap(dpy, blank);
	}
}


static void
do_grabs(int grab)
{
	if (grab == mouse_grabbed) return;

	if (grab) {
		/*
		  Grab mouse
		*/
		if (cursor == None) {
			blank_cursor();
		}

		if (XGrabPointer(dpy, win, True, 0, GrabModeAsync,
				 GrabModeAsync, win, cursor, CurrentTime)
		    != GrabSuccess) {
			Con_Printf("Unable to grab pointer\n");
			Cvar_SetValueByRef (&vid_glx_fullscreen, 0);
			Cvar_SetValueByRef (&_windowed_mouse, 0);
			return;
		}

		/* We can live with this failing. */
		XGrabKeyboard(dpy, win, False, GrabModeAsync, GrabModeAsync,
			      CurrentTime);

		if (hasdga) {
			XF86DGADirectVideo(dpy, screen, XF86DGADirectMouse);
			dgamouse = 1;
		} else
		{
			XWarpPointer(dpy, None, win, 0, 0, 0, 0,
				     vid.width / 2, vid.height / 2);
		}
		mouse_grabbed = 1;
	} else {
		/*
		  Release grab
		*/
		if (dgamouse) {
			XF86DGADirectVideo(dpy, screen, 0);
			dgamouse = 0;
		}
		XUngrabKeyboard(dpy, CurrentTime);
		XUngrabPointer(dpy, CurrentTime);
		mouse_grabbed = 0;
	}
}


static void
do_fullscreen(int full)
{
	if (full == fullscreen) return;

#ifdef XMESA
	if (QF_XMesaSetFXmode) {
		if (QF_XMesaSetFXmode(full ? XMESA_FX_FULLSCREEN
				      : XMESA_FX_WINDOW)) {
			fullscreen = full;
			xmesafullscreen = full;
			return;
		}
		if (xmesafullscreen) {
			/* We are in XMesa fullscren mode and couldn't switch
			   back to windowed mode ??? */
			Cvar_SetValue("vid_glx_fullscreen", fullscreen);
			do_grabs(mouse_shouldgrab);
			return;
		}
	}
#endif
	if (hasdga && hasvidmode) {
		static int prev_x = 0, prev_y = 0, prev_w = 640, prev_h = 480;

		if (full) {
			Window dumwin;
			unsigned int dummy, i, curw = 65535, curh = 65535;
			int curmode = -1;

			XGetGeometry(dpy, win, &dumwin, &prev_x, &prev_y,
				     &prev_w, &prev_h, &dummy, &dummy);

			for (i = 0; i < nummodes; i++) {
				if (vidmodes[i]->hdisplay == scr_width &&
				    vidmodes[i]->vdisplay == scr_height) {
					curmode = i;
					break;
				}
				if (vidmodes[i]->hdisplay
				    >= (scr_width - 10) &&
				    vidmodes[i]->vdisplay
				    >= (scr_height - 10) &&
				    vidmodes[i]->hdisplay <= curw &&
				    vidmodes[i]->vdisplay <= curh) {
					curw = vidmodes[i]->hdisplay;
					curh = vidmodes[i]->vdisplay;
					curmode = i;
				}
			}
			if (curmode >= 0 &&
			    XF86VidModeSwitchToMode(dpy,
				    screen, vidmodes[curmode])) {
				XSync(dpy, 0);
				XF86VidModeSetViewPort(dpy, screen, 0, 0);
				XMoveResizeWindow(dpy, win, 0, 0,
						vidmodes[curmode]->hdisplay,
						vidmodes[curmode]->vdisplay);
				fullscreen = full;
				return;
			}
		} else {
			XF86VidModeSwitchToMode(dpy, screen, vidmodes[0]);
			XSync(dpy, 0);
			XMoveResizeWindow(dpy, win, prev_x, prev_y,
					  prev_w, prev_h);
			fullscreen = full;
			return;
		}
	}
	/* Failed to change anything */
	Cvar_SetValue("vid_glx_fullscreen", fullscreen);
}


void
VID_Shutdown(void)
{
	if (!ctx) return;

	glXDestroyContext(dpy, ctx);
	ctx = NULL;
	if (hasvidmode) {
		int i;

		for (i = 0; i < nummodes; i++) {
			if (vidmodes[i]->private) XFree(vidmodes[i]->private);
		}
		XFree(vidmodes);
	}
#ifdef HAVE_DLOPEN
	if (dlhand) {
		dlclose(dlhand);
		dlhand = NULL;
	}
#endif

	XCloseDisplay(dpy);
	dpy = NULL;
}


static void
signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Sys_Quit();
	exit(sig);
}

static void
InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
/*	signal(SIGFPE, signal_handler); */
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

void VID_ShiftPalette(unsigned char *p)
{
#ifdef _EXPERIMENTAL_
	VID_SetPaletteOld(p);
#endif
}

void	VID_SetPaletteOld (unsigned char *palette)
{
	byte	*pal;
	unsigned r,g,b;
	unsigned v;
	int     r1,g1,b1;
	int		k;
	unsigned short i;
	unsigned	*table;
	FILE *f;
	char s[255];
	float dist, bestdist;
	static qboolean palflag = false;

//
// 8 8 8 encoding
//
//	Con_Printf("Converting 8to24\n");

	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;

//		v = (255<<24) + (r<<16) + (g<<8) + (b<<0);
//		v = (255<<0) + (r<<8) + (g<<16) + (b<<24);
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent

	// JACK: 3D distance calcs - k is last closest, l is the distance.
	// FIXME: Precalculate this and cache to disk.
	if (palflag)
		return;
	palflag = true;

	COM_FOpenFile("glquake/15to8.pal", &f);
	if (f) {
		fread(d_15to8table, 1<<15, 1, f);
		fclose(f);
	} else {
		for (i=0; i < (1<<15); i++) {
			/* Maps
 			000000000000000
 			000000000011111 = Red  = 0x1F
 			000001111100000 = Blue = 0x03E0
 			111110000000000 = Grn  = 0x7C00
 			*/
 			r = ((i & 0x1F) << 3)+4;
 			g = ((i & 0x03E0) >> 2)+4;
 			b = ((i & 0x7C00) >> 7)+4;
			pal = (unsigned char *)d_8to24table;
			for (v=0,k=0,bestdist=10000.0; v<256; v++,pal+=4) {
 				r1 = (int)r - (int)pal[0];
 				g1 = (int)g - (int)pal[1];
 				b1 = (int)b - (int)pal[2];
				dist = sqrt(((r1*r1)+(g1*g1)+(b1*b1)));
				if (dist < bestdist) {
					k=v;
					bestdist = dist;
				}
			}
			d_15to8table[i]=k;
		}
		snprintf(s, sizeof(s), "%s/glquake", com_gamedir);
 		Sys_mkdir (s);
		snprintf(s, sizeof(s), "%s/glquake/15to8.pal", com_gamedir);
		if ((f = fopen(s, "wb")) != NULL) {
			fwrite(d_15to8table, 1<<15, 1, f);
			fclose(f);
		}
	}
}


/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

	glClearColor (0.15,0.15,0.15,0);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;

//    if (!wglMakeCurrent( maindc, baseRC ))
//		Sys_Error ("wglMakeCurrent failed");

//	glViewport (*x, *y, *width, *height);
}


void GL_EndRendering (void)
{
	glFlush();
	glXSwapBuffers(dpy, win);
}

qboolean VID_Is8bit(void)
{
	return is8bit;
}

#ifdef GL_EXT_SHARED
void VID_Init8bitPalette()
{
	// Check for 8bit Extensions and initialize them.
	int i;
	char thePalette[256*3];
	char *oldPalette, *newPalette;

	if (strstr(gl_extensions, "GL_EXT_shared_texture_palette") == NULL)
		return;

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
	glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE, (void *) thePalette);
	is8bit = true;
}

#else

void VID_Init8bitPalette(void)
{
}

#endif

// CSR
static void Check_GammaOld (unsigned char *pal)
{
	float   f, inf;
	unsigned char   palette[768];
	int             i;

	if ((i = COM_CheckParm("-gamma")) == 0) {
		if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
			(gl_vendor && strstr(gl_vendor, "3Dfx")))
			vid_gamma = 1;
		else
			vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
	} else
		vid_gamma = atof(com_argv[i+1]);

	for (i=0 ; i<768 ; i++)
	{
		f = pow ( (pal[i]+1)/256.0 , vid_gamma );
		inf = f*255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	memcpy (pal, palette, sizeof(palette));

	BuildGammaTable(vid_gamma);
}


void VID_Init(unsigned char *palette)
{
	int i;
	int attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		None
	};
	char	gldir[MAX_OSPATH];
	int width = 640, height = 480;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root;
	XVisualInfo *visinfo;

	//S_Init();

	Cvar_RegisterVariable(&vid_mode);
	Cvar_RegisterVariable(&gl_ztrick);
	Cvar_RegisterVariable(&_windowed_mouse);
        Cvar_RegisterVariable(&vid_glx_fullscreen);
	Cvar_RegisterVariable(&vid_dga_mouseaccel);

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	/* Interpret command-line params
	 */

	/* Set vid parameters */
	if ((i = COM_CheckParm("-width")) != 0)
		width = atoi(com_argv[i+1]);
	if ((i = COM_CheckParm("-height")) != 0)
		height = atoi(com_argv[i+1]);

	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = atoi(com_argv[i+1]);
	else
		vid.conwidth = width;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = atoi(com_argv[i+1]);
	if (vid.conheight < 200)
		vid.conheight = 200;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error couldn't open the X display\n");
		exit(1);
	}

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	visinfo = glXChooseVisual(dpy, screen, attrib);
	if (!visinfo) {
		fprintf(stderr, "Error couldn't get an RGB, Double-buffered, Depth visual\n");
		exit(1);
	}

	{
		int maj_ver;

		hasdga = VID_CheckDGA(dpy, &maj_ver, NULL, &hasdgavideo);
		if (!hasdga || maj_ver < 1) {
			hasdga = hasdgavideo = 0;
		}
	}
	hasvidmode = VID_CheckVMode(dpy, NULL, NULL);
	if (hasvidmode) {
		if (! XF86VidModeGetAllModeLines(dpy, DefaultScreen(dpy),
						 &nummodes, &vidmodes)
		    || nummodes <= 0) {
			hasvidmode = 0;
		}
	}
//#endif
#ifdef HAVE_DLOPEN
	dlhand = dlopen(NULL, RTLD_LAZY);
	if (dlhand) {
		QF_XMesaSetFXmode = dlsym(dlhand, "XMesaSetFXmode");
		if (!QF_XMesaSetFXmode) {
			QF_XMesaSetFXmode = dlsym(dlhand,
						  "_XMesaSetFXmode");
		}
	} else {
		QF_XMesaSetFXmode = NULL;
	}
#else
#ifdef XMESA
	QF_XMesaSetFXmode = XMesaSetFXmode;
#endif
#endif
	if (QF_XMesaSetFXmode) {
#ifdef XMESA
		const char *str = getenv("MESA_GLX_FX");
		if (str != NULL && *str != 'f') {
			if (tolower(*str) == 'w') {
				Cvar_SetValueByRef (&vid_glx_fullscreen, 0);
			} else {
				Cvar_SetValueByRef (&vid_glx_fullscreen, 1);
			}
		}
#endif
		/* Glide uses DGA internally, so we don't want to
		   mess with it. */
		hasdga = 0;
	}

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = X_MASK;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, 0, 0, width, height,
						0, visinfo->depth, InputOutput,
						visinfo->visual, mask, &attr);
	XMapWindow(dpy, win);

	XSync(dpy, 0);

	ctx = glXCreateContext(dpy, visinfo, NULL, True);

	glXMakeCurrent(dpy, win, ctx);

	scr_width = width;
	scr_height = height;

	if (vid.conheight > height)
		vid.conheight = height;
	if (vid.conwidth > width)
		vid.conwidth = width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;

	InitSig(); // trap evil signals

	GL_Init();

	snprintf(gldir, sizeof(gldir), "%s/OpenGL", com_gamedir);
	Sys_mkdir (gldir);

        Check_GammaOld(palette);
	VID_SetPaletteOld(palette);

	// Check for 3DFX Extensions and initialize them.
	VID_Init8bitPalette();

	Con_SafePrintf ("Video mode %dx%d initialized.\n", width, height);

	vid.recalc_refdef = 1;		// force a surface cache flush

	do_grabs(mouse_shouldgrab);
}


static int
XLateKey(XKeyEvent *ev)
{
	int key = 0;
	char buf[64];
	KeySym keysym;

	XLookupString(ev, buf, sizeof(buf), &keysym, 0);

	switch(keysym) {
		case XK_KP_Page_Up:
		case XK_Page_Up:	key = K_PGUP; break;

		case XK_KP_Page_Down:
		case XK_Page_Down:	key = K_PGDN; break;

		case XK_KP_Home:
		case XK_Home:		key = K_HOME; break;

		case XK_KP_End:
		case XK_End:		key = K_END; break;

		case XK_KP_Left:
		case XK_Left:		key = K_LEFTARROW; break;

		case XK_KP_Right:
		case XK_Right:		key = K_RIGHTARROW; break;

		case XK_KP_Down:
		case XK_Down:		key = K_DOWNARROW; break;

		case XK_KP_Up:
		case XK_Up:		key = K_UPARROW; break;

		case XK_Escape:		key = K_ESCAPE; break;

		case XK_KP_Enter:
		case XK_Return:		key = K_ENTER; break;

		case XK_Tab:		key = K_TAB; break;

		case XK_F1:		key = K_F1; break;
		case XK_F2:		key = K_F2; break;
		case XK_F3:		key = K_F3; break;
		case XK_F4:		key = K_F4; break;
		case XK_F5:		key = K_F5; break;
		case XK_F6:		key = K_F6; break;
		case XK_F7:		key = K_F7; break;
		case XK_F8:		key = K_F8; break;
		case XK_F9:		key = K_F9; break;
		case XK_F10:		key = K_F10; break;
		case XK_F11:		key = K_F11; break;
		case XK_F12:		key = K_F12; break;

		case XK_BackSpace:	key = K_BACKSPACE; break;

		case XK_KP_Delete:
		case XK_Delete:		key = K_DEL; break;

		case XK_Pause:		key = K_PAUSE; break;

		case XK_Shift_L:
		case XK_Shift_R:	key = K_SHIFT; break;

		case XK_Execute:
		case XK_Control_L:
		case XK_Control_R:	key = K_CTRL; break;

		case XK_Mode_switch:
		case XK_Alt_L:
		case XK_Meta_L:
		case XK_Alt_R:
		case XK_Meta_R:		key = K_ALT; break;

		case XK_KP_Begin:	key = '5'; break;

		case XK_KP_Insert:
		case XK_Insert:		key = K_INS; break;

		case XK_KP_Multiply:	key = '*'; break;
		case XK_KP_Add:		key = '+'; break;
		case XK_KP_Subtract:	key = '-'; break;
		case XK_KP_Divide:	key = '/'; break;

		/* For Sun keyboards */
		case XK_F27:		key = K_HOME; break;
		case XK_F29:		key = K_PGUP; break;
		case XK_F33:		key = K_END; break;
		case XK_F35:		key = K_PGDN; break;

#if 0
		case 0x021: key = '1';break;/* [!] */
		case 0x040: key = '2';break;/* [@] */
		case 0x023: key = '3';break;/* [#] */
		case 0x024: key = '4';break;/* [$] */
		case 0x025: key = '5';break;/* [%] */
		case 0x05e: key = '6';break;/* [^] */
		case 0x026: key = '7';break;/* [&] */
		case 0x02a: key = '8';break;/* [*] */
		case 0x028: key = '9';;break;/* [(] */
		case 0x029: key = '0';break;/* [)] */
		case 0x05f: key = '-';break;/* [_] */
		case 0x02b: key = '=';break;/* [+] */
		case 0x07c: key = '\'';break;/* [|] */
		case 0x07d: key = '[';break;/* [}] */
		case 0x07b: key = ']';break;/* [{] */
		case 0x022: key = '\'';break;/* ["] */
		case 0x03a: key = ';';break;/* [:] */
		case 0x03f: key = '/';break;/* [?] */
		case 0x03e: key = '.';break;/* [>] */
		case 0x03c: key = ',';break;/* [<] */
#endif
		default:
			key = *(unsigned char*)buf;
			if (key >= 'A' && key <= 'Z') {
				key = key + ('a' - 'A');
			}
			break;
	}

	return key;
}


static void
GetEvent(void)
{
	XEvent x_event;
	int but;

	if (!dpy) return;

	XNextEvent(dpy, &x_event);

	switch (x_event.type) {
	case KeyPress:
	case KeyRelease:
		Key_Event(XLateKey(&x_event.xkey), x_event.type == KeyPress);
		break;

	case ButtonPress:
		but = x_event.xbutton.button;
		if (but == 2) but = 3;
		else if (but == 3) but = 2;
		switch(but) {
		case 1:
		case 2:
		case 3:
			Key_Event(K_MOUSE1 + but - 1, true);
		}
		break;

	case ButtonRelease:
		but = x_event.xbutton.button;
		if (but == 2) but = 3;
		else if (but == 3) but = 2;
		switch(but) {
		case 1:
		case 2:
		case 3:
			Key_Event(K_MOUSE1 + but - 1, false);
		}
		break;

	case MotionNotify:
		if (dgamouse) {
			mouse_x += (float)x_event.xmotion.x_root
				* vid_dga_mouseaccel.value;
			mouse_y += (float)x_event.xmotion.y_root
				* vid_dga_mouseaccel.value;
		} else
		{
			if (_windowed_mouse.value) {
				mouse_x += (float) ((int)x_event.xmotion.x
						    - (int)(vid.width/2));
				mouse_y += (float) ((int)x_event.xmotion.y
						    - (int)(vid.height/2));

				/* move the mouse to the window center again */
				XSelectInput(dpy, win, X_MASK & ~PointerMotionMask);
				XWarpPointer(dpy, None, win, 0, 0, 0, 0,
					(vid.width/2), (vid.height/2));
				XSelectInput(dpy, win, X_MASK);
			}
		}
		break;

	case ConfigureNotify:
		if (scr_width == x_event.xconfigure.width &&
		    scr_height == x_event.xconfigure.height) {
			break;
		}
		scr_width = x_event.xconfigure.width;
		scr_height = x_event.xconfigure.height;

		if (scr_width < 320) scr_width = 320;
		if (scr_height < 200) scr_height = 200;

		scr_width &= ~7; /* make it a multiple of eight */

		XResizeWindow(dpy, win, scr_width, scr_height);

		vid.width = vid.conwidth = scr_width;
		vid.height = scr_height;

		/* pick a conheight that matches with correct aspect */
		vid.conheight = vid.conwidth*3 / 4;

		vid.aspect = ((float)vid.height / (float)vid.width)
			* (320.0 / 240.0);

		vid.recalc_refdef = 1;	/* force a surface cache flush */
		Con_CheckResize();
		Con_Clear_f();
		break;
	}

	if (mouse_shouldgrab != mouse_grabbed) {
		do_grabs(mouse_shouldgrab);
	}
	if (vid_glx_fullscreen.value != fullscreen) {
		do_fullscreen(vid_glx_fullscreen.value);
	}
}




void Sys_SendKeyEvents(void)
{
	if (dpy) {
		while (XPending(dpy))
			GetEvent();
	}
}

void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

void IN_Init(void)
{
}

void IN_Shutdown(void)
{
	if (dpy) {
		do_grabs(0);
		do_fullscreen(0);
	}
}

/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
}


/*
===========
IN_Move
===========
*/
void
IN_MouseMove(usercmd_t *cmd)
{
	if (m_filter.value) {
		mouse_x = (mouse_x + old_mouse_x) * 0.5;
		mouse_y = (mouse_y + old_mouse_y) * 0.5;
	}
	old_mouse_x = mouse_x;
	old_mouse_y = mouse_y;

	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;

	/* add mouse X/Y movement to cmd */
	if ((in_strafe.state & 1) || (lookstrafe.value && mlook_active))    // Baker 3.60 - Freelook cvar support
	{
		cmd->sidemove += m_side.value * mouse_x;
	} else {
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;
	}

	if (mlook_active)	V_StopPitchDrift();    // Baker 3.60 - Freelook cvar support

	if (mlook_active && !(in_strafe.state & 1))     // Baker 3.60 - Freelook cvar support
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;

		// JPG 1.05 - added pq_fullpitch
		if (pq_fullpitch.value)
		{
			if (cl.viewangles[PITCH] > 90)
				cl.viewangles[PITCH] = 90;
			if (cl.viewangles[PITCH] < -90)
				cl.viewangles[PITCH] = -90;
		}
		else
		{
			if (cl.viewangles[PITCH] > 80)
				cl.viewangles[PITCH] = 80;
			if (cl.viewangles[PITCH] < -70)
				cl.viewangles[PITCH] = -70;
		}
	} else {
		if ((in_strafe.state & 1) && noclip_anglehack) {
			cmd->upmove -= m_forward.value * mouse_y;
		} else {
			cmd->forwardmove -= m_forward.value * mouse_y;
		}
	}
	mouse_x = mouse_y = 0.0;
}


void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove(cmd);
}

void VID_ExtraOptionDraw(unsigned int options_draw_cursor)
{
	/* Windowed Mouse */
        M_Print(16, options_draw_cursor+=8, "             Use Mouse");
        M_DrawCheckbox(220, options_draw_cursor, _windowed_mouse.value);

//#if defined(XMESA) || defined(HAS_DGA)
	if (hasdga || QF_XMesaSetFXmode) {
		/* Mesa has a fullscreen / windowed glx hack. */
		M_Print(16, options_draw_cursor+=8, "            Fullscreen");
		M_DrawCheckbox(220, options_draw_cursor,
			       vid_glx_fullscreen.value);
	}
//#endif

}

void VID_ExtraOptionCmd(int option_cursor)
{
	switch(option_cursor) {
	case 1:	// _windowed_mouse
		Cvar_SetValue("_windowed_mouse", !_windowed_mouse.value);
		break;

//#if defined(XMESA) || defined(HAS_DGA)
	case 2:
		if (hasdga || QF_XMesaSetFXmode) {
			Cvar_SetValue("vid_glx_fullscreen",
				      !vid_glx_fullscreen.value);
		}
		break;
//#endif
	}
}
