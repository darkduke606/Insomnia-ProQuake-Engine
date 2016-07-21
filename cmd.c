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
// cmd.c -- Quake script command processing module

#include "quakedef.h"

void Cmd_ForwardToServer_f (void);

#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	char	name[MAX_ALIAS_NAME];
	char	*value;
} cmdalias_t;

cmdalias_t	*cmd_alias;

int trashtest;
int *trashspot;

qboolean	cmd_wait;

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
static void Cmd_Wait_f (void)
{
	cmd_wait = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

sizebuf_t	cmd_text;

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	SZ_Alloc (&cmd_text, 8192);		// space for commands and script files
}


/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (char *text)
{
	int		l;

	l = strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Con_Printf ("Cbuf_AddText: overflow\n");
		return;
	}

	SZ_Write (&cmd_text, text, strlen (text));
}

/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (char *text)
{
	char	*temp = NULL;
	int		templen;

// copy off any commands still remaining in the exec buffer
	if ((templen = cmd_text.cursize))
	{
		temp = Z_Malloc (templen);
		memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}

// add the entire text of the file
	Cbuf_AddText (text);

// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
		Z_Free (temp);
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;
#ifdef PROQUAKE_EXTENSION
	int		notcmd;	// JPG - so that the ENTIRE line can be forwarded
#endif
	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
#ifdef PROQUAKE_EXTENSION
		notcmd = strncmp(text, "cmd ", 4);  // JPG - so that the ENTIRE line can be forwarded
#endif
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
#ifdef PROQUAKE_EXTENSION
			if ( !(quotes&1) &&  text[i] == ';' && notcmd)   // JPG - added && cmd so that the ENTIRE line can be forwareded
#else
			if ( !(quotes&1) &&  text[i] == ';')
#endif
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		memcpy (line, text, i);
		line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
		{
			cmd_text.cursize = 0;
		}
		else
		{
			i++;
			cmd_text.cursize -= i;
			memcpy (text, text+i, cmd_text.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line, src_command);

		if (cmd_wait)  {
			// skip out while text still remains in buffer, leaving it for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

#ifdef PROQUAKE_EXTENSION
/*
===============
Cmd_Baker_Inject_Aliases

Adds commonly used aliases in and reassigns the sound keys.
Occurs with stuffcmds and reset to defaults.
Should never be used with dedicated server.
This is hacky, eventually the "right" way to do this
will be decided, but ProQuake isn't a gamedir engine
so the options are limited.
===============
*/

void Cmd_Baker_Inject_Aliases () {

	if (COM_CheckParm ("-noinjectaliases") == 0) {
		// Baker 3.70 - Alias injection point
		Cbuf_AddText ("alias +quickgrenade \"-attack;wait;impulse 6;wait;+attack\"\n");
		Cbuf_AddText ("alias -quickgrenade \"-attack;wait;bestweapon 7 8 5 3 4 2 1\"\n");
		Cbuf_AddText ("alias +quickrocket \"-attack;wait;impulse 7;wait;+attack\"\n");
		Cbuf_AddText ("alias -quickrocket \"-attack;wait;bestweapon 8 5 3 4 2 7 1\"\n");
		Cbuf_AddText ("alias +quickshaft \"-attack;wait;impulse 8;wait;+attack\"\n");
		Cbuf_AddText ("alias -quickshaft \"-attack;wait;bestweapon 7 5 8 3 4 2 1\"\n");
		Cbuf_AddText ("alias +quickshot \"-attack;wait;impulse 2;wait;+attack\"\n");
		Cbuf_AddText ("alias -quickshot \"-attack;wait;bestweapon 7 8 5 3 4 2 1\"\n");
		Cbuf_AddText ("alias bestsafe \"bestweapon 8 5 3 4 2 1\"\n");
		Cbuf_AddText ("alias teamloc \"say_team I am at %l with %h health/%a armor\"\n");
		if (COM_CheckParm ("-nosoundkeys") == 0) {
			Cbuf_AddText ("bind \"-\" \"volumedown\"\n");
			Cbuf_AddText ("bind \"=\" \"volumeup\"\n");
		} else {
			Con_Printf ("Automatic sound keys disabled\n");
		}
		Cbuf_AddText ("alias +zoom \"savefov; savesensitivity; fov 70; sensitivity 4; wait; fov 58; sensitivity 3.25; wait; fov 45; sensitivity 2.50; wait; fov 32; sensitivity 1.74; wait; fov 20; sensitivity 14.0\"\n");
		Cbuf_AddText ("alias -zoom \"fov 32; sensitivity 1.75; wait; fov 45; sensitivity 2.50; wait; fov 58; sensitivity 3.25; wait; sensitivity 4; wait; restoresensitivity; restorefov\"\n");
		Con_Printf ("Extended aliases initialized\n");
		// Baker 3.70 - End Alias injection point
	} else {
		Con_Printf ("Automatic aliases disabled\n");
	}
}
#endif

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
static void Cmd_StuffCmds_f (void)
{
	int		i, j;
	int		s;
	char	*text, *build, c;

	if (Cmd_Argc () != 1)
	{
		Con_Printf ("stuffcmds : execute command line parameters\n");
		return;
	}

#ifdef PROQUAKE_EXTENSION
	// Baker 3.67 - Inject commonly used aliases
	if (cls.state != ca_dedicated)
		Cmd_Baker_Inject_Aliases();
#endif

// build the combined string to parse from
	s = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		s += strlen (com_argv[i]) + 1;
	}
	if (!s)
		return;

	text = Z_Malloc (s+1);
	text[0] = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		strcat (text,com_argv[i]);   // Dynamic string: no strlcat required
		if (i != com_argc-1)
			strcat (text, " ");  // Dynamic string: no strlcat required
	}

// pull out the commands
	build = Z_Malloc (s+1);
	build[0] = 0;

	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;

			strcat (build, text+i); // Dynamic string: no strlcat required
			strcat (build, "\n");  	// Dynamic string: no strlcat required
			text[j] = c;
			i = j-1;
		}
	}

	if (build[0])
		Cbuf_InsertText (build);

	Z_Free (text);
	Z_Free (build);
}


/*
===============
Cmd_Exec_f
===============
*/
static void Cmd_Exec_f (void)
{
	char	*f;
	int		mark;
	char	name[MAX_OSPATH];

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	strlcpy (name, Cmd_Argv(1), sizeof(name));
	mark = Hunk_LowMark ();
#ifdef FLASH_FILE_SYSTEM
	as3ReadFileSharedObject(va("%s/%s", com_gamedir, Cmd_Argv(1)));//config.cfg is stored in the flash shared objects
#endif
	if (!(f = (char *)COM_LoadHunkFile (name))) {
		char	*p;

		p = COM_SkipPath (name);
		if (!strchr(p, '.'))
		{	// no extension, so try the default (.cfg)
			strlcat (name, ".cfg", sizeof(name));
			f = (char *)COM_LoadHunkFile (name);
		}

		if (!f) {
			Con_Printf ("couldn't exec %s\n", name);
		return;
	}
	}

#ifdef MACOSX
        {
// Baker: this replaces carriage return characters with linefeeds
            char *	myData = f;
            while (*myData != 0x00) {
                if (*myData == 0x0D)
					*myData = 0x0A;
                myData++;
            }
    }
#endif // Only Macs need to do this; I don't believe Linux does

	Con_Printf ("execing %s\n",name);

	Cbuf_InsertText (f);
	Hunk_FreeToLowMark (mark);
}
/*
("%s/%s", host_parms.basedir, Cmd_Argv(1)));
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f (void)
{
	int		i;

	for (i=1 ; i<Cmd_Argc() ; i++)
		Con_Printf ("%s ",Cmd_Argv(i));
	Con_Printf ("\n");
}

/*
===============
Cmd_Alias_f -- johnfitz -- rewritten

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
static void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	char		cmd[1024];
	int			i, c;
	char		*s;


	switch (Cmd_Argc())
	{
	case 1: //list all aliases
		for (a = cmd_alias, i = 0; a; a=a->next, i++)
			Con_SafePrintf ("   %s: %s", a->name, a->value);
		if (i)
			Con_SafePrintf ("%i alias command(s)\n", i);
		else
			Con_SafePrintf ("no alias commands found\n");
		break;
	case 2: //output current alias string
		for (a = cmd_alias ; a ; a=a->next)
			if (!strcmp(Cmd_Argv(1), a->name))
				Con_Printf ("   %s: %s", a->name, a->value);
		break;

	default: //set alias string

	s = Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Con_Printf ("Alias name is too long\n");
		return;
	}

	// if the alias allready exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		a = Z_Malloc (sizeof(cmdalias_t));
		a->next = cmd_alias;
		cmd_alias = a;
	}
	strcpy (a->name, s);

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		strlcat (cmd, Cmd_Argv(i), sizeof(cmd));
		if (i != c)
			strlcat (cmd, " ", sizeof(cmd));
	}
	strlcat (cmd, "\n", sizeof(cmd));

	a->value = CopyString (cmd);
		break;
	}
}

/*
===============
Cmd_Unalias_f -- johnfitz
===============
*/
static void Cmd_Unalias_f (void)
{
	cmdalias_t	*a, *prev;

	switch (Cmd_Argc())
	{
	default:
	case 1:
		Con_Printf("unalias <name> : delete alias\n");
		break;
	case 2:
		for (prev = a = cmd_alias; a; a = a->next)
		{
			if (!strcmp(Cmd_Argv(1), a->name))
			{
				prev->next = a->next;
				Z_Free (a->value);
				Z_Free (a);
				prev = a;
				return;
			}
			prev = a;
		}
		break;
	}
}

/*
===============
Cmd_Unaliasall_f -- johnfitz
===============
*/
static void Cmd_Unaliasall_f (void)
{
	cmdalias_t	*blah;

	while (cmd_alias)
	{
		blah = cmd_alias->next;
		Z_Free(cmd_alias->value);
		Z_Free(cmd_alias);
		cmd_alias = blah;
	}
}



















/*
=============================================================================
					COMMAND EXECUTION
=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
	char					*name;
	xcommand_t				function;
} cmd_function_t;

static	cmd_function_t	*cmd_functions;		// possible commands to execute

#define	MAX_ARGS		80

static	int			cmd_argc;
static	char		*cmd_argv[MAX_ARGS];
static	char		*cmd_null_string = "";
static	char		*cmd_args = NULL;

cmd_source_t	cmd_source;

#ifdef PROQUAKE_EXTENSION
void Mat_Init (void);	// JPG
#endif

// Baker 3.83 - CMDLINE

/*
=============
Cmd_Cmdline_f
=============
*/

extern cvar_t cmdline;
void Cmd_Cmdline_f (void)
{
	Con_Printf ("Your command line: %s\n", cmdline.string);
}


/*
============
Cmd_Argc
============
*/
int		Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv (int arg)
{
	if ( (unsigned)arg >= cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];
}

/*
============
Cmd_Args
============
*/
char		*Cmd_Args (void)
{
	return cmd_args;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void Cmd_TokenizeString (char *text)
{
	int		i;

// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);

	cmd_argc = 0;
	cmd_args = NULL;

	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
			text++;

		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;

		if (cmd_argc == 1)
			 cmd_args = text;

		if (!(text = COM_Parse (text)))
			return;

		if (cmd_argc < MAX_ARGS)
		{
			cmd_argv[cmd_argc] = Z_Malloc (strlen(com_token)+1);
			strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}
}


/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand (char *cmd_name, xcommand_t function)
{
	cmd_function_t	*cmd;
	cmd_function_t	*cursor,*prev; //Baker 3.75 - from Fitz johnfitz -- sorted list insert

	if (host_initialized)	// because hunk allocation would get stomped
		Sys_Error ("Cmd_AddCommand after host_initialized");

// fail if the command is a variable name
	if (Cvar_VariableString(cmd_name)[0])
	{
		Con_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}

// fail if the command already exists
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!strcmp (cmd_name, cmd->name))
		{
			Con_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			return;
		}
	}

	cmd = Hunk_Alloc (sizeof(cmd_function_t));
	cmd->name = cmd_name;
	cmd->function = function;

	//johnfitz -- insert each entry in alphabetical order
    if (cmd_functions == NULL || strcmp(cmd->name, cmd_functions->name) < 0) //insert at front
	{
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}
    else //insert later
	{
        prev = cmd_functions;
        cursor = cmd_functions->next;
        while ((cursor != NULL) && (strcmp(cmd->name, cursor->name) > 0))
		{
            prev = cursor;
            cursor = cursor->next;
        }
        cmd->next = prev->next;
        prev->next = cmd;
    }
	//johnfitz
}

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists (char *cmd_name)
{
	cmd_function_t	*cmd;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!strcmp (cmd_name,cmd->name))
			return true;
	}

	return false;
}


/*
============
Cmd_CompleteCommand
============
*/
// JPG 1.05 - completely rewrote this; includes aliases
char *Cmd_CompleteCommand (char *partial)
{
	cmd_function_t	*cmd;
	char *best = "~";
	char *least = "~";
	cmdalias_t *alias;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (strcmp(cmd->name, partial) >= 0 && strcmp(best, cmd->name) > 0)
			best = cmd->name;
		if (strcmp(cmd->name, least) < 0)
			least = cmd->name;
	}
	for (alias = cmd_alias ; alias ; alias = alias->next)
	{
		if (strcmp(alias->name, partial) >= 0 && strcmp(best, alias->name) > 0)
			best = alias->name;
		if (strcmp(alias->name, least) < 0)
			least = alias->name;
	}
	if (best[0] == '~')
		return least;
	return best;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (char *text, cmd_source_t src)
{
	cmd_function_t	*cmd;
	cmdalias_t		*a;

//	Con_Printf("Cmd_ExecuteString: %s \n", text);
	cmd_source = src;
	Cmd_TokenizeString (text);

// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

// check functions
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!strcasecmp (cmd_argv[0],cmd->name))
		{
			cmd->function ();
			return;
		}
	}

// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!strcasecmp (cmd_argv[0], a->name))
		{
			Cbuf_InsertText (a->value);
			return;
		}
	}

// check cvars
	if (!Cvar_Command ())
		Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv(0));

}

#ifdef PROQUAKE_EXTENSION
// JPG - added these for %r formatting
extern cvar_t	pq_needrl;
extern cvar_t	pq_haverl;
extern cvar_t	pq_needrox;

// JPG - added these for %p formatting
extern cvar_t	pq_quad;
extern cvar_t	pq_pent;
extern cvar_t	pq_ring;

// JPG 3.00 - %w formatting
extern cvar_t	pq_weapons;
extern cvar_t	pq_noweapons;
#endif

/*
=====================
Cmd_ForwardToServer_f

Sends the entire command line over to the server
=====================
*/
void Cmd_ForwardToServer_f (void)
{
#ifdef PROQUAKE_EXTENSION
	//from ProQuake --start
	char *src, *dst, buff[128];			// JPG - used for say/say_team formatting
	int minutes, seconds, match_time;	// JPG - used for %t
	//from ProQuake --end
#endif
	if (cls.state != ca_connected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (cls.demoplayback)
		return;		// not really connected

	MSG_WriteByte (&cls.message, clc_stringcmd);

#ifdef PROQUAKE_EXTENSION
	//----------------------------------------------------------------------
	// JPG - handle say separately for formatting--start
	if ((!strcasecmp(Cmd_Argv(0), "say") || !strcasecmp(Cmd_Argv(0), "say_team")) && Cmd_Argc() > 1)
	{
		SZ_Print (&cls.message, Cmd_Argv(0));
		SZ_Print (&cls.message, " ");

		src = Cmd_Args();
		dst = buff;
		while (*src && dst - buff < 100)
		{
			if (*src == '%')
			{
				switch (*++src)
				{
				case 'h':
					dst += sprintf(dst, "%d", cl.stats[STAT_HEALTH]);
					break;

				case 'a':
					dst += sprintf(dst, "%d", cl.stats[STAT_ARMOR]);
					break;

				case 'r':
					if (cl.stats[STAT_HEALTH] > 0 && (cl.items & IT_ROCKET_LAUNCHER))
					{
						if (cl.stats[STAT_ROCKETS] < 5)
							dst += sprintf(dst, "%s", pq_needrox.string);
						else
							dst += sprintf(dst, "%s", pq_haverl.string);
					}
					else
						dst += sprintf(dst, "%s", pq_needrl.string);
					break;

				case 'l':
					dst += sprintf(dst, "%s", LOC_GetLocation(cl_entities[cl.viewentity].origin));
					break;

				case 'd':
					dst += sprintf(dst, "%s", LOC_GetLocation(cl.death_location));
					break;

				case 'c':
					dst += sprintf(dst, "%d", cl.stats[STAT_CELLS]);
					break;

				case 'x':
					dst += sprintf(dst, "%d", cl.stats[STAT_ROCKETS]);
					break;

				case 'p':
					if (cl.stats[STAT_HEALTH] > 0)
					{
						if (cl.items & IT_QUAD)
						{
							dst += sprintf(dst, "%s", pq_quad.string);
							if (cl.items & (IT_INVULNERABILITY | IT_INVISIBILITY))
								*dst++ = ',';
						}
						if (cl.items & IT_INVULNERABILITY)
						{
							dst += sprintf(dst, "%s", pq_pent.string);
							if (cl.items & IT_INVISIBILITY)
								*dst++ = ',';
						}
						if (cl.items & IT_INVISIBILITY)
							dst += sprintf(dst, "%s", pq_ring.string);
					}
					break;

				case 'w':	// JPG 3.00
					{
						int first = 1;
						int item;
						char *ch = pq_weapons.string;
						if (cl.stats[STAT_HEALTH] > 0)
						{
							for (item = IT_SUPER_SHOTGUN ; item <= IT_LIGHTNING ; item *= 2)
							{
								if (*ch != ':' && (cl.items & item))
								{
									if (!first)
										*dst++ = ',';
									first = 0;
									while (*ch && *ch != ':')
										*dst++ = *ch++;
								}
								while (*ch && *ch != ':')
									*ch++;
								if (*ch)
									*ch++;
								if (!*ch)
									break;
							}
						}
						if (first)
							dst += sprintf(dst, "%s", pq_noweapons.string);
					}
					break;

				case '%':
					*dst++ = '%';
					break;

				case 't':
					if ((cl.minutes || cl.seconds) && cl.seconds < 128)
					{
						if (cl.match_pause_time)
							match_time = ceil(60.0 * cl.minutes + cl.seconds - (cl.match_pause_time - cl.last_match_time));
						else
							match_time = ceil(60.0 * cl.minutes + cl.seconds - (cl.time - cl.last_match_time));
						minutes = match_time / 60;
						seconds = match_time - 60 * minutes;
					}
					else
					{
						minutes = cl.time / 60;
						seconds = cl.time - 60 * minutes;
						minutes &= 511;
					}
					dst += sprintf(dst, "%d:%02d", minutes, seconds);
					break;

				default:
					*dst++ = '%';
					*dst++ = *src;
					break;
				}
				if (*src)
					src++;
			}
			else
				*dst++ = *src++;
		}
		*dst = 0;

		SZ_Print (&cls.message, buff);
		return;
	}
	// JPG - handle say separately for formatting--end
	//----------------------------------------------------------------------
#endif

	if (strcasecmp(Cmd_Argv(0), "cmd"))
	{
		SZ_Print (&cls.message, Cmd_Argv(0));
		SZ_Print (&cls.message, " ");
	}
	if (Cmd_Argc() > 1)
		SZ_Print (&cls.message, Cmd_Args());
	else
		SZ_Print (&cls.message, "\n");
}


/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/

int Cmd_CheckParm (char *parm)
{
	int i;

	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (! strcasecmp (parm, Cmd_Argv (i)))
			return i;

	return 0;
}



/*
====================
Cmd_CmdList_f

List all console commands
====================
*/
void Cmd_CmdList_f (void) {
	cmd_function_t	*cmd;

	char 		*partial;
	int				len, count;

	if (Cmd_Argc() > 1) {
		partial = Cmd_Argv (1);
		len = strlen(partial);
	} else {
		partial = NULL;
		len = 0;
	}

	Con_Printf ("\n");

	count=0;
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {

		if (partial && strncmp (partial,cmd->name, len))
			continue;

		Con_Printf ("%s\n", cmd->name);

		count++;
	}

	Con_Printf ("\n%i command(s)", count);

	if (partial) {
		Con_Printf (" beginning with \"%s\"", partial);
	}

	Con_Printf ("\n\n");
}

// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************
// ********************** bsync *********************


















































// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************
// ********************** b2sync *********************


/*
============
Cmd_Init
============
*/
void Cmd_Init (void) {
	void Host_Mapname_f (void);
// register our commands
	Cmd_AddCommand ("stuffcmds",Cmd_StuffCmds_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
	Cmd_AddCommand ("unalias", Cmd_Unalias_f); //johnfitz
	Cmd_AddCommand ("commandline", Cmd_Cmdline_f);

	Cmd_AddCommand ("mapname", Host_Mapname_f); // Baker 3.99 from FitzQuake
#ifdef PROQUAKE_EXTENSION
	Cmd_AddCommand ("matrix", Mat_Init_f);	// JPG
#endif
	Cmd_AddCommand ("cmdlist", Cmd_CmdList_f);
}
