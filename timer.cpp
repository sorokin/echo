#include "timer.h"

#include <cassert>
#include <iostream>

timer::timer()
{}

void timer::add(timer_element* e)
{
    queue.insert(value_t(e->wakeup, e));
}

void timer::remove(timer_element *e)
{
    auto i = queue.find(value_t(e->wakeup, e));
    assert(i != queue.end());
    queue.erase(i);
}

bool timer::empty() const
{
    return queue.empty();
}

timer::clock_t::time_point timer::top() const
{
    assert(!empty());
    return queue.begin()->first;
}

void timer::notify(clock_t::time_point now)
{
    for (;;)
    {
        if (queue.empty())
            break;

        auto i = queue.begin();
        if (i->first > now)
            break;

        i->second->t = nullptr;
        try
        {
            i->second->callback();
        }
        catch (std::exception const& e)
        {
            std::cerr << "error: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "unknown exception in timer::notify()" << std::endl;
        }

        queue.erase(i);
    }
}

timer_element::timer_element()
    : t(nullptr)
{}

timer_element::timer_element(timer_element::callback_t callback)
    : t(nullptr)
    , callback(std::move(callback))
{}

timer_element::timer_element(timer& t, clock_t::duration interval, callback_t callback)
    : t(&t)
    , wakeup(clock_t::now() + interval)
    , callback(std::move(callback))
{
    t.add(this);
}

timer_element::timer_element(timer& t, clock_t::time_point wakeup, callback_t callback)
    : t(&t)
    , wakeup(wakeup)
    , callback(std::move(callback))
{
    t.add(this);
}

timer_element::~timer_element()
{
    if (t)
        t->remove(this);
}

void timer_element::set_callback(timer_element::callback_t callback)
{
    this->callback = std::move(callback);
}

void timer_element::restart(timer& t, clock_t::duration interval)
{
    this->t->remove(this);
    this->t = &t;
    this->wakeup = clock_t::now() + interval;
    this->t->add(this);
}

void timer_element::restart(timer& t, clock_t::time_point wakeup)
{
    this->t->remove(this);
    this->t = &t;
    this->wakeup = wakeup;
    this->t->add(this);
}
