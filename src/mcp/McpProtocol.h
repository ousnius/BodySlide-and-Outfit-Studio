/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Model Context Protocol (MCP) over JSON-RPC 2.0, as served on the Streamable HTTP transport.
// Knows nothing about the HTTP library or the program, so it can be tested on its own.

// An image a tool returns, encoded as PNG
struct McpImage {
	std::vector<uint8_t> png;
};

// Result of a tool call. data goes back as JSON text and as structured content.
struct McpToolResult {
	nlohmann::json data = nlohmann::json::object();
	std::vector<McpImage> images;
	bool isError = false;

	static McpToolResult Error(const std::string& message);
};

// Thrown by tool handlers for bad arguments or failed operations, becomes an error result
class McpToolError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

// Reports how far a tool call is, while it runs. Can be called from any thread, also after the call ended.
using McpProgress = std::function<void(double progress, double total, const std::string& message)>;

struct McpTool {
	std::string name;
	std::string description;
	// JSON schema of the arguments, always an object schema
	nlohmann::json inputSchema = {{"type", "object"}, {"properties", nlohmann::json::object()}};
	// JSON schema of the structured result, if the tool declares one
	nlohmann::json outputSchema;
	// Tells clients that the tool doesn't change the project
	bool readOnly = false;
	// Tells clients that the tool changes the project in a way the window can't undo, or overwrites files
	bool destructive = false;
	std::function<McpToolResult(const nlohmann::json& args)> handler;
	// Used instead of handler when set, for tools that report progress
	std::function<McpToolResult(const nlohmann::json& args, const McpProgress& progress)> progressHandler;
};

class McpToolRegistry {
public:
	void Add(McpTool tool);
	const McpTool* Find(const std::string& name) const;
	const std::vector<McpTool>& Tools() const { return tools; }

private:
	std::vector<McpTool> tools;
};

// An HTTP request to the MCP endpoint, with the headers that matter to MCP
struct McpHttpRequest {
	std::string method; // GET, POST or DELETE
	std::string body;
	std::string sessionId;		 // Mcp-Session-Id
	std::string protocolVersion; // MCP-Protocol-Version
	std::string origin;			 // Origin
	std::string authorization;	 // Authorization
	std::string accept;			 // Accept
};

struct McpHttpReply {
	int status = 200;
	std::string body;
	std::string contentType = "application/json";
	// Mcp-Session-Id to send back, set when a session starts
	std::string sessionId;
	// Set for a reply sent as a stream of server-sent events instead of body: called once to write the
	// events with send, which returns false when the client is gone
	std::function<void(const std::function<bool(const std::string& data)>& send)> stream;
};

class McpProtocol {
public:
	// Newest protocol version first
	static const std::vector<std::string>& SupportedVersions();

	McpProtocol(const McpToolRegistry& inTools, std::string inServerName, std::string inServerVersion, std::string inInstructions = "");

	// Without a token, requests need no Authorization header
	void SetToken(const std::string& inToken) { token = inToken; }

	McpHttpReply Handle(const McpHttpRequest& request);

	// Handles one JSON-RPC message. Returns null for notifications and responses, which get no answer.
	// newSessionId receives the ID of a session started by an initialize request. A tool call reports
	// its progress to progress, when given.
	nlohmann::json HandleMessage(const nlohmann::json& message, std::string* newSessionId = nullptr, const McpProgress& progress = nullptr);

	// Only pages served from this machine may call the server, against DNS rebinding.
	// Requests without an Origin don't come from a browser and are allowed.
	static bool IsAllowedOrigin(const std::string& origin);

	static std::string Base64(const std::vector<uint8_t>& data);

	size_t SessionCount();

private:
	const McpToolRegistry& tools;
	std::string serverName;
	std::string serverVersion;
	std::string instructions;
	std::string token;

	std::mutex sessionMutex;
	std::set<std::string> sessions;

	// Tool calls run one at a time
	std::mutex callMutex;

	nlohmann::json Initialize(const nlohmann::json& params, std::string* newSessionId);
	nlohmann::json ListTools();
	nlohmann::json CallTool(const nlohmann::json& params, const McpProgress& progress);

	// Runs a tool call that asked for progress, writing the progress notifications and then the response as events
	void StreamCall(const nlohmann::json& message, const std::function<bool(const std::string& data)>& send);

	std::string NewSessionId();
	bool HasSession(const std::string& id);
};
