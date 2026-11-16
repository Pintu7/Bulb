#include "stdafx.h"
#include "cp_main.h"
#include "FileRecieve.h"
#include "..\Shared\TextConvert.h"
#include "Path.h"
#include "UnicodeMacros.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

HGLOBAL CFileRecieve::CreateCF_HDROPBufferAsString()
{
	CString data;
	int nFileArraySize = (int) m_RecievedFiles.GetSize();
	for (int i = 0; i < nFileArraySize; i++)
	{
		data += m_RecievedFiles[i];
		data += _T("\r\n");
	}

	HGLOBAL hReturn = NewGlobalP(data.GetBuffer(), (data.GetLength() + 1)*sizeof(TCHAR));

	return hReturn;
}

HGLOBAL CFileRecieve::CreateCF_HDROPBuffer()
{
	int nFileArraySize = (int)m_RecievedFiles.GetSize();
	if(nFileArraySize <= 0)
	{
		LogSendRecieveInfo(_T("Recieved files array is empty not creating cf_hdrop structure"));
		return NULL;
	}

	TCHAR *pBuff = NULL;
	int	 nBuffSize = 0;

	for(int i = 0; i < nFileArraySize; i++)
	{
		nBuffSize += m_RecievedFiles[i].GetLength()+1;
	}

	// Add 1 extra for the final null char,
	// and the size of the DROPFILES struct.
	nBuffSize = sizeof(DROPFILES) + (sizeof(TCHAR) * (nBuffSize + 1));

	pBuff = new TCHAR[nBuffSize];

	ZeroMemory(pBuff, nBuffSize);
	((DROPFILES*)pBuff)->pFiles = sizeof(DROPFILES);
	((DROPFILES*)pBuff)->fWide = TRUE;

	TCHAR* pCurrent = (TCHAR*)(LPBYTE(pBuff) + sizeof(DROPFILES));

	for(int n = 0; n < nFileArraySize; n++)
	{
		STRCPY(pCurrent, (LPCTSTR)m_RecievedFiles[n]);

		LogSendRecieveInfo(StrF(_T("CreateCF_HDROPBuffer adding the file '%s' to local cf_hdrop structure"), pCurrent));

		pCurrent += m_RecievedFiles[n].GetLength(); 
		*pCurrent = 0;
		pCurrent++;
	}

	HGLOBAL hReturn = NewGlobalP(pBuff, nBuffSize);

	delete []pBuff;
	pBuff = NULL;

	return hReturn;
}