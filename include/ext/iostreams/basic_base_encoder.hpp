#pragma once
#include <ext/iostreams/transform_width_encoder.hpp>

namespace ext {
namespace iostreams
{
	template <
		class EncodeFunctor,
		char PaddingCharacter,
		std::size_t BufferSize = DefaultTransformWidthEncoderBufferSize
	>
	using basic_base64_encoder = transform_width_encoder<EncodeFunctor, PaddingCharacter, 3, 4, BufferSize>;

	template <
		class EncodeFunctor,
		char PaddingCharacter,
		std::size_t BufferSize = DefaultTransformWidthEncoderBufferSize
	>
	using basic_base32_encoder = transform_width_encoder<EncodeFunctor, PaddingCharacter, 5, 8, BufferSize>;
	
	template <
		class EncodeFunctor,
		char PaddingCharacter,
		std::size_t BufferSize = DefaultTransformWidthEncoderBufferSize
	>
	using basic_base16_encoder = transform_width_encoder<EncodeFunctor, PaddingCharacter, 1, 2, BufferSize>;
}}
