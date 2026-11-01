#pragma once

#include <vector>

class CppSQLite3DB;

// Tags: many-to-many labels on clips (fork), stored in the Tags and ClipTags tables.
// Names are lower case, one word each, without # (shown and searched as #name).
namespace ClipTags
{
	// column for the Main list queries: the clip's tag names separated by spaces, aliased tagNames
	extern const TCHAR* const ListColumnSql;

	void EnsureSchema(CppSQLite3DB& db);

	// "Lavoro, #server x" -> { "lavoro", "server", "x" }
	std::vector<CString> Parse(const CString& text);

	CString TagsOfClip(int clipId);
	std::vector<CString> AllNames();
	// every tag with how many clips have it, by name
	std::vector<std::pair<CString, int>> NamesWithCounts();
	// adds a tag no clip has yet; false if name is not a single valid word
	bool CreateTag(const CString& name);
	// most used first, unused tags last
	std::vector<CString> NamesByUse();

	// replaces the clip's tags with the ones in text
	void SetTags(int clipId, const CString& text);
	void AddTags(int clipId, const CString& text);
	void RemoveTag(int clipId, const CString& name);

	// renames a tag on every clip; when newName already exists the two tags are merged. Returns false if newName is not a valid name
	bool RenameTag(const CString& oldName, const CString& newName);
	// removes a tag from every clip (the clips stay). Returns how many clips had it
	int DeleteTag(const CString& name);

	// the tag names of the #tag words in a search
	std::vector<CString> TagsInSearch(const CString& search);

	// removes the #tag words from search and returns the SQL condition they make ("" when there are none)
	CString ExtractSearchFilter(CString& search);

	// tags every clip with the name of the group it is in; groups are left as they are. Returns the groups converted.
	int ConvertGroupsToTags();
}
