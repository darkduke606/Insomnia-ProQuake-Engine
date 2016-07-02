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
// sv_main.c -- server main program

#include "quakedef.h"
#ifdef PROQUAKE_EXTENSION
#include <time.h> // JPG - needed for console log
#endif

server_t		sv;
server_static_t	svs;

cvar_t  sv_progs = {"sv_progs", "progs.dat" };

cvar_t	sv_defaultmap = {"sv_defaultmap",""}; //Baker 3.95: R00k
cvar_t	sv_ipmasking = {"sv_ipmasking","1",false, true}; //Baker 3.95: R00k
#ifdef ANTIWALLHACK_SERVER
cvar_t	sv_cullentities = {"sv_cullentities", "1", false, true}; // Baker 3.99c: Rook and I both think sv_cullentities is better cvar name than sv_cullplayers
cvar_t	sv_cullentities_notify = {"sv_cullentities_notify", "0", false, true}; // in event there are multiple modes for anti-wallhack (Rook has a more comprehensive mode)
#endif
//cvar_t 	sv_gameplayfix_monster_lerp = {"sv_gameplayfix_monster_lerp", "0", false, true}; // Baker: No "Not lerping" monsters
cvar_t	sv_allcolors = {"sv_allcolors", "1", false, true};

char	localmodels[MAX_MODELS][5];			// inline model names for precache

//============================================================================

/*
===============
SV_Init
===============
*/
void SV_Init (void)
{
	int		i;
#ifdef PROQUAKE_EXTENSION
	extern  cvar_t  pq_connectmute;
#endif
	if(!cl.gametype == GAME_DEATHMATCH)
	{
	extern	cvar_t	sv_maxvelocity;

	extern	cvar_t	sv_gravity;
	extern	cvar_t	sv_nostep;
	extern	cvar_t	sv_friction;
	extern	cvar_t	sv_edgefriction;
	extern	cvar_t	sv_stopspeed;
	extern	cvar_t	sv_maxspeed;
	extern	cvar_t	sv_accelerate;
	extern	cvar_t	sv_idealpitchscale;
	extern	cvar_t	sv_aim;
	}
	extern	cvar_t	sv_altnoclip; //johnfitz

	Cvar_RegisterVariable (&sv_maxvelocity, NULL);
	Cvar_RegisterVariable (&sv_gravity, NULL);
	Cvar_RegisterVariable (&sv_friction, NULL);
	Cvar_RegisterVariable (&sv_edgefriction, NULL);
	Cvar_RegisterVariable (&sv_stopspeed, NULL);
	Cvar_RegisterVariable (&sv_maxspeed, NULL);
	Cvar_RegisterVariable (&sv_accelerate, NULL);
	Cvar_RegisterVariable (&sv_idealpitchscale, NULL);
	Cvar_RegisterVariable (&sv_aim, NULL);
	Cvar_RegisterVariable (&sv_nostep, NULL);
	Cvar_RegisterVariable (&sv_altnoclip, NULL); //johnfitz
#ifdef PROQUAKE_EXTENSION
	Cvar_RegisterVariable (&pq_fullpitch, NULL);	// JPG 2.01
#endif
#ifdef ANTIWALLHACK_SERVER
	Cvar_RegisterVariable (&sv_cullentities, NULL);
	Cvar_RegisterVariable (&sv_cullentities_notify, NULL);
#endif
	Cvar_RegisterVariable (&sv_defaultmap, NULL); //Baker 3.99b: R00k create a default map to load
	Cvar_RegisterVariable (&sv_ipmasking, NULL);

	Cvar_RegisterVariable (&sv_progs, NULL);

//	Cvar_RegisterVariable (&sv_gameplayfix_monster_lerp, NULL);
	Cvar_RegisterVariable (&sv_allcolors, NULL);

#ifdef PROQUAKE_EXTENSION
	// Baker: Dedicated server "defaults" - this is ok because quake.rc is executed later, so these "defaults" won't override config.cfg settings, etc.
	if (COM_CheckParm ("-dedicated")) {
//		Cvar_SetStringByRef("sv_defaultmap", "start"); // Baker 3.99b: "Default" it to start if dedicated server.
		Cvar_SetValueByRef(&pq_connectmute, 3); 	// Baker 3.99g: "Default" it to 10 seconds
	}
#endif

	for (i=0 ; i<MAX_MODELS ; i++)
		snprintf(localmodels[i], sizeof(localmodels[i]), "*%i", i);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count)
{
	int		i, v;

	if (sv.datagram.cursize > MAX_DATAGRAM-16)
		return;
	MSG_WriteByte (&sv.datagram, svc_particle);
	MSG_WriteCoord (&sv.datagram, org[0]);
	MSG_WriteCoord (&sv.datagram, org[1]);
	MSG_WriteCoord (&sv.datagram, org[2]);
	for (i=0 ; i<3 ; i++)
	{
		v = dir[i]*16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		MSG_WriteChar (&sv.datagram, v);
	}
	MSG_WriteByte (&sv.datagram, count);
	MSG_WriteByte (&sv.datagram, color);
}

/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)
==================
*/
void SV_StartSound (edict_t *entity, int channel, char *sample, int volume, float attenuation)
{
	int         sound_num, field_mask, i, ent;

	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	if (sv.datagram.cursize > MAX_DATAGRAM-16)
		return;

// find precache number for sound
    for (sound_num=1 ; sound_num<MAX_SOUNDS && sv.sound_precache[sound_num] ; sound_num++)
        if (!strcmp(sample, sv.sound_precache[sound_num]))
            break;

    if ( sound_num == MAX_SOUNDS || !sv.sound_precache[sound_num] )
    {
    	if (mod_nosoundwarn) // DPrint it instead
        	Con_DPrintf ("SV_StartSound: %s not precacheed\n", sample);
        else
        Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
        return;
    }

	ent = NUM_FOR_EDICT(entity);

	channel = (ent<<3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;

// directed messages go only to the entity the are targeted on
	MSG_WriteByte (&sv.datagram, svc_sound);
	MSG_WriteByte (&sv.datagram, field_mask);
	if (field_mask & SND_VOLUME)
		MSG_WriteByte (&sv.datagram, volume);
	if (field_mask & SND_ATTENUATION)
		MSG_WriteByte (&sv.datagram, attenuation*64);
	MSG_WriteShort (&sv.datagram, channel);
	MSG_WriteByte (&sv.datagram, sound_num);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord (&sv.datagram, entity->v.origin[i]+0.5*(entity->v.mins[i]+entity->v.maxs[i]));
}

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo (client_t *client)
{
	char			**s, message[2048];

	// JPG - This used to be VERSION 1.09 SERVER (xxxxx CRC)
	MSG_WriteByte (&client->message, svc_print);
	snprintf(message, sizeof(message), "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n"
					  "\n   \01\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\03");
	MSG_WriteString (&client->message,message);
	MSG_WriteByte (&client->message, svc_print);
	snprintf(message, sizeof(message), "\02\n   \04ProQuake Server Version %4.2f\06"
					  "\n   \07\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\11", PROQUAKE_SERIES_VERSION);
	MSG_WriteString (&client->message,message);

	MSG_WriteByte (&client->message, svc_serverinfo);
	MSG_WriteLong (&client->message, PROTOCOL_NETQUAKE);
	MSG_WriteByte (&client->message, svs.maxclients);

	if (!coop.value && deathmatch.value)
		MSG_WriteByte (&client->message, GAME_DEATHMATCH);
	else
		MSG_WriteByte (&client->message, GAME_COOP);

	snprintf(message, sizeof(message), pr_strings+sv.edicts->v.message);

	MSG_WriteString (&client->message,message);

	for (s = sv.model_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

	for (s = sv.sound_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

// send music
	MSG_WriteByte (&client->message, svc_cdtrack);
	MSG_WriteByte (&client->message, sv.edicts->v.sounds);
	MSG_WriteByte (&client->message, sv.edicts->v.sounds);

// set view
	MSG_WriteByte (&client->message, svc_setview);
	MSG_WriteShort (&client->message, NUM_FOR_EDICT(client->edict));
#ifdef PROQUAKE_EXTENSION
	// JPG 2.01 - server side fullpitch fix
	if (!pq_fullpitch.value && client->netconnection->mod != MOD_QSMACK)
	{
		MSG_WriteByte (&client->message, svc_stufftext);
		MSG_WriteString (&client->message, "pq_fullpitch 0; cl_fullpitch 0\n");
	}
#endif
	MSG_WriteByte (&client->message, svc_signonnum);
	MSG_WriteByte (&client->message, 1);

	client->sendsignon = true;
	client->spawned = false;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient (int clientnum)
{
	int				i, edictnum;
	float			spawn_parms[NUM_SPAWN_PARMS];
	edict_t			*ent;
	client_t		*client;
	struct qsocket_s *netconnection;

	client = svs.clients + clientnum;
#ifdef PROQUAKE_EXTENSION
	// JPG - added ProQuake dprint and QSmack print
	if (client->netconnection->mod == MOD_PROQUAKE)
		Con_DPrintf ("ProQuake Client %s connected\n", client->netconnection->address);
	else if (client->netconnection->mod == MOD_QSMACK)
		Con_Printf ("QSmack Client %s connected\n", client->netconnection->address);
	else
#endif
	Con_DPrintf ("Client %s connected\n", client->netconnection->address);

	edictnum = clientnum+1;

	ent = EDICT_NUM(edictnum);

// set up the client_t
	netconnection = client->netconnection;

	if (sv.loadgame)
		memcpy (spawn_parms, client->spawn_parms, sizeof(spawn_parms));
	memset (client, 0, sizeof(*client));
	client->netconnection = netconnection;

	strcpy (client->name, "unconnected");
	client->active = true;
	client->spawned = false;
	client->edict = ent;
	client->message.data = client->msgbuf;
	client->message.maxsize = sizeof(client->msgbuf);
	client->message.allowoverflow = true;		// we can catch it
	client->privileged = false;


	if (sv.loadgame)
	{
		memcpy (client->spawn_parms, spawn_parms, sizeof(spawn_parms));
	}
	else
	{
	// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram (pr_global_struct->SetNewParms);
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
	}

	SV_SendServerinfo (client);
}


/*
===================
SV_CheckForNewClients
===================
*/
void SV_CheckForNewClients (void)
{
	struct qsocket_s	*ret;
	int				i;

// check for new connections
	while (1)
	{
		if (!(ret = NET_CheckNewConnections ()))
			break;

	// init a new client structure
		for (i=0 ; i<svs.maxclients ; i++)
			if (!svs.clients[i].active)
				break;
		if (i == svs.maxclients)
			Sys_Error ("SV_CheckForNewClients: no free clients");

		svs.clients[i].netconnection = ret;
		SV_ConnectClient (i);
#ifdef PROQUAKE_EXTENSION
		if (ret->mod == MOD_QSMACK)
			qsmackActive = true;
#endif
		net_activeconnections++;
	}
}


/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void SV_ClearDatagram (void)
{
	SZ_Clear (&sv.datagram);
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
byte	fatpvs[MAX_MAP_LEAFS/8];
void SV_AddToFatPVS (vec3_t org, mnode_t *node, model_t *worldmodel)
{
	int		i;
	byte	*pvs;
	mplane_t	*plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = Mod_LeafPVS ( (mleaf_t *)node, worldmodel);
				for (i=0 ; i<fatbytes ; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}

		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_AddToFatPVS (org, node->children[0], worldmodel);
			node = node->children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte *SV_FatPVS (vec3_t org, model_t *worldmodel)
{
	fatbytes = (worldmodel->numleafs+31)>>3;
	memset (fatpvs, 0, fatbytes);
	SV_AddToFatPVS (org, worldmodel->nodes, worldmodel);
	return fatpvs;
}

#ifdef SUPPORTS_ANTIWALLHACK_SERVER
/*
====================
Q1_HullPointContents

====================
*/
static int Q1_HullPointContents (hull_t *hull, int num, vec3_t p)
{
	float		d;
#ifdef FITZQUAKE_PROTOCOL
	mclipnode_t *node; //johnfitz -- was dclipnode_t
#else
	dclipnode_t	*node;
#endif
	mplane_t	*plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			Sys_Error ("SV_HullPointContents: bad node number");

		node = hull->clipnodes + num;
		plane = hull->planes + node->planenum;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	return num;
}

#define	DIST_EPSILON	(0.03125)
#define	Q1CONTENTS_EMPTY	-1
#define	Q1CONTENTS_SOLID	-2
#define	Q1CONTENTS_WATER	-3
#define	Q1CONTENTS_SLIME	-4
#define	Q1CONTENTS_LAVA		-5
#define	Q1CONTENTS_SKY		-6
qboolean Q1BSP_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
#ifdef FITZQUAKE_PROTOCOL
	mclipnode_t *node; //johnfitz -- was dclipnode_t
#else
	dclipnode_t	*node;
#endif
	mplane_t	*plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

// check for empty
	if (num < 0)
	{
		if (num != Q1CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == Q1CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
		}
		else
			trace->startsolid = true;
		return true;		// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
		Sys_Error ("Q1BSP_RecursiveHullCheck: bad node number");

//
// find the point distances
//
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
	}

#if 1
	if (t1 >= 0 && t2 >= 0)
		return Q1BSP_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if (t1 < 0 && t2 < 0)
		return Q1BSP_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#else
	if ( (t1 >= DIST_EPSILON && t2 >= DIST_EPSILON) || (t2 > t1 && t1 >= 0) )
		return Q1BSP_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if ( (t1 <= -DIST_EPSILON && t2 <= -DIST_EPSILON) || (t2 < t1 && t1 <= 0) )
		return Q1BSP_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#endif

// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0)
		frac = (t1 + DIST_EPSILON)/(t1-t2);
	else
		frac = (t1 - DIST_EPSILON)/(t1-t2);
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	side = (t1 < 0);

// move up to the node
	if (!Q1BSP_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace) )
		return false;

#ifdef PARANOID
	if (Q1BSP_RecursiveHullCheck (sv_hullmodel, mid, node->children[side])
	== Q1CONTENTS_SOLID)
	{
		Con_Printf ("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (Q1_HullPointContents (hull, node->children[side^1], mid)
	!= Q1CONTENTS_SOLID)
// go past the node
		return Q1BSP_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);

	if (trace->allsolid)
		return false;		// never got out of the solid area

//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy (plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorNegate (plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (Q1_HullPointContents (hull, hull->firstclipnode, mid)
	== Q1CONTENTS_SOLID)
	{ // shouldn't really happen, but does occasionally
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy (mid, trace->endpos);
			Con_DPrintf ("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f)*frac;
		for (i=0 ; i<3 ; i++)
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
	}

	trace->fraction = midf;
	VectorCopy (mid, trace->endpos);

	return false;
}

//=============================================================================

qboolean Q1BSP_Trace(model_t *model, int forcehullnum, int frame, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, trace_t *trace)
{
	hull_t *hull;
	vec3_t size;
	vec3_t start_l, end_l;
	vec3_t offset;

	memset (trace, 0, sizeof(trace_t));
	trace->fraction = 1;
	trace->allsolid = true;

	VectorSubtract (maxs, mins, size);
	if (forcehullnum >= 1 && forcehullnum <= MAX_MAP_HULLS && model->hulls[forcehullnum-1].available)
		hull = &model->hulls[forcehullnum-1];
	else
	{
		if (model->hulls[5].available)
		{	//choose based on hexen2 sizes.

			if (size[0] < 3) // Point
				hull = &model->hulls[0];
			else if (size[0] <= 32 && size[2] <= 28)  // Half Player
				hull = &model->hulls[3];
			else if (size[0] <= 32)  // Full Player
				hull = &model->hulls[1];
			else // Golumn
				hull = &model->hulls[5];
		}
		else
		{
			if (size[0] < 3 || !model->hulls[1].available)
				hull = &model->hulls[0];
			else if (size[0] <= 32)
			{
				if (size[2] < 54 && model->hulls[3].available)
					hull = &model->hulls[3]; // 32x32x36 (half-life's crouch)
				else
					hull = &model->hulls[1];
			}
			else
				hull = &model->hulls[2];
		}
	}

// calculate an offset value to center the origin
	VectorSubtract (hull->clip_mins, mins, offset);
	VectorSubtract(start, offset, start_l);
	VectorSubtract(end, offset, end_l);
	Q1BSP_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start_l, end_l, trace);
	if (trace->fraction == 1)
	{
		VectorCopy (end, trace->endpos);
	}
	else
	{
		VectorAdd (trace->endpos, offset, trace->endpos);
	}

	return trace->fraction != 1;
}


qboolean SV_InvisibleToClient(edict_t *viewer, edict_t *seen)
{
	int i;
	trace_t tr;
	vec3_t start;
	vec3_t end;

//	if (seen->v->solid == SOLID_BSP)
//		return false;	//bsp ents are never culled this way

	//stage 1: check against their origin
	VectorAdd(viewer->v.origin, viewer->v.view_ofs, start);
	tr.fraction = 1;

	if (!Q1BSP_Trace (sv.worldmodel, 1, 0, start, seen->v.origin, vec3_origin, vec3_origin, &tr))
		return false;	//wasn't blocked


	//stage 2: check against their bbox
	for (i = 0; i < 8; i++)
	{
		end[0] = seen->v.origin[0] + ((i&1)?seen->v.mins[0]:seen->v.maxs[0]);
		end[1] = seen->v.origin[1] + ((i&2)?seen->v.mins[1]:seen->v.maxs[1]);
		end[2] = seen->v.origin[2] + ((i&4)?seen->v.mins[2]+0.1:seen->v.maxs[2]);

		tr.fraction = 1;
		if (!Q1BSP_Trace (sv.worldmodel, 1, 0, start, end, vec3_origin, vec3_origin, &tr))
			return false;	//this trace went through, so don't cull
	}

	return true;
}
#endif

/*
=============
SV_WriteEntitiesToClient
=============
*/
void SV_WriteEntitiesToClient (edict_t	*clent, sizebuf_t *msg, qboolean nomap)
{
	int		e, i, bits;
//	int mycount=0;
	float	miss;
	byte	*pvs;
	vec3_t	org;
	edict_t	*ent;
#ifdef SUPPORTS_ENTITY_ALPHA
	float	alpha, fullbright;
	eval_t  *val;
	
#endif
#ifdef SUPPORTS_KUROK_PROTOCOL
    // Tomaz - QC Alpha Scale Glow Begin
	eval_t  *val;
    float	alpha;
    float	scale;
    float	glow_size;
    float	glow_red;
    float	glow_green;
    float	glow_blue;

    // Tomaz - QC Alpha Scale Glow End
#endif

// find the client's PVS
	VectorAdd (clent->v.origin, clent->v.view_ofs, org);
	pvs = SV_FatPVS (org, sv.worldmodel);

// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{

		if (ent != clent)	// clent is ALWAYS sent
		{
// ignore ents without visible models
			if (!ent->v.modelindex || !pr_strings[ent->v.model])
				continue;

// ignore if not touching a PV leaf
			for (i=0 ; i < ent->num_leafs ; i++)
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
					break;

			if (i == ent->num_leafs)
				continue;		// not visible
#ifdef PROQUAKE_EXTENSION
			// JPG 3.30 - don't send updates if the client doesn't have the map
			if (nomap)
				continue;

			// Baker 3.99b: Slot Zero's user activated anti-lag mod capability
	       if ((int)clent->v.flags & FL_LOW_BANDWIDTH_CLIENT && (int)ent->v.effects & EF_MAYBE_DRAW)
	            continue;
#endif

#ifdef ANTIWALLHACK_SERVER
// Baker theoretical sv_cullentities_trace

			if (e<=svs.maxclients && sv_cullentities.value) {
				if(SV_InvisibleToClient(clent, ent)) {
					if (sv_cullentities_notify.value)
						Con_Printf("Not visible\n");
					continue;
				} else {
					if (sv_cullentities_notify.value)
						Con_Printf("Visible\n");
				}
			}

// End Baker theoretical
#endif
		}

		if (msg->maxsize - msg->cursize < 16)
		{
			Con_Printf ("packet overflow\n");
			return;
		}

// send an update
		bits = 0;

		for (i=0 ; i<3 ; i++)
		{
			miss = ent->v.origin[i] - ent->baseline.origin[i];
			if ( miss < -0.1 || miss > 0.1 )
				bits |= U_ORIGIN1<<i;
		}

		if ( ent->v.angles[0] != ent->baseline.angles[0] )
			bits |= U_ANGLE1;

		if ( ent->v.angles[1] != ent->baseline.angles[1] )
			bits |= U_ANGLE2;

		if ( ent->v.angles[2] != ent->baseline.angles[2] )
			bits |= U_ANGLE3;

//		if (!sv_gameplayfix_monster_lerp.value)
		if (ent->v.movetype == MOVETYPE_STEP)
			bits |= U_STEP;	// don't mess up the step animation

		if (ent->baseline.colormap != ent->v.colormap)
			bits |= U_COLORMAP;

		if (ent->baseline.skin != ent->v.skin)
			bits |= U_SKIN;

		if (ent->baseline.frame != ent->v.frame)
			bits |= U_FRAME;

		if (ent->baseline.effects != ent->v.effects)
			bits |= U_EFFECTS;

		if (ent->baseline.modelindex != ent->v.modelindex)
			bits |= U_MODEL;

#ifdef SUPPORTS_ENTITY_ALPHA
   // nehahra: model alpha
      if ((val = GETEDICTFIELDVALUE(ent, eval_alpha)))
         alpha = val->_float;
      else
         alpha = 1;

		if ((val = GETEDICTFIELDVALUE(ent, eval_fullbright)))
			fullbright = val->_float;
		else
			fullbright = 0;

		if ((alpha < 1 && alpha > 0) || fullbright)
         bits |= U_TRANS;
#endif

		if (e >= 256)
			bits |= U_LONGENTITY;

		if (bits >= 256)
			bits |= U_MOREBITS;

//		mycount++;

	// write the message
		MSG_WriteByte (msg,bits | U_SIGNAL);

		if (bits & U_MOREBITS)
			MSG_WriteByte (msg, bits>>8);
		if (bits & U_LONGENTITY)
			MSG_WriteShort (msg,e);
		else
			MSG_WriteByte (msg,e);

		if (bits & U_MODEL)
			MSG_WriteByte (msg,	ent->v.modelindex);

#ifdef SUPPORTS_KUROK_PROTOCOL
    // Tomaz - QC Alpha Scale Glow Begin

    {
        eval_t  *val;

        alpha=1;
        scale=1;

        if ((val = GETEDICTFIELDVALUE(ent, eval_alpha)))
        {
            alpha = val->_float;
        }
        else 
			alpha = 1;

        if ((val = GETEDICTFIELDVALUE(ent, eval_scale)))
        {
            scale = val->_float;
        }
        else
            scale = 1;

        if (scale > 4)
            scale = 4;

        if ((val = GETEDICTFIELDVALUE(ent, eval_glow_size)))
        {
            glow_size = val->_float;
        }
        else
            glow_size = 0;

        if (glow_size > 250)
            glow_size = 250;

        if ((val = GETEDICTFIELDVALUE(ent, eval_glow_red)))
        {
            glow_red = val->_float;
        }
        else
            glow_red = 0;

        if ((val = GETEDICTFIELDVALUE(ent, eval_glow_green)))
        {
            glow_green = val->_float;
        }
        else
            glow_green = 0;

        if ((val = GETEDICTFIELDVALUE(ent, eval_glow_blue)))
        {
            glow_blue = val->_float;
        }
        else
            glow_blue = 0;

        if ((alpha <= 1) && (alpha > 0))
            bits |= U_ALPHA;

        if ((scale <= 4) && (scale > 0))
            bits |= U_SCALE;

        if (glow_size >= 0)
        {
            bits |= U_GLOW_SIZE;
            bits |= U_GLOW_RED;
            bits |= U_GLOW_GREEN;
            bits |= U_GLOW_BLUE;
        }
    }

    // Tomaz - QC Alpha Scale Glow End
#endif
		if (bits & U_FRAME)
			MSG_WriteByte (msg, ent->v.frame);
		if (bits & U_COLORMAP)
			MSG_WriteByte (msg, ent->v.colormap);
		if (bits & U_SKIN)
			MSG_WriteByte (msg, ent->v.skin);
		if (bits & U_EFFECTS)
			MSG_WriteByte (msg, ent->v.effects);
		if (bits & U_ORIGIN1)
			MSG_WriteCoord (msg, ent->v.origin[0]);
		if (bits & U_ANGLE1)
			MSG_WriteAngle(msg, ent->v.angles[0]);
		if (bits & U_ORIGIN2)
			MSG_WriteCoord (msg, ent->v.origin[1]);
		if (bits & U_ANGLE2)
			MSG_WriteAngle(msg, ent->v.angles[1]);
		if (bits & U_ORIGIN3)
			MSG_WriteCoord (msg, ent->v.origin[2]);
		if (bits & U_ANGLE3)
			MSG_WriteAngle(msg, ent->v.angles[2]);
#ifdef SUPPORTS_ENTITY_ALPHA
      	if (bits & U_TRANS)
		{
                MSG_WriteFloat (msg, 2);
                MSG_WriteFloat (msg, alpha);
                MSG_WriteFloat (msg, fullbright);
		}
#endif
	}

//	Con_Printf("%i entities sent\n", mycount);
}

/*
=============
SV_CleanupEnts
=============
*/
void SV_CleanupEnts (void)
{
	int		e;
	edict_t	*ent;

	ent = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
		ent->v.effects = (int)ent->v.effects & ~EF_MUZZLEFLASH;
}

/*
==================
SV_WriteClientdataToMessage
==================
*/
void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg)
{
	int		bits, i, items;
	edict_t	*other;
	eval_t	*val;

// send a damage message
	if (ent->v.dmg_take || ent->v.dmg_save)
	{
		other = PROG_TO_EDICT(ent->v.dmg_inflictor);
		MSG_WriteByte (msg, svc_damage);
		MSG_WriteByte (msg, ent->v.dmg_save);
		MSG_WriteByte (msg, ent->v.dmg_take);
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, other->v.origin[i] + 0.5*(other->v.mins[i] + other->v.maxs[i]));

		ent->v.dmg_take = 0;
		ent->v.dmg_save = 0;
	}

// send the current viewpos offset from the view entity
	SV_SetIdealPitch ();		// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ent->v.fixangle )
	{
		MSG_WriteByte (msg, svc_setangle);
		for (i=0 ; i < 3 ; i++)
			MSG_WriteAngle (msg, ent->v.angles[i] );
		ent->v.fixangle = 0;
	}

	bits = 0;

	if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;

	if (ent->v.idealpitch)
		bits |= SU_IDEALPITCH;

// stuff the sigil bits into the high bits of items for sbar, or else mix in items2
	if ((val = GETEDICTFIELDVALUE(ent, eval_items2)))
		items = (int)ent->v.items | ((int)val->_float << 23);
	else
		items = (int)ent->v.items | ((int)pr_global_struct->serverflags << 28);

	bits |= SU_ITEMS;

	if ( (int)ent->v.flags & FL_ONGROUND)
		bits |= SU_ONGROUND;

	if ( ent->v.waterlevel >= 2)
		bits |= SU_INWATER;

	for (i=0 ; i<3 ; i++)
	{
		if (ent->v.punchangle[i])
			bits |= (SU_PUNCH1<<i);
		if (ent->v.velocity[i])
			bits |= (SU_VELOCITY1<<i);
	}

	if (ent->v.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (ent->v.armorvalue)
		bits |= SU_ARMOR;

//	if (ent->v.weapon)
		bits |= SU_WEAPON;

// send the data
	MSG_WriteByte (msg, svc_clientdata);
	MSG_WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, ent->v.view_ofs[2]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar (msg, ent->v.idealpitch);

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			MSG_WriteChar (msg, ent->v.punchangle[i]);
		if (bits & (SU_VELOCITY1<<i))
			MSG_WriteChar (msg, ent->v.velocity[i]/16);
	}

// [always sent]	if (bits & SU_ITEMS)
	MSG_WriteLong (msg, items);

	if (bits & SU_WEAPONFRAME)
		MSG_WriteByte (msg, ent->v.weaponframe);
	if (bits & SU_ARMOR)
		MSG_WriteByte (msg, ent->v.armorvalue);
	if (bits & SU_WEAPON)
		MSG_WriteByte (msg, SV_ModelIndex(pr_strings+ent->v.weaponmodel));

	MSG_WriteShort (msg, ent->v.health);
	MSG_WriteByte (msg, ent->v.currentammo);
	MSG_WriteByte (msg, ent->v.ammo_shells);
	MSG_WriteByte (msg, ent->v.ammo_nails);
	MSG_WriteByte (msg, ent->v.ammo_rockets);
	MSG_WriteByte (msg, ent->v.ammo_cells);

	if (standard_quake) {
		MSG_WriteByte (msg, ent->v.weapon);
	} else {
		for(i=0;i<32;i++) {
			if ( ((int)ent->v.weapon) & (1<<i) ) {
				MSG_WriteByte (msg, i);
				break;
			}
		}
	}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	byte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;

	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteByte (&msg, svc_time);
	MSG_WriteFloat (&msg, sv.time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client->edict, &msg);
#ifdef PROQUAKE_EXTENSION
	SV_WriteEntitiesToClient (client->edict, &msg, client->nomap);	// JPG 3.30 - added client->nomap
#else
	SV_WriteEntitiesToClient (client->edict, &msg, 0);
#endif

// copy the server datagram if there is space
	if (msg.cursize + sv.datagram.cursize < msg.maxsize)
		SZ_Write (&msg, sv.datagram.data, sv.datagram.cursize);

// send the datagram
	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
	{
		SV_DropClient (true);// if the message couldn't send, kick off
		return false;
	}

	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages (void)
{
	int			i, j;
	client_t *client;

// check for changes to be sent over the reliable streams
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (host_client->old_frags != host_client->edict->v.frags)
		{
			for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
			{
				if (!client->active)
					continue;
				MSG_WriteByte (&client->message, svc_updatefrags);
				MSG_WriteByte (&client->message, i);
				MSG_WriteShort (&client->message, host_client->edict->v.frags);
			}

			host_client->old_frags = host_client->edict->v.frags;
		}
	}

	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		SZ_Write (&client->message, sv.reliable_datagram.data, sv.reliable_datagram.cursize);
	}

	SZ_Clear (&sv.reliable_datagram);
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void SV_SendNop (client_t *client)
{
	sizebuf_t	msg;
	byte		buf[4];

	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteChar (&msg, svc_nop);

	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
		SV_DropClient (true);	// if the message couldn't send, kick off
	client->last_message = realtime;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i;

// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

		if (host_client->spawned)
		{
			if (!SV_SendClientDatagram (host_client))
				continue;
		}
		else
		{
		// the player isn't totally in the game yet
		// send small keepalive messages if too much time has passed
		// send a full message when the next signon stage has been requested
		// some other message data (name changes, etc) may accumulate
		// between signon stages
			if (!host_client->sendsignon)
			{
				if (realtime - host_client->last_message > 5)
					SV_SendNop (host_client);
				continue;	// don't send out non-signon messages
			}
		}
#ifdef PROQUAKE_EXTENSION
		// JPG 3.40 - NAT fix
		if (host_client->netconnection->net_wait)
			continue;
#endif
		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->message.overflowed)
		{
			SV_DropClient (true);
			host_client->message.overflowed = false;
			continue;
		}

		if (host_client->message.cursize || host_client->dropasap)
		{
			if (!NET_CanSendMessage (host_client->netconnection))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->dropasap)
			{
				SV_DropClient (false);	// went to another level
			}
			else
			{
				if (NET_SendMessage (host_client->netconnection, &host_client->message) == -1)
					SV_DropClient (true);	// if the message couldn't send, kick off
				SZ_Clear (&host_client->message);
				host_client->last_message = realtime;
				host_client->sendsignon = false;
			}
		}
	}

// clear muzzle flashes
	SV_CleanupEnts ();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex (char *name)
{
	int		i;

	if (!name || !name[0])
		return 0;

	for (i=0 ; i<MAX_MODELS && sv.model_precache[i] ; i++)
		if (!strcmp(sv.model_precache[i], name))
			return i;

	if (i==MAX_MODELS || !sv.model_precache[i])
		Host_Error ("SV_ModelIndex: model %s not precached", name);   // Baker: 4.59 changed from sys_error to host_error

	return i;
}

/*
================
SV_CreateBaseline
================
*/
void SV_CreateBaseline (void)
{
	int			i, entnum;
	edict_t			*svent;

	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
	// get the current server version
		svent = EDICT_NUM(entnum);
		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !svent->v.modelindex)
			continue;

	// create entity baseline
		VectorCopy (svent->v.origin, svent->baseline.origin);
		VectorCopy (svent->v.angles, svent->baseline.angles);
		svent->baseline.frame = svent->v.frame;
		svent->baseline.skin = svent->v.skin;
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl");
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex = SV_ModelIndex(pr_strings + svent->v.model);
		}

	// add to the message
		MSG_WriteByte (&sv.signon,svc_spawnbaseline);
		MSG_WriteShort (&sv.signon,entnum);

		MSG_WriteByte (&sv.signon, svent->baseline.modelindex);
		MSG_WriteByte (&sv.signon, svent->baseline.frame);
		MSG_WriteByte (&sv.signon, svent->baseline.colormap);
		MSG_WriteByte (&sv.signon, svent->baseline.skin);
		for (i=0 ; i<3 ; i++)
		{
			MSG_WriteCoord(&sv.signon, svent->baseline.origin[i]);
			MSG_WriteAngle(&sv.signon, svent->baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect (void)
{
	char	data[128];
	sizebuf_t	msg;

	msg.data = data;
	msg.cursize = 0;
	msg.maxsize = sizeof(data);

	MSG_WriteChar (&msg, svc_stufftext);
	MSG_WriteString (&msg, "reconnect\n");
	NET_SendToAll (&msg, 5);

	if (cls.state != ca_dedicated)
		Cmd_ExecuteString ("reconnect\n", src_command);
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms (void)
{
	int		i, j;

	svs.serverflags = pr_global_struct->serverflags;

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

	// call the progs to get default spawn parms for the new client
		pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
		PR_ExecuteProgram (pr_global_struct->SetChangeParms);
		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
			host_client->spawn_parms[j] = (&pr_global_struct->parm1)[j];
	}
}

#ifdef PROQUAKE_EXTENSION
void COM_ModelCRC (void);	// JPG 3.20 - model checking
#endif

/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
extern float		scr_centertime_off;

void SV_SpawnServer (char *server)
{
	edict_t		*ent;
	int			i;
#ifdef PROQUAKE_EXTENSION
	time_t  	ltime; // JPG added
	// JPG - print date and time to console log
	time( &ltime );
	Con_Printf( "\nSV_SpawnServer: %s\n", ctime( &ltime ) );
#endif
	// let's not have any servers with no name
	if (hostname.string[0] == 0)
		Cvar_SetStringByRef (&hostname, "UNNAMED");
	scr_centertime_off = 0;

	Con_DPrintf ("SpawnServer: %s\n",server);
	svs.changelevel_issued = false;		// now safe to issue another

// tell all connected clients that we are going to a new level
	if (sv.active)
		SV_SendReconnect ();

// make cvars consistant
	if (coop.value)
		Cvar_SetValueByRef (&deathmatch, 0);
	current_skill = (int)(skill.value + 0.5);
	current_skill = bound(0, current_skill, 3);

	Cvar_SetValueByRef (&skill, (float)current_skill);

// set up the new server
	Host_ClearMemory ();

	memset (&sv, 0, sizeof(sv));

	strcpy (sv.name, server);

// load progs to get entity field count
	PR_LoadProgs (sv_progs.string);

// allocate server memory
	sv.max_edicts = MAX_EDICTS;

	sv.edicts = Hunk_AllocName (sv.max_edicts*pr_edict_size, "edicts");

	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.cursize = 0;
	sv.datagram.data = sv.datagram_buf;

	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.cursize = 0;
	sv.reliable_datagram.data = sv.reliable_datagram_buf;

	sv.signon.maxsize = sizeof(sv.signon_buf);
	sv.signon.cursize = 0;
	sv.signon.data = sv.signon_buf;

// leave slots at start for clients only
	sv.num_edicts = svs.maxclients+1;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		ent = EDICT_NUM(i+1);
		svs.clients[i].edict = ent;
	}

	sv.state = ss_loading;
	sv.paused = false;

	sv.time = 1.0;

	R_PreMapLoad (server);

	strcpy (sv.name, server);
	snprintf(sv.modelname, sizeof(sv.modelname),"maps/%s.bsp", server);
	sv.worldmodel = Mod_ForName (sv.modelname, false);

	//Baker 3.99b: R00k if map isnt found then load the sv_defaultmap instead
	if (!sv.worldmodel && sv_defaultmap.string[0])
	{
		strcpy (sv.name, sv_defaultmap.string);
		vsnprintf (sv.modelname, sizeof(sv.modelname), "maps/%s.bsp", sv_defaultmap.string);
		sv.worldmodel = Mod_ForName (sv.modelname, false);
	}
	// Baker 3.99b: end mod

	if (!sv.worldmodel)
	{
		Con_Printf ("Couldn't spawn server %s\n", sv.modelname);
		sv.active = false;
		return;
	}
	sv.models[1] = sv.worldmodel;

// clear world interaction links
	SV_ClearWorld ();

	sv.sound_precache[0] = pr_strings;

	sv.model_precache[0] = pr_strings;
	sv.model_precache[1] = sv.modelname;
	for (i=1 ; i<sv.worldmodel->numsubmodels ; i++)
	{
		sv.model_precache[i+1] = localmodels[i];
		sv.models[i+1] = Mod_ForName (localmodels[i], false);
	}


// load the rest of the entities
	ent = EDICT_NUM(0);
	memset (&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	ent->v.model = sv.worldmodel->name - pr_strings;
	ent->v.modelindex = 1;		// world model
	ent->v.solid = SOLID_BSP;
	ent->v.movetype = MOVETYPE_PUSH;

	if (coop.value)
		pr_global_struct->coop = coop.value;
	else
		pr_global_struct->deathmatch = deathmatch.value;

	pr_global_struct->mapname = sv.name - pr_strings;

// serverflags are for cross level information (sigils)
	pr_global_struct->serverflags = svs.serverflags;

	ED_LoadFromFile (sv.worldmodel->entities);

	sv.active = true;

// all setup is completed, any further precache statements are errors
	sv.state = ss_active;

// run two frames to allow everything to settle
	host_frametime = 0.1;
	SV_Physics ();
	SV_Physics ();

// create a baseline for more efficient communications
	SV_CreateBaseline ();

// send serverinfo to all connected clients
	for (i=0,host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_SendServerinfo (host_client);

	Con_DPrintf ("Server spawned.\n");
#ifdef PROQUAKE_EXTENSION
	if (pq_cheatfreeEnabled)
		COM_ModelCRC();
#endif
}

