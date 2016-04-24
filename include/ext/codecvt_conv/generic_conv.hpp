#pragma once
#include <string>
#include <utility>
#include <ext/codecvt_conv/base.hpp>
#include <ext/range.hpp>

///Здесь содержатся функции to_bytes/from_bytes и вспомогательные функции для их реализации
///данные функции выполняют то же что и std::wstring_convert, но с меньшими ограничениями, и более обощенны
///названия следуют принятым в wstring_convert
///
///Отличия/возможности
/// - методы не владеют codecvt в отличии от wstring_convert,
///   поэтому с ними можно использовать codecvt из locale
///
/// - методы работают со всеми контейнерами поддерживающими свойство contiguously,
///   т.е. элементы расположены по соседству в памяти.
///   Определяется это с помощью ext::data. Смотри его для более подробного определения
///
/// - поддерживаются как контейнеры динамически изменяющие размеры(std::vector), так и не меняющие(std::array)
///   определяется наличием метода resize.
///   Если способен менять - размер будет выставлен в получившийся в результате преобразования
///   иначе будет добавлен 0 в конце, если для него есть место(если места нету размер результата равен размеру контейнера)
///
/// - поддерживает любые строки, wstring, u16string, c любыми char_traits, allocator

namespace ext
{
	namespace codecvt_convert
	{
		namespace internal
		{
			namespace detail
			{
				template <class Range>
				typename std::enable_if<has_resize_method<Range>::value>::type
					resize(Range & rng, std::size_t newSize) { rng.resize(newSize); }

				template <class Range>
				typename std::enable_if<!has_resize_method<Range>::value>::type
					resize(Range & rng, std::size_t newSize) {}

				template <class OutRange>
				void set_size(OutRange & rng, std::size_t sz)
				{
					if (has_resize_method<OutRange>::value)
						resize(rng, sz);
					else {
						if (boost::size(rng) > sz) //set null terminator
							*(data(rng) + sz) = 0;
					}
				}

				template <class Range>
				void grow(Range & rng)
				{
					auto sz = boost::size(rng) * 3;
					sz = sz / 2 + sz % 2;
					resize(rng, sz);
				}

				template <class Action, class Context, class InRange, class OutRange>
				std::pair<
					typename boost::range_iterator<const InRange>::type,
					typename boost::range_iterator<OutRange>::type
				>
				convert(Action action, Context & ctx, InRange const & in, OutRange & out)
				{
					const bool resizable = ext::has_resize_method<OutRange>::value;

					ctx.state = {};
					ctx.from_beg = ext::data(in);
					ctx.from_end = ctx.from_beg + boost::size(in);

					ctx.to_beg = ext::data(out);
					ctx.to_end = ctx.to_beg + boost::size(out);

					size_t nconv = 0;

					using std::codecvt_base;
					for (;;)
					{
						action(ctx);

						switch (ctx.res) {
						case codecvt_base::ok:
							break;
						case codecvt_base::partial:
							if (ctx.is_partial_input)
								throw conversion_failure("partial input");
							break;
						case codecvt_base::noconv:
							noconv_copy(ctx);
							break;
						default:
							throw conversion_failure();
						}

						nconv += ctx.to_stopped - ctx.to_beg;

						if (ctx.from_stopped >= ctx.from_end) //no more input data
							break;
						else {
							if (!resizable)
								break;

							grow(out);
							//reinit context with respect to nconv
							ctx.to_beg = ext::data(out);
							ctx.to_end = ctx.to_beg + boost::size(out);
							ctx.to_beg += nconv;
							ctx.from_beg = ctx.from_stopped;
						}
					}

					set_size(out, nconv);
					auto from_stopped_pos = ctx.from_stopped - ext::data(in);
					auto to_stopped_pos = ctx.to_stopped - ext::data(out);

					return {
						boost::begin(in) + from_stopped_pos,
						boost::begin(out) + to_stopped_pos
					};
				}
			} //namespace detail

			///converts InRange to OutRange with codecvt.out
			template <class CodeCvt, class InRange, class OutRange>
			std::pair<
				typename boost::range_iterator<const InRange>::type,
				typename boost::range_iterator<OutRange>::type
			>
			to_bytes(CodeCvt const & cvt, InRange const & in, OutRange & out)
			{
				const bool resizable = ext::has_resize_method<OutRange>::value;
				using namespace detail;
				if (boost::empty(in)) {
					set_size(out, 0);
					return {boost::begin(in), boost::begin(out)};
				}

				auto enc = cvt.encoding();
				BOOST_ASSERT(enc >= 0);// -1 state-dependent encodings пока не поддерживаем
				if (boost::empty(out))
				{
					if (!resizable)
						return {boost::begin(in), boost::begin(out)};

					//set initial size
					if (enc) //if fixed ratio
						resize(out, enc * boost::size(in));
					else
						resize(out, 2 * boost::size(in));
				}

				typedef typename make_to_bytes_context<CodeCvt>::type ctx_type;
				auto action = [&cvt](ctx_type & ctx) { to_bytes_step(cvt, ctx); };
				ctx_type ctx;
				return detail::convert(action, ctx, in, out);
			}

			///converts InRange to OutRange with codecvt.out
			template <class CodeCvt, class InRange, class OutRange>
			std::pair<
				typename boost::range_iterator<const InRange>::type,
				typename boost::range_iterator<OutRange>::type
			>
			from_bytes(CodeCvt const & cvt, InRange const & in, OutRange & out)
			{
				const bool resizable = ext::has_resize_method<OutRange>::value;
				using namespace detail;
				if (boost::empty(in)) {
					set_size(out, 0);
					return {boost::begin(in), boost::begin(out)};
				}

				auto enc = cvt.encoding();
				BOOST_ASSERT(enc >= 0);// -1 state-dependent encodings пока не поддерживаем
				if (boost::empty(out))
				{
					if (!resizable)
						return {boost::begin(in), boost::begin(out)};

					//set initial size
					if (enc == 0)
						resize(out, 2 * boost::size(in));
					else //if fixed ratio
					{
						auto sz = boost::size(in);
						resize(out, sz / enc + (sz % enc > 0)); //remainder probably is a sign of bad input data
					}
				}

				typedef typename make_from_bytes_context<CodeCvt>::type ctx_type;
				auto action = [&cvt](ctx_type & ctx) { from_bytes_step(cvt, ctx); };
				ctx_type ctx;
				return detail::convert(action, ctx, in, out);
			}
		} //namespace internal

		///converts InRange to OutRange with codecvt.out
		/// returns pair of:
		///   input range iterator where algorithm stopped
		///   output range iterator where algorithm stopped
		template <class CodeCvt, class InRange, class OutRange>
		std::pair<
			typename boost::range_iterator<const InRange>::type,
			typename boost::range_iterator<OutRange>::type
		>
		to_bytes(CodeCvt const & cvt, InRange const & in, OutRange & out)
		{
			typedef typename CodeCvt::intern_type intern_type;
			typedef typename CodeCvt::extern_type extern_type;
			static_assert(std::is_same<intern_type, typename boost::range_value<InRange>::type>::value, "Type mismatch");
			static_assert(std::is_same<extern_type, typename boost::range_value<OutRange>::type>::value, "Type mismatch");

			return internal::to_bytes(cvt, in, out);
		}

		///converts InRange to OutRange with codecvt.in
		/// returns pair of:
		///   input range iterator where algorithm stopped
		///   output range iterator where algorithm stopped
		template <class CodeCvt, class InRange, class OutRange>
		std::pair<
			typename boost::range_iterator<const InRange>::type,
			typename boost::range_iterator<OutRange>::type
		>
		from_bytes(CodeCvt const & cvt, InRange const & in, OutRange & out)
		{
			typedef typename CodeCvt::intern_type intern_type;
			typedef typename CodeCvt::extern_type extern_type;
			static_assert(std::is_same<extern_type, typename boost::range_value<InRange>::type>::value, "Type mismatch");
			static_assert(std::is_same<intern_type, typename boost::range_value<OutRange>::type>::value, "Type mismatch");

			return internal::from_bytes(cvt, in, out);
		}

		///return форма. Возвращает std::basic_string<CodeCvt::intern_type>
		///для выбора другого контейнера используйте from_bytes(cvt, in, out)
		template <class CodeCvt, class InRange>
		auto from_bytes(CodeCvt const & cvt, InRange const & in) -> std::basic_string<typename CodeCvt::intern_type>
		{
			std::basic_string<typename CodeCvt::intern_type> out;
			from_bytes(cvt, in, out);
			return out;
		}

		///return форма. Возвращает std::basic_string<CodeCvt::extern_type>
		///для выбора другого контейнера используйте to_bytes(cvt, in, out)
		template <class CodeCvt, class InRange>
		auto to_bytes(CodeCvt const & cvt, InRange const & in) -> std::basic_string<typename CodeCvt::extern_type>
		{
			std::basic_string<typename CodeCvt::extern_type> out;
			to_bytes(cvt, in, out);
			return out;
		}
	} //namespace codecvt_convert
} //namespace ext

