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
// gl_warp.c -- sky and water polygons

#include "quakedef.h"

extern	model_t	*loadmodel;

#ifdef SUPPORTS_SKYBOX
static int	  sky_width[6], sky_height[6];
#endif

int		skytexturenum;

int		solidskytexture;
int		alphaskytexture;
float	speedscale;		// for top sky and bottom sky

static msurface_t	*warpface;

extern cvar_t gl_subdivide_size;

static void BoundPoly (int numverts, float *verts, vec3_t mins, vec3_t maxs)
{
	int		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;
	for (i=0 ; i<numverts ; i++)
		for (j=0 ; j<3 ; j++, v++)
		{
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

static unsigned RecursLevel;

static void SubdividePolygon (int numverts, float *verts)
{
	int		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float	*v;
	vec3_t	front[64], back[64];
	int		f, b;
	float	dist[64];
	float	frac;
	glpoly_t	*poly;
	float	    s, t, subdivide;

	if (++RecursLevel > 128) // 16 seems enough and 512 might create stack overflow
		Sys_Error ("SubdividePolygon: excessive tree depth");

	if (numverts > 60)
		Sys_Error ("SubdividePolygon: excessive numverts %i", numverts);

	subdivide = gl_subdivide_size.value;

	if (subdivide < 32)
		subdivide = 32; // Avoid low subdivide values

	subdivide = QMAX(1, gl_subdivide_size.value);
	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++)
	{
		m = (mins[i] + maxs[i]) * 0.5;
		m = subdivide * floorf (m/subdivide + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numverts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numverts ; j++, v+= 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0)
			{
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ( (dist[j] > 0) != (dist[j+1] > 0) )
			{
				// clip point
				frac = dist[j] / (dist[j] - dist[j+1]);
				for (k=0 ; k<3 ; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon (f, front[0]);
		SubdividePolygon (b, back[0]);
		--RecursLevel;
		return;
	}

	poly = Hunk_Alloc (sizeof(glpoly_t) + (numverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts;
	for (i=0 ; i<numverts ; i++, verts+= 3)
	{
		VectorCopy (verts, poly->verts[i]);
		s = DotProduct (verts, warpface->texinfo->vecs[0]);
		t = DotProduct (verts, warpface->texinfo->vecs[1]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;
	}

	--RecursLevel;
}

/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void GL_SubdivideSurface (msurface_t *fa)
{
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float		*vec;

	warpface = fa;

	// convert edges back to a normal polygon
	numverts = 0;
	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;

		if (numverts >= 64)
			Sys_Error ("GL_SubdivideSurface: excessive numverts %i", numverts);

		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	RecursLevel = 0;
	SubdividePolygon (numverts, verts[0]);
}

//=========================================================



// speed up sin calculations - Ed
float	turbsin[] =
{
	#include "gl_warp_sin.h"
};
#define	TURBSINSIZE	256
#define TURBSCALE ((float) TURBSINSIZE / (2 * M_PI))

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
static void EmitFlatPoly (msurface_t *fa)
{
	glpoly_t	*p;
	float		*v;
	int		i;

	for (p = fa->polys ; p ; p = p->next)
	{
		glBegin (GL_POLYGON);
		for (i = 0, v = p->verts[0] ; i < p->numverts ; i++, v += VERTEXSIZE)
			glVertex3fv (v);
		glEnd ();
	}
}

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void EmitWaterPolys (msurface_t *fa)
{
	glpoly_t	*p;
	float		*v;
	int			i;
	float		s, t, os, ot;


	for (p=fa->polys ; p ; p=p->next)
	{
		glBegin (GL_POLYGON);
		for (i=0,v=p->verts[0] ; i<p->numverts ; i++, v+=VERTEXSIZE)
		{
			os = v[3];
			ot = v[4];

			s = os + turbsin[(int)((ot*0.125+cl.time) * TURBSCALE) & 255];
			s *= (1.0/64);

			t = ot + turbsin[(int)((os*0.125+cl.time) * TURBSCALE) & 255];
			t *= (1.0/64);

			glTexCoord2f (s, t);
			glVertex3fv (v);
		}
		glEnd ();
	}
}

/*
=============
EmitSkyPolys
=============
*/
void EmitSkyPolys (msurface_t *fa)
{
	glpoly_t	*p;
	float		*v;
	int			i;
	float	s, t;
	vec3_t	dir;
	float	length;

	for (p=fa->polys ; p ; p=p->next)
	{
		glBegin (GL_POLYGON);
		for (i=0,v=p->verts[0] ; i<p->numverts ; i++, v+=VERTEXSIZE)
		{
			VectorSubtract (v, r_origin, dir);
			dir[2] *= 3;	// flatten the sphere

			length = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
			length = sqrtf (length);
			length = 6*63/length;

			dir[0] *= length;
			dir[1] *= length;

			s = (speedscale + dir[0]) * (1.0f/128.0f);
			t = (speedscale + dir[1]) * (1.0f/128.0f);

			glTexCoord2f (s, t);
			glVertex3fv (v);
		}
		glEnd ();
	}
}

/*
===============
EmitBothSkyLayers

Does a sky warp on the pre-fragmented glpoly_t chain
This will be called for brushmodels, the world
will have them chained together.
===============
*/
void EmitBothSkyLayers (msurface_t *fa)
{

	GL_DisableMultitexture();

	GL_Bind (solidskytexture);

#ifdef SUPPORTS_KUROK
	if (kurok)
		speedscale = cl.time*2;
	else
#endif
	speedscale = cl.time*8;
	speedscale -= (int)speedscale & ~127 ;

	EmitSkyPolys (fa);

	glEnable (GL_BLEND);
	GL_Bind (alphaskytexture);
#ifdef SUPPORTS_KUROK
	if (kurok)
		speedscale = cl.time*4;
	else
#endif
	speedscale = cl.time*16;
	speedscale -= (int)speedscale & ~127 ;

	EmitSkyPolys (fa);

	glDisable (GL_BLEND);
}

/*
=================================================================

  PCX Loading

=================================================================
*/

#pragma pack(1)
typedef struct
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
} pcx_t;
#pragma pack()

static unsigned short SwapShort (unsigned short *SVal)
{
	byte *Buf = (byte *)SVal;

	return (unsigned short)(Buf[0] + Buf[1] * 256);
}

static void ChkBounds (char *Function, int Val, int MaxVal, char *FileName)
{
	if (Val > MaxVal)
		Sys_Error ("%s: invalid buffer access (%d, max = %d) in %s", Function, Val, MaxVal, FileName);
}

/*
============
LoadPCX
============
*/
static byte *LoadPCX (char *name, int *width, int *height, qboolean alphablend)
{
	pcx_t	    *pcx_header;
	FILE	    *f;
	byte	    *palette, *palid;
	byte	*pix;
	byte	    *pcx_rgb, *buf;
	int		x, y;
	int		dataByte, runLength;
	int	    count, rows, columns;
	char	    filename[MAX_OSPATH];
	qboolean    blend;

	sprintf (filename, "%s.pcx", name);
	COM_FOpenFile (filename, &f);

	if (!f)
		return NULL;

	if (com_filesize < sizeof(pcx_t))
		Sys_Error ("LoadPCX: can't read header in %s", filename);

	pcx_header = Q_malloc (com_filesize);

	if (!pcx_header)
		Sys_Error ("LoadPCX1: error allocating %d bytes for %s", com_filesize, filename);

	if (fread (pcx_header, 1, com_filesize, f) != com_filesize)
		Sys_Error ("LoadPCX: error reading %s", filename);

	fclose (f);

	// get palette
	palette = (byte *)pcx_header + com_filesize - 768;
	palid = palette - 1;

	if (*palid != 12)
	{
		// Palette id invalid; assume missing
		Con_SafePrintf ("LoadPCX: palette id invalid (%d, should be 12) in %s\n", *palid, name);
		++palid;
	}

//
// parse the PCX file
//
	if (pcx_header->manufacturer != 0x0a
		|| pcx_header->version != 5
		|| pcx_header->encoding != 1
		|| pcx_header->bits_per_pixel * pcx_header->color_planes != 8)
		Sys_Error ("LoadPCX: bad format in %s (mf=%d, ver=%d, enc=%d, bpp=%d, pal=%d)", filename, pcx_header->manufacturer,
			   pcx_header->version, pcx_header->encoding, pcx_header->bits_per_pixel * pcx_header->color_planes, *palid);

	pcx_header->xmin = SwapShort (&pcx_header->xmin);
	pcx_header->ymin = SwapShort (&pcx_header->ymin);
	pcx_header->xmax = SwapShort (&pcx_header->xmax);
	pcx_header->ymax = SwapShort (&pcx_header->ymax);
	pcx_header->xmin = SwapShort (&pcx_header->xmin);

	columns = pcx_header->xmax - pcx_header->xmin + 1;
	rows = pcx_header->ymax - pcx_header->ymin + 1;
	count = columns * rows;
	++count; // If I don't add 1 here, validation LoadPCX3 below will sometimes fire (?)

	pcx_rgb = Q_malloc (count * 4);

	if (!pcx_rgb)
		Sys_Error ("LoadPCX2: error allocating %d bytes for %s", count * 4, filename);

	blend = alphablend; //  && gl_alphablend.value;  Baker <--- that cvar isn't supported yet
	buf = (byte *)(pcx_header + 1);

	for (y=0 ; y<rows ; y++)
	{
		pix = pcx_rgb + 4*y*columns;
		for (x=0 ; x<columns ; )
		{
			ChkBounds ("LoadPCX1", buf - (byte *)pcx_header, palid - 1 - (byte *)pcx_header, filename);
			dataByte = *buf++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				ChkBounds ("LoadPCX2", buf - (byte *)pcx_header, palid - 1 - (byte *)pcx_header, filename);
				dataByte = *buf++;
			}
			else
				runLength = 1;

			ChkBounds ("LoadPCX3", pix - pcx_rgb + runLength * 4, count * 4, filename);

			while(runLength-- > 0)
			{
				pix[0] = palette[dataByte*3];
				pix[1] = palette[dataByte*3+1];
				pix[2] = palette[dataByte*3+2];
				pix[3] = blend ? (pix[0] + pix[1] + pix[2]) / 3 : 255; // Better way?
				pix += 4;
				x++;
			}
		}
	}

	*width = columns;
	*height = rows;

	free (pcx_header);

	return pcx_rgb;
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

#pragma pack(1)
typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;
#pragma pack()

/*
=============
LoadTGA
=============
*/
static byte *LoadTGA (char *name, int *width, int *height, qboolean alphablend)
{
	TargaHeader *targa_header;
	FILE	    *f;
	int	    columns, rows, numPixels;
	byte	    *pixbuf, *buf;
byte			*targa_rgba;
	int	    row, realrow, column;
	qboolean    upside_down, alpha, blend;
	char	    filename[MAX_OSPATH];

	sprintf (filename, "%s.tga", name);
	COM_FOpenFile (filename, &f);

	if (!f)
		return NULL;

	if (com_filesize < sizeof(TargaHeader))
		Sys_Error ("LoadTGA: can't read header in %s", filename);

	targa_header = Q_malloc (com_filesize);

	if (!targa_header)
		Sys_Error ("LoadTGA1: error allocating %d bytes for %s", com_filesize, filename);

	if (fread (targa_header, 1, com_filesize, f) != com_filesize)
		Sys_Error ("LoadTGA: error reading %s", filename);

	fclose (f);

	targa_header->width = SwapShort (&targa_header->width);
	targa_header->height = SwapShort (&targa_header->height);

	if (targa_header->image_type!=2
		&& targa_header->image_type!=10)
		Sys_Error ("LoadTGA: %s: only type 2 and 10 targa RGB images supported, not %d", filename, targa_header->image_type);

	if (targa_header->colormap_type !=0
		|| (targa_header->pixel_size!=32 && targa_header->pixel_size!=24))
		Sys_Error ("LoadTGA: %s: only 24 and 32 bit images supported (no colormaps, type=%d, bpp=%d)", filename, targa_header->colormap_type, targa_header->pixel_size);

	columns = targa_header->width;
	rows = targa_header->height;
	numPixels = columns * rows;
	upside_down = !(targa_header->attributes & 0x20); // true => picture is stored bottom to top

	targa_rgba = Q_malloc (numPixels*4);

	if (!targa_rgba)
		Sys_Error ("LoadTGA2: error allocating %d bytes for %s", numPixels*4, filename);

	blend = alphablend; // && gl_alphablend.value;
	alpha = targa_header->pixel_size == 32;

	buf = (byte *)(targa_header + 1);

	if (targa_header->id_length != 0)
		buf += targa_header->id_length; // skip TARGA image comment

	if (targa_header->image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			realrow = upside_down ? row : rows - 1 - row;
			pixbuf = targa_rgba + realrow*columns*4;

			ChkBounds ("LoadTGA1", buf - (byte *)targa_header + columns * targa_header->pixel_size / 8, com_filesize, filename);
			ChkBounds ("LoadTGA2", pixbuf - targa_rgba + columns * 4, numPixels * 4, filename);

			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				blue = *buf++;
				green = *buf++;
				red = *buf++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
				*pixbuf++ = alpha ? *buf++ : (blend ? (red + green + blue) / 3 : 255); // Better way?;
			}
		}
	}
	else if (targa_header->image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			realrow = upside_down ? row : rows - 1 - row;
			pixbuf = targa_rgba + realrow*columns*4;
			for(column=0; column<columns; ) {
				packetHeader = *buf++;
				packetSize = 1 + (packetHeader & 0x7f);

				ChkBounds ("LoadTGA3", pixbuf - targa_rgba + packetSize * 4, numPixels * 4, filename);

				if (packetHeader & 0x80) {        // run-length packet
					ChkBounds ("LoadTGA4", buf - (byte *)targa_header + targa_header->pixel_size / 8, com_filesize, filename);
					blue = *buf++;
					green = *buf++;
					red = *buf++;
					alphabyte = alpha ? *buf++ : (blend ? (red + green + blue) / 3 : 255); // Better way?;

					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							realrow = upside_down ? row : rows - 1 - row;
							pixbuf = targa_rgba + realrow*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					ChkBounds ("LoadTGA5", buf - (byte *)targa_header + packetSize * targa_header->pixel_size / 8, com_filesize, filename);
					for(j=0;j<packetSize;j++) {
						blue = *buf++;
						green = *buf++;
						red = *buf++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
						*pixbuf++ = alpha ? *buf++ : (blend ? (red + green + blue) / 3 : 255); // Better way?;
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							realrow = upside_down ? row : rows - 1 - row;
							pixbuf = targa_rgba + realrow*columns*4;
						}
					}
				}
			}
			breakOut:;
		}
	}

	*width = targa_header->width;
	*height = targa_header->height;

	free (targa_header);

	return targa_rgba;
}

/*
==================
RotateImage
==================
*/
static void RotateImage (int *Img, int width, int height, qboolean Left)
{
	int i, j, Size, *Rot;

	if (width != height)
	{
		Con_Printf ("RotateImage: image not symmetrical (%dx%d)\n", width, height);
		return;
	}

	Size = width * height * sizeof(int);

	if (!(Rot = Q_malloc (Size)))
	{
		Con_Printf ("RotateImage: not enough memory for %dk\n", Size / 1024);
		return;
	}

	for (i = 0; i < height; ++i)
	{
		for (j = 0; j < width; ++j)
		{
			if (Left)
				Rot[(height - 1 - j) * width + i] = Img[i * width + j];
			else
				Rot[j * width + (width - 1 - i)] = Img[i * width + j];
		}
	}

	memcpy (Img, Rot, Size);

	free (Rot);
}

/*
============
R_LoadImage
============
*/
byte *R_LoadImage (char *name, int *width, int *height, qboolean alphablend)
{
	byte *data = LoadTGA (name, width, height, alphablend);

	if (data)
		return data;

	return LoadPCX (name, width, height, alphablend);
}

/*
==================
R_LoadSkys
==================
*/
static char *suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
static int  suf_ord1[6] = {0, 1, 2, 3, 4, 5}; // Q2/VRML order
static int  suf_ord2[6] = {3, 0, 1, 2, 4, 5}; // Rotated 90 degrees left
static char skybox_name[MAX_QPATH] = ""; // name of current skybox
extern	int skyboxtextures;
extern	cvar_t r_oldsky;

void R_LoadSkys (char *skybox)
{
	int		i, j, *suf_ord;
	char		name[MAX_QPATH];
	byte		*data[6];
	qboolean	RotateSkyBox = false;
	static qboolean OldRotated = false;

#if 1 // Baker: determine implication of this
	if (skybox == NULL)
	{
		Con_DPrintf("Null skybox\n");
		return;
	}
#endif

#if 1
	if (skybox[0] == 0)
	{
		// Setting skybox to nothing
		strcpy (skybox_name, skybox);
		Con_DPrintf ("skybox set to \"%s\"\n", skybox);
		Cvar_SetValueByRef (&r_oldsky, 1);
		return;
	}
#endif

#if 0
	if (cl.worldmodel->name != NULL)
	{
		name[0] = '\0';

		COM_StripExtension (cl.worldmodel->name + 5, name);

		if (r_skytq.value || !stricmp(name, "invein"))
			RotateSkyBox = true; // Invein's skybox is invalidly rotated 90 degrees right
	}
#endif

	if (RotateSkyBox == OldRotated && !strcmp(skybox, skybox_name)) //no change
	{
		Cvar_SetValueByRef (&r_oldsky, skybox_name[0] == 0);
		return;
	}

	OldRotated = RotateSkyBox;

	// Make sure horizontal images are rotated correctly
	suf_ord = RotateSkyBox ? suf_ord2 : suf_ord1;

	for (i=0 ; i<6 ; i++)
	{
		snprintf (name, sizeof(name), "gfx/env/%s%s", skybox, suf[suf_ord[i]]);

		data[i] = R_LoadImage (name, &sky_width[i], &sky_height[i], false);

		if (!data[i])
		{
			Con_Printf ("Couldn't load %s\n", name);
			break;
		}

		// If entire skybox is rotated, up/down images must be rotated right/left
		if (i >= 4 && RotateSkyBox)
			RotateImage ((int *)data[i], sky_width[i], sky_height[i], i == 5);
	}

	if (i == 6)
	{
		for (j = 0; j < 6; ++j)
		{
			GL_Bind (skyboxtextures + j);
			snprintf (name, sizeof(name), "gfx/env/%s%s", skybox, suf[suf_ord[j]]);
			GL_Upload32 ((unsigned int *)data[j], sky_width[j], sky_height[j], TEX_NOFLAGS);
		}
	}

	for (j = 0; j < i; ++j)
		free (data[j]);

	if (i == 6)
	{
		strcpy (skybox_name, skybox);
		Con_DPrintf ("skybox set to %s%s\n", skybox, RotateSkyBox ? " and rotated" : "");
	}

	// Enable/disable skybox
	Cvar_SetValueByRef (&r_oldsky, skybox_name[0] == 0);
}

/*
=================
R_Sky_NewMap
=================
*/
void R_Sky_NewMap (void)
{
	float fog_density, fog_red, fog_green, fog_blue;
	char  key[MAX_KEY], value[MAX_VALUE];
	char  *data;

	// initially no skybox or fog
	Cvar_SetValueByRef (&r_oldsky, 1);

	// Default fog
	fog_density = 0.8;
	fog_red = fog_green = fog_blue = 0.3;
#ifdef SUPPORTS_FOG
	Cvar_SetValueByRef (&gl_fogenable, 0);
	Cvar_SetValueByRef (&gl_fogdensity, fog_density);
	Cvar_SetValueByRef (&gl_fogred, fog_red);
	Cvar_SetValueByRef (&gl_foggreen, fog_green);
	Cvar_SetValueByRef (&gl_fogblue, fog_blue);
#endif
	data = cl.worldmodel->entities;
	if (!data)
		return;

	data = COM_Parse(data);
	if (!data) //should never happen
		return; // error
	if (com_token[0] != '{') //should never happen
		return; // error
	while (1)
	{
		data = COM_Parse(data);
		if (!data)
			return; // error
		if (com_token[0] == '}')
			break; // end of worldspawn
		if (com_token[0] == '_')
			strcpy(key, com_token + 1); // Support "_sky" and "_fog" also
		else
			strcpy(key, com_token);
		while (key[strlen(key)-1] == ' ') // remove trailing spaces
			key[strlen(key)-1] = 0;
		data = COM_Parse(data);
		if (!data)
			return; // error
		strcpy(value, com_token);

		if (!strcmp("sky", key) && value[0])
			R_LoadSkys (value);
		else if (!strcmp("fog", key) && value[0])
		{
			sscanf(value, "%f %f %f %f", &fog_density, &fog_red, &fog_green, &fog_blue);
#ifdef SUPPORTS_FOG
			Cvar_SetValueByRef (&gl_fogenable, fog_density ? 1 : 0);
			Cvar_SetValueByRef (&gl_fogdensity, fog_density);
			Cvar_SetValueByRef (&gl_fogred, fog_red);
			Cvar_SetValueByRef (&gl_foggreen, fog_green);
			Cvar_SetValueByRef (&gl_fogblue, fog_blue);
#endif
		}
	}
}

/*
=================
R_SkyCommand_f
=================
*/
void R_SkyCommand_f (void)
{
	switch (Cmd_Argc())
	{
	case 1:
		Con_Printf("\"sky\" is \"%s\"\n", skybox_name);
		break;
	case 2:
		R_LoadSkys (Cmd_Argv(1));
		break;
	default:
		Con_Printf("usage: sky <skyname>\n");
	}
}

static vec3_t	skyclip[6] = {
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};

// 1 = s, 2 = t, 3 = 2048
static int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down
};

// s = [0]/[2], t = [1]/[2]
static int	vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}
};

static float	skymins[2][6], skymaxs[2][6];

static void DrawSkyPolygon (int nump, vec3_t vecs)
{
	int		i,j, axis;
	float	s, t, dv, *vp;
	vec3_t	v, av;

	// decide which face it maps to
	VectorClear (v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
		VectorAdd (vp, v, v);
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
		axis = (v[0] < 0) ? 1 : 0;
	else if (av[1] > av[2] && av[1] > av[0])
		axis = (v[1] < 0) ? 3 : 2;
	else
		axis = (v[2] < 0) ? 5 : 4;

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		j = vec_to_st[axis][2];
		dv = (j > 0) ? vecs[j - 1] : -vecs[-j - 1];

		j = vec_to_st[axis][0];
		s = (j < 0) ? -vecs[-j -1] / dv : vecs[j-1] / dv;

		j = vec_to_st[axis][1];
		t = (j < 0) ? -vecs[-j -1] / dv : vecs[j-1] / dv;

		if (s < skymins[0][axis])
			skymins[0][axis] = s;
		if (t < skymins[1][axis])
			skymins[1][axis] = t;
		if (s > skymaxs[0][axis])
			skymaxs[0][axis] = s;
		if (t > skymaxs[1][axis])
			skymaxs[1][axis] = t;
	}
}

#define	MAX_CLIP_VERTS	64
static void ClipSkyPolygon (int nump, vec3_t vecs, int stage)
{
	float	*norm, *v, d, e, dists[MAX_CLIP_VERTS];
	qboolean	front, back;
	int		sides[MAX_CLIP_VERTS], newc[2], i, j;
	vec3_t	newv[2][MAX_CLIP_VERTS];

	if (nump > MAX_CLIP_VERTS-2)
		Sys_Error ("ClipSkyPolygon: nump > MAX_CLIP_VERTS - 2");

	if (stage == 6)
	{	// fully clipped, so draw it
		DrawSkyPolygon (nump, vecs);
		return;
	}

	front = back = false;
	norm = skyclip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;

		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;

		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v[j] + d*(v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}

/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain (msurface_t *s)
{
	if (r_oldsky.value)
	{
	msurface_t	*fa;

		GL_DisableMultitexture();

		// used when gl_texsort is on
		GL_Bind(solidskytexture);
		speedscale = cl.time*8;
		speedscale -= (int)speedscale & ~127 ;

		for (fa=s ; fa ; fa=fa->texturechain)
			EmitSkyPolys (fa);

		glEnable (GL_BLEND);

		GL_Bind (alphaskytexture);
		speedscale = cl.time*16;
		speedscale -= (int)speedscale & ~127 ;

		for (fa=s ; fa ; fa=fa->texturechain)
			EmitSkyPolys (fa);

		glDisable (GL_BLEND);
	}
	else
{
	msurface_t	*fa;
	int		i;
		vec3_t	    verts[MAX_CLIP_VERTS], origin;
	glpoly_t	*p;

	// calculate vertex values for sky box

		VectorCopy (r_origin, origin);
		origin[0] += 0.001; // Offset slightly horizontally to avoid initial top side distortion, kludge

		for (fa=s ; fa ; fa=fa->texturechain)
		{
			for (p=fa->polys ; p ; p=p->next)
			{
			for (i=0 ; i<p->numverts ; i++)
					VectorSubtract (p->verts[i], origin, verts[i]);

			ClipSkyPolygon (p->numverts, verts[0], 0);
		}
	}
}
}


/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox (void)
{
	int		i;

	if (r_oldsky.value)
		return;

	for (i=0 ; i<6 ; i++)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}
}

static void MakeSkyVec (float s, float t, int axis)
{
	int	j, k, farclip;
	float	w, h;
	vec3_t		v, b;

#if 1
	farclip = QMAX((int)r_farclip.value, 4096);
	b[0] = s * (farclip >> 1);
	b[1] = t * (farclip >> 1);
	b[2] = (farclip >> 1);
#endif
#if 0 // aguirRe style
	// Compromise; big enough to avoid sky covering world,
	// but small enough to avoid hiding skyboxes in fog
	farclip = gl_fogenable.value ? gl_skyclip.value : GL_FARCLIP;

	if (farclip > GL_FARCLIP)
		farclip = GL_FARCLIP;

	b[0] = s * farclip / 2;
	b[1] = t * farclip / 2;
	b[2] = farclip / 2;
#endif

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
		v[j] += r_origin[j];
	}

	// avoid bilerp seam
	s = (s+1)*0.5;
	t = (t+1)*0.5;

	w = sky_width[axis];
	h = sky_height[axis];

	// Empirical tests to get good results in most combinations.
	// Maybe possible to have one formula for all
#ifdef DX8QUAKE_GET_GL_MAX_SIZE

	if (w < 256 && gl_max_size > 256)
	{
		s = s * (w-1)/w + 0.5/w;
		t = t * (h-1)/h + 0.5/h;
	}
	else
	{
		int size = gl_max_size * 3 / 2;
#else
	if (w < 256 && gl_max_size.value > 256)
	{
		s = s * (w-1)/w + 0.5/w;
		t = t * (h-1)/h + 0.5/h;
	}
	else
	{
		int size = gl_max_size.value * 3 / 2;


#endif
		if (size > 512)
			size = 512;

		w = QMIN(w, (float)size);
		h = QMIN(h, (float)size);

		if (s < 1/w)
			s = 1/w;
		else if (s > (w-1)/w)
			s = (w-1)/w;
		if (t < 1/h)
			t = 1/h;
		else if (t > (h-1)/h)
			t = (h-1)/h;
	}

	t = 1.0 - t;
	glTexCoord2f (s, t);
	glVertex3fv (v);
}

/*
==============
R_DrawSkyBox
==============
*/
static int skytexorder[6] = {0,2,1,3,4,5};
extern msurface_t  *skychain;
void R_DrawSkyBox (void)
{
	int		i, j, k;
	float	s, t;
	vec3_t	v;
	msurface_t  *fa;
	if (r_oldsky.value)
		return;

#if !defined(D3DQUAKE)
	if (gl_mtexable)
	{
		// Non-transparent skybox only works in multitexture for now
		if (!skychain)
			return;

		R_ClearSkyBox ();
		R_DrawSkyChain (skychain);

		GL_DisableMultitexture ();
	}
#endif

	for (i=0 ; i<6 ; i++)
	{
		if (skymins[0][i] >= skymaxs[0][i] || skymins[1][i] >= skymaxs[1][i])
			continue;

		GL_Bind (skyboxtextures + skytexorder[i]);
		glBegin (GL_QUADS);
		MakeSkyVec (skymins[0][i], skymins[1][i], i);
		MakeSkyVec (skymins[0][i], skymaxs[1][i], i);
		MakeSkyVec (skymaxs[0][i], skymaxs[1][i], i);
		MakeSkyVec (skymaxs[0][i], skymins[1][i], i);
		glEnd ();
	}

#if !defined(D3DQUAKE)
	if (!gl_mtexable)
		return;

	// Non-transparent skybox only works in multitexture for now
	glDisable (GL_TEXTURE_2D);
	glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable (GL_BLEND);
	glBlendFunc (GL_ZERO, GL_ONE);

	for (fa = skychain ; fa ; fa = fa->texturechain)
		EmitFlatPoly (fa);

	glEnable (GL_TEXTURE_2D);
	glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	skychain = NULL;
#endif
}

//===============================================================

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (texture_t *mt)
{
	int		i, j, p, scaledx;
	byte		*src, fixedsky[256 * 128];
	unsigned	trans[128*128];
	unsigned	transpix;
	int			r, g, b;
	unsigned	*rgba;

	src = (byte *)mt + mt->offsets[0];

	if (mt->width * mt->height != sizeof(fixedsky))
	{
		Con_DPrintf ("\002R_InitSky: ");
		Con_DPrintf ("non-standard sky texture '%s' (%dx%d, should be 256x128)\n", mt->name, mt->width, mt->height);

		// Resize sky texture to correct size
		memset (fixedsky, 0, sizeof(fixedsky));

		for (i = 0; i < 256; ++i)
		{
			scaledx = i * mt->width / 256 * mt->height;

			for (j = 0; j < 128; ++j)
				fixedsky[i * 128 + j] = src[scaledx + j * mt->height / 128];
		}

		src = fixedsky;
	}

	// make an average value for the back to avoid
	// a fringe on the top level

	r = g = b = 0;
	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			p = src[i*256 + j + 128];
			rgba = &d_8to24table[p];
			trans[(i*128) + j] = *rgba;
			r += ((byte *)rgba)[0];
			g += ((byte *)rgba)[1];
			b += ((byte *)rgba)[2];
		}

	((byte *)&transpix)[0] = r/(128*128);
	((byte *)&transpix)[1] = g/(128*128);
	((byte *)&transpix)[2] = b/(128*128);
	((byte *)&transpix)[3] = 0;


	if (!solidskytexture)
		solidskytexture = texture_extension_number++;
	GL_Bind (solidskytexture );
	glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			p = src[i*256 + j];
			if (p == 0)
				trans[(i*128) + j] = transpix;
			else
				trans[(i*128) + j] = d_8to24table[p];
		}

	if (!alphaskytexture)
		alphaskytexture = texture_extension_number++;
	GL_Bind(alphaskytexture);
	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

