// D3D diff 14 of 14
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
// gl_draw.c -- this is the only file outside the refresh that touches the vid buffer

#include "quakedef.h"

extern unsigned char d_15to8table[65536];

cvar_t		gl_nobind = {"gl_nobind", "0"};
#ifdef DX8QUAKE_GET_GL_MAX_SIZE
int			gl_max_size = 1024;
#else
cvar_t		gl_max_size = {"gl_max_size", "1024"};
#endif
cvar_t		gl_picmip = {"gl_picmip", "0", true};
cvar_t		gl_crosshairalpha = {"crosshairalpha", "1", true};
cvar_t		gl_texturemode = {"gl_texturemode", "GL_NEAREST", true}; // Let's not save to config

#ifdef SUPPORTS_GL_DELETETEXTURES
cvar_t		gl_free_world_textures = {"gl_free_world_textures","1",true};//R00k
#endif

cvar_t		crosshaircolor = {"crosshaircolor", "15", true};

cvar_t		crosshairsize = {"crosshairsize", "1", true};

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;

int			translate_texture;
int			char_texture;

extern	cvar_t	crosshair, cl_crosshaircentered, cl_crossx, cl_crossy; //, crosshaircolor, crosshairsize;

qpic_t		crosshairpic;

typedef struct
{
	int		texnum;
	float	sl, tl, sh, th;
} glpic_t;

byte		conback_buffer[sizeof(qpic_t) + sizeof(glpic_t)];
qpic_t		*conback = (qpic_t *)&conback_buffer;

extern int GL_LoadPicTexture (qpic_t *pic);
extern qboolean VID_Is8bit();

int		gl_lightmap_format = 4;
int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;


int		texels;

#define		NUMCROSSHAIRS	5
int		crosshairtextures[NUMCROSSHAIRS];

static byte crosshairdata[NUMCROSSHAIRS][64] = {
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
	0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
	0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff,
	0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,

	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
	0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

typedef struct
{
	int				texnum;
	char			identifier[MAX_QPATH];
	int				width, height;
//	qboolean		mipmap;
	unsigned short	crc;  // Baker 3.80x - part of GL_LoadTexture: cache mismatch fix
	int				texmode;	// Baker: 4.26 to all clearing of world textures
} gltexture_t;

gltexture_t	gltextures[MAX_GLTEXTURES];
int			numgltextures;


void GL_Bind (int texnum)
{
#ifdef MACOSX_EXTRA_FEATURES
        extern qboolean		gl_texturefilteranisotropic;
#endif /* MACOSX */

	if (gl_nobind.value)
		texnum = char_texture;
	if (currenttexture == texnum)
		return;

	currenttexture = texnum;
#if defined(DX8QUAKE_NO_BINDTEXFUNC) || !defined(_WIN32)
	glBindTexture(GL_TEXTURE_2D, texnum);
#else // DX8QUAKE or MACOSX
	bindTexFunc (GL_TEXTURE_2D, texnum);
#endif

#ifdef MACOSX_EXTRA_FEATURES
        if (gl_texturefilteranisotropic) {
            extern GLfloat	gl_texureanisotropylevel;

            glTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &gl_texureanisotropylevel);
        }
#endif /* MACOSX */
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar objects into a single texture

  to crutch up stupid hardware / drivers

=============================================================================
*/

// some cards have low quality of alpha pics, so load the pics
// without transparent pixels into a different scrap block.
// scrap 0 is solid pics, 1 is transparent
#define	MAX_SCRAPS		2
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int		scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];
qboolean	scrap_dirty;
int		scrap_texnum;

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j, best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++) {
			best2 = 0;

			for (j=0 ; j<w ; j++) {
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("Scrap_AllocBlock: full");
        return(0);
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	int		texnum;

	scrap_uploads++;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++) {
		GL_Bind(scrap_texnum + texnum);
		GL_Upload8 (scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, TEX_ALPHA);
	}
	scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	qpic_t		pic;
	byte		padding[32];	// for appended glpic
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

byte		menuplyr_pixels[4096];

int		pic_texels;
int		pic_count;

qpic_t *Draw_PicFromWad (char *name)
{
	qpic_t	*p;
	glpic_t	*gl;

	p = W_GetLumpName (name);
	gl = (glpic_t *)p->data;

	// load little ones into the scrap
	if (p->width < 64 && p->height < 64)
	{
		int		x, y, i, j, k, texnum;

		texnum = Scrap_AllocBlock (p->width, p->height, &x, &y);
		scrap_dirty = true;
		k = 0;
		for (i=0 ; i<p->height ; i++)
			for (j=0 ; j<p->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = p->data[k];
		texnum += scrap_texnum;
		gl->texnum = texnum;
		gl->sl = (x+0.01)/(float)BLOCK_WIDTH;
		gl->sh = (x+p->width-0.01)/(float)BLOCK_WIDTH;
		gl->tl = (y+0.01)/(float)BLOCK_WIDTH;
		gl->th = (y+p->height-0.01)/(float)BLOCK_WIDTH;

		pic_count++;
		pic_texels += p->width*p->height;
	}
	else
	{
		gl->texnum = GL_LoadPicTexture (p);
		gl->sl = 0;
		gl->sh = 1;
		gl->tl = 0;
		gl->th = 1;
	}
	return p;
}

/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	int			i;
	cachepic_t	*pic;
	qpic_t		*dat;
	glpic_t		*gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	strcpy (pic->name, path);

// load the pic from disk
	if (!(dat = (qpic_t *)COM_LoadTempFile (path)))
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
	gl->texnum = GL_LoadPicTexture (dat);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return &pic->pic;
}


void Draw_CharToConback (int num, byte *dest)
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

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
===============
Draw_LoadPics -- johnfitz
===============
*/
void Draw_LoadPics (void)
{
	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad ("backtile");
}

/*
===============
Draw_TextureMode_f
===============
*/
void OnChange_gl_texturemode (void)
{
	int		i;
	gltexture_t	*glt;
	static	qboolean recursiveblock=false;

	if (recursiveblock)
		return;		// Get out

	for (i=0 ; i< 6 ; i++)
	{
		char *str = gl_texturemode.string;
		if (!strcasecmp (modes[i].name, str))
			break;

		if (isdigit(*str) && atoi(str) - 1 == i) {
			// We have a number, set the cvar as the mode name
			recursiveblock = true; // Let's prevent this from occuring twice
			Cvar_SetStringByRef (&gl_texturemode, modes[i].name);
			recursiveblock = false;
			break;
		}
	}

	if (i == 6)
	{
		Con_Printf ("bad filter name, available are:\n");
		for (i=0 ; i< 6 ; i++)
			Con_Printf ("%s (%d)\n", modes[i].name, i + 1);

		Cvar_SetValueByRef (&gl_texturemode, 1);
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->texmode & TEX_MIPMAP)
		{
			Con_DPrintf ("Doing texture %s\n", glt->identifier);
			GL_Bind (glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

// D3D diff 1 of 14
#ifdef D3DQ_EXTRA_FEATURES

#define    D3D_TEXTURE_MAXANISOTROPY 0xf70001
float gl_maxAnisotropy = 1.0;
/*
===============
Draw_MaxAnisotropy_f
===============
*/
void Draw_MaxAnisotropy_f (void)
{
	int		i;
	gltexture_t	*glt;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("current max anisotropy is %g\n", gl_maxAnisotropy);
		return;
	}

	gl_maxAnisotropy = atof(Cmd_Argv(1));

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->texmode & TEX_MIPMAP)
		{
			GL_Bind (glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, D3D_TEXTURE_MAXANISOTROPY, gl_maxAnisotropy);
		}
	}
}

#endif // D3D_FEATURE

/*
===============
Draw_SmoothFont_f
===============
*/
static qboolean smoothfont = 1;
qboolean smoothfont_init =false;

static void SetSmoothFont (void)
{
	smoothfont_init = true; // This is now available
	GL_Bind (char_texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smoothfont ? GL_LINEAR : GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smoothfont ? GL_LINEAR : GL_NEAREST);
}


void SmoothFontSet(qboolean smoothfont_choice) {
	smoothfont = smoothfont_choice;
	if (smoothfont_init)
		SetSmoothFont();
}

void Draw_SmoothFont_f (void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("gl_smoothfont is %d\n", smoothfont);
		return;
	}

	smoothfont = atoi (Cmd_Argv(1));
	SetSmoothFont ();
}

static void Load_CharSet (void)
{
	int  i;
	byte *dest, *src;

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
	draw_chars = W_GetLumpName ("conchars");
	for (i=0 ; i<256*64 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	// Expand charset texture with blank lines in between to avoid in-line distortion
	dest = Q_malloc (128 * 256);
	memset (dest, 0, 128 * 256);
	src = draw_chars;

	for (i = 0; i < 16; ++i)
		memcpy (&dest[8 * 128 * 2 * i], &src[8 * 128 * i], 8 * 128); // Copy each line

	// now turn them into textures
	char_texture = GL_LoadTexture ("charset", 128, 256, dest, TEX_ALPHA);

	free (dest);

	SetSmoothFont ();
}

/*
===============
Draw_Init
===============
*/
// D3D diff 2 of 14
#ifdef D3DQ_EXTRA_FEATURES

float d3dGetD3DDriverVersion();
#endif
void Draw_InitConback_Old(void);
void Draw_Init (void)
{
	int		i;
//	qpic_t		*cb;
//	int		start;
//	byte		*dest;
//	int		x, y;
//	char		ver[40];
//	glpic_t		*gl;

//	byte		*ncdata;

	Cvar_RegisterVariable (&gl_nobind, NULL);

	Cvar_RegisterVariable (&gl_picmip, NULL);
	Cvar_RegisterVariable (&gl_crosshairalpha, NULL);
	Cvar_RegisterVariable (&crosshaircolor, NULL);

	Cvar_RegisterVariable (&crosshairsize, NULL);

#ifdef SUPPORTS_GL_DELETETEXTURES
	Cvar_RegisterVariable (&gl_free_world_textures, NULL);//R00k
#endif

#ifdef DX8QUAKE_GET_GL_MAX_SIZE
	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &gl_max_size);
#else
	Cvar_RegisterVariable (&gl_max_size, NULL);
	// 3dfx can only handle 256 wide textures
	if (!strncasecmp ((char *)gl_renderer, "3dfx",4) ||
		strstr((char *)gl_renderer, "Glide"))
		Cvar_SetValueByRef (&gl_max_size, 256);
#endif

	Cvar_RegisterVariable (&gl_texturemode, &OnChange_gl_texturemode);
	Cmd_AddCommand ("gl_smoothfont", Draw_SmoothFont_f);
// D3D diff 3 of 14
#ifdef D3DQ_EXTRA_FEATURES

	Cmd_AddCommand ("d3d_maxanisotropy", Draw_MaxAnisotropy_f);
#endif
	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
/*	draw_chars = W_GetLumpName ("conchars");
	for (i=0 ; i<256*64 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	// now turn them into textures
	char_texture = GL_LoadTexture ("charset", 128, 128, draw_chars, false, true);*/


	Load_CharSet ();

	Draw_InitConback_Old();

	// save a texture slot for translated picture
	translate_texture = texture_extension_number++;

	// save slots for scraps
	scrap_texnum = texture_extension_number;
	texture_extension_number += MAX_SCRAPS;

	// Load the crosshair pics
	for (i=0 ; i<NUMCROSSHAIRS ; i++) {
		crosshairtextures[i] = GL_LoadTexture ("", 8, 8, crosshairdata[i], TEX_ALPHA);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// load game pics
	Draw_LoadPics ();

}

void Draw_InitConback_Old(void) {
//	int		i;
	qpic_t	*cb;
	int		start;
	byte	*dest;
	int		x, y;
	char	ver[40];
	glpic_t	*gl;

	byte	*ncdata;

	start = Hunk_LowMark();

	cb = (qpic_t *)COM_LoadTempFile ("gfx/conback.lmp");
	if (!cb)
		Sys_Error ("Couldn't load gfx/conback.lmp");
	SwapPic (cb);

	// hack the version number directly into the pic


	snprintf(ver, sizeof(ver), "(ProQuake) %4.2f", (float)PROQUAKE_SERIES_VERSION); // JPG - obvious change



	dest = cb->data + 320*186 + 320 - 11 - 8*strlen(ver);
	y = strlen(ver);
	for (x=0 ; x<y ; x++)
		Draw_CharToConback (ver[x], dest+(x<<3));

	conback->width = cb->width;
	conback->height = cb->height;
	ncdata = cb->data;

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	gl = (glpic_t *)conback->data;
	gl->texnum = GL_LoadTexture ("conback", conback->width, conback->height, ncdata, TEX_NOFLAGS);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;
	conback->width = vid.width;
	conback->height = vid.height;

	// free loaded console
	Hunk_FreeToLowMark(start);
}


/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
// D3D diff 6 of 14
// Begin D3DQuake
int gNoChars;
// End D3DQuake

static qboolean IsValid (int y, int num)
{
	if ((num & 127) == 32)
		return false; // space

	if (y <= -8)
		return false; // totally off screen

	return true;
}

static void Character (int x, int y, int num)
{
	int			row, col;
	float	frow, fcol, size, offset;

	num &= 255;

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;
//	offset = 0.002; // slight offset to avoid in-between lines distortion
	offset = 0.03125; // offset to match expanded charset texture

	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + size, frow);
	glVertex2f (x+8, y);
	glTexCoord2f (fcol + size, frow + size - offset);
	glVertex2f (x+8, y+8);
	glTexCoord2f (fcol, frow + size - offset);
	glVertex2f (x, y+8);
}

void Draw_Character (int x, int y, int num)
{
// D3D diff 7 of 14
// Begin D3DQuake
	if ( gNoChars ) return;
// End D3DQuake
	if (!IsValid (y, num))
		return;

	GL_Bind (char_texture);

	glBegin (GL_QUADS);

	Character (x, y, num);

	glEnd ();
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	GL_Bind (char_texture);

	glBegin (GL_QUADS);

	while (*str)
	{
		if (IsValid (y, *str))
			Character (x, y, *str);

		str++;
		x += 8;
	}

	glEnd ();
}

byte *StringToRGB (char *s) {
	byte		*col;
	static	byte	rgb[4];

	Cmd_TokenizeString (s);
	if (Cmd_Argc() == 3) {
		rgb[0] = (byte)atoi(Cmd_Argv(0));
		rgb[1] = (byte)atoi(Cmd_Argv(1));
		rgb[2] = (byte)atoi(Cmd_Argv(2));
	} else {
		col = (byte *)&d_8to24table[(byte)atoi(s)];
		rgb[0] = col[0];
		rgb[1] = col[1];
		rgb[2] = col[2];
	}
	rgb[3] = 255;

	return rgb;
}

/*
================
Draw_Crosshair		-- joe, from FuhQuake
================
*/
void Draw_Crosshair(void)
{
	float		x, y, ofs1, ofs2, sh, th, sl, tl;
	byte		*col;
	extern vrect_t		scr_vrect;

	if (crosshair.value >= 2  && (crosshair.value <= NUMCROSSHAIRS + 1)) {
		x = scr_vrect.x + scr_vrect.width/2 + cl_crossx.value;
		y = scr_vrect.y + scr_vrect.height/2 + cl_crossy.value;

		if (!gl_crosshairalpha.value)
			return;

		glTexEnvf ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

		col = StringToRGB (crosshaircolor.string);

		if (gl_crosshairalpha.value) {
			glDisable (GL_ALPHA_TEST);
			glEnable (GL_BLEND);
			col[3] = bound(0, gl_crosshairalpha.value, 1) * 255;
#if defined(D3DQ_WORKAROUND)// || defined(DX8QUAKE)
// D3DQUAKE doesn't have gl_Color4ubv wrapper or whatever

// so we'd have to extract the RGB colors and pass them in glColor4f

// which I am too lazy to do right now.

			glColor4f (1, 1, 1, bound(0, gl_crosshairalpha.value, 1));

#else
			glColor4ubv (col);
#endif // Workaround

		} else {
#ifdef D3DQ_WORKAROUND
			glColor3f (1, 1, 1);

#else

			glColor3ubv (col);
#endif // ditto

		}

			GL_Bind (crosshairtextures[(int)crosshair.value-2]);
			ofs1 = 3.5;
			ofs2 = 4.5;
			tl = sl = 0;
			sh = th = 1;

		ofs1 *= (vid.width / 320) * bound(0, crosshairsize.value, 20);
		ofs2 *= (vid.width / 320) * bound(0, crosshairsize.value, 20);

		glBegin (GL_QUADS);
		glTexCoord2f (sl, tl);
		glVertex2f (x - ofs1, y - ofs1);
		glTexCoord2f (sh, tl);
		glVertex2f (x + ofs2, y - ofs1);
		glTexCoord2f (sh, th);
		glVertex2f (x + ofs2, y + ofs2);
		glTexCoord2f (sl, th);
		glVertex2f (x - ofs1, y + ofs2);
		glEnd ();

		if (gl_crosshairalpha.value) {
			glDisable (GL_BLEND);
			glEnable (GL_ALPHA_TEST);
		}

		glTexEnvf ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		glColor3f (1, 1, 1);
	} else if (crosshair.value) {


			if (!cl_crosshaircentered.value) {

				// Standard off-center Quake crosshair

				Draw_Character (scr_vrect.x + scr_vrect.width/2 + cl_crossx.value, scr_vrect.y + scr_vrect.height/2 + cl_crossy.value, '+');
			} else {

				// Baker 3.60 - Centered crosshair (FuhQuake)

				Draw_Character (scr_vrect.x + scr_vrect.width / 2 - 4 + cl_crossx.value, scr_vrect.y + scr_vrect.height / 2 - 4 + cl_crossy.value, '+');

			}
	}

}




/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, qpic_t *pic, float alpha)
{
	glpic_t			*gl;

	if (scrap_dirty)
		Scrap_Upload ();
	gl = (glpic_t *)pic->data;
	glDisable(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glCullFace(GL_FRONT);
// D3D diff 8 of 14
#ifdef D3DQ_WORKAROUND

	if ( alpha > 1 ) alpha = 1; // manually clamp
#endif
	glColor4f (1,1,1,alpha);
	GL_Bind (gl->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
	glColor4f (1,1,1,1);
	glEnable(GL_ALPHA_TEST);
	glDisable (GL_BLEND);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	glpic_t			*gl;

	if (scrap_dirty)
		Scrap_Upload ();
	gl = (glpic_t *)pic->data;
	glColor4f (1,1,1,1);
	GL_Bind (gl->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
}

void Draw_SubPic(int x, int y, qpic_t *pic, int srcx, int srcy, int width, int height)
{
	glpic_t			*gl;
	float newsl, newtl, newsh, newth;
	float oldglwidth, oldglheight;

	if (scrap_dirty)
		Scrap_Upload ();
	gl = (glpic_t *)pic->data;

	oldglwidth = gl->sh - gl->sl;
	oldglheight = gl->th - gl->tl;

	newsl = gl->sl + (srcx*oldglwidth)/pic->width;
	newsh = newsl + (width*oldglwidth)/pic->width;

	newtl = gl->tl + (srcy*oldglheight)/pic->height;
	newth = newtl + (height*oldglheight)/pic->height;

	glColor4f (1,1,1,1);
	GL_Bind (gl->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (newsl, newtl);
	glVertex2f (x, y);
	glTexCoord2f (newsh, newtl);
	glVertex2f (x+width, y);
	glTexCoord2f (newsh, newth);
	glVertex2f (x+width, y+height);
	glTexCoord2f (newsl, newth);
	glVertex2f (x, y+height);
	glEnd ();
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 || (unsigned)(y + pic->height) > vid.height)
		Sys_Error ("Draw_TransPic: bad coordinates");

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
	int				v, u, c, p;
	unsigned		trans[64*64], *dest;
	byte			*src;

	GL_Bind (translate_texture);

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			dest[u] = (p == 255) ? p : d_8to24table[translation[p]];
		}
	}

#ifdef MACOSX_TEXRAM_CHECK
        GL_CheckTextureRAM (GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE);
#endif /* MACOSX */

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
	glEnd ();
}

/*
================
Draw_ConsoleBackground
================
*/
// D3D diff 9 of 14
// Begin D3DQuake
int noConsoleBackground;
// End D3DQuake
void Draw_ConsoleBackground (int lines)
{
	int y = (vid.height * 3) >> 2;
// D3D diff 10 of 14
// Begin D3DQuake
	if (noConsoleBackground)
		return;
// End D3DQuake
#if 0
#ifdef MACOSX_UNKNOWN_DIFFERENCE

	conback->width	= vid.width;
	conback->height	= vid.height;

#endif /* MACOSX */
#endif // ^^ Why was this done in Fruitz?
	if (lines > y)
		Draw_Pic(0, lines - vid.height, conback);
	else
		Draw_AlphaPic (0, lines - vid.height, conback, (float)(1.2 * lines)/y);
}

#ifdef MACOSX // Cannot go in gl_vidnt.c equivalent; must go here
/*
==================
VID_Consize_f -- Baker -- called when vid_consize changes
==================
*/
extern qpic_t *conback;
//qboolean vid_smoothfont = false;
extern qboolean smoothfont_init;
extern int gGLDisplayWidth;
extern int gGLDisplayHeight;
void VID_Consize_f(void) {

	float startwidth;
	float startheight;
	float desiredwidth;
	extern cvar_t vid_consize;
	int contype = vid_consize.value;
	int exception = 0;

#ifdef _WIN32
	startwidth = vid.width = modelist[vid_default].width;
	startheight = vid.height = modelist[vid_default].height;
#else
	startwidth = vid.width = gGLDisplayWidth;
	startheight = vid.height = gGLDisplayHeight;

#endif

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
#endif

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	glColor3f (1,1,1);
	GL_Bind (*(int *)draw_backtile->data);
	glBegin (GL_QUADS);
	glTexCoord2f (x/64.0, y/64.0);
	glVertex2f (x, y);
	glTexCoord2f ( (x+w)/64.0, y/64.0);
	glVertex2f (x+w, y);
	glTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	glVertex2f (x+w, y+h);
	glTexCoord2f ( x/64.0, (y+h)/64.0 );
	glVertex2f (x, y+h);
	glEnd ();
}

#ifdef SUPPORTS_2DPICS_ALPHA
/*
=============
Draw_AlphaFill

Fills a box of pixels with a single color
=============
*/
void Draw_AlphaFill(int x, int y, int w, int h, int c, float alpha)
{
	alpha = bound(0, alpha, 1);

	if (!alpha)
		return;

	glDisable (GL_TEXTURE_2D);
	if (alpha < 1)
	{
		glEnable (GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glColor4f (host_basepal[c * 3] / 255.0,  host_basepal[c * 3 + 1] / 255.0, host_basepal[c * 3 + 2] / 255.0, alpha);
	}
	else
	{
		glColor3f (host_basepal[c * 3] / 255.0, host_basepal[c * 3 + 1] / 255.0, host_basepal[c * 3 + 2]  /255.0);
	}

	glBegin (GL_QUADS);
	glVertex2f (x, y);
	glVertex2f (x + w, y);
	glVertex2f (x + w, y + h);
	glVertex2f (x, y + h);
	glEnd ();

	glEnable (GL_TEXTURE_2D);
	if (alpha < 1)
	{
		glEnable(GL_ALPHA_TEST);
		glDisable (GL_BLEND);
	}
	glColor3f (1, 1, 1);
}
#endif

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	glDisable (GL_TEXTURE_2D);
	glColor3f (host_basepal[c*3]/255.0,
		host_basepal[c*3+1]/255.0,
		host_basepal[c*3+2]/255.0);

	glBegin (GL_QUADS);

	glVertex2f (x,y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);

	glEnd ();
	glColor3f (1,1,1);
	glEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen
================
*/
void Draw_FadeScreen (void)
{
	extern cvar_t gl_fadescreen_alpha;

	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);
	glColor4f (0, 0, 0, gl_fadescreen_alpha.value);
	glBegin (GL_QUADS);
	glVertex2f (0,0);
	glVertex2f (vid.width, 0);
	glVertex2f (vid.width, vid.height);
	glVertex2f (0, vid.height);
	glEnd ();

	glColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);

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
#ifdef INTEL_OPENGL_DRIVER_WORKAROUND
	extern qboolean IntelDisplayAdapter;
	// Baker: Intel display adapters issue fix

	if (IntelDisplayAdapter)
		return;
#endif

#ifdef MACOSX
	return; // Baker: ARWOP and Warpspasm troubles from this like Quakespasm?
#endif

	if (!draw_disc)
		return;

	if (mod_conhide==true && (key_dest != key_console && key_dest != key_message)) {
		// No draw this either
		return;
	}

	glDrawBuffer  (GL_FRONT);
	Draw_Pic (vid.width - 24, 0, draw_disc);
	glDrawBuffer  (GL_BACK);
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
	glViewport (glx, gly, glwidth, glheight);

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
//	glDisable (GL_ALPHA_TEST);

	glColor4f (1,1,1,1);
}

//=============================================================================


/*
================
GL_FindTexture
================
*/
int GL_FindTexture (char *identifier)
{
	int		i;
	gltexture_t	*glt;

	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (!strcmp (identifier, glt->identifier))
			return gltextures[i].texnum;
	}

	return -1;
}

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j+=4)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
			out[j+1] = inrow[frac>>16];
			frac += fracstep;
			out[j+2] = inrow[frac>>16];
			frac += fracstep;
			out[j+3] = inrow[frac>>16];
			frac += fracstep;
		}
	}
}

#ifndef DX8QUAKE_NO_8BIT
/*
================
GL_Resample8BitTexture -- JACK
================
*/
void GL_Resample8BitTexture (unsigned char *in, int inwidth, int inheight, unsigned char *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	char *inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j+=4)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
			out[j+1] = inrow[frac>>16];
			frac += fracstep;
			out[j+2] = inrow[frac>>16];
			frac += fracstep;
			out[j+3] = inrow[frac>>16];
			frac += fracstep;
		}
	}
}
#endif

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

#ifndef DX8QUAKE_NO_8BIT
/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void GL_MipMap8Bit (byte *in, int width, int height)
{
	int		i, j;
	unsigned short     r,g,b;
	byte	*out, *at1, *at2, *at3, *at4;

//	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=2, out+=1, in+=2)
		{
			at1 = (byte *) (d_8to24table + in[0]);
			at2 = (byte *) (d_8to24table + in[1]);
			at3 = (byte *) (d_8to24table + in[width+0]);
			at4 = (byte *) (d_8to24table + in[width+1]);

 			r = (at1[0]+at2[0]+at3[0]+at4[0]); r>>=5;
 			g = (at1[1]+at2[1]+at3[1]+at4[1]); g>>=5;
 			b = (at1[2]+at2[2]+at3[2]+at4[2]); b>>=5;

			out[0] = d_15to8table[(r<<0) + (g<<5) + (b<<10)];
		}
	}
}
#endif

/*
===============
GL_Upload32
===============
*/
// D3D diff 11 of 11
#ifdef D3DQ_WORKAROUND

void d3dHint_GenerateMipMaps(int);
#endif
void GL_Upload32 (unsigned *data, int width, int height, int mode)
{
	int			samples;
static	unsigned	scaled[2048*1024]; //scaled[1024*512];	// [512*256];
	int			scaled_width, scaled_height;

	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;

	scaled_width >>= (int)gl_picmip.value;
	scaled_height >>= (int)gl_picmip.value;

#ifdef DX8QUAKE_GET_GL_MAX_SIZE
	if (scaled_width > gl_max_size) scaled_width = gl_max_size;
	if (scaled_height > gl_max_size) scaled_height = gl_max_size;
#else
	if (scaled_width > gl_max_size.value)
		scaled_width = gl_max_size.value;
	if (scaled_height > gl_max_size.value)
		scaled_height = gl_max_size.value;
#endif

	if (scaled_width * scaled_height > sizeof(scaled)/4)
		Sys_Error ("GL_LoadTexture: too big");

	samples = (mode & TEX_ALPHA) ? gl_alpha_format : gl_solid_format;

#if 0
	if (mipmap)
		gluBuild2DMipmaps (GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else if (scaled_width == width && scaled_height == height)
         {
#ifdef MACOSX_TEXRAM_CHECK
                GL_CheckTextureRAM (GL_TEXTURE_2D, 0, samples, width, height, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
		glTexImage2D (GL_TEXTURE_2D, 0, samples, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
         }
	else
	{
                    gluScaleImage (GL_RGBA, width, height, GL_UNSIGNED_BYTE, trans,
                            scaled_width, scaled_height, GL_UNSIGNED_BYTE, scaled);
#ifdef MACOSX_TEXRAM_CHECK
                    GL_CheckTextureRAM (GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
                    glTexImage2D (GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
    }
#else
texels += scaled_width * scaled_height;

	if (scaled_width == width && scaled_height == height)
	{
		if (!(mode & TEX_MIPMAP))
		{
// D3D diff 12 of 14
#ifdef D3DQ_WORKAROUND

			d3dHint_GenerateMipMaps(0);
#endif
#ifdef MACOSX_TEXRAM_CHECK
                        GL_CheckTextureRAM (GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
			glTexImage2D (GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
// D3D diff 13 of 14
#ifdef D3DQ_WORKAROUND

			d3dHint_GenerateMipMaps(1);
#endif
			goto done;
		}
		memcpy (scaled, data, width*height*4);
	}
	else
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);

#ifdef MACOSX_TEXRAM_CHECK
        GL_CheckTextureRAM (GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
	glTexImage2D (GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	if ((mode & TEX_MIPMAP))
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
#ifdef MACOSX_TEXRAM_CHECK
                        GL_CheckTextureRAM (GL_TEXTURE_2D, miplevel, samples, scaled_width, scaled_height, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE);
#endif /* MACOSX */

			glTexImage2D (GL_TEXTURE_2D, miplevel, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		}
	}
done: ;
#endif


	if ((mode & TEX_MIPMAP))
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}

#ifndef DX8QUAKE_NO_8BIT
void GL_Upload8_EXT (byte *data, int width, int height, int mode)
{
	int			i, s;
	qboolean		noalpha;
	int			samples;
        static unsigned char 	scaled[1024*512];	// [512*256];
	int			scaled_width, scaled_height;

	s = width*height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if (mode & TEX_ALPHA)
	{
		noalpha = true;
		for (i=0 ; i<s ; i++)
		{
			if (data[i] == 255)
				noalpha = false;
		}

		if (noalpha)
			mode = mode - TEX_ALPHA;
	}
	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;

	scaled_width >>= (int)gl_picmip.value;
	scaled_height >>= (int)gl_picmip.value;

	if (scaled_width > gl_max_size.value)
		scaled_width = gl_max_size.value;
	if (scaled_height > gl_max_size.value)
		scaled_height = gl_max_size.value;

	if (scaled_width * scaled_height > sizeof(scaled))
		Sys_Error ("GL_LoadTexture: too big");

	samples = 1; // alpha ? gl_alpha_format : gl_solid_format;

	texels += scaled_width * scaled_height;

	if (scaled_width == width && scaled_height == height)
	{
		if (!(mode & TEX_MIPMAP))
		{
#ifdef MACOSX_TEXRAM_CHECK
                        GL_CheckTextureRAM (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
			glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX , GL_UNSIGNED_BYTE, data);
			goto done;
		}
		memcpy (scaled, data, width*height);
	}
	else
		GL_Resample8BitTexture (data, width, height, scaled, scaled_width, scaled_height);

#ifdef MACOSX_TEXRAM_CHECK
        GL_CheckTextureRAM (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
	glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);
	if ((mode & TEX_MIPMAP))
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap8Bit ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
#ifdef MACOSX_TEXRAM_CHECK
            GL_CheckTextureRAM (GL_TEXTURE_2D, miplevel, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
                        glTexImage2D (GL_TEXTURE_2D, miplevel, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);
		}
	}
done: ;


	if (mode & TEX_MIPMAP)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}
#endif

/*
===============
GL_Upload8
===============
*/
void GL_Upload8 (byte *data, int width, int height, int mode)
{
static	unsigned	trans[640*480];		// FIXME, temporary
	int			i, s;
	qboolean	noalpha;
	int			p;

	s = width*height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if (mode & TEX_ALPHA)
	{
		noalpha = true;
		for (i=0 ; i<s ; i++)
		{
			p = data[i];
			if (p == 255)
				noalpha = false;
			trans[i] = d_8to24table[p];
		}

		if (noalpha)
			mode = mode - TEX_ALPHA; // Baker: we know this bit flag exists due to IF, so just subtract it
	}
	else
	{
		if (s&3)
			Sys_Error ("GL_Upload8: s&3");
		for (i=0 ; i<s ; i+=4)
		{
			trans[i] = d_8to24table[data[i]];
			trans[i+1] = d_8to24table[data[i+1]];
			trans[i+2] = d_8to24table[data[i+2]];
			trans[i+3] = d_8to24table[data[i+3]];
		}
	}

#ifndef DX8QUAKE_NO_8BIT
 	if (VID_Is8bit() && !(mode & TEX_ALPHA) && (data!=scrap_texels[0])) {
 		GL_Upload8_EXT (data, width, height, mode);
 		return;
	}
#endif

	GL_Upload32 (trans, width, height, mode);
}

#ifdef SUPPORTS_GL_DELETETEXTURES
/*
================
GL_FreeTextures -- BPJ
================
*/
void GL_FreeTextures (void)
{
	int i, j;

	Con_DPrintf("GL_FreeTextures: Entry.\n");

	if (gl_free_world_textures.value == 0)
	{
		Con_DPrintf("GL_FreeTextures: Not Clearing old Map Textures.\n");
		return;
	}

	Con_DPrintf("GL_FreeTextures: Freeing textures (numgltextures = %i) \n", numgltextures);

	for (i = j = 0; i < numgltextures; ++i, ++j)
	{
		if (gltextures[i].texmode & TEX_WORLD)//Only clear out world textures... for now.
		{
			Con_DPrintf("GL_FreeTextures: Clearing texture %s\n", gltextures[i].identifier);
			glDeleteTextures(1, &gltextures[i].texnum);
			--j;
		}
		else if (j < i) {
//			Con_DPrintf("GL_FreeTextures: NOT Clearing texture %s\n", gltextures[i].identifier);
			gltextures[j] = gltextures[i];
		}
	}

	numgltextures = j;

	Con_DPrintf("GL_FreeTextures: Completed (numgltextures = %i) \n", numgltextures);

}
#endif

/*
================
GL_LoadTexture
Baker 3.80x - Uses LordHavoc's fix to eliminate GL_LoadTexture: Cache mismatch
that occurs when maps with different textures using the same name are loaded.
Code provided by Reckless
================
*/
int GL_LoadTexture (char *identifier, int width, int height, byte *data, int mode)
{
	int			i;
	gltexture_t	*glt;
   unsigned short crc; // Baker 3.80x - LoadTexture fix LordHavoc provided by Reckless

   crc = CRC_Block (data, width*height); // Baker 3.80x - LoadTexture fix LordHavoc provided by Reckless

   // see if the texture is already present
	if (identifier[0])
	{
		for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
		{
			if (!strcmp (identifier, glt->identifier))
			{
            // Baker 3.60 - LoadTexture fix LordHavoc provided by Reckless
			if (width != glt->width || height != glt->height || crc != glt->crc)
               goto setup;
				return gltextures[i].texnum;
			}
		}
		// Jack Palevich -- surely we want to remember this new texture.
		// Doing this costs 1% fps per timedemo, probably because of the
		// linear search through the texture cache, but it saves 10 MB
		// of VM growth per level load. It also makes the GL_TEXTUREMODE
		// console command work correctly.
		// numgltextures++;  <---- Baker 3.70D3D - no implementing this yet, if at all

	}
//   else { get the hell rid of this :P
      glt = &gltextures[numgltextures++];
	//}

	strcpy (glt->identifier, identifier);
   glt->texnum = texture_extension_number++;

setup:
	glt->width = width;
	glt->height = height;
	glt->texmode = mode;
	glt->crc = crc;

	// Baker: part 1 of gl dedicated server fix by Nathan Cline
	if (cls.state != ca_dedicated) {
	   GL_Bind (glt->texnum);
		GL_Upload8 (data, width, height, mode);
	}

	return glt->texnum;
}

#if 0 //SUPPORTS_HLBSP
byte		vid_gamma_table[256];

void HalfLife_Gamma_Table (void) {

	int		i;
	float		inf;
	float   in_gamma;



	if ((i = COM_CheckParm("-gamma")) != 0 && i+1 < com_argc) {

		in_gamma = atof(com_argv[i+1]);

		if (in_gamma < 0.3) in_gamma = 0.3;

		if (in_gamma > 1) in_gamma = 1.0;

	} else {

		in_gamma = 1;

	}



	if (in_gamma != 1) {

		for (i=0 ; i<256 ; i++) {

			inf = QMIN(255 * powf((i + 0.5) / 255.5, in_gamma) + 0.5, 255);

			vid_gamma_table[i] = inf;

		}

	} else {

		for (i=0 ; i<256 ; i++)

			vid_gamma_table[i] = i;

	}



//	for (i=0 ; i<768 ; i++)

//		palette[i] = vid_gamma_table[pal[i]];



}
#endif


#ifdef SUPPORTS_HLBSP
/*

================

GL_LoadTexture32

================

*/

int GL_LoadTexture32 (char *identifier, int width, int height, byte *data, int mode)

{

	qboolean	noalpha;

	int			i, p, s;

	gltexture_t	*glt;

	int image_size = width * height;



	// see if the texture is already present

	if (identifier[0])

	{

		for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)

		{

			if (!strcmp (identifier, glt->identifier))

			{

				if (width != glt->width || height != glt->height)

					Sys_Error ("GL_LoadTexture: cache mismatch");

				return gltextures[i].texnum;

			}

		}

	}

	else {

		glt = &gltextures[numgltextures];

		numgltextures++;

	}



	strcpy (glt->identifier, identifier);

	glt->texnum = texture_extension_number;
	glt->width = width;
	glt->height = height;
	glt->texmode = mode;



	GL_Bind(texture_extension_number );



#if 0

	if (1 /*gamma*/ ) {

		//extern	byte	vid_gamma_table[256];

		for (i = 0; i < image_size; i++){

			data[4 * i] = vid_gamma_table[data[4 * i]];

			data[4 * i + 1] = vid_gamma_table[data[4 * i + 1]];

			data[4 * i + 2] = vid_gamma_table[data[4 * i + 2]];

		}

	}

#endif



	GL_Upload32 ((unsigned *)data, width, height, mode);



	texture_extension_number++;



	return texture_extension_number-1;

}

#endif


/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture (qpic_t *pic)
{
	return GL_LoadTexture ("", pic->width, pic->height, pic->data, TEX_ALPHA);
}

/****************************************/

#ifndef OLD_SGIS
GLenum	gl_oldtarget;
GLenum	gl_Texture0;
GLenum	gl_Texture1;

void GL_SelectTexture (GLenum target)
{
	if (!gl_mtexable)
		return;
	qglSelectTextureSGIS(target);
	if (target == gl_oldtarget)
		return;
	cnttextures[gl_oldtarget-TEXTURE0_SGIS] = currenttexture;
	currenttexture = cnttextures[target-TEXTURE0_SGIS];
	gl_oldtarget = target;
}
#else
static GLenum oldtarget = TEXTURE0_SGIS;

void GL_SelectTexture (GLenum target)
{
	if (!gl_mtexable)
		return;
	qglSelectTextureSGIS(target);
	if (target == oldtarget)
		return;
	cnttextures[oldtarget-TEXTURE0_SGIS] = currenttexture;
	currenttexture = cnttextures[target-TEXTURE0_SGIS];
	oldtarget = target;
}
#endif



lpMTexFUNC qglMTexCoord2fSGIS = NULL;
lpSelTexFUNC qglSelectTextureSGIS = NULL;

qboolean mtexenabled = false;

void GL_DisableMultitexture(void)
{
	if (mtexenabled) {
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(TEXTURE0_SGIS);
		mtexenabled = false;
	}
}

void GL_EnableMultitexture(void)
{
	if (gl_mtexable) {
		GL_SelectTexture(TEXTURE1_SGIS);
		glEnable(GL_TEXTURE_2D);
		mtexenabled = true;
	}
}
