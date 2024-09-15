#include "Basic++/Result.hxx"
#include "Basic++/win32/Win32.hpp"
#include "Basic++/Common.hxx"
#include <dbghelp.h>

using namespace Basic;
using namespace win32;

HMODULE win32::Load_DLL(const std::filesystem::path& dll_path)
{
	auto module_handle = LoadLibraryA(dll_path.string().c_str());

	return module_handle;
}

bool win32::Unload_DLL(const HMODULE& dll)
{
	return FreeLibrary(dll);
}

std::string win32::Undecorate_Module_Symbol_Name(const std::string& decorated_name, uint32_t flags)
{
	char und_name[1000];

	auto undecorated_char_count = UnDecorateSymbolName(
		decorated_name.c_str(), und_name, sizeof(und_name), flags);

	EXPECT(undecorated_char_count > 0, "Function '{}' failed.", stringify(UnDecorateSymbolName));

	return { und_name };
};

HANDLE win32::Get_Handle(
	const std::filesystem::path& path_to_file,
	const FileShareFlags& fsf,
	const CreationDisposition& cd)
{
	HANDLE handle = CreateFileA(
		path_to_file.string().c_str(),
		GENERIC_READ | GENERIC_WRITE,
		(DWORD)fsf,
		NULL,
		(DWORD)cd,
		FILE_ATTRIBUTE_NORMAL,
		NULL

	);

	return handle;
}

void win32::Error_Exit(LPCTSTR function_name, const std::source_location& location)
{
	LPVOID lpMsgBuf{}; // Error message
	LPVOID lpDisplayBuf{};
	DWORD dw = GetLastError(); // Error code
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)function_name) + 45) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s\n\nfailed with error %d: %s\nline %lld, column %lld\n"), function_name, dw, lpMsgBuf, location.line(), location.column());
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
	LocalFree(lpMsgBuf);
	ExitProcess(EXIT_FAILURE);
}



std::string win32::ReadEntireFile(HANDLE handle_to_file)
{
	LARGE_INTEGER file_size{};

	if (!GetFileSizeEx(handle_to_file, &file_size))
		return { "~GetFileSizeEx Failed~" };

	// FIXME: this might be a memory leak, but I feel like std::move should make the
	// 		  string below take ownership of the memory, so... I don't know.
	char* bytes_returned = new char[file_size.QuadPart];

	DWORD number_bytes_of_returned = 0;

	if (!ReadFile(
		handle_to_file,
		bytes_returned,
		file_size.QuadPart,
		&number_bytes_of_returned,
		NULL)) return { "~ReadFile Failed~" };

	std::string output = bytes_returned;

	output.resize(number_bytes_of_returned);

	delete[] bytes_returned;

	return output;
}

std::vector<std::string> win32::Extract_Module_Export_Symbols(HMODULE hModule)
{
	EXPECT(hModule != nullptr and hModule != INVALID_HANDLE_VALUE,
		"Invalid handle passed to {}.", stringify(win32::Extract_Module_Export_Symbols));

	auto enumeration_callback = [](
		_In_opt_ std::vector<std::string>& export_symbols,
		_In_ ULONG nOrdinal,
		_In_opt_ LPCSTR pszName,
		_In_opt_ PVOID pCode) -> bool
		{
			export_symbols.emplace_back(std::string(pszName));

			return true;
		};

	std::vector<std::string> exports;

	Enumerate_DLL_Exports<std::vector<std::string>&>(hModule, exports, enumeration_callback);

	return exports;
}

std::vector<win32::ExportSymbol> win32::Extract_Module_Export_Symbols_Ordinal(HMODULE hModule)
{
	EXPECT(hModule != nullptr and hModule != INVALID_HANDLE_VALUE,
		"Invalid handle passed to {}.", stringify(win32::Extract_Module_Export_Symbols));

	auto enumeration_callback = [](
		_In_opt_ std::vector<win32::ExportSymbol>& export_symbols,
		_In_ ULONG nOrdinal,
		_In_opt_ LPCSTR pszName,
		_In_opt_ PVOID pCode)->bool
		{
			export_symbols.emplace_back(win32::ExportSymbol{ std::string(pszName), (std::uint32_t)nOrdinal });

			return true;
		};

	std::vector<win32::ExportSymbol> exports;

	Enumerate_DLL_Exports<std::vector<win32::ExportSymbol>&>(hModule, exports, enumeration_callback);

	return exports;
}
