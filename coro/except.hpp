#pragma once
#include <WinSock2.h>

namespace raise {
	[[noreturn]] void net(const char*, int code = ::WSAGetLastError());
	[[noreturn]] void sys(const char*, DWORD code = ::GetLastError());
}
