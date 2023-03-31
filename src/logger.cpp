#include <ext/log/logger.hpp>
#include <ext/strings/aci_string.hpp>

namespace ext::log
{
	unsigned parse_log_level(std::string_view word)
	{
		unsigned log_level = Disabled;
		parse_log_level(word, log_level);
		return log_level;
	}
	
	bool parse_log_level(std::string_view word, unsigned & log_level)
	{
		ext::aci_string_view aci_word(word.data(), word.length());

		if      (aci_word == "fatal") return log_level = Fatal, true;
		else if (aci_word == "error") return log_level = Error, true;
		else if (aci_word == "warn" ) return log_level = Warn,  true;
		else if (aci_word == "info" ) return log_level = Info,  true;
		else if (aci_word == "debug") return log_level = Debug, true;
		else if (aci_word == "trace") return log_level = Trace, true;
		else if (aci_word == "disabled") return log_level = Disabled, true;
		else                             return false;
	}
	
	std::string_view log_level_string(unsigned log_level)
	{
		switch (log_level)
		{
		    case Trace:    return "Trace";
		    case Debug:    return "Debug";
		    case Info:     return "Info";
		    case Warn:     return "Warn";
		    case Error:    return "Error";
		    case Fatal:    return "Fatal";

		    case Disabled: return "Disabled";
		    default:       return "Unknown";
		}
	}
	
	
	
	unsigned ostream_logger::do_log_level() const
	{
		return lvl;
	}
	
	void ostream_logger::do_log_level(unsigned log_level)
	{
		lvl = log_level;
	}
	
	bool ostream_logger::do_is_enabled_for(unsigned log_level) const
	{
		return log_level <= lvl; 
	}
	
	void ostream_logger::do_log(unsigned log_level, const std::string & str, const char * source_file, int source_line)
	{
		*ctx.os << str << std::endl;
	}
	
	auto ostream_logger::do_open_record(unsigned log_level, const char * source_file, int source_line) -> record_context *
	{
		return log_level <= lvl ? &ctx : nullptr;
	}
	
	void ostream_logger::do_push_record(record_context * rctx)
	{
		*rctx->os << std::endl;
	}
	
	void ostream_logger::do_discard_record(record_context * rctx) noexcept
	{
		
	}
	
	ostream_logger::~ostream_logger() noexcept
	{
		try
		{
			ctx.os->flush();
		}
		catch (...)
		{
			// nothing
		}
	}
	
	auto abstract_logger::do_open_record(unsigned log_level, const char * source_file, int source_line) -> record_context *
	{
		if (not do_is_enabled_for(log_level))
			return nullptr;

		auto rctx = new record_context;
		rctx->log_level = log_level;
		rctx->source_line = source_line;
		rctx->source_file = source_file;
		rctx->stream.imbue(loc);
		rctx->os = &rctx->stream;
		return rctx;
	}

	void abstract_logger::do_push_record(base_record_context * rctx_base)
	{
		std::unique_ptr<record_context> rctx(static_cast<record_context *>(rctx_base));
		do_log(rctx->log_level, rctx->stream.str(), rctx->source_file, rctx->source_line);
	}

	void abstract_logger::do_discard_record(base_record_context * rctx_base) noexcept
	{
		delete static_cast<record_context *>(rctx_base);
	}

	abstract_seq_logger::abstract_seq_logger(const std::locale & loc)
	{
		stream.imbue(loc);
		rctx.os = &stream;
	}

	auto abstract_seq_logger::do_open_record(unsigned log_level, const char * source_file, int source_line) -> record_context *
	{
		if (!do_is_enabled_for(log_level))
			return nullptr;

		this->log_level = log_level;
		this->source_file = source_file;
		this->source_line = source_line;
		
		return &rctx;
	}

	void abstract_seq_logger::do_push_record(record_context * rctx)
	{
		do_log(log_level, stream.str(), source_file, source_line);
		do_discard_record(rctx);
	}

	void abstract_seq_logger::do_discard_record(record_context * rctx) noexcept
	{
		stream.str(std::string());
		stream.clear();
	}
	
}
