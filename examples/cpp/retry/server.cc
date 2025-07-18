/*
 * Copyright 2024 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
 public:
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    if (++request_counter_ % request_modulo_ != 0) {
      // Return an OK status for every request_modulo_ number of requests,
      // return UNAVAILABLE otherwise.
      std::cout << "return UNAVAILABLE" << std::endl;
      return Status(StatusCode::UNAVAILABLE, "");
    }
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    std::cout << "return OK" << std::endl;
    return Status::OK;
  }

 private:
  static constexpr int request_modulo_ = 4;
  int request_counter_ = 0;
};

void RunServer(uint16_t port) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  GreeterServiceImpl service;// 初始化所属服务器和服务器的调用方法

  grpc::EnableDefaultHealthCheckService(true);// 默认为不开启健康检测
  // 将能创建 grpc::ServerBuilderPlugin 对象的函数添加进 g_plugin_factory_list
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  // 将 g_plugin_factory_list 中的函数创建的 grpc::ServerBuilderPlugin 对象添加进 builder
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);// 将当前的 service 加入 builder 管理
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  RunServer(/*port=*/50052);
  return 0;
}
