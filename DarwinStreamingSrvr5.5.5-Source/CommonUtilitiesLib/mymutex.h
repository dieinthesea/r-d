#ifndef _MYMUTEX_H_
#define _MYMUTEX_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <mach/mach.h>
#include <pthread.h>

typedef void* mymutex_t;
mymutex_t mymutex_alloc();
void mymutex_free(mymutex_t);

void mymutex_lock(mymutex_t);
int mymutex_try_lock(mymutex_t);
void mymutex_unlock(mymutex_t);

#ifdef __cplusplus
}
#endif

#endif
