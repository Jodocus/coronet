#include <iostream>
#include <experimental/coroutine>
#include "sockabs.hpp"
#include "threadpool.hpp"
#include "queue.hpp"
#include "endpoint.hpp"
#include "enc.hpp"

thread_local std::size_t* cache = nullptr;

void worker(Instance&, Queue&, Socket&);

int main() 
try {
	SetConsoleOutputCP(1252);
	Instance inst{ };
	Queue q;
	Socket s{ q };
	s.bind({ "http", true });
	s.listen();

	Threadpool{ [&inst, &q, &s]() { worker(inst, q, s); } }.join();
}
catch(const std::system_error& e) {
	std::cerr << "Exception (Code: " << e.code().value() << ")!\n\t" << e.what();
}

Task handle(Socket s) {
	char buf[512]{ };
	auto transferred = co_await s.async_receive(buf);
	char answer[] = u8"HTTP/1.1 200 OK\r\nServer: GehtDichNixAn!\r\nContent-type: text/html; charset=utf-8\r\nContent-length: 112\r\n\r\n"
		u8"<!doctype html><html><head><title>It works!</title></head><body><h1>It works!</h1><hr />Well done!</body></html>";
	co_await s.async_send(answer);
}

Task server(Queue& q, Socket& l) {
	for(;;) try {
		handle(co_await l.async_accept(q));
	}
	catch(std::system_error& e) {
		std::cerr << e.code().value() << ' ' << e.what();
	}
}

void worker(Instance& inst, Queue& q, Socket& l)
try {
	server(q, l);
	q.dequeue();
}
catch(...) { }
