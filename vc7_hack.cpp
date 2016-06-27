// This should be defined in the C++ runtime.
// But I can't get it to work. This code is taken from isctype.c
#include <ctype.h>

#ifdef _DEBUG
int __cdecl _chvalidator(
        int c,
        int mask
        )
{
        return ( _pctype[c] & mask);
}
#endif