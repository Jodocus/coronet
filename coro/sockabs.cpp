#include "sockabs.hpp"
#include <MSWSock.h>

Accept_awaiter Socket::async_accept(Queue& q) { return { *this, q }; }

void Send_awaiter::await_suspend(std::experimental::coroutine_handle<task::promise_type> resume) {
	resume.promise().coro = resume;
	coro = resume;

	WSABUF wb;
	wb.buf = const_cast<char*>(buf);
	wb.len = static_cast<ULONG>(size);
	DWORD bytes{ };
	DWORD flags{ };
	if(::WSASend(sock._sock, &wb, 1, &bytes, 0, &resume.promise(), nullptr) == SOCKET_ERROR) {
		int err = ::WSAGetLastError();
		if(err != WSA_IO_PENDING) raise::net(u8"WSASend");
	}
}

void Receive_awaiter::await_suspend(std::experimental::coroutine_handle<task::promise_type> resume) {
	resume.promise().coro = resume;
	coro = resume;

	WSABUF wb;
	wb.buf = buf;
	wb.len = static_cast<ULONG>(size);
	DWORD bytes{ };
	DWORD flags{ };
	if(::WSARecv(sock._sock, &wb, 1, &bytes, &flags, &resume.promise(), nullptr) == SOCKET_ERROR) {
		int err = ::WSAGetLastError();
		if(err != WSA_IO_PENDING) raise::net(u8"WSARecv");
	}
}

void Accept_awaiter::await_suspend(std::experimental::coroutine_handle<task::promise_type> resume) {
	coro = resume;
	resume.promise().coro = resume;
	static struct al {
		al(SOCKET sock) : accept{ nullptr } {
			GUID accept_id = WSAID_ACCEPTEX;
			DWORD bytes{ };
			if(::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &accept_id, sizeof accept_id,
				&accept, sizeof accept, &bytes, 0, 0) == SOCKET_ERROR) raise::net(u8"WSAIoctl failure");
		}

		LPFN_ACCEPTEX accept;
	} ldr{ sock._sock };

	DWORD bytes_transferred{ };
	if(ldr.accept(sock._sock, conn._sock, buf, 0, sizeof(sockaddr_storage) + 16, sizeof(sockaddr_storage) + 16,
		reinterpret_cast<DWORD*>(&bytes_transferred), &resume.promise()) == FALSE)
	{
		int err = ::WSAGetLastError();
		if(err != WSA_IO_PENDING) raise::net(u8"AcceptEx");
	}
}