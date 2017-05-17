// VisEngine.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "VisEngine.h"
#include "Render.h"

//Globals
HINSTANCE				g_hInst = nullptr;
HWND                    g_hWnd = nullptr;

//-------------------------------------------------------------------------------------- 
// Forward declarations 
//-------------------------------------------------------------------------------------- 
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

//-------------------------------------------------------------------------------------- 
// Entry point to the program. Initializes everything and goes into a message processing  
// loop. Idle time is used to render the scene. 
//-------------------------------------------------------------------------------------- 
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
	{
		return 0;
	}

	if( FAILED( InitDevice() ) )
	{
		CleanupDevice();
		return 0;
	}

	// Main message loop 
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if( FAILED( Render() ) )
			{
				CleanupDevice();
				return 0;
			}
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}


//-------------------------------------------------------------------------------------- 
// Register class and create window 
//-------------------------------------------------------------------------------------- 
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class 
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_VISENGINE);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"VisEngine";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_VISENGINE);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window 
	g_hInst = hInstance;
	RECT rc = { 0, 0, 800, 600 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"VisEngine", L"VisEngine",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//-------------------------------------------------------------------------------------- 
// Called every time the application receives a message 
//-------------------------------------------------------------------------------------- 
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
