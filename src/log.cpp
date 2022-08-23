#include "log.h"

#include <iomanip>
#include <iostream>

#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/phoenix.hpp>

namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace kw = boost::log::keywords;
namespace trivial = boost::log::trivial;

namespace metamix::log {

namespace {
BOOST_LOG_ATTRIBUTE_KEYWORD(tag_thread_name, "ThreadName", std::string)
}

void
init()
{
  boost::log::add_console_log(
    std::clog,
    // clang-format off
    kw::format = expr::stream
      << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d, %H:%M:%S.%f") << "] "
      << "[" << expr::max_size_decor<char>(18)[
        expr::stream
          << std::setw(18) << std::left
          << expr::if_(expr::has_attr(tag_thread_name))[
            expr::stream << tag_thread_name
          ].else_[
            expr::stream << expr::attr<attrs::current_thread_id::value_type>("ThreadID")
          ]
      ] << "] "
      << "[" << trivial::severity << "] "
      << expr::message
    // clang-format on
  );

  boost::log::add_common_attributes();
}

void
set_filter(std::optional<trivial::severity_level> lvl, std::optional<std::string> thread_name)
{
  auto f = [=](const auto &llvl, const auto &lthread_name) {
    if (lvl && llvl < *lvl)
      return false;
    if (thread_name && lthread_name != *thread_name)
      return false;
    return true;
  };
  boost::log::core::get()->set_filter(boost::phoenix::bind(f, trivial::severity.or_none(), tag_thread_name.or_none()));
}

void
set_thread_name(const std::string &thread_name)
{
  boost::log::core::get()->add_thread_attribute("ThreadName", attrs::constant<std::string>(thread_name));
}
}
