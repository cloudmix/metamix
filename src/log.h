#pragma once

#include <optional>
#include <string>

#include <boost/log/trivial.hpp>

namespace metamix::log {

void
init();

void
set_filter(std::optional<boost::log::trivial::severity_level> level, std::optional<std::string> thread_name);

void
set_thread_name(const std::string &thread_name);
}

/*!
 * The macro is used to initiate logging. The \c lvl argument of the macro specifies one of the following
 * severity levels: \c trace, \c debug, \c info, \c warning, \c error or \c fatal (see \c severity_level enum).
 * Following the macro, there may be a streaming expression that composes the record message string. For example:
 *
 * \code
 * LOG(info) << "Hello, world!";
 * \endcode
 */
#define LOG(lvl) BOOST_LOG_TRIVIAL(lvl)
