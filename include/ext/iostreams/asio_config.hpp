#pragma once
#include <boost/config.hpp>

#if defined(BOOST_WINDOWS) && !defined(_WIN32_WINNT)
#include <sdkddkver.h>
#endif
