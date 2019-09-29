//
// Created by wuyuanyi on 2019-09-26.
//

#ifndef RXSIMEX_RX_REMOTE_BLOCK_H
#define RXSIMEX_RX_REMOTE_BLOCK_H
#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif
#include "brynet/net/TCPService.h"
#include "brynet/net/Wrapper.h"
#include "msgpack.hpp"
#include "rx_block.h"
using namespace brynet::net;
using bytes = std::vector<char>;

namespace simex {

namespace message {

struct block_info {
  double sample_time{};
  double offset_time{};
  std::string path;
  MSGPACK_DEFINE_MAP(sample_time, offset_time, path);
};

struct port_info {
  std::string name;
  int id{};
  std::vector<int> dimensions;
  int type_id{};
  MSGPACK_DEFINE_MAP(name, id, dimensions, type_id);
};

struct info {
  block_info block_info;
  std::vector<port_info> input_port_info;
  std::vector<port_info> output_port_info;

  MSGPACK_DEFINE_MAP(block_info, input_port_info, output_port_info);
};

}  // namespace message

struct rx_data_buffer {
  std::weak_ptr<TcpConnection> session;
  std::shared_ptr<bytes> data;
  rx_data_buffer(TcpConnection::Ptr session, const char *buffer, size_t len)
      : session(session->weak_from_this()), data(std::make_shared<bytes>()) {
    data->assign(buffer, buffer + len);
  }
};

struct rx_msgpack_object {
  std::weak_ptr<TcpConnection> session;
  std::shared_ptr<msgpack::object_handle> object_handle;
  rx_msgpack_object(std::shared_ptr<TcpConnection> session,
                    std::shared_ptr<msgpack::object_handle> object_handle)
      : session(session->weak_from_this()), object_handle(object_handle) {}
};
class rx_remote_block : public rx_block {
 public:
  explicit rx_remote_block(SimStruct *s) : rx_block(s) {}

 protected:
  TcpService::Ptr tcp_service_ptr;
  std::vector<TcpConnection::Ptr> sessions;
  brynet::net::wrapper::ListenerBuilder listener;

 public:
  rxcpp::subjects::subject<TcpConnection::Ptr> session_disconnected;
  rxcpp::subjects::subject<TcpConnection::Ptr> session_connected;

  rxcpp::subjects::subject<std::shared_ptr<rx_data_buffer>> raw_data_received;
  rxcpp::subjects::subject<std::shared_ptr<rx_msgpack_object>>
      msgpack_object_parsed;

 protected:
  std::unique_ptr<rxcpp::observe_on_one_worker> io_thread;

 protected:
  std::unordered_map<std::string, std::shared_ptr<port_base>>
      remote_input_port_registry;
  std::unordered_map<std::string, std::shared_ptr<port_base>>
      remote_output_port_registry;

 protected:
  int port = 12305;

 public:
  void on_start() override {
    rx_block::on_start();
    tcp_service_ptr = TcpService::Create();
    tcp_service_ptr->startWorkerThread(1);

    auto enterCallback = [&](const TcpConnection::Ptr &session) {
      // assign a msgpack unpacker for each session
      auto unpacker = std::make_shared<msgpack::unpacker>();

      session->setDataCallback([=](const char *buffer, size_t len) {
        // data received from the remote
        raw_data_received      //
            .get_subscriber()  //
            .on_next(std::make_shared<rx_data_buffer>(session, buffer, len));

        // parse received bytes into msgpack
        unpacker->reserve_buffer(len);
        memcpy(unpacker->buffer(), buffer, len);
        unpacker->buffer_consumed(len);

        auto result = std::make_shared<msgpack::object_handle>();
        while (unpacker->next(*result)) {
          auto sending_object =
              std::make_shared<rx_msgpack_object>(session, result);
          msgpack_object_parsed  //
              .get_subscriber()  //
              .on_next(sending_object);
        }
        return len;
      });

      session->setDisConnectCallback([&](const TcpConnection::Ptr &session) {
        sessions.erase(std::remove(sessions.begin(), sessions.end(), session),
                       sessions.end());
        session_disconnected   //
            .get_subscriber()  //
            .on_next(session);
      });

      session_connected.get_subscriber().on_next(session);
    };

    // configure socket listener
    listener                                //
        .configureService(tcp_service_ptr)  //
        .configureSocketOptions(
            {[](TcpSocket &socket) { socket.setNodelay(); }})  //
        .configureConnectionOptions(
            {//
             brynet::net::TcpService::AddSocketOption::WithMaxRecvBufferSize(
                 1024 * 1024),  //
             brynet::net::TcpService::AddSocketOption::AddEnterCallback(
                 enterCallback)})  //
        .configureListen([=](wrapper::BuildListenConfig config) {
          config.setAddr(false, "0.0.0.0", port);
        })  //
        .asyncRun();

    // configure subscriptions
    io_thread = std::make_unique<rxcpp::observe_on_one_worker>(
        rxcpp::observe_on_new_thread());
    msgpack_object_parsed        //
        .get_observable()        //
        .observe_on(*io_thread)  //
        .subscribe(subscriptions,
                   [&](std::shared_ptr<rx_msgpack_object> message) {
                     handle_incoming_message(std::move(message));
                   });
  }

  void on_terminate() override { rx_block::on_terminate(); }

  void set_remote_accessible_port(std::shared_ptr<port_base> p, bool is_input) {
    if (is_input) {
      assert(remote_input_port_registry.count(p->name) == 0);
      remote_input_port_registry[p->name] = p;
    } else {
      assert(remote_output_port_registry.count(p->name) == 0);
      remote_output_port_registry[p->name] = p;
    }
  }

 protected:
  // msgpack functions
  void send_info(std::shared_ptr<TcpConnection> session) {
    std::stringstream ss;
    msgpack::packer packer(ss);
    packer.pack_array(2);
    packer.pack("resp_info");

    message::info info;
    info.block_info.offset_time = sample_time.offset_time;
    info.block_info.sample_time = sample_time.sample_time;
    info.block_info.path = std::string(S->path);

    for (auto &ip : input_ports) {
      message::port_info port_info;
      port_info.name = ip->name;
      port_info.id = ip->id;
      port_info.type_id = ip->type_id;
      std::vector<int> dims;
      dims.assign(ip->dims.begin(), ip->dims.end());
      port_info.dimensions = dims;
      info.input_port_info.emplace_back(port_info);
    }
    for (auto &op : output_ports) {
      message::port_info port_info;
      port_info.name = op->name;
      port_info.id = op->id;
      port_info.type_id = op->type_id;
      std::vector<int> dims;
      dims.assign(op->dims.begin(), op->dims.end());
      port_info.dimensions = dims;
      info.output_port_info.emplace_back(port_info);
    }
    packer.pack(info);
    session->send(ss.str().data(), ss.str().length());
  }

  void update_from_remote(msgpack::object &object) {
    std::string name;
    auto identifier_and_data = object.as<std::vector<msgpack::object>>();
    std::shared_ptr<port_base> target_port;

    switch (identifier_and_data[0].type) {
      case msgpack::type::STR: {
        // by name
        name = object.as<std::string>();
        if (remote_output_port_registry.count(name) == 0) {
          throw std::runtime_error(
              fmt::format("name: {} is not registered", name));
        }
        target_port = remote_output_port_registry[name];
        break;
      }
      case msgpack::type::NEGATIVE_INTEGER:
      case msgpack::type::POSITIVE_INTEGER: {
        // by id
        auto id = object.as<int64_t>();
        auto it = std::find_if(remote_output_port_registry.begin(),
                               remote_output_port_registry.end(),
                               [&](auto p) { return p.second->id == id; });
        if (it == remote_output_port_registry.end()) {
          throw std::runtime_error(fmt::format("id: {} is not registered", id));
        }
        target_port = it->second;
        break;
      }
      default: {
        throw std::runtime_error(fmt::format(
            "Unrecognized port update identifier: {}", object.type));
      }
    }

    // port found. update value.
    auto data = identifier_and_data[1].as<std::string>();
    const size_t expected = target_port->get_width();
    const size_t actual = data.length();
    if (expected != actual) {
      throw std::runtime_error(
          fmt::format("Failed to update port (id={}, name={}) with data of "
                      "size={}. Expected size={}",
                      target_port->id, target_port->name, actual, expected));
    }
    target_port->from(data.data());
  }

  void handle_incoming_message(std::shared_ptr<rx_msgpack_object> message) {
    // stop processing the message if the sender is already destroyed
    auto sender_weak = message->session;
    auto sender = sender_weak.lock();
    if (!sender) {
      return;
    }

    auto object = message->object_handle->get();
    try {
      auto outer_wrapper = object.as<std::vector<msgpack::object>>();
      // first item: string for packet intention
      auto intention = outer_wrapper[0].as<std::string>();

      if (intention == "query_info") {
        send_info(sender);
      } else if (intention == "value") {
        // will have one extra parameter
        update_from_remote(outer_wrapper[1]);
      } else {
        // no matching
      }

    } catch (std::exception &e) {
      log("error", e.what());
    }
  }
};
}  // namespace simex

#endif  // RXSIMEX_RX_REMOTE_BLOCK_H
