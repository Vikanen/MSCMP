#include <Windows.h>
#include <process.h>

#include "MonoLoader.h"
#include "MemFunctions.h"

#include <exception>

Mono mono;

void GetModulePath(HMODULE module, char *path)
{
	GetModuleFileName(module, path, MAX_PATH);
	size_t pathLen = strlen(path);
	for (size_t i = pathLen - 1; i > 0; --i) {
		if (path[i] == '\\') {
			path[i] = 0;
			break;
		}
	}
}


static void ShowMessageBox(MonoString *message, MonoString *title)
{
	if (message && title)
	{
		char *messageText = mono.mono_string_to_utf8(message);
		char *titleText = mono.mono_string_to_utf8(title);

		if (messageText && titleText)
		{
			MessageBox(NULL, messageText, titleText, NULL);
		}

		mono.g_free(messageText);
		mono.g_free(titleText);
	}
}

FILE *unityLog = nullptr;

bool RunMP(const char *clientDllPath)
{
	// Register our internal calls.

	mono.mono_add_internal_call ("MSCMP.Client::ShowMessageBox", ShowMessageBox);

	MonoDomain* domain = mono.mono_domain_get();
	if (! domain)
	{
		return false;
	}

	MonoAssembly* domainassembly = mono.mono_domain_assembly_open(domain, clientDllPath);
	if (!domainassembly)
	{
		return false;
	}

	MonoImage* image = mono.mono_assembly_get_image(domainassembly);
	if (!image)
	{
		return false;
	}

	MonoClass* monoClass = mono.mono_class_from_name(image, "MSCMP", "Client");
	if (!monoClass)
	{
		return false;
	}

	MonoMethod* monoClassMethod = mono.mono_class_get_method_from_name(monoClass, "Start", 0);
	if (!monoClassMethod)
	{
		return false;
	}

	// As there is no 'gold method' of verifying that the call succeeded (at least not documented).
	// We trust it and just invoke the method.

	MonoObject *exception = NULL;
	mono.mono_runtime_invoke(monoClassMethod, nullptr, nullptr, nullptr);
	if (exception)
	{
		mono.mono_print_unhandled_exception(exception);
		return false;
	}
	return true;
}

char monoDllPath[MAX_PATH] = { 0 };
char ClientDllPath[MAX_PATH] = { 0 };

extern "C"
void SetupMSCMP()
{
	MessageBox(NULL, "W", NULL, NULL);
	if (!RunMP(ClientDllPath))
	{
		MessageBox(NULL, "Failed to run multiplayer mod!", "MSCMP", MB_ICONERROR);
		ExitProcess(0);
	}
}

extern "C"
void _cdecl SetupDebugger()
{
	if (!getenv("UNITY_GIVE_CHANCE_TO_ATTACH_DEBUGGER"))
	{
		return;
	}

	// First of all attach logger so we will know what happens when mono does not like what we send to it.

	mono.mono_unity_set_vprintf_func([](const char *message, va_list args) -> size_t
	{
		char full_message[2048] = { 0 };
		vsprintf(full_message, message, args);
		va_end(args);
		MessageBox(NULL, full_message, "MSCMP", NULL);

		return 1;
	});

	const char *argv[] = {
		"--debugger-agent=transport=dt_socket,embedding=1,server=y,address=127.0.0.1:56000,defer=y"
	};
	mono.mono_jit_parse_options(1, const_cast<char **>(argv));
	mono.mono_debug_init(MONO_DEBUG_FORMAT_MONO);
}


/**
 * Custom unity log handler.
 */
int _cdecl UnityLog(int a1, const char *message, va_list args)
{
	fprintf(unityLog, "[%i] ", a1);
	vfprintf(unityLog, message, args);
	va_end(args);
#ifdef _DEBUG
	fflush(unityLog);
#endif
	return 0;
}


/** The relative path where common multiplayer mod stuff is stored. */
#define RELATIVE_PATH "\\.."

/** Memory address where unit custom log callback should be set. */
ptrdiff_t CustomLogCallbackAddress = 0;

/**
 * Method used to install architecture dependent hooks.
 *
 * It's implementation can be found in HooksX86.cpp or HooksX64.cpp files.
 */
void InstallHooks(ptrdiff_t moduleAddress);

/**
 * The injector DLL entry point.
 */
BOOL WINAPI DllMain(HMODULE hModule, unsigned Reason, void *Reserved)
{
	switch (Reason) {
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);

		// Setup unity log hook.

		char UnityLogPath[MAX_PATH] = { 0 };
		GetModulePath(GetModuleHandle("MSCMPInjector.dll"), UnityLogPath);
		strcat(UnityLogPath, RELATIVE_PATH "\\unityLog.txt");

		unityLog = fopen(UnityLogPath, "w+");
		if (!unityLog)
		{
			MessageBox(NULL, "Unable to create Unity Log!", "MSCMP", MB_ICONERROR);
			ExitProcess(0);
			return FALSE;
		}

		// Make sure we have mono dll to work with.

		GetModulePath(GetModuleHandle(0), monoDllPath);
		strcat(monoDllPath, "\\mysummercar_Data\\Mono\\mono.dll");

		if (GetFileAttributes(monoDllPath) == INVALID_FILE_ATTRIBUTES)
		{
			MessageBox(NULL, "Unable to find mono.dll!", "MSCMP", MB_ICONERROR);
			ExitProcess(0);
			return FALSE;
		}

		// Now make sure we have client file. Do it here so we will not do any redundant processing.

		GetModulePath(GetModuleHandle("MSCMPInjector.dll"), ClientDllPath);
		strcat(ClientDllPath, RELATIVE_PATH "\\MSCMPClient.dll");

		if (GetFileAttributes(ClientDllPath) == INVALID_FILE_ATTRIBUTES)
		{
			MessageBox(NULL, "Unable to find MSC MP Client files!", "MSCMP", MB_ICONERROR);
			ExitProcess(0);
			return FALSE;
		}

		if (!mono.Setup(monoDllPath))
		{
			MessageBox(NULL, "Unable to setup mono loader!", "MSCMP", MB_ICONERROR);
			ExitProcess(0);
			return FALSE;
		}

		ptrdiff_t moduleAddress = reinterpret_cast<ptrdiff_t>(GetModuleHandle(NULL));
		InstallHooks(moduleAddress);

		// Common memory operations:

		// Set custom log callback.

		WriteValue<ptrdiff_t>(CustomLogCallbackAddress, reinterpret_cast<ptrdiff_t>(UnityLog));
	}
	break;

	case DLL_PROCESS_DETACH:
		if (unityLog)
		{
			fclose(unityLog);
			unityLog = nullptr;
		}
		break;
	}
	return TRUE;
}
