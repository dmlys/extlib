#pragma once
#include <cassert>
#include <ostream>
#include <utility>

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
}}