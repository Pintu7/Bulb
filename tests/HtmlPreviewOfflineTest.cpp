// Checks that previewing an HTML clip never reaches the network: the HTML asks for an image, a script,
// a style sheet and a frame from a listener on this PC, and none of them may arrive.
#include "stdafx.h"
#include <winsock2.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <mshtml.h>
#include "SimpleBrowser.h"

#pragma comment(lib, "ws2_32.lib")

CWinApp theApp;

static std::atomic<int> connections{ 0 };
static std::atomic<bool> stop{ false };

static void Listen(SOCKET server)
{
	while (stop == false)
	{
		fd_set readable;
		FD_ZERO(&readable);
		FD_SET(server, &readable);
		timeval wait = { 0, 100000 };
		if (select(0, &readable, NULL, NULL, &wait) > 0)
		{
			SOCKET client = accept(server, NULL, NULL);
			if (client != INVALID_SOCKET)
			{
				connections++;
				closesocket(client);
			}
		}
	}
}

static void Pump(DWORD milliseconds)
{
	DWORD end = GetTickCount() + milliseconds;
	while (GetTickCount() < end)
	{
		MSG message;
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		Sleep(10);
	}
}

int main()
{
	if (AfxWinInit(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_HIDE) == FALSE || FAILED(OleInitialize(NULL)))
	{
		printf("HtmlPreviewOffline: could not start MFC\n");
		return 1;
	}
	AfxEnableControlContainer();

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in address = {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	int size = sizeof(address);
	if (bind(server, (sockaddr*)&address, size) != 0 || listen(server, 16) != 0 || getsockname(server, (sockaddr*)&address, &size) != 0)
	{
		printf("HtmlPreviewOffline: could not listen on this PC\n");
		return 1;
	}
	int port = ntohs(address.sin_port);
	std::thread listener(Listen, server);

	CWnd parent;
	parent.CreateEx(0, AfxRegisterWndClass(0), _T("HtmlPreviewOfflineTest"), WS_OVERLAPPEDWINDOW, CRect(0, 0, 400, 300), NULL, 0);
	SimpleBrowser browser;
	int failed = 0;
	if (browser.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 380, 260), &parent, 2) == FALSE)
	{
		printf("HtmlPreviewOffline: could not create the preview (parent window %p, error %lu)\n", (void*)parent.m_hWnd, GetLastError());
		failed = 1;
	}
	else
	{
		Pump(1500);

		CString base;
		base.Format(_T("http://127.0.0.1:%d/"), port);
		CString html = _T("<html><head><link rel=\"stylesheet\" href=\"") + base + _T("style.css\"></head><body>")
			_T("<img src=\"") + base + _T("image.png\">")
			_T("<script>new Image().src = \"") + base + _T("script\";</script>")
			_T("<iframe src=\"") + base + _T("frame\"></iframe>")
			_T("<b>still shown</b></body></html>");
		browser.Clear();
		browser.Write(html);
		Pump(4000);

		if (connections != 0)
		{
			printf("FAIL: the HTML preview opened %d connection(s)\n", connections.load());
			failed = 1;
		}

		// blocking the network must not blank the preview
		CString shown;
		IHTMLDocument2* document = browser.GetDocument();
		IHTMLElement* body = NULL;
		BSTR text = NULL;
		if (document != NULL && SUCCEEDED(document->get_body(&body)) && body != NULL && SUCCEEDED(body->get_innerText(&text)) && text != NULL)
		{
			shown = text;
			SysFreeString(text);
		}
		if (body != NULL) body->Release();
		if (document != NULL) document->Release();
		if (shown.Find(_T("still shown")) < 0)
		{
			printf("FAIL: the HTML preview no longer shows the clip's text\n");
			failed = 1;
		}
	}

	stop = true;
	listener.join();
	closesocket(server);
	parent.DestroyWindow();

	printf("HtmlPreviewOffline: %s\n", failed ? "FAILED" : "2 checks passed");
	return failed;
}
