#include <Windows.h>

#include "log.h"
#include "../detourxs-master/detourxs.h"
#include "f4se_common/f4se_version.h"
#include "f4se_common/Utilities.h"


typedef void(*_ProcessEventQueue_Internal) (void * thisPtr);

_ProcessEventQueue_Internal orig_ProcessEventQueue_Internal = nullptr;

void UpdateActors();

void hk_ProcessEventQueue_Internal(void *thisPtr)
{
	orig_ProcessEventQueue_Internal(thisPtr);
	UpdateActors();
}

// See f4se/Hooks_Threads.cpp
// TODO someday address library.  someday.  
// issue is with C++ standards differences between the projects. [RM]
RelocPtr <void*> ProcessEventQueue_Internal(0x1B1E2F0);

/*
[RickM:] 
Address library data: use addresslibdecoder.exe on their 'bin' files to get this data. Looking for index 2287625
version-1-10-984-0.txt:207049:2287625   1A09CB0
version-1-11-137-0.txt:198184:2287625   1B1DFA0
1-11-159 0x1B18B40
1-11-169 1B194C0
version-1-11-191-0.txt:198154:2287625   1B1DD10
version-1-11-221-0.txt:198145: 2287625  1B1DE30
version-1-11-240-0.txt: 2287625 1B1E2F0
*/



DetourXS renderDetour;

void DoHook() {
	logger.Info("Attempting Game Hook\n");

	renderDetour.Create((LPVOID)ProcessEventQueue_Internal.GetPtr(), hk_ProcessEventQueue_Internal, (LPVOID*)(&orig_ProcessEventQueue_Internal));
}
