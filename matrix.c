//
// matrix.c
// 
// JPG
//

#include "quakedef.h"

#define MAXMAT	508
#define TIME1	100
#define MINLEN	6
#define _1		15
#define _2		25

typedef struct
{
	int x;
	int y;
	int l;
	int s;
	int r;
} matrix_t;

matrix_t m[MAXMAT];
int mat_time = 0;
int step = 0;
int xres;
int yres;
int n;
int r2;
float savecross;
char s[200][150];

extern viddef_t vid;
extern	cvar_t	crosshair;

#define _0(x,y) x*=y,r2=x>>13,x&=0x1fff,x+=r2,x-=(x>0x1ffe)?0x1fff:0
#define _ (16*i/5>=vid.width)

int dat[8] = {5206,412,7603,2809,1412,206,809,7000};

void Mat_Init_f (void)
{	
	int i;
	int j;

	if (!mat_time)
	{
		savecross = crosshair.value;
		crosshair.value = 0;
	}

	if (cls.state == ca_connected)
	{
		if (key_dest == key_console)
			Con_ToggleConsole_f();
	}
	else
		return;

	mat_time = 1;
	xres = (vid.width / 8) - 2;
	yres = (vid.height / 8) - 5;
	n = (128 + vid.width * 5) / 16;

	for (i = 0 ; i < n ; i++)
	{
		m[i].x = _?((xres/2)-11+(i+8-n)*3):(rand()%xres);
		m[i].y = _?((rand()%(yres/4))+(yres/4)-4):(rand()%(yres/2));
		m[i].l = _?((yres/2)-m[i].y):((rand()%(yres-m[i].y-MINLEN))+MINLEN);
		m[i].s = _?(TIME1+(rand()%32)):((rand()%TIME1)+(i/4));
		m[i].r = _?dat[i+8-n]:((rand()%8190)+1);
		for (j=1;_&&j<m[i].l;j++)_0(m[i].r,5794);
	}
}

void Mat_Update (void)
{
	int i, j, r, t, f;

	if (!mat_time)
		return;
	if (mat_time > TIME1 + 64 + (n / 4) + yres + _2 + _1)
	{
		mat_time = 0;
		crosshair.value = savecross;
		return;
	}
	if (++step == 3)
	{
		step = 0;
		mat_time++;
	}

	memset(s, 0, sizeof(s));
	for (i = 0 ; i < n ; i++)
	{
		if ((mat_time<m[i].s)||(!_&&mat_time>m[i].s+m[i].l+_1+_2))
			continue;

		r = m[i].r;
		for (j = 0 ; j < m[i].l && j <= mat_time - m[i].s ; j++)
		{
			f = _&&j==m[i].l-1;
			t = mat_time-m[i].s-j;
			_0(r,2187);
			(!(t<_1+_2||f))||(s[m[i].x][m[i].y+j]=(f?r:((r&0xf00)==0xf00)?((r+7*(mat_time/4))%10):((r&0xf00)==0xe00)?((r+7*(mat_time/3))%10):(r%10))+((!t||(f&&t<_1+_2))?48:(t<_1&&!f)?18:176));
		}
	}
	for (j = 0 ; j < yres ; j++)
	{
		for (i = 0 ; i < xres ; i++)
		{
			if (s[i][j])
				Draw_Character(8*i+8,8*j,s[i][j]);
		}
	}
}