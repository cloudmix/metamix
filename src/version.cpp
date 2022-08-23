#include "version.h"

#include <iostream>

#include <boost/format.hpp>
#include <boost/version.hpp>

#include <nlohmann/json.hpp>

#include "ffmpeg.h"

#include <src/config.h>

namespace metamix {

void
print_version()
{
  auto boost_ver =
    boost::format("%1%.%2%.%3%") % (BOOST_VERSION / 100000) % (BOOST_VERSION / 100 % 1000) % (BOOST_VERSION % 100);
  auto json_ver = boost::format("%1%.%2%.%3%") % NLOHMANN_JSON_VERSION_MAJOR % NLOHMANN_JSON_VERSION_MINOR %
                  NLOHMANN_JSON_VERSION_PATCH;

  std::cout << "Metamix " << METAMIX_VERSION_STRING << std::endl
            << "Developed by Software Mansion for 5stream/Cloudmix" << std::endl
            << std::endl
            << "Boost C++ Libraries  " << boost_ver.str() << std::endl
            << "FFmpeg               " << av_version_info() << std::endl
            << "nlohmann/json.hpp    " << json_ver.str() << std::endl
            << std::endl
            << "FFmpeg configuration:" << std::endl
            << avutil_configuration() << std::endl;
}
}
