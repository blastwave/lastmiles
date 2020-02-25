
/*********************************************************************
 * The Open Group Base Specifications Issue 6
 * IEEE Std 1003.1, 2004 Edition
 *
 *    An XSI-conforming application should ensure that the feature
 *    test macro _XOPEN_SOURCE is defined with the value 600 before
 *    inclusion of any header. This is needed to enable the
 *    functionality described in The _POSIX_C_SOURCE Feature Test
 *    Macro and in addition to enable the XSI extension.
 *
 *********************************************************************/
#define _XOPEN_SOURCE 600

#include <stdint.h>
#include <stdlib.h>
#include <NTL/RR.h>

using namespace std;
using namespace NTL;

typedef uint32_t Foo[16][16][64][64];

struct Bar {
    Foo* data;
};

void test(Bar t)
{
    printf("%i----------\n", t.data[0][0][0][0]);
}

uint32_t mbrot( RR c_r, RR c_i, uint32_t bail_out )
{

    /* point c belongs to the Mandelbrot set if and only if
     * the magnitude of the f(c) <= 2.0 */
    uint32_t height = 0;
    RR zr(0.0);
    RR zi(0.0);
    RR tmp_r, tmp_i;
    RR mag(0.0);

    struct Bar t;
    Foo data;
    data[0][0][0][0] = 10;
    t.data = &data;
    test(t);

    while ( ( height < bail_out ) && ( mag <= 4.0 ) ) {
        tmp_r = ( zr * zr ) - ( zi * zi );
        tmp_i = ( zr * zi ) + ( zr * zi );
        zr = tmp_r + c_r;
        zi = tmp_i + c_i;

        /* mag = sqrt( zr * zr + zi * zi ); */
        mag = zr * zr + zi * zi;

        height += 1;
    }

    return ( height );

}

