#include <ctype.h>
#include <cstring>

#include "MemFunctions.h"
#include "HooksCommon.h"

#include "MinHook.h"

extern "C" {
	ptrdiff_t InitFuncReturnAddress = 0;
	ptrdiff_t ModuleManager__Get = 0;
	ptrdiff_t ModuleManager__Load = 0;

	void InitHookJmp();

	ptrdiff_t GiveChanceToAttachDebuggerHookReturnAddress = 0;
	ptrdiff_t sub_1401736F0 = 0;

	void GiveChanceToAttachDebuggerHookReturnJmp();
}

template <typename FUNC_T>
void InstallJmpHook(ptrdiff_t where, FUNC_T func)
{
	const size_t HOOK_SIZE = 16;
	const size_t ADDRESS_OFFSET = 3;
	const size_t ADDRESS_SIZE = 8;

	static const unsigned char hookData[HOOK_SIZE] = {
		/*                            0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09 */
		/* push  rax               */ 0x50,
		/* mov   rax, address      */ 0x48, 0xb8, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
		/* xchg  [rsp], rax        */ 0x48, 0x87, 0x04, 0x24,
		/* ret                     */ 0xc3
	};


	static_assert(sizeof(ptrdiff_t) == ADDRESS_SIZE, "ptrdiff_t size does not matches address size");
	void *hookLocation = reinterpret_cast<void *>(where);

	Unprotect(where, HOOK_SIZE);
	memcpy(hookLocation, hookData, HOOK_SIZE);
	WriteValue<ptrdiff_t>(where + ADDRESS_OFFSET, reinterpret_cast<ptrdiff_t>(func));
}


char (__fastcall *OriginalInitHookFunc)(__int64 a1, __int64 a2, __int64 a3) = 0;
char InitHookFunc(__int64 a1, __int64 a2, __int64 a3)
{
	char ret = OriginalInitHookFunc(a1, a2, a3);
	SetupMSCMP();
	return ret;
}

/**
 * Install hooks for X64 version of the launcher.
 */
void _cdecl InstallHooks(ptrdiff_t moduleAddress)
{
	ptrdiff_t baseAddress = moduleAddress - 0x140000000;

	// Setup offsets

	ptrdiff_t InitHookPos = baseAddress + 0x00000001400304C0;
	InitFuncReturnAddress = baseAddress + 0x00000001401FF480;

	ptrdiff_t GiveChanceToAttachDebuggerHookPos = baseAddress + 0x0000000140173996;
	GiveChanceToAttachDebuggerHookReturnAddress = baseAddress + 0x00000001401739A6;
	sub_1401736F0 = baseAddress + 0x1401736F0;
	CustomLogCallbackAddress = baseAddress + 0x14106FF58;

	// Install initialization hook.

	InstallJmpHook(InitHookPos, InitHookJmp);

	// Install command line init hook used to install debugger.

	InstallJmpHook(GiveChanceToAttachDebuggerHookPos, GiveChanceToAttachDebuggerHookReturnJmp);
}

/* eof */
