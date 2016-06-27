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
// sys_win.c -- Win32 system interface code

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include "conproc.h"
#include <limits.h>
#include <errno.h>
#include <direct.h>

// JPG 3.30 - need these for synchronization
#include <fcntl.h>
#include <sys/stat.h>

#define MINIMUM_WIN_MEMORY		0x0c00000 // 12Mb
#define MAXIMUM_WIN_MEMORY		0x2000000 // Baker 3.75 - increase to 32MB minimum

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running
										//  dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

//qboolean OnChange_sys_highpriority (cvar_t *, char *);
cvar_t	sys_highpriority = {"sys_highpriority", "0", false}; // Baker 3.99r: sometimes this worsens online performance, so not saving this to config

int			starttime;
qboolean	ActiveApp, Minimized;
qboolean	WinNT;

//static	void		*memBasePtr = 0;//Reckless

static double		pfreq;
static double		curtime = 0.0;
static double		lastcurtime = 0.0;
static int			lowshift;
qboolean			isDedicated;
static qboolean		sc_return_on_enter = false;
HANDLE				hinput, houtput;

static char			*tracking_tag = "Clams & Mooses";

static HANDLE	tevent;
static HANDLE	hFile;
static HANDLE	heventParent;
static HANDLE	heventChild;

void MaskExceptions (void);
void Sys_InitDoubleTime (void);
void Sys_PopFPCW (void);
void Sys_PushFPCW_SetHigh (void);


volatile int					sys_checksum;


/*
================
Sys_PageIn
================
*/
void Sys_PageIn (void *ptr, int size)
{
	byte	*x;
	int		m, n;

// touch all the memory to make sure it's there. The 16-page skip is to
// keep Win 95 from thinking we're trying to page ourselves in (we are
// doing that, of course, but there's no reason we shouldn't)
	x = (byte *)ptr;

	for (n=0 ; n<4 ; n++)
	{
		for (m=0 ; m<(size - 16 * 0x1000) ; m += 4)
		{
			sys_checksum += *(int *)&x[m];
			sys_checksum += *(int *)&x[m + 16 * 0x1000];
		}
	}
}


/*
===============================================================================

SYNCHRONIZATION - JPG 3.30

===============================================================================
*/

int hlock;
_CRTIMP int __cdecl _open(const char *, int, ...);
_CRTIMP int __cdecl _close(int);

/*
================
Sys_GetLock
================
*/
void Sys_GetLock (void)
{
	int i;

	for (i = 0 ; i < 10 ; i++)
	{
		hlock = _open(va("%s/lock.dat",com_gamedir), _O_CREAT | _O_EXCL, _S_IREAD | _S_IWRITE);
		if (hlock != -1)
			return;
		Sleep(1000);
	}

	Sys_Printf("Warning: could not open lock; using crowbar\n");
}

/*
================
Sys_ReleaseLock
================
*/
void Sys_ReleaseLock (void)
{
	if (hlock != -1)
		_close(hlock);
	unlink(va("%s/lock.dat",com_gamedir));
}

#ifndef WITHOUT_WINKEYHOOK

static HHOOK WinKeyHook;
static qboolean WinKeyHook_isActive;

LRESULT CALLBACK LLWinKeyHook(int Code, WPARAM wParam, LPARAM lParam);
//qboolean OnChange_sys_disableWinKeys(cvar_t *var, char *string);
cvar_t	sys_disableWinKeys = {"sys_disableWinKeys", "0", true};

void OnChange_sys_disableWinKeys (void) /*(cvar_t *var, char *string)*/ {
	if (sys_disableWinKeys.value) { //(atof(string)) {
		if (!WinKeyHook_isActive) {
			if ((WinKeyHook = SetWindowsHookEx(13, LLWinKeyHook, global_hInstance, 0))) {
				WinKeyHook_isActive = true;
			} else {
				Con_Printf("Failed to install winkey hook.\n");
				Con_Printf("Microsoft Windows NT 4.0, 2000 or XP is required.\n");
				return; // true;
			}
		}
	} else {
		if (WinKeyHook_isActive) {
			UnhookWindowsHookEx(WinKeyHook);
			WinKeyHook_isActive = false;
		}
	}
	return; // false;
}

LRESULT CALLBACK LLWinKeyHook(int Code, WPARAM wParam, LPARAM lParam) {
	PKBDLLHOOKSTRUCT p;

	p = (PKBDLLHOOKSTRUCT) lParam;

//	Baker 3.99r: we aren't allowing these to be bound, at least now right now
	if (ActiveApp) {
		switch(p->vkCode) {
			case VK_LWIN: /*Key_Event (K_LWIN, !(p->flags & LLKHF_UP));*/ return 1;
			case VK_RWIN: /*Key_Event (K_RWIN, !(p->flags & LLKHF_UP));*/ return 1;
			case VK_APPS: /*Key_Event (K_MENU, !(p->flags & LLKHF_UP));*/ return 1;
		}
	}

	return CallNextHookEx(NULL, Code, wParam, lParam);
}

#endif


int Sys_SetPriority(int priority) {
    DWORD p;

	switch (priority) {
		case 0:	p = IDLE_PRIORITY_CLASS; break;
		case 1:	p = NORMAL_PRIORITY_CLASS; break;
		case 2:	p = HIGH_PRIORITY_CLASS; break;
		case 3:	p = REALTIME_PRIORITY_CLASS; break;
		default: return 0;
	}

	return SetPriorityClass(GetCurrentProcess(), p);
}

void OnChange_sys_highpriority (void) /*(cvar_t *var, char *s)*/ {
	int ok, q_priority;
	char *desc;
	float priority;

	priority = (int)sys_highpriority.value; //atof(s);
	if (priority == 1) {
		q_priority = 2;
		desc = "high";
	} else if (priority == -1) {
		q_priority = 0;
		desc = "low";
	} else {
		q_priority = 1;
		desc = "normal";
	}

	if (!(ok = Sys_SetPriority(q_priority))) {
		Con_Printf("Changing process priority failed\n");
		return; // true;
	}

	Con_Printf("Process priority set to %s\n", (q_priority == 1) ? "normal" : ((q_priority == 2) ? "high" : "low"));
	return; // false;
}


/*
===============================================================================
FILE IO
===============================================================================
*/

#define	MAX_HANDLES		10 //Baker 3.79 - FitzQuake uses 32, but I don't think we should ever have 32 file handles open // johnfitz -- was 10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int		pos;
	int		end;
	int		t;

	t = VID_ForceUnlockedAndReturnState ();

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	VID_ForceLockState (t);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int	i, retval, t;

	t = VID_ForceUnlockedAndReturnState ();

	i = findhandle ();

	f = fopen(path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = filelength(f);
	}

	VID_ForceLockState (t);

	return retval;
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int	i, t;

	t = VID_ForceUnlockedAndReturnState ();

	i = findhandle ();

	if (!(f = fopen(path, "wb")))
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	VID_ForceLockState (t);

	return i;
}

void Sys_FileClose (int handle)
{
	int		t;

	t = VID_ForceUnlockedAndReturnState ();
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
	VID_ForceLockState (t);
}

void Sys_FileSeek (int handle, int position)
{
	int		t;

	t = VID_ForceUnlockedAndReturnState ();
	fseek (sys_handles[handle], position, SEEK_SET);
	VID_ForceLockState (t);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	int		t, x;

	t = VID_ForceUnlockedAndReturnState ();
	x = fread (dest, 1, count, sys_handles[handle]);
	VID_ForceLockState (t);

	return x;
}

int Sys_FileWrite (int handle, void *data, int count)
{
	int		t, x;

	t = VID_ForceUnlockedAndReturnState ();
	x = fwrite (data, 1, count, sys_handles[handle]);
	VID_ForceLockState (t);

	return x;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	int		retval;

#ifndef GLQUAKE
	int		t;

	t = VID_ForceUnlockedAndReturnState ();
#endif

	if ((f = fopen(path, "rb")))
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}

#ifndef GLQUAKE
	VID_ForceLockState (t);
#endif
	return retval;
}

void Sys_mkdir (char *path) {
	_mkdir (path);
}

/*
===============================================================================
SYSTEM IO
===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length) {
	DWORD  flOldProtect;

	if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
   		Sys_Error("Protection change failed");
}


#ifndef _M_IX86

void Sys_SetFPCW (void)
{
}

void Sys_PushFPCW_SetHigh (void)
{
}

void Sys_PopFPCW (void)
{
}

void MaskExceptions (void)
{
}

#endif




void Sys_Error (char *error, ...) {
	va_list		argptr;
	char		text[1024];
	char		text2[1024];
	char		*text3 = "Press Enter to exit\n";
	char		*text4 = "***********************************\n";
	char		*text5 = "\n";
	DWORD		dummy;
	double		starttime;
	static int	in_sys_error0 = 0;
	static int	in_sys_error1 = 0;
	static int	in_sys_error2 = 0;
	static int	in_sys_error3 = 0;

	if (!in_sys_error3) {
		in_sys_error3 = 1;
#ifndef GLQUAKE
		VID_ForceUnlockedAndReturnState ();
#endif
	}

	va_start (argptr, error);
	vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	if (isDedicated) {
		va_start (argptr, error);
		vsnprintf (text, sizeof(text), error, argptr);
		va_end (argptr);

		snprintf (text2, sizeof(text2), "ERROR: %s\n", text);
		WriteFile (houtput, text5, strlen (text5), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);
		WriteFile (houtput, text2, strlen (text2), &dummy, NULL);
		WriteFile (houtput, text3, strlen (text3), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);

		starttime = Sys_DoubleTime ();
		sc_return_on_enter = true;	// so Enter will get us out of here

		while (!Sys_ConsoleInput () && ((Sys_DoubleTime () - starttime) < CONSOLE_ERROR_TIMEOUT))
		{
		}
	}
	else
	{
	// switch to windowed so the message box is visible, unless we already
	// tried that and failed
		if (!in_sys_error0)
		{
			in_sys_error0 = 1;
			VID_SetDefaultMode ();
#ifdef UNICODE
			{
				TCHAR ttext[1024];
				mbstowcs(ttext,text,strlen(text));
				MessageBox(NULL, ttext, TEXT("Quake Error"), MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
			}
#else
			MessageBox(NULL, text, TEXT("Quake Error"), MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
#endif
		}
		else
		{
#ifdef UNICODE
			{
				TCHAR ttext[1024];
				mbstowcs(ttext,text,strlen(text));
				MessageBox(NULL, ttext, TEXT("Double Quake Error"),
						   MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
			}
#else
			MessageBox(NULL, text, TEXT("Double Quake Error"), MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
#endif
		}
	}

	if (!in_sys_error1)
	{
		in_sys_error1 = 1;
		Host_Shutdown ();
	}

// shut down QHOST hooks if necessary
	if (!in_sys_error2) {
		in_sys_error2 = 1;
		DeinitConProc ();
	}

/*	if (memBasePtr)
	{
		VirtualFree(memBasePtr, 0, MEM_RELEASE);
	} */

	exit (1);
}

void Sys_Printf (char *fmt, ...) {
	va_list		argptr;
	char		text[2048];	// JPG - changed this from 1024 to 2048
	DWORD		dummy;

	if (!isDedicated)
		return;

	va_start (argptr,fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	// JPG 1.05 - translate to plain text
	if (pq_dequake.value)
	{
		unsigned char *ch;
		for (ch = text ; *ch ; ch++)
			*ch = dequake[*ch];
	}

	WriteFile(houtput, text, strlen (text), &dummy, NULL);

	// JPG 3.00 - rcon (64 doesn't mean anything special, but we need some extra space because NET_MAXMESSAGE == RCON_BUFF_SIZE)
	if (rcon_active  && (rcon_message.cursize < rcon_message.maxsize - strlen(text) - 64))
	{
		rcon_message.cursize--;
		MSG_WriteString(&rcon_message, text);
	}

}

extern char *hunk_base; // JPG - needed for Sys_Quit

void Sys_Quit (void) {
#ifndef GLQUAKE
	VID_ForceUnlockedAndReturnState ();
#endif

	Host_Shutdown();

	if (tevent)
		CloseHandle (tevent);

	if (isDedicated)
		FreeConsole ();
#ifndef WITHOUT_WINKEYHOOK
	if (WinKeyHook_isActive)
		UnhookWindowsHookEx(WinKeyHook);
#endif
	// shut down QHOST hooks if necessary
	DeinitConProc ();

/*	if (memBasePtr)//Reckless
	{
		VirtualFree(memBasePtr, 0, MEM_RELEASE);
	}*/

	// JPG - added this to see if it would fix the strange running out of system
	// memory after running quake multiple times
	free(hunk_base);

	exit (0);
}

// joe: not using just float for timing any more,
// this is copied from ZQuake source to fix overspeeding.

static	double	pfreq;
static qboolean	hwtimer = false;

void Sys_InitDoubleTime (void) {
	__int64	freq;

	if (!COM_CheckParm("-nohwtimer") && QueryPerformanceFrequency ((LARGE_INTEGER *)&freq) && freq > 0) {
		// hardware timer available
		pfreq = (double)freq;
		hwtimer = true;
	} else {
		// make sure the timer is high precision, otherwise NT gets 18ms resolution
		timeBeginPeriod (1);
	}
}

double Sys_DoubleTime (void) {
	__int64		pcount;
	static	__int64	startcount;
	static	DWORD	starttime;
	static qboolean	first = true;
	DWORD	now;

	if (hwtimer) {
		QueryPerformanceCounter ((LARGE_INTEGER *)&pcount);
		if (first) {
			first = false;
			startcount = pcount;
			return 0.0;
		}
		// TODO: check for wrapping
		return (pcount - startcount) / pfreq;
	}

	now = timeGetTime ();

	if (first) {
		first = false;
		starttime = now;
		return 0.0;
	}

	if (now < starttime) // wrapped?
		return (now / 1000.0) + (LONG_MAX - starttime / 1000.0);

	if (now - starttime == 0)
		return 0.0;

	return (now - starttime) / 1000.0;
}

char *Sys_ConsoleInput (void) {
	static char	text[256];
	static int		len;
	INPUT_RECORD	recs[1024];
	int		dummy, ch, numread, numevents;

	if (!isDedicated)
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT) {
			if (!recs[0].Event.KeyEvent.bKeyDown) {
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch) {
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);
						if (len) {
							text[len] = 0;
							len = 0;
							return text;
						}
						else if (sc_return_on_enter)
						{
						// special case to allow exiting from the error handler on Enter
							text[0] = '\r';
							len = 0;
							return text;
						}

						break;

					case '\b':
						WriteFile(houtput, "\b \b", 3, &dummy, NULL);
						if (len)
							len--;
						break;

					default:
						if (ch >= ' ') {
							WriteFile(houtput, &ch, 1, &dummy, NULL);
							text[len] = ch;
							len = (len + 1) & 0xff;
						}
						break;
				}
			}
		}
	}

	return NULL;
}

void Sys_Sleep (void)
{
	Sleep (1);
}

void Sys_SendKeyEvents (void) {
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
	// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}
}



/*
==============================================================================
 WINDOWS CRAP
==============================================================================
*/


void SleepUntilInput (int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}

#ifdef NO_ASSEMBLY // Formerly:  !id386
void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

void Sys_SetFPCW (void)
{
}

void MaskExceptions (void)
{
}

void Sys_PopFPCW (void)
{
}

void Sys_PushFPCW_SetHigh (void)
{
}

#endif


/********************************* CLIPBOARD *********************************/

#define	SYS_CLIPBOARD_SIZE		256

char *Sys_GetClipboardData (void) {
	HANDLE		th;
	char		*clipText, *s, *t;
	static	char	clipboard[SYS_CLIPBOARD_SIZE];

	if (!OpenClipboard(NULL))
		return NULL;

	if (!(th = GetClipboardData(CF_TEXT))) {
		CloseClipboard ();
		return NULL;
	}

	if (!(clipText = GlobalLock(th))) {
		CloseClipboard ();
		return NULL;
	}

	s = clipText;
	t = clipboard;
	while (*s && t - clipboard < SYS_CLIPBOARD_SIZE - 1 && *s != '\n' && *s != '\r' && *s != '\b')
		*t++ = *s++;
	*t = 0;

	GlobalUnlock (th);
	CloseClipboard ();

	return clipboard;
}

// copies given text to clipboard
void Sys_CopyToClipboard(char *text) {
	char *clipText;
	HGLOBAL hglbCopy;

	if (!OpenClipboard(NULL))
		return;

	if (!EmptyClipboard()) {
		CloseClipboard();
		return;
	}

	if (!(hglbCopy = GlobalAlloc(GMEM_DDESHARE, strlen(text) + 1))) {
		CloseClipboard();
		return;
	}

	if (!(clipText = GlobalLock(hglbCopy))) {
		CloseClipboard();
		return;
	}

	strcpy((char *) clipText, text);
	GlobalUnlock(hglbCopy);
	SetClipboardData(CF_TEXT, hglbCopy);

	CloseClipboard();
}

/*
==================
WinMain
==================
*/
HINSTANCE	global_hInstance;
int			global_nCmdShow;
char		*argv[MAX_NUM_ARGVS];
static char	*empty_string = "";
HWND		hwnd_dialog;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	quakeparms_t	parms;
	double			time, oldtime, newtime;
	MEMORYSTATUS	lpBuffer;
	static	char	cwd[1024];
	int				t, i;
	RECT			rect;
	char			*ch;	// JPG 3.00 - for eliminating quotes from exe name
	char			*e;
	FILE			*fpak0;
	char			fpaktest[1024], exeline[MAX_OSPATH];
    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

	global_hInstance = hInstance;
	global_nCmdShow = nCmdShow;

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus (&lpBuffer);

#ifdef UNICODE
	{
		TCHAR tcwd[1024];
		if (!GetCurrentDirectory (sizeof(tcwd), tcwd))
			Sys_Error ("Couldn't determine current directory");
		wcstombs(cwd,tcwd,sizeof(cwd));
	}
#else
	if (!GetCurrentDirectory (sizeof(cwd), cwd))
		Sys_Error ("Couldn't determine current directory");
#endif

	if (cwd[strlen(cwd)-1] == '/')
		cwd[strlen(cwd)-1] = 0;

	// Baker 3.76 - playing demos via file association

	snprintf (fpaktest, sizeof(fpaktest), "%s/id1/pak0.pak", cwd); // Baker 3.76 - Sure this isn't gfx.wad, but let's be realistic here

	if(!(i = GetModuleFileName(NULL, com_basedir, sizeof(com_basedir)-1)))
		Sys_Error("FS_InitFilesystemEx: GetModuleFileName failed");

	com_basedir[i] = 0; // ensure null terminator

//	sprintf(exeline, "%s %%1", com_basedir);
	snprintf(exeline, sizeof(exeline), "%s \"%%1\"", com_basedir);

	if (COM_CheckParm ("-noassocdem") == 0) {
		void CreateSetKeyExtension(void);
		void CreateSetKeyDescription(void);
		void CreateSetKeyCommandLine(const char*  exeline);


		// Build registry entries
		CreateSetKeyExtension();
		CreateSetKeyDescription();
		CreateSetKeyCommandLine(exeline);
		Con_Printf("Registry Init\n");
		// End build entries
	}

	// Strip to the bare path; needed for demos started outside Quake folder
	for (e = com_basedir+strlen(com_basedir)-1; e >= com_basedir; e--)
			if (*e == '/' || *e == '\\')
			{
				*e = 0;
				break;
			}

	snprintf (cwd, sizeof(cwd), "%s", com_basedir);



	if (fpak0 = fopen(fpaktest, "rb"))  {
		fclose( fpak0 ); // Pak0 found so close it; we have a valid directory
	} else {
		// Failed to find pak0.pak, use the dir the exe is in
		snprintf (cwd, sizeof(cwd), "%s", com_basedir);
	}
	// End Baker 3.76

	parms.basedir = cwd;
	parms.cachedir = NULL;

	parms.argc = 1;
	argv[0] = GetCommandLine();	// JPG 3.00 - was empty_string
	lpCmdLine[-1] = 0;			// JPG 3.00 - isolate the exe name, eliminate quotes
	if (argv[0][0] == '\"')
		argv[0]++;
	if (ch = strchr(argv[0], '\"'))
		*ch = 0;


	while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS)) {
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine) {
			if (*lpCmdLine == '\"')
			{
				lpCmdLine++;

				argv[parms.argc] = lpCmdLine;
				parms.argc++;

				while (*lpCmdLine && *lpCmdLine != '\"') // this include chars less that 32 and greate than 126... is that evil?
					lpCmdLine++;
			}
			else
			{
				argv[parms.argc] = lpCmdLine;
				parms.argc++;

				while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
					lpCmdLine++;
			}

			if (*lpCmdLine) {
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

	parms.argv = argv;

	COM_InitArgv (parms.argc, parms.argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	isDedicated = (COM_CheckParm ("-dedicated"));

#if !defined(DX8QUAKE_NO_DIALOGS)
	if (!isDedicated)
	{
		hwnd_dialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

		if (hwnd_dialog)
		{
			if (GetWindowRect (hwnd_dialog, &rect))
			{
				if (rect.left > (rect.top * 2))
				{
					SetWindowPos (hwnd_dialog, 0,
						(rect.left / 2) - ((rect.right - rect.left) / 2),
						rect.top, 0, 0,
						SWP_NOZORDER | SWP_NOSIZE);
				}
			}

			ShowWindow (hwnd_dialog, SW_SHOWDEFAULT);
			UpdateWindow (hwnd_dialog);
			SetForegroundWindow (hwnd_dialog);
		}
	}
#endif

// take the greater of all the available memory or half the total memory,
// but at least 8 Mb and no more than 16 Mb, unless they explicitly request otherwise
	parms.memsize = lpBuffer.dwAvailPhys;

	if (parms.memsize < MINIMUM_WIN_MEMORY)
		parms.memsize = MINIMUM_WIN_MEMORY;

	if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
		parms.memsize = lpBuffer.dwTotalPhys >> 1;

	if (parms.memsize > MAXIMUM_WIN_MEMORY)
		parms.memsize = MAXIMUM_WIN_MEMORY;

	if ((t = COM_CheckParm("-heapsize")) != 0 && t + 1 < com_argc)
		parms.memsize = atoi (com_argv[t+1]) * 1024;

	if ((t = COM_CheckParm("-mem")) != 0 && t + 1 < com_argc)
		parms.memsize = atoi (com_argv[t+1]) * 1024 * 1024;

	parms.membase = Q_malloc (parms.memsize);

	// Baker 3.99n: JoeQuake doesn't do this next one
	Sys_PageIn (parms.membase, parms.memsize);

	if (!(tevent = CreateEvent(NULL, FALSE, FALSE, NULL)))
		Sys_Error ("Couldn't create event");

//	memBasePtr = parms.membase;

	if (isDedicated)
	{
		if (!AllocConsole ())
			Sys_Error ("Couldn't create dedicated server console");

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);

	// give QHOST a chance to hook into the console
		if ((t = COM_CheckParm ("-HFILE")) > 0)
		{
			if (t < com_argc)
				hFile = (HANDLE)atoi (com_argv[t+1]);
		}

		if ((t = COM_CheckParm ("-HPARENT")) > 0)
		{
			if (t < com_argc)
				heventParent = (HANDLE)atoi (com_argv[t+1]);
		}

		if ((t = COM_CheckParm ("-HCHILD")) > 0)
		{
			if (t < com_argc)
				heventChild = (HANDLE)atoi (com_argv[t+1]);
		}

		InitConProc (hFile, heventParent, heventChild);
	}

	Sys_Init ();

// because sound is off until we become active
	S_BlockSound ();

	Sys_Printf ("Host_Init\n");
	Host_Init (&parms);

	oldtime = Sys_DoubleTime ();

    /* main window message loop */
	while (1) {
		if (isDedicated) {
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;

			while (time < sys_ticrate.value )
			{
				Sys_Sleep();
				newtime = Sys_DoubleTime ();
				time = newtime - oldtime;
			}
		}
		else
		{
#ifdef D3DQ_WORKAROUND
			Sleep(1); // For NVIDIA drivers on Windows 2000
#endif
		// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && (!ActiveApp && !DDActive)) || Minimized || block_drawing) {
				SleepUntilInput (PAUSE_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			} else if (!ActiveApp && !DDActive) {
				SleepUntilInput (NOT_FOCUS_SLEEP);
			}

			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;
		}

		Host_Frame (time);
		oldtime = newtime;

	}

    // return success of application
    return TRUE;
}

void Sys_OpenQuakeFolder_f(void)
{
	HINSTANCE			ret;
/*	qboolean			switch_to_windowed = false;

#ifdef GLQUAKE
	if ((switch_to_windowed = VID_CanSwitchedToWindowed()))
		VID_Windowed();
#endif
*/
	ret = ShellExecute(0, "Open", com_basedir, NULL, NULL, SW_NORMAL);

	if (ret==0)
		Con_Printf("Opening Quake folder failed\n");
	else
		Con_Printf("Quake folder opened in Explorer\n");

}

void Sys_HomePage_f(void)
{
	HINSTANCE			ret;
/*
	qboolean			switch_to_windowed = false;

	if ((switch_to_windowed = VID_CanSwitchedToWindowed()))
		VID_Windowed();
*/
//	char	outstring[CON_TEXTSIZE]="";

//	snprintf(outstring, size(outstring), "%s", ENGINE_HOMEPAGE_URL);

	ret = ShellExecute(0, NULL, ENGINE_HOMEPAGE_URL, NULL, NULL, SW_NORMAL);

	if (ret==0)
		Con_Printf("Opening home page failed\n");
	else
		Con_Printf("%s home page opened in default browser\n", ENGINE_NAME);

}

char q_system_string[1024] = "";

void Sys_InfoPrint_f(void) {
	Con_Printf ("%s\n", q_system_string);
}

void Sys_Sleep_f (void) {
	if (Cmd_Argc() == 1) {
		Con_Printf ("Usage: %s <milliseconds> : let system sleep and yield cpu\n", Cmd_Argv(0));
		return;
	}

	Con_Printf ("Sleeping %i milliseconds ...\n", atoi(Cmd_Argv(1)));		
	Sleep (atoi(Cmd_Argv(1)));
}


char * SYSINFO_GetString(void)
{
	return q_system_string;
}
int     SYSINFO_memory = 0;
int     SYSINFO_MHz = 0;
char *  SYSINFO_processor_description = NULL;
char *  SYSINFO_3D_description        = NULL;

qboolean	WinNT, Win2K, WinXP, Win2K3, WinVISTA;
char WinVers[30];

void Sys_InfoInit(void)
{
	MEMORYSTATUS    memstat;
	LONG            ret;
	HKEY            hKey;
	OSVERSIONINFO	vinfo;
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);


	if (!GetVersionEx(&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) || (vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
		Sys_Error ("Qrack requires at least Win95 or greater.");

	WinNT = (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) ? true : false;

	if ((Win2K = WinNT && (vinfo.dwMajorVersion == 5) && (vinfo.dwMinorVersion == 0)))
		dpsnprintf(WinVers, sizeof(WinVers),"Windows 2000");
	else if ((WinXP = WinNT && (vinfo.dwMajorVersion == 5) && (vinfo.dwMinorVersion == 1)))
		dpsnprintf(WinVers, sizeof(WinVers),"Windows XP");
	else if ((Win2K3 = WinNT && (vinfo.dwMajorVersion == 5) && (vinfo.dwMinorVersion == 2)))
		dpsnprintf(WinVers, sizeof(WinVers),"Windows 2003");
	else if ((WinVISTA = WinNT && (vinfo.dwMajorVersion == 6) && (vinfo.dwMinorVersion == 0)))
		dpsnprintf(WinVers, sizeof(WinVers),"Windows Vista");
	else if (vinfo.dwMajorVersion >= 6)
		dpsnprintf(WinVers, sizeof(WinVers),"Windows Vista or later");
	else
		dpsnprintf(WinVers, sizeof(WinVers),"Windows 95/98/ME");

	Con_Printf("Operating System: %s\n", WinVers);

	GlobalMemoryStatus(&memstat);
	SYSINFO_memory = memstat.dwTotalPhys;

	ret = RegOpenKey(
	          HKEY_LOCAL_MACHINE,
	          "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
	          &hKey);

	if (ret == ERROR_SUCCESS) {
		DWORD type;
		byte  data[1024];
		DWORD datasize;

		datasize = 1024;
		ret = RegQueryValueEx(
		          hKey,
		          "~MHz",
		          NULL,
		          &type,
		          data,
		          &datasize);

		if (ret == ERROR_SUCCESS  &&  datasize > 0  &&  type == REG_DWORD)
			SYSINFO_MHz = *((DWORD *)data);

		datasize = 1024;
		ret = RegQueryValueEx(
		          hKey,
		          "ProcessorNameString",
		          NULL,
		          &type,
		          data,
		          &datasize);

		if (ret == ERROR_SUCCESS  &&  datasize > 0  &&  type == REG_SZ)
			SYSINFO_processor_description = Q_strdup((char *) data);

		RegCloseKey(hKey);
	}

#ifdef GLQUAKE
	{
		extern const char *gl_renderer;

		if (gl_renderer  &&  gl_renderer[0])
			SYSINFO_3D_description = Q_strdup(gl_renderer);
	}
#endif

	dpsnprintf(q_system_string, sizeof(q_system_string), "%dMB", (int)(SYSINFO_memory / 1024. / 1024. + .5));



	if (SYSINFO_processor_description) {
		char	myprocessor[256];
		dpsnprintf(myprocessor, 256, (const char*)strltrim(SYSINFO_processor_description));
		strlcat(q_system_string, ", ", sizeof(q_system_string));
		strlcat(q_system_string, myprocessor, sizeof(q_system_string));
	}
	if (SYSINFO_MHz) {
		strlcat(q_system_string, va(" %dMHz", SYSINFO_MHz), sizeof(q_system_string));
	}
	if (SYSINFO_3D_description) {
		strlcat(q_system_string, ", ", sizeof(q_system_string));
		strlcat(q_system_string, SYSINFO_3D_description, sizeof(q_system_string));
	}

	//Con_Printf("sys: %s\n", q_system_string);
	Cmd_AddCommand ("homepage", Sys_HomePage_f);
	Cmd_AddCommand ("openquakefolder", Sys_OpenQuakeFolder_f);
	Cmd_AddCommand ("sysinfo", Sys_InfoPrint_f);
	Cmd_AddCommand ("sleep", Sys_Sleep_f);
	Cvar_RegisterVariable(&sys_disableWinKeys, OnChange_sys_disableWinKeys);
	Cvar_RegisterVariable(&sys_highpriority, OnChange_sys_highpriority);
}

void Sys_Init (void) {
	OSVERSIONINFO	vinfo;

	MaskExceptions ();
	Sys_SetFPCW ();

	Sys_InitDoubleTime ();

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) || (vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
		Sys_Error ("Quake requires at least Win95 or NT 4.0");

	WinNT = (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) ? true:false;
}
