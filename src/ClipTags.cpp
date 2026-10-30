#include "stdafx.h"
#include "CP_Main.h"
#include "ClipTags.h"
#include <algorithm>

namespace ClipTags
{
	const TCHAR* const ListColumnSql =
		_T("(SELECT group_concat(Tags.name, ' ') FROM ClipTags INNER JOIN Tags ON Tags.lID = ClipTags.tagID ")
		_T("WHERE ClipTags.clipID = Main.lID) AS tagNames");

	void EnsureSchema(CppSQLite3DB& db)
	{
		db.execDML(_T("CREATE TABLE IF NOT EXISTS Tags(lID INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE)"));
		db.execDML(_T("CREATE TABLE IF NOT EXISTS ClipTags(clipID INTEGER NOT NULL, tagID INTEGER NOT NULL, PRIMARY KEY(clipID, tagID))"));
		db.execDML(_T("CREATE INDEX IF NOT EXISTS ClipTags_TagID ON ClipTags(tagID)"));

		db.execDML(_T("CREATE TRIGGER IF NOT EXISTS ClipTags_delete_clip_trigger AFTER DELETE ON Main FOR EACH ROW\n")
			_T("BEGIN\n")
			_T("DELETE FROM ClipTags WHERE clipID = old.lID;\n")
			_T("END\n"));

		// a tag lives as long as some clip has it
		db.execDML(_T("CREATE TRIGGER IF NOT EXISTS Tags_delete_unused_trigger AFTER DELETE ON ClipTags FOR EACH ROW\n")
			_T("BEGIN\n")
			_T("DELETE FROM Tags WHERE lID = old.tagID AND NOT EXISTS (SELECT 1 FROM ClipTags WHERE tagID = old.tagID);\n")
			_T("END\n"));
	}

	std::vector<CString> Parse(const CString& text)
	{
		std::vector<CString> names;
		CString words = text;
		words.Replace(_T(','), _T(' '));

		int pos = 0;
		for (CString name = words.Tokenize(_T(" \t\r\n"), pos); pos >= 0; name = words.Tokenize(_T(" \t\r\n"), pos))
		{
			// quotes would break the SQL these names go into, the others clash with the list's symbol markers
			for (TCHAR bad : { '#', '\'', '"', '|', '<', '>', '%' })
			{
				name.Remove(bad);
			}
			name.MakeLower();

			if (name.IsEmpty() == FALSE &&
				std::find(names.begin(), names.end(), name) == names.end())
			{
				names.push_back(name);
			}
		}

		return names;
	}

	CString TagsOfClip(int clipId)
	{
		CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT group_concat(Tags.name, ' ') FROM ClipTags INNER JOIN Tags ON Tags.lID = ClipTags.tagID ")
			_T("WHERE ClipTags.clipID = %d"), clipId);

		if (q.eof())
		{
			return _T("");
		}

		return q.getStringField(0);
	}

	std::vector<CString> AllNames()
	{
		std::vector<CString> names;
		CppSQLite3Query q = theApp.m_db.execQuery(_T("SELECT name FROM Tags ORDER BY name"));
		for (; q.eof() == false; q.nextRow())
		{
			names.push_back(q.getStringField(0));
		}

		return names;
	}

	void SetTags(int clipId, const CString& text)
	{
		theApp.m_db.execDMLEx(_T("DELETE FROM ClipTags WHERE clipID = %d"), clipId);
		AddTags(clipId, text);
	}

	void AddTags(int clipId, const CString& text)
	{
		for (const CString& name : Parse(text))
		{
			theApp.m_db.execDMLEx(_T("INSERT OR IGNORE INTO Tags(name) VALUES('%s')"), (LPCTSTR)name);
			theApp.m_db.execDMLEx(_T("INSERT OR IGNORE INTO ClipTags(clipID, tagID) SELECT %d, lID FROM Tags WHERE name = '%s'"), clipId, (LPCTSTR)name);
		}
	}

	CString ExtractSearchFilter(CString& search)
	{
		CString filter;
		CString rest;

		int pos = 0;
		for (CString word = search.Tokenize(_T(" "), pos); pos >= 0; word = search.Tokenize(_T(" "), pos))
		{
			std::vector<CString> names = (word.GetLength() > 1 && word[0] == '#') ? Parse(word) : std::vector<CString>();
			if (names.empty())
			{
				rest += rest.IsEmpty() ? word : _T(" ") + word;
				continue;
			}

			// a prefix matches, so #lav finds lavoro; several #tags must all be on the clip
			if (filter.IsEmpty() == FALSE)
			{
				filter += _T(" AND ");
			}

			filter += StrF(_T("Main.lID IN (SELECT ClipTags.clipID FROM ClipTags INNER JOIN Tags ON Tags.lID = ClipTags.tagID ")
				_T("WHERE substr(Tags.name, 1, %d) = '%s')"), names[0].GetLength(), (LPCTSTR)names[0]);
		}

		if (filter.IsEmpty() == FALSE)
		{
			search = rest;
		}

		return filter;
	}

	int ConvertGroupsToTags()
	{
		std::vector<std::pair<int, CString>> groups;
		{
			CppSQLite3Query q = theApp.m_db.execQuery(_T("SELECT lID, mText FROM Main WHERE bIsGroup = 1"));
			for (; q.eof() == false; q.nextRow())
			{
				groups.push_back(std::make_pair(q.getIntField(0), CString(q.getStringField(1))));
			}
		}

		int converted = 0;
		for (const auto& group : groups)
		{
			// "Work Notes" -> work-notes
			CString name;
			for (const CString& word : Parse(group.second))
			{
				name += name.IsEmpty() ? word : _T("-") + word;
			}

			if (name.IsEmpty())
			{
				continue;
			}

			theApp.m_db.execDMLEx(_T("INSERT OR IGNORE INTO Tags(name) SELECT '%s' WHERE EXISTS (SELECT 1 FROM Main WHERE lParentID = %d AND bIsGroup = 0)"),
				(LPCTSTR)name, group.first);
			theApp.m_db.execDMLEx(_T("INSERT OR IGNORE INTO ClipTags(clipID, tagID) SELECT Main.lID, Tags.lID FROM Main, Tags ")
				_T("WHERE Main.lParentID = %d AND Main.bIsGroup = 0 AND Tags.name = '%s'"), group.first, (LPCTSTR)name);

			converted++;
		}

		return converted;
	}
}
