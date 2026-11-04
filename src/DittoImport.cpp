#include "stdafx.h"
#include "DittoImport.h"
#include "sqlite/CppSQLite3.h"
#include "SearchTags.h"
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

namespace
{
	const int InvalidSticky = -(2147483647); // Misc.h INVALID_STICKY: not pinned

	std::string Utf8(const std::wstring& text)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
		std::string result(size > 0 ? size - 1 : 0, '\0');
		if (size > 1)
		{
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], size, NULL, NULL);
		}
		return result;
	}

	std::wstring Lower(std::wstring text)
	{
		for (wchar_t& c : text) c = towlower(c);
		return text;
	}

	std::set<std::wstring> ColumnsOf(CppSQLite3DB& db, const wchar_t* table)
	{
		std::set<std::wstring> columns;
		CppSQLite3Query q = db.execQuery((std::wstring(L"PRAGMA ditto.table_info(") + table + L")").c_str());
		for (; q.eof() == false; q.nextRow())
		{
			columns.insert(Lower(q.getStringField(L"name")));
		}
		return columns;
	}

	bool Has(const std::set<std::wstring>& columns, const wchar_t* name)
	{
		return columns.count(Lower(name)) > 0;
	}

	// a Ditto database has had these since its first versions; the other columns came later and may be missing
	bool IsDittoDatabase(const std::set<std::wstring>& main, const std::set<std::wstring>& data)
	{
		for (const wchar_t* name : { L"lID", L"mText", L"CRC", L"bIsGroup", L"lParentID" })
		{
			if (Has(main, name) == false) return false;
		}
		for (const wchar_t* name : { L"lParentID", L"strClipBoardFormat", L"ooData" })
		{
			if (Has(data, name) == false) return false;
		}
		return true;
	}

	// attaches the copy as "ditto" for as long as it lives
	struct Attached
	{
		CppSQLite3DB& db;
		Attached(CppSQLite3DB& bulb, const std::wstring& copy) : db(bulb)
		{
			std::wstring quoted;
			for (wchar_t c : copy) { quoted += c; if (c == L'\'') quoted += L'\''; }
			db.execDML((L"ATTACH DATABASE '" + quoted + L"' AS ditto").c_str());
		}
		~Attached()
		{
			try { db.execDML(L"DETACH DATABASE ditto"); }
			catch (CppSQLite3Exception&) {}
		}
	};

	// "Work Notes" -> "work-notes", as the Convert groups to tags command names them
	std::wstring TagName(const std::wstring& group)
	{
		std::wstring name;
		for (const std::wstring& word : SearchTags::Parse(group))
		{
			name += name.empty() ? word : L"-" + word;
		}
		return name;
	}
}

bool DittoImport::Copy(const std::wstring& from, const std::wstring& to, std::wstring& error)
{
	sqlite3* source = NULL;
	sqlite3* target = NULL;
	int rc = sqlite3_open_v2(Utf8(from).c_str(), &source, SQLITE_OPEN_READONLY, NULL);
	if (rc == SQLITE_OK)
	{
		rc = sqlite3_open_v2(Utf8(to).c_str(), &target, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	}
	if (rc == SQLITE_OK)
	{
		sqlite3_backup* backup = sqlite3_backup_init(target, "main", source, "main");
		if (backup == NULL)
		{
			rc = sqlite3_errcode(target);
		}
		else
		{
			// Ditto may be writing a clip right now: wait for it, up to about 5 seconds
			for (int attempt = 0; attempt < 50; attempt++)
			{
				rc = sqlite3_backup_step(backup, -1);
				if (rc != SQLITE_BUSY && rc != SQLITE_LOCKED) break;
				Sleep(100);
			}
			sqlite3_backup_finish(backup);
			rc = rc == SQLITE_DONE ? SQLITE_OK : rc;
		}
	}
	if (rc != SQLITE_OK)
	{
		error = std::wstring(CA2W(sqlite3_errstr(rc)));
	}
	sqlite3_close(source);
	sqlite3_close(target);
	return rc == SQLITE_OK;
}

int DittoImport::CountGroups(CppSQLite3DB& bulb, const std::wstring& copy)
{
	Attached attached(bulb, copy);
	if (IsDittoDatabase(ColumnsOf(bulb, L"Main"), ColumnsOf(bulb, L"Data")) == false)
	{
		return -1;
	}
	return bulb.execScalar(L"SELECT COUNT(*) FROM ditto.Main AS g WHERE g.bIsGroup = 1 AND "
		L"EXISTS (SELECT 1 FROM ditto.Main AS c WHERE c.lParentID = g.lID AND c.bIsGroup = 0)");
}

DittoImport::Result DittoImport::Import(CppSQLite3DB& bulb, const std::wstring& copy, bool groupsToTags)
{
	Result result;
	Attached attached(bulb, copy);
	std::set<std::wstring> main = ColumnsOf(bulb, L"Main");
	if (IsDittoDatabase(main, ColumnsOf(bulb, L"Data")) == false)
	{
		throw CppSQLite3Exception(SQLITE_MISMATCH, (TCHAR*)L"Not a Ditto database", false);
	}

	// Bulb's Main columns filled from Ditto; ?1 is the new clipOrder, ?2 the new parent, ?3 the Ditto lID
	struct Column { const wchar_t* name; std::wstring value; };
	std::wstring sticky = std::to_wstring(InvalidSticky);
	std::vector<Column> columns = {
		{ L"lDate", L"0" }, { L"mText", L"''" }, { L"lShortCut", L"0" }, { L"lDontAutoDelete", L"0" }, { L"CRC", L"0" },
		{ L"bIsGroup", L"0" }, { L"lParentID", L"?2" }, { L"QuickPasteText", L"''" }, { L"clipOrder", L"?1" },
		{ L"clipGroupOrder", L"0" }, { L"globalShortCut", L"0" }, { L"lastPasteDate", L"0" },
		{ L"stickyClipOrder", sticky }, { L"stickyClipGroupOrder", sticky },
		{ L"MoveToGroupShortCut", L"0" }, { L"GlobalMoveToGroupShortCut", L"0" } };
	std::wstring names, values;
	for (const Column& column : columns)
	{
		bool fixed = column.value[0] == L'?' || wcscmp(column.name, L"globalShortCut") == 0 || wcscmp(column.name, L"GlobalMoveToGroupShortCut") == 0;
		names += (names.empty() ? L"" : L", ") + std::wstring(column.name);
		// a global hotkey from Ditto would collide with Ditto's own while both run, so imported clips keep theirs in Bulb only
		values += (values.empty() ? L"" : L", ") + (fixed || Has(main, column.name) == false ? column.value : std::wstring(column.name));
	}
	std::wstring order = Has(main, L"clipOrder") ? L"clipOrder DESC, lID DESC" : L"lID DESC";

	bulb.execDML(L"BEGIN");
	try
	{
		CppSQLite3Statement insertMain = bulb.compileStatement((L"INSERT INTO main.Main(" + names + L") SELECT " + values + L" FROM ditto.Main WHERE lID = ?3").c_str());
		CppSQLite3Statement insertData = bulb.compileStatement(L"INSERT INTO main.Data(lParentID, strClipBoardFormat, ooData) "
			L"SELECT ?1, strClipBoardFormat, ooData FROM ditto.Data WHERE lParentID = ?2 ORDER BY lID");

		// imported rows go below Bulb's own, keeping Ditto's order among themselves
		double next = 0;
		{
			CppSQLite3Query q = bulb.execQuery(L"SELECT MIN(clipOrder) FROM main.Main");
			next = q.eof() || q.fieldIsNull(0) ? 0 : q.getFloatField(0);
		}

		// groups first, so clips can be placed in them; a group Bulb already has by that name is reused
		std::map<int, int> groupOf;          // Ditto group lID -> Bulb lID
		std::map<int, std::wstring> nameOf;  // Ditto group lID -> name
		std::vector<std::pair<int, int>> added; // Bulb lID, Ditto parent of groups added now
		{
			CppSQLite3Statement existing = bulb.compileStatement(L"SELECT lID FROM main.Main WHERE bIsGroup = 1 AND mText = ?1 LIMIT 1");
			CppSQLite3Query q = bulb.execQuery((L"SELECT lID, mText, lParentID FROM ditto.Main WHERE bIsGroup = 1 ORDER BY " + order).c_str());
			for (; q.eof() == false; q.nextRow())
			{
				int dittoId = q.getIntField(0);
				std::wstring name = q.getStringField(1);
				nameOf[dittoId] = name;

				existing.bind(1, name.c_str());
				CppSQLite3Query found = existing.execQuery();
				if (found.eof() == false)
				{
					groupOf[dittoId] = found.getIntField(0);
				}
				else
				{
					insertMain.bind(1, next -= 1);
					insertMain.bind(2, -1);
					insertMain.bind(3, dittoId);
					insertMain.execDML();
					insertMain.reset();
					groupOf[dittoId] = (int)bulb.lastRowId();
					added.push_back(std::make_pair(groupOf[dittoId], q.getIntField(2)));
					result.groups++;
				}
				found.finalize();
				existing.reset();
			}
		}
		for (const auto& group : added)
		{
			auto parent = groupOf.find(group.second);
			if (parent != groupOf.end())
			{
				bulb.execDMLEx(L"UPDATE main.Main SET lParentID = %d WHERE lID = %d", parent->second, group.first);
			}
		}

		std::unordered_set<int> known;
		{
			CppSQLite3Query q = bulb.execQuery(L"SELECT CRC FROM main.Main WHERE bIsGroup = 0");
			for (; q.eof() == false; q.nextRow())
			{
				known.insert(q.getIntField(0));
			}
		}

		CppSQLite3Statement addTag = bulb.compileStatement(L"INSERT OR IGNORE INTO main.Tags(name) VALUES(?1)");
		CppSQLite3Statement tagClip = bulb.compileStatement(L"INSERT OR IGNORE INTO main.ClipTags(clipID, tagID) SELECT ?1, lID FROM main.Tags WHERE name = ?2");

		CppSQLite3Query q = bulb.execQuery((L"SELECT lID, CRC, lParentID FROM ditto.Main WHERE bIsGroup = 0 ORDER BY " + order).c_str());
		for (; q.eof() == false; q.nextRow())
		{
			int dittoId = q.getIntField(0);
			int crc = q.getIntField(1);
			if (crc != 0 && known.insert(crc).second == false)
			{
				result.duplicates++;
				continue;
			}

			auto group = groupOf.find(q.getIntField(2));
			insertMain.bind(1, next -= 1);
			insertMain.bind(2, group != groupOf.end() ? group->second : -1);
			insertMain.bind(3, dittoId);
			insertMain.execDML();
			insertMain.reset();
			int clipId = (int)bulb.lastRowId();

			insertData.bind(1, clipId);
			insertData.bind(2, dittoId);
			insertData.execDML();
			insertData.reset();
			result.clips++;

			if (groupsToTags && group != groupOf.end())
			{
				std::wstring tag = TagName(nameOf[group->first]);
				if (tag.empty() == false)
				{
					addTag.bind(1, tag.c_str());
					addTag.execDML();
					addTag.reset();
					tagClip.bind(1, clipId);
					tagClip.bind(2, tag.c_str());
					result.tagged += tagClip.execDML();
					tagClip.reset();
				}
			}
		}
		q.finalize();
		insertMain.finalize();
		insertData.finalize();
		addTag.finalize();
		tagClip.finalize();

		bulb.execDML(L"COMMIT");
	}
	catch (...)
	{
		try { bulb.execDML(L"ROLLBACK"); }
		catch (CppSQLite3Exception&) {}
		throw;
	}
	return result;
}
