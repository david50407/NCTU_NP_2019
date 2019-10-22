#if !defined(__LOGGER_HXX__)
#define __LOGGER_HXX__

#include <iostream>

#if defined(DEBUG)
	#define DBG(x) { std::cerr << x << std::endl; }
#else
	#define DBG(x)
#endif // defined(DEBUG)

#endif // !defined(__LOGGER_HXX__)