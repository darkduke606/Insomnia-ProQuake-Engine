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
// r_sky.c

#include "quakedef.h"
//#include "r_local.h"
#include "d_local.h"


#define ISKYSPEED 8
#define ISKYSPEED2 2
float	skyspeed, skyspeed2;

float		skytime;

byte		*r_skysource;

int r_skymade;

// TODO: clean up these routines

byte	bottomsky[128*131];
byte	bottommask[128*131];
byte	newsky[128*256];	// newsky and topsky both pack in here, 128 bytes
							//  of newsky on the left of each scan, 128 bytes
							//  of topsky on the right, because the low-level
							//  drawers need 256-byte scan widths

cvar_t	r_skyname = { "r_skyname", ""}; // Manoel Kasimier - skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (texture_t *mt)
{
	int			i, j;
	byte		*src;

	src = (byte *)mt + mt->offsets[0];

	for (i=0 ; i<128 ; i++)
	{
		for (j=0 ; j<128 ; j++)
		{
			newsky[(i*256) + j + 128] = src[i*256 + j + 128];
		}
	}

	for (i=0 ; i<128 ; i++)
	{
		for (j=0 ; j<131 ; j++)
		{
			if (src[i*256 + (j & 0x7F)])
			{
				bottomsky[(i*131) + j] = src[i*256 + (j & 0x7F)];
				bottommask[(i*131) + j] = 0;
			}
			else
			{
				bottomsky[(i*131) + j] = 0;
				bottommask[(i*131) + j] = 0xff;
			}
		}
	}
	
	r_skysource = newsky;
}

// Manoel Kasimier - skyboxes - begin
// Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
extern	mtexinfo_t		r_skytexinfo[6];
extern	qboolean		r_drawskybox;
byte					r_skypixels[6][512*512]; // Manoel Kasimier - edited
texture_t				r_skytextures[6];
char					skyname[MAX_QPATH];

qboolean R_LoadSkybox (char *name)
{
	int		i;
	char	pathname[MAX_QPATH];
	byte	*pic;
	char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	int		r_skysideimage[6] = {5, 2, 4, 1, 0, 3};
	int		width, height;

	if (!name || !name[0])
	{
		skyname[0] = 0;
		return false;
	}

	// the same skybox we are using now
	if (!strcmp (name, skyname))
		return true;

	strncpy (skyname, name, sizeof(skyname)-1);

	for (i=0 ; i<6 ; i++)
	{
#ifdef __linux__
		snprintf (pathname, sizeof(pathname), "gfx/env/%s%s.pcx", skyname, suf[r_skysideimage[i]]);
#else
		snprintf (pathname, sizeof(pathname), "gfx/env/%s%s.pcx", skyname, suf[r_skysideimage[i]]);
#endif
		LoadPCX (pathname, &pic, &width, &height);

		if (!pic) {
			Con_Printf ("Couldn't load %s\n", pathname);
			return false;
		}
		// Manoel Kasimier - hi-res skyboxes - begin
		if (width != 256 && width != 512)
		{
			Con_Printf ("width for %s must be 256 or 512\n", pathname);
			free (pic);
			return false;
		}
		if (height != 256 && height != 512)
		{
			Con_Printf ("height for %s must be 256 or 512\n", pathname);
			free (pic);
			return false;
		}
		// Manoel Kasimier - hi-res skyboxes - end

		r_skytexinfo[i].texture = &r_skytextures[i];
		r_skytexinfo[i].texture->width = width; // Manoel Kasimier - hi-res skyboxes - edited
		r_skytexinfo[i].texture->height = height; // Manoel Kasimier - hi-res skyboxes - edited
		r_skytexinfo[i].texture->offsets[0] = i;

		// Manoel Kasimier - hi-res skyboxes - begin
		{
			extern vec3_t box_vecs[6][2];
			extern vec3_t box_bigvecs[6][2];
			extern msurface_t *r_skyfaces;

			if (width == 512)
				VectorCopy (box_bigvecs[i][0], r_skytexinfo[i].vecs[0])
			else
				VectorCopy (box_vecs[i][0], r_skytexinfo[i].vecs[0]);

			if (height == 512)
				VectorCopy (box_bigvecs[i][1], r_skytexinfo[i].vecs[1])
			else
				VectorCopy (box_vecs[i][1], r_skytexinfo[i].vecs[1]);

			r_skyfaces[i].texturemins[0] = -(width/2);
			r_skyfaces[i].texturemins[1] = -(height/2);
			r_skyfaces[i].extents[0] = width;
			r_skyfaces[i].extents[1] = height;
		}
		// Manoel Kasimier - hi-res skyboxes - end

		memcpy (r_skypixels[i], pic, width*height); // Manoel Kasimier - hi-res skyboxes - edited
		free (pic);
	}

	return true;
}

void R_LoadSky (char *s)
{
	if (s[0])
		r_drawskybox = R_LoadSkybox (s);
	else
		r_drawskybox = false;
}

void R_LoadSky_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("loadsky <name> : load a skybox\n");
		return;
	}
	R_LoadSky(Cmd_Argv(1)); // Manoel Kasimier
}
// Manoel Kasimier - skyboxes - end


/*
=================
R_MakeSky
=================
*/
void R_MakeSky (void)
{
	int			x, y, ofs, baseofs, xshift, yshift;
	unsigned	*pnewsky;
	static int	xlast = -1, ylast = -1;

	xshift = skytime*skyspeed;
	yshift = skytime*skyspeed;

	if ((xshift == xlast) && (yshift == ylast))
		return;

	xlast = xshift;
	ylast = yshift;
	
	pnewsky = (unsigned *)&newsky[0];

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
#if UNALIGNED_OK
		for (x=0 ; x<SKYSIZE ; x += 4)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

		// PORT: unaligned dword access to bottommask and bottomsky

			*pnewsky = (*(pnewsky + (128 / sizeof (unsigned))) & *(unsigned *)&bottommask[ofs]) | *(unsigned *)&bottomsky[ofs];
			pnewsky++;
		}
#else
		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*(byte *)pnewsky = (*((byte *)pnewsky + 128) & *(byte *)&bottommask[ofs]) | *(byte *)&bottomsky[ofs];
			pnewsky = (unsigned *)((byte *)pnewsky + 1);
		}
#endif

		pnewsky += 128 / sizeof (unsigned);
	}

	r_skymade = 1;
}


/*
=================
R_GenSkyTile
=================
*/
void R_GenSkyTile (void *pdest)
{
	int			x, y, ofs, baseofs, xshift, yshift;
	unsigned	*pnewsky, *pd;

	xshift = skytime*skyspeed;
	yshift = skytime*skyspeed;

	pnewsky = (unsigned *)&newsky[0];
	pd = (unsigned *)pdest;

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
#if UNALIGNED_OK
		for (x=0 ; x<SKYSIZE ; x += 4)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

		// PORT: unaligned dword access to bottommask and bottomsky

			*pd = (*(pnewsky + (128 / sizeof (unsigned))) & *(unsigned *)&bottommask[ofs]) | *(unsigned *)&bottomsky[ofs];
			pnewsky++;
			pd++;
		}
#else
		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*(byte *)pd = (*((byte *)pnewsky + 128) & *(byte *)&bottommask[ofs]) | *(byte *)&bottomsky[ofs];
			pnewsky = (unsigned *)((byte *)pnewsky + 1);
			pd = (unsigned *)((byte *)pd + 1);
		}
#endif

		pnewsky += 128 / sizeof (unsigned);
	}
}


/*
=================
R_GenSkyTile16
=================
*/
void R_GenSkyTile16 (void *pdest)
{
	int				x, y;
	int				ofs, baseofs;
	int				xshift, yshift;
	byte			*pnewsky;
	unsigned short	*pd;

	xshift = skytime * skyspeed;
	yshift = skytime * skyspeed;

	pnewsky = (byte *)&newsky[0];
	pd = (unsigned short *)pdest;

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
// FIXME: do faster unaligned version?
		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*pd = d_8to16table[(*(pnewsky + 128) &
					*(byte *)&bottommask[ofs]) |
					*(byte *)&bottomsky[ofs]];
			pnewsky++;
			pd++;
		}

		pnewsky += TILE_SIZE;
	}
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame (void)
{
	int		g, s1, s2;
	float	temp;

	skyspeed = ISKYSPEED;
	skyspeed2 = ISKYSPEED2;

	g = GreatestCommonDivisor (ISKYSPEED, ISKYSPEED2);
	s1 = ISKYSPEED / g;
	s2 = ISKYSPEED2 / g;
	temp = SKYSIZE * s1 * s2;

	skytime = cl.time - ((int)(cl.time / temp) * temp);
	
	r_skymade = 0;
}
