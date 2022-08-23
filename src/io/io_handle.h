#pragma once

#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "../ffmpeg.h"
#include "../optional_io.h"
#include "../util.h"

#include "stream_classification.h"

namespace metamix::io {

class IOHandle
{
private:
  std::string m_name;
  std::string m_url;

protected:
  ff::AVFormatContextUniquePtr fmt_ctx{ nullptr };

public:
  explicit IOHandle(std::string name, std::string url)
    : m_name{ std::move(name) }
    , m_url{ std::move(url) }
  {}

  IOHandle(const IOHandle &) = delete;
  IOHandle &operator=(const IOHandle &) = delete;

  IOHandle(IOHandle &&other) noexcept = default;
  IOHandle &operator=(IOHandle &&other) noexcept = default;

  virtual ~IOHandle() = default;

  const std::string &name() const { return m_name; }

  const std::string &url() const { return m_url; }

  const AVStream &get_stream(int stream_index) const { return *NULL_PROTECT(get_stream_ptr(stream_index)); }

  const AVStream &get_stream(size_t stream_index) const { return *NULL_PROTECT(get_stream_ptr(stream_index)); }

  template<class K>
  const AVStream &get_stream(const StreamClassification &sc) const
  {
    return *NULL_PROTECT(get_stream_ptr<K>(sc));
  }

  std::vector<const AVStream *> all_streams() const;

  size_t stream_count() const { return fmt_ctx->nb_streams; }

  StreamClassification classify_streams() const;

private:
  const AVStream *get_stream_ptr(int stream_index) const { return get_stream_ptr(static_cast<size_t>(stream_index)); }

  const AVStream *get_stream_ptr(size_t stream_index) const
  {
    if (stream_index < 0 || stream_index >= fmt_ctx->nb_streams) {
      throw std::runtime_error("Out-of-range access to stream table.");
    }

    return fmt_ctx->streams[stream_index];
  }

  template<class K>
  const AVStream *get_stream_ptr(const StreamClassification &sc) const
  {
    const std::optional<size_t> &opt = K::get(sc);
    if (opt) {
      return get_stream_ptr(*opt);
    } else {
      throw std::runtime_error(std::string("Getting unregistered stream class: ") + K::DESCRIPTION);
    }
  }
};
}
