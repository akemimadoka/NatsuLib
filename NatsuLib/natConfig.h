#pragma once

#ifdef _WIN32
#define EnableStackWalker 1
#endif

#ifdef EnableStackWalker
#	define EnableExceptionStackTrace 1
#endif
