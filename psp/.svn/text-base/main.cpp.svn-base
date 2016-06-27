/*
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

#define CONSOLE_DEBUG 0

#include <stdexcept>
#include <vector>
#include <sys/unistd.h>

#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <pspmoduleinfo.h>
#include <psppower.h>
#include <psprtc.h>
#include <pspsdk.h>

extern "C"
{
#include "../quakedef.h"
#include "module.h"

#ifdef SLIM_MODEL
int pspDveMgrSetVideoOut(int, int, int, int, int, int, int);
#endif
}

#include "battery.hpp"
#include "system.hpp"

// Running a dedicated server?
qboolean isDedicated = qfalse;

extern	int  com_argc;
extern	char **com_argv;

void Sys_ReadCommandLineFile (char* netpath);

namespace quake
{
	namespace main
	{
	// Clock speeds.
		extern const int		cpuClockSpeed	= scePowerGetCpuClockFrequencyInt();
		extern const int		ramClockSpeed	= cpuClockSpeed;
		extern const int		busClockSpeed	= scePowerGetBusClockFrequencyInt();

#ifdef PSP_SOFTWARE_VIDEO
		// How big a heap to allocate.
	#ifdef NORMAL_MODEL
		static const size_t  heapSize	= 17 * 1024 * 1024;
	#endif
    #ifdef SLIM_MODEL
		static const size_t  heapSize	= 16 * 1024 * 1024;
    #endif
#else
		// How big a heap to allocate.
	#ifdef NORMAL_MODEL
        static const size_t  heapSize	= 14 * 1024 * 1024;
	#endif
    #ifdef SLIM_MODEL
		static const size_t  heapSize	= 40 * 1024 * 1024;
    #endif
#endif
		// Should the main loop stop running?
		static volatile bool	quit			= false;

		// Is the PSP in suspend mode?
		static volatile bool	suspended		= false;

		static int exitCallback(int arg1, int arg2, void* common)
		{
			// Signal the main thread to stop.
			quit = true;
			return 0;
		}

		static int powerCallback(int unknown, int powerInfo, void* common)
		{
			if (powerInfo & PSP_POWER_CB_POWER_SWITCH || powerInfo & PSP_POWER_CB_SUSPENDING)
			{
				suspended = true;
			}
			else if (powerInfo & PSP_POWER_CB_RESUMING)
			{
			}
			else if (powerInfo & PSP_POWER_CB_RESUME_COMPLETE)
			{
				suspended = false;
			}

			return 0;
		}

		static int callback_thread(SceSize args, void *argp)
		{
			// Register the exit callback.
			const SceUID exitCallbackID = sceKernelCreateCallback("exitCallback", exitCallback, NULL);
			sceKernelRegisterExitCallback(exitCallbackID);

			// Register the power callback.
			const SceUID powerCallbackID = sceKernelCreateCallback("powerCallback", powerCallback, NULL);
			scePowerRegisterCallback(0, powerCallbackID);

			// Sleep and handle callbacks.
			sceKernelSleepThreadCB();
			return 0;
		}

		static int setUpCallbackThread(void)
		{
			const int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
			if (thid >= 0)
				sceKernelStartThread(thid, 0, 0);
			return thid;
		}


		static void disableFloatingPointExceptions()
		{
#ifndef _WIN32
			asm(
				".set push\n"
				".set noreorder\n"
				"cfc1    $2, $31\n"		// Get the FP Status Register.
				"lui     $8, 0x80\n"	// (?)
				"and     $8, $2, $8\n"	// Mask off all bits except for 23 of FCR. (? - where does the 0x80 come in?)
				"ctc1    $8, $31\n"		// Set the FP Status Register.
				".set pop\n"
				:						// No inputs.
				:						// No outputs.
				: "$2", "$8"			// Clobbered registers.
				);
#endif
		}
	}
}

extern bool bmg_type_changed;

using namespace quake;
using namespace quake::main;

#define MAX_NUM_ARGVS	50
#define MAX_ARG_LENGTH  64
static char *empty_string = "";
char    *f_argv[MAX_NUM_ARGVS];
int     f_argc;

int main(int argc, char *argv[])
{
/*
#ifdef SLIM_MODEL

	SceUID mod = pspSdkLoadStartModule("dvemgr.prx", PSP_MEMORY_PARTITION_KERNEL);
	if (mod < 0)
		Sys_Error ("Couldn't load dvemgr.prx, error %08X", mod);

//	vramExtender();
	pspDveMgrSetVideoOut(0, 0x1d2, 720, 480, 1, 15, 0);
#endif
*/
	// Set up the callback thread.
	setUpCallbackThread();

	// Disable floating point exceptions.
	// If this isn't done, Quake crashes from (presumably) divide by zero
	// operations.
	disableFloatingPointExceptions();

	// Allocate the heap.
	std::vector<unsigned char>	heap(heapSize, 0);

	// Initialise the Common module.

	// Get the current working dir.
	char currentDirectory[1024];
	char gameDirectory[1024];

	memset(gameDirectory, 0, sizeof(gameDirectory));
	memset(currentDirectory, 0, sizeof(currentDirectory));
	getcwd(currentDirectory, sizeof(currentDirectory) - 1);

	char   path_f[256];
	strcpy(path_f,currentDirectory);
	strcat(path_f,"/quake.cmdline");
	Sys_ReadCommandLineFile(path_f);

	char *args[MAX_NUM_ARGVS];

	for (int k =0; k < f_argc; k++) {
		int len = strlen(f_argv[k]);
		args[k] = new char[len+1];
		strcpy(args[k], f_argv[k]);
	}

#if CONSOLE_DEBUG
	if (f_argc > 1) {
		args[f_argc++] = "-condebug";
		COM_InitArgv(f_argc, args);
	}
	else {
		args[0] = "";
		args[1] = "-condebug";
		COM_InitArgv(2, args);
	}
#else
	if (f_argc > 1)
		COM_InitArgv(f_argc, args);
	else {
		args[0] = "";
		COM_InitArgv(0, NULL);
	}
#endif

#ifdef PSP_SOFTWARE_VIDEO
	// Bump up the clock frequency.
//	if (tcpipAvailable)
//	    scePowerSetClockFrequency(300, 300, 150); // Stop wifi problems
//    else
	    scePowerSetClockFrequency(333, 333, 166);
#else
	if (COM_CheckParm("-cpu333"))
	{
//	    if (tcpipAvailable)
//	        scePowerSetClockFrequency(300, 300, 150); // Stop wifi problems
//        else
	        scePowerSetClockFrequency(333, 333, 166);
    }
#endif

	if (COM_CheckParm("-gamedir")) {
		char* tempStr = com_argv[COM_CheckParm("-gamedir")+1];
		strncpy(gameDirectory, tempStr, sizeof(gameDirectory)-1);
	}
	else
	{
		strncpy(gameDirectory,currentDirectory,sizeof(gameDirectory)-1);
	}
	// Catch exceptions from here.
	try
	{
		// Initialise the Host module.
		quakeparms_t parms;
		memset(&parms, 0, sizeof(parms));
		parms.argc		= com_argc;
		parms.argv		= com_argv;
		parms.basedir	= gameDirectory;
		parms.memsize	= heap.size();
		parms.membase	= &heap.at(0);
		Host_Init(&parms);

		// Precalculate the tick rate.
		const float oneOverTickRate = 1.0f / sceRtcGetTickResolution();

		// Record the time that the main loop started.
		u64 lastTicks;
		sceRtcGetCurrentTick(&lastTicks);

		// Enter the main loop.

#ifdef PSP_MP3HARDWARE_MP3LIB
		extern int changeMp3Volume;
		extern void CDAudio_VolumeChange(float bgmvolume);
#endif
		while (!quit)
		{

#ifdef PSP_MP3HARDWARE_MP3LIB
			if(changeMp3Volume) CDAudio_VolumeChange(bgmvolume.value);
#endif

			// Handle suspend & resume.
			if (suspended)
			{
				// Suspend.
				S_ClearBuffer();

				quake::system::suspend();

				// Wait for resume.
				while (suspended && !quit)
				{
					sceDisplayWaitVblankStart();
				}

				// Resume.
				quake::system::resume();

				// Reset the clock.
				sceRtcGetCurrentTick(&lastTicks);
			}

			// What is the time now?
			u64 ticks;
			sceRtcGetCurrentTick(&ticks);

			// How much time has elapsed?
			const unsigned int	deltaTicks		= ticks - lastTicks;
			const float			deltaSeconds	= deltaTicks * oneOverTickRate;

			// Check the battery status.
			battery::check();

			// Run the frame.
			Host_Frame(deltaSeconds);

			// Remember the time for next frame.
			lastTicks = ticks;
		}
	}
	catch (const std::exception& e)
	{
		// Report the error and quit.
		Sys_Error("C++ Exception: %s", e.what());
		return 0;
	}

	// Quit.
	Sys_Quit();
	return 0;
}

void Sys_ReadCommandLineFile (char* netpath)
{
	int 	in;
	int     remaining, count;
	char    buf[4096];
	int     argc = 1;

	remaining = Sys_FileOpenRead (netpath, &in);

	if (in > 0 && remaining > 0) {
		count = Sys_FileRead (in, buf, 4096);
		f_argv[0] = empty_string;

		char* lpCmdLine = buf;

		while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
		{
			while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				f_argv[argc] = lpCmdLine;
				argc++;

				while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
					lpCmdLine++;

				if (*lpCmdLine)
				{
					*lpCmdLine = 0;
					lpCmdLine++;
				}
			}
		}
		f_argc = argc;
	} else {
		f_argc = 0;
	}

	if (in > 0)
		Sys_FileClose (in);
}
