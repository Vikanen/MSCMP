#include <ctype.h>

#include "MemFunctions.h"
#include "HooksCommon.h"

template <typename FUNC_T>
void InstallCallHook(ptrdiff_t where, FUNC_T func)
{
	WriteValue<unsigned char>(where, 0xE8);
	WriteValue<unsigned>(where + 1, reinterpret_cast<unsigned>(func) - (where + 5));
}

typedef int (_stdcall *InitHook_t)();
InitHook_t originalInitFunction = NULL;

int _stdcall InitHook()
{
	SetupMSCMP();
	return originalInitFunction();
}

/**
 * Install hook for X86 version of the launcher.
 */
void InstallHooks(ptrdiff_t moduleAddress)
{
	ptrdiff_t baseAddress = moduleAddress - 0x400000;

	// Setup addresses.

	originalInitFunction = reinterpret_cast<InitHook_t>(baseAddress + 0x006596C0);
	ptrdiff_t InitHookPos = baseAddress + 0x0065C2AE;
	ptrdiff_t GiveChanceToAttachDebuggerHookPos = baseAddress + 0x005493D3;
	CustomLogCallbackAddress = baseAddress + 0x11E79C4;

	// Install hooks.

	InstallCallHook(InitHookPos, InitHook);
	InstallCallHook(GiveChanceToAttachDebuggerHookPos, SetupDebugger);
}

/* eof */
