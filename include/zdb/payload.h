#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace zdb::payload {

class Payload {
public:
  virtual std::vector<std::byte> serialise() = 0;
  virtual std::size_t deserialise(std::span<const std::byte>) = 0;
  virtual bool verify_data_model() const = 0;

private:
  virtual void calculate_sizes() = 0;
};

class Request : public Payload {
public:
  enum class Command { set, get, del };

  Command cmd_;
  std::string key_;
  std::vector<std::byte> value_;

  std::vector<std::byte> serialise() override;
  std::size_t deserialise(std::span<const std::byte>) override;
  bool verify_data_model() const override;

private:
  std::size_t req_sz_;
  std::size_t key_sz_;
  std::size_t value_sz_;

  void calculate_sizes() override;
};

class Response : public Payload {
public:
  enum class Status { ok, invalid_req_format, key_not_found, key_existed };

  Status status_;
  std::vector<std::byte> data_;

  std::vector<std::byte> serialise() override;
  std::size_t deserialise(std::span<const std::byte>) override;
  bool verify_data_model() const override;

private:
  std::size_t res_sz_;
  std::size_t data_sz_;

  void calculate_sizes() override;
};
} // namespace zdb::payload
