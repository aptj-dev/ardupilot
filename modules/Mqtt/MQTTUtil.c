
#define _GNU_SOURCE /* for pthread_mutexattr_settype */
#include <stdlib.h>
#include <memory.h>
#if !defined(WIN32) && !defined(WIN64)
	#include <sys/time.h>
#endif

#if !defined(NO_PERSISTENCE)
#include "MQTTPersistence.h"
#endif
#include "MQTTAsync.h"

int getint(void *val)
{
	int wk;

	memcpy(&wk, val, sizeof(int));
	return wk;

}

MQTTAsync_token getMQTTAsync_token(void *val)
{

	MQTTAsync_token wk;

	wk = getint(val);
	
	return wk;


}

