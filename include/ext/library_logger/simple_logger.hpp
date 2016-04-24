#pragma once
#include <ext/library_logger/logger.hpp>
#include <memory>
#include <sstream>

namespace ext { namespace library_logger {

	/// simple_logger/sequenced_simple_logger
	/// упрощение интерфейса к более простому
	/// is_enabled_for(log_level), log(log_level, logStr)
	/// например таким интерфейсом обладает log4cplus

	/// наследник должен реализовать is_enabled_for/log методы
	class simple_logger : public logger
	{
		typedef ext::library_logger::logger::record_context base_record_context;
		struct record_context : base_record_context
		{
			std::ostringstream ss;
			int log_level;
			int source_line;
			const char * source_file;
		};
		std::locale loc;
	public:
		simple_logger(std::locale loc = std::locale())
			: loc(loc) {}
	private:
		record_context * do_open_record(int log_level, const char * source_file, int source_line) final;
		void do_push_record(base_record_context * rctx) final;
		void do_discard_record(base_record_context * rctx) final;

	private:
		/// включен ли логгер для заданного уровня логирования
		virtual bool do_is_enabled_for(int log_level) = 0;
		/// реализация собственно логирования
		virtual void do_log(int log_level, std::string const & logStr, const char * source_file, int source_line) = 0;
	};


	inline
	simple_logger::record_context * simple_logger::do_open_record(int log_level, const char * source_file, int source_line)
	{
		if (!do_is_enabled_for(log_level))
			return nullptr;

		auto rctx = new record_context;
		rctx->log_level = log_level;
		rctx->source_line = source_line;
		rctx->source_file = source_file;
		rctx->ss.imbue(loc);
		rctx->os = &rctx->ss;
		return rctx;
	}

	inline void simple_logger::do_push_record(base_record_context * rctx_base)
	{
		std::unique_ptr<record_context> rctx(static_cast<record_context *>(rctx_base));
		do_log(rctx->log_level, rctx->ss.str(), rctx->source_file, rctx->source_line);
	}

	inline void simple_logger::do_discard_record(base_record_context * rctx_base)
	{
		delete static_cast<record_context *>(rctx_base);
	}

	/// упорядоченная sequenced (seq) версия логгера,
	/// вызовы должны быть строго упорядоченны по парам open_record -> соответствующий ему push_record/discard_record
	/// соответственно логгер должен использоваться только из одного потока, или же гарантии вызовов должны обеспечиваться
	/// механизмами синхронизации.
	/// пользователи logger интерфейса могут предоставлять такие гарантии, тогда можно использовать данный интерфейс
	class sequenced_simple_logger : public logger
	{
		std::ostringstream ss;
		record_context rctx;
		int log_level;
		const char * source_file;
		int source_line;

	public:
		sequenced_simple_logger(std::locale const & loc = std::locale())
		{
			ss.imbue(loc);
			rctx.os = &ss;
		}
	
	private:
		record_context * do_open_record(int log_level, const char * source_file, int source_line) final;
		void do_push_record(record_context * rctx) final;
		void do_discard_record(record_context * rctx) final;

	private:
		/// включен ли логгер для заданного уровня логирования
		virtual bool do_is_enabled_for(int log_level) = 0;
		/// реализация собственно логирования
		virtual void do_log(int log_level, std::string const & logStr, const char * source_file, int source_line) = 0;
	};


	inline
	sequenced_simple_logger::record_context * sequenced_simple_logger::do_open_record(int log_level, const char * source_file, int source_line)
	{
		if (!do_is_enabled_for(log_level))
			return nullptr;

		this->log_level = log_level;
		this->source_file = source_file;
		this->source_line = source_line;
		
		return &rctx;
	}

	inline void sequenced_simple_logger::do_push_record(record_context * rctx)
	{
		do_log(log_level, ss.str(), source_file, source_line);
		do_discard_record(rctx);
	}

	inline void sequenced_simple_logger::do_discard_record(record_context * rctx)
	{
		ss.str("");
		ss.clear();
	}
}}