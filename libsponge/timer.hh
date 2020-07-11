#ifndef SPONGE_LIBSPONGE_TIMER_HH
#define SPONGE_LIBSPONGE_TIMER_HH

class Timer {
    unsigned int _timeout;
    unsigned int _last_expire_time;
    bool _is_turn_on;

  public:
    Timer() : _timeout(0), _last_expire_time(0), _is_turn_on(false) {}
    void turn_on(unsigned int timeout);
    bool is_turn_on() const;
    void turn_off();
    bool is_expire(unsigned int delta) const;
    void set_timeout(unsigned int timeout);
    void set_last_expire_time(unsigned int last_expire_time);
};

#endif  // SPONGE_LIBSPONGE_TCP_RECEIVER_HH
