#pragma once
#include <stdexcept>
#include <locale>

/// Содержит базовые вспомогательные структуры/функции для работы с codecvt
namespace ext
{
	namespace codecvt_convert
	{
		/// исключение ошибок работы codecvt:
		/// codecvt_base::error, codecvt_base::partial
		struct conversion_failure : std::runtime_error
		{
			conversion_failure(const char * msg = "conversion failure") : std::runtime_error(msg) {}
		};

		namespace internal
		{
			// this struct is used with from_bytes_step/to_bytes_step
			// it groups parameters and holds state of invocation of codecvt
			template <class InputType, class OutputType, class StateType>
			struct cvt_context
			{
				typedef InputType input_type;
				typedef OutputType output_type;
				typedef StateType state_type;

				// input vars
				const input_type * from_beg;
				const input_type * from_end;

				output_type * to_beg;
				output_type * to_end;

				state_type state;

				// output vars
				std::codecvt_base::result res;
				// if res == partial, it points is it
				// truncated input(last character not full), or insufficient output buffer
				bool is_partial_input;

				const input_type * from_stopped; // where input stopped, not including
				output_type * to_stopped;        // where output stopped, not including
			};

			/// deduce types for cvt_context from CodeCvt
			template <class CodeCvt>
			struct make_to_bytes_context
			{
				typedef cvt_context<
					typename CodeCvt::intern_type, //input
					typename CodeCvt::extern_type, //output
					typename CodeCvt::state_type
				> type;
			};

			/// deduce types for cvt_context from CodeCvt
			template <class CodeCvt>
			struct make_from_bytes_context
			{
				typedef cvt_context<
					typename CodeCvt::extern_type, //input
					typename CodeCvt::intern_type, //output
					typename CodeCvt::state_type
				> type;
			};

			/// calls cvt.in from contex, if result == partial, determines input or output is partial
			template <class ... CvtTypes>
			void from_bytes_step(const std::codecvt<CvtTypes...> & cvt, typename make_from_bytes_context<std::codecvt<CvtTypes...>>::type & ctx)
			{
				ctx.res = cvt.in(ctx.state,
				                 ctx.from_beg, ctx.from_end, ctx.from_stopped,
				                 ctx.to_beg, ctx.to_end, ctx.to_stopped);

				if (ctx.res == std::codecvt_base::partial) {
					// cvt.length returns 0 on intern_char non empty buffer,
					// it can't convert from input - input is partial
					ctx.is_partial_input = cvt.length(ctx.state, ctx.from_stopped, ctx.from_end, 1) == 0;
				}
			}

			/// calls cvt.out from contex, if result == partial, determines input or output is partial
			template <class ... CvtTypes>
			void to_bytes_step(const std::codecvt<CvtTypes...> & cvt, typename make_to_bytes_context<std::codecvt<CvtTypes...>>::type & ctx)
			{
				ctx.res = cvt.out(ctx.state,
				                  ctx.from_beg, ctx.from_end, ctx.from_stopped,
				                  ctx.to_beg, ctx.to_end, ctx.to_stopped);

				if (ctx.res == std::codecvt_base::partial) {
					//output extern_char buffer contains cvt.max_length or more - input is partial
					ctx.is_partial_input = (ctx.to_stopped - ctx.to_end >= cvt.max_length());
				}
			}

			/// codecvt_base::noconv implementation
			template <class Context>
			void noconv_copy(Context & ctx)
			{
				auto tocopy = (std::min)(ctx.to_end - ctx.to_beg, ctx.from_end - ctx.from_beg);
				// to_stopped/from_stopped tracks where we stopped, we make reference shortcut to them
				auto & to_cur = ctx.to_stopped = ctx.to_beg;
				auto & from_cur = ctx.from_stopped = ctx.from_beg;

				typedef typename Context::output_type output_type;
				typedef typename Context::input_type input_type;

				for (; tocopy; --tocopy) {
					*to_cur++ = static_cast<output_type>(
						static_cast<typename std::make_unsigned<input_type>::type>(*from_cur++));
				}
			}
		} // namespace internal
	} // namespace codecvt_convert
} // namespace ext