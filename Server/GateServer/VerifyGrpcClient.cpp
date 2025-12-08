#include "VerifyGrpcClient.h"



VerifyGrpcClient::VerifyGrpcClient() {
    //// 创建到 gRPC 服务器的通道
    //std::shared_ptr<Channel> channel = grpc::CreateChannel(
    //    "127.0.0.1:50051",      // 服务器地址和端口
    //    grpc::InsecureChannelCredentials()      // 不使用 TLS 加密
    //);                          

    //// 创建服务存根（stub），用于调用远程方法
    //stub_ = VerifyService::NewStub(channel);

    auto& gCfgMgr = ConfigMgr::Inst();
    std::string host = gCfgMgr["VarifyServer"]["Host"];
    std::string port = gCfgMgr["VarifyServer"]["Port"];
    pool_.reset(new RPConPool(5, host, port));
}