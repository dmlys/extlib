#pragma once
#include <memory>
#include <utility> // for std::exchange
#include <streambuf>
#include <ext/iostreams/streambuf.hpp>

namespace ext
{
	/// Базовый класс для построения socket_streambuf классов.
	/// Реализует буферизацию, непосредственно операции работы с сокетами реализуется наследником
	/// при открытии/подключении класс наследник должен вызвать функцию init_buffers,
	/// в которой по желанию может указать желаемый размер буфера по-умолчанию
	/// 
	/// умеет:
	/// * установка пользовательского буфера, буфер делиться пополам на ввод/вывод(подробное смотри соотв метод)
	///   класс всегда буферезирован, минимальный размер буфера - 128 байт
	///   если пользователь попытается убрать буфер или предоставить слишком маленький буфер - класс откатится на буфер по умолчанию
	/// * входящая область по умолчанию автоматически синхронизируется с исходящей. Подобно std::ios::tie
	///   как только входящий буфер исчерпан, прежде чем он будет заполнен из сокета - исходящий буфер будет сброшен в сокет
	class socket_streambuf_base :
		public ext::streambuf
	{
	private:
		typedef ext::streambuf base_type;

	private:
		char_type * m_input_buffer = nullptr;
		char_type * m_output_buffer = nullptr;
		std::size_t m_buffer_size = 0;
		
		bool m_tie_io = true;
		std::unique_ptr<char_type[]> m_buffer;

	protected:
		static const std::size_t m_defbuffer_size = 2 * 4096;

	private:
		std::size_t write_all(const char_type * data, std::size_t count);
		std::size_t read_all(char_type * data, std::size_t count);

	protected:
		/// если буфер не установлен посредством вызова pubsetbuf
		/// аллоцирует внутренниие буфферы размером buffer_size,
		/// буфер делится пополам для input/output областей
		/// в любом случае вызывает reset_buffers
		/// buffer_size должен быть не меньше 128, иначе он устанавливается в m_defbuffer_size
		void init_buffers(std::size_t buffer_size = m_defbuffer_size);
		/// переинициализирует get/put buffer areas. Буфферы никак не синхронизируеются с сокетом
		void reset_buffers();

		// input
		std::streamsize xsgetn(char_type * ptr, std::streamsize n) override;
		std::streamsize showmanyc() override = 0; // should be implemented in derived class
		int_type underflow() override;

		// output
		std::streamsize xsputn(const char_type * ptr, std::streamsize n) override;
		int_type overflow(int_type ch = traits_type::eof()) override;
		int sync() override;
	
		/// устанавливает пользовательский буфер.
		/// Класс всегда буферезирован,
		/// если buffer == nullptr or size < 128 - откатывается на дефолтный внутренний буффер
		/// вызов для уже открытого объекта - undefined behavior, скорее всего ничего хорошего
		std::streambuf * setbuf(char_type * buffer, std::streamsize size) override;

	protected:
		virtual std::size_t read_some(char_type * data, std::size_t count) = 0;
		virtual std::size_t write_some(const char_type * data, std::size_t count) = 0;

	public:
		/// синхронизация входящего потока с исходящим, по умолчанию включена
		bool self_tie() const { return m_tie_io; }
		bool self_tie(bool tie) { return std::exchange(m_tie_io, tie); }

	protected:
		socket_streambuf_base() = default;
		~socket_streambuf_base() = default;

		socket_streambuf_base(const socket_streambuf_base &) = delete;
		socket_streambuf_base & operator=(const socket_streambuf_base &) = delete;

		// msvc 2013 does not support generated move constructors
		//socket_streambuf_base(socket_streambuf_base && right) = default;
		//socket_streambuf_base & operator=(socket_streambuf_base && right) = default;
		
		socket_streambuf_base(socket_streambuf_base && right) noexcept;
		socket_streambuf_base & operator=(socket_streambuf_base && right) noexcept;

		void swap(socket_streambuf_base & right) noexcept;
	};
}
