
#ifndef UTIL_H
#define UTIL_H

#ifdef WIN32
	#define SecondSleep(x) Sleep(x * 1000)
	typedef int socklen_t;

#else
	#include <unistd.h>
	#define SecondSleep(x) sleep(x)
	#define closesocket(x) close(x)
#endif

#endif // UTIL_H
