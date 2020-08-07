#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _current_time - _last_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _last_received = _current_time;
    auto &header = seg.header();
    bool send_empty = false;
    bool ack_valid = false;

    if (header.ack && (_receiver.ackno().has_value() || header.syn)) {
        ack_valid = _sender.ack_received(header.ackno, header.win);
        if (ack_valid) {
            _sender.fill_window();
        } else {
            send_empty = true;
        }
    }
    bool rec_valid = _receiver.segment_received(seg);
    if (!_is_initialized && header.syn) {
        connect();
    }
    if ((rec_valid || (header.ack && (_sender.next_seqno() == header.ackno))) && header.rst) {
        _is_reset_received = true;
        _sender.stream_in().set_error();
        inbound_stream().set_error();
        return;
    }
    if (rec_valid && _segments_out.empty() && seg.length_in_sequence_space()) {
        send_empty = true;
    }
    if (!rec_valid && _receiver.ackno().has_value() && !header.rst) {
        send_empty = true;
    }
    // why the fsm_ack_rst and fsm_ack_rst_relaxed expects exactly opposite behavior, but we need pass both of them?
    if (((!seg.length_in_sequence_space() && !_receiver.ackno().has_value()) || (!_is_initialized && header.ack)) &&
        !header.rst) {
        _send_rst = true;
        send_empty = true;
        _use_rst_seqno = true;
        _rst_seqno = header.ackno;
        _is_reset_received = true;
        _sender.stream_in().set_error();
        inbound_stream().set_error();
    }

    if (send_empty)
        _sender.send_empty_segment();
    fill_queue();
    if (header.fin && !_sender.stream_in().eof() && _is_initialized) {
        _linger_after_streams_finish = false;
    }
}

bool TCPConnection::active() const {
    bool unclean_shutdown = _is_reset_received || _send_rst;
    bool clean_shutdown =
        !unassembled_bytes() && _receiver.stream_out().eof() && _sender.stream_in().eof() && !bytes_in_flight() &&
        (!_linger_after_streams_finish || (time_since_last_segment_received() >= 10 * _cfg.rt_timeout));
    return !unclean_shutdown && !clean_shutdown;
}

size_t TCPConnection::write(const string &data) {
    if (!data.size())
        return 0;
    size_t num_written = _sender.stream_in().write(data);
    _sender.fill_window();
    fill_queue();
    return num_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _current_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    fill_queue();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    fill_queue();
}

void TCPConnection::connect() {
    _is_initialized = true;
    _sender.fill_window();
    fill_queue();
}

void TCPConnection::update_seg(TCPSegment &seg) {
    auto ackno = _receiver.ackno();
    if (ackno.has_value() && !_send_rst) {
        seg.header().ackno = ackno.value();
        seg.header().ack = true;
    }
    if (_send_rst) {
        seg.header().rst = true;
        if (_use_rst_seqno) {
            seg.header().seqno = _rst_seqno;
        }
    }
    seg.header().win = _receiver.window_size();
}

void TCPConnection::fill_queue() {
    auto &sender_out = _sender.segments_out();
    while (!sender_out.empty()) {
        auto seg = sender_out.front();
        if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
            _send_rst = true;
            _use_rst_seqno = true;
            _sender.stream_in().set_error();
            inbound_stream().set_error();
        }
        sender_out.pop();
        update_seg(seg);
        _segments_out.push(seg);
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _send_rst = true;
            _use_rst_seqno = false;
            _sender.stream_in().set_error();
            inbound_stream().set_error();
            _sender.send_empty_segment();
            fill_queue();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
