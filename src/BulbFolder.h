#pragma once
// Where Bulb keeps a new database, and which folders are its own: only those are ever EFS-encrypted, so a folder
// the user keeps other files in is never marked. Plain strings and no Windows calls, so tests\BulbFolderTest.cpp checks it.
#include <string>
#include <wchar.h>

namespace BulbFolder
{
	inline std::wstring TrimSlashes(std::wstring path)
	{
		while (path.empty() == false && (path.back() == L'\\' || path.back() == L'/'))
		{
			path.pop_back();
		}
		return path;
	}

	// Bulb's app data folder, or any folder named Bulb
	inline bool IsOwn(const std::wstring& folder, const std::wstring& appData)
	{
		std::wstring path = TrimSlashes(folder);
		size_t slash = path.find_last_of(L"\\/");
		std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
		return _wcsicmp(path.c_str(), TrimSlashes(appData).c_str()) == 0 || _wcsicmp(name.c_str(), L"Bulb") == 0;
	}

	// the database moved into a Bulb folder inside the folder it names, unless that folder is already Bulb's
	inline std::wstring Place(const std::wstring& dbPath, const std::wstring& appData)
	{
		size_t slash = dbPath.find_last_of(L"\\/");
		if (slash == std::wstring::npos || IsOwn(dbPath.substr(0, slash + 1), appData))
		{
			return dbPath;
		}
		return dbPath.substr(0, slash + 1) + L"Bulb\\" + dbPath.substr(slash + 1);
	}
}
