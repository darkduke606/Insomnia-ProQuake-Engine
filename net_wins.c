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
// net_wins.c

#include "quakedef.h"
#include "winquake.h"

extern cvar_t hostname;

#define MAXHOSTNAMELEN		256

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
static struct qsockaddr broadcastaddr;

static unsigned long myAddr;

qboolean	winsock_lib_initialized;

int (PASCAL FAR *pWSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);
int (PASCAL FAR *pWSACleanup)(void);
int (PASCAL FAR *pWSAGetLastError)(void);
SOCKET (PASCAL FAR *psocket)(int af, int type, int protocol);
int (PASCAL FAR *pioctlsocket)(SOCKET s, long cmd, u_long FAR *argp);
int (PASCAL FAR *psetsockopt)(SOCKET s, int level, int optname, const char FAR * optval, int optlen);
int (PASCAL FAR *precvfrom)(SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen);
int (PASCAL FAR *psendto)(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen);
int (PASCAL FAR *pclosesocket)(SOCKET s);
int (PASCAL FAR *pgethostname)(char FAR * name, int namelen);
struct hostent FAR * (PASCAL FAR *pgethostbyname)(const char FAR * name);
struct hostent FAR * (PASCAL FAR *pgethostbyaddr)(const char FAR * addr, int len, int type);
int (PASCAL FAR *pgetsockname)(SOCKET s, struct sockaddr FAR *name, int FAR * namelen);

#include "net_wins.h"

int winsock_initialized = 0;
WSADATA		winsockdata;

//=============================================================================

static double	blocktime;

BOOL PASCAL FAR BlockingHook(void)
{
    MSG		msg;
    BOOL	ret;

	if ((Sys_DoubleTime() - blocktime) > 2.0)
	{
		WSACancelBlockingCall();
		return FALSE;
	}

	// get the next message, if any
    ret = (BOOL) PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

    // if we got one, process it
    if (ret) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // TRUE if we got a message
    return ret;
}

int WINS_iptype(unsigned long ipaddr) {
	// Baker: 3.99e - workaround for mostly DSL/dialup

	// Return 1 if unresolvable/invalid
	// Return 2 if private network (router, LAN, whatever)
	// Return 3 if internet ip

	// TCP/IP addresses reserved for 'private' networks are:
	//10.0.0.0       to     10.255.255.255
	//172.16.0.0     to     172.31.255.255
	//192.168.0.0    to     192.168.255.255
	//
	//and as of July 2001
	//
	//169.254.0.0    to     169.254.255.255 rfc

	if (((ipaddr >> 24) & 0xff) == 0)
		return 1;  // Invalid ip

	if (((ipaddr >> 24) & 0xff) == 169 && ((ipaddr >> 16) & 0xff) == 254)
		return 1;  // Unresolvable private ip

	if (((ipaddr >> 24) & 0xff) == 10)
		return 2;  // Private ip

	if (((ipaddr >> 24) & 0xff) == 172 && ((ipaddr >> 16) & 0xff) >= 16 && ((ipaddr >> 16) & 0xff) <= 31)
		return 2;  // Private ip

	if (((ipaddr >> 24) & 0xff) == 192 && ((ipaddr >> 16) & 0xff) == 168)
		return 2;  // Private ip

	return 3;

}


void WINS_GetLocalAddress()
{
	struct hostent	*local = NULL;
	char			buff[MAXHOSTNAMELEN];
	unsigned long	addr;

	if (myAddr != INADDR_ANY)
		return;

	if (pgethostname(buff, MAXHOSTNAMELEN) == SOCKET_ERROR)
		return;

	blocktime = Sys_DoubleTime();
	WSASetBlockingHook(BlockingHook);
	local = pgethostbyname(buff);
	WSAUnhookBlockingHook();
	if (local == NULL)
		return;

	if (COM_CheckParm("-oldnet") || isDedicated) {
		// Baker: screens for unresolvable ips like 169.254.x.x (dsl modem at best, unresponsive device at worst)

		myAddr = *(int *)local->h_addr_list[0];
		addr = ntohl(myAddr);
		snprintf(my_tcpip_address, sizeof(my_tcpip_address), "%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);

		// Baker: is ip address #0 = 169.254.x.x ?
		if (WINS_iptype(addr) == 1) {
			// Unresolvable private IP or invalid ip
			Con_DPrintf ("WINS_GetLocalAddress: ip address #0 (%s) is invalid, checking next ...\n", my_tcpip_address);

			if (local->h_addr_list[1]) {
				// ip address #1 exists, use it.
				myAddr = *(int *)local->h_addr_list[1];
				addr = ntohl(myAddr);
				snprintf(my_tcpip_address, sizeof(my_tcpip_address), "%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
				Con_DPrintf ("WINS_GetLocalAddress: ip address #1 (%s) found.  Using.\n", my_tcpip_address);
			} else {
				Con_DPrintf("WINS_GetLocalAddress: ip address #1 does not exist\n");
				Con_DPrintf("WINS_GetLocalAddress: Warning continuing with #0\n");
				Con_Printf("Invalid IP (%s): Is internet/network connected?\n", my_tcpip_address);
			}
		}

		return;
	}

	// Baker 3.99e: New: use internet ip over private network ip over unresolvable/invalid ip

	{
		unsigned long	addr0, addr1, addr2;
		int				choice = 0, addrtype0 = 0, addrtype1 = 0, addrtype2 = 0;
		char			tcpip0[NET_NAMELEN];
		char			tcpip1[NET_NAMELEN];
		char			tcpip2[NET_NAMELEN];

		// Baker: get ip addresses

		myAddr = *(int *)local->h_addr_list[0];
		addr0 = ntohl(myAddr);
/*
		addr0 = 0xA9FE416D; // 169.254.65.109
		addr0 = 0xC0A80164; // 192.168.1.100
*/

		addrtype0 = WINS_iptype(addr0);
		snprintf(tcpip0, sizeof(tcpip0), "%d.%d.%d.%d", (addr0 >> 24) & 0xff, (addr0 >> 16) & 0xff, (addr0 >> 8) & 0xff, addr0 & 0xff);

		if (local->h_addr_list[1]) {

			Con_Printf("\nMultiple IPs found\n%s", Con_Quakebar(36));
			Con_Printf("ip #0: %s %s\n", ((addrtype0 == 3) ? "internet     " : ((addrtype0 == 2) ? "local network" : ((addrtype0 == 1) ? "unresolvable " : "no device    "))), tcpip0);

			myAddr = *(int *)local->h_addr_list[1];
			addr1 = ntohl(myAddr);
			addrtype1 = WINS_iptype(addr1);
			snprintf(tcpip1, sizeof(tcpip1), "%d.%d.%d.%d", (addr1 >> 24) & 0xff, (addr1 >> 16) & 0xff, (addr1 >> 8) & 0xff, addr1 & 0xff);
			Con_Printf("ip #1: %s %s\n", ((addrtype1 == 3) ? "internet     " : ((addrtype1 == 2) ? "local network" : ((addrtype1 == 1) ? "unresolvable " : "no device    "))), tcpip1);

			if (addrtype1 >= addrtype0)
				choice = 1;

			if (local->h_addr_list[2]) {

				myAddr = *(int *)local->h_addr_list[2];
				addr2 = ntohl(myAddr);
				addrtype2 = WINS_iptype(addr2);
				snprintf(tcpip2, sizeof(tcpip2), "%d.%d.%d.%d", (addr2 >> 24) & 0xff, (addr2 >> 16) & 0xff, (addr2 >> 8) & 0xff, addr2 & 0xff);
				Con_Printf("ip #2: %s %s\n", ((addrtype2 == 3) ? "internet     " : ((addrtype2 == 2) ? "local network" : ((addrtype2 == 1) ? "unresolvable " : "no device    "))), tcpip2);

				if (addrtype2 >= (choice == 0 ? addrtype0 : addrtype1))
					choice = 2; // else the above stands
			}

			myAddr = *(int *)local->h_addr_list[choice];
			addr = ntohl(myAddr);
			snprintf(my_tcpip_address, sizeof(my_tcpip_address), "%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
			Con_Printf("%s", Con_Quakebar(36));
			Con_Printf("Using ip %s\n", my_tcpip_address);
			Con_Printf("%s\n", Con_Quakebar(36));
		} else {
			myAddr = *(int *)local->h_addr_list[0];
			addr = ntohl(myAddr);
			snprintf(my_tcpip_address, sizeof(my_tcpip_address), "%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
		}
	}

}

int WINS_Init (void)
{
	int		i, r;
	char	*p, buff[MAXHOSTNAMELEN];
	WORD	wVersionRequested;
	HINSTANCE hInst;

// initialize the Winsock function vectors (we do this instead of statically linking
// so we can run on Win 3.1, where there isn't necessarily Winsock)
	if (!(hInst = LoadLibrary(TEXT("wsock32.dll"))))
	{
		Con_SafePrintf ("Failed to load winsock.dll\n");
		winsock_lib_initialized = false;
		return -1;
	}

	winsock_lib_initialized = true;

    pWSAStartup = (void *)GetProcAddress(hInst, "WSAStartup");
    pWSACleanup = (void *)GetProcAddress(hInst, "WSACleanup");
    pWSAGetLastError = (void *)GetProcAddress(hInst, "WSAGetLastError");
    psocket = (void *)GetProcAddress(hInst, "socket");
    pioctlsocket = (void *)GetProcAddress(hInst, "ioctlsocket");
    psetsockopt = (void *)GetProcAddress(hInst, "setsockopt");
    precvfrom = (void *)GetProcAddress(hInst, "recvfrom");
    psendto = (void *)GetProcAddress(hInst, "sendto");
    pclosesocket = (void *)GetProcAddress(hInst, "closesocket");
    pgethostname = (void *)GetProcAddress(hInst, "gethostname");
    pgethostbyname = (void *)GetProcAddress(hInst, "gethostbyname");
    pgethostbyaddr = (void *)GetProcAddress(hInst, "gethostbyaddr");
    pgetsockname = (void *)GetProcAddress(hInst, "getsockname");

    if (!pWSAStartup || !pWSACleanup || !pWSAGetLastError ||
		!psocket || !pioctlsocket || !psetsockopt ||
		!precvfrom || !psendto || !pclosesocket ||
		!pgethostname || !pgethostbyname || !pgethostbyaddr ||
		!pgetsockname)
	{
		Con_SafePrintf ("Couldn't GetProcAddress from winsock.dll\n");
		return -1;
	}

	if (COM_CheckParm ("-noudp"))
		return -1;

	if (winsock_initialized == 0)
	{
		wVersionRequested = MAKEWORD(1, 1);

		if ((r = pWSAStartup (MAKEWORD(1, 1), &winsockdata)))
		{
			Con_SafePrintf ("Winsock initialization failed.\n");
			return -1;
		}
	}
	winsock_initialized++;

	// determine my name
	if (pgethostname(buff, MAXHOSTNAMELEN) == SOCKET_ERROR)
	{
		Con_DPrintf ("Winsock TCP/IP Initialization failed.\n");
		if (--winsock_initialized == 0)
			pWSACleanup ();

		return -1;
	}

	// if the quake hostname isn't set, set it to the machine name
	if (strcmp(hostname.string, "UNNAMED") == 0)
	{
		// see if it's a text IP address (well, close enough)
		for (p = buff; *p; p++)
			if ((*p < '0' || *p > '9') && *p != '.')
				break;

		// if it is a real name, strip off the domain; we only want the host
		if (*p)
		{
			for (i = 0; i < 15; i++)
				if (buff[i] == '.')
					break;
			buff[i] = 0;
		}
		Cvar_SetStringByRef (&hostname, buff);
	}

	if ((i = COM_CheckParm("-ip")))
	{
		if (i < com_argc-1)
		{
			myAddr = inet_addr(com_argv[i+1]);
			if (myAddr == INADDR_NONE)
				Sys_Error ("%s is not a valid IP address", com_argv[i+1]);
			strcpy(my_tcpip_address, com_argv[i+1]);
		}
		else
		{
			Sys_Error ("NET_Init: you must specify an IP address after -ip");
		}
	}
	else
	{
		myAddr = INADDR_ANY;
		strcpy(my_tcpip_address, "INADDR_ANY");
	}

	// JPG 3.02 - support for a trusted qsmack bot
	i = COM_CheckParm ("-qsmack");
	if (i)
	{
		if (i < com_argc-1)
		{
			qsmackAddr = inet_addr(com_argv[i+1]);
			if (qsmackAddr == INADDR_NONE)
				Sys_Error ("%s is not a valid IP address", com_argv[i+1]);
		}
		else
			Sys_Error ("NET_Init: you must specify an IP address after -qsmack");
	}
	else
		qsmackAddr = INADDR_ANY;

	if ((net_controlsocket = WINS_OpenSocket (0)) == -1)
	{
		Con_Printf("WINS_Init: Unable to open control socket\n");
		if (--winsock_initialized == 0)
			pWSACleanup ();
		return -1;
	}

	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
	((struct sockaddr_in *)&broadcastaddr)->sin_port = htons((unsigned short)net_hostport);

	Con_Printf("Winsock TCP/IP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void WINS_Shutdown (void)
{
	WINS_Listen (false);
	WINS_CloseSocket (net_controlsocket);
	if (--winsock_initialized == 0)
		pWSACleanup ();
}

//=============================================================================

void WINS_Listen (qboolean state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		WINS_GetLocalAddress();
		if ((net_acceptsocket = WINS_OpenSocket (net_hostport)) == -1)
			Sys_Error ("WINS_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	WINS_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int WINS_OpenSocket (int port)
{
	int newsocket;
	struct sockaddr_in address;
	u_long _true = 1;

	if ((newsocket = psocket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	if (pioctlsocket (newsocket, FIONBIO, &_true) == -1)
		goto ErrorReturn;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = myAddr;
	address.sin_port = htons((unsigned short)port);
	if( bind (newsocket, (void *)&address, sizeof(address)) == 0)
		return newsocket;

	// Baker: changed this from Sys_Error to Host_Error in JoeQuake in 4.58
	Host_Error ("Unable to bind to %s", WINS_AddrToString((struct qsockaddr *)&address));
ErrorReturn:
	pclosesocket (newsocket);
	return -1;
}

//=============================================================================

int WINS_CloseSocket (int socket)
{
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;

	return pclosesocket (socket);
}


//=============================================================================
/*
============
PartialIPAddress

this lets you type only as much of the net address as required, using
the local network components to fill in the rest
============
*/
static int PartialIPAddress (char *in, struct qsockaddr *hostaddr)
{
	char buff[256], *b;
	int addr, num, mask, run, port;

	buff[0] = '.';
	b = buff;
	strcpy(buff+1, in);
	if (buff[1] == '.')
		b++;

	addr = 0;
	mask=-1;
	while (*b == '.')
	{
		b++;
		num = run = 0;
		while (!( *b < '0' || *b > '9'))
		{
		  num = num*10 + *b++ - '0';
		  if (++run > 3)
		  	return -1;
		}
		if ((*b < '0' || *b > '9') && *b != '.' && *b != ':' && *b != 0)
			return -1;
		if (num < 0 || num > 255)
			return -1;
		mask<<=8;
		addr = (addr<<8) + num;
	}

	if (*b++ == ':')
		port = atoi(b);
	else
		port = net_hostport;

	hostaddr->sa_family = AF_INET;
	((struct sockaddr_in *)hostaddr)->sin_port = htons((short)port);
	((struct sockaddr_in *)hostaddr)->sin_addr.s_addr = (myAddr & htonl(mask)) | htonl(addr);

	return 0;
}
//=============================================================================

int WINS_Connect (int socket, struct qsockaddr *addr)
{
	return 0;
}

//=============================================================================

int WINS_CheckNewConnections (void)
{
	char buf[4096];

	if (net_acceptsocket == -1)
		return -1;

	// JPG - fixed the zero-sized packet bug by changing > to >=
	if (precvfrom (net_acceptsocket, buf, sizeof(buf), MSG_PEEK, NULL, NULL) >= 0)
	{
		return net_acceptsocket;
	}
	return -1;
}

//=============================================================================

int WINS_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int addrlen = sizeof (struct qsockaddr);
	int ret;

	ret = precvfrom (socket, buf, len, 0, (struct sockaddr *)addr, &addrlen);
	if (ret == -1)
	{
		int errno = pWSAGetLastError();

		if (errno == WSAEWOULDBLOCK || errno == WSAECONNREFUSED)
			return 0;

	}

	return ret;
}

//=============================================================================

int WINS_MakeSocketBroadcastCapable (int socket)
{
	int	i = 1;

	// make this socket broadcast capable
	if (psetsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
		return -1;
	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int WINS_Broadcast (int socket, byte *buf, int len)
{
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets");
		WINS_GetLocalAddress();
		ret = WINS_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return WINS_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int WINS_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret;

	ret = psendto (socket, buf, len, 0, (struct sockaddr *)addr, sizeof(struct qsockaddr));
	if (ret == -1)
		if (pWSAGetLastError() == WSAEWOULDBLOCK)
			return 0;

	return ret;
}

//=============================================================================

char *WINS_AddrToString (struct qsockaddr *addr)
{
	static char buffer[22];
	int haddr;

	haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff, ntohs(((struct sockaddr_in *)addr)->sin_port));

	return buffer;
}

//=============================================================================

int WINS_StringToAddr (char *string, struct qsockaddr *addr)
{
	int ha1, ha2, ha3, ha4, hp, ipaddr;

	sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)hp);

	return 0;
}

//=============================================================================

int WINS_GetSocketAddr (int socket, struct qsockaddr *addr)
{
	int addrlen = sizeof(struct qsockaddr);
	unsigned int a;

	memset(addr, 0, sizeof(struct qsockaddr));
	pgetsockname(socket, (struct sockaddr *)addr, &addrlen);
	a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
	if (a == 0 || a == inet_addr("127.0.0.1"))
		((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddr;

	return 0;
}

//=============================================================================

int WINS_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
//	struct hostent *hostentry;

	/* JPG 2.01 - commented this out because it's slow and completely useless
	hostentry = pgethostbyaddr ((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		strncpy (name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}
	*/

	strcpy (name, WINS_AddrToString (addr));

	return 0;
}

//=============================================================================

int WINS_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	struct hostent *hostentry;

	if (name[0] >= '0' && name[0] <= '9')
		return PartialIPAddress (name, addr);

	if (!(hostentry = pgethostbyname (name)))
		return -1;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)net_hostport);
	((struct sockaddr_in *)addr)->sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];

	return 0;
}

//=============================================================================

int WINS_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	if (addr1->sa_family != addr2->sa_family)
		return -1;

	if (((struct sockaddr_in *)addr1)->sin_addr.s_addr != ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
		return -1;

	if (((struct sockaddr_in *)addr1)->sin_port != ((struct sockaddr_in *)addr2)->sin_port)
		return 1;

	return 0;
}

//=============================================================================

int WINS_GetSocketPort (struct qsockaddr *addr)
{
	return ntohs(((struct sockaddr_in *)addr)->sin_port);
}


int WINS_SetSocketPort (struct qsockaddr *addr, int port)
{
	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)port);
	return 0;
}

//=============================================================================
