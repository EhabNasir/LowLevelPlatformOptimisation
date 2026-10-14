#include <windows.h>
#include "DX11Framework.h"
#include <optick.h>

//Dependencies:user32.lib;d3d11.lib;d3dcompiler.lib;dxgi.lib;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	DX11Framework application = DX11Framework();

	application.Initialise(hInstance, nCmdShow);

	LARGE_INTEGER frequency;

	QueryPerformanceFrequency(&frequency);

	auto const frequencyMs = static_cast<float>(frequency.QuadPart);
	LARGE_INTEGER lastTime;

	QueryPerformanceCounter(&lastTime);

	constexpr float fixedDeltaTime = 1.f / 60.f;
	float accumulator = 0.f;
	MSG msg = { 0 };

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			LARGE_INTEGER currentTime;

			QueryPerformanceCounter(&currentTime);

			float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / static_cast<float>(frequency.QuadPart);

			lastTime = currentTime;
			deltaTime = min(deltaTime, 0.25f);
			accumulator += deltaTime;

			while (accumulator >= fixedDeltaTime)
			{
				application.Simulate(fixedDeltaTime);

				accumulator -= fixedDeltaTime;
			}

			OPTICK_FRAME("Hello Nigger");
			application.Draw();
		}
	}

	return (int)msg.wParam;
}