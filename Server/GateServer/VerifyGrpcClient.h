#pragma once
#include <grpcpp/grpcpp.h>      // gRPC C++ 核心库
#include "message.grpc.pb.h"    // 通过 protobuf 编译器生成的 gRPC 桩代码头文件
#include "const.h"
#include "Singleton.h"

using grpc::Channel;            // gRPC 通信通道
using grpc::Status;             // gRPC 操作状态（包含成功/失败信息）
using grpc::ClientContext;      // 客户端上下文（用于设置超时、元数据等）

using message::GetVerifyReq;    // 请求消息类型（注意：Varify 拼写应为 Verify）
using message::GetVerifyRsp;    // 响应消息类型
using message::VerifyService;   // gRPC 服务接口

class RPConPool {
public:
    RPConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
        for (size_t i = 0; i < poolSize_; ++i) {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
                grpc::InsecureChannelCredentials());
            connections_.push(VerifyService::NewStub(channel));
        }
    }
    ~RPConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        Close();
        while (!connections_.empty()) {
            connections_.pop();
        }
    }
    std::unique_ptr<VerifyService::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !connections_.empty();
            });
        //如果停止则直接返回空指针
        if (b_stop_) {
            return  nullptr;
        }
        auto context = std::move(connections_.front());
        connections_.pop();
        return context;
    }
    void returnConnection(std::unique_ptr<VerifyService::Stub> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        connections_.push(std::move(context));
        cond_.notify_one();
    }
    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }
private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<VerifyService::Stub>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

class VerifyGrpcClient :public Singleton<VerifyGrpcClient>
{
    friend class Singleton<VerifyGrpcClient>;
public:

    GetVerifyRsp GetvarifyCode(std::string email) {
        ClientContext context;      // 创建客户端上下文对象
        GetVerifyRsp reply;         // 创建响应对象
        GetVerifyReq request;       // 创建请求对象
        request.set_email(email);   // 设置请求的 email 字段

        //// 调用 gRPC 远程方法
        //Status status = stub_->GetVerifyCode(&context, request, &reply);

        //if (status.ok()) {          // 检查 RPC 调用是否成功
        //    return reply;           // 返回正常响应
        //}
        //else {
        //    reply.set_error(ErrorCodes::RPCFailed);  // 设置错误码
        //    return reply;           // 返回错误响应
        //}

        auto stub = pool_->getConnection();
        Status status = stub->GetVerifyCode(&context, request, &reply);
        if (status.ok()) {
            pool_->returnConnection(std::move(stub));
            return reply;
        }
        else {
            pool_->returnConnection(std::move(stub));
            reply.set_error(ErrorCodes::RPCFailed);
            return reply;
        }
    }

private:
    VerifyGrpcClient();

    // std::unique_ptr<VerifyService::Stub> stub_;
    std::unique_ptr<RPConPool> pool_;
};

