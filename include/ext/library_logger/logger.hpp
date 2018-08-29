#pragma once
#include <cassert>
#include <memory>
#include <utility>
#include <iostream>
#include <sstream>

namespace ext { namespace library_logger
{
	/*
		WARN: Experimental
		rationale:
			некоторым библиотекам требуется возможность логирования.
			  * они могут использовать стороннюю библиотеку,
			    это решение имеет недостаток предоставление зависимости от этой библиотеки и ее методов конфигурации.
			
			  * использовать абстрактный интерфейс, который должен реализовать пользователь библиотеки.
			    в самом простом случае имеющий метод
			    log(int log_lvl, std::string msg, int souce_line, const char * soure_file).
			
			    данное решение имеет недостаток в том что, _каждая_ библиотека должна реализовать
			    несколько вспомогательных функций/макросов для адекватного использования этого интерфейса
			    а так же то, что данное решение не очень эффективно, поскольку, наверняка,
			    на каждую запись будет создаваться stringstream,
			    даже если, в конечном итоге мы всего лишь напрямую пишем в std::clog/std::ofstream
			    или используется библиотеки предоставляющая свой ostream, например boost.log
			
			library_logger::logger предоставляет интерфейс и вспомогательные макросы позволяющие
			эффективно и безопасно логировать в абстрактный ostream с поддержкой log_level, source_file, source_line.
			реализации logger могут быть как thread-safe, так и нет
	*/

	/// базовый интерфейс логирования, для использования клиентами - библиотеками,
	/// реализуется пользователями библиотек.
	///
	/// клиентская часть:
	///   смотри так же макросы в ext/library_logger/logging_macros.h
	///   шаблон использования:
	///   record rec = logger->open_record(log_level);
	///   if (rec) {
	///       rec.get_ostream() << ....;
	///       rec.push();
	///   }
	///   record - класс записи позволяет получить поток в который можно писать.
	///            запись можно протолкнуть (push) или отбросить(discard), после чего запись становится недействительной
	///            при разрушении запись отбрасывается, если не протолкнута
	///
	///   проталкивание записи вызывается при нормальном ходе логирования
	///   отбрасывание подразумевает что в ходе формирование записи произошла ошибка, например bad_alloc
	///   реализация вольна все равно залогировать такую запись, но она может быть не полной, или даже пустой
	///   open_record может вернуть null запись - это флаг что увроень лоогирования ниже запрошенного
	///
	///   open_record - открывает запись для заданного уровня логирования.
	///   push_record/rec.push() - проталкивает запись
	///   discard_record/rec.discard() - отбрасывает запись
	///
	///
	///
	/// часть реализации:
	///   record_context - контекст записи логирования
	///                    тут хранится указатель на поток логирования,
	///                    классы наследники могут расширить данный контекст путем наследования
	///                    и хранить в нем дополнительные данные нужные им
	///   do_open_record - возвращает указатель на record_context,
	///                    контекст может создаваться каждый раз новый или повторно использоваться
	///                    гарантируется что на каждый вызов do_open_record, вернувший не nullptr
	///                    будет вызов push_record или discard_record(но не оба).
	///                    если уровень логирования ниже запрошенного логгер может вернуть nullptr
	///   do_discard_record - реализация отбрасывания записи, тут может быть освобождение ресурсов
	///                       допустимо все равно залогировать запись - на усмотрение реализации
	///   do_push_record - добавление записи в лог, а так же освобождение ресурсов
	class logger
	{
	public:
		class record;

		record open_record(int log_level, const char * source_file = nullptr, int source_line = -1);
		void push_record(record & rec);
		void discard_record(record & rec);

		virtual ~logger() = default;

	protected:
		struct record_context
		{
			std::ostream * os;
		};

		virtual record_context * do_open_record(int log_level, const char * source_file, int source_line) = 0;
		virtual void do_push_record(record_context * rctx) = 0;
		virtual void do_discard_record(record_context * rctx) = 0;
	};

	/************************************************************************/
	/*   RAII safe record class.                                            */
	/*   представляет запись в которую можно добавлять данные логирования   */
	/************************************************************************/
	class logger::record
	{
		friend logger;
		
		record_context * rctx = nullptr;
		logger * owner = nullptr;

	private:
		record(record_context * rctx, logger * owner) : rctx(rctx), owner(owner) {}

	public:
		record() = default;
		record(record && rec) noexcept : rctx(rec.rctx), owner(rec.owner)  { rec.rctx = nullptr; rec.owner = nullptr; }
		record & operator =(record && rec) noexcept                        { rctx = std::exchange(rec.rctx, nullptr); owner = std::exchange(rec.owner, nullptr); return *this; }
		~record() noexcept                                                 { if (owner) owner->discard_record(*this); }

		//noncopyable
		record(record const & rec) = delete;
		record & operator =(record const & rec) = delete;

		/// проверяет валидность записи(null запись/активная)
		explicit operator bool() { return rctx != nullptr; }
		std::ostream & get_ostream();
		/// аналогично logger->push_record(*this)
		void push() { owner->push_record(*this); }
		/// аналогично logger->discard_record(*this)
		void discard() { owner->discard_record(*this);  }
	};

	inline std::ostream & logger::record::get_ostream()
	{
		assert(rctx);
		return *rctx->os;
	}

	/************************************************************************/
	/*                  Logger methods                                      */
	/************************************************************************/
	inline logger::record logger::open_record(int log_level, const char * source_file, int source_line)
	{
		auto ctx = do_open_record(log_level, source_file, source_line);
		return {ctx, this};
	}

	inline void logger::push_record(record & rec)
	{
		auto rctx = std::exchange(rec.rctx, nullptr);
		if (rctx) do_push_record(rctx);
	}

	inline void logger::discard_record(record & rec)
	{
		auto rctx = std::exchange(rec.rctx, nullptr);
		if (rctx) do_discard_record(rec.rctx);
	}

	/************************************************************************/
	/*                   convenience helpers                                */
	/************************************************************************/
	inline logger::record open_record(logger * lg, int log_level, const char * source_file, int source_line)
	{
		return lg ? lg->open_record(log_level, source_file, source_line) : logger::record();
	}

	inline logger::record open_record(logger & lg, int log_level, const char * source_file, int source_line)
	{
		return lg.open_record(log_level, source_file, source_line);
	}

	/// известные уровни логирования
	const int Fatal = 0;
	const int Error = 1;
	const int Warn = 2;
	const int Info = 3;
	const int Debug = 4;
	const int Trace = 5;







	/************************************************************************/
	/*                some hepler/basic implementations                     */
	/************************************************************************/
	/// simple logger for some iostream:
	///   cout, cerr, clog or any other arbitrary ostream.
	/// Thread safety - same as argument ostream
	class stream_logger : public ext::library_logger::logger
	{
		record_context ctx;
		const int lvl;

	public:
		stream_logger(std::ostream & os = std::clog, int lvl = Info)
			: lvl(lvl) { ctx.os = &os; }
		~stream_logger() { ctx.os->flush(); }

	public:
		record_context * do_open_record(int log_level, const char * source_file, int source_line) override
		{
			return &ctx;
		}

		void do_push_record(record_context * rctx) override { *rctx->os << std::endl; }
		void do_discard_record(record_context * rctx) override {}
	};

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
		simple_logger(std::locale loc = std::locale()) : loc(loc) {}

	private:
		record_context * do_open_record(int log_level, const char * source_file, int source_line) override final;
		void do_push_record(base_record_context * rctx) override final;
		void do_discard_record(base_record_context * rctx) override final;

	private:
		/// включен ли логгер для заданного уровня логирования
		virtual bool do_is_enabled_for(int log_level) = 0;
		/// реализация собственно логирования
		virtual void do_log(int log_level, std::string const & logStr, const char * source_file, int source_line) = 0;
	};


	inline simple_logger::record_context * simple_logger::do_open_record(int log_level, const char * source_file, int source_line)
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
		record_context * do_open_record(int log_level, const char * source_file, int source_line) override final;
		void do_push_record(record_context * rctx) override final;
		void do_discard_record(record_context * rctx) override final;

	private:
		/// включен ли логгер для заданного уровня логирования
		virtual bool do_is_enabled_for(int log_level) = 0;
		/// реализация собственно логирования
		virtual void do_log(int log_level, std::string const & logStr, const char * source_file, int source_line) = 0;
	};


	inline sequenced_simple_logger::record_context * sequenced_simple_logger::do_open_record(int log_level, const char * source_file, int source_line)
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
