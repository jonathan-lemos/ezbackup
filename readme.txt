Dependencies:

openssl
libarchive
ncurses
ncurses menu
libedit
meganz/sdk
	libcurl
	c-ares
	libcrypto
	zlib (included with libarchive)
	sqlite
	freeimage

to fix meganz sdk memory leak:
Insert the following 2 lines in [sdk_directory]/src/posix/net.cpp:
    145	{
    146		LOG_debug << "Initializing OpenSSL locking callbacks";
    147		int numLocks = CRYPTO_num_locks();
    148		sslMutexes = new MUTEX_CLASS*[numLocks];
    149		memset(sslMutexes, 0, numLocks * sizeof(MUTEX_CLASS*));
    150	#if OPENSSL_VERSION_NUMBER >= 0x10000000  || defined (LIBRESSL_VERSION_NUMBER)
    151		CRYPTO_THREADID_set_callback(CurlHttpIO::id_function);
    152	#else
    153		CRYPTO_set_id_callback(CurlHttpIO::id_function);
    154	#endif
    155		CRYPTO_set_locking_callback(CurlHttpIO::locking_function);
>>>>156		delete [] sslMutexes;
>>>>157		sslMutexes = NULL;
    158	}
