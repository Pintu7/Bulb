#include "stdafx.h"
#include "UpdateCheck.h"
#include <winhttp.h>
#include <thread>

#pragma comment(lib, "winhttp.lib")

namespace
{
	// GET https://api.github.com/repos/Pintu7/Bulb/releases?per_page=1; true with the body when GitHub answers 200
	bool Download(std::string& body)
	{
		bool ok = false;
		HINTERNET session = WinHttpOpen(L"Bulb", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		HINTERNET connection = session ? WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0) : NULL;
		HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET", L"/repos/Pintu7/Bulb/releases?per_page=1", NULL,
			WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : NULL;
		if (request != NULL)
		{
			WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);
			DWORD status = 0, size = sizeof(status);
			if (WinHttpSendRequest(request, L"Accept: application/vnd.github+json", (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
				WinHttpReceiveResponse(request, NULL) &&
				WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) &&
				status == 200)
			{
				ok = true;
				char buffer[4096];
				DWORD read = 0;
				// one release is a few KB; stop at 1 MB in case something else answers
				while (body.size() < 1024 * 1024 && WinHttpReadData(request, buffer, sizeof(buffer), &read) && read > 0)
				{
					body.append(buffer, read);
				}
			}
		}
		if (request) WinHttpCloseHandle(request);
		if (connection) WinHttpCloseHandle(connection);
		if (session) WinHttpCloseHandle(session);
		return ok;
	}
}

void UpdateCheck::Start(HWND notify, UINT message)
{
	std::thread([notify, message]
	{
		std::string body;
		std::string* tag = nullptr;
		if (Download(body))
		{
			std::string newest = NewestTag(body);
			if (newest.empty() == false)
			{
				tag = new std::string(newest);
			}
		}
		// the window may be gone by now (Options closed); then nobody takes the tag
		if (PostMessage(notify, message, 0, (LPARAM)tag) == FALSE)
		{
			delete tag;
		}
	}).detach();
}
