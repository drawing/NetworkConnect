PROJECT(main)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

if (WIN32)
        ADD_DEFINITIONS(-DWIN32)
        if (OPENSSL_INCLUDE)
        	INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE})
        else (OPENSSL_INCLUDE)
        	message(fatal_error "OPENSSL_INCLUDE not found")
        endif(OPENSSL_INCLUDE)
        
        if (OPENSSL_LIB)
        	LINK_DIRECTORIES(${OPENSSL_LIB})
        else (OPENSSL_LIB)
        	message(fatal_error "OPENSSL_LIB not found")
        endif(OPENSSL_LIB)
        
        SET(SYS_LIB ws2_32 iphlpapi)
        SET(SSL_LIB ssl crypto)
else (WIN32)
        ADD_DEFINITIONS(-DLINUX)
        
        SET(SYS_LIB pthread)
        SET(SSL_LIB ssl)
endif (WIN32)


SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -g")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

ADD_SUBDIRECTORY(depend)
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)
