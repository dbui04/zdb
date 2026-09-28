#include "zdb/payload.h"
#include "zdb/parse.h"
#include "zdb/utils.h"
#include <span>

namespace zdb::payload {

/* Request */

std::vector<std::byte> Request::serialise() {
  // Calculate sizes before serialisation
  calculate_sizes();
  std::vector<std::byte> result;

  result.append_range(parse::serialise(req_sz_));
  result.append_range(parse::serialise(cmd_));
  result.append_range(parse::serialise(key_sz_));
  result.append_range(parse::serialise(key_));
  result.append_range(parse::serialise(value_sz_));
  result.append_range(parse::serialise(value_));

  return result;
}

std::size_t Request::deserialise(std::span<const std::byte> buf) {
  if (buf.size() < sizeof(req_sz_))
    return 0;

  std::size_t offset{0};

  req_sz_ = parse::deserialise<decltype(req_sz_)>(
      utils::get_static_span<decltype(req_sz_)>(buf, offset));
  offset += sizeof(req_sz_);

  if (buf.size() < req_sz_)
    return 0;

  cmd_ = parse::deserialise<decltype(cmd_)>(
      utils::get_static_span<decltype(cmd_)>(buf, offset));
  offset += sizeof(cmd_);

  key_sz_ = parse::deserialise<decltype(key_sz_)>(
      utils::get_static_span<decltype(key_sz_)>(buf, offset));
  offset += sizeof(key_sz_);

  key_ = parse::deserialise(buf.subspan(offset, key_sz_));
  offset += key_sz_;

  value_sz_ = parse::deserialise<decltype(key_sz_)>(
      utils::get_static_span<decltype(value_sz_)>(buf, offset));
  offset += sizeof(value_sz_);

  value_ = parse::deserialise<std::byte>(buf.subspan(offset, value_sz_));
  offset += value_sz_;

  if (!verify_data_model())
    throw std::logic_error("Request sizes do not match the invariant sizes");
  return offset;
}

void Request::calculate_sizes() {
  key_sz_ = std::as_bytes(std::span{key_}).size();
  value_sz_ = std::as_bytes(std::span{value_}).size();
  req_sz_ = sizeof(req_sz_) + sizeof(cmd_) + sizeof(key_sz_) + key_sz_ +
            sizeof(value_sz_) + value_sz_;
}

bool Request::verify_data_model() const {
  std::size_t key_byte_cnt = std::as_bytes(std::span{key_}).size();
  std::size_t value_byte_cnt = std::as_bytes(std::span{value_}).size();
  std::size_t req_byte_cnt = sizeof(req_sz_) + sizeof(cmd_) + sizeof(key_sz_) +
                             key_byte_cnt + sizeof(value_sz_) + value_byte_cnt;
  bool req_sz_chk = (req_sz_ == req_byte_cnt);
  bool key_sz_chk = (key_sz_ == key_byte_cnt);
  bool value_sz_chk = (value_sz_ == value_byte_cnt);
  return req_sz_chk & key_sz_chk & value_sz_chk;
}

/* Response */

std::vector<std::byte> Response::serialise() {
  // Calculate sizes before serialisation
  calculate_sizes();

  std::vector<std::byte> result;

  result.append_range(parse::serialise(res_sz_));
  result.append_range(parse::serialise(status_));
  result.append_range(parse::serialise(data_sz_));
  result.append_range(parse::serialise(data_));

  return result;
}

std::size_t Response::deserialise(std::span<const std::byte> buf) {
  if (buf.size() < sizeof(res_sz_))
    return 0;

  std::size_t offset{0};

  res_sz_ = parse::deserialise<decltype(res_sz_)>(
      utils::get_static_span<decltype(res_sz_)>(buf, offset));
  offset += sizeof(res_sz_);

  if (buf.size() < res_sz_)
    return 0;

  status_ = parse::deserialise<decltype(status_)>(
      utils::get_static_span<decltype(status_)>(buf, offset));
  offset += sizeof(status_);

  data_sz_ = parse::deserialise<decltype(data_sz_)>(
      utils::get_static_span<decltype(data_sz_)>(buf, offset));
  offset += sizeof(data_sz_);

  data_ = parse::deserialise<std::byte>(buf.subspan(offset, data_sz_));
  offset += data_sz_;

  // Check if the response is valid
  if (!verify_data_model())
    throw std::logic_error("Response sizes do not match the invariant sizes");

  return offset;
}

void Response::calculate_sizes() {
  data_sz_ = std::as_bytes(std::span{data_}).size();
  res_sz_ = sizeof(res_sz_) + sizeof(status_) + sizeof(data_sz_) + data_sz_;
}

bool Response::verify_data_model() const {
  std::size_t data_byte_cnt = std::as_bytes(std::span{data_}).size();
  std::size_t res_byte_cnt =
      sizeof(res_sz_) + sizeof(status_) + sizeof(data_sz_) + data_byte_cnt;
  bool res_sz_chk = (res_sz_ == res_byte_cnt);
  bool data_sz_chk = (data_sz_ == data_byte_cnt);
  return res_sz_chk & data_sz_chk;
}
} // namespace zdb::payload
