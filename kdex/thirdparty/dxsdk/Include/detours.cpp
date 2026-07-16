#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include "detours.h"

#if defined(_M_X64) || defined(__x86_64__)
    #include "hde64.h"
    #include "hde64.c"
    typedef hde64s hde_struct;
    #define hde_disasm hde64_disasm
#else
    #include "hde32.h"
    #include "hde32.c"
    typedef hde32s hde_struct;
    #define hde_disasm hde32_disasm
#endif

int DetourASMlen(BYTE *src, int minlen)	
{
	int i,len;
	hde_struct hs;

	for (len=0; len<minlen; len+=i) {
		i=hde_disasm((void *)src, &hs);
		if (i<=0) return 0;
		src+=i;
	}
	return len;
}

void *DetourCreate(BYTE *src, const BYTE *dst, int minlen)
{
	BYTE	*jmp, *org;
	DWORD	dwback;
	DWORD jumpto, newjump;
	int		len;

	len=DetourASMlen(src,(minlen<6)?6:minlen);
	if (len==0 && minlen>=6)  len=minlen;
	if (len==0) return 0;

	org=jmp = (BYTE*)malloc(len+5+1);
    jmp[0]=len;								
    jmp++;

VirtualProtect(src,len,PAGE_READWRITE,&dwback);

 if(src[0] == 0xE9)
 {
  jmp = (BYTE*)malloc(10);
  jumpto = (*(DWORD*)(src+1))+((DWORD)src)+5;
  newjump = (jumpto-(DWORD)(jmp+5));
  jmp[0] = 0xE9;
  *(DWORD*)(jmp+1) = newjump;
  jmp += 5;
  jmp[0] = 0xE9;
  *(DWORD*)(jmp+1) = (DWORD)(src-jmp);
 }
 else
 {
  jmp = (BYTE*)malloc(5+len);
  memcpy(jmp,src,len);
  jmp += len;
  jmp[0] = 0xE9;
  *(DWORD*)(jmp+1) = (DWORD)(src+len-jmp)-5;
 }
 src[0] = 0xE9;
 *(DWORD*)(src+1) = (DWORD)(dst - src) - 5;

 for(int i = 5; i < len; i++)
  src[i] = 0x90;
 VirtualProtect(src,len,dwback,&dwback);
 return (jmp-len);
}

void DetourRemove(BYTE *src, BYTE *restore, int len)
{
	DWORD dwBack;

	len=*(BYTE *)(restore-1);

	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwBack);
	memcpy(src, restore, len);
	restore[0] = 0xE9;

	*(DWORD*)(restore+1) = (DWORD)(src - restore) - 5;
	VirtualProtect(src, len, dwBack, &dwBack);
}
