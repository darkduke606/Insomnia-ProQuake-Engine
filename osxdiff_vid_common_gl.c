/*
Copyright (C) 2002-2003 A Nourai

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
// vid_common_gl.c -- Common code for vid_wgl.c and vid_glx.c

#include "quakedef.h"
//const char *gl_vendor;
//const char *gl_renderer;
//const char *gl_version;
const char *gl_extensions;

//qboolean gl_mtexable = false;

int			texture_mode = GL_LINEAR;

float		gldepthmin, gldepthmax;

byte		color_white[4] = {255, 255, 255, 0};

void GL_PrintExtensions_f(void) {
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);
}

/*
void BuildGammaTable (float g)
{
	int		i, inf;

	if (g == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
		return;
	}

	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * powf ( (i+0.5f)/255.5f , g ) + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		gammatable[i] = inf;
	}
}
*/