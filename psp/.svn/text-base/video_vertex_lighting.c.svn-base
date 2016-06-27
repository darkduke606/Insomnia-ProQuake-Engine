// Following code is from the Quake Standards Group
// tutorial "Vertex Lighting" by (c)2001 Orbiter Productions,
// and is protected by the terms and conditions described
// in the GNU General Public License, downloadable at
// http://www.gnu.org/copyleft/gpl.html.

// RIOT - Vertex lighting for models

#include "../quakedef.h"
#include "video_vertex_lighting.h"

byte anorm_pitch[162];
byte anorm_yaw[162];
byte vlighttable[256][256];

float VLight_GetLightValue(int index, float apitch, float ayaw)
{
	int pitchofs, yawofs;
	float retval;

	pitchofs = anorm_pitch[index] + (apitch * 256 / 360);
	yawofs = anorm_yaw[index] + (ayaw * 256 / 360);

	while(pitchofs > 255)
		pitchofs -= 256;
	while(yawofs > 255)
		yawofs -= 256;
	while(pitchofs < 0)
		pitchofs += 256;
	while(yawofs < 0)
		yawofs += 256;

	retval = vlighttable[pitchofs][yawofs];

	return retval / 256;
}

float VLight_LerpLight(int index1, int index2, float ilerp, float apitch, float ayaw)
{
	float lightval1;
	float lightval2;

	lightval1 = VLight_GetLightValue(index1, apitch, ayaw);
	lightval2 = VLight_GetLightValue(index2, apitch, ayaw);

	return (lightval2*ilerp) + (lightval1*(1-ilerp));
}

void VLight_ResetAnormTable()
{
	int i,j;
	vec3_t tempanorms[162] = {
#include "../anorms.h"
	};

	float	forward;
	float	yaw, pitch;
	float	angle;
	float	sp, sy, cp, cy;
	float	precut;
	vec3_t	normal;
	vec3_t	lightvec;

	// Define the light vector here
	angle	= DEG2RAD(vlight_pitch.value);
	sy		= sin(angle);
	cy		= cos(angle);
	angle	= DEG2RAD(-vlight_yaw.value);
	sp		= sin(angle);
	cp		= cos(angle);

	lightvec[0]	= cp*cy;
	lightvec[1]	= cp*sy;
	lightvec[2]	= -sp;

	// First thing that needs to be done is the conversion of the
	// anorm table into a pitch/yaw table

	for(i=0;i<162;i++)
	{
		if (tempanorms[i][1] == 0 && tempanorms[i][0] == 0)
		{
			yaw = 0;
			if (tempanorms[i][2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2(tempanorms[i][1], tempanorms[i][0]) * 57.295779513082320);
			if (yaw < 0)
				yaw += 360;

			forward = sqrt (tempanorms[i][0]*tempanorms[i][0] + tempanorms[i][1]*tempanorms[i][1]);
			pitch = (int) (atan2(tempanorms[i][2], forward) * 57.295779513082320);

			if (pitch < 0)
				pitch += 360;
		}
		anorm_pitch[i] = pitch * 256 / 360;
		anorm_yaw[i] = yaw * 256 / 360;
	}

	// Next, a light value table must be constructed for pitch/yaw offsets
	// DotProduct values

	// DotProduct values never go higher than 2, so store bytes as
	// (product * 127.5)

	if(vlight_highcut.value <= vlight_lowcut.value || vlight_highcut.value > 256 || vlight_highcut.value < 10)
		Cvar_SetValue("vl_highcut", 256);

	if(vlight_lowcut.value >= vlight_highcut.value || vlight_lowcut.value < 0 || vlight_lowcut.value > 250)
		Cvar_SetValue("vl_lowcut", 0);

	for(i=0;i<256;i++)
	{

		angle	= DEG2RAD(i * 360 / 256);
		sy		= sin(angle);
		cy		= cos(angle);

		for(j=0;j<256;j++)
		{
			angle	= DEG2RAD(j * 360 / 256);
			sp		= sin(angle);
			cp		= cos(angle);

			normal[0]	= cp*cy;
			normal[1]	= cp*sy;
			normal[2]	= -sp;

			precut = ((DotProduct(normal, lightvec) + 2) * 63.5);
			precut = (precut - (vlight_lowcut.value)) * 256 / (vlight_highcut.value - vlight_lowcut.value);

			if(precut > 255)
				precut = 255;

			if(precut < 0)
				precut = 0;

			vlighttable[i][j] = precut;
		}
	}
}

void VLight_ChangeLightAngle_f(void)
{
	VLight_ResetAnormTable();

}

void VLight_DumpLightTable_f(void)
{
	COM_WriteFile ("lighttable.raw", vlighttable, 256*256);
}
