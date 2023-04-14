#pragma once
#include <ext/codecvt_conv/wchar_cvt.hpp>
#include <ext/log/logger.hpp>
#include <log4cplus/logger.h>

namespace ext::log::log4cplus
{
	inline ::log4cplus::LogLevel l4_log_level(unsigned log_level)
	{
		switch (log_level)
		{
			default:
			case ext::log::Disabled:   return ::log4cplus::OFF_LOG_LEVEL;
			case ext::log::Trace:      return ::log4cplus::TRACE_LOG_LEVEL;
			case ext::log::Debug:      return ::log4cplus::DEBUG_LOG_LEVEL;
			case ext::log::Info:       return ::log4cplus::INFO_LOG_LEVEL;
			case ext::log::Warn:       return ::log4cplus::WARN_LOG_LEVEL;
			case ext::log::Error:      return ::log4cplus::ERROR_LOG_LEVEL;
			case ext::log::Fatal:      return ::log4cplus::FATAL_LOG_LEVEL;
		}
	}
	
	inline unsigned log_level(::log4cplus::LogLevel log_level)
	{
		if (log_level >= ::log4cplus::OFF_LOG_LEVEL)    return ext::log::Disabled;
		if (log_level >= ::log4cplus::FATAL_LOG_LEVEL)  return ext::log::Fatal;
		if (log_level >= ::log4cplus::ERROR_LOG_LEVEL)  return ext::log::Error;
		if (log_level >= ::log4cplus::WARN_LOG_LEVEL)   return ext::log::Warn;
		if (log_level >= ::log4cplus::INFO_LOG_LEVEL)   return ext::log::Info;
		if (log_level >= ::log4cplus::DEBUG_LOG_LEVEL)  return ext::log::Debug;
		if (log_level >= ::log4cplus::TRACE_LOG_LEVEL)  return ext::log::Trace;
		
		return ext::log::Disabled;
	}
}

namespace ext::log
{
	/// log4cplus implementation of ext::log::logger
	class log4cplus_logger : public abstract_logger
	{
		::log4cplus::Logger * m_logger = nullptr;

	protected:
		virtual auto do_log_level() const -> unsigned override;
		virtual void do_log_level(unsigned log_level) override;
		virtual bool do_is_enabled_for(unsigned log_level) const override;
		virtual void do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line) override;

	public:
		log4cplus_logger(::log4cplus::Logger & logger)
		    : m_logger(&logger) {}
	};

	inline auto log4cplus_logger::do_log_level() const -> unsigned
	{
		return log4cplus::log_level(m_logger->getLogLevel());
	}
	
	inline void log4cplus_logger::do_log_level(unsigned log_level)
	{
		m_logger->setLogLevel(log4cplus::l4_log_level(log_level));
	}
	
	inline bool log4cplus_logger::do_is_enabled_for(unsigned log_level) const
	{
		return log_level != Disabled and m_logger->isEnabledFor(log4cplus::l4_log_level(log_level));
	}

	inline void log4cplus_logger::do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line)
	{
#if BOOST_OS_WINDOWS and defined (UNICODE)
		auto wstr = ext::codecvt_convert::wchar_cvt::to_wchar(log_str);
		m_logger->log(log4cplus::l4_log_level(log_level), wstr, source_file, source_line);
#else
		m_logger->log(log4cplus::l4_log_level(log_level), log_str, source_file, source_line);
#endif
	}


	/// log4cplus implementation of ext::log::logger for simple sequenced cases
	class sequenced_log4cplus_logger : public abstract_seq_logger
	{
		::log4cplus::Logger * m_logger = nullptr;

	protected:
		virtual auto do_log_level() const -> unsigned override;
		virtual void do_log_level(unsigned log_level) override;
		virtual bool do_is_enabled_for(unsigned log_level) const override;
		virtual void do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line) override;

	public:
		sequenced_log4cplus_logger(::log4cplus::Logger & logger)
		    : m_logger(&logger) {}
	};

	inline auto sequenced_log4cplus_logger::do_log_level() const -> unsigned
	{
		return log4cplus::log_level(m_logger->getLogLevel());
	}
	
	inline void sequenced_log4cplus_logger::do_log_level(unsigned log_level)
	{
		m_logger->setLogLevel(log4cplus::l4_log_level(log_level));
	}
	
	inline bool sequenced_log4cplus_logger::do_is_enabled_for(unsigned log_level) const
	{
		return log_level != Disabled and m_logger->isEnabledFor(log4cplus::l4_log_level(log_level));
	}

	inline void sequenced_log4cplus_logger::do_log(unsigned log_level, const std::string & log_str, const char * source_file, int source_line)
	{
#if BOOST_OS_WINDOWS and defined (UNICODE)
		auto wstr = ext::codecvt_convert::wchar_cvt::to_wchar(log_str);
		m_logger->log(log4cplus::l4_log_level(log_level), wstr, source_file, source_line);
#else
		m_logger->log(log4cplus::l4_log_level(log_level), log_str, source_file, source_line);
#endif
	}
}
