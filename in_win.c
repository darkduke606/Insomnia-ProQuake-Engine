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
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#define DIRECTINPUT_VERSION	0x0700

#include "dinput.h"
#include "quakedef.h"
#include "winquake.h"
#include "dosisms.h"

#define DINPUT_BUFFERSIZE           16
#define iDirectInputCreate(a,b,c,d)	pDirectInputCreate(a,b,c,d)

HRESULT (WINAPI *pDirectInputCreate)(HINSTANCE hinst, DWORD dwVersion,
	LPDIRECTINPUT * lplpDirectInput, LPUNKNOWN punkOuter);

// mouse variables
cvar_t	m_filter = {"m_filter","0"};
qboolean commandline_dinput = false; // Is dinput on the command line?

//Baker 3.60 -- johnfitz -- compatibility with old Quake -- setting to 0 disables KP_* codes
cvar_t	cl_keypad = {"cl_keypad","0", true};

//Baker 3.85 -- DirectInput ON|OFF cvar!
cvar_t  m_directinput = {"m_directinput", "0", true};

qboolean flex_firstinit;
qboolean flex_firstjoyinit;
static qboolean	flex_dinput_acquired;
static qboolean	flex_dinput; // Baker 3.99n: this stores whether or not we have successfully loaded the dinput library!  Make it reflect that!

void IN_StartupMouse (void);
void IN_DeactivateMouse (void);
static qboolean	mouse_is_showing = true; // Baker 3.99n: we start with mouse showing
static qboolean mouse_lockedto_quakewindow = false;
static qboolean	restore_spi;
qboolean		flex_mouseinitialized;
qboolean		flex_mouseactive;
qboolean		m_dinput_skiponce=false;

static LPDIRECTINPUTDEVICE	g_pMouse;  //Baker 3.99n: is a mouse device of directinput?
static LPDIRECTINPUT		g_pdi;     //Baker 3.99n: is an instance of directinput?

void IN_PrintStatus(char *caption) {

	Con_SafePrintf("\nAt %s\n",caption);
	Con_SafePrintf("------------------------\n");
	Con_SafePrintf("commandline_dinput         = %d \n", commandline_dinput);
	Con_SafePrintf("restore_spi                = %d \n", restore_spi);
	Con_SafePrintf("flex_firstinit             = %d \n", flex_firstinit);
	Con_SafePrintf("flex_mouseinitialized      = %d \n", flex_mouseinitialized);
	Con_SafePrintf("flex_dinput                = %d \n", flex_dinput);
	Con_SafePrintf("flex_mouseactive           = %d \n", flex_mouseactive);
	Con_SafePrintf("flex_dinputacquired        = %d \n", flex_dinput_acquired);
	Con_SafePrintf("g_pmouse                   = %d \n", g_pMouse);
	Con_SafePrintf("g_pdi                      = %d \n", g_pdi);
	Con_SafePrintf("mouse_is_showing           = %d \n", mouse_is_showing);
	Con_SafePrintf("mouse_lockedto_quakewindow = %d \n\n", mouse_lockedto_quakewindow);

}

void IN_SetGlobals (void);

void IN_Restart(void) {
	// Restart the mouse like if the video mode changes
	
//	Con_SafePrintf("Mouse Restarted Called\n");
	
	// We have a problem!
	// M_directinput value is wrong going into this!  
	// But that's ok now, we use g_pmouse to determine if directinput needs released
	
//	IN_PrintStatus("IN_Restart Start");

	IN_DeactivateMouse(); // I think this always needs to happen before shutdown
	IN_ShowMouse();
	
	IN_Shutdown ();
	
	// Reset vars

	IN_SetGlobals();

	// End reset vars	
	
//	IN_PrintStatus("IN_Restart Middle");
	
	IN_Init ();

//	IN_SetGlobals();
	// Baker 3.85: This must be based on
	// whether windowed or not, etc.
	
	// Maybe this should be based on whether or not the mouse should be seen?
	
	//IN_StartupMouse();
	// What is the normal clipcursor?????
	// How do we know whether or not to do this?
	// For now, let's just do it no matter what!
	IN_ActivateMouse (); 
//	Con_SafePrintf("MarkJ\n");
	IN_HideMouse ();

//	IN_PrintStatus("IN_Restart End");
}


void IN_DirectInput_f(void) {
// Baker: the intent of this is to turn dinput on if it is off
//                                 turn it off it is on
//                                 do nothing if there is no change
//                                 but sync the cvar if it isn't

// This is called when
// ... the menu is used
// ... the config is exec'd
// ... the cvar is used

// Needs to be ignored if host isn't init'd AND
// cmdline is the same as this value

	if (m_dinput_skiponce) {
		// Workaround to avoid cvar goofiness on config load
		m_dinput_skiponce = false;
		return;
	}

	if (commandline_dinput && !m_directinput.value) {
		// If cvar is forced, do not allow it to be changed
		m_dinput_skiponce = true; // And don't re-trigger it!
		Cvar_SetValueByRef (&m_directinput, 1);
		return;
	}


//	Con_SafePrintf("Mouse: IN_DirectInput_f 1\n");
	// Called when m_directinput changes
	
	if (m_directinput.value && flex_dinput) { 
		// Let's not turn dinput on twice!
		return;
	}
		
	if (!m_directinput.value && !flex_dinput) {
		// Let's not reinit unnecessarily during initialization
		return;
	}

	IN_Restart();	

}


int			mouse_buttons;
int			mouse_oldbuttonstate;
POINT		current_pos;
double		mouse_x, mouse_y;
int		old_mouse_x, old_mouse_y, mx_accum, my_accum;

static int		originalmouseparms[3], newmouseparms[3] = {0, 0, 1};

static qboolean	mouseparmsvalid;

// Baker 3.85: Need to restructure so mouse can be restarted

qboolean IN_DirectInputON(void) {
	if (mouse_is_showing) {
		// Mouse is deactivated for the menu so
		// guess
		if (flex_dinput) 
			return true;
		else
			return false;
	}

	// Mouse is being used; use true state.
	if (flex_dinput_acquired)
		return true;
	else
		return false;
}

void IN_DirectInput_Status_f (void) {
	Con_SafePrintf("DirectInput Acquired = %s", IN_DirectInputON() ? "true" : "false");
}

static unsigned int		mstate_di;
unsigned int	uiWheelMessage;


// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball
#define	JOY_MAX_AXES		6				// X, Y, Z, R, U, V
#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1
#define JOY_AXIS_Z			2
#define JOY_AXIS_R			3
#define JOY_AXIS_U			4
#define JOY_AXIS_V			5

enum _ControlList {
	AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn, AxisFly
};

DWORD dwAxisFlags[JOY_MAX_AXES] = {
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

DWORD	dwAxisMap[JOY_MAX_AXES];
DWORD	dwControlMap[JOY_MAX_AXES];
PDWORD	pdwRawValue[JOY_MAX_AXES];

// none of these cvars are saved over a session.
// this means that advanced controller configuration needs to be executed each time.
// this avoids any problems with getting back to a default usage or when changing from one controller to another.
// this way at least something works.
cvar_t	in_joystick = {"joystick","0", true};
cvar_t	joy_name = {"joyname", "joystick"};
cvar_t	joy_advanced = {"joyadvanced", "0"};
cvar_t	joy_advaxisx = {"joyadvaxisx", "0"};
cvar_t	joy_advaxisy = {"joyadvaxisy", "0"};
cvar_t	joy_advaxisz = {"joyadvaxisz", "0"};
cvar_t	joy_advaxisr = {"joyadvaxisr", "0"};
cvar_t	joy_advaxisu = {"joyadvaxisu", "0"};
cvar_t	joy_advaxisv = {"joyadvaxisv", "0"};
cvar_t	joy_forwardthreshold = {"joyforwardthreshold", "0.15"};
cvar_t	joy_sidethreshold = {"joysidethreshold", "0.15"};
cvar_t	joy_flysensitivity = {"joyflysensitivity", "-1.0"};
cvar_t  joy_flythreshold = {"joyflythreshold", "0.15"};
cvar_t	joy_pitchthreshold = {"joypitchthreshold", "0.15"};
cvar_t	joy_yawthreshold = {"joyyawthreshold", "0.15"};
cvar_t	joy_forwardsensitivity = {"joyforwardsensitivity", "-1.0"};
cvar_t	joy_sidesensitivity = {"joysidesensitivity", "-1.0"};
cvar_t	joy_pitchsensitivity = {"joypitchsensitivity", "1.0"};
cvar_t	joy_yawsensitivity = {"joyyawsensitivity", "-1.0"};
cvar_t	joy_wwhack1 = {"joywwhack1", "0.0"};
cvar_t	joy_wwhack2 = {"joywwhack2", "0.0"};

qboolean	joy_avail, joy_advancedinit, joy_haspov;
DWORD		joy_oldbuttonstate, joy_oldpovstate;

int			joy_id;
DWORD		joy_flags;
DWORD		joy_numbuttons;


static JOYINFOEX	ji;

static HINSTANCE hInstDI;

typedef struct MYDATA {
	LONG  lX;                   // X axis goes here
	LONG  lY;                   // Y axis goes here
	LONG  lZ;                   // Z axis goes here
	BYTE  bButtonA;             // One button goes here
	BYTE  bButtonB;             // Another button goes here
	BYTE  bButtonC;             // Another button goes here
	BYTE  bButtonD;             // Another button goes here
	BYTE  bButtonE;             // Baker: DINPUT fix for 8 buttons?
	BYTE  bButtonF;             // Baker: DINPUT fix for 8 buttons?
	BYTE  bButtonG;             // Baker: DINPUT fix for 8 buttons?
	BYTE  bButtonH;             // Baker: DINPUT fix for 8 buttons?
} MYDATA;

static DIOBJECTDATAFORMAT rgodf[] = {
  { &GUID_XAxis,    FIELD_OFFSET(MYDATA, lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_YAxis,    FIELD_OFFSET(MYDATA, lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_ZAxis,    FIELD_OFFSET(MYDATA, lZ),       0x80000000 | DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonA), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonB), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonC), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonD), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0, 				FIELD_OFFSET(MYDATA, bButtonE), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0, 				FIELD_OFFSET(MYDATA, bButtonF), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0, 				FIELD_OFFSET(MYDATA, bButtonG), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0, 				FIELD_OFFSET(MYDATA, bButtonH), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
};

#define NUM_OBJECTS (sizeof(rgodf) / sizeof(rgodf[0]))

static DIDATAFORMAT	df = {
	sizeof(DIDATAFORMAT),       // this structure
	sizeof(DIOBJECTDATAFORMAT), // size of object data format
	DIDF_RELAXIS,               // absolute axis coordinates
	sizeof(MYDATA),             // device data size
	NUM_OBJECTS,                // number of objects
	rgodf,                      // and here they are
};

// forward-referenced functions
void IN_StartupJoystick (void);
void Joy_AdvancedUpdate_f (void);
void IN_JoyMove (usercmd_t *cmd);


cvar_t	m_forcewheel	= {"m_forcewheel", "0"};

cvar_t	m_rate		= {"m_rate",	"60", true};
cvar_t	m_showrate	= {"m_showrate", "0"};

qboolean	use_m_smooth;
HANDLE		m_event;

#define	 M_HIST_SIZE  64
#define	 M_HIST_MASK  (M_HIST_SIZE - 1)

typedef	struct msnap_s {
	long   data;	// data (relative axis pos)
	double time;	// timestamp
} msnap_t;

msnap_t	m_history_x[M_HIST_SIZE];	// history
msnap_t	m_history_y[M_HIST_SIZE];
int	m_history_x_wseq = 0;		// write sequence
int	m_history_y_wseq = 0;
int	m_history_x_rseq = 0;		// read	sequence
int	m_history_y_rseq = 0;
int	wheel_up_count = 0;
int	wheel_dn_count = 0;

#define INPUT_CASE_DIMOFS_BUTTON(NUM)			\
	case (DIMOFS_BUTTON0 + NUM):			\
		if (od.dwData &	0x80)			\
			mstate_di |= (1	<< NUM);	\
		else					\
			mstate_di &= ~(1 << NUM);	\
		break;

#define INPUT_CASE_DINPUT_MOUSE_BUTTONS			\
		INPUT_CASE_DIMOFS_BUTTON(0);		\
		INPUT_CASE_DIMOFS_BUTTON(1);		\
		INPUT_CASE_DIMOFS_BUTTON(2);		\
		INPUT_CASE_DIMOFS_BUTTON(3);	\
		INPUT_CASE_DIMOFS_BUTTON(4);	\
		INPUT_CASE_DIMOFS_BUTTON(5);	\
		INPUT_CASE_DIMOFS_BUTTON(6);	\
		INPUT_CASE_DIMOFS_BUTTON(7);		\

DWORD WINAPI IN_SMouseProc (void *lpParameter) {
	// read	mouse events and generate history tables
	DWORD	ret;
	while (1) {
		if ((ret = WaitForSingleObject(m_event,	INFINITE)) == WAIT_OBJECT_0) 		{
			int			mx = 0,	my = 0;
			DIDEVICEOBJECTDATA	od;
			HRESULT	 		hr;
			double 			time;

			if (!ActiveApp || Minimized || !flex_mouseactive || !flex_dinput_acquired) {
				Sleep (50);
				continue;
			}

			time = Sys_DoubleTime ();

			while (1) {
				DWORD	dwElements = 1;

				hr = IDirectInputDevice_GetDeviceData (g_pMouse, sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);

				if ((hr	== DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
//					Con_SafePrintf("Dinput acquire lost #1\n");
					flex_dinput_acquired	= false;
					break;
				}

				/* Unable to read data or no data available	*/
				if (FAILED(hr) || !dwElements)
					break;

				/* Look	at the element to see what happened	*/
				switch (od.dwOfs) {
					case DIMOFS_X:
						m_history_x[m_history_x_wseq & M_HIST_MASK].time = time;
						m_history_x[m_history_x_wseq & M_HIST_MASK].data = od.dwData;
						m_history_x_wseq++;
						break;

					case DIMOFS_Y:
						m_history_y[m_history_y_wseq & M_HIST_MASK].time = time;
						m_history_y[m_history_y_wseq & M_HIST_MASK].data = od.dwData;
						m_history_y_wseq++;
						break;

					case DIMOFS_Z:
						//if (m_forcewheel.value) {
						if (od.dwData &	0x80)
							wheel_dn_count++;
						else
							wheel_up_count++;
						//}
						break;

				INPUT_CASE_DINPUT_MOUSE_BUTTONS;


				}
			}
		}
	}
}

void IN_SMouseRead (int *mx, int *my) {
	static	acc_x, acc_y;
	int	x = 0, y = 0;
	double	t1, t2, maxtime, mintime;

	// acquire device
	IDirectInputDevice_Acquire (g_pMouse);
//	Con_SafePrintf("Dinput acquire gained #1\n");
	flex_dinput_acquired = true;

	// gather data from last read seq to now
	for ( ; m_history_x_rseq < m_history_x_wseq ; m_history_x_rseq++)
		x += m_history_x[m_history_x_rseq&M_HIST_MASK].data;
	for ( ; m_history_y_rseq < m_history_y_wseq ; m_history_y_rseq++)
		y += m_history_y[m_history_y_rseq&M_HIST_MASK].data;

	x -= acc_x;
	y -= acc_y;

	acc_x =	acc_y =	0;

	// show	rate if	requested
	if (m_showrate.value) {
		static	last_wseq_printed;

		if (m_history_x_wseq > last_wseq_printed) {
			double t = m_history_x[(m_history_x_rseq - 1) &	M_HIST_MASK].time -
					   m_history_x[(m_history_x_rseq - 2) &	M_HIST_MASK].time;

			if (t >	0.001)
				Con_Printf ("mouse rate: %3d\n", (int)(1 / t));

			last_wseq_printed = m_history_x_wseq;
		}
	}

	// smoothing goes here
	mintime	= maxtime = 1.0 / QMAX(m_rate.value, 10);
	maxtime	*= 1.2;
	mintime	*= 0.7;

	// X axis
	t1 = m_history_x[(m_history_x_rseq - 2)	& M_HIST_MASK].time;
	t2 = m_history_x[(m_history_x_rseq - 1)	& M_HIST_MASK].time;

	if (t2 - t1 > mintime && t2 - t1 < maxtime) {
		double vel = m_history_x[(m_history_x_rseq - 1)	& M_HIST_MASK].data / (t2 - t1);

		t1 = t2;
		t2 = Sys_DoubleTime ();

		if (t2 - t1 < maxtime)
			acc_x =	vel * (t2 - t1);
	}

	// Y axis
	t1 = m_history_y[(m_history_y_rseq - 2)	& M_HIST_MASK].time;
	t2 = m_history_y[(m_history_y_rseq - 1)	& M_HIST_MASK].time;

	if (t2 - t1 > mintime && t2 - t1 < maxtime) {
		double vel = m_history_y[(m_history_y_rseq-1) &	M_HIST_MASK].data / (t2 - t1);

		t1 = t2;
		t2 = Sys_DoubleTime ();

		if (t2 - t1 < maxtime)
			acc_y =	vel * (t2 - t1);
	}

	x += acc_x;
	y += acc_y;

	// return data
	*mx = x;
	*my = y;

	// serve wheel
	bound(0, wheel_dn_count, 10);
	bound(0, wheel_up_count, 10);

	while (wheel_dn_count >	0) {
		Key_Event (K_MWHEELDOWN, 0, true);
		Key_Event (K_MWHEELDOWN, 0, false);
		wheel_dn_count--;
	}
	while (wheel_up_count >	0) {
		Key_Event (K_MWHEELUP, 0, true);
		Key_Event (K_MWHEELUP, 0, false);
		wheel_up_count--;
	}
}

qboolean flex_firstmsinit = false;
void IN_SMouseInit (void) {
	HRESULT	res;
	DWORD	threadid;
	HANDLE	thread;

//	Con_SafePrintf("Mouse: IN_SMouseInit 1\n");
	use_m_smooth = false;
	if (!COM_CheckParm("-m_smooth"))
		return;

	// create event	object
	m_event	= CreateEvent(
		NULL,			// NULL secutity attributes
		FALSE,			// automatic reset
		FALSE,			// initial state = nonsignaled
				NULL);		  // NULL name
	if (m_event == NULL)
		return;

	// enable di notification
	if ((res = IDirectInputDevice_SetEventNotification(g_pMouse, m_event)) != DI_OK	&& res != DI_POLLEDDEVICE)
		return;

	// create thread
	thread = CreateThread (
		NULL,			// pointer to security attributes
		0,			// initial thread stack	size
		IN_SMouseProc,		// pointer to thread function
		NULL,			// argument for new thread
		CREATE_SUSPENDED,	// creation flags
		&threadid		// pointer to receive thread ID
	);
	if (!thread)
		return;

	SetThreadPriority (thread, THREAD_PRIORITY_HIGHEST);
	ResumeThread (thread);

	if (!flex_firstmsinit) {
		Cvar_RegisterVariable (&m_rate, NULL);
		Cvar_RegisterVariable (&m_showrate, NULL);
		flex_firstmsinit = true;
	}
	use_m_smooth = true;
}



void Force_CenterView_f (void) {
	cl.viewangles[PITCH] = 0;
}

void IN_UpdateClipCursor (void) {
// Baker: this is placing limits on the mousecursor movement
//        and locking it to the window, right?

// ProQuake 3.50 logic is ...
// (mouseinitialized && mouseactive && !dinput)

	// Baker: my guess is that DirectInput does not need clipcursor
	// my guess is that dinput by necessity has to be turned "off"
	// to move the mouse cursor around
	// By what process is the mouse made to freemove in 3.50 if dinput is on?
	if (flex_mouseinitialized && flex_mouseactive && !flex_dinput) // Baker 3.85
		ClipCursor (&window_rect);
}

void IN_ShowMouse (void) {
//Baker notes: Harmless, 100% 3.50 equivalent	
//	Con_SafePrintf("Mouse: IN_ShowMouse 1\n");
	if (!mouse_is_showing) {
		ShowCursor (TRUE);
		mouse_is_showing = true;
	}
}

void IN_HideMouse (void) {
//Baker notes: Harmless, 100% 3.50 equivalent
	//Con_SafePrintf("Mouse: IN_HideMouse 1\n");
	if (mouse_is_showing) {
		ShowCursor (FALSE);
		mouse_is_showing = false;
	}
}

void IN_ActivateMouse (void) {
// Baker notes:
// This causes Quake to "absorb the mouse input"
// i.e. if you hit this, there is no mouse cursor
// So hide mouse is this procedure's buddy
	mouse_lockedto_quakewindow = true;
//	Con_SafePrintf("Mouse: IN_ActivateMouse 1\n");
	if (flex_mouseinitialized) {
		if (flex_dinput) {
			//Con_SafePrintf("Mouse: IN_ActivateMouse 3\n");
			if (g_pMouse) {
			//	Con_SafePrintf("Mouse: IN_ActivateMouse 4\n");
				if (!flex_dinput_acquired) {
			//		Con_SafePrintf("Mouse: IN_ActivateMouse 5\n");
					IDirectInputDevice_Acquire(g_pMouse);
			//		Con_SafePrintf("Dinput acquire gained #1\n");
					flex_dinput_acquired = true;
				}
			} else {
				return;
			}
		} else {
			//Con_SafePrintf("Mouse: IN_ActivateMouse 6\n");
			if (mouseparmsvalid) {
				restore_spi = SystemParametersInfo (SPI_SETMOUSE, 0, newmouseparms, 0);
//				Con_Printf("Setting NEW mouse parameters ...\n");
			}
			SetCursorPos (window_center_x, window_center_y);
			SetCapture (mainwindow);
			ClipCursor (&window_rect);
		}

		flex_mouseactive = true;
	}
}

void IN_SetQuakeMouseState (void) {
//Baker notes: Harmless, 100% 3.50 equivalent
// 
//	Con_SafePrintf("Mouse: IN_SetQuakeMouseState 1\n");
	if (mouse_lockedto_quakewindow)
		IN_ActivateMouse ();
}

void IN_DeactivateMouse (void) {
//Baker notes: 100% 3.50 equivalent
//This code just frees the mouse to Windows
//	Con_SafePrintf("Mouse: IN_DeactivateMouse 1\n");
	mouse_lockedto_quakewindow = false;
//	Con_Printf("Is mouse init? %i\n", flex_mouseinitialized);
	if (flex_mouseinitialized) {
#if 1 // Baker: We have to do this for dinput now too.  We might have changed them.
		if (restore_spi) {
			SystemParametersInfo (SPI_SETMOUSE, 0, originalmouseparms, 0);
//			Con_Printf("Setting OLD mouse parameters ...\n");
		}
#endif

		
		if (flex_dinput) {
			if (g_pMouse) {
				if (flex_dinput_acquired) {
					IDirectInputDevice_Unacquire(g_pMouse);
//					Con_SafePrintf("Dinput acquire removed #1\n");
					flex_dinput_acquired = false;
				}
			}
		} else {
//			Con_SafePrintf("Mouse: IN_DeactivateMouse 6\n");
#if 0 // Baker: We have to do this for dinput now too.  We might have changed them.
			if (restore_spi) {
				SystemParametersInfo (SPI_SETMOUSE, 0, originalmouseparms, 0);
				Con_Printf("Setting OLD mouse parameters ...\n");
			}
#endif
			ClipCursor (NULL);
			ReleaseCapture ();
		}

		flex_mouseactive = false; // Baker: flex_quake_has_mouse_control
	}
}

void IN_RestoreOriginalMouseState (void) {
// Baker notes: 100% equivalent to PQ 3.50
//	Con_SafePrintf("Mouse: IN_RestoreOriginalMouseState 1\n");
	// Despite the weirdness of the following, the logic is correct
	if (mouse_lockedto_quakewindow) {
		IN_DeactivateMouse ();
		mouse_lockedto_quakewindow = true;
	}

	// try to redraw the cursor so it gets reinitialized, because sometimes it has garbage after the mode switch
	ShowCursor (TRUE);
	ShowCursor (FALSE);
}

qboolean IN_InitDInput (void) {
// Baker notes: 100% equivalent to PQ 3.50
// Check when this is called, however!
    HRESULT		hr;
	DIPROPDWORD	dipdw = {
		{
			sizeof(DIPROPDWORD),        // diph.dwSize
			sizeof(DIPROPHEADER),       // diph.dwHeaderSize
			0,                          // diph.dwObj
			DIPH_DEVICE,                // diph.dwHow
		},
		DINPUT_BUFFERSIZE,              // dwData
	};

//	Con_SafePrintf("Mouse: IN_InitDInput 1\n");
	if (!hInstDI) {
		hInstDI = LoadLibrary(TEXT("dinput.dll")); // Baker 3.70D3D - Direct3D Quake change?

		if (hInstDI == NULL) {
//			Con_SafePrintf ("Couldn't load dinput.dll\n");
			return false;
		}
	}

	if (!pDirectInputCreate) {
		pDirectInputCreate = (void *)GetProcAddress(hInstDI,"DirectInputCreateA");

		if (!pDirectInputCreate) {
//			Con_SafePrintf ("Couldn't get DI proc addr\n");
			return false;
		}
	}

// register with DirectInput and get an IDirectInput to play with.
	hr = iDirectInputCreate(global_hInstance, DIRECTINPUT_VERSION, &g_pdi, NULL);

	if (FAILED(hr))
		return false;

// obtain an interface to the system mouse device.
	hr = IDirectInput_CreateDevice(g_pdi, &GUID_SysMouse, &g_pMouse, NULL);

	if (FAILED(hr)) {
//		Con_SafePrintf ("Couldn't open DI mouse device\n");
		return false;
	}

// set the data format to "mouse format".
	hr = IDirectInputDevice_SetDataFormat(g_pMouse, &df);

	if (FAILED(hr)) {
//		Con_SafePrintf ("Couldn't set DI mouse format\n");
		return false;
	}

// set the cooperativity level.
	hr = IDirectInputDevice_SetCooperativeLevel(g_pMouse, mainwindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND);

	if (FAILED(hr)) {
//		Con_SafePrintf ("Couldn't set DI coop level\n");
		return false;
	}


// set the buffer size to DINPUT_BUFFERSIZE elements.
// the buffer size is a DWORD property associated with the device
	hr = IDirectInputDevice_SetProperty(g_pMouse, DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr)) {
//		Con_SafePrintf ("Couldn't set DI buffersize\n");
		return false;
	}

	IN_SMouseInit ();

	return true;
}

void IN_StartupMouse (void) {
//Baker notes: this is not pq 3.50 equivalent
//We want to decide if to use dinput based
// on not just the commandline but also
// the cvar
//	Con_SafePrintf("Mouse: IN_StartupMouse 1\n");
// This can be called before mouse initialization
// But should it be???

// Baker 3.99n: experiment

//	Con_Printf("MOUSEDEBUG: Startup ...\n");

	if ( COM_CheckParm ("-nomouse") )
		return;

	uiWheelMessage = RegisterWindowMessage (TEXT("MSWHEEL_ROLLMSG")); // Baker 3.85: Moved to IN_StartupMouse from IN_Init

	flex_mouseinitialized = true;

	if (m_directinput.value) {
		flex_dinput = IN_InitDInput ();

		if (flex_dinput) {
				Con_Success ("DirectInput initialized\n");
			if (use_m_smooth)
				Con_Success ("Mouse smoothing initialized\n");
		} else {
				Con_Warning ("DirectInput not initialized\n");
		}
	}

#if 1
		if (!mouseparmsvalid) {
			mouseparmsvalid = SystemParametersInfo (SPI_GETMOUSE, 0, originalmouseparms, 0);
//			Con_Printf("Getting mouse parameters ...\n");
		}
#endif

	if (!flex_dinput) {
#if 0		
		if (!mouseparmsvalid) {
			mouseparmsvalid = SystemParametersInfo (SPI_GETMOUSE, 0, originalmouseparms, 0);
//			Con_Printf("Getting mouse parameters ...\n");
		}
#endif
		if (mouseparmsvalid) {
			if ( COM_CheckParm ("-noforcemspd") )
				newmouseparms[2] = originalmouseparms[2];

			if (COM_CheckParm ("-noforcemaccel")) {
				newmouseparms[0] = originalmouseparms[0];
				newmouseparms[1] = originalmouseparms[1];
			}

			if (COM_CheckParm ("-noforcemparms")) {
				newmouseparms[0] = originalmouseparms[0];
				newmouseparms[1] = originalmouseparms[1];
				newmouseparms[2] = originalmouseparms[2];
			}
		}
	}
	mouse_buttons = 8;



// if a fullscreen video mode was set before the mouse was initialized, set the mouse state appropriately
	if (mouse_lockedto_quakewindow)
		IN_ActivateMouse ();

	// Baker 3.85: Moved to here.  It makes more sense
	//Con_Printf("Dinput is %d and fullwindow is %d = %d -> %d", dinput, modestate, MS_WINDOWED, modestate == MS_WINDOWED);
	if (flex_firstinit) {
		// Only call this after IN_Init
		if (flex_dinput && modestate != MS_WINDOWED) {
			// Baker 3.80x: this is an outstanding issue!
			//Cvar_SetDefault("m_forcewheel", 1);
			Cvar_SetValueByRef (&m_forcewheel, 1);
		} else {
			Cvar_SetValueByRef (&m_forcewheel, 0); // Baker 3.85: Added for mouserestart
		}
	}

}
void IN_SetGlobals(void) {
//	mouse_buttons = mouse_oldbuttonstate = 0;
//	flex_input_initialized = false;
//	mx_accum      = my_accum = 0;
//	restore_spi   = false;
//	memset(&current_pos,       0, sizeof(current_pos));
//	memset(originalmouseparms, 0, sizeof(originalmouseparms));
//	originalmouseparms[0] =0;
//	originalmouseparms[1] =0;
//	originalmouseparms[2] =1;
//	newmouseparms[0] =0;
//	newmouseparms[1] =0;
//	newmouseparms[2] =1;

//	mouseparmsvalid  = mouseactivatetoggle = false;

	mstate_di        = 0;
	uiWheelMessage   = 0;

	memset(m_history_x, 0, sizeof(m_history_x));	// history
	memset(m_history_y, 0, sizeof(m_history_y));
	m_history_x_wseq =	0;		// write sequence
	m_history_y_wseq =	0;
	m_history_x_rseq =	0;		// read	sequence
	m_history_y_rseq =	0;
	wheel_up_count	 =  0;
	wheel_dn_count	 =  0;

//	last_wseq_printed=  0;
	uiWheelMessage   = 0;

	flex_dinput_acquired = false;
	flex_mouseinitialized = false;
	flex_mouseactive = false;
	flex_dinput = false;
}

extern cvar_t in_keymap;
void KEY_Keymap_f(void);

void IN_Init (void) {
	
//	Con_SafePrintf("Mouse: IN_Init 1\n");
	// Baker 3.85:  This occurs BEFORE running quake.rc and if manually restarting mouse, such as
	//              changing m_directinput cvar, which occurs in quake.rc
	//              So this normally happens twice!

	if (!flex_firstinit) {
		
//		Con_SafePrintf("Very first init\n");
		// Baker 3.85: Very first input initialization

		if (COM_CheckParm("-dinput") || COM_CheckParm("-m_smooth"))
			commandline_dinput = true;
		else
			commandline_dinput = false;
			
		// mouse variables
		Cvar_RegisterVariable (&m_filter, NULL);
		Cvar_RegisterVariable (&m_forcewheel, NULL);
		Cvar_RegisterVariable (&m_accel, NULL);
		Cvar_RegisterVariable (&m_directinput, IN_DirectInput_f);
		//Con_SafePrintf("Did we re-init dinput below this point but ..\n");
		if (commandline_dinput) {
			m_dinput_skiponce=true;
			Cvar_SetValueByRef (&m_directinput, 1);
		}
//		Con_SafePrintf("... above this point\n");

		// keyboard variables
		Cvar_RegisterVariable (&cl_keypad, NULL);
		Cvar_RegisterVariable (&in_keymap, KEY_Keymap_f);

		// joystick variables 
		Cvar_RegisterVariable (&in_joystick, NULL); // Baker 3.83: Leaving here ONLY because this saves to config.


		Cmd_AddCommand ("force_centerview", Force_CenterView_f);
		Cmd_AddCommand ("dinput_status", IN_DirectInput_Status_f);
		
//		Con_SafePrintf("Input startup initialized\n");
		
	}

	IN_SetGlobals (); // Baker risk
	IN_StartupMouse ();
	IN_StartupJoystick ();

	
	flex_firstinit = true;	// Baker: the placement of this is in question

	
}

void IN_Shutdown (void) {
// Baker: IN_RESTART calls this but is it appropriate
//        to deactivate the mouse if that has already
//        been done?
	
//	Con_SafePrintf("Mouse: IN_Shutdown 1\n");
//	Con_Printf("Mouse shutdown ...\n");
	IN_DeactivateMouse ();
	IN_ShowMouse ();

	if (g_pMouse) {
		IDirectInputDevice_Release(g_pMouse);
		g_pMouse = NULL;
	}

	if (g_pdi) {
		IDirectInput_Release(g_pdi);
		g_pdi = NULL;
	}

//	if (hInstDI) {
//		FreeLibrary(hInstDI);
//		hInstDI = NULL;
		flex_dinput = false;
//		Con_SafePrintf("Dinput acquire lost by turn off #8\n");
//		if (flex_dinput_acquired != false) 
//			Con_SafePrintf("This should have turned off already!\n");
//		flex_dinput_acquired = false; // Baker 3.85 -- right?
//	}

	

}

void IN_MouseEvent (int mstate) {
	int	i;

	if (flex_mouseactive && !flex_dinput) {
	// perform button actions
		for (i=0 ; i<mouse_buttons ; i++) {
			if ((mstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)))
				Key_Event (K_MOUSE1 + i, 0, true);

			if (!(mstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)))
				Key_Event (K_MOUSE1 + i, 0, false);
			}

		mouse_oldbuttonstate = mstate;
	}
}

void IN_MouseMove (usercmd_t *cmd) {
	int			mx, my, i;
	float filterfrac;
	DIDEVICEOBJECTDATA	od;
	DWORD				dwElements;
	HRESULT				hr;

	if (!flex_mouseactive)
		return;

	if (flex_dinput) {
		mx = my = 0;


		if (use_m_smooth) {
			IN_SMouseRead (&mx, &my);
		} else {
			while (1) {
			dwElements = 1;

				hr = IDirectInputDevice_GetDeviceData (g_pMouse, sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);

				if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
//				Con_SafePrintf("Dinput acquired in mousemove #1\n");
				flex_dinput_acquired = true;
				IDirectInputDevice_Acquire(g_pMouse);
				break;
			}

			/* Unable to read data or no data available */
				if (FAILED(hr) || !dwElements)
				break;

			/* Look at the element to see what happened */
				//Con_Printf("%d %d %d\n",od.dwOfs, DIMOFS_BUTTON1, DIMOFS_BUTTON7);

				//Con_Printf("od.dwofs %d od.dwdata %d\n",od.dwOfs, od.dwData);
				//Con_Printf("Mousewheel DIMOFS_Z is %d %d od.dwdata %d\n",DIMOFS_Z);

				switch (od.dwOfs) {
				case DIMOFS_X:
					mx += od.dwData;
					break;

				case DIMOFS_Y:
					my += od.dwData;
					break;

				case DIMOFS_Z:
						//Con_Printf("Mousewheel event\n");
						if (flex_dinput) {
							if (od.dwData & 0x80) {
								Key_Event(K_MWHEELDOWN, 0, true);
								Key_Event(K_MWHEELDOWN, 0, false);
								//Con_Printf("Mousewheelkey up event\n");
							} else {
								Key_Event(K_MWHEELUP, 0, true);
								Key_Event(K_MWHEELUP, 0, false);
								//Con_Printf("Mousewheelkey down event\n");
							}
						}
						//Con_Printf("Exit in_mousewheel\n");
					break;

					INPUT_CASE_DINPUT_MOUSE_BUTTONS



				}
			}
		}

	// perform button actions
		for (i=0 ; i<mouse_buttons ; i++) {
			if ((mstate_di & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)))
				Key_Event (K_MOUSE1 + i, 0, true);

			if (!(mstate_di & (1<<i)) && (mouse_oldbuttonstate & (1<<i)))
				Key_Event (K_MOUSE1 + i, 0, false);
			}

		mouse_oldbuttonstate = mstate_di;
	} else {
		GetCursorPos (&current_pos);
		mx = current_pos.x - window_center_x + mx_accum;
		my = current_pos.y - window_center_y + my_accum;
		mx_accum = my_accum = 0;
	}

//if (mx ||  my)
//	Con_DPrintf("mx=%d, my=%d\n", mx, my);


#ifdef SUPPORTS_XFLIP
	if(gl_xflip.value) mx *= -1;   //Atomizer - GL_XFLIP
#endif

	if (m_filter.value) {
        filterfrac = bound(0, m_filter.value, 1) / 2.0;
        mouse_x = (mx * (1 - filterfrac) + old_mouse_x * filterfrac);
        mouse_y = (my * (1 - filterfrac) + old_mouse_y * filterfrac);
	} else {
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	if (m_accel.value) {
		float mousespeed = sqrt (mx * mx + my * my);
		mouse_x *= (mousespeed * m_accel.value + sensitivity.value);
		mouse_y *= (mousespeed * m_accel.value + sensitivity.value);
	} else {
		mouse_x *= sensitivity.value;
		mouse_y *= sensitivity.value;
	}

// add mouse X/Y movement to cmd
	if ( (in_strafe.state & 1) || (lookstrafe.value && mlook_active ))  // Baker 3.60 - Freelook cvar support
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;

	if (mlook_active)  // Baker 3.60 - Freelook cvar support
		V_StopPitchDrift ();

	if (mlook_active && !(in_strafe.state & 1)) {
		// Baker 3.60 - Freelook cvar support
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;

		// JPG 1.05 - added pq_fullpitch
		if (pq_fullpitch.value) {
			if (cl.viewangles[PITCH] > 90)
				cl.viewangles[PITCH] = 90;
			if (cl.viewangles[PITCH] < -90)
				cl.viewangles[PITCH] = -90;
		} else {
			if (cl.viewangles[PITCH] > 80)
				cl.viewangles[PITCH] = 80;
			if (cl.viewangles[PITCH] < -70)
				cl.viewangles[PITCH] = -70;
		}
	} else {
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}

// if the mouse has moved, force it to the center, so there's room to move
	if (mx || my)
		SetCursorPos (window_center_x, window_center_y);

}

void IN_Move (usercmd_t *cmd) {
	if (ActiveApp && !Minimized) {
		IN_MouseMove (cmd);
		IN_JoyMove (cmd);
	}
}

#ifdef WINDOWS_SCROLLWHEEL_PEEK
// Baker: this is ONLY used to capture mouse input for console scrolling
//        and ONLY if directinput is enabled
void IN_MouseWheel (void) {
	DIDEVICEOBJECTDATA	od;
	DWORD				dwElements;
	HRESULT				hr;

	if (!flex_mouseactive)	// No mouse ... pointless
		return;

	if (!flex_dinput)		// DirectInput needs this, otherwise we get input from vid_wgl in windows messages
		return;
		
	if (use_m_smooth)
		return;				// Smooth read

	
	while (1) 
	{
		dwElements = 1;
		hr = IDirectInputDevice_GetDeviceData (g_pMouse, sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);

		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) 
		{
			flex_dinput_acquired = true;
			IDirectInputDevice_Acquire(g_pMouse);
			break;
		}

		/* Unable to read data or no data available */
		if (FAILED(hr) || !dwElements)
			break;

		if (od.dwOfs == DIMOFS_Z)
		{
			if (od.dwData & 0x80) 
			{
				Key_Event(K_MWHEELDOWN, 0, true);
				Key_Event(K_MWHEELDOWN, 0, false);
			} 
			else
			{
				Key_Event(K_MWHEELUP, 0, true);
				Key_Event(K_MWHEELUP, 0, false);
			}
			break;
		}
	}
}
#endif


void IN_Accumulate (void) {
	if (flex_mouseactive) {
			GetCursorPos (&current_pos);

			mx_accum += current_pos.x - window_center_x;
			my_accum += current_pos.y - window_center_y;

		// force the mouse to the center, so there's room to move
			SetCursorPos (window_center_x, window_center_y);
		}
	}

void IN_ClearStates (void) {
	if (flex_mouseactive)
		mx_accum = my_accum = mouse_oldbuttonstate = 0;
}

void IN_StartupJoystick (void)  {
	int		numdevs;
	JOYCAPS		jc;
	MMRESULT	mmr;

 	// assume no joystick
	joy_avail = false;

	// abort startup if user requests no joystick
	if (!COM_CheckParm ("-joystick") ) // Baker 3.83: Must explicitly indicate joystick, instead of explicitly not
		return;

 	if (!flex_firstjoyinit) {
		// Baker 3.85: Only do this once!

		Cvar_RegisterVariable (&joy_name, NULL);
		Cvar_RegisterVariable (&joy_advanced, NULL);
		Cvar_RegisterVariable (&joy_advaxisx, NULL);
		Cvar_RegisterVariable (&joy_advaxisy, NULL);
		Cvar_RegisterVariable (&joy_advaxisz, NULL);
		Cvar_RegisterVariable (&joy_advaxisr, NULL);
		Cvar_RegisterVariable (&joy_advaxisu, NULL);
		Cvar_RegisterVariable (&joy_advaxisv, NULL);
		Cvar_RegisterVariable (&joy_forwardthreshold, NULL);
		Cvar_RegisterVariable (&joy_sidethreshold, NULL);
		Cvar_RegisterVariable (&joy_flythreshold, NULL);
		Cvar_RegisterVariable (&joy_pitchthreshold, NULL);
		Cvar_RegisterVariable (&joy_yawthreshold, NULL);
		Cvar_RegisterVariable (&joy_forwardsensitivity, NULL);
		Cvar_RegisterVariable (&joy_sidesensitivity, NULL);
		Cvar_RegisterVariable (&joy_flysensitivity, NULL);
		Cvar_RegisterVariable (&joy_pitchsensitivity, NULL);
		Cvar_RegisterVariable (&joy_yawsensitivity, NULL);
		Cvar_RegisterVariable (&joy_wwhack1, NULL);
		Cvar_RegisterVariable (&joy_wwhack2, NULL);
		Cmd_AddCommand ("joyadvancedupdate", Joy_AdvancedUpdate_f);
		flex_firstjoyinit = true;
	}

	// verify joystick driver is present
	if ((numdevs = joyGetNumDevs ()) == 0) {
		Con_Printf ("\njoystick not found -- driver not present\n\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	for (joy_id = 0 ; joy_id < numdevs ; joy_id++) {
		memset (&ji, 0, sizeof(ji));
		ji.dwSize = sizeof(ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		if ((mmr = joyGetPosEx (joy_id, &ji)) == JOYERR_NOERROR)
			break;
	}

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR) {
		Con_DPrintf ("joystick not found -- no valid joysticks (%x)\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	memset (&jc, 0, sizeof(jc));
	if ((mmr = joyGetDevCaps(joy_id, &jc, sizeof(jc))) != JOYERR_NOERROR) {
		Con_Printf ("joystick not found -- invalid joystick capabilities (%x)\n", mmr);
		return;
	}

	// save the joystick's number of buttons and POV status
	joy_numbuttons = jc.wNumButtons;
	joy_haspov = jc.wCaps & JOYCAPS_HASPOV;

	// old button and POV states default to no buttons pressed
	joy_oldbuttonstate = joy_oldpovstate = 0;

	// mark the joystick as available and advanced initialization not completed
	// this is needed as cvars are not available during initialization

	joy_avail = true;
	joy_advancedinit = false;

	Con_Printf ("\njoystick detected\n\n");
}

PDWORD RawValuePointer (int axis) {
	switch (axis) {
	case JOY_AXIS_X:
		return &ji.dwXpos;
	case JOY_AXIS_Y:
		return &ji.dwYpos;
	case JOY_AXIS_Z:
		return &ji.dwZpos;
	case JOY_AXIS_R:
		return &ji.dwRpos;
	case JOY_AXIS_U:
		return &ji.dwUpos;
	case JOY_AXIS_V:
		return &ji.dwVpos;
	}

	return NULL;	// shut up compiler
}

void Joy_AdvancedUpdate_f (void) {

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int	i;
	DWORD dwTemp;

	// initialize all the maps
	for (i=0 ; i<JOY_MAX_AXES ; i++) {
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer(i);
	}

	if (!joy_advanced.value) {
		// default joystick initialization
		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	} else {
		if (strcmp(joy_name.string, "joystick")) {
			// notify user of advanced controller
			Con_Printf ("\n%s configured\n\n", joy_name.string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (DWORD) joy_advaxisx.value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy.value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz.value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr.value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu.value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv.value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}

	// compute the axes to collect from DirectInput
	joy_flags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i = 0; i < JOY_MAX_AXES; i++) {
		if (dwAxisMap[i] != AxisNada)
			joy_flags |= dwAxisFlags[i];
		}
	}

void IN_Commands (void) {
	int		i, key_index;
	DWORD	buttonstate, povstate;

	if (!joy_avail)
		return;

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = ji.dwButtons;
	for (i=0 ; i<joy_numbuttons ; i++) {
		if ((buttonstate & (1<<i)) && !(joy_oldbuttonstate & (1<<i))) {
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i,  0, true);
		}

		if (!(buttonstate & (1<<i)) && (joy_oldbuttonstate & (1<<i))) {
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i,  0, false);
		}
	}
	joy_oldbuttonstate = buttonstate;

	if (joy_haspov) {
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		if(ji.dwPOV != JOY_POVCENTERED) {
			if (ji.dwPOV == JOY_POVFORWARD)
				povstate |= 0x01;
			if (ji.dwPOV == JOY_POVRIGHT)
				povstate |= 0x02;
			if (ji.dwPOV == JOY_POVBACKWARD)
				povstate |= 0x04;
			if (ji.dwPOV == JOY_POVLEFT)
				povstate |= 0x08;
		}
		// determine which bits have changed and key an auxillary event for each change
		for (i=0 ; i<4 ; i++) {
			if ( (povstate & (1<<i)) && !(joy_oldpovstate & (1<<i)) )
				Key_Event (K_AUX29 + i,  0, true);

			if ( !(povstate & (1<<i)) && (joy_oldpovstate & (1<<i)) )
				Key_Event (K_AUX29 + i,  0, false);

		}
		joy_oldpovstate = povstate;
	}
}

qboolean IN_ReadJoystick (void) {
	memset (&ji, 0, sizeof(ji));
	ji.dwSize = sizeof(ji);
	ji.dwFlags = joy_flags;

	if (joyGetPosEx(joy_id, &ji) == JOYERR_NOERROR) {
		// this is a hack -- there is a bug in the Logitech WingMan Warrior DirectInput Driver
		// rather than having 32768 be the zero point, they have the zero point at 32668
		// go figure -- anyway, now we get the full resolution out of the device
		if (joy_wwhack1.value != 0.0)
			ji.dwUpos += 100;

		return true;
	} else {
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,\
		// but what should be done?
		// Con_Printf ("IN_ReadJoystick: no response\n");
		// joy_avail = false;
		return false;
	}
}

void IN_JoyMove (usercmd_t *cmd) {
	float	speed, aspeed;
	float	fAxisValue, fTemp;
	int		i;

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if (joy_advancedinit != true) {
		Joy_AdvancedUpdate_f();
		joy_advancedinit = true;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || !in_joystick.value)
		return;

	// collect the joystick data, if possible
	if (IN_ReadJoystick () != true)
		return;

	speed = (in_speed.state & 1) ? cl_movespeedkey.value : 1;
	aspeed = speed * host_frametime;

	// loop through the axes
	for (i=0 ; i<JOY_MAX_AXES ; i++) {
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float) *pdwRawValue[i];
		// move centerpoint to zero
		fAxisValue -= 32768.0;

		if (joy_wwhack2.value != 0.0) {
			if (dwAxisMap[i] == AxisTurn) {
				// this is a special formula for the Logitech WingMan Warrior
				// y=ax^b; where a = 300 and b = 1.3
				// also x values are in increments of 800 (so this is factored out)
				// then bounds check result to level out excessively high spin rates
				fTemp = 300.0 * pow(abs(fAxisValue) / 800.0, 1.3);
				if (fTemp > 14000.0)
					fTemp = 14000.0;
				// restore direction information
				fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
			}
		}

		// convert range from -32768..32767 to -1..1
		fAxisValue /= 32768.0;

		switch (dwAxisMap[i]) {
		case AxisForward:
			if ((joy_advanced.value == 0.0) && mlook_active) {
				// user wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchthreshold.value) {
					// if mouse invert is on, invert the joystick pitch value
					// only absolute control support here (joy_advanced is false)
					if (m_pitch.value < 0.0) {
						cl.viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					} else {
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					}
					V_StopPitchDrift();
				} else {
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if(lookspring.value == 0.0)
						V_StopPitchDrift();
				}
			} else {
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold.value)
					cmd->forwardmove += (fAxisValue * joy_forwardsensitivity.value) * speed * cl_forwardspeed.value;
				}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold.value)
				cmd->sidemove += (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
			break;

		case AxisFly:
			if (fabs(fAxisValue) > joy_flythreshold.value)
				cmd->upmove += (fAxisValue * joy_flysensitivity.value) * speed * cl_upspeed.value;
			break;

		case AxisTurn:
			if ((in_strafe.state & 1) || (lookstrafe.value && mlook_active)) {
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold.value)
					cmd->sidemove -= (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
			} else {
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold.value) {
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * aspeed * cl_yawspeed.value;
					else
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * speed * 180.0;
					}
			}
			break;

		case AxisLook:
			if (mlook_active) {
				if (fabs(fAxisValue) > joy_pitchthreshold.value) {
					// pitch movement detected and pitch movement desired by user
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					else
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * speed * 180.0;
					V_StopPitchDrift();
				} else {
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if(lookspring.value == 0.0)
						V_StopPitchDrift();
				}
			}
			break;

		default:
			break;
		}
	}

	// bounds check pitch // JPG 1.05 - added pq_fullpitch
	if (pq_fullpitch.value) {
		if (cl.viewangles[PITCH] > 90.0)
			cl.viewangles[PITCH] = 90.0;
		if (cl.viewangles[PITCH] < -90.0)
			cl.viewangles[PITCH] = -90.0;
	} else {
		if (cl.viewangles[PITCH] > 80.0)
			cl.viewangles[PITCH] = 80.0;
		if (cl.viewangles[PITCH] < -70.0)
			cl.viewangles[PITCH] = -70.0;
	}
}

//==========================================================================

static byte        scantokey[128] = {
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,KP_STAR,
	K_ALT,' ',   K_CAPSLOCK  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3
	K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE  ,    K_SCRLCK  , K_HOME,
	K_UPARROW,K_PGUP,KP_MINUS,K_LEFTARROW,KP_5,K_RIGHTARROW,KP_PLUS,K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11,
	K_F12,0  ,    0  ,    K_LWIN  ,    K_RWIN  ,    K_MENU  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7
					};

static byte        shiftscantokey[128] =
					{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0  ,    27,     '!',    '@',    '#',    '$',    '%',    '^',
	'&',    '*',    '(',    ')',    '_',    '+',    K_BACKSPACE, 9, // 0
	'Q',    'W',    'E',    'R',    'T',    'Y',    'U',    'I',
	'O',    'P',    '{',    '}',    13 ,    K_CTRL,'A',  'S',      // 1
	'D',    'F',    'G',    'H',    'J',    'K',    'L',    ':',
	'"' ,    '~',    K_SHIFT,'|',  'Z',    'X',    'C',    'V',      // 2
	'B',    'N',    'M',    '<',    '>',    '?',    K_SHIFT,'*',
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3
	K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE  ,    0  , K_HOME,
	K_UPARROW,K_PGUP,'_',K_LEFTARROW,'%',K_RIGHTARROW,'+',K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11,
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7
					};

/*
===========
IN_MapKey -- Baker 3.72 - Attempt to address international keyboard issue

Map from windows to quake keynums
===========
*/
int IN_MapKey (int key, int ascii) {
	int result;
	int modified = (key >> 16) & 255;
	qboolean is_extended = false;

	if (modified < 128 && scantokey[modified])
		result = scantokey[modified];
	else {
		result = 0;
		//Con_DPrintf("key 0x%02x (0x%8x, 0x%8x) has no translation\n", modified, key, virtualkey);
		//Con_DPrintf("key 0x%02x (0x%8x) has no translation\n", modified, key);
	}


	if (key & (1 << 24))
		is_extended = true;

	if ( !is_extended ) {
		switch (result)  {
			case K_HOME:		return KP_HOME;
			case K_UPARROW:		return KP_UPARROW;
			case K_PGUP:		return KP_PGUP;
			case K_LEFTARROW:	return KP_LEFTARROW;
			case K_RIGHTARROW:	return KP_RIGHTARROW;
			case K_END:			return KP_END;
			case K_DOWNARROW:	return KP_DOWNARROW;
			case K_PGDN:		return KP_PGDN;
			case K_INS:			return KP_INS;
			case K_DEL:			return KP_DEL;
			default:			return result;
		}
	} else {
		// cl_keypad 0, compatibility mode
			if (cl_keypad.value)
				switch (result)
				{
					case K_ENTER:		return KP_ENTER;
					case '/':			return KP_SLASH;
					case K_PAUSE:		return KP_NUMLOCK;
					default:			return result;
				}
			else // cl_keypad 0, compatibility mode
				switch (result)
				{
					case KP_STAR:		return '*';
					case KP_MINUS:		return '-';
					case KP_5:			return '5';
					case KP_PLUS:		return '+';
					default:			return result;
				}
	}

	return result; // Baker 3.72 - no compiler warning
}

