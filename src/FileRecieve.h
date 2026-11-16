#pragma once

// the files behind a file-drop clip (CF_HDROP)
class CFileRecieve
{
public:
	HGLOBAL CreateCF_HDROPBuffer();

	HGLOBAL CreateCF_HDROPBufferAsString();

	void AddFile(CString csFile)	{ m_RecievedFiles.Add(csFile); }

protected:
	CStringArray m_RecievedFiles;
};
