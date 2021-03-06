// windows_arranger.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "ShellIntegrator.h"
#include "WindowGroup.h"
#include "TaskbarManager.h"
#include "Messenger.h"
#include "utils.h"
#include "Timer.h"


#include <io.h>
#include <fcntl.h>


#define WM_NEW_TEXT_INPUT WM_USER
//#define WM_WINDOWS_POSITIONS_CHANGED WM_USER+1

#define ERROR_EXIT __COUNTER__+1


// TODO: better error handling
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	logg("START");

	try {
		std::FILE* standardStreams[] = {stdin, stdout, stderr};
		for (auto& stream : standardStreams) {
			int fileDescriptor = _fileno(stream);
			if (_setmode(fileDescriptor, _O_BINARY) == -1)
				return error("Can't set binary mode of file descriptor: " + std::to_string(fileDescriptor), ERROR_EXIT);
		}
		std::ios::sync_with_stdio();

		MSG msg;
		{
			ShellIntegrator shi;
			WindowGroupFactory wgf(shi.getProcessAppId(shi.getParentProcessId()));
			TaskbarManager tbm(shi, wgf);
			static auto parentThreadNativeId = GetCurrentThreadId();
			Messenger msgr(tbm, wgf, [](std::string* data) {
				return PostThreadMessage(parentThreadNativeId, WM_NEW_TEXT_INPUT, (WPARAM)data, (LPARAM)nullptr);
			}, [] {
				logg("Extracted in last operation: " + std::to_string(std::cin.gcount()));
				logg("Flags after last operation: " + std::to_string(std::cin.rdstate()));
				PostThreadMessage(parentThreadNativeId, WM_QUIT, (WPARAM)0, (LPARAM)0);
			});

			Timer timer(4000);

			while (GetMessage(&msg, nullptr, 0, 0)) {
				switch (msg.message) {
					case WM_NEW_TEXT_INPUT: {
						std::string* data = (std::string*)msg.wParam;
						msgr.processMessage(*data);
						delete data;
					}
					break;

					case WM_TIMER: {
						msgr.processTimer();
					}
					break;

					default: {
						logg("other message: " + std::to_string(msg.message));
					}
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		logg("END");
		return (int)msg.wParam;
	}
	catch (const ShellIntegrator::Exception& e) {
		return error(e.str, ERROR_EXIT);
	}
	catch (const TaskbarManager::Exception& e) {
		return error(e.str, ERROR_EXIT);
	}
	catch (Messenger::Exception& e) {
		return error(e.str, ERROR_EXIT);
	}
	catch (Timer::Exception&) {
		return error("can't create timer", ERROR_EXIT);
	}
	catch (...) {
		return error("unexpected error", ERROR_EXIT);
	}

	// never should be here!
}
