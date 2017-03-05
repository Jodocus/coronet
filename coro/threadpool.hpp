#pragma once
#include <vector>
#include <thread>

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
