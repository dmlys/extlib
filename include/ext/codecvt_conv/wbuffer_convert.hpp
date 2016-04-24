#pragma once
#include <memory>
#include <streambuf>
#include <ext/codecvt_conv/base.hpp>
#include <ext/utility.hpp>

/// дефолтный размер буфера для wbuffer_convert
#ifndef EXT_CODECVT_CONV_WBUFFER_CONVERT_BUFFER_SIZE
#define EXT_CODECVT_CONV_WBUFFER_CONVERT_BUFFER_SIZE 4096
#endif


namespace ext
{
	namespace codecvt_convert
	{
		//template <class CodeCvt>
		template <class CharType, class Traits = std::char_traits<CharType>>
		class wbuffer_convert : public std::basic_streambuf<CharType, Traits>
		{
		public:
			using state_type = std::mbstate_t;
			using streambuf = std::basic_streambuf<CharType, Traits>;
			using codecvt = std::codecvt<CharType, char, state_type>;
			using narrow_streambuf = std::basic_streambuf<char>;

			using typename streambuf::traits_type;
			using typename streambuf::char_type;
			using typename streambuf::int_type;
			using typename streambuf::pos_type;
			using typename streambuf::off_type;

			wbuffer_convert(narrow_streambuf * wrapped, const codecvt * cvt)
				: wrapped(wrapped), cvt(cvt) {}

			wbuffer_convert(wbuffer_convert const &) = delete;
			wbuffer_convert & operator =(wbuffer_convert const &) = delete;

		protected:
			using streambuf::gptr;
			using streambuf::pptr;

		protected:
			int_type underflow() override;
			//std::streamsize showmanyc() override;
			//std::streamsize xsgetn(char_type * ptr, std::streamsize count) override;
			//int_type pbackfail(int_type ch/* = Traits::eof() */) override;

			int_type overflow(int_type ch/* = Traits::eof() */) override;
			int sync() override; //for flush

		private:
			typedef std::size_t buffer_size_type;
			///streambuf can be used both for in and out dataflow
			///we have to context to track state and dataflow, they are unrelated.
			///we don't support seek operations, so we can't have united buffer, we use 2 independent

			struct input_buffer_ctx_type :
				internal::make_from_bytes_context<codecvt>::type
			{
				std::unique_ptr<char[]> from_buffer;
				std::unique_ptr<char_type[]> to_buffer;
			};

			struct output_buffer_ctx_type
				: internal::make_to_bytes_context<codecvt>::type
			{
				std::unique_ptr<char_type[]> from_buffer;
				std::unique_ptr<char[]> to_buffer;
			};

			const buffer_size_type from_buffer_capacity = EXT_CODECVT_CONV_WBUFFER_CONVERT_BUFFER_SIZE;
			const buffer_size_type to_buffer_capacity = EXT_CODECVT_CONV_WBUFFER_CONVERT_BUFFER_SIZE;
			const buffer_size_type history_buffer_capacity = 8;

			std::unique_ptr<input_buffer_ctx_type> input_bufctx;
			std::unique_ptr<output_buffer_ctx_type> output_bufctx;

			narrow_streambuf * wrapped;
			const codecvt * cvt;

			template <class Context>
			void initialize_base(Context & ctx)
			{
				ctx.from_buffer = std::make_unique<typename Context::input_type[]>(to_buffer_capacity);
				ctx.from_beg = ctx.from_buffer.get();
				ctx.from_end = ctx.from_beg + to_buffer_capacity;

				ctx.to_buffer = std::make_unique<typename Context::output_type[]>(from_buffer_capacity);
				ctx.to_beg = ctx.to_buffer.get();
				ctx.to_end = ctx.to_beg + from_buffer_capacity;

				ctx.state = state_type {};
			}

			void initialize(input_buffer_ctx_type & ctx)
			{
				initialize_base(ctx);
				ctx.from_stopped = ctx.from_end;
				ctx.to_stopped = ctx.to_end;
			}

			void initialize(output_buffer_ctx_type & ctx)
			{
				initialize_base(ctx);
				ctx.from_stopped = ctx.from_beg;
				ctx.to_stopped = ctx.to_beg;
			}

			/// after convert there are can be some left data which is not converted
			/// ctx.from_stopped points to it
			/// let unconverted_len = ctx.from_end - ctx.from_stopped
			/// it will move [ctx.from_stopped, ctx.from_end) to [ctx.from_beg, ctx.from_beg + unconverted_len)
			/// and set ctx.from_stopped = to point after last moved character(ctx.from_beg + unconverted_len)
			template <class Context>
			void move_unconverted(Context & ctx)
			{
				auto unconverted_len = ctx.from_end - ctx.from_stopped;
				memmove(ext::unconst(ctx.from_beg), ctx.from_stopped, unconverted_len * sizeof(*ctx.from_end));
				//std::move(ctx.from_stopped, ctx.from_end, ext::unconst(ctx.from_beg)); //or via std C++
				ctx.from_stopped = ctx.from_beg + unconverted_len;
			}

			/// tries to convert [ctx.from_beg, ctx.from_end) to [ctx.to_beg, ctx.to_end)
			/// sets ctx.from_stopped/ctx.to_stopped after last consumed/produced character
			/// throws in case or error
			template <class CvtContext>
			void from_bytes(CvtContext & ctx) const
			{
				from_bytes_step(*cvt, ctx);
				switch (ctx.res) {
				case codecvt::ok:
					break;
				case codecvt::partial:
					// we readed some characters, convert them, got partial and converted nothing
					// we definitely stopped at some partial input
					if (ctx.is_partial_input && ctx.from_beg == ctx.from_stopped)
						throw conversion_failure("partial input");
					break;
				case codecvt::noconv:
					internal::noconv_copy(ctx);
					break;
				default:
					throw conversion_failure();
				}
			}

			/// tries to read more characters from wrapped streambuf and convert them to intern_type
			/// returns true if readed some
			bool readsome(input_buffer_ctx_type & ctx)
			{
				move_unconverted(ctx);
				auto readed = wrapped->sgetn(ext::unconst(ctx.from_stopped), ctx.from_end - ctx.from_stopped);
				ctx.from_end = ctx.from_stopped + readed;
				if (ctx.from_beg == ctx.from_end) //nothing to convert after read, we are finished
					return false;

				from_bytes(ctx);
				return true;
			}

			template <class CvtContext>
			void to_bytes(CvtContext & ctx) const
			{
				to_bytes_step(*cvt, ctx);
				switch (ctx.res)
				{
				case codecvt::ok:
					break;
				case codecvt::partial:
					// we were given some characters via input from ostream, convert them, got partial and converted nothing
					// we definitely stopped at some partial input
					if (ctx.is_partial_input && ctx.from_beg == ctx.from_stopped)
						throw conversion_failure("partial input");
					break;
				case codecvt::noconv:
					internal::noconv_copy(ctx);
					break;
				default:
					throw conversion_failure();
				}
			}

			/// converts data and tries to write it to wrapped streambuf
			/// after write move unconverted data to the beginning of the input buffer
			/// setting ctx.from_stopped to ctx.from_beg + unconverted_len
			bool writesome(output_buffer_ctx_type & ctx)
			{
				to_bytes(ctx);
				std::streamsize ntowrite = ctx.to_stopped - ctx.to_beg;
				auto written = wrapped->sputn(ctx.to_beg, ntowrite);
				if (written != ntowrite)
					return false;

				move_unconverted(ctx);
				return true;
			}
		};

		template <class CharType, class Traits>
		auto wbuffer_convert<CharType, Traits>::underflow() -> int_type
		{
			if (!input_bufctx) {
				input_bufctx = std::make_unique<input_buffer_ctx_type>();
				initialize(*input_bufctx);
			}

			auto readed = readsome(*input_bufctx);
			if (!readed)
				return traits_type::eof();

			setg(input_bufctx->to_beg, input_bufctx->to_beg, input_bufctx->to_stopped);
			return traits_type::to_int_type(*gptr());
		}

		template <class CharType, class Traits>
		auto wbuffer_convert<CharType, Traits>::overflow(int_type ch) -> int_type
		{
			if (!output_bufctx) {
				output_bufctx = std::make_unique<output_buffer_ctx_type>();
				initialize(*output_bufctx);
			}
			else { //first time there is nothing to write
				if (!writesome(*output_bufctx))
					//throw std::runtime_error("write to wrapped failed");
					return traits_type::eof();
			}

			setp(ext::unconst(output_bufctx->from_beg), ext::unconst(output_bufctx->from_stopped), ext::unconst(output_bufctx->from_end));
			if (!traits_type::eq_int_type(ch, traits_type::eof()))
				sputc(ch);

			return traits_type::not_eof(ch);
		}

		template <class CharType, class Traits>
		int wbuffer_convert<CharType, Traits>::sync()
		{
			if (output_bufctx) {
				output_bufctx->from_end = pptr(); //current position in put area
				while (output_bufctx->from_end - output_bufctx->from_beg) {
					if (!writesome(*output_bufctx))
						return -1; //failure
					output_bufctx->from_end = output_bufctx->from_stopped;
				}
			}

			return wrapped->pubsync();
		}
	} //namespace codecvt_convert
} //namespace ext
