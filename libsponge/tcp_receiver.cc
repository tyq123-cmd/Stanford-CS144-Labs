#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool is_overlap(const pair<uint64_t, uint64_t> &inter1, const pair<uint64_t, uint64_t> &inter2) {
    return (inter2.second > inter1.first)     // the data has not been written
           && (inter1.second > inter2.first)  // the data falls into the window
           && (max(inter1.second, inter2.second) - min(inter1.first, inter2.first) <=
               ((inter1.second - inter1.first) + (inter2.second - inter2.first)));  // interval overlap
}

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader header = seg.header();
    if (!syn_set) {
        if (header.syn) {
            isn = header.seqno;
            _ackno = isn;
            syn_set = true;
        } else {
            return false;
        }
    } else {
        if (header.syn) {
            return false;
        }
    }

    if (fin_set && header.fin) {
        return false;
    }

    auto payload = seg.payload().copy();
    auto seg_abs_seqno = unwrap(header.seqno, isn, checkpoint);
    auto length = seg.length_in_sequence_space();
    auto tcp_abs_seqno = unwrap(_ackno, isn, checkpoint);
    auto right_end_of_packet = seg_abs_seqno + length - header.syn - header.fin;
    if (header.syn || header.fin ||
        is_overlap({tcp_abs_seqno, tcp_abs_seqno + window_size()}, {seg_abs_seqno, right_end_of_packet}) ||
        ((tcp_abs_seqno == right_end_of_packet) && !payload.size())  // zero probing
    ) {
        auto pre_cnt = _reassembler.stream_out().buffer_size();
        size_t max_len = min(length - header.syn - header.fin, tcp_abs_seqno + window_size() - seg_abs_seqno);
        fin_set = header.fin;
        _reassembler.push_substring(
            payload.substr(0, max_len), seg_abs_seqno - static_cast<uint64_t>(!header.syn), header.fin);
        auto assembled_size = (_reassembler.stream_out().buffer_size() - pre_cnt);
        _assembled_size += assembled_size;
        checkpoint = assembled_size + seg_abs_seqno;
        _ackno = _ackno + assembled_size + header.syn + header.fin;
        return true;
    }
    return false;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!syn_set) {
        return {};
    } else {
        return _ackno;
    }
}

size_t TCPReceiver::window_size() const {
    // the lab documentation gives a wrong guide here
    return _capacity - _reassembler.stream_out().buffer_size();
}
