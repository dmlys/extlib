#pragma once
#include <ext/log/logger.hpp>

/// можно выключать некоторые макросы логирования
#if defined(EXTLOG_DISABLE_ALL) && !defined(EXTLOG_DISABLE_FATAL)
#define EXTLOG_DISABLE_FATAL
#endif

#if defined(EXTLOG_DISABLE_FATAL) && !defined(EXTLOG_DISABLE_ERROR)
#define EXTLOG_DISABLE_ERROR
#endif
#if defined(EXTLOG_DISABLE_ERROR) && !defined(EXTLOG_DISABLE_WARN)
#define EXTLOG_DISABLE_WARN
#endif
#if defined(EXTLOG_DISABLE_WARN) && !defined(EXTLOG_DISABLE_INFO)
#define EXTLOG_DISABLE_INFO
#endif
#if defined(EXTLOG_DISABLE_INFO) && !defined(EXTLOG_DISABLE_DEBUG)
#define EXTLOG_DISABLE_DEBUG
#endif
#if defined(EXTLOG_DISABLE_DEBUG) && !defined(EXTLOG_DISABLE_TRACE)
#define EXTLOG_DISABLE_TRACE
#endif

namespace ext::log
{
	inline logger * get_logger(logger * lg) { return lg; }
	inline logger * get_logger(logger & lg) { return &lg; }
	inline logger * get_logger(logger && lg) = delete;

	//inline const logger * get_logger(const logger * lg) { return lg; }
	//inline const logger * get_logger(const logger & lg) { return &lg; }
	//inline const logger * get_logger(const logger && lg) = delete;

	// smart pointer overload
	template <class Pointer> inline auto get_logger(Pointer & ptr) { return ptr.get(); }
}

#if defined(_MSC_VER)
#define EXTLOG_SUPPRESS_DOWHILE_WARNING()  \
    __pragma (warning (push))             \
    __pragma (warning (disable:4127))

#define EXTLOG_RESTORE_DOWHILE_WARNING()   \
    __pragma (warning (pop))

#else
#define EXTLOG_SUPPRESS_DOWHILE_WARNING() /* empty */
#define EXTLOG_RESTORE_DOWHILE_WARNING()  /* empty */

#endif

#ifndef EXTLOG_DISABLE_ALL
#define EXTLOG_LOG(lg, log_level, expr)                                                  \
EXTLOG_SUPPRESS_DOWHILE_WARNING()                                                        \
do {                                                                                    \
    ext::log::logger * ll = ext::log::get_logger(lg);             \
    if (ll)                                                                             \
    {                                                                                   \
        auto rec = ll->open_record(log_level, __FILE__, __LINE__);                      \
        if (rec)                                                                        \
        {                                                                               \
            rec.get_ostream() << expr;                                                  \
            rec.push();                                                                 \
        }                                                                               \
    }                                                                                   \
} while(0)                                                                              \
EXTLOG_RESTORE_DOWHILE_WARNING()

#define EXTLOG_LOG_STR(lg, log_level, str)                                               \
EXTLOG_SUPPRESS_DOWHILE_WARNING()                                                        \
do {                                                                                    \
    ext::log::logger * ll = ext::log::get_logger(lg);             \
    if (ll and ll->is_enabled_for(log_level))                                           \
    {                                                                                   \
        ll->log(log_level, str, __FILE__, __LINE__);                                    \
    }                                                                                   \
} while(0)                                                                              \
EXTLOG_RESTORE_DOWHILE_WARNING()

#else

#define EXTLOG_LOG(lg, log_level, expr) ((void)0)
#define EXTLOG_LOG_STR(lg, log_level, expr) ((void)0)

#endif


#ifndef EXTLOG_DISABLE_FATAL
#define EXTLOG_FATAL(lg, expr) EXTLOG_LOG(lg, ext::log::Fatal, expr)
#define EXTLOG_FATAL_STR(lg, str) EXTLOG_LOG_STR(lg, ext::log::Fatal, str)
#else
#define EXTLOG_FATAL(lg, expr) ((void)0)
#define EXTLOG_FATAL_STR(lg, str) ((void)0)
#endif

#ifndef EXTLOG_DISABLE_ERROR
#define EXTLOG_ERROR(lg, expr) EXTLOG_LOG(lg, ext::log::Error, expr)
#define EXTLOG_ERROR_STR(lg, str) EXTLOG_LOG_STR(lg, ext::log::Error, str)
#else
#define EXTLOG_ERROR(lg, expr) ((void)0)
#define EXTLOG_ERROR_STR(lg, str) ((void)0)
#endif

#ifndef EXTLOG_DISABLE_WARN
#define EXTLOG_WARN(lg, expr) EXTLOG_LOG(lg, ext::log::Warn, expr)
#define EXTLOG_WARN_STR(lg, str) EXTLOG_LOG_STR(lg, ext::log::Warn, str)
#else
#define EXTLOG_WARN(lg, expr) ((void)0)
#define EXTLOG_WARN_STR(lg, str) ((void)0)
#endif

#ifndef EXTLOG_DISABLE_INFO
#define EXTLOG_INFO(lg, expr) EXTLOG_LOG(lg, ext::log::Info, expr)
#define EXTLOG_INFO_STR(lg, str) EXTLOG_LOG_STR(lg, ext::log::Info, str)
#else
#define EXTLOG_INFO(lg, expr) ((void)0)
#define EXTLOG_INFO_STR(lg, str) ((void)0)
#endif

#ifndef EXTLOG_DISABLE_DEBUG
#define EXTLOG_DEBUG(lg, expr) EXTLOG_LOG(lg, ext::log::Debug, expr)
#define EXTLOG_DEBUG_STR(lg, str) EXTLOG_LOG_STR(lg, ext::log::Debug, str)
#else
#define EXTLOG_DEBUG(lg, expr) ((void)0)
#define EXTLOG_DEBUG_STR(lg, str) ((void)0)
#endif

#ifndef EXTLOG_DISABLE_TRACE
#define EXTLOG_TRACE(lg, expr) EXTLOG_LOG(lg, ext::log::Trace, expr)
#define EXTLOG_TRACE_STR(lg, str) EXTLOG_LOG_STR(lg, ext::log::Trace, str)
#else
#define EXTLOG_TRACE(lg, expr) ((void)0)
#define EXTLOG_TRACE_STR(lg, str) ((void)0)
#endif



/// addition convenience macros for direct libfmt usage, NOTE this file does not include anything from fmt
/// EXTLOG_INFO_FMT(lg, "Hello {}", "World") same as EXTLOG_INFO(lg, fmt::format("Hello {}", "Worlds"))

#define EXTLOG_LOG_FMT(lg, log_level, ...)   EXTLOG_LOG_STR(lg, log_level, fmt::format(__VA_ARGS__))
#define EXTLOG_FATAL_FMT(lg, ...)            EXTLOG_FATAL_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLOG_ERROR_FMT(lg, ...)            EXTLOG_ERROR_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLOG_WARN_FMT(lg, ...)             EXTLOG_WARN_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLOG_INFO_FMT(lg, ...)             EXTLOG_INFO_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLOG_DEBUG_FMT(lg, ...)            EXTLOG_DEBUG_STR(lg, fmt::format(__VA_ARGS__))
#define EXTLOG_TRACE_FMT(lg, ...)            EXTLOG_TRACE_STR(lg, fmt::format(__VA_ARGS__))
