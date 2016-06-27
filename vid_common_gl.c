/*
Copyright (C) 2002-2003 A Nourai

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
// vid_common_gl.c -- Common code for vid_wgl.c and vid_glx.c

#include "quakedef.h"
const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;
qboolean gl_mtexable = false;

#ifdef MACOSX
// Baker: in windows this is in gl_vidnt.c
int	texture_mode = GL_LINEAR;
#endif // Move to vid_glcommon.c  This does not belong here.
//#endif // Commenting this out to generate compiler error if I do not fix location.

float		gldepthmin, gldepthmax;


PROC glArrayElementEXT;
PROC glColorPointerEXT;
PROC glTexCoordPointerEXT;
PROC glVertexPointerEXT;

float vid_gamma = 1.0;
byte		vid_gamma_table[256];

unsigned	d_8to24table[256];
unsigned	d_8to24table2[256];
//unsigned short	d_8to16table[256]; //johnfitz -- never used




#if !defined(DX8QUAKE_NO_8BIT)
unsigned char d_15to8table[65536];
#endif




#define TEXTURE_EXT_STRING "GL_EXT_texture_object"


qboolean isPermedia = false;

byte		color_white[4] = {255, 255, 255, 0};
byte		color_black[4] = {0, 0, 0, 0};
extern qboolean	fullsbardraw;
qboolean CheckExtension (const char *extension)
{
	char		*where, *terminator;
	const	char	*start;

	if (!gl_extensions && !(gl_extensions = glGetString(GL_EXTENSIONS)))
		return false;

	if (!extension || *extension == 0 || strchr(extension, ' '))
		return false;

	for (start = gl_extensions ; where = strstr(start, extension) ; start = terminator)
	{
		terminator = where + strlen(extension);
		if ((where == start || *(where - 1) == ' ') && (*terminator == 0 || *terminator == ' '))
			return true;
	}

	return false;
}

void CheckTextureExtensions (void)
{

#if !defined(DX8QUAKE)
	char		*tmp;
	qboolean	texture_ext;
	HINSTANCE	hInstGL;

	texture_ext = FALSE;
	/* check for texture extension */
	tmp = (unsigned char *)glGetString(GL_EXTENSIONS);
	while (*tmp)
	{
		if (strncmp((const char*)tmp, TEXTURE_EXT_STRING, strlen(TEXTURE_EXT_STRING)) == 0)
			texture_ext = TRUE;
		tmp++;
	}

	if (!texture_ext || COM_CheckParm ("-gl11") )
	{
		hInstGL = LoadLibrary("opengl32.dll");

		if (hInstGL == NULL)
			Sys_Error ("Couldn't load opengl32.dll\n");

		bindTexFunc = (void *)GetProcAddress(hInstGL,"glBindTexture");

		if (!bindTexFunc)
			Sys_Error ("No texture objects!");
		return;
	}

/* load library and get procedure adresses for texture extension API */
	if ((bindTexFunc = (BINDTEXFUNCPTR)
		wglGetProcAddress((LPCSTR) "glBindTextureEXT")) == NULL)
	{
		Sys_Error ("GetProcAddress for BindTextureEXT failed");
		return;
	}

#endif
}

/*
===============
CheckArrayExtensions
===============
*/
void CheckArrayExtensions (void)
{
#if !defined(DX8QUAKE)

	char		*tmp;

	// check for texture extension
	tmp = (unsigned char *)glGetString(GL_EXTENSIONS);
	while (*tmp) {
		if (strncmp((const char*)tmp, "GL_EXT_vertex_array", strlen("GL_EXT_vertex_array")) == 0) {
			if (
((glArrayElementEXT = wglGetProcAddress("glArrayElementEXT")) == NULL) ||
((glColorPointerEXT = wglGetProcAddress("glColorPointerEXT")) == NULL) ||
((glTexCoordPointerEXT = wglGetProcAddress("glTexCoordPointerEXT")) == NULL) ||
((glVertexPointerEXT = wglGetProcAddress("glVertexPointerEXT")) == NULL) )
			{
				Sys_Error ("GetProcAddress for vertex extension failed");
				return;
			}
			return;
		}
		tmp++;
	}

	Sys_Error ("Vertex array extension not present");

#endif
}

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

#ifndef OLD_SGIS

void CheckMultiTextureExtensions(void)
{
	qboolean SGIS, ARB;

	if (COM_CheckParm("-nomtex"))
	{
		Con_Warning ("Multitexture disabled at command line\n");
		return;
	}

	if (COM_CheckParm("-nomtexarb"))
		ARB = false;
	else
		ARB = strstr (gl_extensions, "GL_ARB_multitexture ") != NULL;

	SGIS = strstr (gl_extensions, "GL_SGIS_multitexture ") != NULL;

	if (!ARB && !SGIS) {
		Con_Warning ("Multitexture extension not found\n");
		return;
	}

	qglMTexCoord2fSGIS = (void *) wglGetProcAddress (ARB ? "glMultiTexCoord2fARB" : "glMTexCoord2fSGIS");
	qglSelectTextureSGIS = (void *) wglGetProcAddress (ARB ? "glActiveTextureARB" : "glSelectTextureSGIS");
	TEXTURE0_SGIS = ARB ? 0x84C0 : 0x835E;
	TEXTURE1_SGIS = ARB ? 0x84C1 : 0x835F;
	gl_oldtarget = TEXTURE0_SGIS;

	Con_Printf ("GL_%s_multitexture extensions found\n", ARB ? "ARB" : "SGIS");
	gl_mtexable = true;
}
#else

void CheckMultiTextureExtensions(void) {
   if (COM_CheckParm("-nomtex"))
   {
      Con_Warning ("Multitexture disabled at command line\n");
      return;
   }

	if (!strstr(gl_extensions, "GL_SGIS_multitexture ")) {
		Con_Warning ("Multitexture extension not found\n");
		return;
	}

	qglMTexCoord2fSGIS = (void *) wglGetProcAddress("glMTexCoord2fSGIS");
	qglSelectTextureSGIS = (void *) wglGetProcAddress("glSelectTextureSGIS");

	Con_Printf("Multitexture extensions found\n");
	gl_mtexable = true;
}

#endif




void GL_SetupState (void);

/*
===============
GL_Init
===============
*/
qboolean IntelDisplayAdapter = false;
void GL_Init (void) {
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);

	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);

	if (strncasecmp(gl_renderer, "Intel", 5) == 0) {
		extern cvar_t gl_ztrick;
		Con_Printf ("Intel Display Adapter detected\n");
		IntelDisplayAdapter = true;
		Cvar_SetValueByRef  (&gl_ztrick, 0);
		Cvar_SetValueByRef  (&gl_clear, 1);
//		Con_Printf ("values are %s and %s", gl_ztrick.value, gl_clear.value);
	}


    if (strncasecmp(gl_renderer,"PowerVR",7)==0)
         fullsbardraw = true;

    if (strncasecmp(gl_renderer,"Permedia",8)==0)
         isPermedia = true;

	CheckTextureExtensions ();
	CheckMultiTextureExtensions ();

	GL_SetupState (); //johnfitz
}

void GL_PrintExtensions_f(void) {
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);
}


/*
===============
GL_SetupState -- johnfitz

does all the stuff from GL_Init that needs to be done every time a new GL render context is created
GL_Init will still do the stuff that only needs to be done once
===============
*/
void GL_SetupState (void) {
	glClearColor (0.15,0.15,0.15,0);  // Baker 3.60 - set to same clear color as FitzQuake for void areas
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);
	// Get rid of Z-fighting for textures by offsetting the
	// drawing of entity models compared to normal polygons.
	// (Only works if gl_ztrick is turned off)
#if !defined(D3DQ_CANNOT_DO_THIS)
	// Baker: d3dquake no support this
	glPolygonOffset(0.05, 0);
#endif
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void	VID_SetPaletteOld (unsigned char *palette)
{
	byte	*pal;
	unsigned r,g,b;
	unsigned v;
	int     r1,g1,b1;
	int		j,k,l;
	unsigned short i;
	unsigned	*table;

//
// 8 8 8 encoding
//
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

	d_8to24table[255] = 0;	// 255 is transparent "MH: says this fixes pink edges"
	//d_8to24table[255] &= 0xffffff;	// 255 is transparent

#if !defined(DX8QUAKE_NO_8BIT)

	// JACK: 3D distance calcs - k is last closest, l is the distance.
	// FIXME: Precalculate this and cache to disk.
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
		for (v=0,k=0,l=10000*10000; v<256; v++,pal+=4) {
			r1 = r-pal[0];
			g1 = g-pal[1];
			b1 = b-pal[2];
			j = (r1*r1)+(g1*g1)+(b1*b1);
			if (j<l) {
				k=v;
				l=j;
			}
		}
		d_15to8table[i]=k;
	}

#endif
}

BOOL	gammaworks;
void	VID_ShiftPaletteOld (unsigned char *palette)
{
	extern	byte rampsold[3][256];
//	VID_SetPaletteOld (palette);
//	gammaworks = SetDeviceGammaRamp (maindc, ramps);
}
BINDTEXFUNCPTR bindTexFunc;
void VID_SetPalette (unsigned char *palette) {
	byte		*pal;
	int		i;
	unsigned	r, g, b, *table;
// 8 8 8 encoding
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++) {
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		*table++ = (255<<24) + (r<<0) + (g<<8) + (b<<16);
	}
	d_8to24table[255] = 0;	// 255 is transparent
// Tonik: create a brighter palette for bmodel textures
	pal = palette;
	table = d_8to24table2;
	for (i=0 ; i<256 ; i++) {
		r = QMIN(pal[0] * (2.0 / 1.5), 255);
		g = QMIN(pal[1] * (2.0 / 1.5), 255);
		b = QMIN(pal[2] * (2.0 / 1.5), 255);
		pal += 3;
		*table++ = (255<<24) + (r<<0) + (g<<8) + (b<<16);
	}
	d_8to24table2[255] = 0;	// 255 is transparent
}

void Check_Gamma (unsigned char *pal) {
	int		i;
	float		inf;
	unsigned char	palette[768];
	if ((i = COM_CheckParm("-gamma")) != 0 && i+1 < com_argc)
		vid_gamma = bound(0.3, atof(com_argv[i+1]), 1);
	else
		vid_gamma = 1;

	Cvar_SetValueByRef (&v_gamma, vid_gamma);
	if (vid_gamma != 1) {
		for (i=0 ; i<256 ; i++) {
			inf = QMIN(255 * powf((i + 0.5) / 255.5, vid_gamma) + 0.5, 255);
			vid_gamma_table[i] = inf;
		}
	} else {
		for (i=0 ; i<256 ; i++)
			vid_gamma_table[i] = i;
	}
	for (i=0 ; i<768 ; i++)
		palette[i] = vid_gamma_table[pal[i]];
	memcpy (pal, palette, sizeof(palette));
}

void Check_GammaOld (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		i;
	if ((i = COM_CheckParm("-gamma")) == 0) {
		if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
			(gl_vendor && strstr(gl_vendor, "3Dfx")))
			vid_gamma = 1;
		else
			vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
	} else {
		vid_gamma = atof(com_argv[i+1]);
		if (vid_gamma == 0) {
			// Baker: Then someone used -gamma parameter incorrectly so use the default
			vid_gamma = 0.7;
		} else {
			vid_gamma = bound(0.3, vid_gamma, 1); //Baker 3.99: place min on vid_gamma value to avoid white screen
		}
	}
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
