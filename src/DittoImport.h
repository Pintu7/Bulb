#pragma once
#include <string>

class CppSQLite3DB;

// Importing the clip history of an installed Ditto into Bulb's database.
// Ditto's own file is only ever read: Copy makes a private copy and Import reads that copy.
namespace DittoImport
{
	struct Result
	{
		int clips = 0;      // clips added to Bulb
		int duplicates = 0; // clips skipped because Bulb already has the same content
		int groups = 0;     // groups added (a group Bulb already has, by name, is reused)
		int tagged = 0;     // clips tagged with their group's name
	};

	// copies a database that Ditto may have open, through SQLite's backup, so recent changes are included
	bool Copy(const std::wstring& from, const std::wstring& to, std::wstring& error);

	// groups in the copy that hold at least one clip; -1 when it is not a Ditto database
	int CountGroups(CppSQLite3DB& bulb, const std::wstring& copy);

	// adds the copy's clips and groups to bulb in one transaction: all of them or, on any error, none.
	// Clips already in Bulb (same content checksum) are skipped, imported clips go below Bulb's own,
	// global hotkeys are not carried over. Throws CppSQLite3Exception when it cannot import.
	Result Import(CppSQLite3DB& bulb, const std::wstring& copy, bool groupsToTags);
}
