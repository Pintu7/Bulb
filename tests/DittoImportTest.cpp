// Checks importing a Ditto database into Bulb's (src/DittoImport.cpp) on databases built here, never a real one.
#include "stdafx.h"
#include "DittoImport.h"
#include "sqlite/CppSQLite3.h"
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
static void Check(bool ok, const char* what)
{
	if (!ok)
	{
		printf("FAIL: %s\n", what);
		failures++;
	}
}

static std::wstring folder;
static std::wstring File(const wchar_t* name) { return folder + name; }

static std::string Bytes(const std::wstring& path)
{
	std::ifstream in(path, std::ios::binary);
	return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Main as Ditto creates it; old = a Ditto from before clipOrder and the sticky and hotkey columns
static void CreateDitto(const std::wstring& path, bool old)
{
	DeleteFile(path.c_str());
	CppSQLite3DB db;
	db.open(path.c_str());
	if (old)
	{
		db.execDML(L"CREATE TABLE Main(lID INTEGER PRIMARY KEY AUTOINCREMENT, lDate INTEGER, mText TEXT, lShortCut INTEGER, lDontAutoDelete INTEGER, CRC INTEGER, bIsGroup INTEGER, lParentID INTEGER, QuickPasteText TEXT)");
	}
	else
	{
		db.execDML(L"CREATE TABLE Main(lID INTEGER PRIMARY KEY AUTOINCREMENT, lDate INTEGER, mText TEXT, lShortCut INTEGER, lDontAutoDelete INTEGER, CRC INTEGER, bIsGroup INTEGER, lParentID INTEGER, QuickPasteText TEXT, "
			L"clipOrder REAL, clipGroupOrder REAL, globalShortCut INTEGER, lastPasteDate INTEGER, stickyClipOrder REAL, stickyClipGroupOrder REAL, MoveToGroupShortCut INTEGER, GlobalMoveToGroupShortCut INTEGER)");
	}
	db.execDML(L"CREATE TABLE Data(lID INTEGER PRIMARY KEY AUTOINCREMENT, lParentID INTEGER, strClipBoardFormat TEXT, ooData BLOB)");
	db.execDML(L"CREATE TABLE Types(lID INTEGER PRIMARY KEY AUTOINCREMENT, TypeText TEXT)");

	auto add = [&](int id, const wchar_t* text, int crc, int group, int parent, int order, int global)
	{
		if (old)
			db.execDMLEx(L"INSERT INTO Main(lID, lDate, mText, lShortCut, lDontAutoDelete, CRC, bIsGroup, lParentID, QuickPasteText) VALUES(%d, %d, '%s', 0, 0, %d, %d, %d, '')", id, 1000 + id, text, crc, group, parent);
		else
			db.execDMLEx(L"INSERT INTO Main(lID, lDate, mText, lShortCut, lDontAutoDelete, CRC, bIsGroup, lParentID, QuickPasteText, clipOrder, globalShortCut, stickyClipOrder) "
				L"VALUES(%d, %d, '%s', 0, 0, %d, %d, %d, '', %d, %d, -2147483647)", id, 1000 + id, text, crc, group, parent, order, global);
		if (group == 0)
			db.execDMLEx(L"INSERT INTO Data(lParentID, strClipBoardFormat, ooData) VALUES(%d, 'CF_UNICODETEXT', X'%02X%02X')", id, id, crc & 0xFF);
	};
	add(1, L"Work Notes", 0, 1, -1, 50, 0);
	add(2, L"Sub", 0, 1, 1, 49, 0);             // a group inside Work Notes
	add(3, L"in work notes", 111, 0, 1, 48, 1); // has a global hotkey in Ditto
	add(4, L"top level", 222, 0, -1, 47, 0);
	add(5, L"bulb has it", 333, 0, -1, 46, 0);
	add(6, L"in sub", 444, 0, 2, 45, 0);
	add(7, L"Empty group", 0, 1, -1, 44, 0);    // no clips: not counted, still imported
}

// Bulb's tables: Ditto's plus sourceApp and the tags, with one clip whose content Ditto also has
static void CreateBulb(CppSQLite3DB& db, const std::wstring& path, bool tags = true)
{
	db.close();
	DeleteFile(path.c_str());
	db.open(path.c_str());
	db.execDML(L"CREATE TABLE Main(lID INTEGER PRIMARY KEY AUTOINCREMENT, lDate INTEGER, mText TEXT, lShortCut INTEGER, lDontAutoDelete INTEGER, CRC INTEGER, bIsGroup INTEGER, lParentID INTEGER, QuickPasteText TEXT, "
		L"clipOrder REAL, clipGroupOrder REAL, globalShortCut INTEGER, lastPasteDate INTEGER, stickyClipOrder REAL, stickyClipGroupOrder REAL, MoveToGroupShortCut INTEGER, GlobalMoveToGroupShortCut INTEGER, sourceApp TEXT)");
	db.execDML(L"CREATE TABLE Data(lID INTEGER PRIMARY KEY AUTOINCREMENT, lParentID INTEGER, strClipBoardFormat TEXT, ooData BLOB)");
	if (tags)
	{
		db.execDML(L"CREATE TABLE Tags(lID INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE)");
		db.execDML(L"CREATE TABLE ClipTags(clipID INTEGER NOT NULL, tagID INTEGER NOT NULL, PRIMARY KEY(clipID, tagID))");
	}
	db.execDML(L"INSERT INTO Main(mText, CRC, bIsGroup, lParentID, clipOrder) VALUES('mine', 333, 0, -1, 10)");
	db.execDML(L"INSERT INTO Data(lParentID, strClipBoardFormat, ooData) VALUES(1, 'CF_UNICODETEXT', X'0505')");
}

static int Scalar(CppSQLite3DB& db, const wchar_t* sql) { return db.execScalar(sql); }

int main()
{
	wchar_t temp[MAX_PATH];
	GetTempPath(MAX_PATH, temp);
	folder = std::wstring(temp) + L"BulbDittoImportTest\\";
	CreateDirectory(folder.c_str(), NULL);

	std::wstring error;
	CreateDitto(File(L"Ditto.db"), false);
	std::string before = Bytes(File(L"Ditto.db"));
	Check(DittoImport::Copy(File(L"Ditto.db"), File(L"copy.db"), error), "copies a Ditto database");
	Check(Bytes(File(L"Ditto.db")) == before, "copying leaves Ditto's file as it was");
	Check(DittoImport::Copy(File(L"missing.db"), File(L"copy2.db"), error) == false && error.empty() == false, "a missing file is an error with a message");

	{
		// Ditto's file open with an unfinished write, as a running Ditto would have it while saving a clip
		CppSQLite3DB ditto;
		ditto.open(File(L"Ditto.db").c_str());
		ditto.execDML(L"BEGIN");
		ditto.execDML(L"INSERT INTO Types(TypeText) VALUES('CF_TEXT')");
		Check(DittoImport::Copy(File(L"Ditto.db"), File(L"copy-open.db"), error), "copies while Ditto has the file open");
		ditto.execDML(L"ROLLBACK");
	}

	CppSQLite3DB bulb;
	CreateBulb(bulb, File(L"Bulb.db"));
	Check(DittoImport::CountGroups(bulb, File(L"copy.db")) == 2, "counts the groups that hold clips");

	DittoImport::Result r = DittoImport::Import(bulb, File(L"copy.db"), true);
	Check(r.clips == 3, "imports the clips Bulb does not have");
	Check(r.duplicates == 1, "skips the clip Bulb already has");
	Check(r.groups == 3, "imports the groups");
	Check(r.tagged == 2, "tags each grouped clip with its group's name");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Main WHERE bIsGroup = 0") == 4, "Bulb's own clip is still there");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Tags WHERE name IN ('work-notes', 'sub')") == 2, "tags are named as Convert groups to tags names them");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Main WHERE bIsGroup = 0 AND mText <> 'mine' AND clipOrder >= 10") == 0, "imported clips go below Bulb's own");
	Check(Scalar(bulb, L"SELECT (SELECT clipOrder FROM Main WHERE mText = 'in work notes') > (SELECT clipOrder FROM Main WHERE mText = 'top level')") == 1, "imported clips keep Ditto's order");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Main WHERE globalShortCut <> 0") == 0, "global hotkeys are not carried over");
	Check(Scalar(bulb, L"SELECT c.lParentID = g.lID FROM Main c, Main g WHERE c.mText = 'in work notes' AND g.mText = 'Work Notes'") == 1, "a clip stays in its group");
	Check(Scalar(bulb, L"SELECT s.lParentID = g.lID FROM Main s, Main g WHERE s.mText = 'Sub' AND g.mText = 'Work Notes'") == 1, "a group inside a group stays inside it");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Main WHERE mText = 'top level' AND lParentID = -1") == 1, "a clip outside groups stays outside");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Data d JOIN Main m ON m.lID = d.lParentID WHERE m.mText = 'top level' AND hex(d.ooData) = '04DE'") == 1, "a clip's data comes with it");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Data") == 4, "no data is copied for skipped clips or groups");

	r = DittoImport::Import(bulb, File(L"copy.db"), true);
	Check(r.clips == 0 && r.duplicates == 4 && r.groups == 0, "importing again adds nothing");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Main WHERE bIsGroup = 1") == 3, "importing again reuses the groups");

	CreateBulb(bulb, File(L"Bulb.db"));
	r = DittoImport::Import(bulb, File(L"copy.db"), false);
	Check(r.clips == 3 && r.tagged == 0 && Scalar(bulb, L"SELECT COUNT(*) FROM Tags") == 0, "without converting, no tags are made");

	CreateDitto(File(L"Old.db"), true);
	Check(DittoImport::Copy(File(L"Old.db"), File(L"old-copy.db"), error), "copies an old Ditto database");
	CreateBulb(bulb, File(L"Bulb.db"));
	r = DittoImport::Import(bulb, File(L"old-copy.db"), true);
	Check(r.clips == 3 && r.duplicates == 1, "imports from an old Ditto without the newer columns");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM Main WHERE mText = 'top level' AND stickyClipOrder = -2147483647") == 1, "an old Ditto's clips are not pinned");

	{
		CppSQLite3DB other;
		DeleteFile(File(L"Other.db").c_str());
		other.open(File(L"Other.db").c_str());
		other.execDML(L"CREATE TABLE Notes(id INTEGER)");
	}
	CreateBulb(bulb, File(L"Bulb.db"));
	Check(DittoImport::CountGroups(bulb, File(L"Other.db")) == -1, "another kind of database is recognised");
	bool threw = false;
	try { DittoImport::Import(bulb, File(L"Other.db"), true); }
	catch (CppSQLite3Exception&) { threw = true; }
	Check(threw, "importing another kind of database fails");

	// a failure halfway (here: no tag tables) leaves Bulb exactly as it was
	CreateBulb(bulb, File(L"Bulb.db"), false);
	threw = false;
	try { DittoImport::Import(bulb, File(L"copy.db"), true); }
	catch (CppSQLite3Exception&) { threw = true; }
	Check(threw && Scalar(bulb, L"SELECT COUNT(*) FROM Main") == 1 && Scalar(bulb, L"SELECT COUNT(*) FROM Data") == 1, "a failed import changes nothing");
	Check(Scalar(bulb, L"SELECT COUNT(*) FROM pragma_database_list WHERE name = 'ditto'") == 0, "the copy is detached afterwards");
	bulb.close();

	if (failures == 0)
	{
		printf("DittoImport: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
