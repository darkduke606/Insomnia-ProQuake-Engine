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
// r_model.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"

model_t	*loadmodel;
char	loadname[32];	// for hunk tags

void Mod_LoadAliasModel (model_t *mod, void *buffer);
void Mod_LoadSpriteModel (model_t *mod, void *buffer);
void Mod_LoadBrushModel (model_t *mod, void *buffer);
model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[MAX_MAP_LEAFS/8];

#ifdef FITZQUAKE_PROTOCOL
#define	MAX_MOD_KNOWN	2048 //johnfitz -- was 512
#else
#define	MAX_MOD_KNOWN	256 // GL uses 512?
#endif
model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

// values for model_t's needload
#define NL_PRESENT		0
#define NL_NEEDS_LOADED	1
#define NL_UNREFERENCED	2

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
}

/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	void	*r;

	if ((r = Cache_Check(&mod->cache)))
		return r;

	Mod_LoadModel (mod, true);

	if (!mod->cache.data)
		Sys_Error ("Mod_Extradata: caching failed");

	return mod->cache.data;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	float		d;
	mnode_t		*node;
	mplane_t	*plane;

	if (!model || !model->nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		node = (d > 0) ? node->children[0] : node->children[1];
	}

	return NULL;	// never reached
}

/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c, row;
	byte	*out;
	
	row = (model->numleafs+7)>>3;
	out = decompressed;

	if (!in) 
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

byte *Mod_LeafPVS (mleaf_t *leaf, model_t *model)
{
	if (leaf == model->leafs)
		return mod_novis;

	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	int		i;
	model_t	*mod;

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++) {
		mod->needload = NL_UNREFERENCED;
//FIX FOR CACHE_ALLOC ERRORS:
		if (mod->type == mod_sprite) 
			mod->cache.data = NULL;
	}
}

/*
==================
Mod_FindName
==================
*/
model_t *Mod_FindName (char *name)
{
	int		i;
	model_t	*mod;
	model_t	*avail = NULL;

	if (!name[0])
		Sys_Error ("Mod_FindName: NULL name");

// search the currently loaded models
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++) 
	{
		if (!strcmp (mod->name, name) )
			break;
		if (mod->needload == NL_UNREFERENCED)
			if (!avail || mod->type != mod_alias)
				avail = mod;
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN) 
		{
			if (avail) {
				mod = avail;
				if (mod->type == mod_alias)
					if (Cache_Check (&mod->cache))
						Cache_Free (&mod->cache);
			} else {
				Sys_Error ("Mod_FindName: mod_numknown == MAX_MOD_KNOWN");
			}
		} else {
			mod_numknown++;
		}
		
		strcpy (mod->name, name);
		mod->needload = NL_NEEDS_LOADED;
	}

	return mod;
}

/*
==================
Mod_TouchModel
==================
*/
void Mod_TouchModel (char *name)
{
	model_t	*mod;

	mod = Mod_FindName (name);

	if (mod->needload == NL_PRESENT)
	{
		if (mod->type == mod_alias)
			Cache_Check (&mod->cache);
	}
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t *Mod_LoadModel (model_t *mod, qboolean crash)
{
	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

	if (mod->type == mod_alias) {
		if (Cache_Check (&mod->cache)) {
			mod->needload = NL_PRESENT;
			return mod;
		}
	} else {
		if (mod->needload == NL_PRESENT)
			return mod;
	}

// because the world is so huge, load it one piece at a time

// load the file
	if (!(buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf))))
	{
		if (crash)
			Host_Error ("Mod_LoadModel: %s not found", mod->name);
		return NULL;
	}

// allocate a new model
	COM_FileBase (mod->name, loadname);

	loadmodel = mod;

// fill it in

// call the apropriate loader
	mod->needload = NL_PRESENT;

	switch (LittleLong(*(unsigned *)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadAliasModel (mod, buf);
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;

	default:
		Mod_LoadBrushModel (mod, buf);
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;

	mod = Mod_FindName (name);

	return Mod_LoadModel (mod, crash);
}

/*
===============================================================================
					BRUSHMODEL LOADING
===============================================================================
*/
#ifdef SUPPORTS_HLBSP_SW
#define ISTURBTEX(name)		((loadmodel->bspversion == Q1_BSPVERSION && (name)[0] == '*') ||	\
							 (loadmodel->bspversion == HL_BSPVERSION && (name)[0] == '!'))
#else
#define ISTURBTEX(name)		((name)[0] == '*')
#endif
#define ISSKYTEX(name)		((name)[0] == 's' && (name)[1] == 'k' && (name)[2] == 'y')

byte	*mod_base;


/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures (lump_t *l)
{
	int		i, j, pixels, num, max, altmax;
#ifdef SUPPORTS_HLBSP_SW
	char *s;
	int		width;
#endif
	miptex_t	*mt;
	texture_t	*tx, *tx2, *anims[10], *altanims[10];
	dmiptexlump_t *m;
	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}

	m = (dmiptexlump_t *)(mod_base + l->fileofs);

	m->nummiptex = LittleLong (m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = Hunk_AllocName (m->nummiptex * sizeof(*loadmodel->textures) , loadname);

	for (i=0 ; i<m->nummiptex ; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;

		mt = (miptex_t *)((byte *)m + m->dataofs[i]);
		mt->width = LittleLong (mt->width);
		mt->height = LittleLong (mt->height);
		for (j=0 ; j<MIPLEVELS ; j++)
			mt->offsets[j] = LittleLong (mt->offsets[j]);

		if ( (mt->width & 15) || (mt->height & 15) )
			Host_Error ("Mod_LoadTextures: Texture %s is not 16 aligned", mt->name);

#ifdef SUPPORTS_HLBSP_SW
		if (loadmodel->bspversion == HL_BSPVERSION) {
			width = ISTURBTEX(mt->name) ? 64 : 16;
			pixels = width * width;

			tx = loadmodel->textures[i] = Hunk_AllocName (sizeof(texture_t) + pixels, loadname);
			memcpy (tx->name, mt->name, sizeof(tx->name));
			tx->height = tx->width = width;

			for (num = (mt->width << 4 | mt->height), s = mt->name; *s; s++) 
				num += *s ^ i;
            memset (tx + 1, 16 + (num % 52) * 4, pixels);

			for (j = 0; j < MIPLEVELS; j++)
				tx->offsets[j] = sizeof(texture_t);

		} else {
#endif
			pixels = mt->width*mt->height/64*85;

			loadmodel->textures[i] = tx = Hunk_AllocName (sizeof(texture_t) +pixels, loadname );

			memcpy (tx->name, mt->name, sizeof(tx->name));
			tx->width = mt->width;
			tx->height = mt->height;
	
			for (j=0 ; j<MIPLEVELS ; j++)
				tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
	
			// the pixels immediately follow the structures
	
			memcpy ( tx+1, mt+1, pixels);
#ifdef SUPPORTS_HLBSP_SW
		}
		if (loadmodel->isworldmodel && loadmodel->bspversion != HL_BSPVERSION && ISSKYTEX(mt->name))	
			R_InitSky (tx);
#else
		if (ISSKYTEX(mt->name))
			R_InitSky (tx);
#endif
	}

// sequence the animations
	for (i=0 ; i<m->nummiptex ; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// already sequenced

	// find the number of frames in the animation
		memset (anims, 0, sizeof(anims));
		memset (altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';

		if (max >= '0' && max <= '9') 
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		} 
		else if (max >= 'A' && max <= 'J') 
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		} 
		else 
		{
			Host_Error ("Mod_LoadTextures: Bad animating texture %s", tx->name);
		}

		for (j=i+1 ; j<m->nummiptex ; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (strcmp (tx2->name+2, tx->name+2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9') 
			{
				num -= '0';
				anims[num] = tx2;
				if (num+1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J') 
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num+1 > altmax)
					altmax = num+1;
			}
			 else 
			{
				Host_Error ("Mod_LoadTextures: Bad animating texture %s", tx->name);
			}
		}

#define	ANIM_CYCLE	2
	// link them all together
		for (j=0 ; j<max ; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Host_Error ("Mod_LoadTextures: Missing frame %i of %s",j, tx->name);
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = anims[ (j+1)%max ];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j=0 ; j<altmax ; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Host_Error ("Mod_LoadTextures: Missing frame %i of %s",j, tx->name);
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = altanims[ (j+1)%altmax ];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}
}

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (lump_t *l)
{
#ifdef SUPPORTS_HLBSP_SW
	int i, j;
	byte *in;
#endif
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}

#ifdef SUPPORTS_HLBSP_SW
	if (loadmodel->bspversion == HL_BSPVERSION) {
		
		if (l->filelen % 3)
			Host_Error("Mod_LoadLighting: l->filelen % 3");
		loadmodel->lightdata = Hunk_AllocName (l->filelen / 3, loadname);
		in = mod_base + l->fileofs;
		for (i = j = 0; i < l->filelen; i += 3, j++)
			loadmodel->lightdata[j] = (in[i + 0] + in[i + 1] + in[i + 2]) / 3.0;
	} else 
#endif
	{
		loadmodel->lightdata = Hunk_AllocName ( l->filelen, loadname);
		memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
	}
}

/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = Hunk_AllocName ( l->filelen, loadname);
	memcpy (loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
	loadmodel->entities = Hunk_AllocName ( l->filelen, loadname);
	memcpy (loadmodel->entities, mod_base + l->fileofs, l->filelen);
}

vec3_t worldmins; // Baker: get world bounds
vec3_t worldmaxs;

/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes (lump_t *l, qboolean storebounds)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	if (storebounds) {
		worldmins[0]=99999999;
		worldmins[1]=99999999;
		worldmins[2]=99999999;
		worldmaxs[0]=-99999999;
		worldmaxs[1]=-99999999;
		worldmaxs[2]=-99999999;
	}

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadVertexes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
#ifdef SUPPORTS_SW_SKYBOX
	out = Hunk_AllocName ( (count+8)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	
#endif

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);

		if (out->position[0] < worldmins[0]) worldmins[0] = out->position[0];
		if (out->position[1] < worldmins[1]) worldmins[1] = out->position[1];
		if (out->position[2] < worldmins[2]) worldmins[2] = out->position[2];
		if (out->position[0] > worldmaxs[0]) worldmaxs[0] = out->position[0];
		if (out->position[1] > worldmaxs[1]) worldmaxs[1] = out->position[1];
		if (out->position[2] > worldmaxs[2]) worldmaxs[2] = out->position[2];

	}
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in, *out;
	int			i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadSubmodels: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	if (count > MAX_MODELS)	
		Host_Error("Mod_LoadSubmodels : count > MAX_MODELS");

	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		for (j=0 ; j<MAX_MAP_HULLS ; j++)
			out->headnode[j] = LittleLong (in->headnode[j]);
		out->visleafs = LittleLong (in->visleafs);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadEdges: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	
#ifdef SUPPORTS_SW_SKYBOX
	out = Hunk_AllocName ( (count + 13) * sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = Hunk_AllocName ( (count + 1) * sizeof(*out), loadname);	
#endif

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo (lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count, miptex;
	float	len1, len2;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadTexinfo: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	
#ifdef SUPPORTS_SW_SKYBOX
	out = Hunk_AllocName ( (count+6)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	
#endif

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
		len1 = VectorLength (out->vecs[0]);
		len2 = VectorLength (out->vecs[1]);
		len1 = (len1 + len2)/2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;


		miptex = LittleLong (in->miptex);
		out->flags = LittleLong (in->flags);

		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->numtextures)
				Host_Error ("Mod_LoadTexinfo: miptex >= loadmodel->numtextures");

			out->texture = loadmodel->textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

#ifdef GLQUAKE
#define MAX_SURF_EXTENTS 512
#else
#define MAX_SURF_EXTENTS 256
#endif


/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i, j, e, bmins[2], bmaxs[2];
	mvertex_t	*v;
	mtexinfo_t	*tex;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] + v->position[1] * tex->vecs[j][1] + v->position[2] * tex->vecs[j][2] + tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > MAX_SURF_EXTENTS)
		{
//			Con_SafePrintf ("excessive surface extents (%d, normal max = %d), texture %s in %s\n", s->extents[i], MAX_SURF_EXTENTS, tex->texture->name, loadmodel->name);
//			s->extents[i] = MAX_SURF_EXTENTS; // Kludge, but seems to work

			Host_Error ("CalcSurfaceExtents: excessive surface extents (%d, normal max = %d), texture %s in %s\n", s->extents[i], MAX_SURF_EXTENTS, tex->texture->name, loadmodel->name); //johnfitz -- was 512 in glquake, 256 in winquake
		}
	}
}

/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces (lump_t *l)
{
	int			i, count, surfnum, planenum, side;
	dface_t		*in;
	msurface_t 	*out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadFaces: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);

#ifdef SUPPORTS_SW_SKYBOX
	out = Hunk_AllocName ( (count+6)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	
#endif

#ifdef FITZQUAKE_PROTOCOL
	//johnfitz -- warn mappers about exceeding old limits
	if (count > 32767)
		Con_Warning ("%i faces exceeds standard limit of 32767.\n", count);
	//johnfitz
#endif
	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->flags = 0;

		planenum = LittleShort(in->planenum);

		if ((side = LittleShort(in->side)))
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleShort (in->texinfo);

		CalcSurfaceExtents (out);

	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
#ifdef SUPPORTS_HLBSP_SW
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + (loadmodel->bspversion == HL_BSPVERSION ? i / 3: i);

#else
		out->samples = (i == -1) ? NULL : loadmodel->lightdata + i;
#endif

	// set the drawing flags flag
		if (ISSKYTEX(out->texinfo->texture->name))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);

			continue;
		}

		if (ISTURBTEX(out->texinfo->texture->name))		// turbulent
		{
#ifdef SUPPORTS_SW_WATERALPHA
			// Manoel Kasimier - translucent water - begin
//			if (strncmp(out->texinfo->texture->name,"*lava",5)) // lava should be opaque
		//	if (Q_strncmp(out->texinfo->texture->name,"*teleport",9)) // teleport should be opaque
				out->flags |= SURF_DRAWTRANSLUCENT;
			// Manoel Kasimier - translucent water - end
#endif
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}

			continue;
		}
	}
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadNodes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

#ifdef FITZQUAKE_PROTOCOL
	//johnfitz -- warn mappers about exceeding old limits
	if (count > 32767)
		Con_Warning ("%i nodes exceeds standard limit of 32767.\n", count);
	//johnfitz
#endif

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

#ifdef FITZQUAKE_PROTOCOL
		out->firstsurface = (unsigned short)LittleShort (in->firstface); //johnfitz -- explicit cast as unsigned short
		out->numsurfaces = (unsigned short)LittleShort (in->numfaces); //johnfitz -- explicit cast as unsigned short
#else
		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);
#endif
		for (j=0 ; j<2 ; j++)
		{
#ifdef FITZQUAKE_PROTOCOL
			//johnfitz -- hack to handle nodes > 32k, adapted from darkplaces
			p = (unsigned short)LittleShort(in->children[j]);
			if (p < count)
				out->children[j] = loadmodel->nodes + p;
			else
			{
				p = 65535 - p; //note this uses 65535 intentionally, -1 is leaf 0
				if (p < loadmodel->numleafs)
					out->children[j] = (mnode_t *)(loadmodel->leafs + p);
				else
				{
					Con_Printf("Mod_LoadNodes: invalid leaf index %i (file has only %i leafs)\n", p, loadmodel->numleafs);
					out->children[j] = (mnode_t *)(loadmodel->leafs); //map it to the solid leaf
				}
			}
			//johnfitz
#else
			p = LittleShort (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
#endif
		}
	}

	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (lump_t *l)
{
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadLeafs: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);
#ifdef FITZQUAKE_PROTOCOL
	//johnfitz
	if (count > 32767)
		Host_Error ("Mod_LoadLeafs: %i leafs exceeds limit of 32767.\n", count);
	//johnfitz
#endif

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

#ifdef FITZQUAKE_PROTOCOL
		out->firstmarksurface = loadmodel->marksurfaces + (unsigned short)LittleShort(in->firstmarksurface); //johnfitz -- unsigned short
		out->nummarksurfaces = (unsigned short)LittleShort(in->nummarksurfaces); //johnfitz -- unsigned short
#else
		out->firstmarksurface = loadmodel->marksurfaces + LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);
#endif

		p = LittleLong(in->visofs);
		out->compressed_vis = (p == -1) ?  NULL :  loadmodel->visdata + p;
		out->efrags = NULL;

		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes (lump_t *l)
{
#ifdef FITZQUAKE_PROTOCOL
	dclipnode_t *in;
	mclipnode_t *out; //johnfitz -- was dclipnode_t
#else
	dclipnode_t *in, *out;
#endif
	int			i, count;
	hull_t		*hull;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadClipnodes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

#ifdef FITZQUAKE_PROTOCOL
	//johnfitz -- warn about exceeding old limits
	if (count > 32767)
		Con_Warning ("%i clipnodes exceeds standard limit of 32767.\n", count);
	//johnfitz
#endif

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;
#ifdef SUPPORTS_HLBSP_SW
	if (loadmodel->bspversion == HL_BSPVERSION) 
	{
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -36;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 36;
		hull->available = true;

		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -32;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = 32;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -18;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 18;
		hull->available = true;
	} 
	else
#endif
	{
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 32;
		hull->available = true;

		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = 64;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -6;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 30;
		hull->available = false;
	}

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);

#ifdef FITZQUAKE_PROTOCOL
		//johnfitz -- bounds check
		if (out->planenum < 0 || out->planenum >= loadmodel->numplanes)
			Host_Error ("Mod_LoadClipnodes: planenum out of bounds");
		//johnfitz

		//johnfitz -- support clipnodes > 32k
		out->children[0] = (unsigned short)LittleShort(in->children[0]);
		out->children[1] = (unsigned short)LittleShort(in->children[1]);
		if (out->children[0] >= count)
			out->children[0] -= 65536;
		if (out->children[1] >= count)
			out->children[1] -= 65536;
		//johnfitz
#else
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
#endif
	}
}

/*
=================
Mod_MakeHull0

Duplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0 (void)
{
	mnode_t		*in, *child;
#ifdef FITZQUAKE_PROTOCOL
	mclipnode_t *out; //johnfitz -- was dclipnode_t
#else
	dclipnode_t *out;
#endif
	int			i, j, count;
	hull_t		*hull;

	hull = &loadmodel->hulls[0];

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces (lump_t *l)
{
	int		i, j, count;
	short		*in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadMarksurfaces: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

#ifdef FITZQUAKE_PROTOCOL
	//johnfitz -- warn mappers about exceeding old limits
	if (count > 32767)
		Con_Warning ("%i marksurfaces exceeds standard limit of 32767.\n", count);
	//johnfitz
#endif

	for ( i=0 ; i<count ; i++)
	{
#ifdef FITZQUAKE_PROTOCOL
		j = (unsigned short)LittleShort(in[i]); //johnfitz -- explicit cast as unsigned short
#else
		j = LittleShort(in[i]);
#endif
		if (j >= loadmodel->numsurfaces)
			Host_Error ("Mod_LoadMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (lump_t *l)
{
	int	i, count, *in, *out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadSurfedges: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);

#ifdef SUPPORTS_SW_SKYBOX
	out = Hunk_AllocName ( (count+24)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = Hunk_AllocName ( count*sizeof(*out), loadname);	
#endif

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (lump_t *l)
{
	int		i, j, count, bits;
	mplane_t	*out;
	dplane_t 	*in;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadPlanes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);

#ifdef SUPPORTS_SW_SKYBOX
	out = Hunk_AllocName ( (count+6)*2*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = Hunk_AllocName ( count*2*sizeof(*out), loadname);	
#endif

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);

	return VectorLength (corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel (model_t *mod, void *buffer)
{
	int			i, j;
	dheader_t	*header;
	dmodel_t 	*bm;

	loadmodel->type = mod_brush;

	header = (dheader_t *)buffer;

#ifdef SUPPORTS_HLBSP_SW
	mod->bspversion = LittleLong (header->version);

	if (mod->bspversion != Q1_BSPVERSION && mod->bspversion != HL_BSPVERSION)
		Host_Error ("Mod_LoadBrushModel: %s has wrong version number (%i should be %i (Quake) or %i (HalfLife))", mod->name, mod->bspversion, Q1_BSPVERSION, HL_BSPVERSION);

#else
	i = LittleLong (header->version);
	if (i != Q1_BSPVERSION)
		Host_Error ("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, Q1_BSPVERSION);

#endif
	{
		extern cvar_t host_mapname;
		loadmodel->isworldmodel = !strcmp (loadmodel->name, va("maps/%s.bsp", host_mapname.string));
	}



// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	Mod_LoadVertexes (&header->lumps[LUMP_VERTEXES],  loadmodel->isworldmodel ? 1 : 0);
	if (loadmodel->isworldmodel)
		Con_DPrintf ("World bounds: %4.1f %4.1f %4.1f to %4.1f %4.1f %4.1f\n", worldmins[0], worldmins[1], worldmins[2], worldmaxs[0], worldmaxs[1], worldmaxs[2]);

	Mod_LoadEdges (&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadTextures (&header->lumps[LUMP_TEXTURES]);
	Mod_LoadLighting (&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces (&header->lumps[LUMP_MARKSURFACES]);
	Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs (&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes (&header->lumps[LUMP_NODES]);
	Mod_LoadClipnodes (&header->lumps[LUMP_CLIPNODES]);
	Mod_LoadEntities (&header->lumps[LUMP_ENTITIES]);
	Mod_LoadSubmodels (&header->lumps[LUMP_MODELS]);

	Mod_MakeHull0 ();

	mod->numframes = 2;		// regular and alternate animation
	mod->flags = 0;

// set up the submodels (FIXME: this is confusing)


	// johnfitz -- okay, so that i stop getting confused every time i look at this loop, here's how it works:
	// we're looping through the submodels starting at 0.  Submodel 0 is the main model, so we don't have to
	// worry about clobbering data the first time through, since it's the same data.  At the end of the loop,
	// we create a new copy of the data to use the next time through.
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j=1 ; j<MAX_MAP_HULLS ; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes-1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);

		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels-1)
		{	// duplicate the basic information
			char	name[10];

			snprintf (name, sizeof(name), "*%i", i+1);
			loadmodel = Mod_FindName (name);
			*loadmodel = *mod;
			strcpy (loadmodel->name, name);
			mod = loadmodel;
		}
	}
}

/*
==============================================================================
ALIAS MODELS
==============================================================================
*/

/*
=================
Mod_LoadAliasFrame
=================
*/
void * Mod_LoadAliasFrame (void * pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
{
	int				i, j;
	trivertx_t		*pframe, *pinframe;
	daliasframe_t	*pdaliasframe;

	pdaliasframe = (daliasframe_t *)pin;

	strcpy (name, pdaliasframe->name);

	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about endianness
		pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
		pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
	}

	pinframe = (trivertx_t *)(pdaliasframe + 1);
	pframe = Hunk_AllocName (numv * sizeof(*pframe), loadname);

	*pframeindex = (byte *)pframe - (byte *)pheader;

	for (j=0 ; j<numv ; j++)
	{
		int		k;

	// these are all byte values, so no need to deal with endianness
		pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

		for (k=0 ; k<3 ; k++)
			pframe[j].v[k] = pinframe[j].v[k];
		}

	pinframe += numv;
	return (void *)pinframe;
}

/*
=================
Mod_LoadAliasGroup
=================
*/
void * Mod_LoadAliasGroup (void * pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
{
	int					i, numframes;
	float				*poutintervals;
	void				*ptemp;
	daliasgroup_t		*pingroup;
	daliasinterval_t	*pin_intervals;
	maliasgroup_t		*paliasgroup;

	pingroup = (daliasgroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	paliasgroup = Hunk_AllocName (sizeof (maliasgroup_t) + (numframes - 1) * sizeof (paliasgroup->frames[0]), loadname);

	paliasgroup->numframes = numframes;

	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about endianness
		pbboxmin->v[i] = pingroup->bboxmin.v[i];
		pbboxmax->v[i] = pingroup->bboxmax.v[i];
	}

	*pframeindex = (byte *)paliasgroup - (byte *)pheader;

	pin_intervals = (daliasinterval_t *)(pingroup + 1);

	poutintervals = Hunk_AllocName (numframes * sizeof (float), loadname);

	paliasgroup->intervals = (byte *)poutintervals - (byte *)pheader;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Host_Error ("Mod_LoadAliasGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadAliasFrame (ptemp, &paliasgroup->frames[i].frame, numv, 
				&paliasgroup->frames[i].bboxmin, 
				&paliasgroup->frames[i].bboxmax, pheader, name);
	}

	return ptemp;
}

//=========================================================

/*
=================
Mod_LoadAliasSkin
=================
*/
void * Mod_LoadAliasSkin (void * pin, int *pskinindex, int skinsize, aliashdr_t *pheader)
{
	byte	*pskin, *pinskin;

	pskin = Hunk_AllocName (skinsize * r_pixbytes, loadname);
	pinskin = (byte *)pin;
	*pskinindex = (byte *)pskin - (byte *)pheader;

	if (r_pixbytes == 1) {
		memcpy (pskin, pinskin, skinsize);
	} else if (r_pixbytes == 2) {
		int				i;
		unsigned short	*pusskin;
		pusskin = (unsigned short *)pskin;

		for (i=0 ; i<skinsize ; i++)
			pusskin[i] = d_8to16table[pinskin[i]];
	} else {
		Host_Error ("Mod_LoadAliasSkin: driver set invalid r_pixbytes: %d\n", r_pixbytes);
	}

	pinskin += skinsize;

	return ((void *)pinskin);
}


/*
=================
Mod_LoadAliasSkinGroup
=================
*/
void * Mod_LoadAliasSkinGroup (void * pin, int *pskinindex, int skinsize, aliashdr_t *pheader)
{
	daliasskingroup_t		*pinskingroup;
	maliasskingroup_t		*paliasskingroup;
	int						i, numskins;
	daliasskininterval_t	*pinskinintervals;
	float					*poutskinintervals;
	void					*ptemp;

	pinskingroup = (daliasskingroup_t *)pin;

	numskins = LittleLong (pinskingroup->numskins);

	paliasskingroup = Hunk_AllocName (sizeof (maliasskingroup_t) +
			(numskins - 1) * sizeof (paliasskingroup->skindescs[0]), loadname);

	paliasskingroup->numskins = numskins;

	*pskinindex = (byte *)paliasskingroup - (byte *)pheader;

	pinskinintervals = (daliasskininterval_t *)(pinskingroup + 1);

	poutskinintervals = Hunk_AllocName (numskins * sizeof (float),loadname);

	paliasskingroup->intervals = (byte *)poutskinintervals - (byte *)pheader;

	for (i=0 ; i<numskins ; i++)
	{
		*poutskinintervals = LittleFloat (pinskinintervals->interval);
		if (*poutskinintervals <= 0)
			Host_Error ("Mod_LoadAliasSkinGroup: interval<=0");

		poutskinintervals++;
		pinskinintervals++;
	}

	ptemp = (void *)pinskinintervals;

	for (i=0 ; i<numskins ; i++)
		ptemp = Mod_LoadAliasSkin (ptemp, &paliasskingroup->skindescs[i].skin, skinsize, pheader);

	return ptemp;
}


/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel (model_t *mod, void *buffer)
{
	int					i, j, version, numframes, numskins, size, skinsize, start, end, total;
	mdl_t				*pmodel, *pinmodel;
	stvert_t			*pstverts, *pinstverts;
	aliashdr_t			*pheader;
	mtriangle_t			*ptri;
	dtriangle_t			*pintriangles;
	daliasframetype_t	*pframetype;
	daliasskintype_t	*pskintype;
	maliasskindesc_t	*pskindesc;


	start = Hunk_LowMark ();

	pinmodel = (mdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Host_Error ("Mod_LoadAliasModel: %s has wrong version number (%i should be %i)", mod->name, version, ALIAS_VERSION);

// allocate space for a working header, plus all the data except the frames, skin and group info	
	size = 	sizeof (aliashdr_t) + (LittleLong (pinmodel->numframes) - 1) *  sizeof (pheader->frames[0]) + sizeof (mdl_t) + LittleLong (pinmodel->numverts) * sizeof (stvert_t) + LittleLong (pinmodel->numtris) * sizeof (mtriangle_t);

	pheader = Hunk_AllocName (size, loadname);
	pmodel = (mdl_t *) ((byte *)&pheader[1] + (LittleLong (pinmodel->numframes) - 1) * sizeof (pheader->frames[0]));

	mod->flags = LittleLong (pinmodel->flags);

// endian-adjust and copy the data, starting with the alias model header
	pmodel->boundingradius = LittleFloat (pinmodel->boundingradius);
	pmodel->numskins = LittleLong (pinmodel->numskins);
	pmodel->skinwidth = LittleLong (pinmodel->skinwidth);
	pmodel->skinheight = LittleLong (pinmodel->skinheight);

	if (pmodel->skinheight > MAX_LBM_HEIGHT)
		Host_Error ("Mod_LoadAliasModel: model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT);

	pmodel->numverts = LittleLong (pinmodel->numverts);

	if (pmodel->numverts <= 0)
		Host_Error ("Mod_LoadAliasModel: model %s has no vertices", mod->name);

	if (pmodel->numverts > MAXALIASVERTS)
		Host_Error ("Mod_LoadAliasModel: model %s has too many vertices", mod->name);

#ifdef SUPPORTS_SOFTWARE_ANIM_INTERPOLATION
	{
		void R_CheckAliasVerts (int numverts);
		R_CheckAliasVerts (pmodel->numverts);
	}
#endif

	pmodel->numtris = LittleLong (pinmodel->numtris);

	if (pmodel->numtris <= 0)
		Host_Error ("Mod_LoadAliasModel: model %s has no triangles", mod->name);

	pmodel->numframes = LittleLong (pinmodel->numframes);
	pmodel->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = LittleLong (pinmodel->synctype);
	mod->numframes = pmodel->numframes;

	for (i=0 ; i<3 ; i++)
	{
		pmodel->scale[i] = LittleFloat (pinmodel->scale[i]);
		pmodel->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pmodel->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}

	numskins = pmodel->numskins;
	numframes = pmodel->numframes;

	if (pmodel->skinwidth & 0x03)
		Host_Error ("Mod_LoadAliasModel: skinwidth not multiple of 4");

	pheader->model = (byte *)pmodel - (byte *)pheader;

// load the skins
	skinsize = pmodel->skinheight * pmodel->skinwidth;

	if (numskins < 1)
		Host_Error ("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

	pskintype = (daliasskintype_t *)&pinmodel[1];
	pskindesc = Hunk_AllocName (numskins * sizeof (maliasskindesc_t), loadname);

	pheader->skindesc = (byte *)pskindesc - (byte *)pheader;

	for (i=0 ; i<numskins ; i++)
	{
		aliasskintype_t	skintype;

		skintype = LittleLong (pskintype->type);
		pskindesc[i].type = skintype;

		if (skintype == ALIAS_SKIN_SINGLE)
			pskintype = (daliasskintype_t *) Mod_LoadAliasSkin (pskintype + 1,  &pskindesc[i].skin, skinsize, pheader);
		else
			pskintype = (daliasskintype_t *) Mod_LoadAliasSkinGroup (pskintype + 1, &pskindesc[i].skin, skinsize, pheader);
	}

// set base s and t vertices
	pstverts = (stvert_t *)&pmodel[1];
	pinstverts = (stvert_t *)pskintype;

	pheader->stverts = (byte *)pstverts - (byte *)pheader;

	for (i=0 ; i<pmodel->numverts ; i++)
	{
		pstverts[i].onseam = LittleLong (pinstverts[i].onseam);
	// put s and t in 16.16 format
		pstverts[i].s = LittleLong (pinstverts[i].s) << 16;
		pstverts[i].t = LittleLong (pinstverts[i].t) << 16;
	}

// set up the triangles
	ptri = (mtriangle_t *)&pstverts[pmodel->numverts];
	pintriangles = (dtriangle_t *)&pinstverts[pmodel->numverts];

	pheader->triangles = (byte *)ptri - (byte *)pheader;

	for (i=0 ; i<pmodel->numtris ; i++)
	{
		ptri[i].facesfront = LittleLong (pintriangles[i].facesfront);

		for (j=0 ; j<3 ; j++)
			ptri[i].vertindex[j] = LittleLong (pintriangles[i].vertindex[j]);
	}

// load the frames
	if (numframes < 1)
		Host_Error ("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

	pframetype = (daliasframetype_t *)&pintriangles[pmodel->numtris];

	for (i=0 ; i<numframes ; i++) {
		aliasframetype_t	frametype;

		frametype = LittleLong (pframetype->type);
		pheader->frames[i].type = frametype;

		if (frametype == ALIAS_SINGLE) {
			pframetype = (daliasframetype_t *) Mod_LoadAliasFrame (pframetype + 1, &pheader->frames[i].frame, pmodel->numverts, &pheader->frames[i].bboxmin, &pheader->frames[i].bboxmax, pheader, pheader->frames[i].name);
		} else {
			pframetype = (daliasframetype_t *) Mod_LoadAliasGroup (pframetype + 1, &pheader->frames[i].frame, pmodel->numverts, &pheader->frames[i].bboxmin, &pheader->frames[i].bboxmax, pheader, pheader->frames[i].name);
		}
	}

	mod->type = mod_alias;

// FIXME: do this right
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

// move the complete, relocatable alias model to the cache
	end = Hunk_LowMark ();
	total = end - start;

	Cache_Alloc (&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;

	memcpy (mod->cache.data, pheader, total);

	Hunk_FreeToLowMark (start);
}

//=============================================================================

/*
=================
Mod_LoadSpriteFrame
=================
*/
void * Mod_LoadSpriteFrame (void * pin, mspriteframe_t **ppframe)
{
	int					width, height, size, origin[2];
	dspriteframe_t		*pinframe;
	mspriteframe_t		*pspriteframe;
	

	pinframe = (dspriteframe_t *)pin;

	width = LittleLong (pinframe->width);
	height = LittleLong (pinframe->height);
	size = width * height;

	pspriteframe = Hunk_AllocName (sizeof (mspriteframe_t) + size*r_pixbytes, loadname);
	memset (pspriteframe, 0, sizeof (mspriteframe_t) + size);

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	origin[0] = LittleLong (pinframe->origin[0]);
	origin[1] = LittleLong (pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

	if (r_pixbytes == 1) {
		memcpy (&pspriteframe->pixels[0], (byte *)(pinframe + 1), size);
	} else if (r_pixbytes == 2) {
		int					i;
		unsigned short		*ppixout;
		byte				*ppixin;

		ppixin = (byte *)(pinframe + 1);
		ppixout = (unsigned short *)&pspriteframe->pixels[0];

		for (i=0 ; i<size ; i++)
			ppixout[i] = d_8to16table[ppixin[i]];
	} else {
		Host_Error ("Mod_LoadSpriteFrame: driver set invalid r_pixbytes: %d\n", r_pixbytes);
	}

	return (void *)((byte *)pinframe + sizeof (dspriteframe_t) + size);
}

/*
=================
Mod_LoadSpriteGroup
=================
*/
void * Mod_LoadSpriteGroup (void * pin, mspriteframe_t **ppframe)
{
	int					i, numframes;
	float				*poutintervals;
	void				*ptemp;
	dspritegroup_t		*pingroup;
	mspritegroup_t		*pspritegroup;
	dspriteinterval_t	*pin_intervals;

	pingroup = (dspritegroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	pspritegroup = Hunk_AllocName (sizeof (mspritegroup_t) + (numframes - 1) * sizeof (pspritegroup->frames[0]), loadname);

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t *)pspritegroup;

	pin_intervals = (dspriteinterval_t *)(pingroup + 1);

	poutintervals = Hunk_AllocName (numframes * sizeof (float), loadname);

	pspritegroup->intervals = poutintervals;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
//			Host_Error ("Mod_LoadSpriteGroup: interval<=0");
			Host_Error ("Mod_LoadSpriteGroup: interval %f <= 0 in %s", *poutintervals, loadmodel->name);

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++) 
		ptemp = Mod_LoadSpriteFrame (ptemp, &pspritegroup->frames[i]);
	
	return ptemp;
}

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel (model_t *mod, void *buffer)
{
	int i, version, numframes, size;
	dsprite_t			*pin;
	msprite_t			*psprite;
	dspriteframetype_t	*pframetype;

	pin = (dsprite_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE_VERSION)
		Host_Error ("Mod_LoadSpriteModel: %s has wrong version number (%i should be %i)", mod->name, version, SPRITE_VERSION);

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = Hunk_AllocName (size, loadname);

	mod->cache.data = psprite;

	psprite->type = LittleLong (pin->type);
	psprite->maxwidth = LittleLong (pin->width);
	psprite->maxheight = LittleLong (pin->height);
	psprite->beamlength = LittleFloat (pin->beamlength);
	mod->synctype = LittleLong (pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;

// load the frames
	if (numframes < 1)
		Host_Error ("Mod_LoadSpriteModel: Invalid # of frames %d in %s", numframes, mod->name);

	mod->numframes = numframes;
	mod->flags = 0;

	pframetype = (dspriteframetype_t *)(pin + 1);

	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = LittleLong (pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
			pframetype = (dspriteframetype_t *) Mod_LoadSpriteFrame (pframetype + 1, &psprite->frames[i].frameptr);
		else
			pframetype = (dspriteframetype_t *) Mod_LoadSpriteGroup (pframetype + 1, &psprite->frames[i].frameptr);

	}

	mod->type = mod_sprite;
}

//=============================================================================

/*
================
Mod_Print_f
================
*/
void Mod_Print_f (void)
{
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++) 
	{
		Con_Printf ("%8p : %s",mod->cache.data, mod->name);
		if (mod->needload & NL_UNREFERENCED)
			Con_Printf (" (!R)");
		if (mod->needload & NL_NEEDS_LOADED)
			Con_Printf (" (!P)");
		Con_Printf ("\n");
	}
}
