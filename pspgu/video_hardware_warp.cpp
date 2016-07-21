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
// gl_warp.c -- sky and water polygons

#include <pspgu.h>
#include <pspkernel.h>
//#include <malloc.h>

extern "C"
{
#include "../quakedef.h"
}

#include "clipping.hpp"

using namespace quake;

extern	model_t	*loadmodel;

//int		skytexturenum;

int		solidskytexture	= -1;
int		alphaskytexture	= -1;

int     sky_rt	= -1;
int     sky_bk	= -1;
int     sky_lf	= -1;
int     sky_ft	= -1;
int     sky_up	= -1;
int     sky_dn	= -1;

int	skytexorder[6] = {0,2,1,3,4,5};
//int	    skytexorder[6] = {3,1,2,0,4,5}; // The order skybox images are drawn.
int	    skyimage[6]; // Where sky images are stored
char	skybox_name[32] = ""; //name of current skybox, or "" if no skybox
char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

// skybox pallete

byte		*host_skybasepal;

void    VID_InitSkyPalette(unsigned char* palette);

void 	VID_SetPaletteToSky();
// switch palette for sky

void 	VID_SetGlobalPalette();
// switch palette for textures

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
	float	s, t;

	if (numverts > 60)
		Sys_Error ("SubdividePolygon: excessive numverts %i", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++)
	{
		m = (mins[i] + maxs[i]) * 0.5;
        m = gl_subdivide_size.value * floorf (m/gl_subdivide_size.value + 0.5);

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
		return;
	}

	poly = static_cast<glpoly_t*>(Hunk_Alloc (sizeof(glpoly_t) + (numverts - 1) * sizeof(glvert_t)));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts;
	for (i=0 ; i<numverts ; i++, verts+= 3)
	{
		VectorCopy (verts, poly->verts[i].xyz);
		s = DotProduct (verts, warpface->texinfo->vecs[0]);
		t = DotProduct (verts, warpface->texinfo->vecs[1]);
		poly->verts[i].st[0] = s;
		poly->verts[i].st[1] = t;
	}
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
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon (numverts, verts[0]);
}

/*
================
GL_Surface
================
*/
void GL_Surface (msurface_t *fa)
{
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float		*vec;
	glpoly_t	*poly;
//	float		texscale;
	float	s, t;

//	texscale = (1.0/32.0);

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	//create the poly
	poly = static_cast<glpoly_t*>(Hunk_Alloc (sizeof(glpoly_t) + (numverts - 1) * sizeof(glvert_t)));
	poly->next = NULL;
	fa->polys = poly;
	poly->numverts = numverts;
	for (i=0, vec=(float *)verts; i<numverts; i++, vec+= 3)
	{
		VectorCopy (vec, poly->verts[i].xyz);
		s = DotProduct(vec, fa->texinfo->vecs[0]);// * texscale;
		t = DotProduct(vec, fa->texinfo->vecs[1]);// * texscale;
		poly->verts[i].st[0] = s;
		poly->verts[i].st[1] = t;
	}
}

//=========================================================



// speed up sin calculations - Ed
float	turbsin[] =
{
	#include "../gl_warp_sin.h"
};

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void EmitWaterPolys (msurface_t *fa)
{
	const float real_time	= static_cast<float>(realtime);
	const float scale		= (1.0f / 64);
	const float turbscale	= (256.0f / (2.0f * static_cast<float>(M_PI)));

	// For each polygon...
	for (const glpoly_t* p = fa->polys; p; p = p->next)
	{
		// Allocate memory for this polygon.
		const int		unclipped_vertex_count	= p->numverts;
		glvert_t* const	unclipped_vertices		=
			static_cast<glvert_t*>(sceGuGetMemory(sizeof(glvert_t) * unclipped_vertex_count));

		// Generate each vertex.
		const glvert_t*	src			= p->verts;
		const glvert_t*	last_vertex = src + unclipped_vertex_count;
		glvert_t*		dst			= unclipped_vertices;

		while (src != last_vertex)
		{
			// Get the input UVs.
			const float	os = src->st[0];
			const float	ot = src->st[1];

			// Fill in the vertex data.
			dst->st[0] = (os + turbsin[(int) ((ot * 0.025f + real_time) * turbscale) & 255]) * scale;
			dst->st[1] = (ot + turbsin[(int) ((os * 0.025f + real_time) * turbscale) & 255]) * scale;
			dst->xyz[0] = src->xyz[0];
			dst->xyz[1] = src->xyz[1];
			dst->xyz[2] = src->xyz[2];

			// Next vertex.
			++src;
			++dst;
		}

		// Do these vertices need clipped?
		if (clipping::is_clipping_required(unclipped_vertices, unclipped_vertex_count))
		{
			// Clip the polygon.
			const glvert_t*	clipped_vertices;
			std::size_t		clipped_vertex_count;
			clipping::clip(
				unclipped_vertices,
				unclipped_vertex_count,
				&clipped_vertices,
				&clipped_vertex_count);

			// Any vertices left?
			if (clipped_vertex_count)
			{
				// Copy the vertices to the display list.
				const std::size_t buffer_size = clipped_vertex_count * sizeof(glvert_t);
				glvert_t* const display_list_vertices = static_cast<glvert_t*>(sceGuGetMemory(buffer_size));
				memcpy(display_list_vertices, clipped_vertices, buffer_size);

				// Draw the clipped vertices.
				sceGuDrawArray(
					GU_TRIANGLE_FAN,
					GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
					clipped_vertex_count, 0, display_list_vertices);
			}
		}
		else
		{
			// Draw the vertices.
			sceGuDrawArray(
				GU_TRIANGLE_FAN,
				GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
				unclipped_vertex_count, 0, unclipped_vertices);
		}
	}
}


/*
=============
EmitReflectivePolys

Does a reflective warp on the pre-fragmented glpoly_t chain
=============
*/
void EmitReflectivePolys (msurface_t *fa)
{
	// For each polygon...
	for (const glpoly_t* p = fa->polys; p; p = p->next)
	{
		// Allocate memory for this polygon.
		const int		unclipped_vertex_count	= p->numverts;
		glvert_t* const	unclipped_vertices		=
			static_cast<glvert_t*>(sceGuGetMemory(sizeof(glvert_t) * unclipped_vertex_count));

		// Generate each vertex.
		const glvert_t*	src			= p->verts;
		const glvert_t*	last_vertex = src + unclipped_vertex_count;
		glvert_t*		dst			= unclipped_vertices;

		while (src != last_vertex)
		{
			vec3_t	dir;
			VectorSubtract(src->xyz, r_origin, dir);
			dir[2] *= 3;	// flatten the sphere

			const float length = 6 * 63 / sqrtf(DotProduct(dir, dir));

			dir[0] *= length;
			dir[1] *= length;

			dst->st[0] = (dir[0]) * (1.0f / 256.0f);
			dst->st[1] = (dir[1]) * (1.0f / 256.0f);
			dst->xyz[0] = src->xyz[0];
			dst->xyz[1] = src->xyz[1];
			dst->xyz[2] = src->xyz[2];

			// Next vertex.
			++src;
			++dst;
		}

		// Do these vertices need clipped?
		if (clipping::is_clipping_required(unclipped_vertices, unclipped_vertex_count))
		{
			// Clip the polygon.
			const glvert_t*	clipped_vertices;
			std::size_t		clipped_vertex_count;
			clipping::clip(
				unclipped_vertices,
				unclipped_vertex_count,
				&clipped_vertices,
				&clipped_vertex_count);

			// Any vertices left?
			if (clipped_vertex_count)
			{
				// Copy the vertices to the display list.
				const std::size_t buffer_size = clipped_vertex_count * sizeof(glvert_t);
				glvert_t* const display_list_vertices = static_cast<glvert_t*>(sceGuGetMemory(buffer_size));
				memcpy(display_list_vertices, clipped_vertices, buffer_size);

				// Draw the clipped vertices.
				sceGuDrawArray(
					GU_TRIANGLE_FAN,
					GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
					clipped_vertex_count, 0, display_list_vertices);
			}
		}
		else
		{
			// Draw the vertices.
			sceGuDrawArray(
				GU_TRIANGLE_FAN,
				GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
				unclipped_vertex_count, 0, unclipped_vertices);
		}
	}
}

/*
=============
EmitSkyPolys
=============
*/
void EmitSkyPolys (msurface_t *fa)
{

  if (!kurok)
  {

	for (const glpoly_t* p = fa->polys; p; p = p->next)
	{

		glvert_t* const vertices = static_cast<glvert_t*>(sceGuGetMemory(sizeof(glvert_t) * p->numverts));

		const glvert_t*	src			= p->verts;
		const glvert_t*	last_vertex = src + p->numverts;
		glvert_t*		dst			= vertices;

		while (src != last_vertex)
		{
			vec3_t	dir;
			VectorSubtract(src->xyz, r_origin, dir);
			dir[2] *= 3;	// flatten the sphere

			const float length = 6 * 63 / sqrtf(DotProduct(dir, dir));

			dir[0] *= length;
			dir[1] *= length;

			dst->st[0] = (speedscale + dir[0]) * (1.0f / 128.0f);
			dst->st[1] = (speedscale + dir[1]) * (1.0f / 128.0f);
			dst->xyz[0] = src->xyz[0];
			dst->xyz[1] = src->xyz[1];
			dst->xyz[2] = src->xyz[2];

			// Next vertex.
			++src;
			++dst;
		}

		sceGuDrawArray(GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_VERTEX_32BITF, p->numverts, 0, vertices);
	}
  }
  else
  {

    for (const glpoly_t* p = fa->polys; p; p = p->next)
	{

		// Allocate memory for this polygon.
		const int		unclipped_vertex_count	= p->numverts;
		glvert_t* const	unclipped_vertices		=
			static_cast<glvert_t*>(sceGuGetMemory(sizeof(glvert_t) * unclipped_vertex_count));

		// Generate each vertex.
		const glvert_t*	src			= p->verts;
		const glvert_t*	last_vertex = src + unclipped_vertex_count;
		glvert_t*		dst			= unclipped_vertices;

		while (src != last_vertex)
		{
			vec3_t	dir;
			VectorSubtract(src->xyz, r_origin, dir);
			dir[2] *= 3;	// flatten the sphere

			const float length = 6 * 63 / sqrtf(DotProduct(dir, dir));

			dir[0] *= length;
			dir[1] *= length;

			dst->st[0] = (speedscale + dir[0]) * (1.0f / 128.0f);
			dst->st[1] = (speedscale + dir[1]) * (1.0f / 128.0f);
			dst->xyz[0] = src->xyz[0];
			dst->xyz[1] = src->xyz[1];
			dst->xyz[2] = src->xyz[2];

			// Next vertex.
			++src;
			++dst;
		}

		// Do these vertices need clipped?
		if (clipping::is_clipping_required(unclipped_vertices, unclipped_vertex_count))
		{
			// Clip the polygon.
			const glvert_t*	clipped_vertices;
			std::size_t		clipped_vertex_count;
			clipping::clip(
				unclipped_vertices,
				unclipped_vertex_count,
				&clipped_vertices,
				&clipped_vertex_count);

			// Any vertices left?
			if (clipped_vertex_count)
			{
				// Copy the vertices to the display list.
				const std::size_t buffer_size = clipped_vertex_count * sizeof(glvert_t);
				glvert_t* const display_list_vertices = static_cast<glvert_t*>(sceGuGetMemory(buffer_size));
				memcpy(display_list_vertices, clipped_vertices, buffer_size);

				// Draw the clipped vertices.
				sceGuDrawArray(
					GU_TRIANGLE_FAN,
					GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
					clipped_vertex_count, 0, display_list_vertices);
			}
		}

		else
		{
			// Draw the vertices.
			sceGuDrawArray(
				GU_TRIANGLE_FAN,
				GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
				unclipped_vertex_count, 0, unclipped_vertices);
        }
	}
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
	GL_Bind (solidskytexture);

#ifdef SUPPORTS_KUROK
	if (kurok)
		speedscale = cl.time*2;
	else
#endif
		speedscale = cl.time*8;

	speedscale -= (int)speedscale & ~127 ;

	EmitSkyPolys (fa);

	sceGuEnable(GU_BLEND);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	GL_Bind (alphaskytexture);
#ifdef SUPPORTS_KUROK
	if (kurok)
		speedscale = cl.time*4;
	else
#endif
		speedscale = cl.time*16;

	speedscale -= (int)speedscale & ~127 ;

	EmitSkyPolys (fa);

	sceGuDisable(GU_BLEND);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
}

/*
=================================================================

  PCX Loading

=================================================================
*/

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
    unsigned 	data;			// unbounded
} pcx_t;

byte	*pcx_rgb;

/*
============
LoadPCX
============
*/

void LoadPCX (FILE *f)
{
	pcx_t	*pcx, pcxbuf;
	byte	palette[768];
	byte	*pix;
	int		x, y;
	int		dataByte, runLength;
	int		count;

//
// parse the PCX file
//
	fread (&pcxbuf, 1, sizeof(pcxbuf), f);

	pcx = &pcxbuf;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 320
		|| pcx->ymax >= 256)
	{
		Con_Printf ("Bad pcx file\n");
		return;
	}

	// seek to palette
	fseek (f, -768, SEEK_END);
	fread (palette, 1, 768, f);

	fseek (f, sizeof(pcxbuf) - 4, SEEK_SET);

	count = (pcx->xmax+1) * (pcx->ymax+1);
//	pcx_rgb = malloc( count * 4);
    pcx_rgb = static_cast<byte*>(Q_malloc(count*4));

	for (y=0 ; y<=pcx->ymax ; y++)
	{
		pix = pcx_rgb + 4*y*(pcx->xmax+1);
		for (x=0 ; x<=pcx->ymax ; )
		{
			dataByte = fgetc(f);

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = fgetc(f);
			}
			else
				runLength = 1;

			while(runLength-- > 0)
			{
				pix[0] = palette[dataByte*3];
				pix[1] = palette[dataByte*3+1];
				pix[2] = palette[dataByte*3+2];
				pix[3] = 255;
				pix += 4;
				x++;
			}
		}
	}
}


/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


TargaHeader		targa_header;
byte			*targa_rgba;

int fgetLittleShort (FILE *f)
{
	byte	b1, b2;

	b1 = fgetc(f);
	b2 = fgetc(f);

	return (short)(b1 + b2*256);
}

int fgetLittleLong (FILE *f)
{
	byte	b1, b2, b3, b4;

	b1 = fgetc(f);
	b2 = fgetc(f);
	b3 = fgetc(f);
	b4 = fgetc(f);

	return b1 + (b2<<8) + (b3<<16) + (b4<<24);
}


/*
=============
LoadTGA
=============
*/

void LoadTGA (FILE *fin)
{
	int				columns, rows, numPixels;
	byte			*pixbuf;
	int				row, column;

	targa_header.id_length = fgetc(fin);
	targa_header.colormap_type = fgetc(fin);
	targa_header.image_type = fgetc(fin);

	targa_header.colormap_index = fgetLittleShort(fin);
	targa_header.colormap_length = fgetLittleShort(fin);
	targa_header.colormap_size = fgetc(fin);
	targa_header.x_origin = fgetLittleShort(fin);
	targa_header.y_origin = fgetLittleShort(fin);
	targa_header.width = fgetLittleShort(fin);
	targa_header.height = fgetLittleShort(fin);
	targa_header.pixel_size = fgetc(fin);
	targa_header.attributes = fgetc(fin);

	if (targa_header.image_type!=2
		&& targa_header.image_type!=10)
		Sys_Error ("LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type !=0
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
		Sys_Error ("Texture_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

//	targa_rgba = Q_malloc (numPixels*4);
    targa_rgba = static_cast<byte*>(Q_malloc(numPixels*4));

	if (targa_header.id_length != 0)
		fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment

	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:

							blue = getc(fin);
							green = getc(fin);
							red = getc(fin);
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = getc(fin);
							green = getc(fin);
							red = getc(fin);
							alphabyte = getc(fin);
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader=getc(fin);
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = getc(fin);
								green = getc(fin);
								red = getc(fin);
								alphabyte = 255;
								break;
						case 32:
								blue = getc(fin);
								green = getc(fin);
								red = getc(fin);
								alphabyte = getc(fin);
								break;
					}

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
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = getc(fin);
									green = getc(fin);
									red = getc(fin);
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = getc(fin);
									green = getc(fin);
									red = getc(fin);
									alphabyte = getc(fin);
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
			}
			breakOut:;
		}
	}

	fclose(fin);
}

/*
==================
R_LoadSkys
Based on fitzquake implementation, modified by MDave.
==================
*/
void Sky_LoadSkyBox (char *name)
{
	int		i, mark;
    byte    *f;
	char	filename[64],skypalette[64];
	bool nonefound = true;

#ifdef NORMAL_MODEL
    char   size[64] = "_";
#endif
#ifdef SLIM_MODEL
    char   size[64] = "_512_";
#endif

	if (strcmp(skybox_name, name) == 0)
		return; //no change

	//purge old textures
	for (i=0; i<6; i++)
	{
	    if (skyimage[i] && skyimage[i] != solidskytexture)
            GL_UnloadTexture(skyimage[i]);
		skyimage[i] = NULL;
	}

	//turn off skybox if sky is set to ""
	if (name[0] == 0)
	{
		skybox_name[0] = 0;
		return;
	}

    // sky palette
    snprintf(skypalette, sizeof(skypalette),  "gfx/env/%s%spalette.lmp", name, size);
    host_skybasepal = static_cast<byte*>(COM_LoadHunkFile (skypalette));
    if (!host_skybasepal)
    {
#ifdef  SLIM_MODEL
        char    size[64] = "_";
        snprintf(skypalette, sizeof(skypalette),  "gfx/env/%s%spalette.lmp", name, size);
        host_skybasepal = static_cast<byte*>(COM_LoadHunkFile (skypalette));

        if (!host_skybasepal)
        {
            Con_Printf ("Couldn't load %s%spalette.lmp, using default palette.\n", name, size);
            host_skybasepal = host_basepal;
        }
#endif
#ifdef  NORMAL_MODEL
        Con_Printf ("Couldn't load %s%spalette.lmp, using default palette.\n", name, size);
        host_skybasepal = host_basepal;
#endif
    }

    VID_InitSkyPalette(host_skybasepal);

    for (i=0 ; i<6 ; i++)
    {
        mark = Hunk_LowMark ();
        snprintf(filename, sizeof(filename),  "gfx/env/%s%s%s.lmp", name, size, suf[i]);
        f = static_cast<byte*>(COM_LoadHunkFile (filename));

#ifdef  SLIM_MODEL
        if (f) // Load up 512 x 512 sky textures if on slim psp.
        {
            if (!strncmp(size,"_512_",5))
                skyimage[i] = GL_LoadTexture ("", 512, 512, f+8, qfalse, GU_LINEAR, 0);
            else // If no 512 x 512 textures are available, use 256 x 256.
                skyimage[i] = GL_LoadTexture ("", 256, 256, f+8, qfalse, GU_LINEAR, 0);

            nonefound = false;
        }
#endif
#ifdef  NORMAL_MODEL
        if (f) // Load up 256 x 256 sky textures if on phat psp.
        {
            skyimage[i] = GL_LoadTexture ("", 256, 256, f+8, qfalse, GU_LINEAR, 0);
            nonefound = false;
        }
#endif
#ifdef  SLIM_MODEL
        else // See if theres 256 x 256 sky textures available at least before giving up.
        {
            char    size[64] = "_";
            snprintf(filename, sizeof(filename),  "gfx/env/%s%s%s.lmp", name, size, suf[i]);
            f = static_cast<byte*>(COM_LoadHunkFile (filename));

            if (f) // Last try
            {
                skyimage[i] = GL_LoadTexture ("", 256, 256, f+8, qfalse, GU_LINEAR, 0);
                nonefound = false;
            }
            else
            {
                Con_Printf ("Couldn't load %s%s\n", name, size);
                skyimage[i] = solidskytexture;
            }
        }
#endif
#ifdef  NORMAL_MODEL
        else
        {
            Con_Printf ("Couldn't load %s%s\n", name, size);
            skyimage[i] = solidskytexture;
        }
#endif
        Hunk_FreeToLowMark (mark);
    }

	if (nonefound) // go back to scrolling sky if skybox is totally missing (should never happen for kurok).
	{
		for (i=0; i<6; i++)
		{
		    if (skyimage[i] && skyimage[i] != solidskytexture)
                GL_UnloadTexture(skyimage[i]);
			skyimage[i] = NULL;
		}
		skybox_name[0] = 0;
		return;
	}

    strcpy(skybox_name, name);
}

/*
=================
Sky_NewMap
=================
*/
void Sky_NewMap (void)
{
	char	key[128], value[4096];
	char	*data;
	int		i;

	//purge old textures
	for (i=0; i<6; i++)
	{
	    if (skyimage[i] && skyimage[i] != 19)
            GL_UnloadTexture(skyimage[i]);
		skyimage[i] = NULL;
	}

	//
	// initially no sky
	//

	Sky_LoadSkyBox ("");

	//
	// read worldspawn (this is so ugly, and shouldn't it be done on the server?)
	//
	data = cl.worldmodel->entities;
	if (!data)
		return; //FIXME: how could this possibly ever happen? -- if there's no
	// worldspawn then the sever wouldn't send the loadmap message to the client

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
			strcpy(key, com_token + 1);
		else
			strcpy(key, com_token);
		while (key[strlen(key)-1] == ' ') // remove trailing spaces
			key[strlen(key)-1] = 0;
		data = COM_Parse(data);
		if (!data)
			return; // error
		strcpy(value, com_token);

        if (kurok)
        {
            if (!strcmp("sky", key))
                Sky_LoadSkyBox(value);
            else if (!strcmp("", skybox_name)) // Kurok loads a sky by default if none is specified
                Sky_LoadSkyBox ("sky");
        }
        else
        {
            if (!strcmp("sky", key))
                Sky_LoadSkyBox(value);

#if 1 //also accept non-standard keys
            else if (!strcmp("skyname", key)) //half-life
                Sky_LoadSkyBox(value);
            else if (!strcmp("qlsky", key)) //quake lives
                Sky_LoadSkyBox(value);
#endif
        }
	}
}

/*
=================
Sky_SkyCommand_f
=================
*/
void Sky_SkyCommand_f (void)
{
	switch (Cmd_Argc())
	{
	case 1:
		Con_Printf("\"sky\" is \"%s\"\n", skybox_name);
		break;
	case 2:
		Sky_LoadSkyBox(Cmd_Argv(1));
		break;
	default:
		Con_Printf("usage: sky <skyname>\n");
	}
}

/*
=============
Sky_Init
=============
*/
void Sky_Init (void)
{
	int		i;

	Cmd_AddCommand ("sky",Sky_SkyCommand_f);

	for (i=0; i<6; i++)
		skyimage[i] = NULL;
}

static vec3_t	skyclip[6] = {
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};
int	c_sky;

// 1 = s, 2 = t, 3 = 2048
static int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down

//	{-1,2,3},
//	{1,2,-3}
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

//	{-1,2,3},
//	{1,2,-3}
};

static float	skymins[2][6], skymaxs[2][6];

static void DrawSkyPolygon (int nump, vec3_t vecs)
{
	int		i,j, axis;
	float	s, t, dv, *vp;
	vec3_t	v, av;


	c_sky++;

	// decide which face it maps to
	VectorCopy (vec3_origin, v);
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
	float	*norm;
	float	*v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		Sys_Error ("ClipSkyPolygon: nump > MAX_CLIP_VERTS - 2");
	if (stage == 6)
	{	// fully clipped, so draw it
		DrawSkyPolygon (nump, vecs);
		return;
	}

	front = back = qfalse;
	norm = skyclip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = qtrue;
			sides[i] = SIDE_FRONT;
		}
		else if (d < ON_EPSILON)
		{
			back = qtrue;
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
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox (void)
{
	int		i;

	for (i=0 ; i<6 ; i++)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}
}

float s_axis;
float t_axis;
vec3_t v_axis;

void MakeSkyVec (float s, float t, int axis)
{
	vec3_t		b;
	int			j, k;

	b[0] = s*r_skyclip.value;
	b[1] = t*r_skyclip.value;
	b[2] = r_skyclip.value;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v_axis[j] = -b[-k - 1];
		else
			v_axis[j] = b[k - 1];
		v_axis[j] += r_origin[j];
	}

	// avoid bilerp seam
	s = (s+1.0f)*0.5f;
	t = (t+1.0f)*0.5f;

	if (s < 1.0f/512.0f)
		s = 1.0f/512.0f;
	else if (s > 511.0f/512.0f)
		s = 511.0f/512.0f;

	if (t < 1.0f/512.0f)
		t = 1.0f/512.0f;
	else if (t > 511.0f/512.0f)
		t = 511.0f/512.0f;

    // Stupid Sony leaving hardware bugs in phat psp's, they don't like the t axis flipped so the images have to be pre-flipped, sigh ...
//	t = 1.0f - t;

	s_axis = s;
	t_axis = t;
}

/*
==============
R_DrawSkyBox
==============
*/
void R_DrawSkyBox (void)
{
    int		i;
    float   r,g,b,a;
	for (i=0 ; i<6 ; i++)
	{
		// Allocate memory for this polygon.
		const int		unclipped_vertex_count	= 4;
		glvert_t* const	unclipped_vertices		=
			static_cast<glvert_t*>(sceGuGetMemory(sizeof(glvert_t) * unclipped_vertex_count));

		if (skymins[0][i] >= skymaxs[0][i]
		|| skymins[1][i] >= skymaxs[1][i])
			continue;

        GL_Bind (skyimage[skytexorder[i]]);

		MakeSkyVec (skymins[0][i], skymins[1][i], i);

        unclipped_vertices[0].st[0]	    = s_axis;
        unclipped_vertices[0].st[1]	    = t_axis;
        unclipped_vertices[0].xyz[0]	= v_axis[0];
        unclipped_vertices[0].xyz[1]	= v_axis[1];
        unclipped_vertices[0].xyz[2]	= v_axis[2];

		MakeSkyVec (skymins[0][i], skymaxs[1][i], i);

        unclipped_vertices[1].st[0]	    = s_axis;
        unclipped_vertices[1].st[1]	    = t_axis;
        unclipped_vertices[1].xyz[0]	= v_axis[0];
        unclipped_vertices[1].xyz[1]	= v_axis[1];
        unclipped_vertices[1].xyz[2]	= v_axis[2];

		MakeSkyVec (skymaxs[0][i], skymaxs[1][i], i);

        unclipped_vertices[2].st[0]	    = s_axis;
        unclipped_vertices[2].st[1]	    = t_axis;
        unclipped_vertices[2].xyz[0]	= v_axis[0];
        unclipped_vertices[2].xyz[1]	= v_axis[1];
        unclipped_vertices[2].xyz[2]	= v_axis[2];

		MakeSkyVec (skymaxs[0][i], skymins[1][i], i);

        unclipped_vertices[3].st[0]	    = s_axis;
        unclipped_vertices[3].st[1]	    = t_axis;
        unclipped_vertices[3].xyz[0]	= v_axis[0];
        unclipped_vertices[3].xyz[1]	= v_axis[1];
        unclipped_vertices[3].xyz[2]	= v_axis[2];

        if (clipping::is_clipping_required(
            unclipped_vertices,
            unclipped_vertex_count))
        {
            // Clip the polygon.
            const glvert_t*	clipped_vertices;
            std::size_t		clipped_vertex_count;
            clipping::clip(
                unclipped_vertices,
                unclipped_vertex_count,
                &clipped_vertices,
                &clipped_vertex_count);

            // Did we have any vertices left?
            if (clipped_vertex_count)
            {
                // Copy the vertices to the display list.
                const std::size_t buffer_size = clipped_vertex_count * sizeof(glvert_t);
                glvert_t* const display_list_vertices = static_cast<glvert_t*>(sceGuGetMemory(buffer_size));
                memcpy(display_list_vertices, clipped_vertices, buffer_size);

 //               sceKernelDcacheWritebackRange(display_list_vertices,buffer_size);

                if(cl.worldmodel)
                    VID_SetPaletteToSky ();

                Fog_DisableGFog();

                if (r_skyfog.value)
                {
                    a = r_refdef.fog_end * 0.00025f;
                    r = r_refdef.fog_red * 0.01f + (a * 0.25f);
                    g = r_refdef.fog_green * 0.01f + (a * 0.25f);
                    b = r_refdef.fog_blue * 0.01f + (a * 0.25f);

                    if (a > 1.0f)
                        a = 1.0f;
                    if (r > 1.0f)
                        r = 1.0f;
                    if (g > 1.0f)
                        g = 1.0f;
                    if (b > 1.0f)
                        b = 1.0f;

                    sceGuEnable(GU_BLEND);
                    sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, GU_COLOR(r,g,b,a), GU_COLOR(r,g,b,a));
                }

                sceGuDepthRange(32767, 65535);
//                sceGuTexWrap(GU_CLAMP, GU_CLAMP);

                // Draw the clipped vertices.
                sceGuDrawArray(
                    GU_TRIANGLE_FAN,
                    GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
                    clipped_vertex_count, 0, display_list_vertices);

//                sceGuTexWrap(GU_REPEAT, GU_REPEAT);
                sceGuDepthRange(0, 65535);

                if (r_skyfog.value)
                {
                    sceGuDisable(GU_BLEND);
                    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
                }

                VID_SetGlobalPalette ();
                Fog_EnableGFog();

            }
        }
        else
        {

            if(cl.worldmodel)
                VID_SetPaletteToSky ();

            Fog_DisableGFog();

            if (r_skyfog.value)
            {
                a = r_refdef.fog_end * 0.00025f;
                r = r_refdef.fog_red * 0.01f + (a * 0.25f);
                g = r_refdef.fog_green * 0.01f + (a * 0.25f);
                b = r_refdef.fog_blue * 0.01f + (a * 0.25f);

                if (a > 1)
                    a = 1;
                if (r > 1)
                    r = 1;
                if (g > 1)
                    g = 1;
                if (b > 1)
                    b = 1;

                sceGuEnable(GU_BLEND);
                sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, GU_COLOR(r,g,b,a), GU_COLOR(r,g,b,a));
            }

            sceGuDepthRange(32767, 65535);
 //           sceGuTexWrap(GU_CLAMP, GU_CLAMP);

            // Draw the poly directly.
            sceGuDrawArray(
                GU_TRIANGLE_FAN,
                GU_TEXTURE_32BITF | GU_VERTEX_32BITF,
                unclipped_vertex_count, 0, unclipped_vertices);

//            sceGuTexWrap(GU_REPEAT, GU_REPEAT);
            sceGuDepthRange(0, 65535);

            if (r_skyfog.value)
            {
                sceGuDisable(GU_BLEND);
                sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
            }

            VID_SetGlobalPalette ();
			Fog_EnableGFog();
        }
	}
}

//===============================================================

/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain (msurface_t *s)
{
	msurface_t	*fa;
    int		i;
    vec3_t	verts[MAX_CLIP_VERTS];
    glpoly_t	*p;
    if (skybox_name[0]) // if the skybox has a name, draw the skybox
	{
        c_sky = 0;
 //       GL_Bind(solidskytexture);

        // calculate vertex values for sky box

        for (fa=s ; fa ; fa=fa->texturechain)
        {
            for (p=fa->polys ; p ; p=p->next)
            {
                for (i=0 ; i<p->numverts ; i++)
                {
                    VectorSubtract (p->verts[i].xyz, r_origin, verts[i]);
                }
                ClipSkyPolygon (p->numverts, verts[0], 0);
            }

        }
	}
    else // otherwise, draw the normal quake sky
	{
        // used when gl_texsort is on
        GL_Bind(solidskytexture);

        if (kurok)
            speedscale = realtime*2;
        else
            speedscale = realtime*8;

        speedscale -= (int)speedscale & ~127 ;

        for (fa=s ; fa ; fa=fa->texturechain)
            EmitSkyPolys (fa);

        sceGuEnable(GU_BLEND);
//        sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

        GL_Bind (alphaskytexture);

        if (kurok)
            speedscale = realtime*4;
        else
            speedscale = realtime*16;

        speedscale -= (int)speedscale & ~127 ;

        for (fa=s ; fa ; fa=fa->texturechain)
            EmitSkyPolys (fa);

        sceGuDisable(GU_BLEND);
//        sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	}
}



//===============================================================

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
//void R_InitSky (texture_t *mt)
void R_InitSky (byte *mt)
{
	byte	trans[128*128];

	const byte* const src = (byte *)mt; //  + mt->offsets[0];

	for (int i=0 ; i<128 ; i++)
	{
		for (int j=0 ; j<128 ; j++)
		{
			const byte p = src[i*256 + j + 128];
			trans[(i*128) + j] = p;
		}
	}

	if (solidskytexture == -1)
		solidskytexture = GL_LoadTexture("", 128, 128, trans, qfalse, GU_LINEAR, 0);

	for (int i=0 ; i<128 ; i++)
	{
		for (int j=0 ; j<128 ; j++)
		{
			const byte p = src[i*256 + j];
			if (p == 0)
				trans[(i*128) + j] = 255;
			else
				trans[(i*128) + j] = p;
		}
	}

	if (alphaskytexture == -1)
		alphaskytexture = GL_LoadTexture("", 128, 128, trans, qfalse, GU_LINEAR, 0);
}
