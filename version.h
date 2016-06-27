/*
Copyright (C) 2002, Anton Gavrilov

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
// version.h

#ifndef VERSION_H
#define VERSION_H

// Messages: 
// MSVC: #pragma message ( "text" )
// GCC version: #warning "hello"
// #error to terminate compilation

// Create our own define for Mac OS X
#if defined(__APPLE__) && defined(__MACH__)
# define MACOSX
#endif

// Define Operating System Names

#ifdef _WIN32
# define OS_NAME "Windows"
//#error no touch
#elif defined(MACOSX)
# define OS_NAME "Mac OSX"
#elif defined(FLASH)
# define OS_NAME "Flash"
#elif defined(PSP)
# define OS_NAME "Sony PSP"
#else // Everything else gets to be Linux for now
# define OS_NAME "Linux"
# define LINUX // Everything else gets to be Linux for now ;)
#endif


// Define platforms where we do not use assembly

#if defined(MACOSX) || defined(FLASH)
# define NO_ASSEMBLY
#endif

#ifdef GLQUAKE
# define NO_ASSEMBLY
#endif

#if defined(_WIN32) && !defined(WINDED) && defined(_M_IX86)
#define __i386__	1
#endif

#if defined(__i386__)  && !defined(NO_ASSEMBLY)
#define id386	1
#else
#define id386	0
#endif

// Smooth rotation test
//#define SMOOTH_SINGLEPLAYER_TEST
#define QCEXEC
#define SUPPORTS_MULTIMAP_DEMOS
#define CHASE_CAM_FIX
#define SUPPORTS_TRANSFORM_INTERPOLATION // We are switching to r_interpolate_transform
#define SUPPORTS_AUTOID
#define FITZQUAKE_PROTOCOL


// Define Support For Cheat-Free Mode
#if defined(_WIN32) || defined(Linux)
# define SUPPORTS_CHEATFREE_MODE // Only Windows and Linux have security modules.
#endif


// Define Renderer Name

#ifdef D3DQUAKE
# define RENDERER_NAME "D3D"
#elif defined(DX8QUAKE) // !d3dquake
# define RENDERER_NAME "DX8"
#elif defined (FLASH)
# define RENDERER_NAME "Software"
#elif defined(GLQUAKE) // !d3dquake
# define RENDERER_NAME "GL"
#elif defined(PSP_HARDWARE_VIDEO)
# define RENDERER_NAME "PSPGU"
#else
# define RENDERER_NAME "Win"
#endif // end !d3dquake

// Define exceptions to the "rule"
#ifdef DX8QUAKE
# define DX8QUAKE_NO_8BIT					// D3D8 wrapper didn't keep the 8bit support
# define DX8QUAKE_GET_GL_MAX_SIZE			// D3D8 wrapper obtains the maxsize from the video card
# define DX8QUAKE_NO_BINDTEXFUNC			// SGIS/ancient GL pathway removal
# define DX8QUAKE_NO_GL_ZTRICK				// DX8QUAKE hates gl_ztrick; clear the buffers every time
# define DX8QUAKE_GL_READPIXELS_NO_RGBA		// Wrapper only supports GL_RGBA; not GL_RGBA like envmap command uses
# define DX8QUAKE_VSYNC_COMMANDLINE_PARAM	// Vsync command line param option ... -vsync
#endif

#ifdef D3DQUAKE
# define D3DQ_EXTRA_FEATURES // (D3D_FEATURE)
# define D3DQ_CANNOT_DO_THIS // D3D_NOT_CAPABLE
# define D3DQ_WORKAROUND     // D3DQ_WORKAROUND
# define D3DQ_CONTRAST
#endif

#ifdef PSP
# define PSP_NETWORKING_CODE	// The PSP has 2 types of network: peer to peer and traditional wi-fi
# define PSP_MP3_SUPPORT		// The PSP engine has MP3 support; it can be turned off / disabled
# define PSP_LOW_MEMORY_SYSTEM	// Memory limitations prevent the PSP from running phatmaps
# define PSP_SYSTEM_STATS		// Features for investigation for Win32 consideration plus OS X, etc
# define PSP_PSPGU				// 

# define PSP_INPUT_CONTROLS
#ifdef PSP_INPUT_CONTROLS 
# define PSP_ANALOG_STICK		// The PSP Analog stick is a little joystick that serves as the mouse
# define PSP_BUTTON_CONTROLS
#endif

# define PSP_ONSCREEN_KEYBOARD	// The PSP engine "On-screen keyboard" since the PSP has none
# define NO_ASSEMBLY			// Self explanatory

// Debug

#define PSP_NO_PROQUAKE_YET
#define PSP_FILESYSTEM_RECONCILE
#define PSP_PSPGU_RECONCILE
#define PSP_FOG_RECONCILE
#define PSP_ALTERED_LIMITS
#define PSP_FIXME
#define PSP_DUMB_DEFAULTS

#undef FITZQUAKE_PROTOCOL 	// PSP doesn't have enough memory as it is
#undef SUPPORTS_AUTOID    	// Let's no go there yet
#undef CHASE_CAM_FIX		// Crashy, crashy
#undef SUPPORTS_TRANSFORM_INTERPOLATION // Not yet ... but well it already does!

// Note: the PSP supports overbright lighting, vertex lighting, interpolation, colored lights
// We will make it support Half-Life BSP.  It does support r_wateralpha.
// It does support 2D transparency.  It does support FOG.
// It does not support colored skins
// ADDITIONS
# define SUPPORTS_KUROK
#ifdef SUPPORTS_KUROK
//#define SUPPORTS_KUROK_PROTOCOL	// This doesn't exist!  // The protocol part
#define SUPPORTS_COLORED_LIGHTS		// The colored lights part
#endif
#define SUPPORTS_HARDWARE_ANIM_INTERPOLATION
# define SUPPORTS_GLHOMFIX_NEARWATER
# define SUPPORTS_2DPICS_ALPHA
// SUPPORTS_TRANSPARENT_SBAR
// OVERBRIGHTS
//HLBSP
//FOG?
// GLSKIN?
#endif

#ifdef MACOSX
# define MACOSX_EXTRA_FEATURES
# define MACOSX_TEXRAM_CHECK
# define MACOSX_UNKNOWN_DIFFERENCE
# define MACOSX_QUESTIONABLE_VALUE
# define MACOSX_NETWORK_DIFFERENCE
# define MACOSX_KEYBOARD_EXTRAS
# define MACOSX_KEYBOARD_KEYPAD
# define MACOSX_PASTING

# define MACOSX_SENS_RANGE
#endif

// Define Specific General Capabilities
#ifdef _WIN32
# define SUPPORTS_AVI_CAPTURE // Hopelessly Windows locked
# define SUPPORTS_INTERNATIONAL_KEYBOARD // I only know how to detect and address on Windows
# define SUPPORTS_CD_PLAYER // Windows yes, maybe Linux too.  Not MACOSX, Fruitz of Dojo not support it.
# define SUPPORTS_DEMO_AUTOPLAY // Windows only.  Uses file association
# define SUPPORTS_DIRECTINPUT
# define SUPPORTS_INTERNATIONAL_KEYBOARD // Windows only implementation for now?; the extra key byte
# define SUPPORTS_SYSSLEEP						// Make this work on OS X sometime; "usleep"
# define SUPPORTS_CLIPBOARD

# define WINDOWS_SCROLLWHEEL_PEEK				// CAPTURES MOUSEWHEEL WHEN keydest != game
# define HTTP_DOWNLOAD
//# define BUILD_MP3_VERSION


	// GLQUAKE additive features on top of _WIN32 only
	#if defined(GLQUAKE) && !defined(D3DQUAKE)
	# define SUPPORTS_ENHANCED_GAMMA 			// Windows only for now.  Probably can be multiplat in future.
	# define SUPPORTS_GLVIDEO_MODESWITCH  		// Windows only for now.  Probably can be multiplat in future.
	# define SUPPORTS_VSYNC 					// Vertical sync; only GL does this for now
	# define SUPPORTS_TRANSPARENT_SBAR 			// Not implemented in OSX?

	# define RELEASE_MOUSE_FULLSCREEN			// D3DQUAKE gets an error if it loses focus in fullscreen, so that'd be stupid
	#endif

	#if defined(GLQUAKE) && !defined(DX8QUAKE)
	# define OLD_SGIS							// Old multitexture ... for now.
# define INTEL_OPENGL_DRIVER_WORKAROUND // Windows only issue?  Or is Linux affected too?  OS X is not affected
#endif

	#ifndef GLQUAKE // WinQuake
	# define SUPPORTS_SW_ALTENTER				// Hacky ALT-ENTER
	#endif
#endif

#ifdef MACOSX
# define SUPPORTS_SYSSLEEP
# define FULL_BACKGROUND_VOLUME_CONTROL
#endif

#ifdef BUILD_MP3_VERSION
# define FULL_BACKGROUND_VOLUME_CONTROL
#endif

// Define Specific Rendering Capabilities

#ifdef GLQUAKE
# define SUPPORTS_SKYBOX						// Skyboxes are 24-bit; WinQuake can't do that (yet)
# define SUPPORTS_GLHOMFIX_NEARWATER			// Specific problem and solution for GL
# define SUPPORTS_CONSOLE_SIZING				// GL can size the console; WinQuake can't do that yet
//# define SUPPORTS_ENTITY_ALPHA					// Transparency
# define SUPPORTS_HARDWARE_ANIM_INTERPOLATION	// The hardware interpolation route
# define SUPPORTS_2DPICS_ALPHA					// Transparency of 2D pics
# define SUPPORTS_HLBSP							// Requires 24 bit color for now
# define SUPPORTS_GL_OVERBRIGHTS				// Overbright method GLQuake is using, WinQuake always had them
# define SUPPORTS_XFLIP

	#if !defined(D3DQUAKE)	// Any platform except D3DQUAKE
	# define SUPPORTS_GL_DELETETEXTURES			// D3DQuake isn't emulating them at this time
	# define SUPPORTS_FOG						// D3DQuake can't do the fog thing
	#endif

# define NO_MGRAPH								// This is for WinQuake rendering, we don't use it for GLQuake
# define GL_QUAKE_SKIN_METHOD					// GLQuake uses a different method for skinning
#endif


#if !defined(GLQUAKE) && !defined(PSP_HARDWARE_VIDEO)	// WinQuake
# define SUPPORTS_SOFTWARE_ANIM_INTERPOLATION	// WinQuake method for animation interpolation
# define SUPPORTS_HLBSP_SW
# define SUPPORTS_3D_CVARS						// 3-D LCD_X stuff
//# define SUPPORTS_ENTITY_ALPHA				// Transparency ... NOT YET
# define SUPPORTS_SOFTWARE_FTESTAIN				// FTESTAIN
# define SUPPORTS_SW_SKYBOX
# define SUPPORTS_SW_WATERALPHA
#endif


#ifdef SUPPORTS_AUTOID
#if defined(GLQUAKE) && !defined(D3DQUAKE) && !defined(DX8QUAKE)
# define SUPPORTS_AUTOID_HARDWARE				// The way I chose to do AUTOID for true OpenGL
#else
# define SUPPORTS_AUTOID_SOFTWARE				// WinQuake, DX8QUAKE and D3DQUAKE use calculations for 2D projection of 3D
#endif
#endif


// Alternate Methods

#ifdef FLASH
# define FLASH_FILE_SYSTEM
# define FLASH_CONSOLE_TRACE_ECHO
# define FLASH_SOUND_DIFFERENCE
#endif



// gl_keeptjunctions: Setting this to 0 will reduce the number of bsp polygon vertices by removing colinear points, however it produces holes in the BSP due to floating point precision errors.
// Baker: this should default to 1.  That's what FTE does.

// 0 = Remove colinear vertexes when loading the map. (will speed up the game performance, but will leave a few artifact pixels).
// Yields a few more frames per second.

// Note: gl_texsort 1 is for when multitexture is unavailable
//       gl_texsort 0 is for when multitexture is available
// Note: in glpro442 as of this time, gl_texsort 0 is going to be a fail becuase mtex is off.

// Discarded DX8QUAKE #ifdefs -- all functional but either not necessary or some such thing

// # define DX8QUAKE_CANNOT_DETECT_FULLSCREEN_BY_MODESTATE	// Detecting modestate == MS_FULLDIB can't distinguish between windowed and fullscreen modes
//# undef  SUPPORTS_GLVIDEO_MODESWITCH  // Not now, isn't working right
//#define DX8QUAKE_NO_DIALOGS				// No "starting Quake type "dialogs for DX8QUAKE, Improvement applicable to GL
//# define DX8QUAKE_NO_FRONT_BACK_BUFFER		// Baker: 4.42 - wasn't necessary, seems DX8 wrapper can do this
//# define DX8QUAKE_GL_MAX_SIZE_FAKE			// Baker  4.42 - this is no different than dx8 wrapper doing it right way
//# define DX8QUAKE_ALT_MODEL_TEXTURE				// Believe this is unnecessary skin sharpening option applicanle to GL
// # define DX8QUAKE_ALT_RESAMPLE				// Removing redundant function ... I think
//# define DX8QUAKE_NO_GL_TEXSORT_ZERO			// gl_texsort 0 is for no multitexture (can be applied to GL)
//# define DX8QUAKE_NO_GL_KEEPTJUNCTIONS_ZERO		// gl_keeptjunction 0 is (can be applied to GL) * Note below
//# define DX8QUAKE_BAKER_ALTTAB_HACK				// Possibly removeable now



int build_number (void);
void Host_Version_f (void);
char *VersionString (void);

#endif 

