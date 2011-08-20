/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define REQUIRED_XINPUT_DLL "xinput1_3.dll"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "XInputHook.h"
#include "HookDLL.h"
#include <XInput.h>
#include <iostream>

HINSTANCE dll;
char name[256];

#pragma data_seg(".SHARED")

HHOOK s_hook = NULL;

// @todo use a struct for multiple gamepad support
WORD s_buttons = 0;
SHORT s_leftStickX = 0;
SHORT s_leftStickY = 0;
SHORT s_rightStickX = 0;
SHORT s_rightStickY = 0;
BYTE s_leftTrigger = 0;
BYTE s_rightTrigger = 0;

#pragma data_seg()

#pragma comment(linker, "/SECTION:.SHARED,RWS")

#include <sstream>
std::stringstream logStream2;
#define LOG(s) \
	logStream2.str(""); \
	logStream2 << s; \
	OutputDebugString( logStream2.str().c_str() );

using namespace std;

typedef DWORD (WINAPI *XInputGetState_Type)(DWORD dwUserIndex, XINPUT_STATE* pState);
DWORD WINAPI HookXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);

typedef DWORD (WINAPI *XInputGetCapabilities_Type)(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities);
DWORD WINAPI HookXInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities);

SDLLHook s_xInputHook =
{
	XINPUT_DLL,
	false, NULL,
	{
		{ (char*)(0x80000002), HookXInputGetState },
		{ (char*)(0x80000004), HookXInputGetCapabilities },
	}
};

BOOL APIENTRY
DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		dll = module;

		// disable unwanted thread notifications to reduce overhead
		DisableThreadLibraryCalls(dll);

		GetModuleFileName(GetModuleHandle(NULL), name, sizeof(name));

		LOG("XInputHook: checking process: "
			<< name << ", hook: " << XINPUT_DLL << endl);

		// don't hook synergys (this needs to detect real input)
		if (string(name).find("synergys.exe") == string::npos)
		{
			HookAPICalls(&s_xInputHook);
		}
	}

	return TRUE;
}

void
SetXInputButtons(DWORD userIndex, WORD buttons)
{
	s_buttons = buttons;
	LOG("XInputHook: SetXInputButtons: " << buttons << endl);
}

void
SetXInputSticks(DWORD userIndex, SHORT lx, SHORT ly, SHORT rx, SHORT ry)
{
	s_leftStickX = lx;
	s_leftStickY = ly;
	s_rightStickX = rx;
	s_rightStickY = rx;

	LOG("XInputHook: SetXInputSticks: " <<
		"left=" << lx << "," << ly <<
		" right=" << rx << "," << ry << endl);
}

void
SetXInputTriggers(DWORD userIndex, BYTE left, BYTE right)
{
	s_leftTrigger = left;
	s_rightTrigger = right;

	LOG("XInputHook: SetXInputTriggers: " <<
		"left=" << (int)left << " right=" << (int)right << endl);
}

DWORD WINAPI
HookXInputGetState(DWORD userIndex, XINPUT_STATE* state)
{
	if (userIndex != 0)
		return 1167; // @todo find macro for this

	LOG("XInputHook: hookXInputGetState index=" 
		<< userIndex <<  ", buttons=" << s_buttons << endl);

	state->Gamepad.wButtons = s_buttons;
	state->Gamepad.bLeftTrigger = s_leftTrigger;
	state->Gamepad.bRightTrigger = s_rightTrigger;
	state->Gamepad.sThumbLX = s_leftStickX;
	state->Gamepad.sThumbLY = s_leftStickY;
	state->Gamepad.sThumbRX = s_rightStickX;
	state->Gamepad.sThumbRY = s_rightStickY;

	return ERROR_SUCCESS;
}

DWORD WINAPI
HookXInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities)
{
	if (userIndex != 0)
		return 1167; // @todo find macro for this

	LOG("XInputHook: hookXInputGetCapabilities id=" << userIndex <<
		", flags=" << flags << endl);

	capabilities->Type = 1;
	capabilities->SubType = 1;
	capabilities->Flags = 4;
	capabilities->Gamepad.bLeftTrigger = 1;
	capabilities->Gamepad.bRightTrigger = 1;
	capabilities->Gamepad.sThumbLX = 1;
	capabilities->Gamepad.sThumbLY = 1;
	capabilities->Gamepad.sThumbRX = 1;
	capabilities->Gamepad.sThumbRY = 1;
	capabilities->Gamepad.wButtons = 62463;
	capabilities->Vibration.wLeftMotorSpeed = 1;
	capabilities->Vibration.wRightMotorSpeed = 1;

	return ERROR_SUCCESS;
}

synxinhk_API LRESULT CALLBACK
HookProc(int code, WPARAM wParam, LPARAM lParam) 
{
	return CallNextHookEx(s_hook, code, wParam, lParam);
}

synxinhk_API BOOL
InstallXInputHook()
{
	if (_stricmp(XINPUT_DLL, REQUIRED_XINPUT_DLL) != 0)
	{
		LOG("XInputHook: DLL not supported: " << XINPUT_DLL << endl);
		return FALSE;
	}

	OutputDebugString("XInputHook: installing hook\n");
	s_hook = SetWindowsHookEx(WH_CBT, HookProc, dll, 0);
	OutputDebugString("XInputHook: hook installed\n");

	return TRUE;
}

synxinhk_API void
RemoveXInputHook()
{
	OutputDebugString("XInputHook: removing hook\n");
	UnhookWindowsHookEx(s_hook);
	OutputDebugString("CXInputHook: hook removed\n");
}