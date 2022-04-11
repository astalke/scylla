#pragma once

#include "seastar/core/future.hh"
#include "seastar/core/shared_ptr.hh"
#include "seastar/core/sharded.hh"

#include "seastar/websocket/server.hh"

#include "protocol_server.hh"
#include "data_dictionary/data_dictionary.hh"

#include "seastarx.hh"

namespace websocket {

constexpr const char* version = "RFC 6455";

class controller : public protocol_server {
    sharded<experimental::websocket::server> _server;
    bool _active;
    std::vector<socket_address> _listen_addresses;
public:
    controller();
    ~controller();
    virtual sstring name() const override;
    virtual sstring protocol() const override;
    virtual sstring protocol_version() const override;
    virtual std::vector<socket_address> listen_addresses() const override;
    virtual future<> start_server() override;
    virtual future<> stop_server() override;
    virtual future<> request_stop_server() override;
};

}
