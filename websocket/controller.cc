#include "websocket/controller.hh"
#include "service/storage_proxy.hh"
#include <seastar/core/coroutine.hh>

namespace websocket {

controller::controller() : _active(false)
{
}

controller::~controller()
{
}

sstring controller::name() const {
    return "websocket";
}

sstring controller::protocol() const {
    return "websocket";
}

sstring controller::protocol_version() const {
    return ::websocket::version;
}

std::vector<socket_address> controller::listen_addresses() const {
    return _listen_addresses;
}

future<> controller::start_server()
{
    if (_active) {
        co_return;
    }
    _active = true;
    co_await _server.start();
    co_await _server.invoke_on_all([] (experimental::websocket::server& ws) {
        ws.register_handler("echo", [] (input_stream<char>& in, output_stream<char>& out) -> future<> {
            while (true) {
                temporary_buffer<char> f = co_await in.read();
                if (f.empty()) {
                    co_return;
                }
                co_await out.write(std::move(f));
                co_await out.flush();
            }
        });
        ws.listen(socket_address(ipv4_addr("127.0.0.1", 8222)));
    });
}

future<> controller::stop_server()
{
    if (_active) {
        return _server.stop().then([this] {
            _listen_addresses.clear();
            return make_ready_future<>();
        });
    }
    return make_ready_future<>();
}

future<> controller::request_stop_server() {
    return stop_server();
}

}
