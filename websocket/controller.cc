#include "websocket/controller.hh"
#include "service/storage_proxy.hh"
#include <seastar/core/coroutine.hh>
#include "db/config.hh"

namespace websocket {

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
    co_await _server.invoke_on_all([this] (experimental::websocket::server& ws) {
        ws.register_handler("cql", [this] (input_stream<char>& in, output_stream<char>& out) -> future<> {
            auto unix_server_socket = seastar::listen(socket_address{unix_domain_addr{""}});
            auto unix_addr = unix_server_socket.local_address();
            auto accept_fut = unix_server_socket.accept();
            auto sock = co_await connect(unix_addr);
            auto accept_result = co_await std::move(accept_fut);
            auto connection = _cql_server.local().inject_connection(unix_addr,
                    std::move(accept_result.connection), std::move(accept_result.remote_address));

            input_stream<char> cql_in = sock.input();
            output_stream<char> cql_out = sock.output();

            auto bridge = [] (input_stream<char>& input, output_stream<char>& output) -> future<> {
                while (true) {
                    temporary_buffer<char> buff = co_await input.read();
                    if (buff.empty()) {
                        co_return;
                    }
                    co_await output.write(std::move(buff));
                    co_await output.flush();
                }
            };

            auto read_loop = bridge(in, cql_out);
            auto write_loop = bridge(cql_in, out);

            co_await std::move(write_loop);
        });
        const std::string addr = this->_cfg.cql_over_websocket_addr();
        ws.listen(socket_address(ipv4_addr(addr, this->_cfg.cql_over_websocket_port())));
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
