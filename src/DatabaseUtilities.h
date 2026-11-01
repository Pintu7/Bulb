// DatabaseUtilites.h: interface for the CDatabaseUtilites class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_)
#define AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DittoPopupWindow.h"
#include "sqlite/CppSQLite3.h"

#define DEFAULT_DB_NAME "Bulb.db"
#define ERROR_OPENING_DATABASE	2

BOOL CreateBackup(CString csPath);
CString GetDBName();
CString GetDefaultDBName();
BOOL OpenDatabase(CString csDB);
// path of the database theApp.m_db last opened, for worker threads that need their own connection
CString CurrentDatabasePath();
// a folder that holds only Bulb's files: Bulb's app data folder, or one named Bulb
bool IsBulbDataFolder(const CString& folder);
// where a new database goes: dbPath itself when its folder is Bulb's, otherwise the same name in a Bulb folder inside it
CString InBulbFolder(const CString& dbPath);
BOOL IsDatabaseOpen();

BOOL CheckDBExists(CString csDBPath);
BOOL ValidDB(CString csPath, BOOL bUpgrade=TRUE);
BOOL CreateDB(CString csPath);

BOOL CompactDatabase();
BOOL RepairDatabase();
BOOL RemoveOldEntries(bool checkIdleTime);
BOOL DeleteNonUsedClips(bool fromAppWindow);

BOOL EnsureDirectory(CString csPath);

BOOL BackupDB(CString dbPath, CString backupPath);
BOOL RestoreDB(CString backupPath);

void ReOrderStickyClips(int parentID, CppSQLite3DB &db);

//BOOL CopyDownDatabase();
//BOOL CopyUpDatabase();

#endif // !defined(AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_)
