#ifndef SUB_STRING_H
#define SUB_STRING_H

#include <string>

struct sub_string
{
    sub_string();
    sub_string(char const* begin, char const* end);
    sub_string(std::string const&);

    template <size_t N>
    static sub_string literal(char const (&)[N]);

    void advance(size_t i);

    char const& operator[](size_t i) const;

    void begin(char const*);
    void end(char const*);
    char const* begin() const;
    char const* end() const;

    bool empty() const;
    size_t size() const;

    char const* data() const;

    bool has_prefix(sub_string other) const;
    bool try_drop_prefix(sub_string other);

    std::string as_string() const;

private:
    char const* begin_;
    char const* end_;
};

template <size_t N>
sub_string sub_string::literal(char const (&str)[N])
{
    static_assert(N != 0, "invalid argument");

    return sub_string{str + 0, str + N - 1};
}

#endif // SUB_STRING_H
