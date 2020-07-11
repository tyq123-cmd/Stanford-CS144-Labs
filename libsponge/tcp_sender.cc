#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _abs_ackno(0)
    , _timer()
    , _current_time(0)
    , _window_size(1)
    , _consecutive_retrans(0)
    , _retransmission_timeout(retx_timeout)
    , _unacked_segments() {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _abs_ackno; }

void TCPSender::fill_window() {
    do {
        size_t len_to_read =
            min(min(TCPConfig::MAX_PAYLOAD_SIZE, _stream.buffer_size()), static_cast<size_t>(_window_size));
        if (!len_to_read && _syn_num == 2)
            return;
        TCPSegment tcp_segment;
        auto &payload = tcp_segment.payload();
        payload = Buffer(_stream.read(len_to_read));
        auto &header = tcp_segment.header();
        header.syn = !_next_seqno;
        _syn_num += !_next_seqno;
        if (!_fin && (tcp_segment.length_in_sequence_space() + _stream.input_ended()) <= _window_size) {
            header.fin = _stream.input_ended();
            _fin = _stream.input_ended();
        }
        if (!tcp_segment.length_in_sequence_space() && !_ack_received)
            return;
        header.seqno = wrap(_next_seqno, _isn);
        _next_seqno += tcp_segment.length_in_sequence_space();
        _window_size -= len_to_read;
        _segments_out.push(tcp_segment);
        // record the largest seqno in the segment

        _unacked_segments.insert({_next_seqno, tcp_segment});

        if (!_timer.is_turn_on()) {
            _timer.turn_on(_retransmission_timeout);
            _timer.set_last_expire_time(_current_time);
        }
        if (!_window_size) {
            break;
        }
        _ack_received = false;
    } while ((!_next_seqno || _stream.buffer_size() || (_window_size == 0 && _ack_received)));
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    auto abs_ackno = unwrap(ackno, _isn, _abs_ackno);
    if (abs_ackno > _next_seqno)
        return false;
    if (abs_ackno < _abs_ackno)
        return true;
    if (window_size == 0)
        _ack_received = true;
    _consecutive_retrans = 0;
    _retransmission_timeout = _initial_retransmission_timeout;
    _timer.set_last_expire_time(_current_time);
    _timer.set_timeout(_retransmission_timeout);
    _abs_ackno = abs_ackno;
    _window_size = window_size - (_next_seqno - _abs_ackno);
    for (auto it = _unacked_segments.begin(); it != _unacked_segments.end();) {
        if (it->first <= abs_ackno) {
            it = _unacked_segments.erase(it);
        } else
            break;
    }
    if (!_unacked_segments.size())
        _timer.turn_off();
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _current_time += ms_since_last_tick;
    if (_timer.is_turn_on() && _timer.is_expire(_current_time) && _unacked_segments.size()) {
        auto earlist_segment = _unacked_segments.begin();
        retrainsmit(earlist_segment->first);
        if (_window_size) {
            _consecutive_retrans++;
            _retransmission_timeout *= 2;
        }
        _timer.set_last_expire_time(_current_time);
        _timer.set_timeout(_retransmission_timeout);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retrans; }

void TCPSender::retrainsmit(uint64_t abs_seqno) { _segments_out.push(_unacked_segments[abs_seqno]); }

void TCPSender::send_empty_segment() {
    TCPSegment tcp_segment;
    auto &header = tcp_segment.header();
    header.seqno = wrap(_next_seqno, _isn);
    _segments_out.push(tcp_segment);
}
