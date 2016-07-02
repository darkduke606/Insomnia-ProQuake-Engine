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

#include <algorithm>
#include <cstddef>

#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <limits.h>
#include "m33libs/include/kubridge.h"

extern "C"
{
#include "../quakedef.h"
}

#include "vram.hpp"

#ifdef _WIN32
# define ALIGNED(x)
#else
# define ALIGNED(x) __attribute__((aligned(x)))
#endif

namespace quake
{
	namespace video
	{
		// Types.
#ifdef NORMAL_MODEL
		typedef ScePspRGB565	pixel;
#else
		typedef ScePspRGBA8888	pixel;
#endif
		typedef u8				texel;
		typedef u16				depth_value;

		// Constants.
		static const std::size_t	screen_width	= 480;
		static const std::size_t	screen_height	= 272;
		static const std::size_t	palette_size	= 256;
		static pixel* 				display_buffer	= 0;
		static pixel*				draw_buffer		= 0;
		static depth_value*			depth_buffer	= 0;

		//! The GU display list.
		//! @note	Aligned to 64 bytes so it doesn't share a cache line with anything.
		unsigned int ALIGNED(64)	display_list[262144];
	}
}

using namespace quake;
using namespace quake::video;

// Regular globals.
ScePspRGBA8888 ALIGNED(16)	d_8to24table[palette_size];
ScePspRGBA8888 ALIGNED(16)	d_8to24tableLM[palette_size];
ScePspRGBA8888 ALIGNED(16)	d_8to24tableSKY[palette_size];

void VID_InitSkyPalette(unsigned char* palette) {
	// Convert the palette to PSP format.
	for (ScePspRGBA8888* color = &d_8to24tableSKY[0]; color < &d_8to24tableSKY[palette_size]; ++color)
	{
		const unsigned int r = *palette++;
		const unsigned int g = *palette++;
		const unsigned int b = *palette++;
		*color = GU_RGBA(r, g, b, 0xff);
	}

	// Upload the palette.
	sceGuClutMode(GU_PSM_8888, 0, palette_size - 1, 0);
	sceKernelDcacheWritebackRange(d_8to24tableSKY, sizeof(d_8to24tableSKY));
	sceGuClutLoad(palette_size / 8, d_8to24tableSKY);
}

void VID_InitLightmapPalette() {
	// Convert the palette to PSP format.
	ScePspRGBA8888* color2 = &d_8to24tableLM[0];
	for (unsigned int i=0; color2 < &d_8to24tableLM[palette_size]; ++color2, i++)
	{
		*color2 = GU_RGBA(i, i, i, i);
	}
}

void VID_SetPaletteToSky() {
	// Upload the palette.
	sceGuClutMode(GU_PSM_8888, 0, palette_size - 1, 0);
	sceKernelDcacheWritebackRange(d_8to24tableSKY, sizeof(d_8to24tableSKY));
	sceGuClutLoad(palette_size / 8, d_8to24tableSKY);
}

void VID_SetLightmapPalette() {
	// Upload the palette.
	sceGuClutMode(GU_PSM_8888, 0, palette_size - 1, 0);
	sceKernelDcacheWritebackRange(d_8to24tableLM, sizeof(d_8to24tableLM));
	sceGuClutLoad(palette_size / 8, d_8to24tableLM);
}

void VID_SetGlobalPalette() {
	// Upload the palette.
	sceGuClutMode(GU_PSM_8888, 0, palette_size - 1, 0);
	sceKernelDcacheWritebackRange(d_8to24table, sizeof(d_8to24table));
	sceGuClutLoad(palette_size / 8, d_8to24table);
}

void VID_SetPaletteOld(unsigned char* palette)
{
	// Convert the palette to PSP format.
	for (ScePspRGBA8888* color = &d_8to24table[0]; color < &d_8to24table[palette_size]; ++color)
	{
		const unsigned int r = *palette++;
		const unsigned int g = *palette++;
		const unsigned int b = *palette++;
		*color = GU_RGBA(r, g, b, 0xff);
	}

	// Color 255 is transparent black.
	// This is a bit of a dirty hack.
	d_8to24table[255] = 0;

	// Upload the palette.
	sceGuClutMode(GU_PSM_8888, 0, palette_size - 1, 0);
	sceKernelDcacheWritebackRange(d_8to24table, sizeof(d_8to24table));
	sceGuClutLoad(palette_size / 8, d_8to24table);
}

void VID_SetPaletteToTexture(ScePspRGBA8888* palette)
{
	sceGuClutMode(GU_PSM_8888, 0, 0xff, 0);
	sceKernelDcacheWritebackRange(palette, sizeof(palette));
	sceGuClutLoad(palette_size / 8, palette);
}

void VID_ShiftPalette(unsigned char* palette)
{
	VID_SetPaletteOld(palette);
}

void VID_Init(unsigned char* palette)
{
	Sys_Printf("VID_Init\n");

	// Allocate the buffers.
	display_buffer	= static_cast<pixel*>(vram::allocate(screen_height * 512 * sizeof(pixel)));
	if (!display_buffer)
	{
		Sys_Error("Couldn't allocate display buffer");
	}
	draw_buffer	= static_cast<pixel*>(vram::allocate(screen_height * 512 * sizeof(pixel)));
	if (!draw_buffer)
	{
		Sys_Error("Couldn't allocate draw buffer");
	}
	depth_buffer	= static_cast<depth_value*>(vram::allocate(screen_height * 512 * sizeof(depth_value)));
	if (!depth_buffer)
	{
		Sys_Error("Couldn't allocate depth buffer");
	}

	// Initialise the GU.
	sceGuInit();

	// Set up the GU.
	sceGuStart(GU_DIRECT, display_list);
	{
		// Set the draw and display buffers.
		void* const	vram_base				= sceGeEdramGetAddr();
		void* const	display_buffer_in_vram	= reinterpret_cast<char*>(display_buffer) - reinterpret_cast<std::size_t>(vram_base);
		void* const draw_buffer_in_vram		= reinterpret_cast<char*>(draw_buffer) - reinterpret_cast<std::size_t>(vram_base);
		void* const depth_buffer_in_vram	= reinterpret_cast<char*>(depth_buffer) - reinterpret_cast<std::size_t>(vram_base);

#ifdef NORMAL_MODEL
        sceGuDrawBuffer(GU_PSM_5650, draw_buffer_in_vram, 512);
#else
        sceGuDrawBuffer(GU_PSM_8888, draw_buffer_in_vram, 512);
#endif

		sceGuDispBuffer(screen_width, screen_height, display_buffer_in_vram, 512);
		sceGuDepthBuffer(depth_buffer_in_vram, 512);

		// Set the rendering offset and viewport.
		sceGuOffset(2048 - (screen_width / 2), 2048 - (screen_height / 2));
		sceGuViewport(2048, 2048, screen_width, screen_height);

		// Set up scissoring.
		sceGuEnable(GU_SCISSOR_TEST);
		sceGuScissor(0, 0, screen_width, screen_height);

		// Set up texturing.
		sceGuEnable(GU_TEXTURE_2D);

		// Set up clearing.
		sceGuClearDepth(65535);
        sceGuClearColor(GU_COLOR(0.25f,0.25f,0.25f,0.5f));

		// Set up depth.
		sceGuDepthRange(0, 65535);
		sceGuDepthFunc(GU_LEQUAL);
		sceGuEnable(GU_DEPTH_TEST);

		// Set the matrices.
		sceGumMatrixMode(GU_PROJECTION);
		sceGumLoadIdentity();
		sceGumUpdateMatrix();
		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();
		sceGumUpdateMatrix();
		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		sceGumUpdateMatrix();

		// Set up culling.
		sceGuFrontFace(GU_CW);
		sceGuEnable(GU_CULL_FACE);
		sceGuEnable(GU_CLIP_PLANES);

		// Set up blending.
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	}

	sceGuFinish();
	sceGuSync(0,0);

	// Turn on the display.
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	// Set up Quake's video parameters.
	vid.aspect			= (static_cast<float>(screen_height) / static_cast<float>(screen_width)) * (4.0f / 3.0f);
	vid.buffer			= 0;
	vid.colormap		= host_colormap;
	vid.colormap16		= 0;
	vid.conbuffer		= 0;
	vid.conheight		= screen_height;
	vid.conrowbytes		= 0;
	vid.conwidth		= screen_width;
	vid.direct			= 0;
	vid.fullbright		= palette_size - LittleLong(*((int *) vid.colormap + 2048));
	vid.height			= screen_height;
	vid.maxwarpheight	= screen_width;
	vid.maxwarpwidth	= screen_height;
	vid.numpages		= INT_MAX;
	vid.recalc_refdef	= 0;
	vid.rowbytes		= 0;
	vid.width			= screen_width;

	// Start a render.
	sceGuStart(GU_DIRECT, display_list);

	// Set the palette.
	VID_SetPaletteOld(palette);
	VID_InitLightmapPalette();

	Sys_Printf("VID_Init OK\n");
}

void VID_Shutdown(void)
{
	// Finish rendering.
	sceGuFinish();
	sceGuSync(0,0);

	// Shut down the display.
	sceGuTerm();

	// Free the buffers.
	vram::free(depth_buffer);
	depth_buffer = 0;
	vram::free(draw_buffer);
	draw_buffer = 0;
	vram::free(display_buffer);
	display_buffer = 0;
}

int DitherMatrix[2][16] = { {    0,  8,  2, 10,
 	                             12,  4, 14,  6,
	                             3, 11,  1,  9,
                                 15,  7, 13,  5 },
                          {      0,  8,  2, 10,
	                             12, 10, 14, 12,
	                             2,  9,  1, 11,
	                             14, 12, 13, 12 } };

                        /*{ { 0, 8, 0, 8,
                              8, 0, 8, 0,
                              0, 8, 0, 8,
                              8, 0, 8, 0 },
                            { 8, 0, 8, 0,
                              0, 8, 0, 8,
                              8, 0, 8, 0,
                              0, 8, 0, 8 } };*/

void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = 2048;
	*y = 2048;

	if (r_antialias.value)
	{
	    *width = screen_width + (r_framecount&1)*2;   // For fake antialising effect
        *height = screen_height + (r_framecount&1)*2; // For fake antialising effect
    }
	else
	{
	    *width = screen_width;
	    *height = screen_height;
    }

//    sceGuSetDither( (ScePspIMatrix4*)DitherMatrix[(r_framecount&1)] );

    if (r_dithering.value)
		sceGuEnable(GU_DITHER);
	else
		sceGuDisable(GU_DITHER);
}

void GL_EndRendering (void)
{
	// Finish rendering.
	sceGuFinish();
	sceGuSync(0, 0);

	// At the moment only do this if we are in network mode, once we get above
	// 60fps we might as well leave it on for all games
	if (tcpipAvailable || r_vsync.value)
		sceDisplayWaitVblankStart();

	// Switch the buffers.
	sceGuSwapBuffers();
	std::swap(draw_buffer, display_buffer);

	// Start a new render.
	sceGuStart(GU_DIRECT, display_list);
}

void D_StartParticles (void)
{
	if (r_particles_simple.value == qtrue)
	{
		sceGuDisable(GU_TEXTURE_2D);
	}
	else
	{
		GL_BindLightmap (particletexture);

		sceGuEnable(GU_ALPHA_TEST);
		sceGuDepthMask(GU_TRUE);
		sceGuEnable(GU_BLEND);
		//sceGuDisable(GU_FOG);
		sceGuTexFunc(GU_TFX_MODULATE , GU_TCC_RGBA);

		if (kurok)
			sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
		else
			sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	}
}

void D_EndParticles (void)
{
	if (r_particles_simple.value == qtrue)
	{
		sceGuEnable(GU_TEXTURE_2D);
	}
	else
	{
        sceGuDisable(GU_ALPHA_TEST);
		sceGuDepthMask(GU_FALSE);
		sceGuDisable(GU_BLEND);
		//sceGuEnable(GU_FOG);
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	}
}



static int BufIdx  = 0;
static int BufSize = 0;

psp_particle* D_CreateBuffer (int size) {
	psp_particle* const vertices = static_cast<psp_particle*>(sceGuGetMemory(size*sizeof(psp_particle)));

	BufSize = size;
	BufIdx  = 0;

	return vertices;
}


void D_DeleteBuffer (psp_particle* vertices) {
    if (BufIdx > 0) {
    	sceGuDrawArray(GU_SPRITES, GU_VERTEX_32BITF|GU_TEXTURE_32BITF|GU_COLOR_8888, BufIdx, 0, vertices);
    	BufIdx = 0;
    	BufSize = -1;
    }

}

int D_DrawParticleBuffered (psp_particle* vertices, particle_t *pparticle, vec3_t up, vec3_t right, float scale) {
	unsigned int color = d_8to24table[static_cast<int>(pparticle->color)];
	int i = BufIdx;

	vertices[i].first.x = pparticle->org[0];
	vertices[i].first.y = pparticle->org[1];
	vertices[i].first.z = pparticle->org[2];
	vertices[i].first.s = 0.0;
	vertices[i].first.t = 0.0;
	vertices[i].first.color = color;

	vertices[i].second.x = pparticle->org[0] + scale*(up[0] + right[0]);
	vertices[i].second.y = pparticle->org[1] + scale*(up[1] + right[1]);
	vertices[i].second.z = pparticle->org[2] + scale*(up[2] + right[2]);
	vertices[i].second.s = 1.0;
	vertices[i].second.t = 1.0;
	vertices[i].second.color = color;

    BufIdx++;

    if (BufIdx >= BufSize) {
    	sceGuDrawArray(GU_SPRITES, GU_VERTEX_32BITF|GU_TEXTURE_32BITF|GU_COLOR_8888, BufSize, 0, vertices);
    	BufIdx = 0;
    	BufSize = -1;
    	return -1;
    }

    return BufIdx;
}


void D_DrawParticle (particle_t *pparticle, vec3_t up, vec3_t right, float scale)
{
	unsigned int color = d_8to24table[static_cast<int>(pparticle->color)];

	struct part_vertex {
		float s, t;
		unsigned int color;
		float x, y, z;
	};

	part_vertex* const vertices = static_cast<part_vertex*>(sceGuGetMemory(2*sizeof(part_vertex)));

	vertices[0].x = pparticle->org[0];
	vertices[0].y = pparticle->org[1];
	vertices[0].z = pparticle->org[2];
	vertices[0].s = 0.0;
	vertices[0].t = 0.0;
	vertices[0].color = color;

	vertices[1].x = pparticle->org[0] + scale*(up[0] + right[0]);
	vertices[1].y = pparticle->org[1] + scale*(up[1] + right[1]);
	vertices[1].z = pparticle->org[2] + scale*(up[2] + right[2]);
	vertices[1].s = 1.0;
	vertices[1].t = 1.0;
	vertices[1].color = color;

	sceGuDrawArray(GU_SPRITES, GU_VERTEX_32BITF|GU_TEXTURE_32BITF|GU_COLOR_8888, 2, 0, vertices);

}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (void)
{
	byte		*buffer;
	char		pcxname[80];
	char		checkname[MAX_OSPATH];
//
// find a file name to save it to
//
    if (kurok)
	    strcpy(pcxname,"sshot00.tga");
    else
        strcpy(pcxname,"quake00.tga");

	int i;
	for (i=0 ; i<=99 ; i++)
	{
		pcxname[5] = i/10 + '0';
		pcxname[6] = i%10 + '0';
		snprintf (checkname, sizeof(checkname), "%s/%s", com_gamedir, pcxname);
		if (Sys_FileTime(checkname) == -1)
			break;	// file doesn't exist
	}
	if (i==100)
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't create a TGA file\n");
		return;
 	}


	buffer = static_cast<byte*>(Q_malloc(glwidth*glheight*3 + 18));
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = glwidth&255;
	buffer[13] = glwidth>>8;
	buffer[14] = glheight&255;
	buffer[15] = glheight>>8;
	buffer[16] = 24;	// pixel size

	// Get the pixels and swap the colours from ARGB to BGR.
	i = 18;
	for (int y = 0; y < glheight; ++y)
	{
		const pixel* src = display_buffer + ((glheight - y - 1) * 512);
		for (int x = 0; x < glwidth; ++x)
		{
			const pixel argb = *src++;

#ifdef NORMAL_MODEL
// For RGB 565 pixel format
			buffer[i++]	= ((argb >> 11) & 0x1f) << 3;
			buffer[i++]	= ((argb >> 5) & 0x3f) << 2;
			buffer[i++]	= (argb & 0x1f) << 3;
#else
// For RGB 888 pixel format
			buffer[i++]	= (argb >> 16) & 0xff;
			buffer[i++]	= (argb >> 8) & 0xff;
			buffer[i++]	= argb & 0xff;
#endif
		}
	}
	COM_WriteFile (pcxname, buffer, glwidth*glheight*3 + 18 );

	free (buffer);
	Con_Printf ("Wrote %s\n", pcxname);
}
