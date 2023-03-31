#pragma once
#include <cassert>
#include <memory>  // for std::unique_ptr
#include <utility> // for std::exchange
#include <string>
#include <string_view>
#include <sstream>

namespace ext::log
{
	/// well-known log levels
	constexpr unsigned Fatal = 0;
	constexpr unsigned Error = 1;
	constexpr unsigned Warn  = 2;
	constexpr unsigned Info  = 3;
	constexpr unsigned Debug = 4;
	constexpr unsigned Trace = 5;

	/// explicitly disabled, not matter what configuration states
	constexpr unsigned Disabled = -1; // MAX_UINT

	/// returns string representation of log_level
	std::string_view log_level_string(unsigned log_level);
	/// parses word as log_level, if parsing fails - returns Disabled
	unsigned parse_log_level(std::string_view word);
	/// parses word as log_level, if parsing fails - returns false, log_level is unchanged
	bool parse_log_level(std::string_view word, unsigned & log_level);

	// forwards
	class logger;
	class ostream_logger;
	class abstract_logger;
	class abstract_seq_logger;
	
	/// Rationale:
	/// Some libraries and other code sometimes need logging:
	///   * they can use some logging library - this imposes their choice of logging library
	///   * use some abstract interface, which must be implemented by client
	///     in most simple form:
	///        log(int log_lvl, const std::string & msg, int source_line, const char * source_file);
	///     
	/// Declare such logging interface, and provide all useful and needed helpers for logging.
	
	/// Abstract logging interface(see also logging macros in ext/log/logging_macros.hpp)
	///  Should be used as follows:
	///   if (logger->is_enabled_for(log_level)
	///      logger->log(log_level, str, __FILE__, __LINE__);
	/// 
	///   or ostream like:
	///    record rec = logger->open_record(log_level, __FILE__, __LINE__);
	///    if (rec)
	///    {
	///        rec.get_ostream() << ... ;
	///        rec.push();
	///    }
	/// 
	///  record - helper class managing lifetime of logging record ostream
	///     can pushed - record is logged, or discarded - does nothing
	///   
	///   Pushing is usually done in normal flow, discarding - some error/exception happened.
	///   Implementation is free to handle discarded records in their judging.
	/// 
	///   Record can be null(false when casted to bool) - indicates that logging level is lower than requested in open_record call
	///   
	class logger
	{
	protected:
		/// record context, holding logging ostream,
		/// can be extended by implementation to add some more needed context
		struct record_context { std::ostream * os; };
		
	public:
		/// RAII and front class for logging records, completely implemented by this interface
		class record;
		
	protected:
		/// implementation delegation for set_log_level
		virtual auto do_log_level() const -> unsigned = 0;
		/// implementation delegation for set_log_level
		virtual void do_log_level(unsigned log_level) = 0;
		/// implementation delegation for is_enabled_for
		virtual bool do_is_enabled_for(unsigned log_level) const = 0;
		/// implementation delegation for log
		virtual void do_log(unsigned log_level, const std::string & str, const char * source_file, int source_line) = 0;
		/// implementation delegation for open_record
		virtual record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) = 0;
		/// implementation delegation for push_record
		virtual void do_push_record(record_context * rctx) = 0;
		/// implementation delegation for discard_record
		virtual void do_discard_record(record_context * rctx) noexcept = 0;

	public:
		/// Gets current log level
		auto log_level() const -> unsigned;
		/// Sets new log. Allows changing log level on the fly, support is optional
		void log_level(unsigned log_level);
		/// checks if logging is enabled for given log-level
		bool is_enabled_for(unsigned log_level) const;
		/// logs given string at given log-level, also passes source_file(__FILE__) and line(__LINE__)
		void log(unsigned log_level, const std::string & str, const char * source_file, int source_line);
		/// opens logging record for given log-level
		/// if requested log-level is "low" - returns "null" record
		record open_record(unsigned log_level, const char * source_file = nullptr, int source_line = -1);
		/// pushes records
		void push_record(record & rec);
		/// discards records
		void discard_record(record & rec) noexcept;

	public:
		virtual ~logger() = default;
	};
	
	inline logger::record open_record(logger * lg, unsigned log_level, const char * source_file, int source_line);
	inline logger::record open_record(logger & lg, unsigned log_level, const char * source_file, int source_line);
	
	/************************************************************************/
	/*   RAII safe record class.                                            */
	/************************************************************************/
	class logger::record
	{
		friend logger;
		
		record_context * rctx = nullptr;
		logger * owner = nullptr;

	public:
		/// is record valid(not null record)
		explicit operator bool() const noexcept { return rctx != nullptr; }
		/// return logging ostream
		std::ostream & get_ostream();
		/// same as logger->push_record(*this)
		void push() { owner->push_record(*this); }
		/// same as logger->discard_record(*this)
		void discard() noexcept { owner->discard_record(*this);  }

	private:
		record(record_context * rctx, logger * owner) : rctx(rctx), owner(owner) {}

	public:
		record() noexcept = default;
		~record() noexcept { if (owner) owner->discard_record(*this); }

		record(record && rec) noexcept : rctx(rec.rctx), owner(rec.owner)  { rec.rctx = nullptr; rec.owner = nullptr; }
		record & operator =(record && rec) noexcept                        { rctx = std::exchange(rec.rctx, nullptr); owner = std::exchange(rec.owner, nullptr); return *this; }

		record(const record & rec) = delete;
		record & operator =(const record & rec) = delete;
	};

	inline std::ostream & logger::record::get_ostream()
	{
		assert(rctx);
		return *rctx->os;
	}
	
	/************************************************************************/
	/*                  Logger methods                                      */
	/************************************************************************/
	inline auto logger::log_level() const -> unsigned
	{
		return do_log_level();
	}
	
	inline void logger::log_level(unsigned log_level)
	{
		return do_log_level(log_level);
	}
	
	inline bool logger::is_enabled_for(unsigned log_level) const
	{
		return do_is_enabled_for(log_level);
	}

	inline void logger::log(unsigned log_level, const std::string & str, const char * source_file, int source_line)
	{
		do_log(log_level, str, source_file, source_line);
	}

	inline logger::record logger::open_record(unsigned log_level, const char * source_file, int source_line)
	{
		auto ctx = do_open_record(log_level, source_file, source_line);
		return {ctx, this};
	}

	inline void logger::push_record(record & rec)
	{
		auto rctx = std::exchange(rec.rctx, nullptr);
		if (rctx) do_push_record(rctx);
	}

	inline void logger::discard_record(record & rec) noexcept
	{
		auto rctx = std::exchange(rec.rctx, nullptr);
		if (rctx) do_discard_record(rec.rctx);
	}

	/************************************************************************/
	/*                   convenience helpers                                */
	/************************************************************************/
	inline logger::record open_record(logger * lg, unsigned log_level, const char * source_file, int source_line)
	{
		return lg ? lg->open_record(log_level, source_file, source_line) : logger::record();
	}

	inline logger::record open_record(logger & lg, unsigned log_level, const char * source_file, int source_line)
	{
		return lg.open_record(log_level, source_file, source_line);
	}
	
	/************************************************************************/
	/*                some helper/basic implementations                     */
	/************************************************************************/
	/************************************************************************/
	/*                   ostream_logger implementation                      */
	/************************************************************************/
	/// implementation of logger interface for std::ostream object:
	///   cout, cerr, clog or any other arbitrary ostream.
	/// Thread safety - same as argument ostream
	class ostream_logger : public logger
	{
	protected:
		record_context ctx;
		unsigned lvl = Info;

	protected:
		auto do_log_level() const -> unsigned override;
		void do_log_level(unsigned log_level) override;
		bool do_is_enabled_for(unsigned log_level) const override;
		void do_log(unsigned log_level, const std::string & str, const char * source_file, int source_line) override;

		record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) override;
		void do_push_record(record_context * rctx) override;
		void do_discard_record(record_context * rctx) noexcept override;

	public:
		void flush() { ctx.os->flush(); }
		auto stream() const noexcept { return ctx.os; }

	public:
		ostream_logger(std::ostream & os, int lvl = Info)
		    : lvl(lvl) { ctx.os = &os; }
		~ostream_logger() noexcept;

		ostream_logger(const ostream_logger & lg) = delete;
		ostream_logger & operator =(const ostream_logger &) = delete;
	};
	
	/************************************************************************/
	/*                  abstract_logger implementation                      */
	/************************************************************************/
	/// Helper class for implementing loggers - implements record methods(do_open_record, etc)
	/// via do_is_enabled_for and do_log methods
	/// 
	/// Derived class should implement do_log and do_is_enabled_for
	class abstract_logger : public logger
	{
	protected:
		using base_record_context = logger::record_context;
		struct record_context : base_record_context
		{
			std::ostringstream stream;
			unsigned log_level;
			unsigned source_line;
			const char * source_file;
		};
		
	protected:
		std::locale loc;
		
	protected:
		record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) override;
		void do_push_record(base_record_context * rctx) override;
		void do_discard_record(base_record_context * rctx) noexcept override;

	public:
		abstract_logger() = default;
		abstract_logger(std::locale loc) : loc(std::move(loc)) {}

		abstract_logger(const abstract_logger &) = delete;
		abstract_logger & operator =(const abstract_logger &) = delete;
	};
	

	/// Similar to abstract_logger but expects logging in sequenced way -> only one record is opened at ony time.
	/// Also logging have to be done from one thread
	/// 
	/// This allows reusing same record and allocating new one every time
	class abstract_seq_logger : public logger
	{
	protected:
		std::ostringstream stream;
		record_context rctx;
		unsigned log_level;
		unsigned source_line;
		const char * source_file;
	
	private:
		record_context * do_open_record(unsigned log_level, const char * source_file, int source_line) override;
		void do_push_record(record_context * rctx) override;
		void do_discard_record(record_context * rctx) noexcept override;

	public:
		abstract_seq_logger(const std::locale & loc = std::locale());
		
		abstract_seq_logger(const abstract_seq_logger &) = delete;
		abstract_seq_logger & operator =(const abstract_seq_logger &) = delete;
	};
}
