// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Shared.h"
#include "UnityTypes.h"
#include "InjectionHook.h"
#include "Snapshot.h"
#include <string>
#include <codecvt>
#include <tlhelp32.h>
#include <Unknwn.h>

typedef UnityEngine_GameObject_o* (*Method_UnityEngine_GameObject__Find)(System_String_o* name, const MethodInfo* method);

void MakeUnityCall() noexcept
{
	const char* ModuleBaseAddress{ GetBaseAddressOfModule(L"GameAssembly.dll") };
	HMODULE hModuleGasm{ GetModuleHandleA("gameassembly.dll") };
	Method<const MethodInfo*, Il2CppClass*, const char*, int> il2cpp_class_get_method_from_name{ hModuleGasm, "il2cpp_class_get_method_from_name" };
	Method<const Il2CppObject*, const MethodInfo*, void*, void**, void**> il2cpp_runtime_invoke{ hModuleGasm, "il2cpp_runtime_invoke" };

	const Il2CppClass* GameObjectKlass{ FindClass(hModuleGasm, "UnityEngine", "GameObject") };
	const Il2CppClass* StringKlass{ FindClass(hModuleGasm, "System", "String") };
	const MethodInfo* pFind{ il2cpp_class_get_method_from_name(const_cast<Il2CppClass*>(GameObjectKlass), "Find", 1) };
	Method_UnityEngine_GameObject__Find findMethod{ reinterpret_cast<Method_UnityEngine_GameObject__Find>(pFind->methodPointer) };
	size_t cb{ sizeof(System_String_o) + 512 };
	void* strAlloc{ malloc(cb) };
	memset(strAlloc, 0, cb);
	std::wstring canvasStr{ L"Canvas" };
	System_String_o* pStr{ reinterpret_cast<System_String_o*>(strAlloc) };
	pStr->klass = reinterpret_cast<System_String_c*>(const_cast<Il2CppClass*>(StringKlass));
	pStr->monitor = reinterpret_cast<void*>(0x0);
	pStr->fields.m_stringLength = canvasStr.length();
	memcpy(&pStr->fields.m_firstChar, canvasStr.c_str(), canvasStr.length() * 2);
	void* args[1] = { pStr };
	void* pException{};
	const Il2CppObject* pObj{ il2cpp_runtime_invoke(pFind, reinterpret_cast<void*>(const_cast<Il2CppClass*>(GameObjectKlass)), args, &pException) };
	UnityEngine_GameObject_o* result{ (*findMethod)(pStr, pFind) };
}

extern "C"
__declspec(dllexport) LRESULT HandleHookedMessage(int code, WPARAM wParam, LPARAM lParam)
{
	static bool firstRun{ true };
	if (firstRun)
	{
		MakeUnityCall();
	}
	firstRun = false;
	return CallNextHookEx(NULL, code, wParam, lParam);
}

extern "C"
__declspec(dllexport)
void CALLBACK Hook(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter{};
	std::wstring wzInjectionTarget{ converter.from_bytes(lpszCmdLine) };
	
	HMODULE thisModule{};
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		static_cast<LPCWSTR>(static_cast<void*>(&Hook)), &thisModule) == 0)
	{
		return;
	}
	InjectionHook injection{ thisModule, &HandleHookedMessage };
	Snapshot snapshot;
	if (!snapshot.FindProcess(wzInjectionTarget) || !snapshot.FindFirstThread())
		return;
	HookHandle hook{ injection.Hook(WH_GETMESSAGE, snapshot.Thread().th32ThreadID) };

	MessageBoxA(NULL, "Hook injected. Click OK to unhook", "Hook", MB_OK| MB_SYSTEMMODAL);
}



extern "C"
__declspec(dllexport)
void CALLBACK Inject(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter{};
	std::wstring wzInjectionTarget{ converter.from_bytes(lpszCmdLine) };
	DWORD processId{ FindProcessByName(wzInjectionTarget) };
	if (processId == 0)
		return;

	InjectSelfToProcessById(processId);
}


DWORD WINAPI HackThread(HMODULE hModule)
{
	// 47CA30
	/*
	while (true)
	{
		if (GetAsyncKeyState(VK_END) & 1)
		{
			break;
		}
	}*/
	FreeLibraryAndExitThread(hModule, 0);
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	char buf[MAX_PATH];
	GetModuleFileNameA(nullptr, buf, MAX_PATH);
	if (_stricmp(buf, "C:\\Windows\\system32\\rundll32.exe") == 0)
		return TRUE;

	{
		DWORD processId{ GetCurrentProcessId() };
		std::string message{ "Process#" };
		message.append(std::to_string(processId));
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			//CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		}
	}

	return TRUE;
}

