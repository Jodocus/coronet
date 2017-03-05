#pragma once
#include <experimental/coroutine>

#include <system_error>
#include <string>
#include <thread>
#include <vector>

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

namespace raise {
	[[noreturn]] void net(const char*, int code = ::WSAGetLastError());
	[[noreturn]] void sys(const char*, DWORD code = ::GetLastError());
}

struct operation : WSAOVERLAPPED {
	std::error_code ec;
	std::size_t bytes_transferred;
	std::experimental::coroutine_handle<> coro;

	operation() : WSAOVERLAPPED{ }, ec{ }, coro{ }, bytes_transferred { } { }
};

struct task {
	struct promise_type : operation {
		task get_return_object() { return { std::experimental::coroutine_handle<promise_type>::from_promise(*this) }; }
		std::experimental::suspend_never initial_suspend() { return { }; }
		std::experimental::suspend_always final_suspend() { return { }; }
	};

	std::experimental::coroutine_handle<promise_type> coro;
	task(std::experimental::coroutine_handle<promise_type> h) : coro{ h } { }

	void cancel() {
		auto prom = coro.promise();
	}
};

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

class Queue {
	HANDLE _iocp;
public:
	friend class Socket;
	Queue(std::size_t = 0);
	Queue(const Queue&) = delete;
	Queue(Queue&&) noexcept;
	Queue& operator =(const Queue&) = delete;
	Queue& operator =(Queue&&) noexcept;
	~Queue();

	void swap(Queue&) noexcept;
	void close();

	void dequeue();
};

struct Accept_awaiter;
struct Receive_awaiter;
struct Send_awaiter;

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

class Threadpool {
	std::vector<std::thread> _pool;
public:
	template <typename T>
	Threadpool(T f, std::size_t size = std::thread::hardware_concurrency() * 2)
		: _pool(size) 
	{
		for(auto& t : _pool) t.swap(std::thread{ f });
	}

	Threadpool(const Threadpool&) = delete;
	Threadpool(Threadpool&&) noexcept;
	Threadpool& operator =(const Threadpool&) = delete;
	Threadpool& operator =(Threadpool&&) noexcept;

	void swap(Threadpool&) noexcept;
	void join();
};
