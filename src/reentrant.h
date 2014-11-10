#ifndef REENTRANT_H
#define REENTRANT_H

// 此文件和对应cpp文件从网上下载

#ifndef WIN32
  #include <unistd.h>
  #include <pthread.h>
#else
  #include <windows.h>
#endif

#include <openssl/ssl.h>

int THREAD_setup(void);
int THREAD_cleanup(void);

#endif /* REENTRANT_H */
