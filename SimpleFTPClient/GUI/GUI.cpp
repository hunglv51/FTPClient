// GUI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "stdio.h"
#include "GUI.h"
#include "winsock2.h"
#include "commctrl.h"
#include "Richedit.h"
#include "ftpparse.h"
#include "ftpparse.c"



#define DEFAULT_ICON_INDEX 0


#define MAX_PATH_LEN 10240

#define MAX_LOADSTRING 100
#define WM_SOCKET WM_USER + 1
#define DOWNLOAD 1
#define UPLOAD 2
#define DOWNLOAD_MULTI 3
// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hHost, hUsername, hPassword, hPort, hWndEdit, hLeftWnd, hList, hRList, hLocalPath, hRemotePath;
SOCKET cmdSoc, dataSoc;					//socket
int state = 0;
char cmdRecvBuf[256], cmdSendBuf[128];
char user[64], pass[64];
WCHAR localPath[MAX_PATH], remotePath[MAX_PATH]; char fileNameUp[64];//duong dan local,remote,ten file duoc upload
SOCKADDR_IN addr;
int indexRItem; //id file trong list view
char remoteFileName[64];// ten file hoac dir duoc chon phia server

//for multi connection download
FILE *fmul;
CRITICAL_SECTION cs;
SOCKET socArrays[64];



// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndTopProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndRightProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndLeftProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    RequestName(HWND, UINT, WPARAM, LPARAM);//for rename
INT_PTR CALLBACK    RequestNewName(HWND, UINT, WPARAM, LPARAM);//for new dir
HWND CreatListView(HWND hwndParent);
BOOL InitListViewColumns(HWND hWndListView);
BOOL InsertListViewItems(HWND hWndListView, LPWSTR path);//for local
BOOL InsertListViewItem(HWND hWndListView, struct ftpparse*);//for remote
void displayRemote();
void recvData();
void sendData();

DWORD WINAPI MyThread(LPVOID);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	//khoi tao winsock
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);


	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_GUI, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GUI));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex, wct, wcl, wcr;

	wcl.cbSize = wcr.cbSize = wct.cbSize = wcex.cbSize = sizeof(WNDCLASSEX);

	wcr.style = wct.style = wcl.style = wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wct.lpfnWndProc = WndTopProc;
	wcr.lpfnWndProc = WndRightProc;
	wcl.lpfnWndProc = WndLeftProc;

	wct.cbClsExtra = wcl.cbClsExtra = wcr.cbClsExtra = wcex.cbClsExtra = 0;
	wct.cbWndExtra = wcl.cbWndExtra = wcr.cbWndExtra = wcex.cbWndExtra = 0;
	wct.hInstance = wcl.hInstance = wcr.hInstance = wcex.hInstance = hInstance;
	wct.hIcon = wcl.hIcon = wcr.hIcon = wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GUI));
	wct.hCursor = wcl.hCursor = wcr.hCursor = wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wct.hbrBackground = wcl.hbrBackground = wcr.hbrBackground = wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wct.lpszMenuName = wcl.lpszMenuName = wcr.lpszMenuName = wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GUI);
	wct.lpszClassName = TEXT("Top Class");
	wcl.lpszClassName = TEXT("Left Class");
	wcr.lpszClassName = TEXT("Right Class");
	wcex.lpszClassName = szWindowClass;
	wct.hIconSm = wcl.hIconSm = wcr.hIconSm = wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcr);
	RegisterClassExW(&wcl);
	RegisterClassExW(&wct);
	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND CreateRichEdit(HWND hwndOwner,        // Dialog box handle.
	int x, int y,          // Location.
	int width, int height, // Dimensions.
	HINSTANCE hinst)       // Application or DLL instance.
{
	LoadLibrary(TEXT("Msftedit.dll"));

	HWND hwndEdit = CreateWindowEx(0, MSFTEDIT_CLASS, TEXT(""),
		ES_MULTILINE | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL,
		x, y, width, height,
		hwndOwner, NULL, hinst, NULL);
	CHARRANGE cr;
	cr.cpMin = -1;
	cr.cpMax = -1;

	// hwnd = rich edit hwnd
	SendMessage(hWndEdit, EM_EXSETSEL, 0, (LPARAM)&cr);


	return hwndEdit;
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hWnd);




	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//  WM_SOCKET   - transfer data socket
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{



	switch (message)
	{
	case WM_CREATE: {
		//create 3 window
		CreateWindow(TEXT("static"), TEXT("Host: "),
			WS_VISIBLE | WS_CHILD,
			0, 0, 50, 25,
			hWnd, (HMENU)1, NULL, NULL);
		hHost = CreateWindow(TEXT("Edit"), TEXT(""),
			WS_VISIBLE | WS_CHILD,
			50, 0, 150, 25,
			hWnd, (HMENU)1, NULL, NULL);
		CreateWindow(TEXT("static"), TEXT("User name: "),
			WS_VISIBLE | WS_CHILD,
			200, 0, 100, 25,
			hWnd, (HMENU)1, NULL, NULL);
		hUsername = CreateWindow(TEXT("edit"), TEXT(""),
			WS_VISIBLE | WS_CHILD,
			300, 0, 150, 25,
			hWnd, (HMENU)1, NULL, NULL);
		CreateWindow(TEXT("static"), TEXT("Password: "),
			WS_VISIBLE | WS_CHILD,
			450, 0, 100, 25,
			hWnd, (HMENU)1, NULL, NULL);
		hPassword = CreateWindow(TEXT("edit"), TEXT(""),
			WS_VISIBLE | WS_CHILD | ES_PASSWORD,
			550, 0, 150, 25,
			hWnd, (HMENU)1, NULL, NULL);
		CreateWindow(TEXT("static"), TEXT("Port: "),
			WS_VISIBLE | WS_CHILD,
			700, 0, 50, 25,
			hWnd, (HMENU)1, NULL, NULL);
		hPort = CreateWindow(TEXT("edit"), TEXT(""),
			WS_VISIBLE | WS_CHILD,
			750, 0, 100, 25,
			hWnd, (HMENU)1, NULL, NULL);
		CreateWindow(TEXT("button"), TEXT("Connect"),
			WS_VISIBLE | WS_CHILD,
			850, 0, 150, 25,
			hWnd, (HMENU)9, NULL, NULL);
		CreateWindow(TEXT("button"), TEXT("Disconnect"),
			WS_VISIBLE | WS_CHILD,
			1000, 0, 150, 25,
			hWnd, (HMENU)10, NULL, NULL);

		HWND hTopWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Top Class"), L"Top",
			WS_CHILD | WS_VISIBLE, 0, 25, 1366,
			150, hWnd, NULL,
			hInst, NULL);
		if (NULL != hTopWnd)
		{
			ShowWindow(hTopWnd, SW_SHOW);
			UpdateWindow(hTopWnd);
		}
		hLeftWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Left Class"), L"Left",
			WS_CHILD | WS_VISIBLE, 0, 175, 683,
			559, hWnd, NULL,
			hInst, NULL);
		if (NULL != hLeftWnd)
		{
			ShowWindow(hLeftWnd, SW_SHOW);
			UpdateWindow(hLeftWnd);
		}


		HWND hRightWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Right Class"), L"Right",
			WS_CHILD | WS_VISIBLE, 683, 175, 683,
			559, hWnd, NULL,
			hInst, NULL);
		if (NULL != hRightWnd)
		{
			ShowWindow(hRightWnd, SW_SHOW);
			UpdateWindow(hRightWnd);
		}

		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case 9: {
			//connect socket
			//cai dat socket
			cmdSoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			SendMessage(hWndEdit, EM_REPLACESEL, 0, (LPARAM)TEXT("Connecting\n"));
			char host[16], sPort[8];
			GetWindowTextA(hHost, host, sizeof(host));
			GetWindowTextA(hPort, sPort, sizeof(sPort));

			int port = atoi(sPort);

			addr.sin_addr.s_addr = inet_addr(host);
			addr.sin_port = htons(port);
			addr.sin_family = AF_INET;
			connect(cmdSoc, (SOCKADDR *)&addr, sizeof(addr));
			//change to non stop
			WSAAsyncSelect(cmdSoc, hLeftWnd, WM_SOCKET, FD_READ | FD_CLOSE);

			break;
		}
		case 10:
		{
			//Disconnect socket


			send(cmdSoc, "QUIT\n", 5, 0);
			SendMessage(hWndEdit, EM_REPLACESEL, 0, (LPARAM)TEXT("Disconnect\n"));
			ListView_DeleteAllItems(hRList);

			break;
		}
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);

		}
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
INT_PTR CALLBACK RequestName(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDOK)
		{
			char newDirName[64];
			GetDlgItemTextA(hDlg, IDC_EDIT1, newDirName, 63);
			char cmd[128];
			ListView_DeleteAllItems(hRList);
			sprintf(cmd, "MKD %s\n", newDirName);
			send(cmdSoc, cmd, strlen(cmd), 0);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
INT_PTR CALLBACK RequestNewName(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			char newName[64];
			GetDlgItemTextA(hDlg, IDC_EDIT1, newName, 63);
			char cmd[128];
			ListView_DeleteAllItems(hRList);
			sprintf(cmd, "RNTO %s\n", newName);
			send(cmdSoc, cmd, strlen(cmd), 0);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndTopProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:

		hWndEdit = CreateRichEdit(hWnd, 0, 0, 1366, 150, hInst);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
//LRESULT CALLBACK WndBotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	
//	case WM_DESTROY:
//		PostQuitMessage(0);
//		break;
//	default:
//		return DefWindowProc(hWnd, message, wParam, lParam);
//	}
//	return 0;
//}
LRESULT CALLBACK WndRightProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
	{
		POINT point;
		wmemcpy(remotePath, L"/", sizeof(WCHAR));
		RECT rect;
		GetClientRect(hWnd, &rect);
		hRemotePath = CreateWindow(TEXT("static"), remotePath, WS_VISIBLE | WS_CHILD, rect.left + 152, rect.top, rect.right - rect.left - 150, 25, hWnd, NULL, NULL, NULL);
		CreateWindow(TEXT("button"), L"Back", WS_VISIBLE | WS_CHILD, rect.left, rect.top, 50, 25, hWnd, (HMENU)5, NULL, NULL);
		CreateWindow(TEXT("button"), L"New Directory", WS_VISIBLE | WS_CHILD, rect.left + 50, rect.top, 100, 25, hWnd, (HMENU)4, NULL, NULL);
		hRList = CreatListView(hWnd);
		InitListViewColumns(hRList);

		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case 1:
		{
			//download button
			int len = strlen(remoteFileName);
			char type[4];
			memcpy(type, remoteFileName + len - 3, 3);
			type[3] = 0;
			if (strcmp(type, "txt") == 0)
			{
				send(cmdSoc, "TYPE A\n", 7, 0);
			}
			else
			{
				send(cmdSoc, "TYPE I\n", 7, 0);
			}
			state = DOWNLOAD;
			send(cmdSoc, "PASV\n", 5, 0);
			break;
		}
		case 7:
		{
			//download multi connection button
			int len = strlen(remoteFileName);
			char type[4];
			memcpy(type, remoteFileName + len - 3, 3);
			type[3] = 0;
			if (strcmp(type, "txt") == 0)
			{
				send(cmdSoc, "TYPE A\n", 7, 0);
			}
			else
			{
				send(cmdSoc, "TYPE I\n", 7, 0);
			}
			state = DOWNLOAD_MULTI;
			send(cmdSoc, "PASV\n", 5, 0);
			break;
		}
		case 2:
		{
			//rename button
			char cmd[128];
			sprintf(cmd, "RNFR %s\n", remoteFileName);
			send(cmdSoc, cmd, strlen(cmd), 0);
			break;
		}
		case 6:
		{
			//remove dir button
			char cmd[128];
			sprintf(cmd, "RMD %s\n", remoteFileName);
			send(cmdSoc, cmd, strlen(cmd), 0);
			ListView_DeleteAllItems(hRList);
			break;
		}
		case 3:
		{
			//delete file button
			char cmd[128];
			sprintf(cmd, "DELE %s\n", remoteFileName);
			send(cmdSoc, cmd, strlen(cmd), 0);
			ListView_DeleteAllItems(hRList);
			break;
		}
		case 5:
		{
			//back button
			send(cmdSoc, "CDUP\n", 5, 0);
			ListView_DeleteAllItems(hRList);
			break;
		}
		case 4:
		{
			//new dir button
			HWND dialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, RequestName);
			ShowWindow(dialog, SW_SHOWDEFAULT);
			UpdateWindow(dialog);

			break;
		}

		}
		break;
	}
	case WM_NOTIFY: {


		//The program has started and loaded all necessary component

		NMHDR* notifyMess = (NMHDR*)lParam; //Notification Message
		switch (notifyMess->code)
		{
		case NM_RCLICK:
		{
			if (notifyMess->hwndFrom == hRList)
			{

				POINT point;
				int indexSelected = ListView_GetSelectionMark(hRList);
				WCHAR name[32], type[8];
				ListView_GetItemText(hRList, indexSelected, 0, name, lstrlenW(name));

				RECT rect;
				GetClientRect(hLeftWnd, &rect);
				ListView_GetItemText(hRList, indexSelected, 1, type, lstrlenW(type));
				ListView_GetItemPosition(hRList, indexSelected + 2, &point);
				if (lstrcmpW(type, L"File") == 0)
				{
					sprintf(remoteFileName, "%ws", name);
					HMENU hMenu = CreatePopupMenu();
					AppendMenu(hMenu, MF_STRING, 1, TEXT("&Download"));
					AppendMenu(hMenu, MF_STRING, 2, TEXT("&Rename"));
					AppendMenu(hMenu, MF_STRING, 3, TEXT("&Delete"));
					AppendMenu(hMenu, MF_STRING, 7, TEXT("&Download with multi connections"));
					TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, point.x + rect.right, point.y + 175, 0, hWnd,
						NULL);
					DestroyMenu(hMenu);

				}
				if (lstrcmpW(type, L"Folder") == 0) {
					sprintf(remoteFileName, "%ws", name);
					HMENU hMenu = CreatePopupMenu();
					AppendMenu(hMenu, MF_STRING, 2, TEXT("&Rename"));
					AppendMenu(hMenu, MF_STRING, 6, TEXT("&Delete directory"));
					TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, point.x + rect.right, point.y + 175, 0, hWnd,
						NULL);
					DestroyMenu(hMenu);

				}

			}
			break;
		}
		case NM_DBLCLK:
			//Get hwndFrom for window handle to the control sending the message
			//To check whether this event fire by Listview
			if (notifyMess->hwndFrom == hRList)
			{
				WCHAR name[32], type[8];
				ListView_GetItemText(hRList, ListView_GetSelectionMark(hRList), 0, name, lstrlenW(name));
				ListView_GetItemText(hRList, ListView_GetSelectionMark(hRList), 1, type, lstrlenW(type));
				if (lstrcmpW(type, L"Folder") == 0)
				{
					char cmd[64];
					sprintf(cmd, "CWD %ws\n", name);
					send(cmdSoc, cmd, strlen(cmd), 0);
					ListView_DeleteAllItems(hRList);



				}
			}
			break;
		}


		break;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
LRESULT CALLBACK WndLeftProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case 1:
		{
			//Upload button
			char cmd[128];

			int len = strlen(fileNameUp);
			char type[4];
			memcpy(type, fileNameUp + len - 3, 3);
			type[3] = 0;
			if (strcmp(type, "txt") == 0)
			{
				send(cmdSoc, "TYPE A\n", 7, 0);
			}
			else
			{
				send(cmdSoc, "TYPE I\n", 7, 0);
			}
			state = UPLOAD;
			send(cmdSoc, "PASV\n", 5, 0);
			break;
		}


		}
		break;
	}
	case WM_SOCKET:
	{
		switch (lParam)
		{
		case FD_READ:
		{
			int res = -1;
			//nhan ma lenh tra ve tu server
			res = recv(cmdSoc, cmdRecvBuf, sizeof(cmdRecvBuf), 0);
			cmdRecvBuf[res] = 0;
			SendMessageA(hWndEdit, EM_REPLACESEL, 0, (LPARAM)cmdRecvBuf);
			if (strlen(user) < 1) {
				GetWindowTextA(hUsername, user, sizeof(user));
				GetWindowTextA(hPassword, pass, sizeof(pass));


			}
			char strCode[4];
			strncpy(strCode, cmdRecvBuf, 3);
			strCode[3] = 0;
			int iCode = atoi(strCode);
			switch (iCode)
			{
				//Ma loi chao cua server, nhap user
			case 220:
			{

				sprintf_s(cmdSendBuf, "USER %s\n", user);
				SendMessageA(hWndEdit, EM_REPLACESEL, 0, (LPARAM)cmdSendBuf);

				res = send(cmdSoc, cmdSendBuf, strlen(cmdSendBuf), 0);
				cmdRecvBuf[0] = 0;
				break;
			}
			//server yeu cau pass
			case 331:
			{

				sprintf_s(cmdSendBuf, "PASS %s\n", pass);
				SendMessageA(hWndEdit, EM_REPLACESEL, 0, (LPARAM)cmdSendBuf);
				res = send(cmdSoc, cmdSendBuf, strlen(cmdSendBuf), 0);
				cmdRecvBuf[0] = 0;
				break;
			}
			//server gui ma ket thuc phien lam viec disconnect
			case 221:
				closesocket(cmdSoc);
				break;
				//server gui cong dap ung lenh pasv
			case 227:
			{
				//227 Entering Passive Mode(127, 0, 0, 1, 213, 209)
				char *token;
				token = strtok(cmdRecvBuf, "(");
				token = strtok(NULL, ")");
				int a, b, c, d, high, low;

				sscanf(token, "%i,%i,%i,%i,%i,%i", &a, &b, &c, &d, &high, &low);
				// port data = high * 256 + low
				int dataPort = high * 256 + low;
				addr.sin_port = htons(dataPort);


				if (state == DOWNLOAD_MULTI)
				{
					for (int i = 0; i < 8; i++)
					{
						socArrays[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
						connect(socArrays[i], (SOCKADDR *)&addr, sizeof(addr));
					}
					InitializeCriticalSection(&cs);

					char cmd[64];
					sprintf(cmd, "RETR %s\n", remoteFileName);
					send(cmdSoc, cmd, strlen(cmd), 0);
				}
				else
				{
					dataSoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					connect(dataSoc, (SOCKADDR *)&addr, sizeof(addr));
					//neu yeu cau list gui list
					if (state == DOWNLOAD)
					{
						char cmd[64];
						sprintf(cmd, "RETR %s\n", remoteFileName);
						send(cmdSoc, cmd, strlen(cmd), 0);
					}
					else if (state == UPLOAD)
					{
						char cmd[64];
						sprintf(cmd, "STOR %s\n", fileNameUp);
						send(cmdSoc, cmd, strlen(cmd), 0);
					}

					else
					{
						send(cmdSoc, "LIST\n", 5, 0);
					}
				}
				break;
			}
			//thong bao mo cong du lieu truyen nhan
			case 150:
			{
				//150 Opening data channel for directory listing of "/"
				//neu yeu cau list hien thi, dong thoi cap nhat remotepath
				if (state == DOWNLOAD)
				{
					recvData();
				}
				else if (state == UPLOAD)
				{

					sendData();
				}
				else if (state == DOWNLOAD_MULTI)
				{
					HANDLE hArrays[8];
					char path[MAX_PATH + 32];
					sprintf(path, "%ws%s", localPath, remoteFileName);

					fmul = fopen(path, "wb");
					for (int i = 0; i < 8; i++)
					{
						hArrays[i] = CreateThread(NULL, 0, MyThread, &(socArrays[i]), 0, NULL);
					}
					WaitForMultipleObjects(8, hArrays, true, INFINITE);
					DeleteCriticalSection(&cs);
					fclose(fmul);
					state = 0;
					ListView_DeleteAllItems(hList);
					InsertListViewItems(hList, localPath);
				}
				else
				{
					displayRemote();
					send(cmdSoc, "PWD\n", 4, 0);
				}
				break;
			}
			//230 Logged on
			case 230:
			{
				send(cmdSoc, "PASV\n", 5, 0);
				break;

			}
			case 226:
			{
				if (state == UPLOAD)
				{
					state = 0;
					ListView_DeleteAllItems(hRList);
					send(cmdSoc, "PASV\n", 5, 0);
				}
				break;
			}
			case 250:
			case 200:
			{
				//type set to do nothing
				char *token;
				token = strtok(cmdRecvBuf, " ");
				token = strtok(NULL, "\r\n");
				if (strncmp(token, "Type set to", 11) == 0)
					break;
				//CWD or CDUP,RMD,DELE success
				send(cmdSoc, "PASV\n", 5, 0);
				break;
			}
			case 350:
			{
				//request new name
				HWND dialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, RequestNewName);
				ShowWindow(dialog, SW_SHOWDEFAULT);
				UpdateWindow(dialog);

				break;
			}
			case 530:
			{
				//login fail
				MessageBox(hWnd, L"Please insert your name and password", L"Login or password incorrect!", MB_OK);
				user[0] = 0;
				pass[0] = 0;
				closesocket(cmdSoc);
				break;
			}
			case 257:
			{
				//pwd success cap nhat remote path
				char *token, *path;
				token = strtok(cmdRecvBuf, "\"");
				token = strtok(NULL, "\"");
				path = token;
				token = strtok(NULL, "\r\n");
				if (strcmp(token, " created successfully") != 0)
				{
					swprintf(remotePath, sizeof(remotePath), L"%hs", path);

				}
				else
					send(cmdSoc, "PASV\n", 5, 0);
				SetWindowText(hRemotePath, remotePath);
				break;
			}
			default:
				break;
			}

			break;
		}

		default: break;
		}
		break;
	}
	case WM_CREATE:
	{
		wmemcpy(localPath, L"C:\\", 4 * sizeof(WCHAR));
		RECT rect;
		GetClientRect(hWnd, &rect);
		hLocalPath = CreateWindow(TEXT("static"), localPath, WS_VISIBLE | WS_CHILD, rect.left, rect.top, rect.right - rect.left, 25, hWnd, NULL, NULL, NULL);
		hList = CreatListView(hWnd);
		InitListViewColumns(hList);
		InsertListViewItems(hList, localPath);

		break;
	}
	case WM_NOTIFY: {


		//The program has started and loaded all necessary component

		NMHDR* notifyMess = (NMHDR*)lParam; //Notification Message
		switch (notifyMess->code)
		{
		case NM_RCLICK:
		{
			if (notifyMess->hwndFrom == hList)
			{
				POINT point;
				int indexSelected = ListView_GetSelectionMark(hList);
				WCHAR name[32], type[8];
				ListView_GetItemText(hList, indexSelected, 0, name, lstrlenW(name));

				ListView_GetItemText(hList, indexSelected, 1, type, lstrlenW(type));
				ListView_GetItemPosition(hList, indexSelected + 2, &point);
				if (lstrcmpW(type, L"File") == 0)
				{

					HMENU hMenu = CreatePopupMenu();
					//ClientToScreen(hList, &point);
					AppendMenu(hMenu, MF_STRING, 1, TEXT("&Upload"));
					TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, point.x, point.y + 175, 0, hWnd,
						NULL);
					DestroyMenu(hMenu);
					WCHAR fileName[32];
					ListView_GetItemText(hList, indexSelected, 0, fileName, lstrlenW(fileName));
					sprintf(fileNameUp, "%ws", fileName);
					break;
				}
			}
			break;
		}
		case NM_DBLCLK:
			//Get hwndFrom for window handle to the control sending the message
			//To check whether this event fire by Listview
			if (notifyMess->hwndFrom == hList)
			{
				WCHAR name[32], type[8];
				ListView_GetItemText(hList, ListView_GetSelectionMark(hList), 0, name, lstrlenW(name));
				ListView_GetItemText(hList, ListView_GetSelectionMark(hList), 1, type, lstrlenW(type));
				if (lstrcmpW(type, L"Folder") == 0)
				{
					if (lstrcmp(name, L"..") == 0) //back to parent dir
					{
						int len = lstrlenW(localPath);
						for (int i = len - 2; i > 0; i--)
							if (localPath[i - 1] == L'\\')
							{
								localPath[i] = 0;
								break;
							}

					}
					else
					{
						wsprintf(localPath, L"%ws%ws%ws", localPath, name, L"\\");

					}
					ListView_DeleteAllItems(hList);
					InsertListViewItems(hList, localPath);
					SetWindowText(hLocalPath, localPath);
				}
			}
			break;
		}


		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//tao list view hien thi noi dung dir
HWND CreatListView(HWND hwndParent) {
	INITCOMMONCONTROLSEX ic;
	ic.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&ic);
	RECT rect;
	GetClientRect(hwndParent, &rect);
	HWND hwndListView = CreateWindow(WC_LISTVIEW, L"", WS_CHILD | LVS_REPORT | LVS_EDITLABELS | WS_VISIBLE, rect.left, rect.top + 25, rect.right - rect.left, rect.bottom - rect.top - 25, hwndParent, (HMENU)1, hInst, NULL);
	return hwndListView;
}
//chen cot list view
BOOL InitListViewColumns(HWND hWndListView)
{
	LVCOLUMN lvc;
	RECT rect;
	GetClientRect(hWndListView, &rect);
	int listW = rect.right - rect.left;

	// Initialize the LVCOLUMN structure.
	// The mask specifies that the format, width, text,
	// and subitem members of the structure are valid.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	// Add the columns.
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = listW / 2;
	lvc.pszText = _T("Name");
	if (ListView_InsertColumn(hWndListView, 0, &lvc) == -1)
		return FALSE;


	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = listW / 4;
	lvc.pszText = _T("Type");
	if (ListView_InsertColumn(hWndListView, 1, &lvc) == -1)
		return FALSE;


	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = listW / 4;
	lvc.pszText = _T("Size(KB)");
	if (ListView_InsertColumn(hWndListView, 2, &lvc) == -1)
		return FALSE;





	return TRUE;
}
// InsertListViewItems: Inserts items into a list view. 
// hWndListView:        Handle to the list-view control.
// cItems:              Number of items to insert.
// Returns TRUE if successful, and FALSE otherwise.

//chen item list view phia local su dung win32 find data
BOOL InsertListViewItems(HWND hWndListView, LPWSTR path)
{
	LVITEM lvI;
	WCHAR filePath[MAX_PATH];
	wsprintf(filePath, L"%ws%ws", path, L"*.*");
	// Initialize LVITEM members that are common to all items.
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvI.stateMask = 0;
	lvI.iSubItem = 0;
	lvI.state = 0;
	WIN32_FIND_DATA fd;
	HANDLE h = FindFirstFile(filePath, &fd);
	int index = 0;
	do {
		lvI.iItem = index;
		lvI.iImage = index;
		lvI.pszText = fd.cFileName;
		if (ListView_InsertItem(hWndListView, &lvI) == -1)
			return FALSE;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			ListView_SetItemText(hWndListView, index, 0, fd.cFileName);
			ListView_SetItemText(hWndListView, index, 1, L"Folder");
			ListView_SetItemText(hWndListView, index, 2, L"");


		}
		else {
			ListView_SetItemText(hWndListView, index, 0, fd.cFileName);
			ListView_SetItemText(hWndListView, index, 1, L"File");
			LARGE_INTEGER fileSize;
			fileSize.LowPart = fd.nFileSizeLow;
			fileSize.HighPart = fd.nFileSizeHigh;

			WCHAR szSize[16];
			_ltow(fileSize.QuadPart / 1024, szSize, 10);
			ListView_SetItemText(hWndListView, index, 2, szSize);

		}
		index++;
	} while (FindNextFile(h, &fd));

	return TRUE;
}
//hien thi noi dung dir ben phia remote
void displayRemote() {

	indexRItem = 0;

	char dataBuf[MAX_PATH_LEN];
	int res = recv(dataSoc, dataBuf, MAX_PATH_LEN, 0);

	struct ftpparse fp;
	char *token;
	/* lay token dau tien */
	token = strtok(dataBuf, "\r\n");

	/* duyet qua cac token con lai */
	while (token != NULL)
	{
		ftpparse(&fp, token, strlen(token));
		//insert item list view
		if (fp.name == NULL) break;
		InsertListViewItem(hRList, &fp);

		token = strtok(NULL, "\r\n");
		indexRItem++;
	}

	closesocket(dataSoc);
}
//chen 1 item vao list view phia remote
BOOL InsertListViewItem(HWND hWndListView, struct ftpparse *fp)
{
	LVITEM lvI;
	// Initialize LVITEM members that are common to all items.
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvI.stateMask = 0;
	lvI.iSubItem = 0;
	lvI.state = 0;
	lvI.iItem = indexRItem;
	lvI.iImage = indexRItem;
	WCHAR name[64];
	swprintf(name, sizeof(name), L"%hs", fp->name);
	lvI.pszText = name;
	if (ListView_InsertItem(hWndListView, &lvI) == -1)
		return FALSE;

	if (fp->flagtryretr == 0) {//dir
		ListView_SetItemText(hWndListView, indexRItem, 0, name);
		ListView_SetItemText(hWndListView, indexRItem, 1, L"Folder");
		ListView_SetItemText(hWndListView, indexRItem, 2, L"");


	}
	else {
		ListView_SetItemText(hWndListView, indexRItem, 0, name);
		ListView_SetItemText(hWndListView, indexRItem, 1, L"File");

		WCHAR szSize[16];
		_ltow(fp->size / 1024, szSize, 10);
		ListView_SetItemText(hWndListView, indexRItem, 2, szSize);

	}

	return TRUE;
}

void recvData() {
	char buf[1024];
	int res;
	char path[MAX_PATH + 32];
	sprintf(path, "%ws%s", localPath, remoteFileName);
	FILE *f;
	f = fopen(path, "wb");
	while (true)
	{
		res = recv(dataSoc, buf, sizeof(buf), 0);
		if (res == 0) break;
		fwrite(buf, 1, res, f);

	}
	fclose(f);
	closesocket(dataSoc);
	state = 0;
	ListView_DeleteAllItems(hList);
	InsertListViewItems(hList, localPath);

}
void sendData() {
	char buf[1024];
	int res;
	char path[MAX_PATH + 32];
	sprintf(path, "%ws%s", localPath, fileNameUp);
	FILE *f = fopen(path, "rb");
	while (true)
	{
		res = fread(buf, 1, sizeof(buf), f);
		if (res > 0)
			send(dataSoc, buf, res, 0);
		else
			break;
	}
	fclose(f);
	closesocket(dataSoc);

}
// tao thread
DWORD WINAPI MyThread(LPVOID lParam) {
	SOCKET s = *((SOCKET *)(lParam));
	char buf[1024];
	int res;
	while (true)
	{
		res = recv(s, buf, sizeof(buf), 0);
		if (res <= 0) break;
		EnterCriticalSection;
		fwrite(buf, 1, res, fmul);
		LeaveCriticalSection;
	}
	closesocket(s);
	return 0;
}