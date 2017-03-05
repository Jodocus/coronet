#pragma once
#include <WinSock2.h>
#include <system_error>
#include <experimental/coroutine>

struct operation : WSAOVERLAPPED {
	std::error_code ec;
	std::size_t bytes_transferred;
	std::experimental::coroutine_handle<> coro;

	operation() : WSAOVERLAPPED{ }, ec{ }, coro{ }, bytes_transferred{ } { }
};

extern thread_local std::size_t* cache;

struct task {
	struct promise_type : operation {
		task get_return_object() { return { std::experimental::coroutine_handle<promise_type>::from_promise(*this) }; }
		std::experimental::suspend_never initial_suspend() { return { }; }
		std::experimental::suspend_never final_suspend() { return { }; }

		void* operator new(std::size_t count) {
			if(cache != nullptr) {
				void* ret = cache;
				cache = reinterpret_cast<std::size_t*>(*cache);
				return ret;
			}
			return ::operator new(count);
		}

		void operator delete(void* ptr) {
			*static_cast<std::size_t*>(ptr) = reinterpret_cast<std::size_t>(cache);
			cache = static_cast<std::size_t*>(ptr);
		}
	};

	std::experimental::coroutine_handle<promise_type> coro;
	task(std::experimental::coroutine_handle<promise_type> h) : coro{ h } { }

	void cancel() {
		auto prom = coro.promise();
	}
};