#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// ByteStream::ByteStream(const size_t capacity) { DUMMY_CODE(capacity); }
 ByteStream::ByteStream(const size_t capacity) : _buffer(), _capacity(capacity), _size(0), _NumWritten(0), _NumRead(0),_input_end(false){}

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    // return {};
    string data_write = data.substr(0,remaining_capacity());    // drop redundant bytes
    for(auto ch : data_write)
        _buffer.push_back(ch);
    
    size_t data_size = data_write.size();
    _size+=data_size;
    _NumWritten += data_size;A
    return data_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    // return {};
    return string(_buffer.begin(), _buffer.begin() + min(_size,len));  // constructor of iterator
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    // DUMMY_CODE(len); 
    size_t len_pop = min(_size, len);

    for (size_t i = 0; i < len_pop; ++i) {
        _buffer.pop_front();
    }
    _size -= len_pop;
    _NumRead += len_pop;

}

void ByteStream::end_input() {_input_end=true; }

bool ByteStream::input_ended() const { return _input_end; }

size_t ByteStream::buffer_size() const { return _size; }

bool ByteStream::buffer_empty() const { return _size==0; }

bool ByteStream::eof() const { return input_ended()&&buffer_empty(); }

size_t ByteStream::bytes_written() const { return _NumWritten; }

size_t ByteStream::bytes_read() const { return _NumRead; }

size_t ByteStream::remaining_capacity() const { return _capacity - _size; }
