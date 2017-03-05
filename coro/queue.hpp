#pragma once
#include <WinSock2.h>
#include <cstdlib>

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
