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
// gl_rmain.c

#include "quakedef.h"

entity_t	r_worldentity;

qboolean r_cache_thrash;		// compatibility

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

int			particletexture;	// little dot for particles
int			playertextures;		// up to 16 color translated skins
qboolean	envmap;				// true during envmap command capture

int			currenttexture = -1;		// to avoid unnecessary texture sets

int			cnttextures[2] = {-1, -1};     // cached

int		skyboxtextures;

int			mirrortexturenum;	// quake texturenum, not gltexturenum
qboolean	mirror;
mplane_t	*mirror_plane;

// view origin
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

// screen size info
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value



cvar_t	r_norefresh = {"r_norefresh","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1", true};  // Baker 3.80x - Save to config
cvar_t	r_ringalpha = {"r_ringalpha", "0.4", true}; // Baker 3.80x - gl_ringalpha
cvar_t	r_truegunangle = {"r_truegunangle","0", true};  // Baker 3.80x - Optional "true" gun positioning on viewmodel
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_lightmap = {"r_lightmap","0"};
cvar_t	r_shadows = {"r_shadows","0"};
cvar_t	r_mirroralpha = {"r_mirroralpha","1"};
cvar_t	r_wateralpha = {"r_wateralpha","1",true};
cvar_t	r_dynamic = {"r_dynamic","1"};
cvar_t	r_novis = {"r_novis","0"};
cvar_t  r_interpolate_animation = {"r_interpolate_animation", "0", true};
cvar_t  r_interpolate_transform = {"r_interpolate_transform", "0", true};
cvar_t  r_interpolate_weapon = {"r_interpolate_weapon", "0", true};
cvar_t	r_clearcolor = {"r_clearcolor", "0"};
cvar_t	r_farclip = {"r_farclip","16384",true};
cvar_t	gl_nearwater_fix = {"gl_nearwater_fix","1",true};
cvar_t	gl_fadescreen_alpha = {"gl_fadescreen_alpha","0.7",true};

cvar_t	gl_clear = {"gl_clear","0"};
cvar_t	gl_cull = {"gl_cull","1"};
cvar_t	gl_texsort = {"gl_texsort","1"};
cvar_t	gl_smoothmodels = {"gl_smoothmodels","1"};
cvar_t	gl_affinemodels = {"gl_affinemodels","0"};
cvar_t	gl_polyblend = {"gl_polyblend","1", true};
cvar_t	gl_flashblend = {"gl_flashblend","1", true};
cvar_t	gl_playermip = {"gl_playermip","0", true};
cvar_t	gl_nocolors = {"gl_nocolors","0"};
cvar_t	gl_finish = {"gl_finish","0"};

cvar_t  gl_fullbright = {"gl_fullbright","0", true};
#ifdef SUPPORTS_GL_OVERBRIGHTS
cvar_t  gl_overbright = {"gl_overbright","0", true};
#endif

#ifdef MACOSX
cvar_t	gl_keeptjunctions = {"gl_keeptjunctions","1"};
#else
cvar_t	gl_keeptjunctions = {"gl_keeptjunctions","0"};
#endif /* MACOSX */

cvar_t	gl_reporttjunctions = {"gl_reporttjunctions","0"};
cvar_t	gl_doubleeyes = {"gl_doubleeyes", "1"};				// JPG 3.02 - fixed spelling

#ifdef SUPPORTS_XFLIP
cvar_t gl_xflip = {"gl_xflip", "0"}; //Atomizer - GL_XFLIP
#endif

#ifdef SUPPORTS_SKYBOX
cvar_t r_oldsky = {"r_oldsky", "1"}; // set the skybox to on, and save the value in the config file (true)
#endif

#ifdef SUPPORTS_FOG
// NATAS - BramBo - Fog Code
cvar_t gl_fogenable = {"gl_fogenable", "0"};
cvar_t gl_fogstart = {"gl_fogstart", "50.0"};
cvar_t gl_fogend = {"gl_fogend", "800.0"};
cvar_t gl_fogdensity = {"gl_fogdensity", "0.8"};
cvar_t gl_fogred = {"gl_fogred","0.6"};
cvar_t gl_foggreen = {"gl_foggreen","0.5"};
cvar_t gl_fogblue = {"gl_fogblue","0.4"};
cvar_t gl_fogalpha = {"gl_fogalpha", "0.5"};
// END
#endif

extern	cvar_t	gl_ztrick;



/*
=================
R_CullBox

Returns true if the box is completely outside the frustum
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BoxOnPlaneSide (mins, maxs, &frustum[i]) == 2)
			return true;

	return false;
}

#ifdef SUPPORTS_AUTOID_HARDWARE
/*
=================
R_CullSphere

Returns true if the sphere is completely outside the frustum
=================
*/
qboolean R_CullSphere (vec3_t centre, float radius)
{
	int			i;
	mplane_t	*p;

	for (i = 0, p = frustum ; i < 4 ; i++, p++)
		if (PlaneDiff(centre, p) <= -radius)
			return true;

	return false;
}
#endif

void R_RotateForEntity (entity_t *ent)
{
    glTranslatef (ent->origin[0],  ent->origin[1],  ent->origin[2]);

    glRotatef (ent->angles[1],  0, 0, 1);
    glRotatef (-ent->angles[0],  0, 1, 0);
    glRotatef (ent->angles[2],  1, 0, 0);
}

#if 0
/*
=============
R_BlendedRotateForEntity

fenix@io.com: model transform interpolation
=============
*/
void R_BlendedRotateForEntity (entity_t *ent)
{
	int		i;	
	float	blend, timepassed;
	vec3_t	d;

	// positional interpolation
	timepassed = cl.time - ent->translate_start_time;

	if (ent->translate_start_time == 0  || timepassed > 1) 
	{
		ent->translate_start_time = cl.time;
		VectorCopy (ent->origin, ent->lastorigin);
		VectorCopy (ent->origin, ent->currorigin);
	}

	if (!VectorCompare (ent->origin, ent->currorigin)) 
	{
		ent->translate_start_time = cl.time;
		VectorCopy (ent->currorigin, ent->lastorigin);
		VectorCopy (ent->origin,  ent->currorigin);
		blend = 0;
	}
	else
	{
		blend = timepassed / 0.1;
		if (cl.paused || blend > 1)
			blend = 1;
	}

	VectorSubtract (ent->currorigin, ent->lastorigin, d);

	glTranslatef (ent->lastorigin[0] + (blend * d[0]), ent->lastorigin[1] + (blend * d[1]), ent->lastorigin[2] + (blend * d[2]));

	// orientation interpolation (Euler angles, yuck!)
	timepassed = cl.time - ent->rotate_start_time;

	if (ent->rotate_start_time == 0 || timepassed > 1) 
	{
		ent->rotate_start_time = cl.time;
		VectorCopy (ent->angles, ent->lastangles);
		VectorCopy (ent->angles, ent->currangles);
	}

	if (!VectorCompare (ent->angles, ent->currangles)) 
	{
		ent->rotate_start_time = cl.time;
		VectorCopy (ent->currangles, ent->lastangles);
		VectorCopy (ent->angles,  ent->currangles);
		blend = 0;
	}
	else
	{
		blend = timepassed / 0.1;
		if (cl.paused || blend > 1)
			blend = 1;
	}

	VectorSubtract (ent->currangles, ent->lastangles, d);

	// always interpolate along the shortest path
	for (i = 0; i < 3; i++)  
	{
		if (d[i] > 180)
			d[i] -= 360;
		else if (d[i] < -180)
			d[i] += 360;
	}

	glRotatef ( ent->lastangles[1] + ( blend * d[1]),  0, 0, 1);
	glRotatef (-ent->lastangles[0] + (-blend * d[0]),  0, 1, 0);
	glRotatef ( ent->lastangles[2] + ( blend * d[2]),  1, 0, 0);
}
#endif

/*
===============================================================================

  SPRITE MODELS

===============================================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	int		i, numframes, frame;
	float		*pintervals, fullinterval, targettime, time;
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;

	psprite = currententity->model->cache.data;
	frame = currententity->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}

/*
=================
R_DrawSpriteModel
=================
*/
void R_DrawSpriteModel (entity_t *ent)
{
	vec3_t		point, v_forward, v_right, v_up;
	mspriteframe_t	*frame;
	float		*up, *right;

	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (ent);
	psprite = currententity->model->cache.data;

	if (psprite->type == SPR_ORIENTED) {
		// bullet marks on walls
		AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	} else {
		// normal sprite
		up = vup;
		right = vright;
	}

	glColor3f (1,1,1);

	GL_DisableMultitexture();

    GL_Bind(frame->gl_texturenum);

	glEnable (GL_ALPHA_TEST);
	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (ent->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (ent->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (ent->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (ent->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);

	glEnd ();

	glDisable (GL_ALPHA_TEST);
}

/*
===============================================================================

  ALIAS MODELS

===============================================================================
*/


#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	shadevector;
float	shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];

int	lastposenum;
int lastposenum0;  // Interpolation

/*
=============
GL_DrawAliasFrame
=============
*/

// Begin D3DQuake
int gNoAlias;
// End D3DQuake
void GL_DrawAliasFrame (aliashdr_t *paliashdr, int posenum)
{
	float	alpha;
	float 	l;
	trivertx_t	*verts;
	int		*order;
	int		count;

	// Baker 3.80x - Transparent weapon (invisibility ring option)
	alpha = (currententity == &cl.viewent) ? ((cl.items & IT_INVISIBILITY) ? (r_ringalpha.value < 1 ? r_ringalpha.value : 0) : (r_drawviewmodel.value ? 1: 0)) : 1;

	lastposenum = posenum;

// Begin D3DQuake
	if ( gNoAlias ) return;
// End D3DQuake

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;

	order = (int *)((byte *)paliashdr + paliashdr->commands);

	if (alpha < 1)
		glEnable (GL_BLEND);

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
		{
			glBegin (GL_TRIANGLE_STRIP);
		}

		do
		{
			// texture coordinates come from the draw list
			glTexCoord2f (((float *)order)[0], ((float *)order)[1]);
			order += 2;

			if (r_fullbright.value || !cl.worldmodel->lightdata)
				l = 1;
			else
				// normals and vertexes come from the frame list
				l = shadedots[verts->lightnormalindex] * shadelight;
#ifdef D3DQ_WORKAROUND
				if ( l > 1 ) l = 1; // Manually clamp
#endif
				// Baker 3.80x - This is no longer used
				// glColor3f (l, l, l);
				glColor4f (l, l, l, alpha); // Baker 3.80x - transparent weapon
				glVertex3f (verts->v[0], verts->v[1], verts->v[2]);

				verts++;
		} while (--count);

		glEnd ();
	}


	if (alpha < 1)
		glDisable (GL_BLEND);

}

/*
=============
 GL_DrawAliasBlendedFrame

fenix@io.com: model animation interpolation
=============
*/
void GL_DrawAliasBlendedFrame (aliashdr_t *paliashdr, int pose1, int pose2, float blend)
{
	float	alpha; // Baker 3.80x - added alpha for r_ringalpha
	float       l;
    trivertx_t* verts1;
    trivertx_t* verts2;
    int*        order;
    int         count;
    vec3_t      d;

	// Baker 3.80x - Transparent weapon (invisibility ring option)
	alpha = (currententity == &cl.viewent) ? ((cl.items & IT_INVISIBILITY) ? (r_ringalpha.value < 1 ? r_ringalpha.value : 0) : (r_drawviewmodel.value ? 1: 0)) : 1;

	lastposenum0 = pose1;
    lastposenum  = pose2;

    verts1  = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
    verts2  = verts1;

    verts1 += pose1 * paliashdr->poseverts;
    verts2 += pose2 * paliashdr->poseverts;

    order = (int *)((byte *)paliashdr + paliashdr->commands);

	if (alpha < 1)
		glEnable (GL_BLEND);

    for (;;)
	{
		// get the vertex count and primitive type
		count = *order++;

		if (!count) break;

		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
		{
			glBegin (GL_TRIANGLE_STRIP);
		}

		do {
			// texture coordinates come from the draw list
			glTexCoord2f (((float *)order)[0], ((float *)order)[1]);
			order += 2;

			// normals and vertexes come from the frame list
			// blend the light intensity from the two frames together
			d[0] = shadedots[verts2->lightnormalindex] - shadedots[verts1->lightnormalindex];

			l = shadelight * (shadedots[verts1->lightnormalindex] + (blend * d[0]));
			glColor3f (l, l, l);

			VectorSubtract(verts2->v, verts1->v, d);

			// blend the vertex positions from each frame together
			glColor4f (l, l, l, alpha); // Baker 3.80x - transparent weapon
			glVertex3f (verts1->v[0] + (blend * d[0]), verts1->v[1] + (blend * d[1]), verts1->v[2] + (blend * d[2]));

			verts1++;
			verts2++;
		} while (--count);

		glEnd ();
	}

	if (alpha < 1)
		glDisable (GL_BLEND);

}

/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GL_DrawAliasShadow (aliashdr_t *paliashdr, int posenum)
{
	trivertx_t	*verts;
	int		*order;
	vec3_t	point;
	float	height, lheight;
	int		count;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;
	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
		{
			glBegin (GL_TRIANGLE_STRIP);
		}

		do
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) glTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			point[0] = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point[1] = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point[2] = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
//			height -= 0.001;
			glVertex3fv (point);

			verts++;
		} while (--count);

		glEnd ();
	}
}

/*
=============
GL_DrawAliasBlendedShadow

fenix@io.com: model animation interpolation
=============
*/
void GL_DrawAliasBlendedShadow (aliashdr_t *paliashdr, int pose1, int pose2, entity_t* e)
{
	trivertx_t* verts1;
	trivertx_t* verts2;
	int*        order;
	vec3_t      point1;
	vec3_t      point2;
	vec3_t      d;
	float       height;
	float       lheight;
	int         count;
	float       blend;

	blend = (cl.time - e->frame_start_time) / e->frame_interval;

	if (blend > 1) blend = 1;

	lheight = e->origin[2] - lightspot[2];
	height  = 1.0 -lheight;

	verts1 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts2 = verts1;

	verts1 += pose1 * paliashdr->poseverts;
	verts2 += pose2 * paliashdr->poseverts;

	order = (int *)((byte *)paliashdr + paliashdr->commands);

	for (;;)
	{
		// get the vertex count and primitive type
		count = *order++;

		if (!count) break;

		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
		{
			glBegin (GL_TRIANGLE_STRIP);
		}

		do
		{
			order += 2;

			point1[0] = verts1->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point1[1] = verts1->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point1[2] = verts1->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point1[0] -= shadevector[0]*(point1[2]+lheight);
			point1[1] -= shadevector[1]*(point1[2]+lheight);

			point2[0] = verts2->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point2[1] = verts2->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point2[2] = verts2->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point2[0] -= shadevector[0]*(point2[2]+lheight);
			point2[1] -= shadevector[1]*(point2[2]+lheight);

			VectorSubtract(point2, point1, d);

			glVertex3f (point1[0] + (blend * d[0]), point1[1] + (blend * d[1]), height);

			verts1++;
			verts2++;
		} while (--count);

		glEnd ();
	}
}

/*
=================
R_SetupAliasFrame
=================
*/
void R_SetupAliasFrame (int frame, aliashdr_t *paliashdr)
{
	int				pose, numposes;
	float			interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / interval) % numposes;
	}

	GL_DrawAliasFrame (paliashdr, pose);
}

/*
 =================
R_SetupAliasBlendedFrame

fenix@io.com: model animation interpolation
=================
*/
void R_SetupAliasBlendedFrame (int frame, aliashdr_t *paliashdr, entity_t* e)
{
	int   pose;
	int   numposes;
	float blend;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		e->frame_interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / e->frame_interval) % numposes;
	}
	else
	{
// One tenth of a second is a good for most Quake animations.
// If the nextthink is longer then the animation is usually meant to pause
// (e.g. check out the shambler magic animation in shambler.qc). If its
// shorter then things will still be smoothed partly, and the jumps will be
// less noticable because of the shorter time. So, this is probably a good assumption.
		e->frame_interval = 0.1;
	}

	if (e->currpose != pose)
	{
		e->frame_start_time = cl.time;
		e->lastpose = e->currpose;
		e->currpose = pose;
		blend = 0;
	}
	else
	{
		blend = (cl.time - e->frame_start_time) / e->frame_interval;
	}

	// weird things start happening if blend passes 1
	if (cl.paused || blend > 1)
		blend = 1;

	GL_DrawAliasBlendedFrame (paliashdr, e->lastpose, e->currpose, blend);
}

/*
=================
R_DrawAliasModel
=================
*/
void R_DrawAliasModel (entity_t *ent)
{
	int			client_no;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel = ent->model;
	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	float		an;
	int			anim;

	qboolean	torch = false; // Flags is this model is a torch

	VectorAdd (ent->origin, clmodel->mins, mins);
	VectorAdd (ent->origin, clmodel->maxs, maxs);

	if (ent->angles[0] || ent->angles[1] || ent->angles[2])
	{
		if (R_CullSphere(ent->origin, clmodel->radius))
			return;
	}
	else if (R_CullBox(mins, maxs)) 
	{
		return;
	}

	VectorCopy (ent->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	// get lighting information

	ambientlight = shadelight = R_LightPoint (currententity->origin);

	// always give the gun some light
	if (ent == &cl.viewent && ambientlight < 24)
		ambientlight = shadelight = 24;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (currententity->origin, cl_dlights[lnum].origin, dist);
			add = cl_dlights[lnum].radius - VectorLength(dist);

			if (add > 0) {
				ambientlight += add;
				//ZOID models should be affected by dlights as well
				shadelight += add;
			}
		}
	}

	// clamp lighting so it doesn't overbright as much
	if (ambientlight > 128)
		ambientlight = 128;
	if (ambientlight + shadelight > 192)
		shadelight = 192 - ambientlight;

	// ZOID: never allow players to go totally black
	client_no = currententity - cl_entities;
	if (client_no >= 1 && client_no<=cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
		if (ambientlight < 8)
			ambientlight = shadelight = 8;

	// HACK HACK HACK -- no fullbright colors, so make torches full light
	if (!strcmp (clmodel->name, "progs/flame2.mdl") || !strcmp (clmodel->name, "progs/flame.mdl") || !strcmp (clmodel->name, "progs/bolt2.mdl"))	// JPG 3.20 - LG should be fullbright too
	{
		ambientlight = shadelight = 255;	// JPG 3.02 - was 256
		torch = true; // This model is a torch. KH
	}

	ambientlight = gammatable[(int) ambientlight];	// JPG 3.02 - gamma correction
	shadelight = gammatable[(int) shadelight];		// JPG 3.02 - gamma correction

	shadedots = r_avertexnormal_dots[((int)(ent->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	shadelight = shadelight / 200.0;

	an = ent->angles[1]/180*M_PI;
	shadevector[0] = cosf(-an);
	shadevector[1] = sinf(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	// locate the proper data
	paliashdr = (aliashdr_t *)Mod_Extradata (currententity->model);

	c_alias_polys += paliashdr->numtris;

	// draw all the triangles

	GL_DisableMultitexture();

    glPushMatrix ();

	R_RotateForEntity (ent);

	if (!strcmp (clmodel->name, "progs/eyes.mdl") && gl_doubleeyes.value) {
		// double size of eyes, since they are really hard to see in gl
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2] - (22 + 8));
		glScalef (paliashdr->scale[0]*2, paliashdr->scale[1]*2, paliashdr->scale[2]*2);
	} else {
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		glScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
	}

	anim = (int)(cl.time*10) & 3;
    GL_Bind(paliashdr->gl_texturenum[currententity->skinnum][anim]);


	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (currententity->colormap != vid.colormap && !gl_nocolors.value) {
		client_no = currententity - cl_entities;
		if (client_no >= 1 && client_no<=cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
		    GL_Bind(playertextures - 1 + client_no);
#if 0
		else {
			// Color the player
			for (client_no=1 ; client_no < (cl.maxclients+1) ; client_no++) {
//				Con_Printf("Colormap is %i : %i but mine is %i\n", i, cl_entities[i].colormap, currententity->colormap);
				Con_Printf("b2 ");
				Con_Printf("b2 ce model %s", currententity->model->name);
				Con_Printf("b2 cl model num %i is %s", client_no, cl_entities[client_no].model->name);
				if (cl_entities[client_no].colormap == currententity->colormap && currententity->model->name !=NULL && cl_entities[client_no].model->name != NULL && !strcmp(currententity->model->name, cl_entities[client_no].model->name)) {
					Con_Printf("b3 ");
					if (client_no >= 1 && client_no<=cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */) {
						Con_Printf("b4 ");
						Con_Printf("Match on %i %i and mine %i\n", client_no, cl_entities[client_no].colormap, currententity->colormap);
						Con_Printf("b5 ");
						GL_Bind(playertextures - 1 + client_no);
						Con_Printf("b6 break");
						break;
					} else {
						Con_Printf("b6b break ");
						break;
					}
				}
			}
			Con_Printf("cln %i", client_no);
		}
#endif
	}


	if (gl_smoothmodels.value)
		glShadeModel (GL_SMOOTH);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

       // fenix@io.com: model animation interpolation
	// Baker: r_interpolate_weapon here!
    if (r_interpolate_animation.value) {
//		if (client_no == 1)
//			Con_Debugf("Player entity 1 current num is %i %s\n", currententity, currententity->model->name);
//		if (client_no == 0)
//			Con_Debugf("Player entity 0 current num is %i %s\n", currententity, currententity->model->name);
		R_SetupAliasBlendedFrame (currententity->frame, paliashdr, currententity);
    } else {
		R_SetupAliasFrame (currententity->frame, paliashdr);
    }

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glShadeModel (GL_FLAT);
	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glPopMatrix ();

	// aguirRe ... no shadows if r_fullbright or unlit world
	if (r_shadows.value && !r_fullbright.value && cl.worldmodel->lightdata) 	{
		//
		// Test for models that we don't want to shadow. KH
		// Not a nice way to do it...
		//

		// Torches. Early-out to avoid the strcmp's. KH
		if (torch)
			return;
		// Grenades. KH
		if (!strcmp (clmodel->name, "progs/grenade.mdl"))
			return;
		// Lightning bolts. KH
		if (!strncmp (clmodel->name, "progs/bolt", 10))
			return;
		glPushMatrix ();
		R_RotateForEntity (ent);
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);

		// Baker 3.60 - interpolation plus flicker fix
		// Quick-fix issue with self-overlapping alias triangles.
		//glColor4f (0,0,0,0.5); // Original.
		// glColor4f (0.0f, 0.0f, 0.0f, 1.0f); // KH
		glColor4f (0.0f, 0.0f, 0.0f, 0.5f /* r_shadows.value */); // KH

		// fenix@io.com: model animation interpolation
		if (r_interpolate_animation.value) {
            GL_DrawAliasBlendedShadow (paliashdr, lastposenum0, lastposenum, currententity);
		} else {
			GL_DrawAliasShadow (paliashdr, lastposenum);
		}


		glEnable (GL_TEXTURE_2D);
		glDisable (GL_BLEND);
		glColor4f (1,1,1,1);
		glPopMatrix ();
	}
}



/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;
	void SortEntitiesByTransparency (void);

	if (!r_drawentities.value)
		return;

	// Baker: http://forums.inside3d.com/viewtopic.php?p=13458
	//        Transparent entities need sorted to ensure items behind transparent objects get drawn (z-buffer)

#ifdef SUPPORTS_ENTITY_ALPHA
	SortEntitiesByTransparency ();
#endif

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_alias:
			R_DrawAliasModel (currententity);
			break;

		case mod_brush:

				// Get rid of Z-fighting for textures by offsetting the
				// drawing of entity models compared to normal polygons.
				// (Only works if gl_ztrick is turned off)
				if(!gl_ztrick.value)
				{
					glEnable(GL_POLYGON_OFFSET_FILL);
				}

				R_DrawBrushModel (currententity);

				if(!gl_ztrick.value)
				{
					glDisable(GL_POLYGON_OFFSET_FILL);
				}

			break;

		default:
			break;
		}
	}

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;

		default:
			break;
		}
	}
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	float		ambient[4], diffuse[4];
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	int			ambientlight, shadelight;

	if (!r_drawviewmodel.value)
		return;

	if (chase_active.value && !pq_cheatfree)	// JPG 3.20 - added pq_cheatfree
		return;

	if (envmap)
		return;

	if (!r_drawentities.value)
		return;

	if (cl.items & IT_INVISIBILITY && r_ringalpha.value == 1.0f)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent;
	if (!currententity->model)
		return;

	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - VectorLength(dist);
		if (add > 0)
			ambientlight += add;
	}

	// JPG 3.02 - gamma correction
	if (ambientlight > 255)
		ambientlight = 255;
	if (shadelight > 255)
		shadelight = 255;

	ambientlight = gammatable[ambientlight];
	shadelight = gammatable[shadelight];

	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;

	// hack the depth range to prevent view model from poking into walls
	glDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	R_DrawAliasModel (currententity);

	glDepthRange (gldepthmin, gldepthmax);
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!v_blend[3])	// No blends ... get outta here
		return;

#ifdef SUPPORTS_ENHANCED_GAMMA
	// Baker hwgamma support
	if (using_hwgamma && vid_hwgamma_enabled && gl_hwblend.value) {
//		Con_Printf("gl_hwblend.value is %f\n",gl_hwblend.value);

		if (!vid_hwgamma_enabled) {// Hardware gamma unavailable
			return;
		}

		if (!gl_hwblend.value) {
			return;
		}

		glDisable (GL_ALPHA_TEST);
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);

//		Con_Printf("our blends are %i, %i, %i, %i\n", v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
		glColor4fv (v_blend);

		glBegin (GL_QUADS);
		glVertex2f (r_refdef.vrect.x, r_refdef.vrect.y);
		glVertex2f (r_refdef.vrect.x + r_refdef.vrect.width, r_refdef.vrect.y);
		glVertex2f (r_refdef.vrect.x + r_refdef.vrect.width, r_refdef.vrect.y + r_refdef.vrect.height);
		glVertex2f (r_refdef.vrect.x, r_refdef.vrect.y + r_refdef.vrect.height);
		glEnd ();

		glDisable (GL_BLEND);
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_ALPHA_TEST);

		glColor3ubv (color_white);
	} else
#endif // !d3dquake
	{ // Baker end hwgamma support
		if (!gl_polyblend.value)
			return;

		GL_DisableMultitexture();

		glDisable (GL_ALPHA_TEST);
		glEnable (GL_BLEND);
		glDisable (GL_DEPTH_TEST);
		glDisable (GL_TEXTURE_2D);

	    glLoadIdentity ();

	    glRotatef (-90,  1, 0, 0);	    // put Z going up
	    glRotatef (90,  0, 0, 1);	    // put Z going up

		glColor4fv (v_blend);

#ifdef D3DQ_WORKAROUND
	/* Work around GeForce D3D driver bug where drawing abutting
	 * triangles with the viewport set to something less than full screen
	 * causes a visible seam. Draw one triangle instead of one quad.
	 */
		glBegin (GL_TRIANGLES);
		glVertex3f (10, 400, 100);
		glVertex3f (10, -400, 100);
		glVertex3f (10, 0, -400);
		glEnd ();
#else
		glBegin (GL_QUADS);

		glVertex3f (10, 100, 100);
		glVertex3f (10, -100, 100);
		glVertex3f (10, -100, -100);
		glVertex3f (10, 100, -100);
		glEnd ();
#endif
		glDisable (GL_BLEND);
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_ALPHA_TEST);
	}

}

/*
================
R_BrightenScreen
================
*/
#ifdef SUPPORTS_ENHANCED_GAMMA
// baker hwgamma support - if disabled, this should not be called
void R_BrightenScreen (void)
{
	extern float vid_gamma;
	float		f;

	if (vid_hwgamma_enabled || v_contrast.value <= 1.0)
		return;

	f = QMIN(v_contrast.value, 3);
	f = pow(f, v_gamma.value);

// inf = 255 * pow((i + 0.5) / 255.5 * c, g) + 0.5;

	glDisable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glBlendFunc (GL_DST_COLOR, GL_ONE);

	glBegin (GL_QUADS);
	while (f > 1)
	{
		if (f >= 2)
			glColor3ubv (color_white);
		else
			glColor3f (f - 1, f - 1, f - 1);
		glVertex2f (0, 0);
		glVertex2f (vid.width, 0);
		glVertex2f (vid.width, vid.height);
		glVertex2f (0, vid.height);
		f *= 0.5;
	}
	glEnd ();

	glEnable (GL_TEXTURE_2D);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable (GL_BLEND);

	glColor3ubv (color_white);
}

// Baker end hwgamma support
#endif

static int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}

/*
===============
TurnVector

turn forward towards side on the plane defined by forward and side
if angle = 90, the result will be equal to side
assumes side and forward are perpendicular, and normalized
to turn away from side, use a negative angle
===============
*/
void TurnVector (vec3_t out, vec3_t forward, vec3_t side, float angle)
{
	float scale_forward, scale_side;

	scale_forward = cos (DEG2RAD(angle));
	scale_side = sin (DEG2RAD(angle));

	out[0] = scale_forward*forward[0] + scale_side*side[0];
	out[1] = scale_forward*forward[1] + scale_side*side[1];
	out[2] = scale_forward*forward[2] + scale_side*side[2];
}

/*
===============
R_SetFrustum
===============
*/
void R_SetFrustum (void)
{
	int i;

	TurnVector (frustum[0].normal, vpn, vright, r_refdef.fov_x/2 - 90); //left plane
	TurnVector (frustum[1].normal, vpn, vright, 90 - r_refdef.fov_x/2); //right plane
	TurnVector (frustum[2].normal, vpn, vup, 90 - r_refdef.fov_y/2); //bottom plane
	TurnVector (frustum[3].normal, vpn, vup, r_refdef.fov_y/2 - 90); //top plane

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{

// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
		Cvar_SetValueByRef  (&r_fullbright, 0);

	R_AnimateLight ();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	V_SetContentsColor (r_viewleaf->contents);

	V_CalcBlend ();


	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;
}


void MYgluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h, farclip;

	// set up viewpoint
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	x = r_refdef.vrect.x * glwidth/vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth/vid.width;
	y = (vid.height-r_refdef.vrect.y) * glheight/vid.height;
	y2 = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight/vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	glViewport (glx + x, gly + y2, w, h);
    screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
//	yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*180/M_PI;
    farclip = QMAX((int)r_farclip.value, 4096);
	MYgluPerspective (r_refdef.fov_y,  screenaspect,  4,  farclip); // 4096

	if (mirror)
	{
		if (mirror_plane->normal[2])
			glScalef (1, -1, 1);
		else
			glScalef (-1, 1, 1);
		glCullFace(GL_BACK);
	}
	else
		glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up


#ifdef SUPPORTS_XCLIP
    if (gl_xflip.value)  //Atomizer - GL_XFLIP
	{
		glScalef (1, -1, 1);
		glCullFace(GL_BACK);
	}
#endif

    glRotatef (-r_refdef.viewangles[2],  1, 0, 0);
    glRotatef (-r_refdef.viewangles[0],  0, 1, 0);
    glRotatef (-r_refdef.viewangles[1],  0, 0, 1);
    glTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	// set drawing parms
	if (gl_cull.value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

cvar_t r_waterwarp = {"r_waterwarp", "0", true}; // Baker 3.60 - Save this to config now

/*
===============
R_Init
===============
*/
void R_Init (void)
{
	extern cvar_t gl_finish;
	extern cvar_t r_truegunangle;
	extern cvar_t r_farclip;
	extern cvar_t r_ringalpha;
	extern cvar_t gl_fullbright;
	extern void R_Envmap_f (void);
	extern void R_SetClearColor_f (void);

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);
	Cmd_AddCommand ("envmap", R_Envmap_f);
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);

#ifdef SUPPORTS_SKYBOX
	Cmd_AddCommand ("sky", R_SkyCommand_f);
	Cmd_AddCommand ("loadsky", R_SkyCommand_f);
#endif

	Cvar_RegisterVariable (&r_norefresh, NULL);
	Cvar_RegisterVariable (&r_lightmap, NULL);
	Cvar_RegisterVariable (&r_fullbright, NULL);
	Cvar_RegisterVariable (&r_drawentities, NULL);
	Cvar_RegisterVariable (&r_drawviewmodel, NULL);
	Cvar_RegisterVariable (&r_ringalpha, NULL);
	Cvar_RegisterVariable (&r_truegunangle, NULL);

	Cvar_RegisterVariable (&r_shadows, NULL);
	Cvar_RegisterVariable (&r_mirroralpha, NULL);
	Cvar_RegisterVariable (&r_wateralpha, NULL);
	Cvar_RegisterVariable (&r_dynamic, NULL);
	Cvar_RegisterVariable (&r_novis, NULL);
	Cvar_RegisterVariable (&r_speeds, NULL);
	Cvar_RegisterVariable (&r_waterwarp, NULL);
	Cvar_RegisterVariable (&r_farclip, NULL);

	// fenix@io.com: register new cvar for model interpolation
	Cvar_RegisterVariable (&r_interpolate_animation, NULL);
	Cvar_RegisterVariable (&r_interpolate_transform, NULL);
	Cvar_RegisterVariable (&r_interpolate_weapon, NULL);
	Cvar_RegisterVariable (&r_clearcolor, R_SetClearColor_f);

	Cvar_RegisterVariable (&gl_finish, NULL);
	Cvar_RegisterVariable (&gl_texsort, NULL);

#if 0 // Baker this isn't good at the moment
	if (gl_mtexable)
		Cvar_SetValueByRef (&gl_texsort, 0.0);
#endif

	Cvar_RegisterVariable (&gl_cull, NULL);
	Cvar_RegisterVariable (&gl_smoothmodels, NULL);
	Cvar_RegisterVariable (&gl_affinemodels, NULL);
	Cvar_RegisterVariable (&gl_polyblend, NULL);
	Cvar_RegisterVariable (&gl_flashblend, NULL);
	Cvar_RegisterVariable (&gl_playermip, NULL);
	Cvar_RegisterVariable (&gl_nocolors, NULL);

	Cvar_RegisterVariable (&gl_keeptjunctions, NULL);
	Cvar_RegisterVariable (&gl_reporttjunctions, NULL);
	Cvar_RegisterVariable (&gl_fullbright, NULL);
	Cvar_RegisterVariable (&gl_overbright, NULL);

	Cvar_RegisterVariable (&gl_doubleeyes, NULL);

#ifdef SUPPORTS_XFLIP
	Cvar_RegisterVariable (&gl_xflip, NULL);  //Atomizer - GL_XFLIP
#endif

#ifdef SUPPORTS_SKYBOX
	Cvar_RegisterVariable (&r_oldsky, NULL);
#endif

#ifdef SUPPORTS_FOG // Baker: d3dquake hates fog
	Cvar_RegisterVariable (&gl_fogenable, NULL);
	Cvar_RegisterVariable (&gl_fogstart, NULL);
	Cvar_RegisterVariable (&gl_fogend, NULL);
	Cvar_RegisterVariable (&gl_fogdensity, NULL);
	Cvar_RegisterVariable (&gl_fogalpha, NULL);
	Cvar_RegisterVariable (&gl_fogred, NULL);
	Cvar_RegisterVariable (&gl_fogblue, NULL);
	Cvar_RegisterVariable (&gl_foggreen, NULL);

#endif

	Cvar_RegisterVariable (&gl_nearwater_fix, NULL);
	Cvar_RegisterVariable (&gl_fadescreen_alpha, NULL);

	R_InitTextures ();
	R_InitParticles ();
	R_InitParticleTexture ();

#ifdef GLTEST
	Test_Init ();
#endif

	playertextures = texture_extension_number;
	texture_extension_number += MAX_SCOREBOARD;

	skyboxtextures = texture_extension_number;
	texture_extension_number += 6;
}



/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene (void)
{
	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList ();

	GL_DisableMultitexture();

	R_RenderDlights ();

	R_DrawParticles ();

#ifdef GLTEST
	Test_Draw ();
#endif

}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	int	clearbits = 0;

#ifdef SUPPORTS_ENHANCED_GAMMA
	// If gl_clear is 1, we always clear the color buffer
	// Or if hardware gamma isn't enabled and contrast is greater than 1
	if (using_hwgamma) // Baker hwgamma
		if (gl_clear.value || (!vid_hwgamma_enabled && v_contrast.value > 1))
			clearbits |= GL_COLOR_BUFFER_BIT;
#endif // D3D no support v_constrast

	if (r_mirroralpha.value < 1.0) // Baker 3.99: was != 1.0, changed in event gets set to # higher than 1.0
	{
		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 0.5;
		glDepthFunc (GL_LEQUAL);
	}
#ifndef DX8QUAKE_NO_GL_ZTRICK // MH says "ztrick doesn't play nice with D3D (it shouldn't be used in GL either)"
	else if (gl_ztrick.value)
	{
		static int trickframe;

		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			glDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			glDepthFunc (GL_GEQUAL);
		}
	}
#endif
	else
	{
		// Baker hwgamma support
#ifdef SUPPORTS_ENHANCED_GAMMA
		if (using_hwgamma) {
			clearbits |= GL_DEPTH_BUFFER_BIT;
			glClear (clearbits);
		} else
#endif
		{
			if (gl_clear.value)
				glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			else
				glClear (GL_DEPTH_BUFFER_BIT);
		}
		// Baker end hwgamma support

		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);
	}

	glDepthRange (gldepthmin, gldepthmax);
}

/*
=============
R_Mirror
=============
*/
void R_Mirror (void)
{
	float		d;
	msurface_t	*s;
	entity_t	*ent;

	if (!mirror)
		return;

	memcpy (r_base_world_matrix, r_world_matrix, sizeof(r_base_world_matrix));

	d = DotProduct (r_refdef.vieworg, mirror_plane->normal) - mirror_plane->dist;
	VectorMA (r_refdef.vieworg, -2*d, mirror_plane->normal, r_refdef.vieworg);

	d = DotProduct (vpn, mirror_plane->normal);
	VectorMA (vpn, -2*d, mirror_plane->normal, vpn);

	r_refdef.viewangles[0] = -asinf (vpn[2])/M_PI*180;
	r_refdef.viewangles[1] = atan2f (vpn[1], vpn[0])/M_PI*180;
	r_refdef.viewangles[2] = -r_refdef.viewangles[2];

	ent = &cl_entities[cl.viewentity];
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts] = ent;
		cl_numvisedicts++;
	}

	gldepthmin = 0.5;
	gldepthmax = 1;
	glDepthRange (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);

	R_RenderScene ();
	R_DrawWaterSurfaces ();

	gldepthmin = 0;
	gldepthmax = 0.5;
	glDepthRange (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);

	// blend on top
	glEnable (GL_BLEND);
	//Baker 3.60 - Mirror alpha fix - from QER

	if (r_mirroralpha.value <1) // Baker 3.61 - Only run mirror alpha fix if it is being used; hopefully this may fix a possible crash issue on some video cards
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//mirror fix
	glMatrixMode(GL_PROJECTION);
	if (mirror_plane->normal[2])
		glScalef (1,-1,1);
	else
		glScalef (-1,1,1);
	glCullFace(GL_FRONT);
	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf (r_base_world_matrix);

	glColor4f (1,1,1,r_mirroralpha.value);
	s = cl.worldmodel->textures[mirrortexturenum]->texturechain;
	for ( ; s ; s=s->texturechain)
		R_RenderBrushPoly (s);
	cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;
	glDisable (GL_BLEND);
	//Baker 3.60 - Mirror alpha fix - from QER
	if (r_mirroralpha.value <1) // Baker 3.61 - Only run mirror alpha fix if it is being used; hopefully this may fix a possible crash issue on some video cards
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//mirror fix
	glColor4f (1,1,1,1);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1 = 0.0, time2;
	GLfloat colors[4] = {(GLfloat) 0.0, (GLfloat) 0.0, (GLfloat) 1, (GLfloat) 0.20};

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		glFinish ();
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = false;

	if (gl_finish.value)
		glFinish ();

	R_Clear ();

	// render normal view

#ifdef SUPPORTS_FOG //Baker: d3dquake hates
// NATAS - BramBo - fog code

if( gl_fogenable.value ) {
	glFogi(GL_FOG_MODE, GL_LINEAR);
	colors[0] = gl_fogred.value;
	colors[1] = gl_foggreen.value;
	colors[2] = gl_fogblue.value;
	glFogfv(GL_FOG_COLOR, colors);
	glFogf(GL_FOG_START, gl_fogstart.value);
	glFogf(GL_FOG_END, gl_fogend.value);
	glFogf(GL_FOG_DENSITY, gl_fogdensity.value);
	glEnable(GL_FOG);
} else {
	glDisable(GL_FOG);
}
#endif

	R_RenderScene ();
	R_DrawViewModel ();
	R_DrawWaterSurfaces ();

#ifdef SUPPORTS_FOG
   // NATAS - BramBo - Fog code
   glDisable(GL_FOG);
#endif

	// render mirror view
	R_Mirror ();

	R_PolyBlend ();

	if (r_speeds.value)
	{
		time2 = Sys_DoubleTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys);
	}
}
