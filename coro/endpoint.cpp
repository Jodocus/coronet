#include "endpoint.hpp"
#include "enc.hpp"
#include "except.hpp"

#include <WS2tcpip.h>

Endpoint::Endpoint(const std::string& host, const std::string& service, bool passive)
	: _rep{ nullptr } 
{
	ADDRINFOW hints{ };
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_ALL | AI_V4MAPPED | (passive ? AI_PASSIVE : 0);

	const int err = ::GetAddrInfoW(host.empty() ? nullptr : code_cast<std::wstring>(host).c_str(),
		code_cast<std::wstring>(service).c_str(), nullptr, &_rep);
	if(err != 0) raise::net(u8"GetAddrInfoW", err);
}

Endpoint::Endpoint(const std::string& service, bool passive) 
	: Endpoint({ }, service, passive) { }

Endpoint::Endpoint(Endpoint&& e) noexcept
	: _rep{ e._rep } {
	e._rep = nullptr;
}

Endpoint& Endpoint::operator =(Endpoint&& e) noexcept {
	if(this != &e) swap(e);
	return *this;
}

void Endpoint::swap(Endpoint& e) noexcept {
	using std::swap;
	swap(e._rep, _rep);
}

Endpoint::~Endpoint() {
	if(_rep != nullptr) ::FreeAddrInfoW(_rep);
}