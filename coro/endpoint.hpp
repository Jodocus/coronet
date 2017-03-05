#pragma once
#include <WinSock2.h>
#include <string>

class Endpoint final {
	ADDRINFOW* _rep;
public:
	friend class Socket;
	Endpoint() = default;
	Endpoint(const std::string&, bool = false);
	Endpoint(const std::string&, const std::string&, bool = false);
	Endpoint(const Endpoint&) = delete;
	Endpoint(Endpoint&&) noexcept;
	Endpoint& operator =(const Endpoint&) = delete;
	Endpoint& operator =(Endpoint&&) noexcept;
	~Endpoint();

	void swap(Endpoint&) noexcept;
};
