/*

Copyright (C) 2001-2002       A Nourai

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// gl_texture.h

// Baker ...
/*
Basically texture related functions and variables go here
What doesn't go here is general OpenGL stuff.

And Cvars really shouldn't go here except in the
most exception of circumstances, right?
*/



// GL constants and API stuffs (not internal vars)

#ifndef GL_TEXTURE_H
#define GL_TEXTURE_H


#define	MAX_GLTEXTURES	1024

// Multitexture
#ifdef MACOSX
#define    TEXTURE0_SGIS				GL_TEXTURE0_ARB
#define    TEXTURE1_SGIS				GL_TEXTURE1_ARB
#else
#ifndef OLD_SGIS
extern GLenum gl_oldtarget;
extern GLenum gl_Texture0;
extern GLenum gl_Texture1;
#define    TEXTURE0_SGIS gl_Texture0
#define    TEXTURE1_SGIS gl_Texture1
#else
#define    TEXTURE0_SGIS				0x835E
#define    TEXTURE1_SGIS				0x835F
#endif
#endif /* MACOSX */

#ifndef GL_RGBA4
#define	GL_RGBA4	0
#endif

#if !defined(DX8QUAKE_NO_8BIT)
#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT     0x80E5
#endif /* GL_COLOR_INDEX8_EXT */
#endif

// GL external vars, functions

#ifdef _WIN32
// Function prototypes for the Texture Object Extension routines
typedef GLboolean (APIENTRY *ARETEXRESFUNCPTR)(GLsizei, const GLuint *, const GLboolean *);
typedef void (APIENTRY *BINDTEXFUNCPTR)(GLenum, GLuint);
typedef void (APIENTRY *DELTEXFUNCPTR)(GLsizei, const GLuint *);
typedef void (APIENTRY *GENTEXFUNCPTR)(GLsizei, GLuint *);
typedef GLboolean (APIENTRY *ISTEXFUNCPTR)(GLuint);
typedef void (APIENTRY *PRIORTEXFUNCPTR)(GLsizei, const GLuint *, const GLclampf *);
typedef void (APIENTRY *TEXSUBIMAGEPTR)(int, int, int, int, int, int, int, int, void *);

extern	BINDTEXFUNCPTR bindTexFunc;
extern	DELTEXFUNCPTR delTexFunc;
extern	TEXSUBIMAGEPTR TexSubImage2DFunc;

extern	PROC glArrayElementEXT;
extern	PROC glColorPointerEXT;
extern	PROC glTexturePointerEXT;
extern	PROC glVertexPointerEXT;
#endif

typedef void (APIENTRY *lpMTexFUNC) (GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *lpSelTexFUNC) (GLenum);
extern lpMTexFUNC qglMTexCoord2fSGIS;
extern lpSelTexFUNC qglSelectTextureSGIS;



// Engine internal vars

extern qboolean gl_mtexable;
#if !defined(DX8QUAKE_GET_GL_MAX_SIZE)
extern	cvar_t	gl_max_size;
#else
extern	int		gl_max_size;
#endif

extern	int texture_extension_number;

extern	texture_t	*r_notexture_mip;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	int	cnttextures[2];
extern	int	currenttexture;
extern	int	particletexture;
extern	int	playertextures;

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	int			mirrortexturenum;	// quake texturenum, not gltexturenum
extern	qboolean	mirror;
extern	mplane_t	*mirror_plane;

extern	int		texture_mode;
extern	int		gl_lightmap_format;


// Engine internal functions

void GL_Bind (int texnum);

void GL_SelectTexture (GLenum target);
void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);



void GL_Upload8_EXT (byte *, int, int, int);
void GL_Upload32 (unsigned *data, int width, int height, int mode);
void GL_Upload8 (byte *data, int width, int height, int mode);
int GL_LoadTexture (char *identifier, int width, int height, byte *data, int mode);
int GL_FindTexture (char *identifier);
int GL_LoadTexture32 (char *identifier, int width, int height, byte *data, int mode);

#endif