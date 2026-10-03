#include "zdb/util/byte_buffer.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <print>

namespace zdb::util {

ByteBuffer::ByteBuffer() : buffer_(DEFAULT_BUF_SIZE) {}

void ByteBuffer::prepare(std::size_t n) {
  // Necessary to compact only when space is needed for new data, and worth
  // compacting only when the amount consumed is at least the amount to move
  if (read_pos_ >= size())
    compact();

  std::size_t writable_mem = buffer_.size() - write_pos_;
  if (writable_mem < n) {
    buffer_.resize(std::max(write_pos_ + n,
                            buffer_.size() * 2)); // Ensure O(1) amortised
  }
}

void ByteBuffer::append(std::span<const std::byte> stream) {
  prepare(stream.size());
  std::memcpy(buffer_.data() + write_pos_, stream.data(), stream.size());
  if (!commit(stream.size())) {
    std::println("ByteBuffer logic flawed, verify the logic again!");
    std::exit(EXIT_FAILURE);
  }
}

bool ByteBuffer::consume(std::size_t n) {
  if (n > size())
    return false;

  read_pos_ += n;
  return true;
}

std::span<const std::byte> ByteBuffer::readable() const {
  return std::span<const std::byte>{buffer_}.subspan(read_pos_, size());
}

std::span<std::byte> ByteBuffer::writable(std::size_t n) {
  prepare(n);
  return std::span<std::byte>{buffer_}.subspan(write_pos_, n);
}

std::size_t ByteBuffer::size() const { return write_pos_ - read_pos_; }

bool ByteBuffer::empty() const { return size() == 0; }

bool ByteBuffer::commit(std::size_t n) {
  if (write_pos_ + n > buffer_.size())
    return false;

  write_pos_ += n;
  return true;
}

void ByteBuffer::compact() {
  if (read_pos_ > 0) {
    std::memmove(buffer_.data(), readable().data(), size());
    write_pos_ = size();
    read_pos_ = 0;
  }
}

} // namespace zdb::util
