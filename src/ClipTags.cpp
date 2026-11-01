#include "stdafx.h"
#include "CP_Main.h"
#include "ClipTags.h"
#include "ClipIds.h"
#include <algorithm>

namespace ClipTags
{
	const TCHAR* const ListColumnSql =
		_T("(SELECT group_concat(Tags.name, ' ') FROM ClipTags INNER JOIN Tags ON Tags.lID = ClipTags.tagID ")
		_T("WHERE ClipTags.clipID = Main.lID) AS tagNames, Main.sourceApp AS sourceApp, ")
		_T("NOT EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID AND Data.strClipBoardFormat IN ('CF_UNICODETEXT', 'CF_TEXT')) AS noText");

	void EnsureSchema(CppSQLite3DB& db)
	{
		db.execDML(_T("CREATE TABLE IF NOT EXISTS Tags(lID INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE)"));
		db.execDML(_T("CREATE TABLE IF NOT EXISTS ClipTags(clipID INTEGER NOT NULL, tagID INTEGER NOT NULL, PRIMARY KEY(clipID, tagID))"));
		db.execDML(_T("CREATE INDEX IF NOT EXISTS ClipTags_TagID ON ClipTags(tagID)"));

		db.execDML(_T("CREATE TRIGGER IF NOT EXISTS ClipTags_delete_clip_trigger AFTER DELETE ON Main FOR EACH ROW\n")
			_T("BEGIN\n")
			_T("DELETE FROM ClipTags WHERE clipID = old.lID;\n")
			_T("END\n"));

		// tags are kept when no clip has them, so they can be created and managed on their own (an early beta deleted unused ones)
		db.execDML(_T("DROP TRIGGER IF EXISTS Tags_delete_unused_trigger"));

		std::vector<CString> columns;
		{
			CppSQLite3Query q = db.execQuery(_T("PRAGMA table_info(Main)"));
			for (; q.eof() == false; q.nextRow())
			{
				columns.push_back(CString(q.getStringField(_T("name"))).MakeLower());
			}
		}
		auto ensureColumn = [&](LPCTSTR name)
		{
			if (std::find(columns.begin(), columns.end(), CString(name).MakeLower()) == columns.end())
			{
				db.execDMLEx(_T("ALTER TABLE Main ADD COLUMN %s TEXT"), name);
			}
		};
		// the program a clip was copied from ("chrome.exe"), for the app: search word and the row label
		ensureColumn(_T("sourceApp"));
		// the words read in an image clip (OCR), for search only: the clip's own description is never replaced
		ensureColumn(_T("ocrText"));
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

	std::vector<std::pair<CString, int>> NamesWithCounts()
	{
		std::vector<std::pair<CString, int>> tags;
		CppSQLite3Query q = theApp.m_db.execQuery(_T("SELECT Tags.name, COUNT(ClipTags.clipID) FROM Tags LEFT JOIN ClipTags ON ClipTags.tagID = Tags.lID ")
			_T("GROUP BY Tags.lID ORDER BY Tags.name"));
		for (; q.eof() == false; q.nextRow())
		{
			tags.push_back(std::make_pair(CString(q.getStringField(0)), q.getIntField(1)));
		}

		return tags;
	}

	bool CreateTag(const CString& name)
	{
		std::vector<CString> names = Parse(name);
		if (names.size() != 1)
		{
			return false;
		}

		theApp.m_db.execDMLEx(_T("INSERT OR IGNORE INTO Tags(name) VALUES('%s')"), (LPCTSTR)names[0]);
		return true;
	}

	std::vector<CString> NamesByUse()
	{
		std::vector<CString> names;
		CppSQLite3Query q = theApp.m_db.execQuery(_T("SELECT Tags.name FROM Tags LEFT JOIN ClipTags ON ClipTags.tagID = Tags.lID ")
			_T("GROUP BY Tags.lID ORDER BY COUNT(ClipTags.clipID) DESC, Tags.name"));
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

	bool RenameTag(const CString& oldName, const CString& newName)
	{
		std::vector<CString> names = Parse(newName);
		if (names.size() != 1)
		{
			return false;
		}

		if (names[0] == oldName)
		{
			return true;
		}

		// names went through Parse, so they hold no quotes
		CppSQLite3Query existing = theApp.m_db.execQueryEx(_T("SELECT lID FROM Tags WHERE name = '%s'"), (LPCTSTR)names[0]);
		if (existing.eof())
		{
			theApp.m_db.execDMLEx(_T("UPDATE Tags SET name = '%s' WHERE name = '%s'"), (LPCTSTR)names[0], (LPCTSTR)oldName);
			return true;
		}

		// merge: give the clips the existing tag, then drop the old one
		int targetId = existing.getIntField(0);
		theApp.m_db.execDMLEx(_T("INSERT OR IGNORE INTO ClipTags(clipID, tagID) SELECT ClipTags.clipID, %d FROM ClipTags INNER JOIN Tags ON Tags.lID = ClipTags.tagID WHERE Tags.name = '%s'"),
			targetId, (LPCTSTR)oldName);
		DeleteTag(oldName);
		return true;
	}

	int DeleteTag(const CString& name)
	{
		int clips = theApp.m_db.execDMLEx(_T("DELETE FROM ClipTags WHERE tagID = (SELECT lID FROM Tags WHERE name = '%s')"), (LPCTSTR)name);
		theApp.m_db.execDMLEx(_T("DELETE FROM Tags WHERE name = '%s'"), (LPCTSTR)name);
		return clips;
	}

	void RemoveTag(int clipId, const CString& name)
	{
		theApp.m_db.execDMLEx(_T("DELETE FROM ClipTags WHERE clipID = %d AND tagID = (SELECT lID FROM Tags WHERE name = '%s')"), clipId, (LPCTSTR)name);
	}

	std::vector<CString> TagsInSearch(const CString& search)
	{
		std::vector<CString> tags;
		int pos = 0;
		for (CString word = search.Tokenize(_T(" "), pos); pos >= 0; word = search.Tokenize(_T(" "), pos))
		{
			std::vector<CString> names = (word.GetLength() > 1 && word[0] == '#') ? Parse(word) : std::vector<CString>();
			if (names.empty() == false)
			{
				tags.push_back(names[0]);
			}
		}

		return tags;
	}

	// type:image / file / link / text and app:<name> search words; "" when the word is not one of them
	static CString FilterForWord(const CString& word)
	{
		const TCHAR* imageSql = _T("EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID AND Data.strClipBoardFormat = 'CF_DIB')");
		const TCHAR* fileSql = _T("EXISTS (SELECT 1 FROM Data WHERE Data.lParentID = Main.lID AND Data.strClipBoardFormat = 'CF_HDROP')");

		CString lower = word;
		lower.MakeLower();
		if (lower == _T("type:image")) return imageSql;
		if (lower == _T("type:file")) return fileSql;
		if (lower == _T("type:link")) return _T("(Main.mText LIKE 'http://%' OR Main.mText LIKE 'https://%' OR Main.mText LIKE 'www.%')");
		if (lower == _T("type:text")) return CString(_T("NOT ")) + imageSql + _T(" AND NOT ") + fileSql;

		if (lower.Left(4) == _T("app:") && lower.GetLength() > 4)
		{
			// only characters an exe name can have, so nothing can break out of the quotes
			CString name;
			for (int i = 4; i < lower.GetLength(); i++)
			{
				TCHAR c = lower[i];
				if (_istalnum(c) || c == '.' || c == '-' || c == '_')
				{
					name += c;
				}
			}
			return name.IsEmpty() ? _T("") : _T("Main.sourceApp LIKE '") + name + _T("%'");
		}

		return _T("");
	}

	CString ExtractSearchFilter(CString& search)
	{
		CString filter;
		CString rest;

		int pos = 0;
		for (CString word = search.Tokenize(_T(" "), pos); pos >= 0; word = search.Tokenize(_T(" "), pos))
		{
			CString wordFilter = FilterForWord(word);
			if (wordFilter.IsEmpty() == FALSE)
			{
				filter += filter.IsEmpty() ? wordFilter : _T(" AND ") + wordFilter;
				continue;
			}

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
