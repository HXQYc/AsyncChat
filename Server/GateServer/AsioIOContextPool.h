#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"

class AsioIOContextPool:public Singleton<AsioIOContextPool>
{
	friend Singleton<AsioIOContextPool>;
public:
	using IOContext = boost::asio::io_context;
	using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	using WorkPtr = std::unique_ptr<WorkGuard>;

	~AsioIOContextPool();
	AsioIOContextPool(const AsioIOContextPool&) = delete;
	AsioIOContextPool& operator= (const AsioIOContextPool&) = delete;

	// 使用round-robin轮询算法依次返回不同的ioc
	boost::asio::io_context& GetIOContext();
	// 停止所有ioc服务
	void Stop();

private:
	AsioIOContextPool(std::size_t = std::thread::hardware_concurrency());
	std::vector<IOContext> _ioContexts;		// io服务
	std::vector<WorkPtr> _workGuards;		// 工作对象指针容器，保持io服务运行
	std::vector<std::thread> _threads;		// 工作线程容器
	std::size_t _nextIOContext;				// 下一个要分配的io上下文索引，用于轮询
};

