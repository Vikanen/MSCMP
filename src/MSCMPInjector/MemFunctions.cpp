#include <Windows.h>

void Unprotect(ptrdiff_t where, size_t count)
{
	DWORD oldProtection = NULL;
	VirtualProtect(reinterpret_cast<void *>(where), count, PAGE_EXECUTE_READWRITE, &oldProtection);
}

/* eof */
