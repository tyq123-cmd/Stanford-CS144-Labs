
#include "timer.hh"

void Timer::turn_on(unsigned int timeout) {
    _timeout = timeout;
    _is_turn_on = true;
}

bool Timer::is_turn_on() const { return _is_turn_on; }

void Timer::turn_off() { _is_turn_on = false; }

bool Timer::is_expire(unsigned int current_time) const { return (current_time - _last_expire_time) >= _timeout; }

void Timer::set_timeout(unsigned int timeout) { _timeout = timeout; }

void Timer::set_last_expire_time(unsigned int last_expire_time) { _last_expire_time = last_expire_time; }