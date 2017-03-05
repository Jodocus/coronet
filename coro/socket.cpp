#include "sockabs.hpp"

Socket::Socket(Queue& q)
	: _sock{ ::WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED) }
{
	if(_sock == INVALID_SOCKET) raise::net(u8"WSASocket");
	if(::CreateIoCompletionPort(reinterpret_cast<HANDLE>(_sock), q._iocp, 0, 0) == FALSE)
		raise::sys(u8"CreateIoCompletionPort");
}

Socket::~Socket() {
	try {
		if(_sock != INVALID_SOCKET) close();
	} catch(...) { }
}

Socket::Socket(Socket&& s) noexcept
	: _sock(s._sock)
{
	s._sock = INVALID_SOCKET;
}

void Socket::swap(Socket& s) noexcept {
	using std::swap;
	swap(s._sock, _sock);
}

Socket& Socket::operator =(Socket&& s) noexcept {
	if(&s != this) swap(s);
	return *this;
}

void Socket::close() {
	if(::closesocket(_sock) == SOCKET_ERROR) raise::net(u8"closesocket");
}

void Socket::bind(const Endpoint& ep) {
	if(::bind(_sock, ep._rep->ai_addr, static_cast<int>(ep._rep->ai_addrlen)) == SOCKET_ERROR)
		raise::net(u8"bind");
}

void Socket::listen() {
	if(::listen(_sock, SOMAXCONN) == SOCKET_ERROR) raise::net(u8"listen");
}
