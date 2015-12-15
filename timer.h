#ifndef TIMER_H
#define TIMER_H

#include <cstdint>
#include <functional>
#include <set>
#include <chrono>

struct timer_element;

struct timer
{
    typedef std::chrono::steady_clock clock_t;

    timer();

    void add(timer_element* e);
    void remove(timer_element* e);

    bool empty() const;
    clock_t::time_point top() const;
    void notify(clock_t::time_point now);

private:
    typedef std::pair<clock_t::time_point, timer_element*> value_t;
    std::set<value_t> queue;
};

struct timer_element
{
    typedef timer::clock_t clock_t;
    typedef std::function<void ()> callback_t;

    timer_element();
    timer_element(callback_t callback);
    timer_element(timer& t, clock_t::duration interval, callback_t callback);
    timer_element(timer& t, clock_t::time_point wakeup, callback_t callback);
    timer_element(timer_element const&) = delete;
    timer_element& operator=(timer_element const&) = delete;
    ~timer_element();

    void set_callback(callback_t callback);
    void restart(timer& t, clock_t::duration interval);
    void restart(timer& t, clock_t::time_point wakeup);

private:
    timer* t;
    clock_t::time_point wakeup;
    uint64_t wakeup_time_point;
    callback_t callback;

    friend struct timer;
};

#endif // TIMER_H
