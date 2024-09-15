#pragma once

#define STRSAFE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>


#include <filesystem>
#include <cstdint>
#include <vector>
#include <source_location>
#include <functional>
#include <tuple>
#include <cstdint>

namespace Basic::win32
{
	#define WIN32_CHECK(cond) if (!(cond)) { Basic::win32::Error_Exit(TEXT(#cond)); }

	// Copied from https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
	void Error_Exit(LPCTSTR function_name, const std::source_location& location = std::source_location::current());

	std::string Undecorate_Module_Symbol_Name(const std::string& decorated_name, uint32_t flags = UNDNAME_COMPLETE);

	HMODULE Load_DLL(const std::filesystem::path& dll_path);

	bool Unload_DLL(const HMODULE& dll);

	std::vector<std::string> Extract_Module_Export_Symbols(HMODULE hModule);

	struct ExportSymbol { std::string symbol; std::uint32_t ordinal; };
	std::vector<ExportSymbol> Extract_Module_Export_Symbols_Ordinal(HMODULE hModule);

	enum class FileShareFlags : DWORD { Read = FILE_SHARE_READ, Write = FILE_SHARE_WRITE, Delete = FILE_SHARE_DELETE };
	constexpr size_t operator | (const FileShareFlags& a, const FileShareFlags& b) { return (size_t)a | (size_t)b; }

	enum class CreationDisposition : DWORD {
		CreateAlways = CREATE_ALWAYS, CreateNew = CREATE_NEW, OpenAlways = OPEN_ALWAYS,
		OpenExisting = OPEN_EXISTING, TruncateExisting = TRUNCATE_EXISTING
	};

	HANDLE Get_Handle(
		const std::filesystem::path& path_to_file,
		const FileShareFlags& fsf = (FileShareFlags)(FileShareFlags::Read | FileShareFlags::Write),
		const CreationDisposition& cd = CreationDisposition::OpenExisting);

	std::string ReadEntireFile(HANDLE handle_to_file);

	template<typename FuncT>
	std::function<FuncT> Load_Function_From_Module(HMODULE module, const char* symbol)
	{
		return (FuncT*)(GetProcAddress(module, (LPCSTR)symbol));
	}

	template<typename FuncT>
	std::function<FuncT> Load_Function_From_Module(HMODULE module, uint64_t ordinal)
	{
		return (FuncT*)(GetProcAddress(module, (LPCSTR)ordinal));
	}

	template<typename T>
	using Enumerate_DLL_Exports_Callback = bool (*)(
		_In_opt_ T pContext,
		_In_ ULONG nOrdinal,
		_In_opt_ LPCSTR pszName,
		_In_opt_ PVOID pCode);

/*
 This functions code was stolen, borrowed, taken... from https://github.com/microsoft/Detours.
 It's the code from DetourEnumerateExports (https://github.com/microsoft/Detours/wiki/DetourEnumerateExports)
 just renamed and modified a little. I love the MIT license. :) Thanks, Microsoft. I also hate you because
 this shouldn't be this hard.
 */
	template<typename T>
	bool Enumerate_DLL_Exports(
		HMODULE hModule,
		T pContext,
		Enumerate_DLL_Exports_Callback<T> pfExport)
	{
		auto RvaAdjust = [](_Pre_notnull_ PIMAGE_DOS_HEADER pDosHeader, _In_ DWORD raddr) -> PBYTE
			{
				return raddr != 0 ? ((PBYTE)pDosHeader) + raddr : nullptr;
			};

		if (pfExport == nullptr)
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return false;
		}

		auto pDosHeader = (PIMAGE_DOS_HEADER)hModule;

		if (hModule == nullptr)
		{
			pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(nullptr);
		}

		if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		{
			SetLastError(ERROR_BAD_EXE_FORMAT);
			return true;
		}

		auto pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader + pDosHeader->e_lfanew);

		if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
		{
			SetLastError(ERROR_INVALID_EXE_SIGNATURE);
			return false;
		}
		if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0)
		{
			SetLastError(ERROR_EXE_MARKED_INVALID);
			return false;
		}

		auto pExportDir =
			(PIMAGE_EXPORT_DIRECTORY)RvaAdjust(pDosHeader, pNtHeader->OptionalHeader
				.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

		if (pExportDir == nullptr)
		{
			SetLastError(ERROR_EXE_MARKED_INVALID);

			return false;
		}

		PBYTE pExportDirEnd =
			(PBYTE)pExportDir + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

		auto pdwFunctions = (PDWORD)RvaAdjust(pDosHeader, pExportDir->AddressOfFunctions);
		auto pdwNames = (PDWORD)RvaAdjust(pDosHeader, pExportDir->AddressOfNames);
		auto pwOrdinals = (PWORD)RvaAdjust(pDosHeader, pExportDir->AddressOfNameOrdinals);

		for (DWORD nFunc = 0; nFunc < pExportDir->NumberOfFunctions; nFunc++)
		{
			PBYTE pbCode = (pdwFunctions != nullptr)
				? (PBYTE)RvaAdjust(pDosHeader, pdwFunctions[nFunc]) : nullptr;

			PCHAR pszName = nullptr;

			// if the pointer is in the export region, then it is a forwarder.
			if (pbCode > (PBYTE) pExportDir && pbCode < pExportDirEnd)
			{
				pbCode = nullptr;
			}

			for (DWORD n = 0; n < pExportDir->NumberOfNames; n++)
			{
				if (pwOrdinals[n] == nFunc)
				{
					pszName = (pdwNames != nullptr)
						? (PCHAR)RvaAdjust(pDosHeader, pdwNames[n]) : nullptr;
					break;
				}
			}

			ULONG nOrdinal = pExportDir->Base + nFunc;

			if (!pfExport(pContext, nOrdinal, pszName, pbCode))
			{
				break;
			}
		}

		SetLastError(NO_ERROR);

		return true;
	}
}