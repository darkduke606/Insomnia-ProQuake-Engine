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
// mathlib.h
#ifdef PSP_HARDWARE_VIDEO
#include <pspgu.h>
#endif

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

typedef	int	fixed4_t;
typedef	int	fixed8_t;
typedef	int	fixed16_t;

#ifndef M_PI
#if defined(GU_PI)
#define M_PI = GU_PI	// matches value in gcc v2 math.h
#else
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif
#endif

#define M_PI_DIV_180 (M_PI / 180.0) //johnfitz
#define DEG2RAD( a ) ( (a) * M_PI_DIV_180 ) //johnfitz

struct mplane_s;

extern vec3_t vec3_origin;
extern	int nanmask;

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

#define Q_rint(x) ((x) > 0 ? (int)((x) + 0.5) : (int)((x) - 0.5))
#define CLAMP(min, x, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x)) //johnfitz

#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c) {c[0]=a[0]-b[0];c[1]=a[1]-b[1];c[2]=a[2]-b[2];}
#define VectorAdd(a,b,c) {c[0]=a[0]+b[0];c[1]=a[1]+b[1];c[2]=a[2]+b[2];}
#define VectorCopy(a,b) {b[0]=a[0];b[1]=a[1];b[2]=a[2];}
#define VectorClear(a)		((a)[0] = (a)[1] = (a)[2] = 0)
#define VectorNegate(a, b)	((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])


// MDave -- courtesy of johnfitz, lordhavoc
#define VectorNormalizeFast(_v)\
{\
	float _y, _number;\
	_number = DotProduct(_v, _v);\
	if (_number != 0.0)\
	{\
		*((long *)&_y) = 0x5f3759df - ((* (long *) &_number) >> 1);\
		_y = _y * (1.5f - (_number * 0.5f * _y * _y));\
		VectorScale(_v, _y, _v);\
	}\
}


#ifndef PSP
#define sqrtf(x) sqrt(x)
#define cosf(x) cos(x)
#define sinf(x) sin(x)
#define atanf(x) atan(x)
#define tanf(x) tan(x)
#define floorf(x) floor(x)
#define ceilf(x) ceil(x)
#define fabsf(x) fabs(x)
#define powf(x,y) pow(x,y)

#ifdef FLASH
//For Flash, we need to swap round the arguments for atan2
#define atan2f(x,y) atan2(y,x)
#else
#define atan2f(x,y) atan2(x,y)
#endif
#endif

void VectorMA (vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

vec_t _DotProduct (vec3_t v1, vec3_t v2);
void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy (vec3_t in, vec3_t out);

int VectorCompare (vec3_t v1, vec3_t v2);
vec_t VectorLength (vec3_t v);
float VecLength2(vec3_t v1, vec3_t v2);
void LerpVector (const vec3_t from, const vec3_t to, float frac, vec3_t out);

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross);

void VectorInverse (vec3_t v);
void VectorScale (vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);

//#ifdef PSP_RECONCILE
//void vectoangles (vec3_t vec, vec3_t ang);
//#endif
int ParseFloats(char *s, float *f, int *f_size);



float	anglemod(float a);



#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

//#ifdef SUPPORTS_AUTOID
#define PlaneDiff(point, plane) (				\
	(((plane)->type < 3) ? (point)[(plane)->type] - (plane)->dist : DotProduct((point), (plane)->normal) - (plane)->dist)	\
)
//#endif

float VectorNormalize (vec3_t v);		// returns vector length

void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4]);

//#ifdef PSP_RECONCILE
// Prototypes added by PM.
//void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
//void VectorVectors (vec3_t forward, vec3_t right, vec3_t up);
//#endif
void FloorDivMod (double numer, double denom, int *quotient, int *rem);

fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor (int i1, int i2);

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct mplane_s *plane);

#ifdef SUPPORTS_AUTOID_SOFTWARE
float *Matrix4_NewRotation(float a, float x, float y, float z);
float *Matrix4_NewTranslation(float x, float y, float z);
void Matrix4_Multiply(float *a, float *b, float *out);
void Matrix4_Transform4(float *matrix, float *vector, float *product);
void ML_ProjectionMatrix(float *proj, float wdivh, float fovy);
void ML_ModelViewMatrix(float *modelview, vec3_t viewangles, vec3_t vieworg);
void ML_Project (vec3_t in, vec3_t out, vec3_t viewangles, vec3_t vieworg, float wdivh, float fovy);
#endif
