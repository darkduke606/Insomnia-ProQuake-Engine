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
// cl_main.c  -- client main loop

#include "quakedef.h"
#ifdef HTTP_DOWNLOAD
#include "curl.h"
#endif

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly
cvar_t	cl_name = {"_cl_name", "player", true};
cvar_t	cl_color = {"_cl_color", "0", true};

cvar_t	cl_shownet = {"cl_shownet","0"};	// can be 0, 1, or 2
cvar_t	cl_nolerp = {"cl_nolerp","0"};
cvar_t  cl_gameplayhack_monster_lerp = {"cl_gameplayhack_monster_lerp","1"};

cvar_t	lookspring = {"lookspring","0", true};
cvar_t	lookstrafe = {"lookstrafe","0", true};
cvar_t	sensitivity = {"sensitivity","3", true};

cvar_t	m_pitch = {"m_pitch","0.022", true};
cvar_t	m_yaw = {"m_yaw","0.022", true};
cvar_t	m_forward = {"m_forward","1", true};
cvar_t	m_side = {"m_side","0.8", true};


#ifdef PSP_ANALOG_STICK
cvar_t	cl_autoaim = {"cl_autoaim", "0", true};
cvar_t	lookcenter = {"lookcenter","1", true};
cvar_t	in_tolerance = {"tolerance","0.25", true};
cvar_t	in_acceleration = {"acceleration","1.0", true};

cvar_t	in_freelook_analog = {"in_freelook_analog", "0", true};
cvar_t	in_disable_analog = {"in_disable_analog", "0", true};
cvar_t	in_analog_strafe = {"in_analog_strafe", "0", true};

cvar_t  in_x_axis_adjust = {"in_x_axis_adjust", "0", true};
cvar_t  in_y_axis_adjust = {"in_y_axis_adjust", "0", true};
#endif

#ifdef PROQUAKE_EXTENSION
// JPG - added these for %r formatting
cvar_t	pq_needrl = {"pq_needrl", "I need RL", true};
cvar_t	pq_haverl = {"pq_haverl", "I have RL", true};
cvar_t	pq_needrox = {"pq_needrox", "I need rockets", true};

// JPG - added these for %p formatting
cvar_t	pq_quad = {"pq_quad", "quad", true};
cvar_t	pq_pent = {"pq_pent", "pent", true};
cvar_t	pq_ring = {"pq_ring", "eyes", true};

// JPG 3.00 - added these for %w formatting
cvar_t	pq_weapons = {"pq_weapons", "SSG:NG:SNG:GL:RL:LG", true};
cvar_t	pq_noweapons = {"pq_noweapons", "no weapons", true};

// JPG 1.05 - translate +jump to +moveup under water
cvar_t	pq_moveup = {"pq_moveup", "0", true};

// JPG 3.00 - added this by request
cvar_t	pq_smoothcam = {"pq_smoothcam", "1", true};
#endif

#ifdef HTTP_DOWNLOAD
cvar_t	cl_web_download		= {"cl_web_download", "1", true};
cvar_t	cl_web_download_url	= {"cl_web_download_url", "http://downloads.quake-1.com/", true};
#endif

#ifdef SUPPORTS_DEMO_CONTROLS
cvar_t	cl_demospeed		= {"cl_demospeed", "1"}; // Baker 3.75 - demo rewind/ff
#endif
cvar_t	cl_bobbing		= {"cl_bobbing", "0"};

client_static_t	cls;
client_state_t	cl;
// FIXME: put these on hunk?
efrag_t			cl_efrags[MAX_EFRAGS];
entity_t		cl_entities[MAX_EDICTS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

int				cl_numvisedicts;
entity_t		*cl_visedicts[MAX_VISEDICTS];

#ifdef SUPPORTS_AUTOID
modelindex_t		cl_modelindex[NUM_MODELINDEX];
char			*cl_modelnames[NUM_MODELINDEX];
#endif

#ifdef PROQUAKE_EXTENSION
extern cvar_t scr_fov;
static float			savedsensitivity;
static float			savedfov;
#endif

/*
=====================
CL_ClearState
=====================
*/
void CL_ClearState (void)
{
	int			i;

	if (!sv.active)
		Host_ClearMemory ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	SZ_Clear (&cls.message);

// clear other arrays
	memset (cl_efrags, 0, sizeof(cl_efrags));
	memset (cl_entities, 0, sizeof(cl_entities));
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));
	memset (cl_temp_entities, 0, sizeof(cl_temp_entities));
	memset (cl_beams, 0, sizeof(cl_beams));

// allocate the efrags and chain together into a free list
	cl.free_efrags = cl_efrags;
	for (i=0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;

}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
// stop sounds (especially looping!)
	S_StopAllSounds (true);

// CDAudio_Stop
	CDAudio_Stop();


// bring the console down and fade the colors back to normal
//	SCR_BringDownConsole ();

	// This makes sure ambient sounds remain silent
	cl.worldmodel = NULL;

#ifdef HTTP_DOWNLOAD
	// We have to shut down webdownloading first
	if( cls.download.web )
	{
		cls.download.disconnect = true;
		return;
	}

#endif

// if running a local server, shut it down
	if (cls.demoplayback)
	{
		CL_StopPlayback ();
	}
	else if (cls.state == ca_connected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending clc_disconnect\n");
		SZ_Clear (&cls.message);
		MSG_WriteByte (&cls.message, clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		SZ_Clear (&cls.message);
		NET_Close (cls.netcon);

		cls.state = ca_disconnected;
		if (sv.active)
			Host_ShutdownServer(false);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;

	SCR_EndLoadingPlaque (); // Baker: any disconnect state should end the loading plague, right?

}

void CL_Disconnect_f (void)
{
#ifdef HTTP_DOWNLOAD
	// We have to shut down webdownloading first
	if( cls.download.web )
	{
		cls.download.disconnect = true;
		return;
	}

#endif
	CL_Disconnect ();
	if (sv.active)
		Host_ShutdownServer (false);
}


/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (char *host)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	cls.netcon = NET_Connect (host);
	if (!cls.netcon) // Baker 3.60 - Rook's Qrack port 26000 notification on failure
	{
		Con_Printf ("\nsyntax: connect server:port (port is optional)\n");//r00k added
		if (net_hostport != 26000)
			Con_Printf ("\nTry using port 26000\n");//r00k added
		Host_Error ("connect failed");
	}

	Con_DPrintf ("CL_EstablishConnection: connected to %s\n", host);

#ifdef PROQUAKE_EXTENSION
		// JPG - proquake message
	if (cls.netcon->mod == MOD_PROQUAKE)
	{
		if (pq_cheatfree)
			Con_Printf("%c%cConnected to Cheat-Free server%c\n", 1, 29, 31);
		else
			Con_Printf("%c%cConnected to ProQuake server%c\n", 1, 29, 31);
	}
#endif
	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing

	MSG_WriteByte(&cls.message, clc_nop);	// JPG 3.40 - fix for NAT
}

#ifdef PROQUAKE_EXTENSION
unsigned source_data[1056] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

byte *COM_LoadFile (char *path, int usehunk);	// JPG 3.00 - model checking
unsigned source_key1 = 0x36117cbd;
unsigned source_key2 = 0x2e26857c;
#endif

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
	char 	str[8192];
#ifdef PROQUAKE_EXTENSION
	int i;	// JPG 3.00
#endif

Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");

#ifdef PROQUAKE_EXTENSION
		// JPG 3.50
		if (cls.netcon && !cls.netcon->encrypt)
			cls.netcon->encrypt = 3;
#endif
		break;

	case 2:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("name \"%s\"\n", cl_name.string));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("color %i %i\n", ((int)cl_color.value)>>4, ((int)cl_color.value)&15));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		snprintf(str, sizeof(str), "spawn %s", cls.spawnparms);
		MSG_WriteString (&cls.message, str);
#ifdef PROQUAKE_EXTENSION
		// JPG 3.20 - model and .exe checking
		if (pq_cheatfree)
		{
			FILE *f;
			unsigned crc;
			char path[64];
#ifdef SUPPORTS_CHEATFREE_MODE

			strcpy(path, argv[0]);
#endif // ^^ MACOSX can't support this code but Windows/Linux do
#ifdef _WIN32
			if (!strstr(path, ".exe") && !strstr(path, ".EXE"))
				strlcat (path, ".exe", sizeof(path));
#endif // ^^ This is Windows operating system specific; Linux does not need
			f = fopen(path, "rb");
			if (!f)
				Host_Error("Could not open %s", path);
			fclose(f);
			crc = Security_CRC_File(path);
			MSG_WriteLong(&cls.message, crc);
			MSG_WriteLong(&cls.message, source_key1);

			if (!cl.model_precache[1])
				MSG_WriteLong(&cls.message, 0);
			for (i = 1 ; cl.model_precache[i] ; i++)
			{
				if (cl.model_precache[i]->name[0] != '*')
				{
					byte *data;
					int len;

					data = COM_LoadFile(cl.model_precache[i]->name, 2);			// 2 = temp alloc on hunk
					if (data)
					{
						len = (*(int *)(data - 12)) - 16;							// header before data contains size
						MSG_WriteLong(&cls.message, Security_CRC(data, len));
					}
					else
						Host_Error("Could not load %s", cl.model_precache[i]->name);
				}
			}
		}
#endif
		break;

	case 3:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "begin");
		Cache_Report ();		// print remaining memory
#ifdef PROQUAKE_EXTENSION
		// JPG 3.50
		if (cls.netcon)
			cls.netcon->encrypt = 1;
#endif
		break;

	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
#ifdef SUPPORTS_DEMO_AUTOPLAY
			if (nostartdemos)
				nostartdemos = false; // Baker 3.76 -- part of hack to avoid start demos with dem autoplay
			else
#endif  // ^^ Uses Windows specific functionality
				Con_DPrintf ("No demos listed with startdemos\n");

			CL_Disconnect();	// JPG 1.05 - patch by CSR to fix crash
			cls.demonum = -1;
			return;
		}
	}

	snprintf(str, sizeof(str),"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
static void CL_PrintEntities_f (void)
{
	entity_t	*ent;
	int			i;

	for (i=0,ent=cl_entities ; i<cl.num_entities ; i++,ent++)
	{
		Con_Printf ("%3i:",i);
		if (!ent->model)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",ent->model->name,ent->frame, ent->origin[0], ent->origin[1], ent->origin[2], ent->angles[0], ent->angles[1], ent->angles[2]);
	}
}



/*
===============
CL_AllocDlight
===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
#ifdef SUPPORTS_COLORED_LIGHTS
				dl->color[0] = dl->color[1] = dl->color[2] = 1; // LordHavoc: .lit support
#endif
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
#ifdef SUPPORTS_COLORED_LIGHTS
			dl->color[0] = dl->color[1] = dl->color[2] = 1; // LordHavoc: .lit support
#endif
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
#ifdef SUPPORTS_COLORED_LIGHTS
	dl->color[0] = dl->color[1] = dl->color[2] = 1; // LordHavoc: .lit support
#endif
	return dl;
}


/*
===============
CL_DecayLights
===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float		time;

	time = cl.time - cl.oldtime;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;

		dl->radius -= time*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
static float	CL_LerpPoint (void)
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];

	if (!f || cl_nolerp.value || cls.timedemo || sv.active)
	{
		// Baker 3.75 demo rewind
		cl.time = cl.ctime = cl.mtime[0];
		return 1;
	}

	if (f > 0.1)
	{	// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1;
	}
	frac = (cl.ctime - cl.mtime[1]) / f;

	if (frac < 0)
	{
		if (frac < -0.01)
			cl.time = cl.ctime = cl.mtime[1];
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
			cl.time = cl.ctime = cl.mtime[0];
		frac = 1;
	}

	return frac;
}

#ifdef PROQUAKE_EXTENSION
extern cvar_t pq_timer; // JPG - need this for CL_RelinkEntities
#endif

/*
===============
CL_RelinkEntities
===============
*/
static void CL_RelinkEntities (void)
{
	entity_t	*ent;
	int			i, j;
	float		frac, f, d, bobjrotate;
	vec3_t		delta, oldorg;
	dlight_t	*dl;

// determine partial update time
	frac = CL_LerpPoint ();

#ifdef PROQUAKE_EXTENSION
// JPG - check to see if we need to update the status bar
	if (pq_timer.value && ((int) cl.time != (int) cl.oldtime))
		Sbar_Changed();
#endif

	cl_numvisedicts = 0;

// interpolate player info
	for (i=0 ; i<3 ; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);
#ifdef PROQUAKE_EXTENSION
	//PROQUAKE ADDITION --START
	if (cls.demoplayback || (last_angle_time > host_time && !(in_attack.state & 3)) && pq_smoothcam.value) // JPG - check for last_angle_time for smooth chasecam!
#else
	if (cls.demoplayback)
#endif
	{
	// interpolate the angles
		for (j=0 ; j<3 ; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;
#ifdef PROQUAKE_EXTENSION
			// JPG - I can't set cl.viewangles anymore since that messes up the demorecording.  So instead,
			// I'll set lerpangles (new variable), and view.c will use that instead.
			cl.lerpangles[j] = cl.mviewangles[1][j] + frac*d;
#else
			cl.viewangles[j] = cl.mviewangles[1][j] + frac*d;
#endif
		}
	}
#ifdef PROQUAKE_EXTENSION
	else
		VectorCopy(cl.viewangles, cl.lerpangles);
	//PROQUAKE ADDITION --END
#endif

	bobjrotate = anglemod(100*cl.time);

// start on the entity after the world
	for (i=1,ent=cl_entities+1 ; i<cl.num_entities ; i++,ent++)
	{
		if (!ent->model)
		{	// empty slot
			if (ent->forcelink)
				R_RemoveEfrags (ent);	// just became empty
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0])
		{
			ent->model = NULL;
#if 1 // Baker: interpolation fix you need
		    // fenix@io.com: model transform interpolation
            ent->frame_start_time = 0;
            ent->translate_start_time = 0;
            ent->rotate_start_time = 0;
#endif
			continue;
		}

		VectorCopy (ent->origin, oldorg);

		if (ent->forcelink)
		{	// the entity was not updated in the last message so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j=0 ; j<3 ; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
				if (delta[j] > 100 || delta[j] < -100)
					f = 1;		// assume a teleportation, not a motion
			}
#if 1 // Baker: interpolation fix you need
            // fenix@io.com: model transform interpolation
            // interpolation should be reset in the event of a large delta

            if (f >= 1)
            {
//              ent->frame_start_time = 0;
                ent->translate_start_time = 0;
                ent->rotate_start_time = 0;
            }
#endif

		    // interpolate the origin and angles
			for (j=0 ; j<3 ; j++)
			{
				ent->origin[j] = ent->msg_origins[1][j] + f*delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;
				ent->angles[j] = ent->msg_angles[1][j] + f*d;
			}

		}

// rotate binary objects locally
		if (ent->model->flags & EF_ROTATE)
		{
			ent->angles[1] = bobjrotate;
			if (cl_bobbing.value)
				ent->origin[2] += sin(bobjrotate / 90 * M_PI) * 5 + 5;
		}

		// EF_BRIGHTFIELD is not used by original progs
		if (ent->effects & EF_BRIGHTFIELD)
			R_EntityParticles (ent);

		if (ent->effects & EF_MUZZLEFLASH)
		{
			vec3_t		fv, rv, uv;

			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->origin[2] += 16;
			AngleVectors (ent->angles, fv, rv, uv);

			VectorMA (dl->origin, 18, fv, dl->origin);
			dl->radius = 200 + (rand()&31);
			dl->minlight = 32;
			dl->die = cl.time + 0.1;
#ifdef SUPPORTS_KUROK
            dl->minlight = 16;
            dl->color[0] = 0.9;
            dl->color[1] = 0.8;
            dl->color[2] = 0.6;
#endif
		}
		if (ent->effects & EF_BRIGHTLIGHT)
		{
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->origin[2] += 16;
			dl->radius = 400 + (rand()&31);
			dl->die = cl.time + 0.001;
#ifdef SUPPORTS_KUROK
            dl->color[0] = 1;
            dl->color[1] = 0.8;
            dl->color[2] = 0.5;
#endif
		}
		if (ent->effects & EF_DIMLIGHT)
		{
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->radius = 200 + (rand()&31);
			dl->die = cl.time + 0.001;
#ifdef SUPPORTS_KUROK
            dl->radius = 100 + (rand()&31);
            dl->color[0] = 0.5;
            dl->color[1] = 0.5;
            dl->color[2] = 0.5;
#endif
		}
#ifdef SUPPORTS_KUROK
        // Kurok effects
        if (ent->effects & EF_REDLIGHT)
        {
            dl = CL_AllocDlight (i);
            VectorCopy (ent->origin,  dl->origin);
            dl->radius = 200 + (rand()&31);
            dl->die = cl.time + 0.001;
            dl->radius = 150 + (rand()&31);
            dl->color[0] = 2;
            dl->color[1] = 0.25;
            dl->color[2] = 0.25;
        }
        if (ent->effects & EF_BLUELIGHT)
        {
            dl = CL_AllocDlight (i);
            VectorCopy (ent->origin,  dl->origin);
            dl->radius = 200 + (rand()&31);
            dl->die = cl.time + 0.001;
            dl->radius = 150 + (rand()&31);
            dl->color[0] = 0.25;
            dl->color[1] = 0.25;
            dl->color[2] = 2;
        }
#endif

#ifdef QUAKE2
		if (ent->effects & EF_DARKLIGHT)
		{
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->radius = 200.0 + (rand()&31);
			dl->die = cl.time + 0.001;
			dl->dark = true;
		}
		if (ent->effects & EF_LIGHT)
		{
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.001;
		}
#endif

		if (ent->model->flags & EF_GIB)
			R_RocketTrail (oldorg, ent->origin, 2);
		else if (ent->model->flags & EF_ZOMGIB)
			R_RocketTrail (oldorg, ent->origin, 4);
		else if (ent->model->flags & EF_TRACER)
			R_RocketTrail (oldorg, ent->origin, 3);
		else if (ent->model->flags & EF_TRACER2)
			R_RocketTrail (oldorg, ent->origin, 5);
		else if (ent->model->flags & EF_ROCKET)
		{
			R_RocketTrail (oldorg, ent->origin, 0);
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.01;
		}
		else if (ent->model->flags & EF_GRENADE)
			R_RocketTrail (oldorg, ent->origin, 1);
		else if (ent->model->flags & EF_TRACER3)
			R_RocketTrail (oldorg, ent->origin, 6);
#ifdef SUPPORTS_KUROK_PROTOCOL
        // Tomaz - QC Glow Begin

        else if (ent->glow_size)
        {
            dl = CL_AllocDlight (i);
            VectorCopy (ent->origin, dl->origin);
            dl->radius = ent->glow_size;
            dl->die = cl.time + 0.001;
            dl->color[0] = ent->glow_red;
            dl->color[1] = ent->glow_green;
            dl->color[2] = ent->glow_blue;
        }

        // Tomaz - QC Glow End
#endif
		ent->forcelink = false;

#ifdef PROQUAKE_EXTENSION
		if (i == cl.viewentity && (!chase_active.value || pq_cheatfree))	// JPG 3.20 - added pq_cheatfree
#else
		if (i == cl.viewentity && !chase_active.value)
#endif
			continue;

		if (cl_numvisedicts < MAX_VISEDICTS)
		{
			cl_visedicts[cl_numvisedicts] = ent;
			cl_numvisedicts++;
		}
#ifdef SUPPORTS_ENTITY_ALPHA
		if (!ent->transparency)
			ent->transparency = 1;
#endif
	}

}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (void)
{
	int		ret;

	/*cl.oldtime = cl.time;
	cl.time += host_frametime;  //Baker 3.75 old way */

	// Baker 3.75 - demo rewind
	cl.oldtime = cl.ctime;
	cl.time += host_frametime;

#ifdef SUPPORTS_DEMO_CONTROLS
	if (cl_demorewind.value && cls.demoplayback)	// by joe
		cl.ctime -= host_frametime;
	else
#endif
		cl.ctime += host_frametime;
	// Baker 3.75 - end demo fast rewind

	do
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;

		cl.last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && cls.state == ca_connected);

	if (cl_shownet.value)
		Con_Printf ("\n");

	CL_RelinkEntities ();
	CL_UpdateTEnts ();

// bring the links up to date
	return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);

	// allow mice or other external controllers to add to the move
#ifdef PSP_ANALOG_STICK
	if (!in_disable_analog.value)
#endif
		IN_Move (&cmd);

	// send the unreliable message
		CL_SendMove (&cmd);

	}

	if (cls.demoplayback)
	{
		SZ_Clear (&cls.message);
		return;
	}

// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all

	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_DPrintf ("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_WriteToServer: lost server connection");

	SZ_Clear (&cls.message);
}

// Baker 3.85:  This should really be located elsewhere, but duplicating it in both gl_screen.c and screen.c is silly.
//              Quakeworld has the equivalent in cl_cmd.c

#ifdef PROQUAKE_EXTENSION
extern cvar_t default_fov;

void CL_Fov_f (void) {
	if (scr_fov.value == 90.0 && default_fov.value) {
		if (default_fov.value == 90)
			return; // Baker 3.99k: Don't do a message saying default FOV has been set to 90 if it is 90!

		Cvar_SetValueByRef (&scr_fov, default_fov.value);
		Con_Printf("fov set to default_fov %s\n", default_fov.string);
	}
}

void CL_Default_fov_f (void) {

	if (default_fov.value == 0)
		return; // Baker: this is totally permissible and happens with Reset to defaults.

	if (default_fov.value < 10.0 || default_fov.value > 140.0) {
		Cvar_SetValueByRef (&default_fov, 0.0f);
		Con_Printf("Default fov %s is out-of-range; set to 0\n", default_fov.string);
	}

}

// End Baker



/*
================
CL_SaveFOV

Saves the FOV
================
*/
static void CL_SaveFOV_f (void) {
	savedfov = scr_fov.value;
}

/*
================
CL_RestoreFOV

Restores FOV to saved level
================
*/
static void CL_RestoreFOV_f (void) {
	if (!savedfov) {
		Con_Printf("RestoreFOV: No saved FOV to restore\n");
		return;
	}

	Cvar_SetValueByRef (&scr_fov, savedfov);
}

/*
================
CL_SaveSensivity

Saves the Sensitivity
================
*/
static void CL_SaveSensitivity_f (void) {
	savedsensitivity = sensitivity.value;
}

/*
================
CL_RestoreSensitivity

Restores Sensitivity to saved level
================
*/
static void CL_RestoreSensitivity_f (void) {
	if (!savedsensitivity) {
		Con_Printf("RestoreSensitivity: No saved SENSITIVITY to restore\n");
		return;
	}

	Cvar_SetValueByRef (&sensitivity, savedsensitivity);
}
#endif

/*
=============
CL_Tracepos_f -- johnfitz

display impact point of trace along VPN
=============
*/

static void CL_Tracepos_f (void)
{
	vec3_t	v, w;
	extern void TraceLine (vec3_t start, vec3_t end, vec3_t impact);

	VectorScale(vpn, 8192.0, v);
	TraceLine(r_refdef.vieworg, v, w);

	if (VectorLength(w) == 0)
		Con_Printf ("Tracepos: trace didn't hit anything\n");
	else
		Con_Printf ("Tracepos: (%i %i %i)\n", (int)w[0], (int)w[1], (int)w[2]);
}

/*
=============
CL_Viewpos_f -- johnfitz

display client's position and angles
=============
*/
void CL_Viewpos_f (void)
{
#if 0
	//camera position
	Con_Printf ("Viewpos: (%i %i %i) %i %i %i\n",
		(int)r_refdef.vieworg[0],
		(int)r_refdef.vieworg[1],
		(int)r_refdef.vieworg[2],
		(int)r_refdef.viewangles[PITCH],
		(int)r_refdef.viewangles[YAW],
		(int)r_refdef.viewangles[ROLL]);
#else
	//player position
	Con_Printf ("You are at xyz = %i %i %i   angles: %i %i %i\n",
		(int)cl_entities[cl.viewentity].origin[0],
		(int)cl_entities[cl.viewentity].origin[1],
		(int)cl_entities[cl.viewentity].origin[2],
		(int)cl.viewangles[PITCH],
		(int)cl.viewangles[YAW],
		(int)cl.viewangles[ROLL]);
#endif
}

#ifdef SUPPORTS_ENTITY_ALPHA
void SortEntitiesByTransparency (void)
{
	int		i, j;
	entity_t	*tmp;

	for (i = 0 ; i < cl_numvisedicts ; i++)
	{
		if (cl_visedicts[i]->istransparent)
		{
			for (j = cl_numvisedicts - 1 ; j > i ; j--)
			{
				// if not transparent, exchange with transparent
				if (!(cl_visedicts[j]->istransparent))
				{
					tmp = cl_visedicts[i];
					cl_visedicts[i] = cl_visedicts[j];
					cl_visedicts[j] = tmp;
					break;
				}
			}
			if (j == i)
				return;
		}
	}
}
#endif


/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	SZ_Alloc (&cls.message, 1024);

	CL_InitInput ();
	CL_InitTEnts ();

// register our commands
	Cvar_RegisterVariable (&cl_name, NULL);
	Cvar_RegisterVariable (&cl_color, NULL);
	Cvar_RegisterVariable (&cl_upspeed, NULL);
	Cvar_RegisterVariable (&cl_forwardspeed, NULL);
	Cvar_RegisterVariable (&cl_backspeed, NULL);
	Cvar_RegisterVariable (&cl_sidespeed, NULL);
	Cvar_RegisterVariable (&cl_movespeedkey, NULL);
	Cvar_RegisterVariable (&cl_yawspeed, NULL);
	Cvar_RegisterVariable (&cl_pitchspeed, NULL);
	Cvar_RegisterVariable (&cl_anglespeedkey, NULL);
	Cvar_RegisterVariable (&cl_shownet, NULL);
	Cvar_RegisterVariable (&cl_nolerp, NULL);
	Cvar_RegisterVariable (&lookspring, NULL);
	Cvar_RegisterVariable (&lookstrafe, NULL);
	Cvar_RegisterVariable (&sensitivity, NULL);

#ifdef PSP_ANALOG_STICK
	Cvar_RegisterVariable (&cl_autoaim, NULL);
	Cvar_RegisterVariable (&lookcenter, NULL);

	Cvar_RegisterVariable (&in_tolerance, NULL);
	Cvar_RegisterVariable (&in_acceleration, NULL);
	Cvar_RegisterVariable (&in_freelook_analog, NULL);
	Cvar_RegisterVariable (&in_disable_analog, NULL);
	Cvar_RegisterVariable (&in_analog_strafe, NULL);

	Cvar_RegisterVariable (&in_x_axis_adjust, NULL);
	Cvar_RegisterVariable (&in_y_axis_adjust, NULL);
#endif
	Cvar_RegisterVariable (&m_pitch, NULL);
	Cvar_RegisterVariable (&m_yaw, NULL);
	Cvar_RegisterVariable (&m_forward, NULL);
	Cvar_RegisterVariable (&m_side, NULL);

	Cvar_RegisterVariable (&cl_gameplayhack_monster_lerp, NULL); // Hacks!

//	Cvar_RegisterVariable (&cl_autofire, NULL);

#ifdef SUPPORTS_DEMO_CONTROLS
	Cvar_RegisterVariable (&cl_demorewind, NULL);
	Cvar_RegisterVariable (&cl_demospeed, NULL);
#endif
	Cvar_RegisterVariable (&cl_bobbing, NULL);

	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
	Cmd_AddCommand ("tracepos", CL_Tracepos_f); //johnfitz
	Cmd_AddCommand ("viewpos", CL_Viewpos_f); //Baker 3.76 - Using FitzQuake's viewpos instead of my own

#ifdef PROQUAKE_EXTENSION
	Cmd_AddCommand ("savefov", CL_SaveFOV_f);
	Cmd_AddCommand ("savesensitivity", CL_SaveSensitivity_f);
	Cmd_AddCommand ("restorefov", CL_RestoreFOV_f);
	Cmd_AddCommand ("restoresensitivity", CL_RestoreSensitivity_f);


// JPG - added these for %r formatting
	Cvar_RegisterVariable (&pq_needrl, NULL);
	Cvar_RegisterVariable (&pq_haverl, NULL);
	Cvar_RegisterVariable (&pq_needrox, NULL);

	// JPG - added these for %p formatting
	Cvar_RegisterVariable (&pq_quad, NULL);
	Cvar_RegisterVariable (&pq_pent, NULL);
	Cvar_RegisterVariable (&pq_ring, NULL);

	// JPG 3.00 - %w formatting
	Cvar_RegisterVariable (&pq_weapons, NULL);
	Cvar_RegisterVariable (&pq_noweapons, NULL);

	// JPG 1.05 - added this for +jump -> +moveup translation
	Cvar_RegisterVariable (&pq_moveup, NULL);

	// JPG 3.02 - added this by request
	Cvar_RegisterVariable (&pq_smoothcam, NULL);
#endif
#ifdef HTTP_DOWNLOAD
	Cvar_RegisterVariable (&cl_web_download, NULL);
	Cvar_RegisterVariable (&cl_web_download_url, NULL);
#endif
}
