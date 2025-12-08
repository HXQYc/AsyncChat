#include "AsioIOContextPool.h"
#include <iostream>

AsioIOContextPool::AsioIOContextPool(std::size_t size) :
	_ioContexts(size), _nextIOContext(0)
{
	for (std::size_t i = 0; i < size; ++i)
	{
		_workGuards.push_back(
			std::make_unique<WorkGuard>(
				boost::asio::make_work_guard(_ioContexts[i])
		));
	}

	for (std::size_t i = 0; i < size; ++i)
	{
		_threads.emplace_back(
			[this, i]() {
				_ioContexts[i].run();
			});
	}
}

AsioIOContextPool::~AsioIOContextPool()
{
	Stop();
	std::cout << "AsioIOContextPool destruct" << std::endl;
}

boost::asio::io_context& AsioIOContextPool::GetIOContext()
{
	auto& ioc = _ioContexts[_nextIOContext++];
	if (_nextIOContext == _ioContexts.size())
	{
		_nextIOContext = 0;
	}
	return ioc;
}

void AsioIOContextPool::Stop() {
	//因为仅仅执行work.reset并不能让iocontext从run的状态中退出
	//当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
	// 先停止所有 io_context
	for (auto& ioc : _ioContexts) {
		ioc.stop();
	}

	// 重置 work guard
	for (auto& work : _workGuards) {
		work.reset();
	}

	// 等待所有线程结束
	for (auto& t : _threads) {
		if (t.joinable()) {
			t.join();
		}
	}
}