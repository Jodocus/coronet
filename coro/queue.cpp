#include "queue.hpp"
#include "except.hpp"
#include "task.hpp"

#include <utility>
#include <system_error>

Queue::Queue(std::size_t concurrency_hint)
	: _iocp{ ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, static_cast<DWORD>(concurrency_hint)) }
{
	if(_iocp == NULL) raise::sys(u8"CreateIoCompletionPort");
}

Queue::Queue(Queue&& q) noexcept
	: _iocp{ q._iocp }
{
	q._iocp = NULL;
}

Queue::~Queue() {
	try { close(); } catch(...) { }
}

Queue& Queue::operator =(Queue&& q) noexcept {
	if(this != &q) swap(q);
	return *this;
}

void Queue::swap(Queue& q) noexcept {
	using std::swap;
	swap(_iocp, q._iocp);
}

void Queue::close() {
	if(::CloseHandle(_iocp) == FALSE) raise::sys(u8"CloseHandle");
}

void Queue::dequeue() {
	for(;;) {
		WSAOVERLAPPED* ol{ };
		DWORD bytes{ };
		ULONG_PTR key{ };
		int error{ };
		if(::GetQueuedCompletionStatus(_iocp, &bytes, &key, &ol, 3000) == FALSE) {
			DWORD err = ::GetLastError();
			if(ol == nullptr) raise::sys(u8"GetQueuedCompletionStatus", err);
			DWORD flags{ };
			if(::WSAGetOverlappedResult(static_cast<SOCKET>(key), ol, &bytes, FALSE, &flags) == FALSE)
				error = ::WSAGetLastError();
		}
		auto op = static_cast<task::promise_type*>(ol);
		op->bytes_transferred = bytes;
		op->ec.assign(error, std::system_category());
		op->coro.resume();
	}
}
