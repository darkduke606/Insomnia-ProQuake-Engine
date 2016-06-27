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

#include <pspctrl.h>

extern "C"
{
#include "../quakedef.h"
}

namespace quake
{
	namespace input
	{
		// A map from button mask to Quake key.
		static const unsigned int	buttonCount	= sizeof(unsigned int) * 8;
		typedef int	ButtonToKeyMap[buttonCount];
		static ButtonToKeyMap		buttonToGameKeyMap;
		static ButtonToKeyMap		buttonToConsoleKeyMap;
		static ButtonToKeyMap		buttonToMessageKeyMap;
		static ButtonToKeyMap		buttonToMenuKeyMap;

		// The previous key state (for checking if things changed).
		static SceCtrlData		lastPad;
		static bool				readyToBindKeys	= false;

		static unsigned int buttonMaskToShift(unsigned int mask)
		{
			// Bad mask?
			if (!mask)
				return 0;

			// Shift right until we hit a 1.
			unsigned int shift = 0;
			while ((mask & 1) == 0)
			{
				mask >>= 1;
				++shift;
			}
			return shift;
		}

		static float inverseLerp(float a, float b, float v)
		{
			return (v - a) / (b - a);
		}

		static float applyDeadZone(float axis, float deadZone)
		{
			if (axis > deadZone)
			{
				return inverseLerp(deadZone, 1.0f, axis);
			}
			else if (axis < -deadZone)
			{
				return -inverseLerp(deadZone, 1.0f, -axis);
			}
			else
			{
				return 0.0f;
			}
		}
	}
}

// Quake globals.
extern "C" int bind_grab;

using namespace quake;
using namespace quake::input;

extern cvar_t cl_yawspeed;
extern cvar_t cl_pitchspeed;

extern cvar_t in_freelook_analog;
extern cvar_t in_analog_strafe;

extern cvar_t in_x_axis_adjust;
extern cvar_t in_y_axis_adjust;
extern cvar_t scr_fov;

void IN_Init (void)
{
	// Set up the controller.
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	// Japanese users would prefer to have X as cancel and O as OK.
	unsigned int	okButton		= PSP_CTRL_CROSS;
	unsigned int	cancelButton	= PSP_CTRL_CIRCLE;

	// Build the button to Quake key maps.
	// Common keys:
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_SELECT)]	= '~';
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_START)]	= K_ESCAPE;

	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_UP)]		= K_UPARROW;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_RIGHT)]	= K_RIGHTARROW;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_DOWN)]	= K_DOWNARROW;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_LEFT)]	= K_LEFTARROW;
	memcpy(buttonToConsoleKeyMap, buttonToGameKeyMap, sizeof(ButtonToKeyMap));
	memcpy(buttonToMessageKeyMap, buttonToGameKeyMap, sizeof(ButtonToKeyMap));
	memcpy(buttonToMenuKeyMap, buttonToGameKeyMap, sizeof(ButtonToKeyMap));

	// Game keys:
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_LTRIGGER)]	= K_AUX1;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_RTRIGGER)]	= K_AUX2;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_TRIANGLE)]	= K_JOY1;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_CIRCLE)]		= K_JOY2;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_CROSS)]		= K_JOY3;
	buttonToGameKeyMap[buttonMaskToShift(PSP_CTRL_SQUARE)]		= K_JOY4;

	// Console keys:
	buttonToConsoleKeyMap[buttonMaskToShift(PSP_CTRL_LTRIGGER)]	= K_PGUP;
	buttonToConsoleKeyMap[buttonMaskToShift(PSP_CTRL_RTRIGGER)]	= K_PGDN;
	buttonToConsoleKeyMap[buttonMaskToShift(okButton)]			= K_ENTER;
	buttonToConsoleKeyMap[buttonMaskToShift(cancelButton)]		= K_ESCAPE;
	buttonToConsoleKeyMap[buttonMaskToShift(PSP_CTRL_TRIANGLE)]	= K_DEL;
	buttonToConsoleKeyMap[buttonMaskToShift(PSP_CTRL_SQUARE)]	= K_INS;

	// Message keys:
	memcpy(buttonToMessageKeyMap, buttonToConsoleKeyMap, sizeof(ButtonToKeyMap));

	// Menu keys:
	buttonToMenuKeyMap[buttonMaskToShift(PSP_CTRL_SQUARE)]	= K_INS;
	buttonToMenuKeyMap[buttonMaskToShift(cancelButton)]		= K_ESCAPE;
	buttonToMenuKeyMap[buttonMaskToShift(okButton)]			= K_ENTER;
	buttonToMenuKeyMap[buttonMaskToShift(PSP_CTRL_TRIANGLE)]	= K_DEL;
	buttonToMenuKeyMap[buttonMaskToShift(PSP_CTRL_LTRIGGER)]	= K_AUX1;
	buttonToMenuKeyMap[buttonMaskToShift(PSP_CTRL_RTRIGGER)]	= K_AUX2;
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
	// Changed in or out of key binding mode?
	if ((bind_grab != 0) != readyToBindKeys)
	{
		// Was in key binding mode?
		if (readyToBindKeys)
		{
			// Just left key binding mode.
			// Release all keys which are pressed.
			for (unsigned int button = 0; button < buttonCount; ++button)
			{
				// Was the button pressed?
				if (lastPad.Buttons & (1 << button))
				{
					// Is the button in the map?
					const int key = buttonToGameKeyMap[button];
					if (key)
					{
						// Send a release event.
						Key_Event(key, qfalse);
					}
				}
			}

			// We're not ready to bind keys any more.
			readyToBindKeys = false;
		}
		else
		{
			// Entering key binding mode.
			// Release all keys which are pressed.
			for (unsigned int button = 0; button < buttonCount; ++button)
			{
				// Was the button pressed?
				if (lastPad.Buttons & (1 << button))
				{
					// Is the button in the map?
					const int key = buttonToMenuKeyMap[button];
					if (key)
					{
						// Send a release event.
						Key_Event(key, qfalse);
					}
				}
			}

			// We're now ready to bind keys.
			readyToBindKeys = true;
		}
	}

	// Use a different key mapping depending on where inputs are going to go.
	const ButtonToKeyMap* buttonToKeyMap = 0;
	switch (key_dest)
	{
	case key_game:
		buttonToKeyMap = &buttonToGameKeyMap;
		break;

	case key_console:
		buttonToKeyMap = &buttonToConsoleKeyMap;
		break;

	case key_message:
		buttonToKeyMap = &buttonToMessageKeyMap;
		break;

	case key_menu:
		// Binding keys?
		if (readyToBindKeys)
		{
			buttonToKeyMap = &buttonToGameKeyMap;
		}
		else
		{
			buttonToKeyMap = &buttonToMenuKeyMap;
		}
		break;

	default:
		Sys_Error("Unhandled key destination %d", key_dest);
		return;
	}

	// Read the pad state.
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(&pad, 1);

	// Find out which buttons have changed.
	SceCtrlData deltaPad;
	deltaPad.Buttons	= pad.Buttons ^ lastPad.Buttons;
	deltaPad.Lx			= pad.Lx - lastPad.Lx;
	deltaPad.Ly			= pad.Ly - lastPad.Ly;
	deltaPad.TimeStamp	= pad.TimeStamp	- lastPad.TimeStamp;

	// Handle buttons which have changed.
	for (unsigned int button = 0; button < buttonCount; ++button)
	{
		// Has the button changed?
		const unsigned int buttonMask = 1 << button;
		if (deltaPad.Buttons & buttonMask)
		{
			// Is the button in the map?
			const int key = (*buttonToKeyMap)[button];
			if (key)
			{
				// Send an event.
				const qboolean	state	= (pad.Buttons & buttonMask) ? qtrue : qfalse;
				Key_Event(key, state);
			}
		}
	}

	// Remember the pad state for next time.
	lastPad = pad;
}

float IN_CalcInput(int axis, float speed, float tolerance, float acceleration) {

	float value = ((float) axis / 128.0f) - 1.0f;

	if (value == 0.0f) {
		return 0.0f;
	}

	float abs_value = fabs(value);

	if (abs_value < tolerance) {
		return 0.0f;
	}

	abs_value -= tolerance;
	abs_value /= (1.0f - tolerance);
	abs_value = powf(abs_value, acceleration);
	abs_value *= speed;

	if (value < 0.0f) {
		value = -abs_value;
	} else {
		value = abs_value;
	}
	return value;
}

void IN_Move (usercmd_t *cmd)
{
	unsigned char analog_strafe = 0;

	int   x_adjust;
	int   y_adjust;
	float speed;

	// Don't let the pitch drift back to centre if analog nub look is on.
	if (in_freelook_analog.value)
	{
        Cbuf_AddText("+mlook\n");
		V_StopPitchDrift();
    }
	else
    {
        Cbuf_AddText("-mlook\n");
        if (in_analog_strafe.value || (in_strafe.state & 1))
			analog_strafe = 1;
	}

	if(kurok)
	{
		if(scr_fov.value <= 25)
		{
			x_adjust = in_x_axis_adjust.value / 4;
			y_adjust = in_y_axis_adjust.value / 4;
			speed = sensitivity.value / 4;
		}
		else if(scr_fov.value <= 50)
		{
			x_adjust = in_x_axis_adjust.value / 3;
			y_adjust = in_y_axis_adjust.value / 3;
			speed = sensitivity.value / 3;
		}
		else if(scr_fov.value <= 75)
		{
			x_adjust = in_x_axis_adjust.value / 2;
			y_adjust = in_y_axis_adjust.value / 2;
			speed = sensitivity.value / 2;
		}
		else
		{
			x_adjust = in_x_axis_adjust.value;
			y_adjust = in_y_axis_adjust.value;
   			speed = sensitivity.value;
		}
	}
	else
	{
		x_adjust = in_x_axis_adjust.value;
		y_adjust = in_y_axis_adjust.value;
		speed = sensitivity.value;
	}

	// Read the pad state.
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(&pad, 1);

	// Convert the inputs to floats in the range [-1, 1].
	// Implement the dead zone.
	float deadZone = in_tolerance.value;
	float acceleration = in_acceleration.value;

	if (speed <= 0)
		speed = 0;

    float x = IN_CalcInput(pad.Lx, (speed *0.1) + x_adjust, deadZone, acceleration);
	float y = IN_CalcInput(pad.Ly, (speed *0.1) + y_adjust, deadZone, acceleration);

	// Set the yaw.

	// Analog nub look?
	if (!analog_strafe) {
		const float yawScale = 30.0f;
		cl.viewangles[YAW] -= yawScale * x * host_frametime;

		if (in_freelook_analog.value)
		{
			// Set the pitch.
			const bool invertPitch = m_pitch.value < 0;
			const float pitchScale = yawScale * (invertPitch ? -1 : 1);
			cl.viewangles[PITCH] += pitchScale * y * host_frametime;

			// Don't look too far up or down.
			if (cl.viewangles[PITCH] > 90.0f)
				cl.viewangles[PITCH] = 90.0f;
			if (cl.viewangles[PITCH] < -90.0f)
				cl.viewangles[PITCH] = -90.0f;
		}
		else
		{
			// Move using up and down.
			cmd->forwardmove -= cl_forwardspeed.value * y;
		}
	} else {
		cmd->sidemove += cl_sidespeed.value * x;
		cmd->forwardmove -= cl_forwardspeed.value * y;
	}
}
