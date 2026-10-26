#include "../src/mcp/McpProtocol.h"
#include "../src/mcp/McpServer.h"

#include <catch2/catch_test_macros.hpp>

#include <httplib.h>

using nlohmann::json;

namespace {

McpToolRegistry MakeTools() {
	McpToolRegistry tools;

	McpTool echo;
	echo.name = "echo";
	echo.description = "Returns its text";
	echo.inputSchema = {{"type", "object"}, {"properties", {{"text", {{"type", "string"}}}}}, {"required", {"text"}}};
	echo.readOnly = true;
	echo.handler = [](const json& args) {
		McpToolResult result;
		result.data["text"] = args.at("text").get<std::string>();
		return result;
	};
	tools.Add(echo);

	McpTool fail;
	fail.name = "fail";
	fail.description = "Always fails";
	fail.handler = [](const json&) -> McpToolResult { throw McpToolError("Nothing to do."); };
	tools.Add(fail);

	McpTool image;
	image.name = "image";
	image.description = "Returns an image";
	image.handler = [](const json&) {
		McpToolResult result;
		result.images.push_back({{'P', 'N', 'G'}});
		return result;
	};
	tools.Add(image);

	McpTool slow;
	slow.name = "slow";
	slow.description = "Reports progress";
	slow.destructive = true;
	slow.progressHandler = [](const json&, const McpProgress& progress) {
		if (progress) {
			progress(25.0, 100.0, "Starting");
			progress(75.0, 100.0, "");
		}
		McpToolResult result;
		result.data["done"] = true;
		return result;
	};
	tools.Add(slow);

	return tools;
}

json Request(int id, const std::string& method, json params = json::object()) {
	return {{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}};
}

McpHttpRequest Post(const json& body, const std::string& sessionId = "") {
	McpHttpRequest request;
	request.method = "POST";
	request.body = body.dump();
	request.sessionId = sessionId;
	return request;
}

// Starts a session and returns its ID
std::string Initialize(McpProtocol& protocol) {
	McpHttpReply reply = protocol.Handle(Post(Request(1, "initialize", {{"protocolVersion", "2025-06-18"}})));
	REQUIRE(reply.status == 200);
	REQUIRE_FALSE(reply.sessionId.empty());
	return reply.sessionId;
}

} // namespace

TEST_CASE("MCP initialize negotiates the protocol version and starts a session", "[MCP]") {
	McpToolRegistry tools = MakeTools();
	McpProtocol protocol(tools, "test-server", "1.0.0", "Some instructions");

	std::string sessionId;
	json response = protocol.HandleMessage(Request(1, "initialize", {{"protocolVersion", "2025-03-26"}}), &sessionId);
	CHECK(response["id"] == 1);
	CHECK(response["result"]["protocolVersion"] == "2025-03-26");
	CHECK(response["result"]["serverInfo"]["name"] == "test-server");
	CHECK(response["result"]["capabilities"].contains("tools"));
	CHECK(response["result"]["instructions"] == "Some instructions");
	CHECK(sessionId.size() == 32);

	// An unknown version gets the newest one
	response = protocol.HandleMessage(Request(2, "initialize", {{"protocolVersion", "1999-01-01"}}));
	CHECK(response["result"]["protocolVersion"] == McpProtocol::SupportedVersions().front());
	CHECK(protocol.SessionCount() == 2);
}

TEST_CASE("MCP requests need a known session", "[MCP]") {
	McpToolRegistry tools = MakeTools();
	McpProtocol protocol(tools, "test-server", "1.0.0");

	CHECK(protocol.Handle(Post(Request(2, "tools/list"))).status == 400);
	CHECK(protocol.Handle(Post(Request(2, "tools/list"), "unknown")).status == 404);

	std::string sessionId = Initialize(protocol);
	CHECK(protocol.Handle(Post(Request(2, "tools/list"), sessionId)).status == 200);

	// Notifications are accepted without an answer
	McpHttpReply reply = protocol.Handle(Post({{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}}, sessionId));
	CHECK(reply.status == 202);
	CHECK(reply.body.empty());

	McpHttpRequest wrongVersion = Post(Request(3, "ping"), sessionId);
	wrongVersion.protocolVersion = "1999-01-01";
	CHECK(protocol.Handle(wrongVersion).status == 400);

	McpHttpRequest end;
	end.method = "DELETE";
	end.sessionId = sessionId;
	CHECK(protocol.Handle(end).status == 200);
	CHECK(protocol.Handle(Post(Request(4, "ping"), sessionId)).status == 404);
}

TEST_CASE("MCP rejects bad requests", "[MCP]") {
	McpToolRegistry tools = MakeTools();
	McpProtocol protocol(tools, "test-server", "1.0.0");

	McpHttpRequest request;
	request.method = "POST";
	request.body = "{not json";
	McpHttpReply reply = protocol.Handle(request);
	CHECK(reply.status == 400);
	CHECK(json::parse(reply.body)["error"]["code"] == -32700);

	request.body = json::array({Request(1, "ping")}).dump();
	CHECK(protocol.Handle(request).status == 400);

	request.method = "GET";
	CHECK(protocol.Handle(request).status == 405);

	json response = protocol.HandleMessage(Request(5, "resources/list"));
	CHECK(response["error"]["code"] == -32601);
}

TEST_CASE("MCP lists and calls tools", "[MCP]") {
	McpToolRegistry tools = MakeTools();
	McpProtocol protocol(tools, "test-server", "1.0.0");

	json list = protocol.HandleMessage(Request(1, "tools/list"))["result"]["tools"];
	REQUIRE(list.size() == 4);
	CHECK(list[0]["name"] == "echo");
	CHECK(list[0]["inputSchema"]["required"][0] == "text");
	CHECK(list[0]["annotations"]["readOnlyHint"] == true);
	CHECK(list[0]["annotations"]["openWorldHint"] == false);
	CHECK_FALSE(list[0]["annotations"].contains("destructiveHint"));
	CHECK(list[1]["annotations"]["readOnlyHint"] == false);
	CHECK(list[1]["annotations"]["destructiveHint"] == false);
	CHECK(list[3]["annotations"]["destructiveHint"] == true);
	CHECK_FALSE(list[0].contains("outputSchema"));

	json result = protocol.HandleMessage(Request(2, "tools/call", {{"name", "echo"}, {"arguments", {{"text", "hello"}}}}))["result"];
	CHECK(result["isError"] == false);
	CHECK(result["structuredContent"]["text"] == "hello");
	CHECK(result["content"][0]["type"] == "text");
	CHECK(json::parse(result["content"][0]["text"].get<std::string>())["text"] == "hello");

	// Wrong arguments and failing tools are tool errors, so the agent sees them
	result = protocol.HandleMessage(Request(3, "tools/call", {{"name", "echo"}, {"arguments", json::object()}}))["result"];
	CHECK(result["isError"] == true);

	result = protocol.HandleMessage(Request(4, "tools/call", {{"name", "fail"}}))["result"];
	CHECK(result["isError"] == true);
	CHECK(result["structuredContent"]["error"] == "Nothing to do.");

	result = protocol.HandleMessage(Request(5, "tools/call", {{"name", "image"}}))["result"];
	REQUIRE(result["content"].size() == 2);
	CHECK(result["content"][1]["type"] == "image");
	CHECK(result["content"][1]["mimeType"] == "image/png");
	CHECK(result["content"][1]["data"] == "UE5H");

	// Unknown tools are protocol errors
	json response = protocol.HandleMessage(Request(6, "tools/call", {{"name", "nope"}}));
	CHECK(response["error"]["code"] == -32602);
}

TEST_CASE("MCP answers with text that isn't UTF-8", "[MCP]") {
	McpToolRegistry tools;
	McpTool latin1;
	latin1.name = "latin1";
	latin1.description = "Returns a Windows-1252 name, like some NIFs and archives have";
	latin1.handler = [](const json&) {
		McpToolResult result;
		result.data["name"] = std::string("R\xF6" "be");
		return result;
	};
	tools.Add(latin1);

	McpProtocol protocol(tools, "test-server", "1.0.0");
	std::string session = Initialize(protocol);

	McpHttpReply reply = protocol.Handle(Post(Request(2, "tools/call", {{"name", "latin1"}}), session));
	REQUIRE(reply.status == 200);
	json response = json::parse(reply.body);
	CHECK(response["result"]["isError"] == false);
	CHECK(json::parse(response["result"]["content"][0]["text"].get<std::string>())["name"] == "R\xEF\xBF\xBD" "be");
}

TEST_CASE("MCP answers with compact text and float precision", "[MCP]") {
	McpToolRegistry tools;
	McpTool vertex;
	vertex.name = "vertex";
	vertex.description = "Returns float positions, which carry noise digits as doubles";
	vertex.handler = [](const json&) {
		McpToolResult result;
		result.data["index"] = 0;
		result.data["position"] = json::array({8.208552360534668f, -0.001f, 0.0f});
		result.data["weight"] = 0.00001234567891;
		return result;
	};
	tools.Add(vertex);

	McpProtocol protocol(tools, "test-server", "1.0.0");
	json result = protocol.HandleMessage(Request(1, "tools/call", {{"name", "vertex"}}))["result"];
	CHECK(result["content"][0]["text"] == R"({"index":0,"position":[8.208552,-0.001,0.0],"weight":1.234568e-05})");
	CHECK(result["structuredContent"]["position"][0] == 8.208552);
}

TEST_CASE("MCP only takes requests from this machine", "[MCP]") {
	CHECK(McpProtocol::IsAllowedOrigin(""));
	CHECK(McpProtocol::IsAllowedOrigin("http://localhost"));
	CHECK(McpProtocol::IsAllowedOrigin("http://localhost:3000"));
	CHECK(McpProtocol::IsAllowedOrigin("https://127.0.0.1:8443/"));
	CHECK(McpProtocol::IsAllowedOrigin("http://[::1]:80"));
	CHECK_FALSE(McpProtocol::IsAllowedOrigin("http://example.com"));
	CHECK_FALSE(McpProtocol::IsAllowedOrigin("http://localhost.example.com"));
	CHECK_FALSE(McpProtocol::IsAllowedOrigin("null"));
	CHECK_FALSE(McpProtocol::IsAllowedOrigin("file://localhost"));

	McpToolRegistry tools = MakeTools();
	McpProtocol protocol(tools, "test-server", "1.0.0");
	McpHttpRequest request = Post(Request(1, "initialize"));
	request.origin = "http://evil.example";
	CHECK(protocol.Handle(request).status == 403);

	protocol.SetToken("secret");
	request.origin.clear();
	CHECK(protocol.Handle(request).status == 401);
	request.authorization = "Bearer secret";
	CHECK(protocol.Handle(request).status == 200);
}

TEST_CASE("MCP base64 encodes images", "[MCP]") {
	CHECK(McpProtocol::Base64({}) == "");
	CHECK(McpProtocol::Base64({'f'}) == "Zg==");
	CHECK(McpProtocol::Base64({'f', 'o'}) == "Zm8=");
	CHECK(McpProtocol::Base64({'f', 'o', 'o'}) == "Zm9v");
	CHECK(McpProtocol::Base64({'f', 'o', 'o', 'b', 'a', 'r'}) == "Zm9vYmFy");
	CHECK(McpProtocol::Base64({0xff, 0x00, 0x80}) == "/wCA");
}

TEST_CASE("MCP server answers over HTTP on the loopback address", "[MCP]") {
	McpToolRegistry tools = MakeTools();
	McpProtocol protocol(tools, "test-server", "1.0.0");
	McpServer server(protocol);

	std::string error;
	REQUIRE(server.Start(0, error));
	REQUIRE(server.GetPort() > 0);

	httplib::Client client("127.0.0.1", server.GetPort());
	auto res = client.Post("/mcp", Request(1, "initialize").dump(), "application/json");
	REQUIRE(res);
	CHECK(res->status == 200);
	std::string sessionId = res->get_header_value("Mcp-Session-Id");
	REQUIRE_FALSE(sessionId.empty());

	httplib::Headers headers = {{"Mcp-Session-Id", sessionId}};
	res = client.Post("/mcp", headers, Request(2, "tools/call", {{"name", "echo"}, {"arguments", {{"text", "over http"}}}}).dump(), "application/json");
	REQUIRE(res);
	CHECK(res->status == 200);
	CHECK(json::parse(res->body)["result"]["structuredContent"]["text"] == "over http");

	res = client.Get("/mcp", headers);
	REQUIRE(res);
	CHECK(res->status == 405);

	server.Stop();
	CHECK_FALSE(server.IsRunning());
}

TEST_CASE("MCP sends progress as events before the result when asked", "[MCP]") {
	McpToolRegistry tools = MakeTools();
	McpProtocol protocol(tools, "test-server", "1.0.0");
	std::string sessionId = Initialize(protocol);

	json call = Request(7, "tools/call", {{"name", "slow"}, {"_meta", {{"progressToken", "p1"}}}});

	// Without event streams accepted, the result comes as plain JSON
	McpHttpReply reply = protocol.Handle(Post(call, sessionId));
	CHECK_FALSE(reply.stream);
	CHECK(json::parse(reply.body)["result"]["structuredContent"]["done"] == true);

	McpHttpRequest request = Post(call, sessionId);
	request.accept = "application/json, text/event-stream";
	reply = protocol.Handle(request);
	REQUIRE(reply.stream);
	CHECK(reply.contentType == "text/event-stream");

	std::vector<json> events;
	reply.stream([&](const std::string& data) {
		REQUIRE(data.rfind("event: message\ndata: ", 0) == 0);
		REQUIRE(data.size() > 2);
		CHECK(data.substr(data.size() - 2) == "\n\n");
		events.push_back(json::parse(data.substr(21)));
		return true;
	});

	REQUIRE(events.size() == 3);
	CHECK(events[0]["method"] == "notifications/progress");
	CHECK(events[0]["params"]["progressToken"] == "p1");
	CHECK(events[0]["params"]["progress"] == 25.0);
	CHECK(events[0]["params"]["message"] == "Starting");
	CHECK_FALSE(events[1]["params"].contains("message"));
	CHECK(events[2]["id"] == 7);
	CHECK(events[2]["result"]["structuredContent"]["done"] == true);

	// A tool without progress still answers in the stream
	request = Post(Request(8, "tools/call", {{"name", "echo"}, {"arguments", {{"text", "hi"}}}, {"_meta", {{"progressToken", 3}}}}), sessionId);
	request.accept = "text/event-stream";
	reply = protocol.Handle(request);
	REQUIRE(reply.stream);
	events.clear();
	reply.stream([&](const std::string& data) {
		events.push_back(json::parse(data.substr(21)));
		return true;
	});
	REQUIRE(events.size() == 1);
	CHECK(events[0]["result"]["structuredContent"]["text"] == "hi");
}
