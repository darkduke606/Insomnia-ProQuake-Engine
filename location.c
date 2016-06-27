//
// location.c
//
// JPG
//
// This entire file is new in proquake.  It is used to translate map areas
// to names for the %l formatting specifier
//

#include "quakedef.h"

#define MAX_LOCATIONS 64

location_t	locations[MAX_LOCATIONS];
int			numlocations = 0;

/*
===============
LOC_LoadLocations

Load the locations for the current level from the location file
===============
*/
void LOC_LoadLocations (void)
{
	FILE *f;
	char *mapname, *ch;
	char filename[64] = "locs/";
	char buff[256];
	location_t *l;
	int i;
	float temp;

	numlocations = 0;
	mapname = cl.worldmodel->name;
	if (strncasecmp(mapname, "maps/", 5))
		return;
	strcpy(filename + 5, mapname + 5);
	ch = strrchr(filename, '.');
	if (ch)
		*ch = 0;
	strlcat (filename, ".loc", sizeof(filename));

	COM_FOpenFile(filename, &f);
	if (!f)
		return;

	l = locations;
	while (!feof(f) && numlocations < MAX_LOCATIONS)
	{
		if (fscanf(f, "%f, %f, %f, %f, %f, %f, ", &l->a[0], &l->a[1], &l->a[2], &l->b[0], &l->b[1], &l->b[2]) == 6)
		{
			l->sd = 0;	// JPG 1.05
			for (i = 0 ; i < 3 ; i++)
			{
				if (l->a[i] > l->b[i])
				{
					temp = l->a[i];
					l->a[i] = l->b[i];
					l->b[i] = temp;
				}
				l->sd += l->b[i] - l->a[i];  // JPG 1.05
			}
			l->a[2] -= 32.0;
			l->b[2] += 32.0;
			fgets(buff, 256, f);

			ch = strrchr(buff, '\n');
			if (ch)
				*ch = 0;
			ch = strrchr(buff, '\"');
			if (ch)
				*ch = 0;
			for (ch = buff ; *ch == ' ' || *ch == '\t' || *ch == '\"' ; ch++);
			strncpy(l->name, ch, 31);
			l = &locations[++numlocations];
		}
		else
			fgets(buff, 256, f);
	}

	fclose(f);
}

/*
===============
LOC_GetLocation

Get the name of the location of a point
===============
*/
// JPG 1.05 - rewrote this to return the nearest rectangle if you aren't in any (manhattan distance)
char *LOC_GetLocation (vec3_t p)
{
	location_t *l;
	location_t *bestloc;
	float dist, bestdist;

	bestloc = NULL;
	bestdist = 999999;
	for (l = locations ; l < locations + numlocations ; l++)
	{
		dist =	fabs(l->a[0] - p[0]) + fabs(l->b[0] - p[0]) +
				fabs(l->a[1] - p[1]) + fabs(l->b[1] - p[1]) +
				fabs(l->a[2] - p[2]) + fabs(l->b[2] - p[2]) - l->sd;

		if (dist < .01)
			return l->name;

		if (dist < bestdist)
		{
			bestdist = dist;
			bestloc = l;
		}
	}
	if (bestloc)
		return bestloc->name;
	return "somewhere";
}

