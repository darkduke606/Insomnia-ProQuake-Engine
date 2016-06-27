/*
Copyright (C) 2004 MH

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
// gl_fullbright.c

#include "quakedef.h"
extern qboolean mtexenabled;
int FindFullbrightTexture (byte *pixels, int num_pix)
{
    int i;
    for (i = 0; i < num_pix; i++)
        if (pixels[i] > 223)
            return 1;

    return 0;
}

void ConvertPixels (byte *pixels, int num_pixels)
{
    int i;

    for (i = 0; i < num_pixels; i++)
        if (pixels[i] < 224)
            pixels[i] = 255;
}

void DrawFullBrightTextures (msurface_t *first_surf, int num_surfs)
{
    // gl_texsort 1 version
    int i;
    msurface_t *fa;
    texture_t *t;

    if (r_fullbright.value)
        return;

    if (mtexenabled)
        return;

    GL_DisableMultitexture ();

    for (fa = first_surf, i = 0; i < num_surfs; fa++, i++)
    {
        // find the correct texture
        t = R_TextureAnimation (fa->texinfo->texture);

        if (t->fullbright != -1 && fa->draw_this_frame)
        {
            glEnable (GL_BLEND);
            GL_Bind (t->fullbright);
            DrawGLPoly (fa->polys);
            glDisable (GL_BLEND);
        }

        fa->draw_this_frame = 0;
    }
}