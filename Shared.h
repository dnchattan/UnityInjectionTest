#pragma once
#include <string>

struct Il2CppClass;

template<typename TReturn, typename... TArgs>
struct Method
{
private:
    typedef TReturn(*MethodRef)(TArgs...);
    MethodRef method;
public:
    Method(HMODULE hModule, const std::string& szMethodName)
        : method{ reinterpret_cast<MethodRef>(GetProcAddress(hModule, szMethodName.c_str())) }
    {
    }
    TReturn operator() (TArgs... args)
    {
        return method(args...);
    }
};

const Il2CppClass* FindClass(HMODULE hModuleGasm, const std::string& szNamespace, const std::string& szName);
void InjectSelfToProcessById(DWORD processId);
DWORD FindProcessByName(const std::wstring& wzName);
char* GetBaseAddressOfModule(const std::wstring& wzName);
std::wstring GetPathOfThisModule();