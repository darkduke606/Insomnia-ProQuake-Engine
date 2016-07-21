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
// host_cmd.c

#include "quakedef.h"

extern cvar_t	pausable;

cvar_t	cl_confirmquit = {"cl_confirmquit", "1", true}; // Baker 3.60
cvar_t	host_mapname	= {"host_mapname", ""};

#ifdef PROQUAKE_EXTENSION
// JPG - added these for spam protection
extern cvar_t	pq_spam_rate;
extern cvar_t	pq_spam_grace;
extern cvar_t   pq_connectmute; // Baker 3.99f: from Rook

// JPG 3.20 - control muting of players that change colour/name
extern cvar_t	pq_tempmute;

// JPG - feature request from Slot
extern cvar_t	pq_showedict;

// JPG 3.20 - optionally remove '\r'
extern cvar_t	pq_removecr;

// JPG 3.20 - optionally write player binds to server log
extern cvar_t	pq_logbinds;
#endif

int	current_skill;

void Mod_Print_f (void);

void VLight_ChangeLightAngle_f();   // RIOT - Vertex lights
void VLight_DumpLightTable_f();

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f (void);

void Host_Quit_f (void)
{
	if (cl_confirmquit.value)
	{
		if (key_dest != key_console && cls.state != ca_dedicated)
		{
			M_Menu_Quit_f ();
			return;
		}
	}
	CL_Disconnect ();
	Host_ShutdownServer(false);

	Sys_Quit ();
}

/*
=============
Host_Mapname_f -- johnfitz
=============
*/
void Host_Mapname_f (void)
{
	char name[MAX_QPATH];

	if (sv.active)
	{
		COM_StripExtension (sv.worldmodel->name + 5, name);
		Con_Printf ("\"mapname\" is \"%s\"\n", name);
		return;
	}

	if (cls.state == ca_connected)
	{
		COM_StripExtension (cl.worldmodel->name + 5, name);
		Con_Printf ("\"mapname\" is \"%s\"\n", name);
		return;
	}

	Con_Printf ("no map loaded\n");
}

/*
==================
Host_Status_f
==================
*/
#ifdef SUPPORTS_ANTIWALLHACK_SERVER
extern cvar_t sv_cullentities;
#endif
void Host_Status_f (void)
{
	client_t	*client;
	int			seconds;
	int			minutes;
	int			hours = 0;
	int			j, a, b, c; // Baker 3.60 - a,b,c added for IP
	void		(*print) (char *fmt, ...);

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			cl.console_status = true;	// JPG 1.05 - added this;
			Cmd_ForwardToServer_f ();
			return;
		}
		print = Con_Printf;
	}
	else
		print = SV_ClientPrintf;

#ifdef SUPPORTS_ANTIWALLHACK_SERVER
	print ("host:    %s (anti-wallhack %s)\n", Cvar_VariableString ("hostname"), sv_cullentities.value ? "on [mode: players]" : "off");
#else
	print ("host:    %s\n", Cvar_VariableString ("hostname"));
#endif
#ifdef PROQUAKE_EXTENSION
	print ("version: %s %4.2f %s\n", ENGINE_NAME, PROQUAKE_SERIES_VERSION, pq_cheatfree ? "cheat-free" : ""); // JPG - added ProQuake
#else
	print ("version: %s %4.2f %s\n", ENGINE_NAME, PROQUAKE_SERIES_VERSION, ""); // JPG - added ProQuake
#endif
	if (tcpipAvailable)
		print ("tcp/ip:  %s\n", my_tcpip_address);
	if (ipxAvailable)
		print ("ipx:     %s\n", my_ipx_address);
	print ("map:     %s\n", sv.name);
	print ("players: %i active (%i max)\n\n", net_activeconnections, svs.maxclients);
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		seconds = (int)(net_time - client->netconnection->connecttime);
		minutes = seconds / 60;
		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;
			if (hours)
				minutes -= (hours * 60);
		}
		else
			hours = 0;
		print ("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j+1, client->name, (int)client->edict->v.frags, hours, minutes, seconds);

		if (cmd_source != src_command && sscanf(client->netconnection->address, "%d.%d.%d", &a, &b, &c) == 3 && sv_ipmasking.value )  // Baker 3.60 - engine side ip masking from RocketGuy's ProQuake-r
         print ("   %d.%d.%d.xxx\n", a, b, c);  // Baker 3.60 - engine side ip masking from RocketGuy's ProQuake-r
         else  // Baker 3.60 - engine side ip masking from RocketGuy's ProQuake-r
		print ("   %s\n", client->netconnection->address);
	}
}

#ifdef QCEXEC
/*
==================
Host_QC_Exec

Execute QC commands from the console
==================
*/
void Host_QC_Exec (void)
{
	dfunction_t *f;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}
	if (!developer.value)
		return;
	f = 0;
	if ((f = ED_FindFunction(Cmd_Argv(1))) != NULL)
	{

		pr_global_struct->self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram ((func_t)(f - pr_functions));
	}
	else
		Con_Printf("bad function\n");

}
#endif

#ifdef PROQUAKE_EXTENSION
/*
==================
Host_Cheatfree_f
==================
*/
void Host_Cheatfree_f (void)
{
	if (sv.active)
		Con_Printf(pq_cheatfree ? "This is a cheat-free server\n" : "This is not a cheat-free server\n");
	else
		Con_Printf(pq_cheatfree ? "Connected to a cheat-free server\n" : "Not connected to a cheat-free server\n");
}
#endif

/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}

	if (pr_global_struct->deathmatch)
		return;

	//johnfitz -- allow user to explicitly set god mode to on or off
	switch (Cmd_Argc())
	{
	case 1:
	sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
	if (!((int)sv_player->v.flags & FL_GODMODE) )
		SV_ClientPrintf ("godmode OFF\n");
	else
		SV_ClientPrintf ("godmode ON\n");
		break;
	case 2:
		if (atof(Cmd_Argv(1)))
		{
			sv_player->v.flags = (int)sv_player->v.flags | FL_GODMODE;
			SV_ClientPrintf ("godmode ON\n");
		}
		else
		{
			sv_player->v.flags = (int)sv_player->v.flags & ~FL_GODMODE;
			SV_ClientPrintf ("godmode OFF\n");
		}
		break;
	default:
		Con_Printf("god [value] : toggle god mode. values: 0 = off, 1 = on\n");
		break;
	}
	//johnfitz
}

/*
==================
Host_Notarget_f
==================
*/
void Host_Notarget_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}

	if (pr_global_struct->deathmatch)
		return;

	//johnfitz -- allow user to explicitly set notarget to on or off
	switch (Cmd_Argc())
	{
	case 1:
	sv_player->v.flags = (int)sv_player->v.flags ^ FL_NOTARGET;
	if (!((int)sv_player->v.flags & FL_NOTARGET) )
		SV_ClientPrintf ("notarget OFF\n");
	else
		SV_ClientPrintf ("notarget ON\n");
		break;
	case 2:
		if (atof(Cmd_Argv(1)))
		{
			sv_player->v.flags = (int)sv_player->v.flags | FL_NOTARGET;
			SV_ClientPrintf ("notarget ON\n");
		}
		else
		{
			sv_player->v.flags = (int)sv_player->v.flags & ~FL_NOTARGET;
			SV_ClientPrintf ("notarget OFF\n");
		}
		break;
	default:
		Con_Printf("notarget [value] : toggle notarget mode. values: 0 = off, 1 = on\n");
		break;
	}
	//johnfitz
}

qboolean noclip_anglehack;

/*
==================
Host_Noclip_f
==================
*/
void Host_Noclip_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}

	if (pr_global_struct->deathmatch)
		return;

	//johnfitz -- allow user to explicitly set noclip to on or off
	switch (Cmd_Argc())
	{
	case 1:
	if (sv_player->v.movetype != MOVETYPE_NOCLIP)
	{
		noclip_anglehack = true;
		sv_player->v.movetype = MOVETYPE_NOCLIP;
		SV_ClientPrintf ("noclip ON\n");
	}
	else
	{
		noclip_anglehack = false;
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf ("noclip OFF\n");
	}
		break;
	case 2:
		if (atof(Cmd_Argv(1)))
		{
			noclip_anglehack = true;
			sv_player->v.movetype = MOVETYPE_NOCLIP;
			SV_ClientPrintf ("noclip ON\n");
		}
		else
		{
			noclip_anglehack = false;
			sv_player->v.movetype = MOVETYPE_WALK;
			SV_ClientPrintf ("noclip OFF\n");
		}
		break;
	default:
		Con_Printf("noclip [value] : toggle noclip mode. values: 0 = off, 1 = on\n");
		break;
	}
	//johnfitz
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}

	if (pr_global_struct->deathmatch)
		return;

	//johnfitz -- allow user to explicitly set noclip to on or off
	switch (Cmd_Argc())
	{
	case 1:
	if (sv_player->v.movetype != MOVETYPE_FLY)
	{
		sv_player->v.movetype = MOVETYPE_FLY;
		SV_ClientPrintf ("flymode ON\n");
	}
	else
	{
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf ("flymode OFF\n");
	}
		break;
	case 2:
		if (atof(Cmd_Argv(1)))
		{
			sv_player->v.movetype = MOVETYPE_FLY;
			SV_ClientPrintf ("flymode ON\n");
		}
		else
		{
			sv_player->v.movetype = MOVETYPE_WALK;
			SV_ClientPrintf ("flymode OFF\n");
		}
		break;
	default:
		Con_Printf("fly [value] : toggle fly mode. values: 0 = off, 1 = on\n");
		break;
	}
	//johnfitz
}


/*
==================
Host_Ping_f
==================
*/
void Host_Ping_f (void)
{
	int		i, j;
	float	total;
	client_t	*client;
#ifdef PROQUAKE_EXTENSION
	char *n;	// JPG - for ping +N
#endif
	if (cmd_source == src_command)
	{
#ifdef PROQUAKE_EXTENSION
		// JPG - check for ping +N
		if (Cmd_Argc() == 2)
		{
			if (cls.state != ca_connected)
				return;

			n = Cmd_Argv(1);
			if (*n == '+')
			{
				extern cvar_t pq_lag;
				Cvar_SetStringByRef (&pq_lag, n+1);
				return;
			}
		}
		cl.console_ping = true;		// JPG 1.05 - added this
#endif
		Cmd_ForwardToServer_f ();
		return;
	}

	SV_ClientPrintf ("Client ping times:\n");
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		total = 0;
		for (j=0 ; j<NUM_PING_TIMES ; j++)
			total+=client->ping_times[j];
		total /= NUM_PING_TIMES;
		SV_ClientPrintf ("%4i %s\n", (int)(total*1000), client->name);
	}
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f (void)
{
	int		i;
	char	name[MAX_QPATH];

	if (cmd_source != src_command)
		return;

	//johnfitz -- check for client having map before anything else
	//	snprintf (name, sizeof(name), "maps/%s.bsp", Cmd_Argv(1));
	//	if (COM_OpenFile (name, &i) == -1)
	//	{
	//		Con_Printf("Host_Map_f: cannot find map %s\n", name);
	//		return;
	//	}
	//johnfitz

	cls.demonum = -1;		// stop demo loop in case this fails

	CL_Disconnect ();
	Host_ShutdownServer(false);

	key_dest = key_game;			// remove console or menu
	SCR_BeginLoadingPlaque ();

	cls.mapstring[0] = 0;
	for (i=0 ; i<Cmd_Argc() ; i++)
	{
		strlcat (cls.mapstring, Cmd_Argv(i), sizeof(cls.mapstring));
		strlcat (cls.mapstring, " ", sizeof(cls.mapstring));
	}
	strlcat (cls.mapstring, "\n", sizeof(cls.mapstring));

	svs.serverflags = 0;			// haven't completed an episode yet
	strcpy (name, Cmd_Argv(1));
	SV_SpawnServer (name);
	if (!sv.active)
		return;
#ifdef PROQUAKE_EXTENSION
	// JPG 3.20 - cheat free
	pq_cheatfree = (pq_cvar_cheatfree.value && pq_cheatfreeEnabled);
	if (pq_cheatfree)
		Con_Printf("Spawning cheat-free server\n");
#endif

	if (cls.state != ca_dedicated)
	{
		strcpy (cls.spawnparms, "");

		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			strlcat (cls.spawnparms, Cmd_Argv(i), sizeof(cls.spawnparms));
			strlcat (cls.spawnparms, " ", sizeof(cls.spawnparms));
		}

		Cmd_ExecuteString ("connect local", src_command);
	}
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f (void)
{
	char	level[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("changelevel <levelname> : continue game on a new level\n");
		return;
	}
	if (!sv.active || cls.demoplayback)
	{
		Con_Printf ("Only the server may changelevel\n");
		return;
	}

	//johnfitz -- check for client having map before anything else
	//snprintf (level, sizeof(level), "maps/%s.bsp", Cmd_Argv(1));
	//if (COM_OpenFile (level, &i) == -1)
	//{
	//	Con_Printf("Host_Changelevel_f: cannot find map %s\n", level);
	//	//shut down server, disconnect, etc.
	//	return;
	//}
	//johnfitz

	SV_SaveSpawnparms ();
	strcpy (level, Cmd_Argv(1));
	SV_SpawnServer (level);
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f (void)
{
	char	mapname[MAX_QPATH];

	if (cls.demoplayback || !sv.active)
		return;

	if (cmd_source != src_command)
		return;

	// must copy out, because it gets cleared in sv_spawnserver
	strcpy (mapname, sv.name);
//	strlcpy (mapname, sv.name, MAX_QPATH); // Baker: string safe unless the map is 64 chars long?

	SV_SpawnServer (mapname);
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f (void)
{

#ifdef SUPPORTS_MULTIMAP_DEMOS
	if (cls.demoplayback) {
		Con_DPrintf("Demo playing; ignoring reconnect\n");
		return;
	}
#endif
	SCR_BeginLoadingPlaque ();
	cls.signon = 0;		// need new connection messages
}

extern char server_name[MAX_QPATH];	// JPG 3.50

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f (void)
{
	char	name[MAX_QPATH];

	cls.demonum = -1;		// stop demo loop in case this fails
	if (cls.demoplayback)
	{
		CL_StopPlayback ();
		CL_Disconnect ();
	}
	strcpy (name, Cmd_Argv(1));
	CL_EstablishConnection (name);
	Host_Reconnect_f ();

	strcpy(server_name, name);	// JPG 3.50
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

// mankrip - serverflags bugfix - begin
#define   VANILLA_SAVESTATE_VERSION   5
#define   SAVESTATE_VERSION   6 // only used for full saves
#define   SAVEGAME_VERSION   5 // only used for small saves

cvar_t		savegame_compatibility = {"savegame_compatibility", "0", true};
// mankrip - serverflags bugfix - end

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment (char *text)
{
	int		i;
	char	kills[20];

	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		text[i] = ' ';
	memcpy (text, cl.levelname, QMIN(strlen(cl.levelname),22)); //johnfitz -- only copy 22 chars.
	snprintf(kills, sizeof(kills), "kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	memcpy (text+22, kills, strlen(kills));
// convert space to _ to make stdio happy
	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		if (text[i] == ' ')
			text[i] = '_';
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}


/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f (void)
{
	char	name[256];
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH+1];

	if (cmd_source != src_command)
		return;

	if (!sv.active)
	{
		Con_Printf ("Not playing a local game.\n");
		return;
	}

	if (cl.intermission)
	{
		Con_Printf ("Can't save in intermission.\n");
		return;
	}

	if (svs.maxclients != 1)
	{
		Con_Printf ("Can't save multiplayer games.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}

	for (i=0 ; i<svs.maxclients ; i++)
	{
		if (svs.clients[i].active && (svs.clients[i].edict->v.health <= 0) )
		{
			Con_Printf ("Can't savegame with a dead player\n");
			return;
		}
	}

	snprintf(name, sizeof(name),  "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_ForceExtension (name, ".sav"); // joe: force to ".sav"

	Con_Printf ("Saving game to %s...\n", name);
#ifdef FLASH_FILE_SYSTEM
	f = as3OpenWriteFile(name);
#else
	f = fopen (name, "w");
#endif
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open save file for writing.\n");
		return;
	}

	//fprintf (f, "%i\n", SAVEGAME_VERSION);
	//Host_SavegameComment (comment);
	fprintf (f, "%i\n", savegame_compatibility.value ? VANILLA_SAVESTATE_VERSION : SAVESTATE_VERSION); // mankrip - serverflags bugfix
	Host_SavegameComment (comment);
	fprintf (f, "%s\n", comment);
	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);
	fprintf (f, "%d\n", current_skill);
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n",sv.time);
	
	if (!savegame_compatibility.value) // mankrip - serverflags bugfix
    fprintf (f, "%i\n", svs.serverflags); // mankrip - serverflags bugfix

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv.lightstyles[i])
			fprintf (f, "%s\n", sv.lightstyles[i]);
		else
			fprintf (f,"m\n");
	}


	ED_WriteGlobals (f);
	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ED_Write (f, EDICT_NUM(i));
		fflush (f);
	}
	fclose (f);

#ifdef FLASH_FILE_SYSTEM
	as3UpdateFileSharedObject(name);
#endif

	Con_Printf ("done.\n");
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time, tfloat;
	char	str[32768], *start;
	int		i, r;
	edict_t	*ent;
	int		entnum;
	int		version;
	float			spawn_parms[NUM_SPAWN_PARMS];

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("load <savename> : load a game\n");
		return;
	}

	cls.demonum = -1;		// stop demo loop in case this fails

	snprintf(name, sizeof(name),  "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension (name, ".sav");

// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

	Con_Printf ("Loading game from %s...\n", name);
	f = fopen (name, "r");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open save file for reading.\n");
		return;
	}

	/*
	fscanf (f, "%i\n", &version);
	if (version != SAVEGAME_VERSION)
	{
		fclose (f);
		Con_Printf ("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);

		return;
	}
	*/
	
	 fscanf (f, "%i\n", &version);
		if (version != SAVESTATE_VERSION && version != VANILLA_SAVESTATE_VERSION) // mankrip - serverflags bugfix
		{
		fclose (f);
		Con_Printf ("Savegame is version %i, not %i or %i\n", version, VANILLA_SAVESTATE_VERSION, SAVESTATE_VERSION); // mankrip - serverflags bugfix - edited
		return;
		}

	fscanf (f, "%s\n", str);
	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		fscanf (f, "%f\n", &spawn_parms[i]);

// this silliness is so we can load 1.06 save files, which have float skill values
	fscanf (f, "%f\n", &tfloat);
	current_skill = (int)(tfloat + 0.1);
	Cvar_SetValueByRef (&skill, (float)current_skill);

	fscanf (f, "%s\n",mapname);
	fscanf (f, "%f\n",&time);

   if (version != VANILLA_SAVESTATE_VERSION) // mankrip - serverflags bugfix
   fscanf (f, "%i\n", &svs.serverflags); // mankrip - serverflags bugfix

   CL_Disconnect_f ();
	/*
	fscanf (f, "%f\n",&time);

	CL_Disconnect_f ();
	*/

	SV_SpawnServer (mapname);

	if (!sv.active)
	{
		Con_Printf ("Couldn't load map\n");
		return;
	}
	sv.paused = true;		// pause until all clients connect
	sv.loadgame = true;

// load the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		fscanf (f, "%s\n", str);
		sv.lightstyles[i] = Hunk_Alloc (strlen(str)+1);
		strcpy (sv.lightstyles[i], str);
	}

// load the edicts out of the savegame file
	entnum = -1;		// -1 is the globals
	while (!feof(f))
	{
		for (i=0 ; i<sizeof(str)-1 ; i++)
		{
			r = fgetc (f);
			if (r == EOF || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i == sizeof(str)-1)
			Sys_Error ("Loadgame buffer overflow");
		str[i] = 0;
		start = str;
		start = COM_Parse(str);
		if (!com_token[0])
			break;		// end of file
		if (strcmp(com_token,"{"))
			Sys_Error ("First token isn't a brace");

		if (entnum == -1)
		{	// parse the global vars
			ED_ParseGlobals (start);
		}
		else
		{	// parse an edict

			ent = EDICT_NUM(entnum);
			memset (&ent->v, 0, progs->entityfields * 4);
			ent->free = false;
			ED_ParseEdict (start, ent);

		// link it into the bsp tree
			if (!ent->free)
				SV_LinkEdict (ent, false);
		}

		entnum++;
	}

	sv.num_edicts = entnum;
	sv.time = time;

	fclose (f);

	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		svs.clients->spawn_parms[i] = spawn_parms[i];

	if (cls.state != ca_dedicated)
	{
		CL_EstablishConnection ("local");
		Host_Reconnect_f ();
	}
}

//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f (void)
{
	char	*newName;
	int a, b, c;	// JPG 1.05 - ip address logging

	if (Cmd_Argc () == 1)
	{
		Con_Printf ("\"name\" is \"%s\"\n", cl_name.string);
		return;
	}
	if (Cmd_Argc () == 2)
		newName = Cmd_Argv(1);
	else
		newName = Cmd_Args();
	newName[15] = 0;

	// JPG 3.02 - remove bad characters
	for (a = 0 ; newName[a] ; a++)
	{
		if (newName[a] == 10)
			newName[a] = ' ';
		else if (newName[a] == 13)
			newName[a] += 128;
	}

	if (cmd_source == src_command)
	{
		if (strcmp(cl_name.string, newName) == 0)
			return;
		Cvar_SetStringByRef (&cl_name, newName);
		if (cls.state == ca_connected)
			Cmd_ForwardToServer_f ();
		return;
	}

	if (host_client->name[0] && strcmp(host_client->name, "unconnected") )
		if (strcmp(host_client->name, newName) != 0)
			Con_Printf ("%s renamed to %s\n", host_client->name, newName);

	strcpy (host_client->name, newName);
	host_client->edict->v.netname = host_client->name - pr_strings;
#ifdef PROQUAKE_EXTENSION
	// JPG 1.05 - log the IP address
	if (sscanf(host_client->netconnection->address, "%d.%d.%d", &a, &b, &c) == 3)
		IPLog_Add((a << 16) | (b << 8) | c, newName);

	// JPG 3.00 - prevent messages right after a colour/name change
	host_client->change_time = sv.time;
#endif
// send notification to all clients

	MSG_WriteByte (&sv.reliable_datagram, svc_updatename);
	MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteString (&sv.reliable_datagram, host_client->name);
}


void Host_Say(qboolean teamonly)
{
	client_t *client;
	client_t *save;
	int		j;
	char	*p;
	unsigned char	text[64];
	qboolean	fromServer = false;

	if (cmd_source == src_command)
	{
		if (cls.state == ca_dedicated)
		{
			fromServer = true;
			teamonly = false;
		}
		else
		{
			Cmd_ForwardToServer_f ();
			return;
		}
	}

	if (Cmd_Argc () < 2)
		return;

	save = host_client;

	p = Cmd_Args();

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}


// turn on color set 1
	if (!fromServer)
	{
#ifdef PROQUAKE_EXTENSION
		// R00k - dont allow new connecting players to spam obscenities...
		if (pq_connectmute.value && (net_time - host_client->netconnection->connecttime) < pq_connectmute.value)
			return;

		// JPG - spam protection
		if (sv.time - host_client->spam_time > pq_spam_rate.value * pq_spam_grace.value)
			host_client->spam_time = sv.time - pq_spam_rate.value * pq_spam_grace.value;
		host_client->spam_time += pq_spam_rate.value;
		if (host_client->spam_time > sv.time)
			return;

		// JPG 3.00 - don't allow messages right after a colour/name change
		if (pq_tempmute.value && sv.time - host_client->change_time < 1 && host_client->netconnection->mod != MOD_QSMACK)
			return;

		// JPG 3.11 - feature request from Slot Zero
		if (pq_showedict.value)
			Sys_Printf("#%d ", NUM_FOR_EDICT(host_client->edict));
		if (teamplay.value && teamonly) // JPG - added () for mm2
			snprintf(text, sizeof(text), "%c(%s): ", 1, save->name);
		else
#endif
		snprintf(text, sizeof(text),  "%c%s: ", 1, save->name);
#ifdef PROQUAKE_EXTENSION
		// JPG 3.20 - optionally remove '\r'
		if (pq_removecr.value)
		{
			char *ch;
			for (ch = p ; *ch ; ch++)
			{
				if (*ch == '\r')
					*ch += 128;
			}
		}
#endif
	}
	else
	{
		snprintf(text, sizeof(text),  "%c<%s> ", 1, hostname.string);
	}

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if (strlen(p) > j)
		p[j] = 0;

	strlcat (text, p, sizeof(text));
	strlcat (text, "\n", sizeof(text));

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned)
			continue;
		if (teamplay.value && teamonly && client->edict->v.team != save->edict->v.team)
			continue;
		host_client = client;
		SV_ClientPrintf("%s", text);
	}
	host_client = save;
#ifdef PROQUAKE_EXTENSION
	// JPG 3.20 - optionally write player binds to server log
	if (pq_logbinds.value)
		Con_Printf("%s", &text[1]);
	else
#endif
	Sys_Printf("%s", &text[1]);
}


void Host_Say_f(void)
{
	Host_Say(false);
}


void Host_Say_Team_f(void)
{
	Host_Say(true);
}


void Host_Tell_f(void)
{
	client_t *client;
	client_t *save;
	int		j;
	char	*p;
	char	text[64];

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}
#ifdef PROQUAKE_EXTENSION
	// JPG - disabled Tell (to prevent cheating)
	if (host_client->netconnection->mod != MOD_QSMACK)
	{
		SV_ClientPrintf("%cTell is diabled on this server\n", 1);
		return;
	}
#endif
	if (Cmd_Argc () < 3)
		return;

	strcpy(text, host_client->name);
	strlcat (text, ": ", sizeof(text));

	p = Cmd_Args();

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

// check length & truncate if necessary
	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if (strlen(p) > j)
		p[j] = 0;

	strlcat (text, p, sizeof(text));
	strlcat (text, "\n", sizeof(text));

	save = host_client;
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active || !client->spawned)
			continue;
		if (strcasecmp(client->name, Cmd_Argv(1)))
			continue;
		host_client = client;
		SV_ClientPrintf("%s", text);
		break;
	}
	host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f(void)
{
	int		top, bottom;
	int		playercolor;
	extern cvar_t sv_allcolors;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"color\" is \"%i %i\"\n", ((int)cl_color.value) >> 4, ((int)cl_color.value) & 0x0f);
		Con_Printf ("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		top = bottom = atoi(Cmd_Argv(1));
	else
	{
		top = atoi(Cmd_Argv(1));
		bottom = atoi(Cmd_Argv(2));
	}

	top &= 15;
	bottom &= 15;
	if (!sv_allcolors.value)
	{
	if (top > 13)
		top = 13;

	if (bottom > 13)
		bottom = 13;
	}

	playercolor = top*16 + bottom;

	if (cmd_source == src_command)
	{
		Cvar_SetValueByRef (&cl_color, playercolor);
		if (cls.state == ca_connected)
			Cmd_ForwardToServer_f ();
		return;
	}
#ifdef PROQUAKE_EXTENSION
	// JPG 3.11 - bail if the color isn't actually changing
	if (host_client->colors == playercolor)
		return;
#endif
	host_client->colors = playercolor;
	host_client->edict->v.team = bottom + 1;
#ifdef PROQUAKE_EXTENSION
// JPG 3.00 - prevent messages right after a colour/name change
	if (bottom)
		host_client->change_time = sv.time;
#endif
// send notification to all clients
	MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
	MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteByte (&sv.reliable_datagram, host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}

	if (sv_player->v.health <= 0)
	{
		SV_ClientPrintf ("Can't suicide -- already dead!\n");	// JPG 3.02 allready->already
		return;
	}

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(sv_player);
	PR_ExecuteProgram (pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f (void)
{
	cl.paused ^= 2;		// by joe: to handle demo-pause

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}

	if (!pausable.value)
		SV_ClientPrintf ("Pause not allowed.\n");
	else
	{
		sv.paused ^= 1;

		if (sv.paused)
		{
			SV_BroadcastPrintf ("%s paused the game\n", pr_strings + sv_player->v.netname);
		}
		else
		{
			SV_BroadcastPrintf ("%s unpaused the game\n",pr_strings + sv_player->v.netname);
		}

	// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_setpause);
		MSG_WriteByte (&sv.reliable_datagram, sv.paused);
	}
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f (void)
{
	if (cmd_source == src_command)
	{
		Con_Printf ("prespawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf ("prespawn not valid -- already spawned\n");	// JPG 3.02 allready->already
		return;
	}

	SZ_Write (&host_client->message, sv.signon.data, sv.signon.cursize);
	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 2);
	host_client->sendsignon = true;
#ifdef PROQUAKE_EXTENSION
	host_client->netconnection->encrypt = 2; // JPG 3.50
#endif
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f (void)
{
	int		i;
	client_t	*client;
	edict_t	*ent;

	if (cmd_source == src_command)
	{
		Con_Printf ("spawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf ("Spawn not valid -- already spawned\n");	// JPG 3.02 allready->already
		return;
	}
#ifdef PROQUAKE_EXTENSION
	// JPG 3.20 - model and exe checking
	host_client->nomap = false;
	if (pq_cheatfree && host_client->netconnection->mod != MOD_QSMACK)
	{
		int i;
		unsigned long crc;
		unsigned a, b;

		a = MSG_ReadLong();
		b = MSG_ReadLong();

		if (!Security_Verify(a, b))
		{
			MSG_WriteByte(&host_client->message, svc_print);
			MSG_WriteString(&host_client->message, "Invalid executable\n");
			Con_Printf("%s (%s) connected with an invalid executable\n", host_client->name, host_client->netconnection->address);
			SV_DropClient(false);
			return;
		}

		for (i = 1 ; sv.model_precache[i] ; i++)
		{
			if (sv.model_precache[i][0] != '*')
			{
				crc = MSG_ReadLong();
				if (crc != sv.model_crc[i])
				{
					if (i == 1 && crc == 0)	// allow clients to connect if they don't have the map
					{
						Con_Printf("%s does not have map %s\n", host_client->name, sv.model_precache[1]);
						host_client->nomap = true;
						break;
					}
					else
					{
						MSG_WriteByte(&host_client->message, svc_print);
						MSG_WriteString(&host_client->message, va("%s is invalid\n", sv.model_precache[i]));
						Con_Printf("%s (%s) connected with an invalid %s\n", host_client->name, host_client->netconnection->address, sv.model_precache[i]);
						SV_DropClient(false);
						return;
					}
				}
			}
		}
	}
#endif
// run the entrance script
	if (sv.loadgame)
	{	// loaded games are fully inited already
		// if this is the last client to be connected, unpause
		sv.paused = false;
	}
	else
	{
		// set up the edict
		ent = host_client->edict;

		memset (&ent->v, 0, progs->entityfields * 4);
		ent->v.colormap = NUM_FOR_EDICT(ent);
		ent->v.team = (host_client->colors & 15) + 1;
		ent->v.netname = host_client->name - pr_strings;

		// copy spawn parms out of the client_t

		for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
			(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

		// call the spawn function

		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram (pr_global_struct->ClientConnect);

		if ((Sys_DoubleTime() - host_client->netconnection->connecttime) <= sv.time)
			Sys_Printf ("%s entered the game\n", host_client->name);

		PR_ExecuteProgram (pr_global_struct->PutClientInServer);
	}


// send all current names, colors, and frag counts
	SZ_Clear (&host_client->message);

// send time of update
	MSG_WriteByte (&host_client->message, svc_time);
	MSG_WriteFloat (&host_client->message, sv.time);

	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		MSG_WriteByte (&host_client->message, svc_updatename);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteString (&host_client->message, client->name);
		MSG_WriteByte (&host_client->message, svc_updatefrags);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteShort (&host_client->message, client->old_frags);
		MSG_WriteByte (&host_client->message, svc_updatecolors);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteByte (&host_client->message, client->colors);
	}

// send all current light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		MSG_WriteByte (&host_client->message, svc_lightstyle);
		MSG_WriteByte (&host_client->message, (char)i);
		MSG_WriteString (&host_client->message, sv.lightstyles[i]);
	}

// send some stats
	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALSECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALMONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_monsters);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_SECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->found_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_MONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->killed_monsters);


//
// send a fixangle
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = EDICT_NUM( 1 + (host_client - svs.clients) );
	MSG_WriteByte (&host_client->message, svc_setangle);
	for (i=0 ; i < 2 ; i++)
		MSG_WriteAngle (&host_client->message, ent->v.angles[i] );
	MSG_WriteAngle (&host_client->message, 0 );

	SV_WriteClientdataToMessage (sv_player, &host_client->message);

	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 3);
	host_client->sendsignon = true;
#ifdef PROQUAKE_EXTENSION
	// JPG - added this for spam protection
	host_client->spam_time = 0;
#endif
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f (void)
{
	if (cmd_source == src_command)
	{
		Con_Printf ("begin is not valid from the console\n");
		return;
	}

	host_client->spawned = true;
#ifdef PROQUAKE_EXTENSION
	host_client->netconnection->encrypt = 0;	// JPG 3.50
#endif
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f (void)
{
	char		*who;
	char		*message = NULL;
	client_t	*save;
	int			i;
	qboolean	byNumber = false;

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer_f ();
			return;
		}
	}
	else if (pr_global_struct->deathmatch)
		return;

	save = host_client;

	if (Cmd_Argc() > 2 && strcmp(Cmd_Argv(1), "#") == 0)
	{
		i = atof(Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.maxclients)
			return;
		if (!svs.clients[i].active)
			return;
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active)
				continue;
			if (strcasecmp(host_client->name, Cmd_Argv(1)) == 0)
				break;
		}
	}

	if (i < svs.maxclients)
	{
		if (cmd_source == src_command)
			if (cls.state == ca_dedicated)
				who = "Console";
			else
				who = cl_name.string;
		else
			who = save->name;

		// can't kick yourself!
		if (host_client == save)
			return;

		if (Cmd_Argc() > 2)
		{
			message = COM_Parse(Cmd_Args());
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += strlen(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			SV_ClientPrintf ("Kicked by %s: %s\n", who, message);
		else
			SV_ClientPrintf ("Kicked by %s\n", who);
		SV_DropClient (false);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/
void Host_Give_f (void)
{
	char	*t;
	int		v;
	eval_t	*val;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer_f ();
		return;
	}

	if (pr_global_struct->deathmatch)
		return;

	t = Cmd_Argv(1);
	v = atoi (Cmd_Argv(2));

	switch (t[0])
	{
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':

      // MED 01/04/97 added hipnotic give stuff
      if (hipnotic)
      {
         if (t[0] == '6')
         {
            if (t[1] == 'a')
               sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
            else
               sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
         }
         else if (t[0] == '9')
            sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
         else if (t[0] == '0')
            sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
         else if (t[0] >= '2')
            sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
      }
      else
      {
         if (t[0] >= '2')
            sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
      }
		break;

    case 's':
		if (rogue)
		{
		    if ((val = GETEDICTFIELDVALUE(sv_player, eval_ammo_shells1)))
			    val->_float = v;
		}

        sv_player->v.ammo_shells = v;
        break;

    case 'n':
		if (rogue)
		{
			if ((val = GETEDICTFIELDVALUE(sv_player, eval_ammo_nails1)))
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
		else
		{
			sv_player->v.ammo_nails = v;
		}
        break;

    case 'l':
		if (rogue)
		{
			if ((val = GETEDICTFIELDVALUE(sv_player, eval_ammo_lava_nails)))
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
        break;

    case 'r':
		if (rogue)
		{
			if ((val = GETEDICTFIELDVALUE(sv_player, eval_ammo_rockets1)))
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
		else
		{
			sv_player->v.ammo_rockets = v;
		}
        break;

    case 'm':
		if (rogue)
		{
			if ((val = GETEDICTFIELDVALUE(sv_player, eval_ammo_multi_rockets)))
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
        break;

    case 'h':
        sv_player->v.health = v;
        break;

    case 'c':
		if (rogue)
		{
			if ((val = GETEDICTFIELDVALUE(sv_player, eval_ammo_cells1)))
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
		else
		{
			sv_player->v.ammo_cells = v;
		}
        break;

    case 'p':
		if (rogue)
		{
			if ((val = GETEDICTFIELDVALUE(sv_player, eval_ammo_plasma)))
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
        break;
	// Baker 3.60 - give "a" for armor from FitzQuake
	//johnfitz -- give armour
    case 'a':
		if (v >= 0 && v <= 100)
		{
			sv_player->v.armortype = 0.3;
	        sv_player->v.armorvalue = v;
			sv_player->v.items = sv_player->v.items - ((int)(sv_player->v.items) & (int)(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) + IT_ARMOR1;
		}
		if (v > 100 && v <= 150)
		{
			sv_player->v.armortype = 0.6;
	        sv_player->v.armorvalue = v;
			sv_player->v.items = sv_player->v.items - ((int)(sv_player->v.items) & (int)(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) + IT_ARMOR2;
		}
		if (v > 150 && v <= 200)
		{
			sv_player->v.armortype = 0.8;
	        sv_player->v.armorvalue = v;
			sv_player->v.items = sv_player->v.items - ((int)(sv_player->v.items) & (int)(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) + IT_ARMOR3;
		}
		break;
	//johnfitz
    }
}

edict_t	*FindViewthing (void)
{
	int		i;
	edict_t	*e;

	for (i=0 ; i<sv.num_edicts ; i++)
	{
		e = EDICT_NUM(i);
		if ( !strcmp (pr_strings + e->v.classname, "viewthing") )
			return e;
	}
	Con_Printf ("No viewthing on map\n");
	return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f (void)
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();
	if (!e)
		return;

	m = Mod_ForName (Cmd_Argv(1), false);
	if (!m)
	{
		Con_Printf ("Can't load %s\n", Cmd_Argv(1));
		return;
	}

	e->v.frame = 0;
	cl.model_precache[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f (void)
{
	edict_t	*e;
	int		f;
	model_t	*m;

	e = FindViewthing ();
	if (!e)
		return;
	m = cl.model_precache[(int)e->v.modelindex];

	f = atoi(Cmd_Argv(1));
	if (f >= m->numframes)
		f = m->numframes-1;

	e->v.frame = f;
}


void PrintFrameName (model_t *m, int frame)
{
	aliashdr_t 			*hdr;
	maliasframedesc_t	*pframedesc;

	hdr = (aliashdr_t *)Mod_Extradata (m);
	if (!hdr)
		return;
	pframedesc = &hdr->frames[frame];

	Con_Printf ("frame %i: %s\n", frame, pframedesc->name);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f (void)
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();
	if (!e)
		return;
	m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame + 1;
	if (e->v.frame >= m->numframes)
		e->v.frame = m->numframes - 1;

	PrintFrameName (m, e->v.frame);
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f (void)
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();
	if (!e)
		return;

	m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame - 1;
	if (e->v.frame < 0)
		e->v.frame = 0;

	PrintFrameName (m, e->v.frame);
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f (void)
{
	int		i, c;

#ifdef SUPPORTS_DEMO_AUTOPLAY
	if (nostartdemos)
	{
		// Baker 3.76
		// This is hack to prevent demo auto-play from having demos in queue play
		// after a file associated demo plays
		return;
	}
#endif

	if (cls.state == ca_dedicated)
	{
		if (!sv.active)
			Cbuf_AddText ("map start\n");
		return;
	}

	// If no params ... clear the queue and set next demo to nothing
	if (Cmd_Argc() == 1) {
		Con_Printf("Demo queue cleared\n");

		for (i=1;i <= MAX_DEMOS;i++)	// Clear demo loop queue
			cls.demos[i-1][0] = 0;
		cls.demonum = -1;				// Set next demo to none

		return;
	}


#if 0 // QView connecting plays demos so this is out
	if (sv.active || cls.demoplayback)
	{
#if 0	// Baker: no this is annoying if you use +map xxxx in the command line
		Con_Printf("Disconnect before playing demo loop\n");
#endif
		return;
	}
#endif

	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Con_Printf ("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Con_DPrintf ("%i demo(s) in loop\n", c);

	for (i=1 ; i<c+1 ; i++)
		strncpy (cls.demos[i-1], Cmd_Argv(i), sizeof(cls.demos[0])-1);

	// LordHavoc: clear the remaining slots
	for (;i <= MAX_DEMOS;i++)
		cls.demos[i-1][0] = 0;

	if (!sv.active && cls.demonum != -1 && !cls.demoplayback)
	{
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
		cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f (void)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demonum == -1)
		cls.demonum = 1;

	CL_Disconnect_f ();
	CL_NextDemo ();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f (void)
{
	if (cls.state == ca_dedicated)
		return;
	if (!cls.demoplayback)
		return;

	CL_StopPlayback ();
	CL_Disconnect ();
}

#ifdef PROQUAKE_EXTENSION
/*
===============================================================================

PROQUAKE FUNCTIONS (JPG 1.05)

===============================================================================
*/

// used to translate to non-fun characters for identify <name>
char unfun[129] =
"................[]olzeasbt89...."
"........[]......olzeasbt89..[.]."
"aabcdefghijklmnopqrstuvwxyz[.].."
".abcdefghijklmnopqrstuvwxyz[.]..";

// try to find s1 inside of s2
int unfun_match (char *s1, char *s2)
{
	int i;
	for ( ; *s2 ; s2++)
	{
		for (i = 0 ; s1[i] ; i++)
		{
			if (unfun[s1[i] & 127] != unfun[s2[i] & 127])
				break;
		}
		if (!s1[i])
			return true;
	}
	return false;
}

/* JPG 1.05
==================
Host_Identify_f

Print all known names for the specified player's ip address
==================
*/
void Host_Identify_f (void)
{
	int i;
	int a, b, c;
	char name[16];

	if (!iplog_size)
	{
		Con_Printf("IP logging not available\nRemove -noiplog command line option\n"); // Baker 3.83: Now -iplog is the default
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Con_Printf("usage: identify <player number or name>\n");
		return;
	}
	if (sscanf(Cmd_Argv(1), "%d.%d.%d", &a, &b, &c) == 3)
	{
		Con_Printf("known aliases for %d.%d.%d:\n", a, b, c);
		IPLog_Identify((a << 16) | (b << 8) | c);
		return;
	}

	i = atoi(Cmd_Argv(1)) - 1;
	if (i == -1)
	{
		if (sv.active)
		{
			for (i = 0 ; i < svs.maxclients ; i++)
			{
				if (svs.clients[i].active && unfun_match(Cmd_Argv(1), svs.clients[i].name))
					break;
			}
		}
		else
		{
			for (i = 0 ; i < cl.maxclients ; i++)
			{
				if (unfun_match(Cmd_Argv(1), cl.scores[i].name))
					break;
			}
		}
	}
	if (sv.active)
	{
		if (i < 0 || i >= svs.maxclients || !svs.clients[i].active)
		{
			Con_Printf("No such player\n");
			return;
		}
		if (sscanf(svs.clients[i].netconnection->address, "%d.%d.%d", &a, &b, &c) != 3)
		{
			Con_Printf("Could not determine IP information for %s\n", svs.clients[i].name);
			return;
		}
		strncpy(name, svs.clients[i].name, 15);
		name[15] = 0;
		Con_Printf("known aliases for %s:\n", name);
		IPLog_Identify((a << 16) | (b << 8) | c);
	}
	else
	{
		if (i < 0 || i >= cl.maxclients || !cl.scores[i].name[0])
		{
			Con_Printf("No such player\n");
			return;
		}
		if (!cl.scores[i].addr)
		{
			Con_Printf("No IP information for %.15s\nUse 'status'\n", cl.scores[i].name);
			return;
		}
		strncpy(name, cl.scores[i].name, 15);
		name[15] = 0;
		Con_Printf("known aliases for %s:\n", name);
		IPLog_Identify(cl.scores[i].addr);
	}
}
#endif

//=============================================================================

#ifdef PROQUAKE_EXTENSION
// JPG SAY_RAND
int num_rand[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int next_rand[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char msg_rand[10][64][64];
char msg_order[10][64];
char cmd_rand[10][10] =
{
	"say_rand0",
	"say_rand1",
	"say_rand2",
	"say_rand3",
	"say_rand4",
	"say_rand5",
	"say_rand6",
	"say_rand7",
	"say_rand8",
	"say_rand9"
};

void Host_Say_Rand_f (void)
{
	int i, j, k, t;

	if (sscanf(Cmd_Argv(0), "say_rand%d", &k))
	{
		if (num_rand[k] && cls.state == ca_connected)
		{
			if (!next_rand[k])
			{
				for (i = 0 ; i < num_rand[k] ; i++)
					msg_order[k][i] = i;
				for (i = 0 ; i < num_rand[k] - 1 ; i++)
				{
					j = (rand() % (num_rand[k] - i)) + i;
					t = msg_order[k][j];
					msg_order[k][j] = msg_order[k][i];
					msg_order[k][i] = t;
				}
			}

			MSG_WriteByte (&cls.message, clc_stringcmd);
			SZ_Print (&cls.message, "say ");
			SZ_Print (&cls.message, msg_rand[k][msg_order[k][next_rand[k]]]);
			if (++next_rand[k] == num_rand[k])
				next_rand[k] = 0;
		}
	}
}
#endif

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands (void)
{
#ifdef PROQUAKE_EXTENSION
	// JPG - SAY_RAND
	int i;
	FILE *f;
	for (i = 0 ; i < 10 ; i++)
	{
		f = fopen(va("%s/msgrand%d.txt", com_gamedir, i), "r");
		if (f)
		{
			Cmd_AddCommand (cmd_rand[i], Host_Say_Rand_f);
			num_rand[i] = 0;
			while (fgets(msg_rand[i][num_rand[i]], 64, f))
			{
				char *ch = strchr(msg_rand[i][num_rand[i]], '\n');
				if (ch)
					*ch = 0;
				if (msg_rand[i][num_rand[i]][0])
					num_rand[i]++;
			}
			fclose(f);
		}
	}
	Cmd_AddCommand ("cheatfree", Host_Cheatfree_f);	// JPG 3.50 - print cheat-free status
#endif
	Cmd_AddCommand ("status", Host_Status_f);
#ifdef SUPPORTS_GAMEDIR_SWITCHING
	Cmd_AddCommand ("gamedir", Host_Game_f); //johnfitz
#endif

	Cmd_AddCommand ("quit", Host_Quit_f);
	Cmd_AddCommand ("god", Host_God_f);
	Cmd_AddCommand ("notarget", Host_Notarget_f);
	Cmd_AddCommand ("fly", Host_Fly_f);
	Cmd_AddCommand ("map", Host_Map_f);
	Cmd_AddCommand ("restart", Host_Restart_f);
	Cmd_AddCommand ("changelevel", Host_Changelevel_f);
	Cmd_AddCommand ("connect", Host_Connect_f);
	Cmd_AddCommand ("reconnect", Host_Reconnect_f);
	Cmd_AddCommand ("name", Host_Name_f);
	Cmd_AddCommand ("noclip", Host_Noclip_f);
	Cmd_AddCommand ("version", Host_Version_f);
#ifdef IDGODS
	Cmd_AddCommand ("please", Host_Please_f);
#endif
	Cmd_AddCommand ("say", Host_Say_f);
	Cmd_AddCommand ("say_team", Host_Say_Team_f);
	Cmd_AddCommand ("tell", Host_Tell_f);
	Cmd_AddCommand ("color", Host_Color_f);
	Cmd_AddCommand ("kill", Host_Kill_f);
	Cmd_AddCommand ("pause", Host_Pause_f);
	Cmd_AddCommand ("spawn", Host_Spawn_f);
	Cmd_AddCommand ("begin", Host_Begin_f);
	Cmd_AddCommand ("prespawn", Host_PreSpawn_f);
	Cmd_AddCommand ("kick", Host_Kick_f);
	Cmd_AddCommand ("ping", Host_Ping_f);
	Cmd_AddCommand ("load", Host_Loadgame_f);
	Cmd_AddCommand ("save", Host_Savegame_f);
	Cmd_AddCommand ("give", Host_Give_f);

	Cmd_AddCommand ("startdemos", Host_Startdemos_f);
	Cmd_AddCommand ("demos", Host_Demos_f);
	Cmd_AddCommand ("stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand ("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand ("viewframe", Host_Viewframe_f);
	Cmd_AddCommand ("viewnext", Host_Viewnext_f);
	Cmd_AddCommand ("viewprev", Host_Viewprev_f);

	Cmd_AddCommand ("mcache", Mod_Print_f);

    Cmd_AddCommand ("vl_changeangle", VLight_ChangeLightAngle_f);
    Cmd_AddCommand ("vl_dumplight", VLight_DumpLightTable_f);

#ifdef QCEXEC
	Cmd_AddCommand ("qcexec", Host_QC_Exec);
#endif

#ifdef PROQUAKE_EXTENSION
	Cmd_AddCommand ("identify", Host_Identify_f);	// JPG 1.05 - player IP logging
	Cmd_AddCommand ("ipdump", IPLog_Dump_f);			// JPG 1.05 - player IP logging
	Cmd_AddCommand ("ipmerge", IPLog_Import_f);		// JPG 3.00 - import an IP data file
#endif
	Cvar_RegisterVariable (&cl_confirmquit, NULL); // Baker 3.60
	Cvar_RegisterVariable (&savegame_compatibility, NULL); // mankrip - serverflags bugfix

	Cvar_RegisterVariable (&host_mapname, NULL);	// Baker: 5.51 -- really this needs to be registered elsewhere ... sv cannot see this cvar.   Maybe make it "host_mapname?"
}
