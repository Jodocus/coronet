#include "except.hpp"
#include <system_error>

namespace raise {
	[[noreturn]] void net(const char* msg, int code) {
		throw std::system_error{ std::error_code{ code, std::system_category() }, msg };
	}

	[[noreturn]] void sys(const char* msg, DWORD code) {
		throw std::system_error{ std::error_code(code, std::system_category()), msg };
	}
}
