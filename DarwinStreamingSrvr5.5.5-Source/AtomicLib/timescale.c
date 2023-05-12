#include "timestamp.h"
#include <stdlib.h>
#include "SafeStdLib.h"
void
utimescale(struct timescale *tscp)
{
// stop not supported
    char *death = 0;
    *death = 0;
    exit (-1);
#if 0 // old code
    unsigned int theNanosecondNumerator = 0;
    unsigned int theNanosecondDenominator = 0;

        MKGetTimeBaseInfo(NULL, &theNanosecondNumerator, &theNanosecondDenominator, NULL, NULL);
        tscp->tsc_numerator = theNanosecondNumerator / 1000; /* PPC magic number */
        tscp->tsc_denominator = theNanosecondDenominator;
        return;
#endif

}

