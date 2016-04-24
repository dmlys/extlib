#pragma once
#include <ext/library_logger/logger.hpp>

/// можно выключать некоторые макросы логирования
#if defined(EXTLL_DISABLE_ALL) && !defined(EXTLL_DISABLE_FATAL)
#define EXTLL_DISABLE_FATAL
#endif

#if defined(EXTLL_DISABLE_FATAL) && !defined(EXTLL_DISABLE_ERROR)
#define EXTLL_DISABLE_ERROR
#endif
#if defined(EXTLL_DISABLE_ERROR) && !defined(EXTLL_DISABLE_WARN)
#define EXTLL_DISABLE_WARN
#endif
#if defined(EXTLL_DISABLE_WARN) && !defined(EXTLL_DISABLE_INFO)
#define EXTLL_DISABLE_INFO
#endif
#if defined(EXTLL_DISABLE_INFO) && !defined(EXTLL_DISABLE_DEBUG)
#define EXTLL_DISABLE_DEBUG
#endif
#if defined(EXTLL_DISABLE_DEBUG) && !defined(EXTLL_DISABLE_TRACE)
#define EXTLL_DISABLE_TRACE
#endif

namespace ext {
namespace library_logger
{
	inline logger * get_logger(logger * lg) { return lg; }
	inline logger * get_logger(logger & lg) { return &lg; }
	inline logger * get_logger(logger && lg) = delete;
}}

#if defined(_MSC_VER)
#define EXTLL_SUPPRESS_DOWHILE_WARNING()  \
    __pragma (warning (push))             \
    __pragma (warning (disable:4127))

#define EXTLL_RESTORE_DOWHILE_WARNING()   \
    __pragma (warning (pop))

#else
#define EXTLL_SUPPRESS_DOWHILE_WARNING() /* empty */
#define EXTLL_RESTORE_DOWHILE_WARNING()  /* empty */

#endif

#ifndef EXTLL_DISABLE_ALL
#define EXTLL_LOG(lg, log_level, expr)                                                  \
EXTLL_SUPPRESS_DOWHILE_WARNING()                                                        \
do {                                                                                    \
	ext::library_logger::logger * ll = get_logger(lg);                                  \
	if (ll) {                                                                           \
		auto rec = ll->open_record(log_level, __FILE__, __LINE__);                      \
		if (rec)                                                                        \
		{                                                                               \
			rec.get_ostream() << expr;                                                  \
			rec.push();                                                                 \
		}                                                                               \
	}                                                                                   \
} while(0)                                                                              \
EXTLL_RESTORE_DOWHILE_WARNING()
#else
#define EXTLL_LOG(lg, log_level, expr) ((void)0)
#endif


#ifndef EXTLL_DISABLE_FATAL
#define EXTLL_FATAL(lg, expr) EXTLL_LOG(lg, ext::library_logger::Fatal, expr)
#else
#define EXTLL_FATAL(lg, expr) ((void)0)
#endif

#ifndef EXTLL_DISABLE_ERROR
#define EXTLL_ERROR(lg, expr) EXTLL_LOG(lg, ext::library_logger::Error, expr)
#else
#define EXTLL_ERROR(lg, expr) ((void)0)
#endif

#ifndef EXTLL_DISABLE_WARN
#define EXTLL_WARN(lg, expr) EXTLL_LOG(lg, ext::library_logger::Warn, expr)
#else
#define EXTLL_WARN(lg, expr) ((void)0)
#endif

#ifndef EXTLL_DISABLE_INFO
#define EXTLL_INFO(lg, expr) EXTLL_LOG(lg, ext::library_logger::Info, expr)
#else
#define EXTLL_INFO(lg, expr) ((void)0)
#endif

#ifndef EXTLL_DISABLE_DEBUG
#define EXTLL_DEBUG(lg, expr) EXTLL_LOG(lg, ext::library_logger::Debug, expr)
#else
#define EXTLL_DEBUG(lg, expr) ((void)0)
#endif

#ifndef EXTLL_DISABLE_TRACE
#define EXTLL_TRACE(lg, expr) EXTLL_LOG(lg, ext::library_logger::Trace, expr)
#else
#define EXTLL_TRACE(lg, expr) ((void)0)
#endif
