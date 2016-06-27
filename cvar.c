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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

cvar_t	*cvar_vars;
char	*cvar_null_string = "";

void Cvar_SetStringByName (char *var_name, char *value);
void Cvar_SetValueByName (char *var_name, float value);


//==============================================================================
//
//  USER COMMANDS
//
//==============================================================================

void Cvar_Reset (char *name); //johnfitz

// Baker 3.60 -- 2000-01-09 CvarList command by Maddes  start

/*
============
Cvar_List_f -- johnfitz
============
*/
void Cvar_List_f (void)
{
	cvar_t		*cvar;
	char 		*partial;
	int		len, count;

	if (Cmd_Argc() > 1)
	{
		partial = Cmd_Argv (1);
		len = strlen(partial);
	}
	else
	{
		partial = NULL;
		len = 0;
	}

	Con_Printf ("\n");

	count=0;
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
	{
		if (partial && strncmp (partial,cvar->name, len))
		{
			continue;
		}

		Con_Printf ("%s is [%s]\n", cvar->name, cvar->string);

		count++;

	}



	Con_Printf ("\n%i cvar(s)", count);

	if (partial)
	{
		Con_Printf (" beginning with \"%s\"", partial);
	}

	Con_Printf ("\n\n");
}

/*
============
Cvar_Inc_f -- johnfitz
============
*/
void Cvar_Inc_f (void)
{
	switch (Cmd_Argc())
	{
	default:
	case 1:
		Con_Printf("inc <cvar> [amount] : increment cvar\n");
		break;
	case 2:
		Cvar_SetValueByName (Cmd_Argv(1), Cvar_VariableValue(Cmd_Argv(1)) + 1);
		break;
	case 3:
		Cvar_SetValueByName (Cmd_Argv(1), Cvar_VariableValue(Cmd_Argv(1)) + atof(Cmd_Argv(2)));
		break;
	}
}

/*
============
Cvar_Toggle_f -- johnfitz
============
*/
void Cvar_Toggle_f (void)
{
	switch (Cmd_Argc())
	{
	default:
	case 1:
		Con_Printf("toggle <cvar> : toggle cvar\n");
		break;
	case 2:
		if (Cvar_VariableValue(Cmd_Argv(1)))
			Cvar_SetStringByName  (Cmd_Argv(1), "0");
		else
			Cvar_SetStringByName  (Cmd_Argv(1), "1");
		break;
	}
}

/*
============
Cvar_Cycle_f -- johnfitz
============
*/
void Cvar_Cycle_f (void)
{
	int i;

	if (Cmd_Argc() < 3)
	{
		Con_Printf("cycle <cvar> <value list>: cycle cvar through a list of values\n");
		return;
	}

	//loop through the args until you find one that matches the current cvar value.
	//yes, this will get stuck on a list that contains the same value twice.
	//it's not worth dealing with, and i'm not even sure it can be dealt with.

	for (i=2;i<Cmd_Argc();i++)
	{
		//zero is assumed to be a string, even though it could actually be zero.  The worst case
		//is that the first time you call this command, it won't match on zero when it should, but after that,
		//it will be comparing strings that all had the same source (the user) so it will work.
		if (atof(Cmd_Argv(i)) == 0)
		{
			if (!strcmp(Cmd_Argv(i), Cvar_VariableString(Cmd_Argv(1))))
				break;
		}
		else
		{
			if (atof(Cmd_Argv(i)) == Cvar_VariableValue(Cmd_Argv(1)))
				break;
		}
	}

	if (i == Cmd_Argc())
		Cvar_SetStringByName (Cmd_Argv(1), Cmd_Argv(2)); // no match
	else if (i + 1 == Cmd_Argc())
		Cvar_SetStringByName (Cmd_Argv(1), Cmd_Argv(2)); // matched last value in list
	else
		Cvar_SetStringByName (Cmd_Argv(1), Cmd_Argv(i+1)); // matched earlier in list
}

/*
============
Cvar_Reset_f -- johnfitz
============
*/
void Cvar_Reset_f (void)
{
	switch (Cmd_Argc())
	{
	default:
	case 1:
		Con_Printf ("reset <cvar> : reset cvar to default\n");
		break;
	case 2:
		Cvar_Reset (Cmd_Argv(1));
		break;
	}
}

/*
============
Cvar_ResetAll_f -- johnfitz
============
*/
void Cvar_ResetAll_f (void)
{
	cvar_t	*var;

	for (var = cvar_vars; var; var = var->next)
		Cvar_Reset (var->name);
}

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
============
Cvar_Init -- johnfitz
============
*/

void Cvar_Init (void)
{
	Cmd_AddCommand ("cvarlist", Cvar_List_f);
	Cmd_AddCommand ("toggle", Cvar_Toggle_f);
	Cmd_AddCommand ("cycle", Cvar_Cycle_f);
	//Cmd_AddCommand ("inc", Cvar_Inc_f);
	Cmd_AddCommand ("resetcvar", Cvar_Reset_f);
	Cmd_AddCommand ("resetall", Cvar_ResetAll_f);
}

//==============================================================================
//
//  CVAR FUNCTIONS
//
//==============================================================================

// 2000-01-09 CvarList command by Maddes  end

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t	*var;

	for (var=cvar_vars ; var ; var=var->next)
		if (!strcmp (var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return cvar_null_string;
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int			len;

	len = strlen(partial);

	if (!len)
		return NULL;

// check functions
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}

/*
============
Cvar_Reset -- johnfitz
============
*/
void Cvar_Reset (char *name)
{
	cvar_t	*var;

	var = Cvar_FindVar (name);
	if (!var)
		Con_Printf ("variable \"%s\" not found\n", name);
	else
		Cvar_SetStringByName (var->name, var->default_string);
}

/*
============
Cvar_SetStringByName
============
*/
void Cvar_SetStringByName (char *var_name, char *value)
{
	cvar_t	*var;
	qboolean changed;

	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_SetStringByName: variable %s not found\n", var_name);
		return;
	}

	changed = strcmp(var->string, value);

	Z_Free (var->string);	// free the old value string

	var->string = Z_Malloc (strlen(value)+1);
	strcpy (var->string, value);
	var->value = atof (var->string);

	//johnfitz -- during initialization, update default too
	if (!host_initialized)
	{
		Z_Free (var->default_string);
		var->default_string = Z_Malloc (strlen(value)+1);
		strcpy (var->default_string, value);
	}
	//johnfitz

	if ((var->server == 1) && changed)  // JPG - so that server = 2 will mute the variable
	{
		if (sv.active)
			SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", var->name, var->string);
	}

	//johnfitz
	if(var->callback && changed)
		var->callback();
	//johnfitz


	// JPG 3.00 - rcon (64 doesn't mean anything special, but we need some extra space because NET_MAXMESSAGE == RCON_BUFF_SIZE)
	if (rcon_active && (rcon_message.cursize < rcon_message.maxsize - strlen(var->name) - strlen(var->string) - 64))
	{
		rcon_message.cursize--;
		MSG_WriteString(&rcon_message, va("\"%s\" set to \"%s\"\n", var->name, var->string));
	}
#ifdef PROQUAKE_EXTENSION
	// JPG - there's probably a better place for this, but it works.
	if (!strcmp(var_name, "pq_lag"))
	{

		if (var->value < 0)
		{
			Cvar_SetStringByName ("pq_lag", "0");
			return;
		}
		if (var->value > 400)
		{
			Cvar_SetStringByName ("pq_lag", "400");
			return;
		}

		if (var->value == 0 && key_dest == key_menu) // Baker 3.99k: reset to defaults shouldn't trigger the pq_lag msg
			return;


		Cbuf_AddText(va("say \"%cping +%d%c\"\n", 157, (int) var->value, 159));
	}
#endif
}


void Cvar_SetStringByRef (cvar_t *var, char *value) 
{
	Cvar_SetStringByName (var->name, value);
}

/*
============
Cvar_SetValueByRef
============
*/
void Cvar_SetValueByRef (cvar_t *var, float value) 
{
	Cvar_SetValueByName (var->name, value);
}


/*
============
Cvar_SetValueByName
============
*/
void Cvar_SetValueByName (char *var_name, float value)
{
	char	val[32];

	if (value == (int)value)
		snprintf(val, sizeof(val),  "%d", (int)value);
	else
	snprintf (val, sizeof(val), "%f",value);
	Cvar_SetStringByName (var_name, val);
}

/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable, void *function)
{
	char	*oldstr;
	cvar_t	*cursor,*prev; //johnfitz -- sorted list insert

// first check to see if it has already been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, already defined\n", variable->name);	// JPG 3.02 allready->already
		return;
	}

// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}

// copy the value off, because future sets will Z_Free it
	oldstr = variable->string;
	variable->string = Z_Malloc (strlen(variable->string)+1);
	strcpy (variable->string, oldstr);
	variable->value = atof (variable->string);

	//johnfitz -- save initial value for "reset" command
	variable->default_string = Z_Malloc (strlen(variable->string)+1);
	strcpy (variable->default_string, oldstr);
	//johnfitz
// link the variable in

	//johnfitz -- insert each entry in alphabetical order
    if (cvar_vars == NULL || strcmp(variable->name, cvar_vars->name) < 0) //insert at front
	{
	variable->next = cvar_vars;
	cvar_vars = variable;
}
    else //insert later
	{
        prev = cvar_vars;
        cursor = cvar_vars->next;
        while (cursor && (strcmp(variable->name, cursor->name) > 0))
		{
            prev = cursor;
            cursor = cursor->next;
        }
        variable->next = prev->next;
        prev->next = variable;
    }
	//johnfitz

	variable->callback = function; //johnfitz
}




/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;

// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_SetStringByName (v->name, Cmd_Argv(1));
	return true;
}

/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cvar_t	*var;

	fprintf (f, "\n// Variables\n\n");
	for (var = cvar_vars ; var ; var = var->next)
		if (var->archive)
			fprintf (f, "%s \"%s\"\n", var->name, var->string);
}

