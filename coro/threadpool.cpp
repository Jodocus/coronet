#include "threadpool.hpp"

Threadpool::Threadpool(Threadpool&& tp) noexcept
	: _pool{ std::move(tp._pool) } { }

Threadpool& Threadpool::operator =(Threadpool&& tp) noexcept {
	if(&tp != this) swap(tp);
	return *this;
}

void Threadpool::swap(Threadpool& tp) noexcept {
	using std::swap;
	swap(tp._pool, _pool);
}

void Threadpool::join() {
	for(auto& t : _pool) if(t.joinable()) t.join();
}