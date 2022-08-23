#include "controller.h"

#include <algorithm>
#include <thread>
#include <variant>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/format.hpp>

#include <nlohmann/json.hpp>

#include "../application_context.h"
#include "../clock.h"
#include "../input_manager.h"
#include "../iospec.h"
#include "../log.h"
#include "../metadata_queue.h"
#include "../program_options.h"

#include <src/config.h>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
using json = nlohmann::json;

namespace metamix::proc {

namespace {

constexpr const char *SERVER_STRING = "METAMIX/" METAMIX_VERSION_STRING;

http::response<http::string_body>
json_response(const http::request<http::string_body> &req, http::status result, const json &body)
{
  auto body_string = body.dump();

  http::response<http::string_body> res(result, req.version());
  res.set(http::field::server, SERVER_STRING);
  res.set(http::field::content_type, "text/json");
  res.content_length(body.size());
  res.body() = body_string;
  res.prepare_payload();
  return res;
}

std::variant<InputId, std::string>
parse_input_ref(const json &args)
{
  if (args.find("name") != args.end()) {
    const std::string &name = args["name"];
    return name;
  } else if (args.find("id") != args.end()) {
    InputId id = args["id"];
    return id;
  } else {
    throw std::invalid_argument(R"(Expected "name" or "id" argument)");
  }
}

AbstractInput &
get_input_by_ref(const ApplicationContext &ctx, const std::variant<InputId, std::string> &ref)
{
  std::optional<std::reference_wrapper<AbstractInput>> input_opt;

  if (std::holds_alternative<std::string>(ref)) {
    input_opt = ctx.input_manager->get_input_by_name(std::get<std::string>(ref));
  } else {
    input_opt = ctx.input_manager->get_input_by_id(std::get<InputId>(ref));
  }

  if (!input_opt) {
    throw std::invalid_argument("Unknown input name");
  }

  return *input_opt;
}

AbstractInput &
get_input_by_json(const ApplicationContext &ctx, const json &args)
{
  return get_input_by_ref(ctx, parse_input_ref(args));
}

json
get_stats(const ApplicationContext &ctx)
{
  json queue_size_json;
  InputCapabilities::Kinds::for_each([&](auto k) {
    using K = decltype(k);
    queue_size_json[K::API_NAME] = ctx.meta_queue->get<K>().size();
  });

  return json{
    { "queueSize", queue_size_json },
    { "clockNow", ctx.clock->now().val },
  };
}

json
get_input_info(const ApplicationContext &ctx, InputId id)
{
  auto input_opt = ctx.input_manager->get_input_by_id(id);
  if (!input_opt) {
    throw std::invalid_argument("Unknown input id");
  }

  const auto &input = input_opt->get();
  const auto &spec = input.spec();
  const InputCapabilities &caps = input.caps();

  json caps_json;
  InputCapabilities::Kinds::for_each([&](auto k) {
    using K = decltype(k);
    caps_json[K::API_NAME] = caps.has<K>();
  });

  return json{ { "id", id },
               { "name", spec.name },
               { "source", spec.source },
               { "sink", spec.sink },
               { "sourceFormat", spec.source_format.value_or("") },
               { "sinkFormat", spec.sink_format.value_or("") },
               { "isVirtual", spec.is_virtual },
               { "caps", caps_json } };
}

json
get_current_input(const ApplicationContext &ctx)
{
  json result;
  InputCapabilities::Kinds::for_each([&](auto k) {
    using K = decltype(k);
    auto id = ctx.input_manager->get_current_input_id<K>();
    result[K::API_NAME] = get_input_info(ctx, id);
  });
  return result;
}

json
get_all_inputs(const ApplicationContext &ctx)
{
  json j;
  for (const auto &input : *ctx.input_manager) {
    j.push_back(get_input_info(ctx, input.spec().id));
  }
  return j;
}

json
set_current_input(const ApplicationContext &ctx, json args)
{
  InputManager::InputsChangeset<InputId> changeset;

  if (args.find("id") != args.end() || args.find("name") != args.end()) {
    const auto &input = get_input_by_json(ctx, args);
    InputCapabilities::Kinds::for_each([&](auto k) {
      using K = decltype(k);
      K::get(changeset) = input.spec().id;
    });
  } else {
    InputCapabilities::Kinds::for_each([&](auto k) {
      using K = decltype(k);

      if (args.find(K::API_NAME) != args.end()) {
        const auto &input = get_input_by_json(ctx, args[K::API_NAME]);
        K::get(changeset) = input.spec().id;
      }
    });
  }

  ctx.input_manager->set_current_inputs(changeset);

  return json{ { "ok", true } };
}

json
restart_input(const ApplicationContext &ctx, json args)
{
  auto &input = get_input_by_json(ctx, args);
  input.schedule_restart();
  return json{ { "ok", true } };
}

json
get_config(const ApplicationContext &ctx)
{
  return json{ { "tsAdjustment", ctx.ts_adjustment().val } };
}

json
set_config(ApplicationContext &ctx, json args)
{
  std::optional<int64_t> set_ts_adjustment;
  if (args.find("tsAdjustment") != args.end()) {
    set_ts_adjustment = args["tsAdjustment"];
  }

  if (set_ts_adjustment) {
    ctx.ts_adjustment(ClockTS(*set_ts_adjustment));
  }

  return json{ { "ok", true } };
}

void
handle_request(ApplicationContext &ctx,
               http::request<http::string_body> req,
               const std::function<void(http::response<http::string_body>)> &send)
{
  const auto ok = [&req](json body) { return json_response(req, http::status::ok, body); };

  const auto bad_request = [&req](const auto &why) {
    return json_response(req, http::status::bad_request, { { "error", why } });
  };

  const auto not_found = [&req]() {
    return json_response(req, http::status::not_found, { { "error", "Resource does not exist." } });
  };

  const auto server_error = [&req](const auto &why) {
    const auto f = boost::format("Internal server error: %1%") % why;
    return json_response(req, http::status::internal_server_error, { { "error", f.str() } });
  };

  try {
    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != boost::beast::string_view::npos) {
      send(bad_request("Illegal request-target."));
      return;
    }

    LOG(info) << http::to_string(req.method()) << " " << req.target();

    if (req.method() == http::verb::get && req.target() == "/stats") {
      send(ok(get_stats(ctx)));
    } else if (req.method() == http::verb::get && req.target() == "/input") {
      send(ok(get_all_inputs(ctx)));
    } else if (req.method() == http::verb::get && req.target() == "/input/current") {
      send(ok(get_current_input(ctx)));
    } else if (req.method() == http::verb::post && req.target() == "/input/current") {
      send(ok(set_current_input(ctx, json::parse(req.body()))));
    } else if (req.method() == http::verb::post && req.target() == "/input/restart") {
      send(ok(restart_input(ctx, json::parse(req.body()))));
    } else if (req.method() == http::verb::get && req.target() == "/config") {
      send(ok(get_config(ctx)));
    } else if (req.method() == http::verb::post && req.target() == "/config") {
      send(ok(set_config(ctx, json::parse(req.body()))));
    } else {
      send(not_found());
    }
  } catch (const json::parse_error &error) {
    send(bad_request(error.what()));
  } catch (const json::type_error &error) {
    send(bad_request(error.what()));
  } catch (const std::invalid_argument &error) {
    send(bad_request(error.what()));
  } catch (const std::logic_error &error) {
    LOG(error) << error.what();
    send(server_error(error.what()));
  } catch (const std::runtime_error &error) {
    LOG(error) << error.what();
    send(server_error(error.what()));
  }
}

void
process_session(tcp::socket socket, std::shared_ptr<ApplicationContext> ctx)
{
  bool close = false;
  boost::system::error_code ec;

  boost::beast::flat_buffer buffer;

  const auto sender = [&socket, &close, &ec](http::response<http::string_body> &&response) {
    close = response.need_eof();
    http::write(socket, response, ec);
  };

  while (!close) {
    http::request<http::string_body> request;
    http::read(socket, buffer, request, ec);

    if (ec == http::error::end_of_stream) {
      break;
    }

    if (ec) {
      LOG(error) << "Failed reading request: " << ec;
      return;
    }

    handle_request(*ctx, std::move(request), sender);

    if (ec) {
      LOG(error) << "Failed writing response: " << ec;
      return;
    }
  }

  socket.shutdown(tcp::socket::shutdown_send, ec);
  if (ec) {
    LOG(warning) << "Failed shutting down socket: " << ec;
    return;
  }
}
}

void
controller(std::shared_ptr<ApplicationContext> ctx)
{
  metamix::log::set_thread_name("controller");

  const auto listen_address = boost::asio::ip::make_address(ctx->options->http_address);
  const auto listen_port = ctx->options->http_port;

  boost::asio::io_context ioc{ 1 };
  boost::system::error_code ec;

  tcp::acceptor acceptor{ ioc, { listen_address, listen_port } };

  boost::signals2::scoped_connection on_exit_conn(ctx->on_exit.connect([&]() {
    LOG(debug) << "Stopping REST server...";

    acceptor.cancel(ec);
    if (ec) {
      LOG(error) << "Failed stopping REST server: " << ec;
      return;
    }
  }));

  acceptor.listen(boost::asio::socket_base::max_listen_connections);

  tcp::socket socket{ ioc };

  std::function<void()> do_accept;
  do_accept = [&]() {
    acceptor.async_accept(socket, [&](boost::system::error_code ec) {
      if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
          LOG(error) << "Failed accepting connection: " << ec;
        } else {
          LOG(debug) << "Accepting cancelled";
        }
      } else {
        LOG(debug) << "Accepting new connection";
        process_session(std::move(socket), ctx);
      }

      if (ec != boost::asio::error::operation_aborted) {
        do_accept();
      }
    });
  };

  do_accept();

  ioc.run();

  LOG(debug) << "REST server stopped.";
}
}
