#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _buffer(), _capacity(capacity), _size(0), _nwritten(0), _nread(0), _input_ended(false) {}

size_t ByteStream::write(const string &data) {
    size_t remain_capacity = remaining_capacity();
    string data_to_write = data.substr(0, remain_capacity);
    for (auto chr : data_to_write) {
        _buffer.emplace_back(chr);
    }
    _nwritten += data_to_write.size();
    _size += data_to_write.size();
    return data_to_write.size();
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return string(_buffer.begin(), _buffer.begin() + min(_size, len));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t len_to_pop = min(_size, len);
    for (size_t i = 0; i < len_to_pop; ++i) {
        _buffer.pop_front();
    }
    _nread += len_to_pop;
    _size -= len_to_pop;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _size; }

bool ByteStream::buffer_empty() const { return _size == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _nwritten; }

size_t ByteStream::bytes_read() const { return _nread; }

size_t ByteStream::remaining_capacity() const { return _capacity - _size; }
