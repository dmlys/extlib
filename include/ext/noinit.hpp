#pragma once

namespace ext
{
	/// Special type/value used as indication that initialization should not be done,
	/// usually used as an constructor argument, but can be used in other contexts too(for example vector:resize(n, ext::noinit).
	/// What initialization is - is defined by each class itself
	struct noinit_type {} constexpr noinit {};
}
