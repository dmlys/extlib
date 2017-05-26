#include <ext/iostreams/socket_types.hpp>

#ifdef EXT_ENABLE_OPENSSL
#include <ext/openssl.hpp>
#endif

namespace ext
{
	struct socket_condition_category_impl : std::error_category
	{
		virtual const char * name() const noexcept override { return "sock_errc"; }
		virtual std::string message(int val) const override;
		virtual bool equivalent(const std::error_code & code, int cond_val) const noexcept override;
	};

	std::string socket_condition_category_impl::message(int val) const
	{
		switch (static_cast<sock_errc>(val))
		{
			case sock_errc::eof:       return "end of stream";
			case sock_errc::timeout:   return "timeout";
			case sock_errc::regular:   return "regular, not a error";
			case sock_errc::error:     return "socket error";
			
			default: return "unknown sock_errc code";
		}
	}

	bool socket_condition_category_impl::equivalent(const std::error_code & code, int cond_val) const noexcept
	{
		switch (static_cast<sock_errc>(cond_val))
		{
#ifdef EXT_ENABLE_OPENSSL
			case sock_errc::eof:     return code == ext::openssl_error::zero_return;
#else
			case sock_errc::eof:     return false;
#endif
			case sock_errc::regular:
				return code != sock_errc::error;

			case sock_errc::error:
				return code && code != sock_errc::eof;
			
			default: return false;
		}
	}


	static socket_condition_category_impl socket_condition_category_impl_instance;

	const std::error_category & socket_condition_category() noexcept
	{
		return socket_condition_category_impl_instance;
	}
}
