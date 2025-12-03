#include "CServer.h"
#include "HttpConnection.h"

// 构造函数
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port) 
	:_ioc(ioc),_acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _socket(ioc) {

}

// Start函数，用来监听新链接
void CServer::Start()
{
    auto self = shared_from_this();
    _acceptor.async_accept(_socket, [self](beast::error_code ec) {
        try {
            //出错则放弃这个连接，继续监听新链接
            if (ec) {
                self->Start();
                return;
            }

            //处理新链接，创建HttpConnection类管理新连接
            std::make_shared<HttpConnection>(std::move(self->_socket))->Start();        // 这里实际上是把底层的nativesocket传递给httpconnection类中的socket，原本的socket可以继续accept
            //继续监听
            self->Start();
        }
        catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        }
        });
}