#ifndef SPACE_HPP
#define SPACE_HPP

#include <cstdint>
#include <ratio>

namespace space {
template <class _Rep, class _Capacity = std::ratio<1>>
class Space;

template <typename _ToSpace, typename _Rep, typename _Capacity>
constexpr _ToSpace space_cast(const Space<_Rep, _Capacity>& __d) {
    auto __r = __d.count() * std::ratio_divide<_Capacity, typename _ToSpace::period>::num /
               std::ratio_divide<_Capacity, typename _ToSpace::period>::den;
    return _ToSpace(__r);
}

template <typename _Rep, typename _Capacity>
class Space {
public:
    using rep = _Rep;
    using period = _Capacity;

    constexpr Space() : _rep() {};
    template <typename _Rep2>
    constexpr Space(const _Rep2& __r) : _rep(__r) {}
    template <typename _Rep2, typename _Capacity2>
    constexpr Space(const Space<_Rep2, _Capacity2>& __d)
        : _rep(space_cast<Space<_Rep, _Capacity>>(__d).count()) {}
    ~Space() = default;

    _Rep count() const { return _rep; }
    constexpr Space operator+() const { return *this; }
    constexpr Space operator-() const { return Space(-count()); }
    Space& operator++() {
        ++_rep;
        return *this;
    }
    Space operator++(int) {
        Space __tmp(*this);
        ++*this;
        return __tmp;
    }
    Space& operator--() {
        --_rep;
        return *this;
    }
    Space operator--(int) {
        Space __tmp(*this);
        --*this;
        return __tmp;
    }
    Space& operator+=(const Space& __d) {
        _rep += __d.count();
        return *this;
    }
    Space& operator-=(const Space& __d) {
        _rep -= __d.count();
        return *this;
    }
    Space& operator*=(const Space& __d) {
        _rep *= __d.count();
        return *this;
    }
    Space& operator/=(const Space& __d) {
        _rep /= __d.count();
        return *this;
    }
    Space& operator%=(const Space& __d) {
        _rep %= __d.count();
        return *this;
    }

private:
    _Rep _rep;
};

template <typename _Rep, typename _Capacity>
constexpr Space<_Rep, _Capacity> operator+(Space<_Rep, _Capacity> __l_num, Space<_Rep, _Capacity> __r_num) {
    return Space<_Rep, _Capacity>(__l_num.count() + __r_num.count());
}

template <typename _Rep, typename _Capacity>
constexpr Space<_Rep, _Capacity> operator-(Space<_Rep, _Capacity> __l_num, Space<_Rep, _Capacity> __r_num) {
    return Space<_Rep, _Capacity>(__l_num.count() - __r_num.count());
}

using kilo = std::ratio<static_cast<std::intmax_t>(1024), 1>;
using mega = std::ratio<static_cast<std::intmax_t>(1024) * 1024, 1>;
using giga = std::ratio<static_cast<std::intmax_t>(1024) * 1024 * 1024, 1>;
using tera = std::ratio<static_cast<std::intmax_t>(1024) * 1024 * 1024 * 1024, 1>;

using bytes = Space<uint64_t>;
using kilobytes = Space<uint64_t, kilo>;
using megabytes = Space<uint64_t, mega>;
using gigabytes = Space<uint64_t, giga>;
using terabytes = Space<uint64_t, tera>;
}  // namespace space

#endif
