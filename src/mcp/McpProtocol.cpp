/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "McpProtocol.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <random>
#include <thread>

using nlohmann::json;

namespace {

constexpr int ParseError = -32700;
constexpr int InvalidRequest = -32600;
constexpr int MethodNotFound = -32601;
constexpr int InvalidParams = -32602;

json ErrorResponse(const json& id, int code, const std::string& message) {
	return {{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", message}}}};
}

json ResultResponse(const json& id, json result) {
	return {{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}};
}

// Thrown while handling a request, becomes a JSON-RPC error
struct RpcError {
	int code;
	std::string message;
};

// Mesh floats hold about seven significant digits, the rest of a double is noise that only costs tokens
void TrimFloats(json& value) {
	if (value.is_number_float()) {
		double number = value.get<double>();
		if (number == 0.0 || !std::isfinite(number) || std::fabs(number) >= 1e7)
			return;

		int decimals = 7 - static_cast<int>(std::ceil(std::log10(std::fabs(number))));
		if (decimals > 22)
			return;

		// Dividing by an exact power of ten gives the double closest to the short decimal
		double scale = std::pow(10.0, decimals);
		value = std::round(number * scale) / scale;
	}
	else if (value.is_structured()) {
		for (auto& item : value)
			TrimFloats(item);
	}
}

std::string ToLower(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}

McpHttpReply JsonReply(int status, const json& body) {
	McpHttpReply reply;
	reply.status = status;
	// Names from NIFs and archives aren't always UTF-8, which dump() would throw on
	reply.body = body.dump(-1, ' ', false, json::error_handler_t::replace);
	return reply;
}

// One server-sent event carrying a JSON-RPC message
std::string Event(const json& message) {
	return "event: message\ndata: " + message.dump(-1, ' ', false, json::error_handler_t::replace) + "\n\n";
}

McpHttpReply TextReply(int status, const std::string& text) {
	McpHttpReply reply;
	reply.status = status;
	reply.body = text;
	reply.contentType = "text/plain";
	return reply;
}

} // namespace

McpToolResult McpToolResult::Error(const std::string& message) {
	McpToolResult result;
	result.isError = true;
	result.data = {{"error", message}};
	return result;
}

void McpToolRegistry::Add(McpTool tool) {
	tools.push_back(std::move(tool));
}

const McpTool* McpToolRegistry::Find(const std::string& name) const {
	for (auto& tool : tools)
		if (tool.name == name)
			return &tool;

	return nullptr;
}

const std::vector<std::string>& McpProtocol::SupportedVersions() {
	static const std::vector<std::string> versions = {"2025-06-18", "2025-03-26", "2024-11-05"};
	return versions;
}

McpProtocol::McpProtocol(const McpToolRegistry& inTools, std::string inServerName, std::string inServerVersion, std::string inInstructions)
	: tools(inTools)
	, serverName(std::move(inServerName))
	, serverVersion(std::move(inServerVersion))
	, instructions(std::move(inInstructions)) {}

bool McpProtocol::IsAllowedOrigin(const std::string& origin) {
	if (origin.empty())
		return true;

	std::string host = ToLower(origin);
	size_t scheme = host.find("://");
	if (scheme == std::string::npos)
		return false;

	const std::string protocol = host.substr(0, scheme);
	if (protocol != "http" && protocol != "https")
		return false;

	host = host.substr(scheme + 3);
	if (!host.empty() && host.back() == '/')
		host.pop_back();

	// Remove the port
	if (!host.empty() && host.front() == '[') {
		size_t end = host.find(']');
		if (end == std::string::npos)
			return false;
		host = host.substr(0, end + 1);
	}
	else if (size_t colon = host.find(':'); colon != std::string::npos)
		host = host.substr(0, colon);

	return host == "localhost" || host == "127.0.0.1" || host == "[::1]";
}

std::string McpProtocol::Base64(const std::vector<uint8_t>& data) {
	static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string out;
	out.reserve((data.size() + 2) / 3 * 4);

	size_t i = 0;
	for (; i + 2 < data.size(); i += 3) {
		uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
		out += chars[(n >> 18) & 63];
		out += chars[(n >> 12) & 63];
		out += chars[(n >> 6) & 63];
		out += chars[n & 63];
	}

	if (i + 1 == data.size()) {
		uint32_t n = data[i] << 16;
		out += chars[(n >> 18) & 63];
		out += chars[(n >> 12) & 63];
		out += "==";
	}
	else if (i + 2 == data.size()) {
		uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
		out += chars[(n >> 18) & 63];
		out += chars[(n >> 12) & 63];
		out += chars[(n >> 6) & 63];
		out += '=';
	}

	return out;
}

size_t McpProtocol::SessionCount() {
	std::lock_guard<std::mutex> lock(sessionMutex);
	return sessions.size();
}

std::string McpProtocol::NewSessionId() {
	static const char hex[] = "0123456789abcdef";
	std::random_device random;
	std::string id;
	for (int i = 0; i < 32; i++)
		id += hex[random() & 15];

	std::lock_guard<std::mutex> lock(sessionMutex);
	sessions.insert(id);
	return id;
}

bool McpProtocol::HasSession(const std::string& id) {
	std::lock_guard<std::mutex> lock(sessionMutex);
	return sessions.count(id) != 0;
}

McpHttpReply McpProtocol::Handle(const McpHttpRequest& request) {
	if (!IsAllowedOrigin(request.origin))
		return TextReply(403, "Origin not allowed.");

	if (!token.empty() && request.authorization != "Bearer " + token)
		return TextReply(401, "Missing or wrong bearer token.");

	if (request.method == "DELETE") {
		std::lock_guard<std::mutex> lock(sessionMutex);
		if (sessions.erase(request.sessionId) == 0)
			return TextReply(404, "Unknown session.");
		return TextReply(200, "");
	}

	// There is no stream for messages from the server
	if (request.method != "POST")
		return TextReply(405, "Method not allowed.");

	json message = json::parse(request.body, nullptr, false);
	if (message.is_discarded())
		return JsonReply(400, ErrorResponse(nullptr, ParseError, "Parse error"));

	if (message.is_array())
		return JsonReply(400, ErrorResponse(nullptr, InvalidRequest, "Batches are not supported."));

	if (!message.is_object())
		return JsonReply(400, ErrorResponse(nullptr, InvalidRequest, "Invalid request"));

	const bool isInitialize = message.value("method", "") == "initialize";
	if (!isInitialize) {
		if (request.sessionId.empty())
			return JsonReply(400, ErrorResponse(message.value("id", json()), InvalidRequest, "Missing Mcp-Session-Id header."));
		if (!HasSession(request.sessionId))
			return JsonReply(404, ErrorResponse(message.value("id", json()), InvalidRequest, "Unknown session, initialize again."));

		if (!request.protocolVersion.empty()) {
			auto& versions = SupportedVersions();
			if (std::find(versions.begin(), versions.end(), request.protocolVersion) == versions.end())
				return JsonReply(400, ErrorResponse(message.value("id", json()), InvalidRequest, "Unsupported MCP-Protocol-Version."));
		}
	}

	// A tool call with a progress token gets its progress as events before the result, when the client takes events
	const bool wantsProgress = message.value("method", "") == "tools/call" && message.contains("params") && message["params"].is_object()
							   && message["params"].contains("_meta") && message["params"]["_meta"].is_object()
							   && message["params"]["_meta"].contains("progressToken") && message.contains("id");
	if (wantsProgress && ToLower(request.accept).find("text/event-stream") != std::string::npos) {
		McpHttpReply reply;
		reply.contentType = "text/event-stream";
		reply.stream = [this, message](const std::function<bool(const std::string&)>& send) { StreamCall(message, send); };
		return reply;
	}

	std::string newSessionId;
	json response = HandleMessage(message, &newSessionId);
	if (response.is_null()) {
		McpHttpReply reply = TextReply(202, "");
		return reply;
	}

	McpHttpReply reply = JsonReply(200, response);
	reply.sessionId = newSessionId;
	return reply;
}

void McpProtocol::StreamCall(const json& message, const std::function<bool(const std::string&)>& send) {
	// Shared with the progress callback, which a tool that timed out may still call later
	struct State {
		std::mutex mutex;
		std::condition_variable changed;
		std::deque<json> events;
		bool done = false;
		json response;
	};
	auto state = std::make_shared<State>();
	const json progressToken = message["params"]["_meta"]["progressToken"];

	McpProgress progress = [state, progressToken](double value, double total, const std::string& text) {
		json notification = {
			{"jsonrpc", "2.0"},
			{"method", "notifications/progress"},
			{"params", {{"progressToken", progressToken}, {"progress", value}, {"total", total}}},
		};
		if (!text.empty())
			notification["params"]["message"] = text;

		std::lock_guard<std::mutex> lock(state->mutex);
		if (state->done)
			return;
		// Only the latest progress matters when the client reads slower than it comes
		if (state->events.size() > 16)
			state->events.pop_front();
		state->events.push_back(std::move(notification));
		state->changed.notify_one();
	};

	std::thread worker([this, state, message, progress]() {
		json response = HandleMessage(message, nullptr, progress);
		std::lock_guard<std::mutex> lock(state->mutex);
		state->response = std::move(response);
		state->done = true;
		state->changed.notify_one();
	});

	bool connected = true;
	for (;;) {
		std::deque<json> events;
		bool done = false;
		{
			std::unique_lock<std::mutex> lock(state->mutex);
			state->changed.wait(lock, [&]() { return state->done || !state->events.empty(); });
			events.swap(state->events);
			done = state->done;
		}

		for (auto& event : events)
			if (connected)
				connected = send(Event(event));
		if (done)
			break;
	}

	worker.join();
	if (connected)
		send(Event(state->response));
}

json McpProtocol::HandleMessage(const json& message, std::string* newSessionId, const McpProgress& progress) {
	if (!message.is_object())
		return ErrorResponse(nullptr, InvalidRequest, "Invalid request");

	// Responses to requests of the server, which never sends any
	if (!message.contains("method"))
		return nullptr;

	const bool hasId = message.contains("id");
	const json id = hasId ? message["id"] : json();

	if (message.value("jsonrpc", "") != "2.0" || !message["method"].is_string())
		return hasId ? ErrorResponse(id, InvalidRequest, "Invalid request") : json();

	const std::string method = message["method"];
	const json params = message.contains("params") ? message["params"] : json::object();

	// Notifications get no answer, whatever they are
	if (!hasId)
		return nullptr;

	try {
		if (method == "initialize")
			return ResultResponse(id, Initialize(params, newSessionId));
		if (method == "ping")
			return ResultResponse(id, json::object());
		if (method == "tools/list")
			return ResultResponse(id, ListTools());
		if (method == "tools/call")
			return ResultResponse(id, CallTool(params, progress));
	}
	catch (const RpcError& error) {
		return ErrorResponse(id, error.code, error.message);
	}

	return ErrorResponse(id, MethodNotFound, "Method not found: " + method);
}

json McpProtocol::Initialize(const json& params, std::string* newSessionId) {
	auto& versions = SupportedVersions();
	std::string version = versions.front();
	if (params.is_object() && params.contains("protocolVersion") && params["protocolVersion"].is_string()) {
		std::string requested = params["protocolVersion"];
		if (std::find(versions.begin(), versions.end(), requested) != versions.end())
			version = requested;
	}

	std::string sessionId = NewSessionId();
	if (newSessionId)
		*newSessionId = sessionId;

	json result = {
		{"protocolVersion", version},
		{"capabilities", {{"tools", {{"listChanged", false}}}}},
		{"serverInfo", {{"name", serverName}, {"version", serverVersion}}},
	};

	if (!instructions.empty())
		result["instructions"] = instructions;

	return result;
}

json McpProtocol::ListTools() {
	json list = json::array();
	for (auto& tool : tools.Tools()) {
		json entry = {{"name", tool.name}, {"description", tool.description}, {"inputSchema", tool.inputSchema}};
		if (!tool.outputSchema.is_null())
			entry["outputSchema"] = tool.outputSchema;

		// Tools only work on the window's project and the files it names
		json annotations = {{"readOnlyHint", tool.readOnly}, {"openWorldHint", false}};
		if (tool.readOnly)
			annotations["idempotentHint"] = true;
		else
			annotations["destructiveHint"] = tool.destructive;
		entry["annotations"] = annotations;
		list.push_back(entry);
	}

	return {{"tools", list}};
}

json McpProtocol::CallTool(const json& params, const McpProgress& progress) {
	if (!params.is_object() || !params.contains("name") || !params["name"].is_string())
		throw RpcError{InvalidParams, "Missing tool name."};

	const std::string name = params["name"];
	const McpTool* tool = tools.Find(name);
	if (!tool)
		throw RpcError{InvalidParams, "Unknown tool: " + name};

	json args = params.contains("arguments") ? params["arguments"] : json::object();
	if (args.is_null())
		args = json::object();
	if (!args.is_object())
		throw RpcError{InvalidParams, "Tool arguments have to be an object."};

	McpToolResult result;
	{
		std::lock_guard<std::mutex> lock(callMutex);
		try {
			result = tool->progressHandler ? tool->progressHandler(args, progress) : tool->handler(args);
		}
		catch (const McpToolError& error) {
			result = McpToolResult::Error(error.what());
		}
		catch (const json::exception& error) {
			result = McpToolResult::Error(std::string("Invalid arguments: ") + error.what());
		}
		catch (const std::exception& error) {
			result = McpToolResult::Error(error.what());
		}
	}

	// Compact, since agents read it as tokens and whitespace doesn't help them
	TrimFloats(result.data);
	json content = json::array();
	content.push_back({{"type", "text"}, {"text", result.data.dump(-1, ' ', false, json::error_handler_t::replace)}});
	for (auto& image : result.images)
		content.push_back({{"type", "image"}, {"data", Base64(image.png)}, {"mimeType", "image/png"}});

	json response = {{"content", content}, {"isError", result.isError}};
	if (result.data.is_object())
		response["structuredContent"] = result.data;

	return response;
}
