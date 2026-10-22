/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

class McpProtocol;

namespace httplib {
class Server;
}

// Serves an McpProtocol on http://127.0.0.1:<port>/mcp, on threads of its own
class McpServer {
public:
	explicit McpServer(McpProtocol& inProtocol);
	~McpServer();

	McpServer(const McpServer&) = delete;
	McpServer& operator=(const McpServer&) = delete;

	// Only listens on the loopback address. Port 0 picks a free port.
	bool Start(int port, std::string& outError);

	// Stops listening and waits for the requests being handled. Requests can wait for the
	// thread that stops the server, so pump is called while waiting, to let them finish.
	void Stop(const std::function<void()>& pump = {});

	bool IsRunning() const { return running; }
	int GetPort() const { return port; }
	std::string GetUrl() const;

private:
	McpProtocol& protocol;
	std::unique_ptr<httplib::Server> server;
	std::thread thread;
	std::atomic<bool> running{false};
	std::atomic<bool> finished{true};
	int port = 0;
};
