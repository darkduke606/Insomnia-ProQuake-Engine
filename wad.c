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
// wad.c

#include "quakedef.h"

int			wad_numlumps;
lumpinfo_t	*wad_lumps;
byte		*wad_base;

void SwapPic (qpic_t *pic);

/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName (char *in, char *out)
{
	int	i, c;

	for (i=0 ; i<16 ; i++ )
	{
		c = in[i];
		if (!c)
			break;

		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		out[i] = c;
	}

	for ( ; i< 16 ; i++ )
		out[i] = 0;
}

/*
====================
W_LoadWadFile
====================
*/
void W_LoadWadFile (char *filename)
{
	lumpinfo_t		*lump_p;
	wadinfo_t		*header;
	unsigned		i;
	int				infotableofs;

	if (!(wad_base = COM_LoadHunkFile (filename)))
		Sys_Error ("W_LoadWadFile: couldn't load %s\n\nGame data files are required to run; usually this means you need Quake shareware or registered version.", filename);

	header = (wadinfo_t *)wad_base;

	if (memcmp(header->identification, "WAD2", 4))
		Sys_Error ("Wad file %s doesn't have WAD2 id\n",filename);

	wad_numlumps = LittleLong(header->numlumps);
	infotableofs = LittleLong(header->infotableofs);
	wad_lumps = (lumpinfo_t *)(wad_base + infotableofs);

	for (i=0, lump_p = wad_lumps ; i<wad_numlumps ; i++,lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName (lump_p->name, lump_p->name);
		if (lump_p->type == TYP_QPIC)
			SwapPic ( (qpic_t *)(wad_base + lump_p->filepos));
	}
}


/*
=============
W_GetLumpinfo
=============
*/
lumpinfo_t	*W_GetLumpinfo (char *name)
{
	int		i;
	lumpinfo_t	*lump_p;
	char	clean[16];

	W_CleanupName (name, clean);

	for (lump_p=wad_lumps, i=0 ; i<wad_numlumps ; i++,lump_p++)
	{
		if (!strcmp(clean, lump_p->name))
			return lump_p;
	}

	Sys_Error ("W_GetLumpinfo: %s not found", name);
	return NULL;
}

void *W_GetLumpName (char *name)
{
	lumpinfo_t	*lump;

	lump = W_GetLumpinfo (name);

	return (void *)(wad_base + lump->filepos);
}

void *W_GetLumpNum (int num)
{
	lumpinfo_t	*lump;

	if (num < 0 || num > wad_numlumps)
		Sys_Error ("W_GetLumpNum: bad number: %i", num);

	lump = wad_lumps + num;

	return (void *)(wad_base + lump->filepos);
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic (qpic_t *pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);
}



/*
=============================================================================
WAD3 Texture Loading for BSP 3.0 Support
=============================================================================
*/


unsigned char*	hlbsp_texpalette;

//converts paletted to same
static byte *ConvertWad3ToPalette (miptex_t *tex) 
{
	byte *in, *data;
	unsigned char  *pal;
	int i, p, image_size;

	if (!tex->offsets[0])
		Sys_Error("ConvertWad3ToRGBA: tex->offsets[0] == 0");

	image_size = tex->width * tex->height;
	in = (byte *) ((byte *) tex + tex->offsets[0]);
   data = Q_malloc(image_size); // Baker

	pal = in + ((image_size * 85) >> 6) + 2;
   hlbsp_texpalette = (unsigned char*)Q_malloc(768);
   memcpy((void*)hlbsp_texpalette, (void*)pal, 768);
	for (i = 0; i < image_size; i++) 
	{
		p = *in++;
		if (tex->name[0] == '{' && p == 255)
		{
			((int *) data)[i] = 0;
		} 
		else 
		{
		 data[i] = p;
		 p++;
		}
	}
	return data;
}


byte *WAD3_LoadTexture(miptex_t *mt) 
{

	if (mt->offsets[0])
		return ConvertWad3ToPalette(mt);

	Sys_Error ("External WAD3s are NOT supported");
}
