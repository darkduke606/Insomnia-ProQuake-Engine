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
// r_surf.c: surface-related refresh code

#include "quakedef.h"

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#ifdef FITZQUAKE_PROTOCOL
#define MAX_LIGHTMAPS 256 //johnfitz -- was 64
#define MAX_NETQUAKE_LIGHTMAPS 64
#else
#define	MAX_LIGHTMAPS		64
#endif


int		lightmap_bytes;		// 1, 2, or 4

int		lightmap_textures;
unsigned		blocklights[18*18];
int			active_lightmaps;

typedef struct glRect_s {
	unsigned char l,t,w,h;
} glRect_t;

static glpoly_t	*lightmap_polys[MAX_LIGHTMAPS];
static qboolean	lightmap_modified[MAX_LIGHTMAPS];
static glRect_t	lightmap_rectchange[MAX_LIGHTMAPS];

static int			allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
byte		lightmaps[4*MAX_LIGHTMAPS*BLOCK_WIDTH*BLOCK_HEIGHT];

// For gl_texsort 0
msurface_t  *skychain = NULL;
msurface_t  *waterchain = NULL;

void R_RenderDynamicLightmaps (msurface_t *fa);

/*
================
DrawGLPoly
================
*/
void DrawGLPoly (glpoly_t *p)
{
	int		i;
	float	*v;

	glBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f (v[3], v[4]);
		glVertex3fv (v);
	}
	glEnd ();
}






































/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int			lnum, i, smax, tmax, s, t, sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	mtexinfo_t	*tex;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		rad = cl_dlights[lnum].radius;
		dist = DotProduct (cl_dlights[lnum].origin, surf->plane->normal) -
				surf->plane->dist;
		rad -= fabsf(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = cl_dlights[lnum].origin[i] -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		for (t = 0 ; t<tmax ; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
					blocklights[t*smax + s] += (rad - dist)*256;
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
extern cvar_t gl_overbright;
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride)
{
	int			smax, tmax, t, i, j, size, maps;
	byte		*lightmap;
	unsigned	scale, *bl;

	surf->cached_dlight = (surf->dlightframe == r_framecount);

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	lightmap = surf->samples;

// set to full bright if no light data
	if (r_fullbright.value || !cl.worldmodel->lightdata)
	{
		for (i=0 ; i<size ; i++)
			blocklights[i] = 255*256;
		goto store;
	}

// clear to no light
	for (i=0 ; i<size ; i++)
		blocklights[i] = 0;

// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;	// 8.8 fraction
			for (i=0 ; i<size ; i++)
				blocklights[i] += lightmap[i] * scale;
			lightmap += size;	// skip to next lightmap
		}

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights (surf);

// bound, invert, and shift
store:
	switch (gl_lightmap_format)
	{
	case GL_RGBA:
		stride -= (smax<<2);
		bl = blocklights;
		for (i=0 ; i<tmax ; i++, dest += stride)
		{
			for (j=0 ; j<smax ; j++)
			{
				t = *bl++;
				t >>= 7;
				if (t > 255)
					t = 255;
#ifdef D3DQ_WORKAROUND
				// Really??????
				dest[3] = 255-t;
#else
				dest[3] = 255-gammatable[t];	// JPG 3.02 - t -> gammatable[t]
#endif
				dest += 4;
			}
		}
		break;
//	case GL_ALPHA:
//	case GL_INTENSITY:
	case GL_LUMINANCE:

		bl = blocklights;
		for (i=0 ; i<tmax ; i++, dest += stride)
		{
			for (j=0 ; j<smax ; j++)
			{
#ifdef SUPPORTS_GL_OVERBRIGHTS // DX8QUAKE
				if (gl_overbright.value) {
					t = *bl++;
					t >>= 8;
					if (t > 255)
						t = 255;
					dest[j] = t; 
				} 
				else
#endif
				{
					t = *bl++;
					t >>= 7;
					if (t > 255)
						t = 255;
					// Baker: if hardware gamma shouldn't this go?
					dest[j] = 255-gammatable[t];	// JPG 3.02 - t -> gammatable[t]
				}

			}
		}
		break;
	default:
		Sys_Error ("Bad lightmap format");
	}
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base)
{
	int	relative, count;

	if (currententity->frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (!base->anim_total)
		return base;

	relative = (int)(cl.time*10) % base->anim_total;

	count = 0;
	while (base->anim_min > relative || base->anim_max <= relative)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}

/*
===============================================================================

	BRUSH MODELS

===============================================================================
*/

extern	int		solidskytexture;
extern	int		alphaskytexture;
extern	float	speedscale;		// for top sky and bottom sky

/*
===============
R_UploadLightmap -- uploads the modified lightmap to opengl if necessary

assumes lightmap texture is already bound
===============
*/
void R_UploadLightmap (int lmap)
{
	glRect_t *theRect;

	if (!lightmap_modified[lmap])
		return;

	lightmap_modified[lmap] = false;

	theRect = &lightmap_rectchange[lmap];
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, theRect->t, BLOCK_WIDTH, theRect->h, gl_lightmap_format, GL_UNSIGNED_BYTE, lightmaps+(lmap * BLOCK_HEIGHT + theRect->t) *BLOCK_WIDTH*lightmap_bytes);
	theRect->l = BLOCK_WIDTH;
	theRect->t = BLOCK_HEIGHT;
	theRect->h = 0;
	theRect->w = 0;
}

/*
================
R_BlendLightmaps
================
*/
void R_BlendLightmaps (void) {
	int			i, j;
	float		*v;
	glpoly_t	*p;
	glRect_t	*theRect;

	if (r_fullbright.value)
		return;
#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
	if (!gl_texsort.value)
		return;
#endif

	glDepthMask (GL_FALSE);		// GL_FALSE = 0  don't bother writing Z

	if (gl_lightmap_format == GL_LUMINANCE)
#ifdef SUPPORTS_GL_OVERBRIGHTS // DX8QUAKE
	if (gl_overbright.value)
		glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
	else
#endif
		glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

// Baker: everything except intensity and luminance are broke
//        and GLQuake uses luminance by default.
//	else if (gl_lightmap_format == GL_INTENSITY)
//	{
//		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//		glColor4f (0,0,0,1);
//		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	}

	if (!r_lightmap.value)
		glEnable (GL_BLEND);

	for (i=0 ; i<MAX_LIGHTMAPS ; i++) {
		p = lightmap_polys[i];
		if (!p)
			continue;

		GL_Bind(lightmap_textures+i);
	
		// BengtQuake uploads lightmap here

		R_UploadLightmap (i); // BengtQuake way

		for ( ; p ; p=p->chain) {
			// JPG - added r_waterwarp
			if ((p->flags & SURF_UNDERWATER) && r_waterwarp.value) {

				DrawGLWaterPolyLightmap (p);
			} else {
				glBegin (GL_POLYGON);
				v = p->verts[0];
				for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
				{
					glTexCoord2f (v[5], v[6]);
					glVertex3fv (v);
				}
				glEnd ();
			}
		}
	}

	glDisable (GL_BLEND);
	if (gl_lightmap_format == GL_LUMINANCE) {
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
// else if (gl_lightmap_format == GL_INTENSITY) {
//		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//		glColor4f (1,1,1,1);
//	}

	glDepthMask (GL_TRUE);	// GLTRUE = 1 back to normal Z buffering
}



/*
================
R_RenderDynamicLightmaps
Multitexture
================
*/
void R_RenderDynamicLightmaps (msurface_t *fa)
{
	int			maps, smax, tmax;
	byte		*base;
	glRect_t    *theRect;

	c_brush_polys++;

	if (fa->flags & ( SURF_DRAWSKY | SURF_DRAWTURB) )
		return;

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

#ifdef SUPPORTS_GL_OVERBRIGHTS // MH Overbrights
	// mh - overbrights - need to rebuild the lightmap if this changes
	if (fa->overbright != gl_overbright.value)
	{
		fa->overbright = gl_overbright.value;
		goto dynamic;
	}

#endif

	// check for lightmap modification
	for (maps = 0 ; maps < MAXLIGHTMAPS && fa->styles[maps] != 255 ; maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;

	if (fa->dlightframe == r_framecount || fa->cached_dlight) {	// dynamic this frame dynamic previously

dynamic:
		if (r_dynamic.value) {
			lightmap_modified[fa->lightmaptexturenum] = true;
			theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t) {
				if (theRect->h)
					theRect->h += theRect->t - fa->light_t;
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l) {
				if (theRect->w)
					theRect->w += theRect->l - fa->light_s;
				theRect->l = fa->light_s;
			}
			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;
			if (theRect->w + theRect->l < fa->light_s + smax)
				theRect->w = fa->light_s - theRect->l + smax;
			if (theRect->h + theRect->t < fa->light_t + tmax)
				theRect->h = fa->light_t - theRect->t + tmax;
			base = lightmaps + fa->lightmaptexturenum * lightmap_bytes * BLOCK_WIDTH * BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * lightmap_bytes + fa->light_s * lightmap_bytes;
			R_BuildLightMap (fa, base, BLOCK_WIDTH*lightmap_bytes);
		}
	}
}

/*
================
R_DrawWaterSurfaces
================
*/
void R_DrawWaterSurfaces (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;

	// JPG 3.20 - cheat protection, Baker 3.99: changed in event set higher than max value of 1	
	if ((r_wateralpha.value >= 1.0 || pq_cheatfree) 
#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
			&& gl_texsort.value
#endif 
			)
		return;


	// go back to the world matrix

    glLoadMatrixf (r_world_matrix);

	if (r_wateralpha.value < 1.0 && !pq_cheatfree) {	 // JPG 3.20 - cheat protection
		glEnable (GL_BLEND);
		glColor4f (1,1,1,r_wateralpha.value);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
	if (!gl_texsort.value) 
	{
		if (!waterchain)
			return;

		for ( s = waterchain ; s ; s=s->texturechain) {
			GL_Bind (s->texinfo->texture->gl_texturenum);
			EmitWaterPolys (s);
		}

		waterchain = NULL;
	} 
	else 
#endif
	{

		for (i=0 ; i<cl.worldmodel->numtextures ; i++)
		{
			t = cl.worldmodel->textures[i];
			if (!t)
				continue;
			s = t->texturechain;
			if (!s)
				continue;
			if ( !(s->flags & SURF_DRAWTURB ) )
				continue;

			// set modulate mode explicitly

			GL_Bind (t->gl_texturenum);

			for ( ; s ; s=s->texturechain)
				EmitWaterPolys (s);

			t->texturechain = NULL;
		}

	}

	if (r_wateralpha.value < 1.0 && !pq_cheatfree) {	// JPG 3.20 - cheat protection
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		glColor4f (1,1,1,1);
		glDisable (GL_BLEND);
	}
}



// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************
// *********** SYNC LOCKER *************



/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	texture_t	*t;

#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
	if (!gl_texsort.value) {
		GL_DisableMultitexture();

		if (skychain) {
			R_DrawSkyChain(skychain);
			skychain = NULL;
		}

		return;
	}
#endif

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];
		if (!t)
			continue;
		s = t->texturechain;
		if (!s)
			continue;
		if (i == skytexturenum)
			R_DrawSkyChain (s);
		else if (i == mirrortexturenum && r_mirroralpha.value < 1.0) // Baker 3.99: changed, max value is 1
		{
			R_MirrorChain (s);
			continue;
		}
		else
		{
			if ((s->flags & SURF_DRAWTURB) && r_wateralpha.value < 1.0 && !pq_cheatfree)	// JPG 3.20 - cheat protection, Baker 3.99: changed in event r_wateralpha is above max value of 1
				continue;	// draw translucent water later
			for ( ; s ; s=s->texturechain)
				R_RenderBrushPoly (s);
		}

		t->texturechain = NULL;
	}
}


#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
/*
================
R_DrawSequentialPoly

Systems that have fast state and texture changes can
just do everything as it passes with no need to sort
================
*/
void R_DrawSequentialPoly (msurface_t *s)
{
	glpoly_t	*p;
	float		*v;
	int			i;
	texture_t	*t;
	vec3_t		nv;
	glRect_t	*theRect;

// Begin D3DQuake
#ifdef D3DQ_WORKAROUND
int gNoSurfaces=0;
	if ( gNoSurfaces ) return;
// End D3DQuake
#endif // D3DQ_WORKAROUND

	//
	// normal lightmaped poly
	//

	// JPG - added r_waterwarp
	if (! (s->flags & (SURF_DRAWSKY|SURF_DRAWTURB) ) && (!r_waterwarp.value || !(s->flags & SURF_UNDERWATER)))
	{
		R_RenderDynamicLightmaps (s);
		if (gl_mtexable) {
			p = s->polys;

			t = R_TextureAnimation (s->texinfo->texture);
			// Binds world to texture env 0
			GL_SelectTexture(TEXTURE0_SGIS);
			GL_Bind (t->gl_texturenum);
//			BengtQuake uploads the lightmap here
			R_UploadLightmap (s->lightmaptexturenum);
			// BengtOverbright does this
//			GL_BeginLightBlend ();
//          instead of this next line
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			
			// Binds lightmap to texenv 1
			GL_EnableMultitexture(); // Same as SelectTexture (TEXTURE1)
			GL_Bind (lightmap_textures + s->lightmaptexturenum);
			R_UploadLightmap (s->lightmaptexturenum);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
			glBegin(GL_POLYGON);
			v = p->verts[0];
			for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
			{
				qglMTexCoord2fSGIS (TEXTURE0_SGIS, v[3], v[4]);
				qglMTexCoord2fSGIS (TEXTURE1_SGIS, v[5], v[6]);
				glVertex3fv (v);
			}
			glEnd ();
#ifdef NOFULLBRIGHT
			if (!gl_fullbright.value)
				return;
#endif
		} else {
			p = s->polys;

			t = R_TextureAnimation (s->texinfo->texture);
			GL_Bind (t->gl_texturenum);
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[3], v[4]);
				glVertex3fv (v);
			}
			glEnd ();

			GL_Bind (lightmap_textures + s->lightmaptexturenum);
			glEnable (GL_BLEND);
			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[5], v[6]);
				glVertex3fv (v);
			}
			glEnd ();

			glDisable (GL_BLEND);
		}
#ifndef NOFULLBRIGHT
		if (gl_fullbright.value) {
			// draw fullbright mask if appropriate
			if (t->fullbright == -1)
			    return;

			GL_DisableMultitexture ();
			glEnable (GL_BLEND);
			GL_Bind (t->fullbright);
			DrawGLPoly (s->polys);
			glDisable (GL_BLEND);
		}
#endif
		return;
	}

	// subdivided water surface warp

	if (s->flags & SURF_DRAWTURB)
	{
		GL_DisableMultitexture();
		GL_Bind (s->texinfo->texture->gl_texturenum);
		EmitWaterPolys (s);
		return;
	}

	// subdivided sky warp
	if (s->flags & SURF_DRAWSKY)
	{
		GL_DisableMultitexture();
		GL_Bind (solidskytexture);
		speedscale = realtime*8;
		speedscale -= (int)speedscale & ~127;

		EmitSkyPolys (s);

		glEnable (GL_BLEND);
		GL_Bind (alphaskytexture);
		speedscale = realtime*16;
		speedscale -= (int)speedscale & ~127;
		EmitSkyPolys (s);

		glDisable (GL_BLEND);
		return;
	}

	// underwater warped with lightmap
	R_RenderDynamicLightmaps (s);
	if (gl_mtexable) {
		p = s->polys;

		t = R_TextureAnimation (s->texinfo->texture);
		GL_SelectTexture(TEXTURE0_SGIS);
		GL_Bind (t->gl_texturenum);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		GL_EnableMultitexture();
		GL_Bind (lightmap_textures + s->lightmaptexturenum);
		
		R_UploadLightmap (s->lightmaptexturenum);
		
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		glBegin (GL_TRIANGLE_FAN);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
		{
			qglMTexCoord2fSGIS (TEXTURE0_SGIS, v[3], v[4]);
			qglMTexCoord2fSGIS (TEXTURE1_SGIS, v[5], v[6]);

			nv[0] = v[0] + 8*sin(v[1]*0.05+realtime)*sin(v[2]*0.05+realtime);
			nv[1] = v[1] + 8*sin(v[0]*0.05+realtime)*sin(v[2]*0.05+realtime);
			nv[2] = v[2];

			glVertex3fv (nv);
		}
		glEnd ();

	} else {
		p = s->polys;

		t = R_TextureAnimation (s->texinfo->texture);
		GL_Bind (t->gl_texturenum);
		DrawGLWaterPoly (p);

		GL_Bind (lightmap_textures + s->lightmaptexturenum);
		glEnable (GL_BLEND);
		DrawGLWaterPolyLightmap (p);
		glDisable (GL_BLEND);
	}
#ifndef NOFULLBRIGHT
	if (gl_fullbright.value) {
		// draw fullbright mask if appropriate
		if (t->fullbright == -1)
		    return;

		GL_DisableMultitexture ();
		glEnable (GL_BLEND);
		GL_Bind (t->fullbright);
		DrawGLWaterPoly (s->polys);
		glDisable (GL_BLEND);

		return;
	}
#endif
}
#endif

/*
================
DrawGLWaterPoly

Warp the vertex coordinates
================
*/
void DrawGLWaterPoly (glpoly_t *p) {
	int		i;
	float	*v;
	vec3_t	nv;

	GL_DisableMultitexture();

	glBegin (GL_TRIANGLE_FAN);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f (v[3], v[4]);

		nv[0] = v[0] + 8*sin(v[1]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[1] = v[1] + 8*sin(v[0]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[2] = v[2];

		glVertex3fv (nv);
	}
	glEnd ();
}

void DrawGLWaterPolyLightmap (glpoly_t *p)
{
	int		i;
	float	*v;
	vec3_t	nv;

	GL_DisableMultitexture();

	glBegin (GL_TRIANGLE_FAN);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f (v[5], v[6]);

		nv[0] = v[0] + 8*sin(v[1]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[1] = v[1] + 8*sin(v[0]*0.05+realtime)*sin(v[2]*0.05+realtime);
		nv[2] = v[2];

		glVertex3fv (nv);
	}
	glEnd ();
}


/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa)
{
	texture_t	*t;
	byte		*base;
	int			maps;
	glRect_t    *theRect;
	int smax, tmax;

	c_brush_polys++;

	if (fa->flags & SURF_DRAWSKY)
	{	// warp texture, no lightmaps
		EmitBothSkyLayers (fa);
		return;
	}

	t = R_TextureAnimation (fa->texinfo->texture);
	GL_Bind (t->gl_texturenum);

	if (fa->flags & SURF_DRAWTURB)
	{	// warp texture, no lightmaps
		EmitWaterPolys (fa);
		return;
	}

	if ((fa->flags & SURF_UNDERWATER) && r_waterwarp.value)		// JPG - added r_waterwarp
		DrawGLWaterPoly (fa->polys);
	else
		DrawGLPoly (fa->polys);

#ifndef NOFULLBRIGHT
	if (gl_fullbright.value)
		fa->draw_this_frame = 1;
#endif
	// add the poly to the proper lightmap chain

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;
#ifdef SUPPORTS_GL_OVERBRIGHTS
	// mh - overbrights - need to rebuild the lightmap if this changes
	if (fa->overbright != gl_overbright.value)
	{
		fa->overbright = gl_overbright.value;
		goto dynamic;
	}
#endif

	// check for lightmap modification
	for (maps = 0 ; maps < MAXLIGHTMAPS && fa->styles[maps] != 255 ;
		 maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;

	if (fa->dlightframe == r_framecount	// dynamic this frame
		|| fa->cached_dlight)			// dynamic previously
	{
dynamic:
		if (r_dynamic.value&& !r_fullbright.value) // Bengt: added if no fullbrights
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t) {
				if (theRect->h)
					theRect->h += theRect->t - fa->light_t;
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l) {
				if (theRect->w)
					theRect->w += theRect->l - fa->light_s;
				theRect->l = fa->light_s;
			}
			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
				theRect->w = (fa->light_s-theRect->l)+smax;
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
				theRect->h = (fa->light_t-theRect->t)+tmax;
			base = lightmaps + fa->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * lightmap_bytes + fa->light_s * lightmap_bytes;
			R_BuildLightMap (fa, base, BLOCK_WIDTH*lightmap_bytes);
		}
	}
}



/*
================
R_MirrorChain
================
*/
void R_MirrorChain (msurface_t *s)
{
	if (mirror)
		return;
	mirror = true;
	mirror_plane = s->plane;
}


#if 0
void GL_PolygonOffset (int offset)
{
	if (offset > 0)
	{
		glEnable (GL_POLYGON_OFFSET_FILL);
		glEnable (GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(1, offset);
	}
	else if (offset < 0)
	{
		glEnable (GL_POLYGON_OFFSET_FILL);
		glEnable (GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1, offset);
	}
	else
	{
		glDisable (GL_POLYGON_OFFSET_FILL);
		glDisable (GL_POLYGON_OFFSET_LINE);
	}
}
#endif

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *ent)
{
	int			i, k;
	float		dot;
	vec3_t		mins, maxs;
	msurface_t	*psurf;
	mplane_t	*pplane;
	model_t		*clmodel = ent->model;
	qboolean	rotated;

	currenttexture = -1;

	if (ent->angles[0] || ent->angles[1] || ent->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++) 
		{
			mins[i] = ent->origin[i] - clmodel->radius;
			maxs[i] = ent->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (ent->origin, clmodel->mins, mins);
		VectorAdd (ent->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

#ifdef SUPPORTS_ENTITY_ALPHA
	if (ISTRANSPARENT(ent)) 
	{
		glEnable (GL_BLEND);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f (1, 1, 1, ent->transparency);
	} 
	else 
#endif
	{
		glColor3f (1,1,1);
	}
	memset (lightmap_polys, 0, sizeof(lightmap_polys));

	VectorSubtract (r_refdef.vieworg, ent->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp, forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (ent->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

	// calculate dynamic lighting for bmodel if it's not an instanced model
	if (clmodel->firstmodelsurface != 0 && !gl_flashblend.value)
	{
		for (k=0 ; k<MAX_DLIGHTS ; k++)
		{
			if (cl_dlights[k].die < cl.time || !cl_dlights[k].radius)
				continue;

			R_MarkLights (&cl_dlights[k], 1<<k, clmodel->nodes + clmodel->hulls[0].firstclipnode);
		}
	}

    glPushMatrix ();
	ent->angles[0] = -ent->angles[0];	// stupid quake bug
	R_RotateForEntity (ent);
	ent->angles[0] = -ent->angles[0];	// stupid quake bug


	// draw texture
	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;
		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
			if (!gl_texsort.value)
				R_DrawSequentialPoly (psurf);
			else
#endif
				R_RenderBrushPoly (psurf);
		}
	}

	R_BlendLightmaps ();

	if (gl_fullbright.value)
		DrawFullBrightTextures (clmodel->surfaces, clmodel->numsurfaces);

	glPopMatrix ();

#ifdef SUPPORTS_ENTITY_ALPHA
	if (ISTRANSPARENT(ent))
	{
		glColor3ubv (color_white);
		glDisable (GL_BLEND);
	}
#endif
}

/*
===============================================================================

	WORLD MODEL

===============================================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	if (R_CullBox (node->minmaxs, node->minmaxs+3))
		return;

// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		pleaf = (mleaf_t *)node;
		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

	// deal with model fragments in this leaf
		if (pleaf->efrags)
			R_StoreEfrags (&pleaf->efrags);

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	side = (dot >= 0) ? 0 : 1;

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);

// draw stuff
	c = node->numsurfaces;

	if (c)
	{
		if (dot < 0 -BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;

		{
#ifdef DX8QUAKE_NO_GL_TEXSORT_ZERO
			for (surf = cl.worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
			{
				if (surf->visframe != r_framecount)
					continue;

				if (((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
					continue;		// wrong side

				// if sorting by texture, just store it out
				surf->texturechain = surf->texinfo->texture->texturechain;
				surf->texinfo->texture->texturechain = surf;
			}

#else
			for (surf = cl.worldmodel->surfaces + node->firstsurface ; c ; c--, surf++)
			{
				if (surf->visframe != r_framecount)
					continue;

				// don't backface underwater surfaces, because they warp // JPG - added r_waterwarp
				if ( (!(surf->flags & SURF_UNDERWATER) || !r_waterwarp.value) && ( (dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)) )
					continue;		// wrong side

				// if sorting by texture, just store it out
				if (gl_texsort.value) {
					if (!mirror || surf->texinfo->texture != cl.worldmodel->textures[mirrortexturenum]) {
						surf->texturechain = surf->texinfo->texture->texturechain;
						surf->texinfo->texture->texturechain = surf;
					}
				} else if (surf->flags & SURF_DRAWSKY) {
					surf->texturechain = skychain;
					skychain = surf;
				} else if (surf->flags & SURF_DRAWTURB) {
					surf->texturechain = waterchain;
					waterchain = surf;
				} else
					R_DrawSequentialPoly (surf);
				}
#endif
		}
	}

// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}

/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;

	memset (&ent, 0, sizeof(ent));
	ent.model = cl.worldmodel;

	VectorCopy (r_refdef.vieworg, modelorg);

	currententity = &ent;
	currenttexture = -1;

	glColor3f (1,1,1);
	memset (lightmap_polys, 0, sizeof(lightmap_polys));
#ifdef SUPPORTS_SKYBOX
	R_ClearSkyBox ();
#endif

	R_RecursiveWorldNode (cl.worldmodel->nodes);

	DrawTextureChains ();

	R_BlendLightmaps ();

#ifndef NOFULLBRIGHT
	if (gl_fullbright.value)
		DrawFullBrightTextures (cl.worldmodel->surfaces, cl.worldmodel->numsurfaces);
#endif

#ifdef SUPPORTS_SKYBOX
	R_DrawSkyBox ();
#endif
}

/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	int		i;
	byte	*vis, solid[4096];
	mnode_t	*node;
#ifdef SUPPORTS_GLHOMFIX_NEARWATER
	extern	cvar_t gl_nearwater_fix;
	msurface_t **mark;
	qboolean   nearwaterportal = false;

	// Check if near water to avoid HOMs when crossing the surface
	if (gl_nearwater_fix.value)
		for (i=0, mark = r_viewleaf->firstmarksurface; i < r_viewleaf->nummarksurfaces; i++, mark++)
		{
			if ((*mark)->flags & SURF_DRAWTURB)
			{
				nearwaterportal = true;
				//	Con_SafePrintf ("R_MarkLeaves: nearwaterportal, surfs=%d\n", r_viewleaf->nummarksurfaces);
				break;
			}
		}
#endif

	if (r_oldviewleaf == r_viewleaf && ((!r_novis.value && !nearwaterportal) || pq_cheatfree))	// JPG 3.20 - cheat protection
		return;

	if (mirror)
		return;

	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	if (r_novis.value && !pq_cheatfree)		// JPG 3.20 - cheat protection
	{
		vis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs+7)>>3);
#ifdef SUPPORTS_GLHOMFIX_NEARWATER
	}
	else if (nearwaterportal)
	{
//		extern byte *SV_FatPVS (vec3_t org, model_t *worldmodel);
		vis = SV_FatPVS (r_origin, cl.worldmodel);
#endif
	} 
	else 
	{
		vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);
	}

	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}

/*
===============================================================================

  LIGHTMAP ALLOCATION

===============================================================================
*/

// returns a texture number and the position inside it
static int AllocBlock (int w, int h, int *x, int *y)
{
	int	i, j, best, best2, texnum;

	for (texnum=0 ; texnum<MAX_LIGHTMAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (allocated[texnum][i+j] >= best)
					break;
				if (allocated[texnum][i+j] > best2)
					best2 = allocated[texnum][i+j];
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
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("AllocBlock: full");
	return 0;
}

mvertex_t	*r_pcurrentvertbase;
model_t		*currentmodel;

int	nColinElim;

/*
================
BuildSurfaceDisplayList
================
*/
static void BuildSurfaceDisplayList (msurface_t *fa)
{
	int			i, lindex, lnumverts, vertpage;
	float		*vec, s, t;
	medge_t		*pedges, *r_pedge;
	glpoly_t	*poly;

// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	// draw texture
	poly = Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}

		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		// lightmap texture coordinates
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}
#if !defined(DX8QUAKE_NO_GL_KEEPTJUNCTIONS_ZERO)
	// remove co-linear points - Ed
	if (!gl_keeptjunctions.value && (!(fa->flags & SURF_UNDERWATER) && !r_waterwarp.value)) {
		for (i = 0 ; i < lnumverts ; ++i) 
		{
			vec3_t v1, v2;
			float *prev, *this, *next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			this = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract( this, prev, v1 );
			VectorNormalize( v1 );
			VectorSubtract( next, prev, v2 );
			VectorNormalize( v2 );

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001
			if ((fabsf( v1[0] - v2[0] ) <= COLINEAR_EPSILON) &&
				(fabsf( v1[1] - v2[1] ) <= COLINEAR_EPSILON) &&
				(fabsf( v1[2] - v2[2] ) <= COLINEAR_EPSILON))
			{
				int j;
				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;
					for (k = 0; k < VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}
				--lnumverts;
				++nColinElim;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
#endif
	poly->numverts = lnumverts;
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
static void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	surf->lightmaptexturenum = AllocBlock (smax, tmax, &surf->light_s, &surf->light_t);
	base = lightmaps + surf->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;
	R_BuildLightMap (surf, base, BLOCK_WIDTH*lightmap_bytes);
}

/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps (void)
{
	int		i, j;
	model_t	*m;
	extern qboolean isPermedia;
#ifdef MACOSX_EXTRA_FEATURES
        extern qboolean	gl_luminace_lightmaps;
#endif /* MACOSX */

	memset (allocated, 0, sizeof(allocated));

	r_framecount = 1;		// no dlightcache

	if (!lightmap_textures)
	{
		lightmap_textures = texture_extension_number;
		texture_extension_number += MAX_LIGHTMAPS;
	}

#ifdef MACOSX_EXTRA_FEATURES

    // <AWE> MacOS X v10.1, GLQuake v1.0.2:
    // Using GL_LUMINACE causes frame drops in rooms with many lightmap textures.
    // Since GL_INTENSITY, GL_ALPHA result in totaly dark surfaces, we have to use GL_RGBA as default.

    if (gl_luminace_lightmaps == false)
        gl_lightmap_format = GL_RGBA;
    else

#endif /* MACOSX */
		gl_lightmap_format = GL_LUMINANCE;

	// default differently on the Permedia
	if (isPermedia)
		gl_lightmap_format = GL_RGBA;

	if (COM_CheckParm ("-lm_1"))
		gl_lightmap_format = GL_LUMINANCE;
// Baker these other lightmap formats simply don't work
//	if (COM_CheckParm ("-lm_a"))
//		gl_lightmap_format = GL_ALPHA;
//	if (COM_CheckParm ("-lm_i"))
//		gl_lightmap_format = GL_INTENSITY;
//	if (COM_CheckParm ("-lm_2"))
//		gl_lightmap_format = GL_RGBA4;
	if (COM_CheckParm ("-lm_4"))
		gl_lightmap_format = GL_RGBA;

	switch (gl_lightmap_format)
	{
	case GL_RGBA:
		lightmap_bytes = 4;
		break;
//	case GL_RGBA4:
//		lightmap_bytes = 2;
//		break;
//	case GL_INTENSITY:
//	case GL_ALPHA:
	case GL_LUMINANCE:
		lightmap_bytes = 1;
		break;
	}

	for (j=1 ; j<MAX_MODELS ; j++) 
	{
		m = cl.model_precache[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		r_pcurrentvertbase = m->vertexes;
		currentmodel = m;
		for (i=0 ; i<m->numsurfaces ; i++)
		{
			GL_CreateSurfaceLightmap (m->surfaces + i);
			if ( m->surfaces[i].flags & SURF_DRAWTURB )
				continue;
#ifndef QUAKE2
			if ( m->surfaces[i].flags & SURF_DRAWSKY )
				continue;
#endif
			BuildSurfaceDisplayList (m->surfaces + i);
		}
	}

#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
 	if (!gl_texsort.value)
 		GL_SelectTexture(TEXTURE1_SGIS);
#endif

	// upload all lightmaps that were filled
	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		if (!allocated[i][0])
			break;		// no more used
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		GL_Bind(lightmap_textures + i);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef MACOSX_TEXRAM_CHECK
                GL_CheckTextureRAM (GL_TEXTURE_2D, 0, lightmap_bytes, BLOCK_WIDTH, BLOCK_HEIGHT, 0, 0, gl_lightmap_format, GL_UNSIGNED_BYTE);
#endif /* MACOSX */
		glTexImage2D (GL_TEXTURE_2D, 0, lightmap_bytes, BLOCK_WIDTH, BLOCK_HEIGHT, 0, gl_lightmap_format, GL_UNSIGNED_BYTE, lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*lightmap_bytes);
	}

#if !defined(DX8QUAKE_NO_GL_TEXSORT_ZERO)
 	if (!gl_texsort.value)
 		GL_SelectTexture(TEXTURE0_SGIS);
#endif

}
