#pragma once

#include <ostream>
#include <cstdio>
#include <cstdarg>

namespace color
{
	enum code
	{
		FG_DEFAULT = 39,
		FG_BLACK = 30,
		FG_RED = 31,
		FG_GREEN = 32,
		FG_YELLOW = 33,
		FG_BLUE = 34,
		FG_MAGENTA = 35,
		FG_CYAN = 36,
		FG_LIGHT_GRAY = 37,
		FG_DARK_GRAY = 90,
		FG_LIGHT_RED = 91,
		FG_LIGHT_GREEN = 92,
		FG_LIGHT_YELLOW = 93,
		FG_LIGHT_BLUE = 94,
		FG_LIGHT_MAGENTA = 95,
		FG_LIGHT_CYAN = 96,
		FG_WHITE = 97
	};

	inline void color_printf(const code code, const char* __restrict __fmt, ...)
	{
		va_list args;
		va_start(args, __fmt);

		printf("\033[%dm", static_cast<int>(code));
		vprintf(__fmt, args);
		va_end(args);
		printf("\033[%dm", static_cast<int>(FG_DEFAULT));
		// fflush(stdout);
	}
} // namespace color
