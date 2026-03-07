#include "Windows.h"
#include "vulkan/vulkan.h"

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_  HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	MessageBoxA(NULL, "Hello, Vulkan!", "Vulkan Learning", MB_OK);
	return 0;
}