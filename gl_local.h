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
// gl_local.h -- private refresh defs

#ifndef GL_LOCAL_H
#define GL_LOCAL_H

// disable data conversion warnings

#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef MACOSX
#include 	<OpenGL/gl.h>
#include 	<OpenGL/glu.h>
#include	<OpenGL/glext.h>
#include	<math.h>
#elif defined(DX8QUAKE)
#include "dx8_fakegl.h"
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif /* MACOSX */

#ifndef APIENTRY
#define APIENTRY
#endif

#define TEX_NOFLAGS			0 // Baker: I use this to mark the absense of any flags
#define TEX_MIPMAP			2
#define TEX_ALPHA			4
#define	TEX_WORLD			128//R00k

#include "gl_texture.h"


void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

typedef struct {
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;

extern glvert_t glv;

extern	int glx, gly, glwidth, glheight;

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0) //normalizing factor so player model works out to about 1 pixel per triangle
#define	MAX_LBM_HEIGHT		480

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01






//====================================================


extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	entity_t	*currententity;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	mplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys;

// view origin
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

// screen size info
extern	refdef_t	r_refdef;
extern	mleaf_t		*r_viewleaf, *r_oldviewleaf;

#ifdef SUPPORTS_FOG // Baker: d3dquake hates this NATAS - BramBo - fog code
extern cvar_t gl_fogenable;
extern cvar_t gl_fogstart;
extern cvar_t gl_fogend;
extern cvar_t gl_fogdensity;
extern cvar_t gl_fogalpha;
extern cvar_t gl_fogred;
extern cvar_t gl_fogblue;
extern cvar_t gl_foggreen;
#endif

extern	int		gl_solid_format;
extern	int		gl_alpha_format;

// rendering cvar stuffs
extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel; // client cvar
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_lightmap;
extern	cvar_t	r_shadows;
extern	cvar_t	r_mirroralpha;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_novis;
extern	cvar_t	r_farclip;

// fenix@io.com: model interpolation
extern  cvar_t  r_interpolate_animation;
extern  cvar_t  r_interpolate_transform;
extern  cvar_t  r_interpolate_weapon;


// gl rendering cvars and stuff
extern	cvar_t	gl_clear;
extern	cvar_t	gl_cull;
extern	cvar_t	gl_poly;
extern	cvar_t	gl_texsort;
extern	cvar_t	gl_smoothmodels;
extern	cvar_t	gl_affinemodels;
extern	cvar_t	gl_polyblend;
extern	cvar_t	gl_keeptjunctions;
extern	cvar_t	gl_reporttjunctions;
extern	cvar_t	gl_flashblend;
extern	cvar_t	gl_nocolors;
extern	cvar_t	gl_doubleeyes;
extern	cvar_t	gl_xflip;
extern	cvar_t	gl_playermip;
extern	cvar_t	gl_fullbright;
extern	cvar_t	r_ringalpha;





extern	float	r_world_matrix[16];

extern	const char *gl_vendor;
extern	const char *gl_renderer;
extern	const char *gl_version;
extern	const char *gl_extensions;





// gl_warp.c
void GL_SubdivideSurface (msurface_t *);
void EmitBothSkyLayers (msurface_t *);
void EmitWaterPolys (msurface_t *);
void EmitSkyPolys (msurface_t *);
void R_DrawSkyChain (msurface_t *);
void R_Sky_NewMap (void);

#ifdef SUPPORTS_SKYBOX
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_LoadSkys (char *skybox);
#endif


// gl_draw.c
void GL_Set2D (void);
void Draw_Crosshair(void);
void Build_Gamma_Table (void);
void SmoothFontSet(qboolean smoothfont_choice);

// gl_rmain.c
qboolean R_CullBox (vec3_t, vec3_t);
#ifdef SUPPORTS_AUTOID_HARDWARE
qboolean R_CullSphere (vec3_t centre, float radius);
#endif

void R_RotateForEntity (entity_t *);
#ifdef SUPPORTS_ENHANCED_GAMMA // WINDOWS + GLQUAKE only
void R_PolyBlend (void);
void R_BrightenScreen (void);
#endif

// gl_rlight.c
void R_MarkLights (dlight_t *, int, mnode_t *);
void R_AnimateLight (void);
void R_RenderDlights (void);
int R_LightPoint (vec3_t p);

// gl_refrag.c
void  R_StoreEfrags (efrag_t **);

// gl_mesh.c
void GL_MakeAliasModelDisplayLists (aliashdr_t *);

// gl_rsurf.c
void R_RenderBrushPoly (msurface_t *fa);
void R_DrawBrushModel (entity_t *e);
void R_DrawWorld (void);
void R_DrawWaterSurfaces (void);
void R_MarkLeaves (void);
void R_MirrorChain (msurface_t *s);
void DrawGLPoly (glpoly_t *p);
void DrawGLWaterPoly (glpoly_t *p);
void DrawGLWaterPolyLightmap (glpoly_t *p);
void GL_BuildLightmaps (void);
texture_t *R_TextureAnimation (texture_t *base);

// gl_rmisc.c
void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
void R_TranslatePlayerSkin (int playernum);
void R_InitParticleTexture (void);

// gl_rpart.c

void R_InitParticles (void);
void R_DrawParticles (void);
void R_ClearParticles (void);

// gl_fullbright.c
void DrawFullBrightTextures (msurface_t *first_surf, int num_surfs);

// gl_screen.c
void R_PolyBlend (void);

// matrix.c - move it
void Mat_Update (void);


//vid_common_gl.c
#if !defined(MACOSX)
void GL_Init (void);
#endif

void GL_CheckTextureRAM (GLenum theTarget, GLint theLevel, GLint theInternalFormat, GLsizei theWidth, GLsizei theHeight, GLsizei theDepth , GLint theBorder, GLenum theFormat, GLenum theType);
void GL_SetupState (void);

void Check_GammaOld (unsigned char *pal);
//vid_wgl.c :( and may osx ... needs moved to ^^
qboolean VID_Is8bit (void);
qboolean CheckExtension (const char *extension);





// move this to vid_glcommon.c?
#ifdef SUPPORTS_ENHANCED_GAMMA
extern qboolean using_hwgamma; // Baker hw gamma support
#endif

extern	float	gldepthmin, gldepthmax;
extern	byte	color_white[4], color_black[4];

#endif
