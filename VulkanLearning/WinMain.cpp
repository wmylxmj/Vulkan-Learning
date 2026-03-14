#include <Windows.h>

#include "VulkanUtils.h"

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_CLOSE:
		PostQuitMessage(0); // 发送WM_QUIT消息，通知消息循环退出
		break;
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_  HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	// 创建窗口类
	WNDCLASSEX wc;
	wc.cbSize = sizeof(wc); // 结构的大小
	wc.style = CS_HREDRAW | CS_VREDRAW; // 类样式
	wc.lpfnWndProc = MainWndProc; // 窗口处理函数
	wc.cbClsExtra = 0; // 此类的额外数据量
	wc.cbWndExtra = 0; // 此类型的每个窗口的额外数据量
	wc.hInstance = hInstance; // 应用程序实例的句柄
	wc.hIcon = nullptr; // 32x32大图标
	wc.hCursor = nullptr; // 光标
	wc.hbrBackground = nullptr; // 背景笔刷来设置窗口颜色
	wc.lpszMenuName = nullptr; // 菜单名称
	wc.lpszClassName = L"MainWnd"; // 类名称
	wc.hIconSm = nullptr; // 16x16小图标

	// 注册窗口类
	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"Register Window Class Failed.", 0, 0);
		return -1;
	}

	// 调整窗口大小以适应客户区
	RECT rect = { 0 };
	rect.right = 1280;
	rect.bottom = 720;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false); // 调整窗口大小以适应客户区

	// 创建窗口实例
	HWND hwnd = CreateWindowEx(
		0, // 拓展窗口样式
		wc.lpszClassName, // 窗口类名称
		L"Title", // 窗口标题
		WS_OVERLAPPEDWINDOW, // 窗口样式参数
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, // 左上角坐标，宽高
		0, 0, hInstance, 0 // 父窗口句柄、菜单句柄、应用程序实例句柄和一个指向窗口创建数据的指针
	);

	if (!hwnd)
	{
		MessageBox(0, L"Create Window Failed.", 0, 0);
		return -1;
	}

	InitVulkanUserData initVulkanUserData = { hInstance, hwnd };

	// 初始化Vulkan
	if (!InitVulkan(&initVulkanUserData, rect.right - rect.left, rect.bottom - rect.top))
	{
		return -1;
	}

	// 显示并更新窗口
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	MSG msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Rendering code goes here
			RenderOneFrame();
		}
	}

	return 0;
}