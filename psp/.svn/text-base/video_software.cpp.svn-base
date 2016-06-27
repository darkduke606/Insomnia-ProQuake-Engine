/*
Quake PSP port.
Copyright (C) 2005-2006 Peter Mackay

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <cstddef>

#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <malloc.h>

extern "C"
{
#include "../quakedef.h"
#include "../d_local.h"
}


#define DEF_SCREEN_WIDTH  480
#define DEF_SCREEN_HEIGHT 272 
#define MIN_SCREEN_WIDTH  320
#define MIN_SCREEN_HEIGHT 200 
#define MAX_SCREEN_WIDTH  480
#define MAX_SCREEN_HEIGHT 272 

#define DEF_RENDER_WIDTH  320
#define DEF_RENDER_HEIGHT 200 
#define MIN_RENDER_WIDTH  320
#define MIN_RENDER_HEIGHT 200 
#define MAX_RENDER_WIDTH  480
#define MAX_RENDER_HEIGHT 272 



namespace quake
{
	namespace video
	{
		// Types.
		typedef ScePspRGBA8888	Pixel;
		typedef u8				Texel;

		// Constants.
		static size_t			screenWidth	= DEF_SCREEN_WIDTH;
		static size_t			screenHeight	= DEF_SCREEN_HEIGHT;
		static size_t			renderWidth	= DEF_RENDER_WIDTH;
		static size_t			renderHeight	= DEF_RENDER_HEIGHT;
		static const size_t		textureWidth	= 512;
		static const size_t		textureHeight	= 512;
		static const size_t		paletteSize	= 256;
		static void* const			vramBase			= sceGeEdramGetAddr();
		static Pixel* 			displayBuffer	= static_cast<Pixel*>(vramBase);
		static Pixel* 			backBuffer	= static_cast<Pixel*>(displayBuffer + (512 * screenHeight));
		static Texel* 			texture0	= reinterpret_cast<Texel*>(displayBuffer + 2 * (512 * screenHeight));
		static Texel* 			texture1	= texture0 + (textureWidth * textureHeight);

		// Regular globals.
		static Pixel __attribute__((aligned(16)))	texturePalette[paletteSize];
		static short					*zbuffer;
		static unsigned char				*surfaceCache;

		//! The GU display list.
		//! @note	Aligned to 64 bytes so it doesn't share a cache line with anything.
		unsigned int *displayList;
	}
}

using namespace quake;
using namespace quake::video;

// Globals required by Quake.
unsigned short	d_8to16table[paletteSize];

void VID_SetPalette(unsigned char* palette)
{
	// Convert the palette to PSP format.
	for(Pixel* color = &texturePalette[0]; color < &texturePalette[paletteSize]; ++color)
	{
		const unsigned int r = *palette++;
		const unsigned int g = *palette++;
		const unsigned int b = *palette++;
		*color = r | (g << 8) | (b << 16);
	}

	// Upload the palette.
	sceGuClutMode(GU_PSM_8888, 0, paletteSize - 1, 0);
	sceKernelDcacheWritebackRange(texturePalette, sizeof(texturePalette));
	sceGuClutLoad(paletteSize / 8, texturePalette);
}

void VID_ShiftPalette(unsigned char* palette)
{
	VID_SetPalette(palette);
}

void VID_Init(unsigned char* palette)
{
	if (COM_CheckParm("-rwidth")) {
		renderWidth = atoi(com_argv[COM_CheckParm("-rwidth")+1]);
		
		if (renderWidth < MIN_RENDER_WIDTH)
			renderWidth = MIN_RENDER_WIDTH;
		if (renderWidth > MAX_RENDER_WIDTH)
			renderWidth = MAX_RENDER_WIDTH;
		if ((renderWidth % 8) != 0)
			renderWidth = DEF_RENDER_WIDTH;
	} 
	else
	{
		renderWidth = DEF_RENDER_WIDTH;
	}

	if (COM_CheckParm("-rheight")) {
		renderHeight = atoi(com_argv[COM_CheckParm("-rheight")+1]);
		
		if (renderHeight < MIN_RENDER_HEIGHT)
			renderHeight = MIN_RENDER_HEIGHT;
		if (renderHeight > MAX_RENDER_HEIGHT)
			renderHeight = MAX_RENDER_HEIGHT;
		if ((renderHeight % 8) != 0)
			renderHeight = DEF_RENDER_HEIGHT;
	} 
	else
	{
		renderHeight = DEF_RENDER_HEIGHT;
	}

	if (COM_CheckParm("-swidth")) {
		screenWidth = atoi(com_argv[COM_CheckParm("-swidth")+1]);
		
		if (screenWidth < MIN_SCREEN_WIDTH)
			screenWidth = MIN_SCREEN_WIDTH;
		if (screenWidth > MAX_SCREEN_WIDTH)
			screenWidth = MAX_SCREEN_WIDTH;
		if ((screenWidth % 8) != 0)
			screenWidth = DEF_SCREEN_WIDTH;
	} 
	else
	{
		screenWidth = DEF_SCREEN_WIDTH;
	}

	if (COM_CheckParm("-sheight")) {
		screenHeight = atoi(com_argv[COM_CheckParm("-sheight")+1]);
		
		if (screenHeight < MIN_SCREEN_HEIGHT)
			screenHeight = MIN_SCREEN_HEIGHT;
		if (screenHeight > MAX_SCREEN_HEIGHT)
			screenHeight = MAX_SCREEN_HEIGHT;
		if ((screenHeight % 8) != 0)
			screenHeight = DEF_SCREEN_HEIGHT;
	} 
	else
	{
		screenHeight = DEF_SCREEN_HEIGHT;
	}
	
	int surfaceCacheSize = SURFCACHE_SIZE_AT_320X200 + ((renderWidth * renderHeight) - (320 * 200)) * 3;
	
	zbuffer = (short *)malloc(sizeof(short) * renderWidth * renderHeight);
	surfaceCache = (unsigned char *)malloc(surfaceCacheSize);
	displayList = (unsigned int *)memalign(64, sizeof(int) * 128 * 1024);

	if(!zbuffer)
		Sys_Error("No memory for zbuffer\n");

	if(!surfaceCache)
		Sys_Error("No memory for surfaceCache\n");

	if(!displayList)
		Sys_Error("No memory for displayList\n");

	// Initialise the GU.
	sceGuInit();

	// Set up the GU.
	sceGuStart(GU_DIRECT, displayList);
	{
		// Set the draw and display buffers.
		void* const displayBufferInVRAM = reinterpret_cast<char*>(displayBuffer) - reinterpret_cast<size_t>(vramBase);
		void* const backBufferInVRAM = reinterpret_cast<char*>(backBuffer) - reinterpret_cast<size_t>(vramBase);

		sceGuDrawBuffer(GU_PSM_8888, backBufferInVRAM, 512);
		sceGuDispBuffer(MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT, displayBufferInVRAM, 512);

		// Set the rendering offset and viewport.
		sceGuOffset(2048 - (MAX_SCREEN_WIDTH / 2), 2048 - (MAX_SCREEN_HEIGHT / 2));
		sceGuViewport(2048, 2048, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);

		// Set up scissoring.
		sceGuEnable(GU_SCISSOR_TEST);
		sceGuScissor(0, 0, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);

		// Set up texturing.
		sceGuEnable(GU_TEXTURE_2D);
		sceGuTexMode(GU_PSM_T8, 0, 0, GU_FALSE);
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	}
	sceGuFinish();
	sceGuSync(0,0);

	// Turn on the display.
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	// What is the aspect ratio?
	const float	screenAspectRatio	= static_cast<float>(screenWidth) / screenHeight;
	const float	renderAspectRatio	= static_cast<float>(renderWidth) / renderHeight;

	// Set up Quake's video parameters.
	vid.aspect			= screenAspectRatio / renderAspectRatio;
	vid.buffer			= texture0;
	vid.colormap		= host_colormap;
	vid.colormap16		= 0;
	vid.conbuffer		= texture0;
	vid.conheight		= renderHeight;
	vid.conrowbytes		= textureWidth;
	vid.conwidth		= renderWidth;
	vid.direct		= texture0;
	vid.fullbright		= paletteSize - LittleLong(*((int *) vid.colormap + 2048));
	vid.height			= renderHeight;
	vid.maxwarpheight	= WARP_HEIGHT;
	vid.maxwarpwidth	= WARP_WIDTH;
	vid.numpages		= 2;
	vid.recalc_refdef	= 0;
	vid.rowbytes		= textureWidth;
	vid.width			= renderWidth;
	
	// Set the z buffer address.
	d_pzbuffer		= zbuffer;

	// Initialise the surface cache.
	D_InitCaches(surfaceCache, surfaceCacheSize);

	Con_Printf ("Rendered  at ( %d x %d ) \n",vid.width, vid.height);
	Con_Printf ("Displayed at ( %d x %d ) \n",screenWidth, screenHeight);
	
	// This is an awful hack to get around a division by zero later on...
	r_refdef.vrect.width	= renderWidth;
	r_refdef.vrect.height	= renderHeight;

	// Start a render.
	sceGuStart(GU_DIRECT, displayList);

	// Set the palette.
	VID_SetPalette(palette);
}

void VID_Shutdown(void)
{
	// Finish rendering.
	sceGuFinish();
	sceGuSync(0,0);

	// Shut down the display.
	sceGuTerm();

	if(zbuffer)
		free(zbuffer);

	if(surfaceCache)
		free(surfaceCache);

	if(displayList)
		free(displayList);
}

void VID_Update(vrect_t* rects)
{
	// Finish rendering.
	sceGuFinish();
	sceGuSync(0, 0);

	// Switch the buffers.
	sceGuSwapBuffers();

	// Start a new render.
	sceGuStart(GU_DIRECT, displayList);

	// Allocate memory in the display list for some vertices.
	struct Vertex
	{
		SceUShort16	u, v;
		SceShort16	x, y, z;
	} * const vertices = static_cast<Vertex*>(sceGuGetMemory(sizeof(Vertex) * 2));

	int dif_x = (MAX_SCREEN_WIDTH/2)  - (screenWidth/2);
	int dif_y = (MAX_SCREEN_HEIGHT/2) - (screenHeight/2);
	
		
	// Set up the vertices.
	vertices[0].u	= 0;
	vertices[0].v	= 0;
	vertices[0].x	= dif_x;
	vertices[0].y	= dif_y;
	vertices[0].z	= 0;
	vertices[1].u	= renderWidth;
	vertices[1].v	= renderHeight;
	vertices[1].x	= screenWidth+dif_x;
	vertices[1].y	= screenHeight+dif_y;
	vertices[1].z	= 0;

	// Flush the data cache.
	sceKernelDcacheWritebackRange(vid.buffer, textureWidth * textureHeight);

	// Set the texture.
	sceGuTexImage(0, textureWidth, textureHeight, textureWidth, vid.buffer);

	// Draw the sprite.
	sceGuDrawArray(
		GU_SPRITES,
		GU_VERTEX_16BIT | GU_TEXTURE_16BIT | GU_TRANSFORM_2D,
		2,
		0,
		vertices);

	// Swap Quake's buffers.
	Texel* const newTexture = (vid.buffer == texture0) ? texture1 : texture0;
	vid.buffer		= newTexture;
	vid.conbuffer	= newTexture;
	vid.direct	= newTexture;
}

void D_BeginDirectRect(int x, int y, byte* pbitmap, int width, int height)
{
}

void D_EndDirectRect(int x, int y, int width, int height)
{
}
