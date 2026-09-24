#pragma once

#include <cstddef>
#include <span>
#include <vector>

constexpr std::size_t DEFAULT_BUF_SIZE =
    4096; // Should profile to find best initial size to avoid multiple small
          // reallocations
constexpr std::size_t BUF_DEL_THRESHOLD =
    4096; // Need profiling later to find out

class ByteBuffer {
public:
  ByteBuffer();
  void append(std::span<const std::byte>);
  void consume(std::size_t); // Mark first n readable bytes as read
  void commit(std::size_t);  // Commit next n bytes as written
  void prepare(std::size_t); // Ensure n bytes are available for writing
  std::span<const std::byte>
  readable() const; // Call consume after parsing from readable
  std::span<std::byte>
  writable(std::size_t); // Call commit() after writing to writable
  std::size_t size() const;
  bool empty() const;

private:
  std::vector<std::byte> buffer_;
  std::size_t read_pos_ = 0;
  std::size_t write_pos_ = 0;

  void compact();
};
