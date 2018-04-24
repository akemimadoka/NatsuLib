#pragma once

#ifndef EnableStackWalker
#	define EnableStackWalker 1
#endif

#if EnableStackWalker && !defined(EnableExceptionStackTrace)
#	define EnableExceptionStackTrace 1
#endif

#ifndef UseFastInverseSqrt
#	define UseFastInverseSqrt 1
#endif
