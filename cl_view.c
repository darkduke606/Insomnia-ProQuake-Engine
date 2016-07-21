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
// cl_view.c -- player eye positioning

#include "quakedef.h"
//#include "r_local.h"
#ifdef PSP_INPUT_CONTROLS
#include <pspctrl.h>
#endif
/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

cvar_t		lcd_x = {"lcd_x","0"};
cvar_t		lcd_yaw = {"lcd_yaw","0"};

cvar_t	scr_ofsx = {"scr_ofsx","0", false};
cvar_t	scr_ofsy = {"scr_ofsy","0", false};
cvar_t	scr_ofsz = {"scr_ofsz","0", false};

#ifdef SUPPORTS_AUTOID
cvar_t		scr_autoid		= {"scr_autoid", "0", true};
#endif

cvar_t	r_truegunangle = {"r_truegunangle","0", true};

cvar_t	cl_rollspeed = {"cl_rollspeed", "200",true};
cvar_t	v_gunkick = {"v_gunkick", "0", true};
cvar_t	cl_rollangle = {"cl_rollangle", "3.0"}; // Quake classic default = 2.0

cvar_t	cl_bob = {"cl_bob","0.02", true}; // Quake classic default = 0.02
cvar_t	cl_bobcycle = {"cl_bobcycle","0.6", true};  // Leave it
cvar_t	cl_bobup = {"cl_bobup","0.5", true}; // Quake classic default is 0.5

#ifdef SUPPORTS_KUROK
cvar_t	cl_bobside = {"cl_bobside", "0.02" /* "0.02" */, true};
cvar_t	cl_bobsidecycle = {"cl_bobsidecycle","0.6", true};
cvar_t	cl_bobsideup = {"cl_bobsideup","0.5", true};
#endif

cvar_t	v_kicktime = {"v_kicktime", "0.5", true};   //"0.5", true};  // Baker 3.80x - Save to config
cvar_t	v_kickroll = {"v_kickroll", "0.6", true}; //"0.6", true};  // Baker 3.80x - Save to config
cvar_t	v_kickpitch = {"v_kickpitch", "0.6", true}; //"0.6", true};  // Baker 3.80x - Save to config

cvar_t	v_iyaw_cycle = {"v_iyaw_cycle", "2", false};
cvar_t	v_iroll_cycle = {"v_iroll_cycle", "0.5", false};
cvar_t	v_ipitch_cycle = {"v_ipitch_cycle", "1", false};
cvar_t	v_iyaw_level = {"v_iyaw_level", "0.3", false};
cvar_t	v_iroll_level = {"v_iroll_level", "0.1", false};
cvar_t	v_ipitch_level = {"v_ipitch_level", "0.3", false};

cvar_t	v_idlescale = {"v_idlescale", "0", false};

cvar_t	crosshair = {"crosshair", "1", true};
cvar_t r_viewmodeloffset = {"r_viewmodeloffset", "0", true};
cvar_t	cl_crossx = {"cl_crossx", "0", true};
cvar_t	cl_crossy = {"cl_crossy", "0", true};
#ifdef SUPPORTS_KUROK
cvar_t  cl_gunpitch = {"cl_gunpitch", "0", false};
#endif
cvar_t	gl_cshiftpercent = {"gl_cshiftpercent", "100", false};

#ifdef PROQUAKE_EXTENSION
// JPG 1.05 - palette changes
cvar_t	pq_waterblend = {"pq_waterblend", "0", true};
cvar_t	pq_quadblend = {"pq_quadblend", "0.3", true};
cvar_t	pq_ringblend = {"pq_ringblend", "0", true};
cvar_t	pq_pentblend = {"pq_pentblend", "0.3", true};
cvar_t	pq_suitblend = {"pq_suitblend", "0.3", true};
#ifndef GLQUAKE
cvar_t	r_polyblend = {"r_polyblend", "1"};		// JPG 3.30 - winquake version of r_polyblend
#endif
#endif

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

extern	int			in_forward, in_forward2, in_back;
#ifdef SUPPORTS_KUROK
extern	cvar_t		lookcenter;
#endif

/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/
vec3_t	forward, right, up;

float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign, side;
#ifdef SUPPORTS_KUROK
	float	value;
#endif
	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabsf(side);

#ifdef SUPPORTS_KUROK
	if (kurok)
	{
		value = cl_rollangle.value;
		if (cl.inwater)
			value *= 2;
	
		if (side < cl_rollspeed.value)
			side = side * value / cl_rollspeed.value;
		else
			side = value;
	}
	else
#endif
	{
		side = (side < cl_rollspeed.value) ? side * cl_rollangle.value / cl_rollspeed.value : cl_rollangle.value;
	}

	return side*sign;

}


/*
===============
V_CalcBob
===============
*/
static float V_CalcBob (void)
{
	float	bob;
	float	cycle;

	cycle = cl.time - (int)(cl.time/cl_bobcycle.value)*cl_bobcycle.value;
	cycle /= cl_bobcycle.value;
	if (cycle < cl_bobup.value)
		cycle = M_PI * cycle / cl_bobup.value;
	else
		cycle = M_PI + M_PI*(cycle-cl_bobup.value)/(1.0 - cl_bobup.value);

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)
	bob = sqrtf(cl.velocity[0]*cl.velocity[0] + cl.velocity[1]*cl.velocity[1]) * cl_bob.value;
//    Con_Printf ("speed: %5.1f\n", Length(cl.velocity));
	bob = bob*0.3 + bob*0.7*sinf(cycle);
	if (bob > 4)
		bob = 4;
	else if (bob < -7)
		bob = -7;
	return bob;
}

#ifdef SUPPORTS_KUROK
/*
===============
V_CalcBobSide

===============
*/
float V_CalcBobSide (void)
{
	float	bobside;
	float	cycle;

	cycle = cl.time - (int)(cl.time/cl_bobsidecycle.value)*cl_bobsidecycle.value;
	cycle /= cl_bobsidecycle.value;
	if (cycle < cl_bobsideup.value)
		cycle = M_PI * cycle / cl_bobsideup.value;
	else
		cycle = M_PI + M_PI*(cycle-cl_bobsideup.value)/(1.0 - cl_bobsideup.value);

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bobside = sqrtf(cl.velocity[0]*cl.velocity[0] + cl.velocity[1]*cl.velocity[1]) * cl_bobside.value;
//Con_Printf ("speed: %5.1f\n", Length(cl.velocity));
	bobside = bobside*0.3 + bobside*0.7*sinf(cycle);
	if (bobside > 4)
		bobside = 4;
	else if (bobside < -7)
		bobside = -7;
	return bobside;

}

/*
===============
V_ZoomIN

Kurok specific zoom code.
===============
*/
/*
float V_ZoomIN (void)
{

}
*/
#endif

//=============================================================================


cvar_t	v_centermove = {"v_centermove", "1", true};
cvar_t	v_centerspeed = {"v_centerspeed","100", true};


void V_StartPitchDrift_f (void)
{
	if (cl.laststop == cl.time)
		return;		// something else is keeping it from drifting

	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed.value;
		cl.nodrift = false;
		cl.driftmove = 0;
	}
}

void V_StopPitchDrift (void)
{
	cl.laststop = cl.time;
	cl.nodrift = true;
	cl.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when
===============
*/
static void V_DriftPitch (void)
{
	float		delta, move;

	if (noclip_anglehack || !cl.onground || cls.demoplayback || lookcenter.value)
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( fabsf(cl.cmd.forwardmove) < cl_forwardspeed.value)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;

		if ( cl.driftmove > v_centermove.value)
			V_StartPitchDrift_f ();

		return;
	}

	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = host_frametime * cl.pitchvel;
	cl.pitchvel += host_frametime * v_centerspeed.value;

//Con_Printf ("move: %f (%f)\n", move, host_frametime);

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
			move = -delta;
		}
		cl.viewangles[PITCH] -= move;
	}
}


/*
==============================================================================

						PALETTE FLASHES

==============================================================================
*/

cshift_t	cshift_empty = { {130,80,50}, 0 };
cshift_t	cshift_water = { {130,80,50}, 128 };
#ifdef SUPPORTS_KUROK
cshift_t	cshift_kwater = { {64,64,128}, 128 }; // Blue water for Kurok
#endif
cshift_t	cshift_slime = { {0,25,5}, 150 };
cshift_t	cshift_lava = { {255,80,0}, 150 };

cvar_t		vold_gamma = {"gamma", "1", true};

byte		gammatable[256];	// palette is sent through this

#if defined(GLQUAKE) || defined(PSP_HARDWARE_VIDEO)
byte		rampsold[3][256];
float		v_blend[4];		// rgba 0.0 - 1.0
#endif	// GLQUAKE

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
		gammatable[i] =bound(0, inf, 255);
	}
}

/*
=================
V_CheckGamma
=================
*/
#ifdef D3D_FEATURE
void d3dSetGammaRamp(const unsigned char* gammaTable);
#endif // d3dfeature

static qboolean V_CheckGamma (void)
{
	static float oldgammavalue;

	if (vold_gamma.value == oldgammavalue)
		return false;
	oldgammavalue = vold_gamma.value;

	BuildGammaTable (vold_gamma.value);
	vid.recalc_refdef = 1;				// force a surface cache flush

#ifdef D3D_FEATURE
	d3dSetGammaRamp(gammatable);
#endif
	return true;
}



/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage (void)
{
	int		i, armor, blood;
	vec3_t	from, forward, right, up;
	entity_t	*ent;
	float	side, count;

	armor = MSG_ReadByte ();
	blood = MSG_ReadByte ();
	for (i=0 ; i<3 ; i++)
		from[i] = MSG_ReadCoord ();

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 3*count;
	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)
	{
#ifdef SUPPORTS_KUROK
	    if (kurok) // Flash blue for damage when wearing armor
	    {
            cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 100;   //r
            cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;   //g
            cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 200;   //b
	    }
	    else
#endif
	    {
            cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;   //r
            cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;   //g
            cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;   //b
	    }
	}
	else if (armor)
	{
#ifdef SUPPORTS_KUROK
	    if (kurok) // Flash blue for damage when wearing armor
	    {
            cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 0;     //r
            cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;     //g
            cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 255;   //b
	    }
	    else
#endif
	    {
            cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;   //r
            cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;    //g
            cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;    //b
	    }
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;   //r
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;     //g
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;     //b
	}

// calculate view angle kicks
	ent = &cl_entities[cl.viewentity];

	VectorSubtract (from, ent->origin, from);
	VectorNormalize (from);

	AngleVectors (ent->angles, forward, right, up);

	side = DotProduct (from, right);
	v_dmg_roll = count*side*v_kickroll.value;

	side = DotProduct (from, forward);
	v_dmg_pitch = count*side*v_kickpitch.value;

	v_dmg_time = v_kicktime.value;
}

/*
==================
V_cshift_f
==================
*/
static void V_cshift_f (void)
{
	cshift_empty.destcolor[0] = atoi(Cmd_Argv(1));
	cshift_empty.destcolor[1] = atoi(Cmd_Argv(2));
	cshift_empty.destcolor[2] = atoi(Cmd_Argv(3));
	cshift_empty.percent = atoi(Cmd_Argv(4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
static void V_BonusFlash_f (void)
{
#ifdef SUPPORTS_KUROK
	    if (kurok) // Flash blue for damage when wearing armor
	    {	
			cl.cshifts[CSHIFT_BONUS].destcolor[0] = 128;
			cl.cshifts[CSHIFT_BONUS].destcolor[1] = 128;
			cl.cshifts[CSHIFT_BONUS].destcolor[2] = 128;
		}
		else
#endif
		{
			cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
			cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
			cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
		}
	cl.cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor (int contents)
{
	switch (contents)
	{
	case CONTENTS_EMPTY:
	case CONTENTS_SOLID:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case CONTENTS_SLIME:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
#ifdef SUPPORTS_KUROK
	if (kurok)
		cl.cshifts[CSHIFT_CONTENTS] = cshift_kwater;
    else
#endif
        cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
static void V_CalcPowerupCshift (void)
{
	if (cl.items & IT_QUAD)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else if (cl.items & IT_SUIT)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 20;
	}
	else if (cl.items & IT_INVISIBILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
#if 0 // Yes ring blend should be lower by default
		cl.cshifts[CSHIFT_POWERUP].percent = 50;
#else // ... still this is what Quake uses ...
		cl.cshifts[CSHIFT_POWERUP].percent = 100;
#endif
	}
	else if (cl.items & IT_INVULNERABILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
}

/*
=============
V_CalcBlend
=============
*/
#if defined(GLQUAKE) || defined(PSP_HARDWARE_VIDEO)
void V_CalcBlend (void)
{
	float	r, g, b, a, a2;
	int		j;

	r = g = b = a = 0;

	// Baker hwgamma support
#ifdef SUPPORTS_ENHANCED_GAMMA
	if (using_hwgamma) {
		if (cls.state != ca_connected)
		{
			cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
			cl.cshifts[CSHIFT_POWERUP].percent = 0;
		}
		else
		{
			V_CalcPowerupCshift ();
		}

		// drop the damage value
		cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime * 150;
		if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
			cl.cshifts[CSHIFT_DAMAGE].percent = 0;

		// drop the bonus value
		cl.cshifts[CSHIFT_BONUS].percent -= host_frametime * 100;
		if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
			cl.cshifts[CSHIFT_BONUS].percent = 0;

		for (j=0 ; j<NUM_CSHIFTS ; j++)
		{
			if ((!gl_cshiftpercent.value || !gl_polyblend.value) && j != CSHIFT_CONTENTS)
				continue;

			if (j == CSHIFT_CONTENTS)
				a2 = cl.cshifts[j].percent / 100.0 / 255.0;
			else
			a2 = ((cl.cshifts[j].percent * gl_cshiftpercent.value) / 100.0) / 255.0;

			if (!a2)
				continue;
			a = a + a2*(1-a);

			a2 /= a;
			r = r*(1-a2) + cl.cshifts[j].destcolor[0]*a2;
			g = g*(1-a2) + cl.cshifts[j].destcolor[1]*a2;
			b = b*(1-a2) + cl.cshifts[j].destcolor[2]*a2;
		}

		v_blend[0] = r/255.0;
		v_blend[1] = g/255.0;
		v_blend[2] = b/255.0;
		v_blend[3] = bound(0, a, 1);
	}
	else
#endif
	{  // Baker end hwgamma support
		for (j=0 ; j<NUM_CSHIFTS ; j++) {
			if (!gl_cshiftpercent.value)
				continue;

			a2 = ((cl.cshifts[j].percent * gl_cshiftpercent.value) / 100.0) / 255.0;

	//		a2 = cl.cshifts[j].percent/255.0;
			if (!a2)
				continue;
			a = a + a2*(1-a);
	//Con_Printf ("j:%i a:%f\n", j, a);
			a2 = a2/a;
			r = r*(1-a2) + cl.cshifts[j].destcolor[0]*a2;
			g = g*(1-a2) + cl.cshifts[j].destcolor[1]*a2;
			b = b*(1-a2) + cl.cshifts[j].destcolor[2]*a2;
		}

		v_blend[0] = r/255.0;
		v_blend[1] = g/255.0;
		v_blend[2] = b/255.0;
		v_blend[3] = a;

		if (v_blend[3] > 1)
			v_blend[3] = 1;
		if (v_blend[3] < 0)
			v_blend[3] = 0;
	}
    Draw_FadeScreenColor (v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
}
#endif

/*
=============
V_UpdatePaletteNew
=============
*/
#if defined(GLQUAKE) || defined(PSP_HARDWARE_VIDEO)
void V_UpdatePaletteNew (void)
{
	int		i, j;
	qboolean	blend_changed;
//	byte	*basepal, *newpal;
//	byte	pal[768];
//	float	r,g,b,a;
//	int		ir, ig, ib;
//	qboolean force;

	V_CalcPowerupCshift ();

	blend_changed = false;

	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (cl.cshifts[i].percent != cl.prev_cshifts[i].percent)
		{
			blend_changed = true;
			cl.prev_cshifts[i].percent = cl.cshifts[i].percent;
		}
		for (j=0 ; j<3 ; j++)
			if (cl.cshifts[i].destcolor[j] != cl.prev_cshifts[i].destcolor[j])
			{
				blend_changed = true;
				cl.prev_cshifts[i].destcolor[j] = cl.cshifts[i].destcolor[j];
			}
	}

// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime*150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= host_frametime*100;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;

//	force = V_CheckGamma ();
//	if (!blend_changed && !force)
//		return;

	V_CalcBlend ();
/*

	a = v_blend[3];
	r = 255*v_blend[0]*a;
	g = 255*v_blend[1]*a;
	b = 255*v_blend[2]*a;

	a = 1-a;
	for (i=0 ; i<256 ; i++)
	{
		ir = i*a + r;
		ig = i*a + g;
		ib = i*a + b;
		if (ir > 255)
			ir = 255;
		if (ig > 255)
			ig = 255;
		if (ib > 255)
			ib = 255;

		rampsold[0][i] = gammatable[ir];
		rampsold[1][i] = gammatable[ig];
		rampsold[2][i] = gammatable[ib];
	}

	basepal = host_basepal;
	newpal = pal;

	for (i=0 ; i<256 ; i++)
	{
		ir = basepal[0];
		ig = basepal[1];
		ib = basepal[2];
		basepal += 3;

		newpal[0] = rampsold[0][ir];
		newpal[1] = rampsold[1][ig];
		newpal[2] = rampsold[2][ib];
		newpal += 3;
	}

	VID_ShiftPalette (pal);
	*/
}
#else	// !GLQUAKE
void V_UpdatePaletteOld (void)
{
	int		i, j;
	qboolean	blend_changed;
	byte	*basepal, *newpal;
	byte	pal[768];
	int		r,g,b;
	qboolean force;

	V_CalcPowerupCshift ();

	blend_changed = false;

	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (cl.cshifts[i].percent != cl.prev_cshifts[i].percent)
		{
			blend_changed = true;
			cl.prev_cshifts[i].percent = cl.cshifts[i].percent;
		}
		for (j=0 ; j<3 ; j++)
			if (cl.cshifts[i].destcolor[j] != cl.prev_cshifts[i].destcolor[j])
			{
				blend_changed = true;
				cl.prev_cshifts[i].destcolor[j] = cl.cshifts[i].destcolor[j];
			}
	}

// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime*150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= host_frametime*100;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;

	force = V_CheckGamma ();
	if (!blend_changed && !force)
		return;

	basepal = host_basepal;
	newpal = pal;

	for (i=0 ; i<256 ; i++)
	{
		r = basepal[0];
		g = basepal[1];
		b = basepal[2];
		basepal += 3;

		for (j=0 ; j<NUM_CSHIFTS ; j++)
		{
			r += (cl.cshifts[j].percent*(cl.cshifts[j].destcolor[0]-r))>>8;
			g += (cl.cshifts[j].percent*(cl.cshifts[j].destcolor[1]-g))>>8;
			b += (cl.cshifts[j].percent*(cl.cshifts[j].destcolor[2]-b))>>8;
		}

		newpal[0] = gammatable[r];
		newpal[1] = gammatable[g];
		newpal[2] = gammatable[b];
		newpal += 3;
	}

	VID_ShiftPalette (pal);
}
#endif	// !GLQUAKE


/*
==============================================================================

						VIEW RENDERING

==============================================================================
*/

float angledelta (float a)
{
	a = anglemod(a);
	if (a > 180)
		a -= 360;
	return a;
}

/*
==================
CalcGunAngle
==================
*/
void CalcGunAngle (void)
{
	float	yaw, pitch, move;
	static float oldyaw = 0;
	static float oldpitch = 0;
#ifdef PSP_ANALOG_STICK
    extern cvar_t	in_freelook_analog;
    extern cvar_t	in_disable_analog;
    extern cvar_t	in_analog_strafe;

    extern cvar_t   in_x_axis_adjust;
    extern cvar_t   in_y_axis_adjust;
#endif
	yaw = r_refdef.viewangles[YAW];
	pitch = -r_refdef.viewangles[PITCH];

	yaw = angledelta(yaw - r_refdef.viewangles[YAW]) * 0.4;
	if (yaw > 10)
		yaw = 10;
	if (yaw < -10)
		yaw = -10;
	pitch = angledelta(-pitch - r_refdef.viewangles[PITCH]) * 0.4;
	if (pitch > 10)
		pitch = 10;
	if (pitch < -10)
		pitch = -10;
	move = host_frametime*20;
	if (yaw > oldyaw)
	{
		if (oldyaw + move < yaw)
			yaw = oldyaw + move;
	}
	else
	{
		if (oldyaw - move > yaw)
			yaw = oldyaw - move;
	}

	if (pitch > oldpitch)
	{
		if (oldpitch + move < pitch)
			pitch = oldpitch + move;
	}
	else
	{
		if (oldpitch - move > pitch)
			pitch = oldpitch - move;
	}

	oldyaw = yaw;
	oldpitch = pitch;

	cl.viewent.angles[YAW] = r_refdef.viewangles[YAW] + yaw;
	cl.viewent.angles[PITCH] = - (r_refdef.viewangles[PITCH] + pitch);
#ifdef PSP_ANALOG_STICK
	// Read the pad state.
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(&pad, 1);
#endif

	extern cvar_t scr_fov;

    if (kurok && scr_fov.value == 90)
    {
        cl.viewent.angles[ROLL] -= sin(cl.time*3);
        if (!in_disable_analog.value)
        {
            if (in_freelook_analog.value)
            {
                if (m_pitch.value < 0)
                    cl.viewent.angles[PITCH] -= sin(cl.time*1.8) - ((pad.Ly - 128) * in_y_axis_adjust.value * 0.015);
                else
                    cl.viewent.angles[PITCH] -= sin(cl.time*1.8) + ((pad.Ly - 128) * in_y_axis_adjust.value * 0.015);
                cl.viewent.angles[YAW] -= sin(cl.time*1.5) + ((pad.Lx - 128) * in_x_axis_adjust.value * 0.015);
            }
            else
            {
                if (!in_analog_strafe.value)
                    cl.viewent.angles[YAW] -= sin(cl.time*1.5) + ((pad.Lx - 128) * in_x_axis_adjust.value * 0.015);
            }
        }

        if (cl_gunpitch.value)
            cl.viewent.angles[PITCH] -= cl_gunpitch.value;

    }
    else
    {
	   cl.viewent.angles[ROLL] -= v_idlescale.value * sinf(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	   cl.viewent.angles[PITCH] -= v_idlescale.value * sinf(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	   cl.viewent.angles[YAW] -= v_idlescale.value * sinf(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;
    }

}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets (void)
{
	entity_t	*ent;

	ent = &cl_entities[cl.viewentity];

// absolutely bound refresh relative to entity clipping hull
// so the view can never be inside a solid wall
	r_refdef.vieworg[0] = QMAX(r_refdef.vieworg[0], ent->origin[0] - 14);
	r_refdef.vieworg[0] = QMIN(r_refdef.vieworg[0], ent->origin[0] + 14);
	r_refdef.vieworg[1] = QMAX(r_refdef.vieworg[1], ent->origin[1] - 14);
	r_refdef.vieworg[1] = QMIN(r_refdef.vieworg[1], ent->origin[1] + 14);
	r_refdef.vieworg[2] = QMAX(r_refdef.vieworg[2], ent->origin[2] - 22);
	r_refdef.vieworg[2] = QMIN(r_refdef.vieworg[2], ent->origin[2] + 30);
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle (void)
{
  	    r_refdef.viewangles[ROLL] += v_idlescale.value * sinf(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	    r_refdef.viewangles[PITCH] += v_idlescale.value * sinf(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	    r_refdef.viewangles[YAW] += v_idlescale.value * sinf(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll (void)
{
	float		side;

	side = V_CalcRoll (cl_entities[cl.viewentity].angles, cl.velocity);
	r_refdef.viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		r_refdef.viewangles[ROLL] += v_dmg_time/v_kicktime.value*v_dmg_roll;
		r_refdef.viewangles[PITCH] += v_dmg_time/v_kicktime.value*v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.stats[STAT_HEALTH] <= 0)
	{
		r_refdef.viewangles[ROLL] = 80;	// dead view angle
		return;
	}

}


/*
==================
V_CalcIntermissionRefdef
==================
*/
void V_CalcIntermissionRefdef (void)
{
	entity_t	*ent, *view;
	float		old;

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

	VectorCopy (ent->origin, r_refdef.vieworg);
	VectorCopy (ent->angles, r_refdef.viewangles);
	view->model = NULL;

// always idle in intermission
#ifdef SUPPORTS_KUROK
    if (!kurok)
#endif
    {
	    old = v_idlescale.value;
	    v_idlescale.value = 1;
	    V_AddIdle ();
	    v_idlescale.value = old;
    }
}

/*
==================
V_CalcRefdef
==================
*/
void V_CalcRefdef (void)
{
	entity_t	*ent, *view;
	int			i;
	vec3_t		forward, right, up;
	vec3_t		angles;
	float		bob;
#ifdef SUPPORTS_KUROK
	float		bobside;
#endif
	static float oldz = 0;

	V_DriftPitch ();

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

// transform the view offset by the model's matrix to get the offset from
// model origin for the view
	ent->angles[YAW] = cl.viewangles[YAW];	// the model should face
										// the view dir
	ent->angles[PITCH] = -cl.viewangles[PITCH];	// the model should face
										// the view dir
	if (chase_active.value)
	{
        bob = 0;
        bobside = 0;
	}
	else
	{
        bob = V_CalcBob ();
        bobside = V_CalcBobSide ();
	}

	// set up the refresh position
	VectorCopy (ent->origin, r_refdef.vieworg);
	r_refdef.vieworg[2] += cl.viewheight + bob;

// never let it sit exactly on a node line, because a water plane can
// disappear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	r_refdef.vieworg[0] += 1.0/32;
	r_refdef.vieworg[1] += 1.0/32;
	r_refdef.vieworg[2] += 1.0/32;

	VectorCopy (cl.viewangles, r_refdef.viewangles);

	if (!chase_active.value)
	{
        V_CalcViewRoll ();
        V_AddIdle ();
	}

// offsets
	angles[PITCH] = -ent->angles[PITCH];	// because entity pitches are
											//  actually backward
	angles[YAW] = ent->angles[YAW];
	angles[ROLL] = ent->angles[ROLL];

	AngleVectors (angles, forward, right, up);

	for (i=0 ; i<3 ; i++)
		r_refdef.vieworg[i] += scr_ofsx.value*forward[i] + scr_ofsy.value*right[i] + scr_ofsz.value*up[i];


	V_BoundOffsets ();

// set up gun position
	VectorCopy (cl.viewangles, view->angles);

	CalcGunAngle ();

	VectorCopy (ent->origin, view->origin);
	view->origin[2] += cl.viewheight;

	for (i=0 ; i<3 ; i++)
	{
//		view->origin[i] += forward[i]*bob*0.2;
		view->origin[i] += right[i]*bobside*0.2;
		view->origin[i] += up[i]*bob*0.2;
	}
    view->origin[2] += bob;

#ifdef SUPPORTS_KUROK
    if(!kurok)
#endif
    {
	
#if 0
		VectorCopy (r_refdef.vieworg, view->origin);
		VectorMA (view->origin, bob * 0.4, forward, view->origin);
	
		if (r_viewmodeloffset.string[0]) 
		{
			float offset[3];
			int size = sizeof(offset)/sizeof(offset[0]);
	
			ParseFloats(r_viewmodeloffset.string, offset, &size);
			VectorMA (view->origin,  offset[0], right,   view->origin);
			VectorMA (view->origin, -offset[1], up,      view->origin);
			VectorMA (view->origin,  offset[2], forward, view->origin);
		}
#endif
	
	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
		if (!r_truegunangle.value) // Baker 3.80x - True gun angle option (FitzQuake/DarkPlaces style)
		{	
#if 0
			if (cl.model_precache[cl.stats[STAT_WEAPON]] && strcmp (cl.model_precache[cl.stats[STAT_WEAPON]]->name,  "progs/v_shot2.mdl"))
#endif
			// Somehow I trust the PSPQuake more
			// But wtf is viewsize 130?
			if (scr_viewsize.value == 130)
				view->origin[2] += 2.25;
			else if (scr_viewsize.value == 120)
				view->origin[2] += 2.25;
			else if (scr_viewsize.value == 110)
				view->origin[2] += 2.5;
			else if (scr_viewsize.value == 100)
				view->origin[2] += 3;
			else if (scr_viewsize.value == 90)
				view->origin[2] += 2.5;
			else if (scr_viewsize.value <= 80)
				view->origin[2] += 2;
	    }
	}
	
	view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
	view->frame = cl.stats[STAT_WEAPONFRAME];
	view->colormap = vid.colormap;

// set up the refresh position
	if (v_gunkick.value)
		VectorAdd (r_refdef.viewangles, cl.punchangle, r_refdef.viewangles);

// smooth out stair step ups
	if (cl.onground && ent->origin[2] - oldz > 0)
	{
		float steptime;
	
		steptime = cl.time - cl.oldtime;
		if (steptime < 0)
	//FIXME		I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 80;
		if (oldz > ent->origin[2])
			oldz = ent->origin[2];
		if (ent->origin[2] - oldz > 12)
			oldz = ent->origin[2] - 12;
		r_refdef.vieworg[2] += oldz - ent->origin[2];
		view->origin[2] += oldz - ent->origin[2];
	}
	else
	{
		oldz = ent->origin[2];
	}		

    if (chase_active.value)
		Chase_Update ();
}
/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
extern vrect_t	scr_vrect;
#ifndef GLQUAKE
int	Sbar_ColorForMap (int m);
#endif
void V_RenderView (void)
{
	if (con_forcedup)
		return;

#ifdef SUPPORTS_SOFTWARE_FTESTAIN
	R_LessenStains();  //qbism ftestain
#endif
#ifdef SUPPORTS_KUROK
    if (!kurok)
#endif
    {
		// don't allow cheats in multiplayer
		if (cl.maxclients > 1)
		{
			Cvar_SetValueByRef (&scr_ofsx, 0);
			Cvar_SetValueByRef (&scr_ofsy, 0);
			Cvar_SetValueByRef (&scr_ofsz, 0);
		}

#ifdef SUPPORTS_ENHANCED_GAMMA
	// Baker hwgamma support
	if (using_hwgamma) {
		if (cls.state != ca_connected) {
			V_CalcBlend ();
			return;
		}
	}
#endif
    }

	if (cl.intermission) // intermission / finale rendering
	{
		V_CalcIntermissionRefdef ();
	}
	else
	{
		if (!cl.paused)
			V_CalcRefdef ();
	}

	R_PushDlights ();

	if (lcd_x.value)
	{
		// render two interleaved views
		int		i;

		vid.rowbytes <<= 1;
		vid.aspect *= 0.5;

		r_refdef.viewangles[YAW] -= lcd_yaw.value;
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg[i] -= right[i]*lcd_x.value;
		R_RenderView ();

		vid.buffer += vid.rowbytes>>1;

		R_PushDlights ();

		r_refdef.viewangles[YAW] += lcd_yaw.value*2;
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg[i] += 2*right[i]*lcd_x.value;
		R_RenderView ();

		vid.buffer -= vid.rowbytes>>1;

		r_refdef.vrect.height <<= 1;

		vid.rowbytes >>= 1;
		vid.aspect *= 2;
	}
	else
	{
		R_RenderView ();
	}

#if !defined(GLQUAKE) && !defined(PSP_HARDWARE_VIDEO)

	if (cl_colorshow.value && !cls.demoplayback &&  cl.maxclients>1) {
		// Don't display this during demo playback because we actually don't know!
		Draw_Fill	(12, 12, 16, 16, Sbar_ColorForMap(((int)cl_color.value) & 15<<4));	// Baker 3.99n: display pants color in top/left
	}

#endif


}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("v_cshift", V_cshift_f);
	Cmd_AddCommand ("bf", V_BonusFlash_f);
	Cmd_AddCommand ("centerview", V_StartPitchDrift_f);

	Cvar_RegisterVariable (&lcd_x, NULL);
	Cvar_RegisterVariable (&lcd_yaw, NULL);

#ifdef SUPPORTS_ENHANCED_GAMMA
	// Baker hwgamma support
	Cvar_RegisterVariable (&gl_hwblend, NULL);
	Cvar_RegisterVariable (&v_gamma, NULL);
	Cvar_RegisterVariable (&v_contrast, NULL);
	// Baker end hwgamma support
#endif

	Cvar_RegisterVariable (&v_centermove, NULL);
	Cvar_RegisterVariable (&v_centerspeed, NULL);

	Cvar_RegisterVariable (&v_iyaw_cycle, NULL);
	Cvar_RegisterVariable (&v_iroll_cycle, NULL);
	Cvar_RegisterVariable (&v_ipitch_cycle, NULL);
	Cvar_RegisterVariable (&v_iyaw_level, NULL);
	Cvar_RegisterVariable (&v_iroll_level, NULL);
	Cvar_RegisterVariable (&v_ipitch_level, NULL);

	Cvar_RegisterVariable (&v_idlescale, NULL);
	Cvar_RegisterVariable (&crosshair, NULL);
	Cvar_RegisterVariable (&r_viewmodeloffset, NULL);

	Cvar_RegisterVariable (&cl_crossx, NULL);
	Cvar_RegisterVariable (&cl_crossy, NULL);
	Cvar_RegisterVariable (&gl_cshiftpercent, NULL);

	Cvar_RegisterVariable (&scr_ofsx, NULL);
	Cvar_RegisterVariable (&scr_ofsy, NULL);
	Cvar_RegisterVariable (&scr_ofsz, NULL);
	Cvar_RegisterVariable (&cl_rollspeed, NULL);
	Cvar_RegisterVariable (&cl_rollangle, NULL);

	Cvar_RegisterVariable (&cl_bob, NULL);
	Cvar_RegisterVariable (&cl_bobcycle, NULL);
	Cvar_RegisterVariable (&cl_bobup, NULL);
#ifdef SUPPORTS_KUROK
	Cvar_RegisterVariable (&cl_bobside, NULL);
	Cvar_RegisterVariable (&cl_bobsidecycle, NULL);
	Cvar_RegisterVariable (&cl_bobsideup, NULL);

	Cvar_RegisterVariable (&cl_gunpitch, NULL);
#endif

	Cvar_RegisterVariable (&r_truegunangle, NULL);
	Cvar_RegisterVariable (&v_kicktime, NULL);
	Cvar_RegisterVariable (&v_kickroll, NULL);
	Cvar_RegisterVariable (&v_kickpitch, NULL);
	Cvar_RegisterVariable (&v_gunkick, NULL);

	Cvar_RegisterVariable (&vold_gamma, NULL);

	BuildGammaTable (1.0);	// no gamma yet

#ifdef PROQUAKE_EXTENSION
	// JPG 1.05 - colour shifts
	Cvar_RegisterVariable (&pq_waterblend, NULL);
	Cvar_RegisterVariable (&pq_quadblend, NULL);
	Cvar_RegisterVariable (&pq_pentblend, NULL);
	Cvar_RegisterVariable (&pq_ringblend, NULL);
	Cvar_RegisterVariable (&pq_suitblend, NULL);
#ifndef GLQUAKE
	Cvar_RegisterVariable (&r_polyblend, NULL);	// JPG 3.30 - winquake version of gl_polyblend
#endif
#endif

#ifdef SUPPORTS_AUTOID
	Cvar_RegisterVariable (&scr_autoid, NULL);
#endif

}


