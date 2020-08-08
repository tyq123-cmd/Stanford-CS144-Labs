#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader header = seg.header();
    if (!_syn_set) {
        if (header.syn) {
            _isn = header.seqno;
            _ackno = _isn;
            _syn_set = true;
        } else {
            return false;
        }
    } else {
        if (header.syn) {
            return false;
        }
    }

    if (_fin_set && header.fin) {
        return false;
    }

    auto seg_seqno_start = unwrap(header.seqno, _isn, _checkpoint);
    auto seg_seqno_end = seg_seqno_start + (seg.length_in_sequence_space() ? seg.length_in_sequence_space() - 1 : 0);
    auto seqno_start = unwrap(_ackno, _isn, _checkpoint);
    auto seqno_end = seqno_start + (window_size() ? window_size() - 1 : 0);
    auto payload_end = seg_seqno_end;
    if (header.syn && header.fin) {
        payload_end = payload_end > 2 ? payload_end - 2 : 0;
    } else if (header.syn || header.fin) {
        payload_end = payload_end > 1 ? payload_end - 1 : 0;
    }
    if (seg_seqno_end < seg_seqno_start)
        seg_seqno_end++;
    bool fall_into_window =
        (seg_seqno_start >= seqno_start && seg_seqno_start <= seqno_end)  // seqno fall into the interval
        || (payload_end >= seqno_start && seg_seqno_end <= seqno_end);    // some new contents
    if (fall_into_window) {
        size_t max_len = min(seg.length_in_sequence_space() - header.syn - header.fin,
                             seqno_start + window_size() - seg_seqno_start);
        _reassembler.push_substring(seg.payload().copy().substr(0, max_len), seg_seqno_start - 1, header.fin);
    }
    _checkpoint = _reassembler.stream_out().bytes_written();
    if (!_fin_set && header.fin) {
        _fin_set = true;
        if (header.syn && seg.length_in_sequence_space() == 2)
            _reassembler.stream_out().end_input();
    }
    if (payload_end <= _checkpoint) {
        _ackno = wrap(_reassembler.stream_out().bytes_written() +
                          (_fin_set && (_reassembler.unassembled_bytes() == 0) ? 1 : 0) + 1,
                      _isn);
    } else if (seg.length_in_sequence_space()) {
        _ackno = wrap(_reassembler.stream_out().bytes_written() + 1, _isn);
    }
    if (fall_into_window || header.fin || header.syn) {
        return true;
    }
    return false;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn_set) {
        return {};
    } else {
        return _ackno;
    }
}

size_t TCPReceiver::window_size() const {
    // the lab documentation gives a wrong guide here
    return _capacity - _reassembler.stream_out().buffer_size();
}
