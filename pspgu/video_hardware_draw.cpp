/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2007 Peter Mackay and Chris Swindle.

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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include <valarray>
#include <vector>
#include <malloc.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <pspctrl.h>

extern "C"
{
#include "../quakedef.h"
}

#include "vram.hpp"

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;

int			translate_texture;
int			char_texture;

bool 	 	tex_scale_down = true;

typedef byte texel;

typedef struct
{
	// Source.
	char	identifier[64];
	int		original_width;
	int		original_height;
	bool	stretch_to_power_of_two;

	// Texture description.
	int		format;
	int		filter;
	int		width;
	int		height;
	int 	mipmaps;

	ScePspRGBA8888*	palette;

	// Buffers.
	texel*	ram;
	texel*	vram;



} gltexture_t;

byte		conback_buffer[sizeof(qpic_t) + sizeof(glpic_t)];
qpic_t		*conback = (qpic_t *)&conback_buffer;

#define	MAX_GLTEXTURES	1024
gltexture_t	gltextures[MAX_GLTEXTURES];
bool 		gltextures_used[MAX_GLTEXTURES];
int			numgltextures;

void GL_InitTextureUsage () {
	for (int i=0; i<MAX_GLTEXTURES; i++) {
		gltextures_used[i] = false;
	}
	numgltextures = 0;
}

void GL_Bind (int texture_index)
{

	// Which texture is it?
	const gltexture_t& texture = gltextures[texture_index];

	if (texture.palette)
		VID_SetPaletteToTexture (texture.palette);
	else
		VID_SetGlobalPalette ();

	// Binding the currently bound texture?
	if (currenttexture == texture_index)
	{
		// Don't bother.
		return;
	}

	// Remember the current texture.
	currenttexture = texture_index;

	// Set the texture mode.
	if (texture.palette)
		sceGuTexMode(GU_PSM_T8, texture.mipmaps , 0, GU_TRUE);
	else
		sceGuTexMode(texture.format, texture.mipmaps , 0, GU_TRUE);

	if (texture.mipmaps > 0 && r_mipmaps.value > 0)
    {
		float slope = 0.4f;
		sceGuTexSlope(slope); // the near from 0 slope is the lower (=best detailed) mipmap it uses
        sceGuTexFilter(GU_LINEAR_MIPMAP_LINEAR, GU_LINEAR_MIPMAP_LINEAR);
		sceGuTexLevelMode(int(r_mipmaps_func.value), r_mipmaps_bias.value);
	}
	else
		sceGuTexFilter(texture.filter, texture.filter);

	// Set the texture image.
	const void* const texture_memory = texture.vram ? texture.vram : texture.ram;
	sceGuTexImage(0, texture.width, texture.height, texture.width, texture_memory);

	if (texture.mipmaps > 0 && r_mipmaps.value > 0) {
		int size = (texture.width * texture.height);
		int offset = size;
		int div = 2;

		for (int i = 1; i <= texture.mipmaps; i++) {
			void* const texture_memory2 = ((byte*) texture_memory)+offset;
			sceGuTexImage(i, texture.width/div, texture.height/div, texture.width/div, texture_memory2);
			offset += size/(div*div);
			div *=2;
		}
	}
}


void GL_BindLightmap (int texture_index)
{
	// Binding the currently bound texture?
	if (currenttexture == texture_index)
	{
		// Don't bother.
		return;
	}

	// Remember the current texture.
	currenttexture = texture_index;

	// Which texture is it?
	const gltexture_t& texture = gltextures[texture_index];

	// Set the texture mode.
	sceGuTexMode(texture.format, 0, 0, GU_FALSE);
	sceGuTexFilter(texture.filter, texture.filter);

	// Set the texture image.
	const void* const texture_memory = texture.vram ? texture.vram : texture.ram;
	sceGuTexImage(0, texture.width, texture.height, texture.width, texture_memory);
}


//=============================================================================
/* Support Routines */

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

byte		menuplyr_pixels[4096];

static int GL_LoadPicTexture (qpic_t *pic)
{
	return GL_LoadTexture ("", pic->width, pic->height, pic->data, qfalse, GU_NEAREST, 0);
}

qpic_t *Draw_PicFromWad (char *name)
{
	qpic_t	*p;
	glpic_t	*gl;

	p = static_cast<qpic_t*>(W_GetLumpName (name));
	gl = (glpic_t *)p->data;
	gl->index = GL_LoadPicTexture (p);

	return p;
}


/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	glpic_t		*gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	strcpy (pic->name, path);

//
// load the pic from disk
//
	dat = (qpic_t *)COM_LoadTempFile (path);
	if (!dat)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	SwapPic (dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp (path, "gfx/menuplyr.lmp"))
		memcpy (menuplyr_pixels, dat->data, dat->width*dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	gl = (glpic_t *)pic->pic.data;
	gl->index = GL_LoadPicTexture (dat);

	return &pic->pic;
}


static void Draw_CharToConback (int num, byte *dest)
{
	int		row, col, drawline, x;
	byte	*source;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}

}

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int		i;
	qpic_t	*cb;
	byte	*dest;
	int		x, y;
	char	ver[40];
	glpic_t	*gl;
	int		start;
	byte	*ncdata;

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
	draw_chars = static_cast<byte*>(W_GetLumpName ("conchars"));
	for (i=0 ; i<256*64 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	// now turn them into textures
	char_texture = GL_LoadTexture ("charset", 128, 128, draw_chars, qfalse, GU_NEAREST, 0);

	start = Hunk_LowMark();

	cb = (qpic_t *)COM_LoadTempFile ("gfx/conback.lmp");
	if (!cb)
		Sys_Error ("Couldn't load gfx/conback.lmp");
	SwapPic (cb);

	// hack the version number directly into the pic
	if(!kurok)
	{
	   	snprintf(ver, sizeof(ver),  "(ProQuake) %4.2f", PROQUAKE_SERIES_VERSION); // JPG - obvious change
	   	dest = cb->data + 320*186 + 320 - 11 - 8*strlen(ver);
		y = strlen(ver);
		for (x=0 ; x<y ; x++)
			Draw_CharToConback (ver[x], dest+(x<<3));
	}

	conback->width = cb->width;
	conback->height = cb->height;
	ncdata = cb->data;

	gl = (glpic_t *)conback->data;
	if (COM_CheckParm("-nearest")){
	gl->index = GL_LoadTexture ("conback", conback->width, conback->height, ncdata, qfalse, GU_NEAREST, 0);
	}
	else{
	gl->index = GL_LoadTexture ("conback", conback->width, conback->height, ncdata, qfalse, GU_LINEAR, 0);
	}
	conback->width = vid.width;
	conback->height = vid.height;

	// free loaded console
	Hunk_FreeToLowMark(start);

	// save a texture slot for translated picture
	// TODO Handle translating.
	/*translate_texture = texture_extension_number++;*/

#if 0
	// save slots for scraps
	scrap_texnum = texture_extension_number;
	texture_extension_number += MAX_SCRAPS;
#endif

	//
	// get the other pics we need
	//
	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad ("backtile");
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, int num)
{
	int	row, col;

	if (num == 32)
		return;		// space

	num &= 255;

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	GL_Bind (char_texture);

	struct vertex
	{
		short u, v;
		short x, y, z;
	};

	vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].u = col * 8;
	vertices[0].v = row * 8;
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0;

	vertices[1].u = (col + 1) * 8;
	vertices[1].v = (row + 1) * 8;
	vertices[1].x = x + 8;
	vertices[1].y = y + 8;
	vertices[1].z = 0;

	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

/*
=============
Draw_AlphaPicConsole
=============
*/
static void Draw_AlphaPicConsole (int x, int y, qpic_t *pic, float alpha)
{
	if (alpha != 1.0f)
	{
		sceGuTexFunc(GU_TFX_DECAL, GU_TCC_RGBA);
	}

	glpic_t			*gl;

#if 0
	if (scrap_dirty)
		Scrap_Upload ();
#endif
	gl = (glpic_t *)pic->data;
	GL_Bind (gl->index);

	struct vertex
	{
		short			u, v;
		short			x, y, z;
	};

	vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].u		= 0;
	vertices[0].v		= 0;
	vertices[0].x		= x;
	vertices[0].y		= y;
	vertices[0].z		= 0;

	const gltexture_t& glt = gltextures[gl->index];
	vertices[1].u		= glt.original_width;
	vertices[1].v		= glt.original_height;
	vertices[1].x		= x + pic->width;
	vertices[1].y		= y + pic->height;
	vertices[1].z		= 0;

	sceGuColor(GU_RGBA(0xff, 0xff, 0xff, static_cast<unsigned int>(alpha * 255.0f)));
	sceGuDrawArray(
		GU_SPRITES,
		GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
		2, 0, vertices);

	if (alpha != 1.0f)
	{
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	}
}



/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, qpic_t *pic, float alpha)
{
	if (alpha != 1.0f)
	{
		sceGuTexFunc(GU_TFX_DECAL, GU_TCC_RGBA);
	}

	glpic_t			*gl;

#if 0
	if (scrap_dirty)
		Scrap_Upload ();
#endif
	gl = (glpic_t *)pic->data;
	GL_Bind (gl->index);

	struct vertex
	{
		short			u, v;
		short			x, y, z;
	};

    vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].u		= 0;
    vertices[0].v		= 0;
    vertices[0].x		= x;
    vertices[0].y		= y;
    vertices[0].z		= 0;

	const gltexture_t& glt = gltextures[gl->index];
    vertices[1].u		= glt.original_width;
    vertices[1].v		= glt.original_height;
    vertices[1].x		= x + pic->width;
    vertices[1].y		= y + pic->height;
    vertices[1].z		= 0;

    sceGuColor(GU_RGBA(0xff, 0xff, 0xff, static_cast<unsigned int>(alpha * 255.0f)));
    sceGuDrawArray(
		GU_SPRITES,
		GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
		2, 0, vertices);

	if (alpha != 1.0f)
	{
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	}
}

void Draw_Crosshair(void)
{
    extern cvar_t crosshair;
    extern cvar_t cl_crossx;
    extern cvar_t cl_crossy;

	extern cvar_t scr_fov;

    extern cvar_t in_x_axis_adjust;
    extern cvar_t in_y_axis_adjust;
    extern cvar_t in_freelook_analog;
    extern cvar_t in_disable_analog;
    extern cvar_t in_analog_strafe;

	qpic_t	*pic;

	pic = 0;


	if (crosshair.value == 1 && !chase_active.value)
//     Draw_Character (scr_vrect.x + scr_vrect.width/2 - 2, scr_vrect.y + scr_vrect.height/2 - 4, '+');
		Draw_Character ((vid.width - 8)/2, (vid.height - 8)/2, '+');
	else if (crosshair.value == 2 && !chase_active.value)
		Draw_Character ((vid.width - 8)/2, (vid.height - 8)/2, '.');
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	Draw_AlphaPic(x, y, pic, 1.0f);
}

/*
=============
Draw_Pic
=============
*/
void Draw_PicConsole (int x, int y, qpic_t *pic)
{
	Draw_AlphaPicConsole(x, y, pic, 1.0f);
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}

	Draw_Pic (x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{
/*
	int				v, u, c;
	unsigned		trans[64*64], *dest;
	byte			*src;
	int				p;

	GL_Bind (translate_texture);

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] =  d_8to24table[translation[p]];
		}
	}
	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glColor3f (1,1,1);
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex2f (x, y);
	glTexCoord2f (1, 0);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (1, 1);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (0, 1);
	glVertex2f (x, y+pic->height);
	glEnd ();*/

	struct vertex
	{
		short u, v;
		short x, y, z;
	};

	vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].u = 0;
	vertices[0].v = 0;
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0;

	vertices[1].u = 1;
	vertices[1].v = 1;
	vertices[1].x = x + pic->width;
	vertices[1].y = y + pic->height;
	vertices[1].z = 0;

	sceGuDisable(GU_TEXTURE_2D);
	sceGuColor(GU_RGBA(0xff, 0, 0, 0xff));
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
	sceGuColor(0xffffffff);
	sceGuEnable(GU_TEXTURE_2D);
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	int y = (vid.height * 3) >> 2;

	if (lines > y)
		Draw_PicConsole(0, lines - vid.height, conback);
	else
		Draw_AlphaPicConsole (0, lines - vid.height, conback, (float)(1.2 * lines)/y);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	GL_Bind (*(int *)draw_backtile->data);

	struct vertex
	{
		short u, v;
		short x, y, z;
	};

	vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].u = x;
	vertices[0].v = y;
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0;

	vertices[1].u = x + w;
	vertices[1].v = y + h;
	vertices[1].x = x + w;
	vertices[1].y = y + h;
	vertices[1].z = 0;

	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	struct vertex
	{
		short x, y, z;
	};

	vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0;

	vertices[1].x = x + w;
	vertices[1].y = y + h;
	vertices[1].z = 0;

	sceGuDisable(GU_TEXTURE_2D);
	sceGuColor(GU_RGBA(host_basepal[c*3], host_basepal[c*3+1], host_basepal[c*3+2], 0xff));
	sceGuDrawArray(GU_SPRITES, GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
	sceGuColor(0xffffffff);
	sceGuEnable(GU_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	struct vertex
	{
		short	x, y, z;
	};

	vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].x		= 0;
	vertices[0].y		= 0;
	vertices[0].z		= 0;
	vertices[1].x		= vid.width;
	vertices[1].y		= vid.height;
	vertices[1].z		= 0;

	sceGuDisable(GU_TEXTURE_2D);

	sceGuColor(GU_RGBA(0, 0, 0, 0x40));
	sceGuDrawArray(
		GU_SPRITES,
		GU_VERTEX_16BIT | GU_TRANSFORM_2D,
		2, 0, vertices);

	sceGuEnable(GU_TEXTURE_2D);

	Sbar_Changed();
}

#ifdef SUPPORTS_KUROK
/*
================
Draw_FadeScreen2

================
*/
void Draw_FadeScreen2 (void)
{
    extern cvar_t r_menufade;

	Draw_AlphaPic (0, 0, conback, r_menufade.value);
}
#endif

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreenColor (float r, float g, float b, float a)
{
	struct vertex
	{
		short	x, y, z;
	};

	vertex* const vertices = static_cast<vertex*>(sceGuGetMemory(sizeof(vertex) * 2));

	vertices[0].x		= 0;
	vertices[0].y		= 0;
	vertices[0].z		= 0;
	vertices[1].x		= vid.width;
	vertices[1].y		= vid.height;
	vertices[1].z		= 0;

	sceGuDisable(GU_TEXTURE_2D);

    if (r > 1)
        r = 1;
    if (g > 1)
        g = 1;
    if (b > 1)
        b = 1;
    if (a > 1)
        a = 1;

	sceGuColor(GU_COLOR(r, g, b, a * 1.0f));
	sceGuDrawArray(
		GU_SPRITES,
		GU_VERTEX_16BIT | GU_TRANSFORM_2D,
		2, 0, vertices);

    sceGuColor(0xffffffff);

	sceGuEnable(GU_TEXTURE_2D);

	Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	if (!draw_disc)
		return;

	//glDrawBuffer  (GL_FRONT);
	Draw_Pic (vid.width - 24, 0, draw_disc);
	//glDrawBuffer  (GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D (void)
{
	sceGuViewport (glx, gly, glwidth, glheight);
	sceGuScissor(0, 0, glwidth, glheight);

	sceGuEnable(GU_BLEND);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
}

static void GL_ResampleTexture(const byte *in, int inwidth, int inheight, unsigned char *out,  int outwidth, int outheight)
{
	const unsigned int fracstep = inwidth * 0x10000 / outwidth;
	for (int i = 0; i < outheight ; ++i, out += outwidth)
	{
		const byte*		inrow	= in + inwidth * (i * inheight / outheight);
		unsigned int	frac	= fracstep >> 1;
		for (int j = 0; j < outwidth; ++j, frac += fracstep)
		{
			out[j] = inrow[frac >> 16];
		}
	}
}

static void swizzle_fast(u8* out, const u8* in, unsigned int width, unsigned int height)
{
	unsigned int blockx, blocky;
	unsigned int j;

	unsigned int width_blocks = (width / 16);
	unsigned int height_blocks = (height / 8);

	unsigned int src_pitch = (width-16)/4;
	unsigned int src_row = width * 8;

	const u8* ysrc = in;
	u32* dst = (u32*)out;

	for (blocky = 0; blocky < height_blocks; ++blocky)
	{
		const u8* xsrc = ysrc;
		for (blockx = 0; blockx < width_blocks; ++blockx)
		{
			const u32* src = (u32*)xsrc;
			for (j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}

void GL_Upload8(int texture_index, const byte *data, int width, int height)
{
	if ((texture_index < 0) || (texture_index >= MAX_GLTEXTURES) || gltextures_used[texture_index] == false)
	{
		Sys_Error("Invalid texture index %d", texture_index);
	}

	const gltexture_t& texture = gltextures[texture_index];

	// Check that the texture matches.
	if ((width != texture.original_width) != (height != texture.original_height))
	{
		Sys_Error("Attempting to upload a texture which doesn't match the destination");
	}

	// Create a temporary buffer to use as a source for swizzling.
	std::size_t buffer_size = texture.width * texture.height;
	std::vector<byte> unswizzled(buffer_size);

	if (texture.mipmaps > 0) {
		int size_incr = buffer_size/4;
		for (int i= 1;i <= texture.mipmaps;i++) {
			buffer_size += size_incr;
			size_incr = size_incr/4;
		}
	}

	// Do we need to resize?
	if (texture.stretch_to_power_of_two)
	{
		// Resize.
		GL_ResampleTexture(data, width, height, &unswizzled[0], texture.width, texture.height);
	}
	else
	{
		// Straight copy.
		for (int y = 0; y < height; ++y)
		{
			const byte* const	src	= data + (y * width);
			byte* const			dst = &unswizzled[y * texture.width];
			memcpy(dst, src, width);
		}
	}

	// Swizzle to system RAM.
	swizzle_fast(texture.ram, &unswizzled[0], texture.width, texture.height);

	if (texture.mipmaps > 0) {
		int size = (texture.width * texture.height);
		int offset = size;
		int div = 2;

		for (int i = 1; i <= texture.mipmaps;i++) {
			GL_ResampleTexture(data, width, height, &unswizzled[0], texture.width/div, texture.height/div);
			swizzle_fast(texture.ram+offset, &unswizzled[0], texture.width/div, texture.height/div);
			offset += size/(div*div);
			div *=2;
		}
	}

	unswizzled.clear();

	// Copy to VRAM?
	if (texture.vram)
	{
		// Copy.
		memcpy(texture.vram, texture.ram, buffer_size);

		// Flush the data cache.
		sceKernelDcacheWritebackRange(texture.vram, buffer_size);
	}

	// Flush the data cache.
	sceKernelDcacheWritebackRange(texture.ram, buffer_size);
}

void GL_Upload8_A(int texture_index, const byte *data, int width, int height)
{
	if ((texture_index < 0) || (texture_index >= MAX_GLTEXTURES) || gltextures_used[texture_index] == false)
	{
		Sys_Error("Invalid texture index %d", texture_index);
	}

	const gltexture_t& texture = gltextures[texture_index];

	// Check that the texture matches.
	if ((width != texture.original_width) != (height != texture.original_height))
	{
		Sys_Error("Attempting to upload a texture which doesn't match the destination");
	}

	// Create a temporary buffer to use as a source for swizzling.
	const std::size_t buffer_size = texture.width * texture.height;
	memcpy((void *) texture.ram, (void *) data, buffer_size);

	// Copy to VRAM?
	if (texture.vram)
	{
		// Copy.
		memcpy(texture.vram, texture.ram, buffer_size);

		// Flush the data cache.
		sceKernelDcacheWritebackRange(texture.vram, buffer_size);
	}

	// Flush the data cache.
	sceKernelDcacheWritebackRange(texture.ram, buffer_size);
}

void GL_Upload16(int texture_index, const byte *data, int width, int height)
{
	if ((texture_index < 0) || (texture_index >= MAX_GLTEXTURES) || gltextures_used[texture_index] == false)
	{
		Sys_Error("Invalid texture index %d", texture_index);
	}

	const gltexture_t& texture = gltextures[texture_index];

	// Check that the texture matches.
	if ((width != texture.original_width) != (height != texture.original_height))
	{
		Sys_Error("Attempting to upload a texture which doesn't match the destination");
	}

	// Create a temporary buffer to use as a source for swizzling.
	const std::size_t buffer_size = texture.width * texture.height * 2;
	memcpy((void *) texture.ram, (void *) data, buffer_size);


	// Copy to VRAM?
	if (texture.vram)
	{
		// Copy.
		memcpy(texture.vram, texture.ram, buffer_size);

		// Flush the data cache.
		sceKernelDcacheWritebackRange(texture.vram, buffer_size);
	}

	// Flush the data cache.
	sceKernelDcacheWritebackRange(texture.ram, buffer_size);
}

void GL_Upload24(int texture_index, const byte *data, int width, int height)
{
	if ((texture_index < 0) || (texture_index >= MAX_GLTEXTURES) || gltextures_used[texture_index] == false)
	{
		Sys_Error("Invalid texture index %d", texture_index);
	}

	const gltexture_t& texture = gltextures[texture_index];

	// Check that the texture matches.
	if ((width != texture.original_width) != (height != texture.original_height))
	{
		Sys_Error("Attempting to upload a texture which doesn't match the destination");
	}

	// Create a temporary buffer to use as a source for swizzling.
	const std::size_t buffer_size = texture.width * texture.height * 3;
	memcpy((void *) texture.ram, (void *) data, buffer_size);


	// Copy to VRAM?
	if (texture.vram)
	{
		// Copy.
		memcpy(texture.vram, texture.ram, buffer_size);

		// Flush the data cache.
		sceKernelDcacheWritebackRange(texture.vram, buffer_size);
	}

	// Flush the data cache.
	sceKernelDcacheWritebackRange(texture.ram, buffer_size);
}

void GL_Upload32(int texture_index, const byte *data, int width, int height)
{
	if ((texture_index < 0) || (texture_index >= MAX_GLTEXTURES) || gltextures_used[texture_index] == false)
	{
		Sys_Error("Invalid texture index %d", texture_index);
	}

	const gltexture_t& texture = gltextures[texture_index];

	// Check that the texture matches.
	if ((width != texture.original_width) != (height != texture.original_height))
	{
		Sys_Error("Attempting to upload a texture which doesn't match the destination");
	}

	// Create a temporary buffer to use as a source for swizzling.

	const std::size_t buffer_size = texture.width * texture.height * 4;
	memcpy((void *) texture.ram, (void *) data, buffer_size);

	// Copy to VRAM?
	if (texture.vram)
	{
		// Copy.
		memcpy(texture.vram, texture.ram, buffer_size);

		// Flush the data cache.
		sceKernelDcacheWritebackRange(texture.vram, buffer_size);
	}

	// Flush the data cache.
	sceKernelDcacheWritebackRange(texture.ram, buffer_size);
}

static std::size_t round_up(std::size_t size)
{
	static const float	denom	= 1.0f / logf(2.0f);
	const float			logged	= logf(size) * denom;
	const float			ceiling	= ceilf(logged);
	return 1 << static_cast<int>(ceiling);
}


static std::size_t round_down(std::size_t size)
{
	static const float	denom	= 1.0f / logf(2.0f);
	const float			logged	= logf(size) * denom;
	const float			floor	= floorf(logged);
	return 1 << static_cast<int>(floor);
}


void GL_UnloadTexture(int texture_index) {

	if (gltextures_used[texture_index] == true)
	{
		gltexture_t& texture = gltextures[texture_index];

		Con_DPrintf("Unloading: %s\n",texture.identifier);
		// Source.
		strcpy(texture.identifier,"");
		texture.original_width = 0;
		texture.original_height = 0;
		texture.stretch_to_power_of_two = qfalse;

		// Texture description.
		texture.format = GU_PSM_T8;
		texture.filter = GU_LINEAR;
		texture.width = 0;
		texture.height = 0;
		texture.mipmaps = 0;
		// Buffers.
		if (texture.ram != NULL)
		{
			free(texture.ram);
			texture.ram = NULL;
		}
		if (texture.vram != NULL)
		{
			quake::vram::free(texture.vram);
			texture.vram = NULL;
		}
	}

	gltextures_used[texture_index] = false;
	numgltextures--;
}

int GL_LoadTexture (const char *identifier, int width, int height, const byte *data, qboolean stretch_to_power_of_two, int filter, int mipmap_level)
{
	int texture_index = -1;

	tex_scale_down = r_tex_scale_down.value == qtrue;
	// See if the texture is already present.
	if (identifier[0])
	{
		for (int i = 0; i < MAX_GLTEXTURES; ++i)
		{
			if (gltextures_used[i] == true)
			{
				const gltexture_t& texture = gltextures[i];
				if (!strcmp (identifier, texture.identifier))
				{
					return i;
				}
			}
		}
	}

	// Out of textures?
	if (numgltextures == MAX_GLTEXTURES)
	{
		Sys_Error("Out of OpenGL textures");
	}

	// Use the next available texture.
	numgltextures++;
	texture_index = numgltextures;

	for (int i = 0; i < MAX_GLTEXTURES; ++i)
	{
		if (gltextures_used[i] == false) {
			texture_index = i;
			break;
		}
	}
	gltexture_t& texture = gltextures[texture_index];
	gltextures_used[texture_index] = true;

	// Fill in the source data.
	strcpy(texture.identifier, identifier);
	texture.original_width			= width;
	texture.original_height			= height;
	texture.stretch_to_power_of_two	= stretch_to_power_of_two != qfalse;

	// Fill in the texture description.
           texture.format = GU_PSM_T8;
	texture.filter			= filter;
	texture.mipmaps			= mipmap_level;

	if (tex_scale_down == true && texture.stretch_to_power_of_two == true) {
		texture.width			= std::max(round_down(width), 32U);
		texture.height			= std::max(round_down(height),32U);
	} else {
		texture.width			= std::max(round_up(width), 32U);
		texture.height			= std::max(round_up(height),32U);
	}

	for (int i=0; i <= mipmap_level;i++) {
		int div = (int) powf(2,i);
		if ((texture.width / div) > 16 && (texture.height / div) > 16 ) {
			texture.mipmaps = i;
		}
	}

	// Do we really need to resize the texture?
	if (texture.stretch_to_power_of_two)
	{
		// Not if the size hasn't changed.
		texture.stretch_to_power_of_two = (texture.width != width) || (texture.height != height);
	}

	Con_DPrintf("Loading: %s [%dx%d](%0.2f KB)\n",texture.identifier,texture.width,texture.height, (float) (texture.width*texture.height)/1024);

	// Allocate the RAM.
	std::size_t buffer_size = texture.width * texture.height;

	if (texture.mipmaps > 0) {
		int size_incr = buffer_size/4;
		for (int i= 1;i <= texture.mipmaps;i++) {
			buffer_size += size_incr;
			size_incr = size_incr/4;
		}
	}

	texture.ram	= static_cast<texel*>(memalign(16, buffer_size));

	if (!texture.ram)
	{
		Sys_Error("Out of RAM for textures.");
	}

	// Allocate the VRAM.
	texture.vram = static_cast<texel*>(quake::vram::allocate(buffer_size));

	// Upload the texture.
	GL_Upload8(texture_index, data, width, height);

	if (texture.vram && texture.ram) {
		free(texture.ram);
		texture.ram = NULL;
	}
	// Done.
	return texture_index;
}

int GL_LoadLightmapTexture (const char *identifier, int width, int height, const byte *data, int bpp, int filter, qboolean update)
{
	tex_scale_down = r_tex_scale_down.value == qtrue;
	int texture_index = -1;
	// See if the texture is already present.
	if (identifier[0])
	{
		for (int i = 0; i < MAX_GLTEXTURES; ++i)
		{
			if (gltextures_used[i] == true)
			{
				const gltexture_t& texture = gltextures[i];
				if (!strcmp (identifier, texture.identifier))
				{
					if (update == qfalse) {
						return i;
					}
					else {
						texture_index = i;
						break;
					}
				}
			}
		}
	}

	if (update == qfalse || texture_index == -1) {
		// Out of textures?
		if (numgltextures == MAX_GLTEXTURES)
		{
			Sys_Error("Out of OpenGL textures");
		}

		// Use the next available texture.
		numgltextures++;
		texture_index = numgltextures;

		for (int i = 0; i < MAX_GLTEXTURES; ++i)
		{
			if (gltextures_used[i] == false) {
				texture_index = i;
				break;
			}
		}
		gltexture_t& texture = gltextures[texture_index];
		gltextures_used[texture_index] = true;

		// Fill in the source data.
		strcpy(texture.identifier, identifier);
		texture.original_width			= width;
		texture.original_height			= height;
		texture.stretch_to_power_of_two	= false;

		// Fill in the texture description.
		if (bpp == 1)
			texture.format		= GU_PSM_T8;
		else if (bpp == 2)
			texture.format		= GU_PSM_4444;
		else if (bpp == 3)
			texture.format		= GU_PSM_8888;
		else if (bpp == 4)
			texture.format		= GU_PSM_8888;

		texture.filter			= filter;
		texture.mipmaps			= 0;

		if (tex_scale_down == true && texture.stretch_to_power_of_two == true) {
			texture.width			= std::max(round_down(width),  16U);
			texture.height			= std::max(round_down(height), 16U);
		} else {
			texture.width			= std::max(round_up(width),  16U);
			texture.height			= std::max(round_up(height), 16U);
		}

		// Allocate the RAM.
		const std::size_t buffer_size = texture.width * texture.height * bpp;
		texture.ram	= static_cast<texel*>(memalign(16, buffer_size));
		if (!texture.ram)
		{
			Sys_Error("Out of RAM for lightmap textures.");
		}

		// Allocate the VRAM.
		texture.vram = static_cast<texel*>(quake::vram::allocate(buffer_size));

		// Upload the texture.
		if (bpp == 1)
			GL_Upload8_A(texture_index, data, width, height);
		else if (bpp == 2)
			GL_Upload16(texture_index, data, width, height);
		else if (bpp == 3)
			GL_Upload24(texture_index, data, width, height);
		else if (bpp == 4)
			GL_Upload32(texture_index, data, width, height);

		if (texture.vram && texture.ram) {
			free(texture.ram);
			texture.ram = NULL;
		}
	}
	else {
		gltexture_t& texture = gltextures[texture_index];

		if ((width == texture.original_width) &&
		    (height == texture.original_height)) {

				if (bpp == 1)
					GL_Upload8_A(texture_index, data, width, height);
				else if (bpp == 2)
					GL_Upload16(texture_index, data, width, height);
				else if (bpp == 3)
					GL_Upload24(texture_index, data, width, height);
				else if (bpp == 4)
					GL_Upload32(texture_index, data, width, height);
			}

		if (texture.vram && texture.ram) {
			free(texture.ram);
			texture.ram = NULL;
		}
	}
	// Done.
	return texture_index;
}


int GL_LoadPalettedTexture (const char *identifier, int width, int height, const byte *data, qboolean stretch_to_power_of_two, int filter, int mipmap_level, unsigned char* palette) //int bytesperpixel)
{

	int	i, p, s;
	int texture_index = -1;

	s = width*height;

	tex_scale_down = r_tex_scale_down.value == qtrue;

	// Out of textures?
	if (numgltextures == MAX_GLTEXTURES)
	{
		Sys_Error("Out of OpenGL textures");
	}

	// Use the next available texture.
	numgltextures++;
	texture_index = numgltextures;

	for (int i = 0; i < MAX_GLTEXTURES; ++i)
	{
		if (gltextures_used[i] == false) {
			texture_index = i;
			break;
		}
	}
	gltexture_t& texture = gltextures[texture_index];
	gltextures_used[texture_index] = true;

	// Fill in the source data.
	strcpy(texture.identifier, identifier);
	texture.original_width			= width;
	texture.original_height			= height;
	texture.stretch_to_power_of_two	= stretch_to_power_of_two != qfalse;

	texture.palette = static_cast<ScePspRGBA8888*>(Q_malloc(1024));
	for (ScePspRGBA8888* color = &texture.palette[0]; color < &texture.palette[256]; ++color)
	{
		const unsigned int r = *palette;
		*palette++;
		const unsigned int g = *palette;
		*palette++;
		const unsigned int b = *palette;*palette++;
		*color = GU_RGBA(r, g, b, 0xff);
	}

	texture.format = GU_PSM_T8;

	texture.filter			= filter;
	texture.mipmaps			= mipmap_level;

	if (tex_scale_down == true && texture.stretch_to_power_of_two == true) {
		texture.width			= std::max(round_down(width), 32U);
		texture.height			= std::max(round_down(height),32U);
	} else {
		texture.width			= std::max(round_up(width), 32U);
		texture.height			= std::max(round_up(height),32U);
	}

	for (int i=0; i <= mipmap_level;i++) {
		int div = (int) powf(2,i);
		if ((texture.width / div) > 16 && (texture.height / div) > 16 ) {
			texture.mipmaps = i;
		}
	}

	// Do we really need to resize the texture?
	if (texture.stretch_to_power_of_two)
	{
		// Not if the size hasn't changed.
		texture.stretch_to_power_of_two = (texture.width != width) || (texture.height != height);
	}

	Con_DPrintf("Loading: %s [%dx%d](%0.2f KB)\n",texture.identifier,texture.width,texture.height, (float) (texture.width*texture.height)/1024);

	// Allocate the RAM.
	std::size_t buffer_size = texture.width * texture.height;

	if (texture.mipmaps > 0) {
		int size_incr = buffer_size/4;
		for (int i= 1;i <= texture.mipmaps;i++) {
			buffer_size += size_incr;
			size_incr = size_incr/4;
		}
	}

	texture.ram	= static_cast<texel*>(memalign(16, buffer_size));

	if (!texture.ram)
	{
		Sys_Error("Out of RAM for textures.");
	}

	// Allocate the VRAM.
	texture.vram = static_cast<texel*>(quake::vram::allocate(buffer_size));

    GL_Upload8(texture_index, data, width, height);

	if (texture.vram && texture.ram) {
		free(texture.ram);
		texture.ram = NULL;
	}

	// Done.
	return texture_index;
}
