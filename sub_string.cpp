#include "sub_string.h"

#include <cassert>

sub_string::sub_string()
    : begin_(nullptr)
    , end_(nullptr)
{}

sub_string::sub_string(char const* begin, char const* end)
    : begin_(begin)
    , end_(end)
{}

void sub_string::advance(size_t i)
{
    assert(i <= size());
    begin_ += i;
}

char const& sub_string::operator[](size_t i) const
{
    assert(i < size());
    return begin_[i];
}

void sub_string::begin(char const* arg)
{
    begin_ = arg;
}

void sub_string::end(char const* arg)
{
    end_ = arg;
}

char const* sub_string::begin() const
{
    return begin_;
}

char const* sub_string::end() const
{
    return end_;
}

bool sub_string::empty() const
{
    return begin_ == end_;
}

size_t sub_string::size() const
{
    return end_ - begin_;
}

bool sub_string::has_prefix(sub_string other) const
{
    if (size() < other.size())
        return false;

    return std::equal(other.begin_, other.end_, begin_);
}

bool sub_string::try_drop_prefix(sub_string other)
{
    if (has_prefix(other))
    {
        advance(other.size());
        return true;
    }
    return false;
}

std::string sub_string::as_string() const
{
    return std::string{begin_, end_};
}
