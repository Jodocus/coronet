#pragma once
#include <experimental/coroutine>

#include <system_error>
#include <string>
#include <thread>
#include <vector>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#include "task.hpp"
#include "except.hpp"

struct Instance final {
	Instance() {
		WSADATA wsa{ };
		if(::WSAStartup(MAKEWORD(2, 2), &wsa) == SOCKET_ERROR) raise::net(u8"WSAStartup");
	}

	Instance(const Instance&) = delete;
	Instance(Instance&&) = delete;
	Instance& operator =(const Instance&) = delete;
	Instance& operator =(Instance&&) = delete;

	void close() { if(::WSACleanup() == SOCKET_ERROR) raise::net(u8"WSACleanup"); }

	~Instance() { try { close(); } catch(...) { } }
};

struct Accept_awaiter;
struct Receive_awaiter;
struct Send_awaiter;

class Queue;
class Endpoint;

class Socket {
	SOCKET _sock;
public:
	friend struct Accept_awaiter;
	friend struct Receive_awaiter;
	friend struct Send_awaiter;

	Socket(Queue&);
	~Socket();
	Socket(Socket&) = delete; 
	Socket(Socket&&) noexcept;
	Socket& operator =(const Socket&) = delete;
	Socket& operator =(Socket&&) noexcept;

	void swap(Socket&) noexcept;
	void close();

	void bind(const Endpoint&);
	void listen();

	Accept_awaiter async_accept(Queue&);
	template <std::size_t N>
	Receive_awaiter async_receive(char(&)[N]);
	template <std::size_t N>
	Send_awaiter async_send(const char(&)[N]);
};

struct Awaiter {
	std::experimental::coroutine_handle<task::promise_type> coro;
	Socket& sock;

	Awaiter(Socket& s) : sock{ s }, coro{ } { }
};

struct Accept_awaiter : Awaiter {
	Socket conn;
	char buf[2 * (sizeof(sockaddr_storage) + 16)];

	Accept_awaiter(Socket& l, Queue& q) : Awaiter(l), conn{ q }, buf{ } { }

	bool await_ready() { return false; }
	void await_suspend(std::experimental::coroutine_handle<task::promise_type>);
	Socket await_resume() { return { std::move(conn) }; }
};

struct Receive_awaiter : Awaiter {
	char* buf;
	std::size_t size;

	Receive_awaiter(Socket& s, char* b, std::size_t ss) : Awaiter(s), buf{ b }, size{ ss } { }

	bool await_ready() { return false; }
	void await_suspend(std::experimental::coroutine_handle<task::promise_type>);
	std::size_t await_resume() { return coro.promise().bytes_transferred; }
};

struct Send_awaiter : Awaiter {
	const char* buf;
	std::size_t size;

	Send_awaiter(Socket& s, const char* b, std::size_t ss) : Awaiter(s), buf{ b }, size{ ss } { }

	bool await_ready() { return false; }
	void await_suspend(std::experimental::coroutine_handle<task::promise_type>);
	std::size_t await_resume() { return coro.promise().bytes_transferred; coro.destroy(); }
};

template <std::size_t N>
Receive_awaiter Socket::async_receive(char(&buf)[N]) { return { *this, buf, N }; }

template <std::size_t N>
Send_awaiter Socket::async_send(const char(&buf)[N]) { return { *this, buf, N }; }
