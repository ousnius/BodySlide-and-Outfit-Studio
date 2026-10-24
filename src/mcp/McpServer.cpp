/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "McpServer.h"
#include "McpProtocol.h"

#include <httplib.h>

#include <chrono>

namespace {

constexpr const char* Host = "127.0.0.1";

void HandleRequest(McpProtocol& protocol, const std::string& method, const httplib::Request& req, httplib::Response& res) {
	McpHttpRequest request;
	request.method = method;
	request.body = req.body;
	request.sessionId = req.get_header_value("Mcp-Session-Id");
	request.protocolVersion = req.get_header_value("MCP-Protocol-Version");
	request.origin = req.get_header_value("Origin");
	request.authorization = req.get_header_value("Authorization");
	request.accept = req.get_header_value("Accept");

	McpHttpReply reply = protocol.Handle(request);
	res.status = reply.status;
	if (!reply.sessionId.empty())
		res.set_header("Mcp-Session-Id", reply.sessionId);
	if (reply.status == 405)
		res.set_header("Allow", "POST, DELETE");

	if (reply.stream) {
		auto stream = reply.stream;
		res.set_chunked_content_provider(reply.contentType, [stream](size_t, httplib::DataSink& sink) {
			stream([&sink](const std::string& data) { return sink.write(data.data(), data.size()); });
			sink.done();
			return true;
		});
	}
	else if (!reply.body.empty())
		res.set_content(reply.body, reply.contentType);
}

// The defaults of cpp-httplib let a second program listen on the same port (SO_REUSEADDR on Windows,
// SO_REUSEPORT elsewhere), so a second Outfit Studio would silently share the port with the first.
// Taking the port exclusively makes the second one fail to start its server instead.
void ExclusiveSocketOptions(socket_t sock) {
	int opt = 1;
#ifdef _WIN32
	setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
	// Only lets the port be taken again while old connections are closing
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void*>(&opt), sizeof(opt));
#endif
}

} // namespace

McpServer::McpServer(McpProtocol& inProtocol)
	: protocol(inProtocol) {}

McpServer::~McpServer() {
	Stop();
}

bool McpServer::Start(int inPort, std::string& outError) {
	if (running)
		return true;

	server = std::make_unique<httplib::Server>();
	server->set_socket_options(ExclusiveSocketOptions);

	// Tool calls like conforming can take minutes
	server->set_read_timeout(30, 0);
	server->set_write_timeout(30, 0);

	server->Post("/mcp", [this](const httplib::Request& req, httplib::Response& res) { HandleRequest(protocol, "POST", req, res); });
	server->Get("/mcp", [this](const httplib::Request& req, httplib::Response& res) { HandleRequest(protocol, "GET", req, res); });
	server->Delete("/mcp", [this](const httplib::Request& req, httplib::Response& res) { HandleRequest(protocol, "DELETE", req, res); });

	if (inPort == 0)
		port = server->bind_to_any_port(Host);
	else
		port = server->bind_to_port(Host, inPort) ? inPort : -1;

	if (port <= 0) {
		outError = "Couldn't listen on " + std::string(Host) + ":" + std::to_string(inPort)
				   + ". Another program, maybe another Outfit Studio, is using the port. Choose another port in the "
					 "settings or with --mcp-port.";
		server.reset();
		port = 0;
		return false;
	}

	running = true;
	finished = false;
	thread = std::thread([this]() {
		server->listen_after_bind();
		finished = true;
	});

	// stop() does nothing before the server runs, so a Stop right after this would wait forever
	server->wait_until_ready();
	return true;
}

void McpServer::Stop(const std::function<void()>& pump) {
	if (!server)
		return;

	server->stop();

	while (!finished) {
		if (pump)
			pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	if (thread.joinable())
		thread.join();

	server.reset();
	running = false;
	port = 0;
}

std::string McpServer::GetUrl() const {
	return "http://" + std::string(Host) + ":" + std::to_string(port) + "/mcp";
}
