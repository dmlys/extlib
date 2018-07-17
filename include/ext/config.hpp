#pragma once
#include <boost/predef.h>
#include <boost/config.hpp>

/// EXT_UNREACHABLE - marks code as unreachable, allowing optimizations from compiler
///                   __assume(0) / __builtin_unreachable()
/// 
/// при применении в switch
/// перед default: EXT_UNREACHABLE - нужно не забывать ставить break,
/// иначе возможны проблемы, unreachable код не должен выполняться.
/// или как вариант можно ставить default в начале switch.

#ifndef EXT_UNREACHABLE

#if BOOST_COMP_MSVC
#define EXT_UNREACHABLE() __assume(0)
#endif // BOOST_COMP_MSVC


#if BOOST_COMP_GNUC >= BOOST_VERSION_NUMBER(4,5,0)
#define EXT_UNREACHABLE() __builtin_unreachable()
#endif // BOOST_COMP_GNUC >= BOOST_VERSION_NUMBER(4,5,0)


#if BOOST_COMP_CLANG
#if __has_builtin(__builtin_unreachable)
#define EXT_UNREACHABLE() __builtin_unreachable()
#endif // __has_builtin(__builtin_unreachable)
#endif // BOOST_COMP_CLANG

#ifndef EXT_UNREACHABLE
#define EXT_UNREACHABLE() (void(0))
#endif // EXT_UNREACHABLE

#endif // ifndef EXT_UNREACHABLE


#define EXT_NODISCARD [[nodiscard]]
#define EXT_NORETURN  [[noreturn]]
