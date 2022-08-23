#include "program_options.h"

#include <iomanip>
#include <sstream>
#include <unordered_set>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/range/combine.hpp>

#include "clear_input.h"
#include "util.h"
#include "version.h"

namespace po = boost::program_options;

namespace metamix {

namespace {

InputSpec &
get_input_spec_or_create(ProgramOptions &o, const std::string &input_name)
{
  for (auto &is : o.user_inputs) {
    if (is.name == input_name) {
      return is;
    }
  }

  InputSpec is;
  is.id = o.user_inputs.size();
  is.name = input_name;
  o.user_inputs.push_back(std::move(is));
  return o.user_inputs.back();
}

std::string
ts_adjustment_description()
{
  std::stringstream ss;
  ss << "constant time offset of injected metadata, expressed in ticks with time base of "
     << std::to_string(SYS_CLOCK_RATE / 1000) << "kHz"
     << ", may be negative";
  return ss.str();
}
}

ProgramOptions *
ProgramOptions::parse(int argc, char **argv)
{
  auto o = new ProgramOptions;

  std::map<std::string, boost::log::trivial::severity_level> log_level_map{
    { "trace", boost::log::trivial::severity_level::trace },
    { "debug", boost::log::trivial::severity_level::debug },
    { "info", boost::log::trivial::severity_level::info },
    { "warning", boost::log::trivial::severity_level::warning },
    { "error", boost::log::trivial::severity_level::error },
    { "fatal", boost::log::trivial::severity_level::fatal },
  };

  boost::optional<std::string> config_file;

  po::options_description generic("Generic options (cannot be set via configuration file)");
  // clang-format off
  generic.add_options()
    ("config-file,c", po::value(&config_file)->value_name("path"), "load configuration file")
    ("help,h", "show help message and exit")
    ("version,v", "show version information and exit");
  // clang-format on

  boost::optional<std::string> start_input_name;
  std::string log_level_str;
  boost::optional<std::string> log_thread_name;

  po::options_description behavior("System options");
  // clang-format off
  behavior.add_options()
    ("http-address", po::value(&o->http_address)->value_name("ip")->default_value("0.0.0.0"), "REST API listen address")
    ("http-port", po::value(&o->http_port)->value_name("port")->default_value(3445), "REST API listen port")
    ("starting-input", po::value(&start_input_name)->value_name("name"),
     "name of the first input which metadata to pass")
    ("log", po::value(&log_level_str)->value_name("level")->default_value("info"),
     "logging severity level, must be one of: trace, debug, info, warning, error, fatal")
    ("log-thread", po::value(&log_thread_name)->value_name("name"), "show logs only from specified thread")
    ("no-restart", "don't restart streams");
  // clang-format on

  po::options_description inputs("Specifying inputs (at least one required, replace * with input name)");
  // clang-format off
  inputs.add_options()
    ("input.*.source", po::value<std::string>()->value_name("url"), "input source url")
    ("input.*.sink", po::value<std::string>()->value_name("url"), "input sink url")
    ("input.*.sourceformat", po::value<std::string>()->value_name("format"),
     "input source format, or auto detect")
    ("input.*.sinkformat", po::value<std::string>()->value_name("format"),
     "input sink format, or auto detect");
  // clang-format on

  boost::optional<std::string> output_source_format, output_sink_format;

  po::options_description outputs("Specifying output (required)");
  // clang-format off
  outputs.add_options()
    ("output.source", po::value(&o->output.source)->value_name("url"), "output source url")
    ("output.sink", po::value(&o->output.sink)->value_name("url"), "output sink url")
    ("output.sourceformat", po::value(&output_source_format)->value_name("format"),
     "output source format, or auto detect")
    ("output.sinkformat", po::value(&output_sink_format)->value_name("format"), "output sink format, or auto detect")
    ("output.ts_adjustment", po::value(&o->output.ts_adjustment)->value_name("ticks")->default_value(0),
     ts_adjustment_description().c_str());
  // clang-format on

  po::options_description cmdline_opts;
  cmdline_opts.add(generic).add(behavior).add(inputs).add(outputs);

  po::options_description config_opts;
  config_opts.add(behavior).add(inputs).add(outputs);

  std::vector<po::option> unrecognized;

  auto cmdline_parsed = po::command_line_parser(argc, argv).options(cmdline_opts).allow_unregistered().run();

  po::variables_map vm;
  po::store(cmdline_parsed, vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << "usage: " << (argc > 0 ? argv[0] : "metamix") << " [options]" << std::endl
              << std::endl
              << generic << std::endl
              << behavior << std::endl
              << inputs << std::endl
              << outputs << std::endl;

    std::exit(0);
  }

  if (vm.count("version")) {
    print_version();
    std::exit(0);
  }

  std::copy_if(cmdline_parsed.options.begin(),
               cmdline_parsed.options.end(),
               std::back_inserter(unrecognized),
               [](const auto &it) { return it.unregistered; });

  if (config_file) {
    auto config_parsed = po::parse_config_file<char>(config_file->c_str(), config_opts, true);
    po::store(config_parsed, vm);
    po::notify(vm);

    std::copy_if(config_parsed.options.begin(),
                 config_parsed.options.end(),
                 std::back_inserter(unrecognized),
                 [](const auto &it) { return it.unregistered; });
  }

  for (const auto &opt : unrecognized) {
    if (boost::starts_with(opt.string_key, "input.")) {
      std::vector<std::string> parts;
      boost::split(parts, opt.string_key, [](auto it) { return it == '.'; });

      if (parts.size() != 3) {
        throw std::runtime_error("Unknown option " + opt.string_key);
      }

      const auto &input_name = parts[1];
      const auto &param = parts[2];

      const auto &opt_value = opt.value;

      if (opt_value.empty()) {
        throw std::runtime_error("Missing option " + opt.string_key + " value.");
      } else if (opt_value.size() > 1) {
        throw std::runtime_error("Option " + opt.string_key + " is single-value.");
      }

      const auto &value = opt_value.front();

      InputSpec &input = get_input_spec_or_create(*o, input_name);

      if (param == "source") {
        input.source = value;
      } else if (param == "sink") {
        input.sink = value;
      } else if (param == "sourceformat") {
        input.source_format = value;
      } else if (param == "sinkformat") {
        input.sink_format = value;
      } else {
        throw std::runtime_error("Unknown option " + opt.string_key);
      }
    } else {
      LOG(warning) << "Unrecognised option " + opt.string_key;
    }
  }

  if (log_level_map.find(log_level_str) != log_level_map.end()) {
    o->logging_level = log_level_map[log_level_str];
  } else {
    LOG(warning) << "Unknown logging level " << std::quoted(log_level_str) << ". Defaulting to 'info'.";
    o->logging_level = boost::log::trivial::severity_level::info;
  }

  o->start_input_name = boost_optional_to_std(start_input_name);
  o->logging_thread = boost_optional_to_std(log_thread_name);
  o->norestart = vm.count("no-restart") > 0;

  o->output.source_format = boost_optional_to_std(output_source_format);
  o->output.sink_format = boost_optional_to_std(output_sink_format);

  return o;
}

void
ProgramOptions::validate() const
{
  bool terminate = false;

  if (user_inputs.empty()) {
    LOG(error) << "No input provided";
    terminate = true;
  }

  for (const auto &is : user_inputs) {
    if (is.name.empty()) {
      LOG(error) << "One of the inputs has missing name";
      terminate = true;
      continue;
    }

    for (const auto &n : { ClearInput::NAME }) {
      if (is.name == n) {
        LOG(error) << "Input name '" << n << "' is reserved";
        terminate = true;
        continue;
      }
    }

    if (is.source.empty()) {
      LOG(error) << "Input " << is.name << " source missing";
      terminate = true;
    }

    if (is.sink.empty()) {
      LOG(error) << "Input " << is.name << " sink missing";
      terminate = true;
    }
  }

  if (output.source.empty()) {
    LOG(error) << "Output source missing";
    terminate = true;
  }

  if (output.sink.empty()) {
    LOG(error) << "Output sink missing";
    terminate = true;
  }

  if (terminate) {
    throw std::runtime_error("Invalid options provided, terminating");
  }
}
}
