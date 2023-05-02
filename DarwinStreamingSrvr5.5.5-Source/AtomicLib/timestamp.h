#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Get a 64 bit timestamp */
extern long long timestamp(void);

struct timescale {
    long long tsc_numerator;
    long long tsc_denominator;
};

/*
 * Get numerator and denominator to convert value returned 
 * by timestamp() to microseconds
 */
extern void utimescale(struct timescale *tscp);

extern long long scaledtimestamp(double scale);

#ifdef __cplusplus
}
#endif

#endif /* _TIMESTAMP_H_ */
