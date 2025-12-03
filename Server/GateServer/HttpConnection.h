#pragma once
#include "const.h"

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem;
public:
    HttpConnection(tcp::socket socket);
    void Start();

private:
    void CheckDeadline();       // 监测超时，定时器
    void WriteResponse();       // 应答
    void HandleReq();           // 处理请求
    void PreParseGetParam();    // 解析url，保存为键值对



    tcp::socket  _socket;

    // 用来接收数据 The buffer for performing reads.
    beast::flat_buffer  _buffer{ 8192 };

    // 用来解析请求（dynamic动态） The request message.
    http::request<http::dynamic_body> _request;

    // 用来回应客户端 The response message.
    http::response<http::dynamic_body> _response;

    // 用来做定时器判断请求是否超时 The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        _socket.get_executor(), std::chrono::seconds(60)    // 调度器、六十秒计时
    }; // 使用{}列表初始化可以防止()语义上的歧义

    // 解析url，保存为键值对
    std::string _get_url;
    std::unordered_map<std::string, std::string> _get_params;
};

