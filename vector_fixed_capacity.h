#ifndef VECTOR_FIXED_CAPACITY_H
#define VECTOR_FIXED_CAPACITY_H

#include <iterator>
#include <type_traits>
#include <memory>
#include <utility>

struct uninitialized_t
{} constexpr uninitialized = uninitialized_t{};

template <typename T, size_t N>
struct vector_fixed_capacity
{
    using value_type     = T;
    using iterator       = T*;
    using const_iterator = T const*;

    vector_fixed_capacity()
        : size_()
    {}

    vector_fixed_capacity(vector_fixed_capacity const& other)
        : size_(other.size_)
    {
        std::uninitialized_copy(other.element_ptr(0), other.element_ptr(size_), element_ptr(0));
    }

    vector_fixed_capacity(size_t size, T const& val = T{})
        : size_()
    {
        resize(size, val);
    }

    vector_fixed_capacity(size_t size, uninitialized_t)
        : size_()
    {
        resize(size, uninitialized);
    }

    vector_fixed_capacity& operator=(vector_fixed_capacity const& other)
    {
        // FIXME: no strong exception-safety guarantee
        std::copy(other.element_ptr(0), other.element_ptr(size_), element_ptr(0));
    }

    ~vector_fixed_capacity()
    {
        destroy_list(element_ptr(0), element_ptr(size_));
    }

    template<typename ...Args>
    void emplace_back(Args&&... args)
    {
        if (size_ >= N)
            throw std::bad_alloc{};
        new (data+size_) T(std::forward<Args>(args)...);
        ++size_;
    }

    void resize(size_t new_size, T const& val = T{})
    {
        if (new_size > size_)
        {
            std::uninitialized_fill(element_ptr(size_), element_ptr(new_size), val);
            size_ = new_size;
        }
        else if (new_size < size_)
        {
            destroy_list(element_ptr(new_size), element_ptr(size_));
        }
    }

    void resize(size_t new_size, uninitialized_t)
    {
        static_assert(std::is_trivial<T>::value, "uninitialized_resize is applicable only to trivial types");

        size_ = new_size;
    }

    T const* data() const
    {
        return element_ptr(0);
    }

    size_t size() const
    {
        return size_;
    }

    iterator       begin()        { return element_ptr(0); }
    const_iterator begin()  const { return element_ptr(0); }
    const_iterator cbegin() const { return element_ptr(0); }
    iterator       end()          { return element_ptr(size_); }
    const_iterator end()    const { return element_ptr(size_); }
    const_iterator cend()   const { return element_ptr(size_); }

private:
    T* element_ptr(size_t index)
    {
        return reinterpret_cast<T*>(&storage[index]);
    }

    T const* element_ptr(size_t index) const
    {
        return reinterpret_cast<T*>(&storage[index]);
    }

    template <typename BidirectionalIterator>
    static void destroy_list(BidirectionalIterator first, BidirectionalIterator last)
    {
        while (first != last)
        {
            --last;
            typedef typename std::iterator_traits<BidirectionalIterator>::value_type value_type;
            value_type* val = &*last;
            val->~value_type();
        }
    }

private:
    size_t size_;
    std::aligned_storage<sizeof(T), alignof(T)>::type storage[N];
};

#endif // VECTOR_FIXED_CAPACITY_H
