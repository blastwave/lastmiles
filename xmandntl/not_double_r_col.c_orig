
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

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

unsigned long linear_inter( uint8_t  in_val,
                            uint32_t low_col, uint32_t high_col,
                            uint8_t  low_val, uint8_t upper_val);

unsigned long ndr ( uint8_t height )
{
    unsigned long cpixel = 0;

    if ( height < 16 ) {
        cpixel = linear_inter( height, (uint32_t)0x8a410a,
                                       (uint32_t)0xe62063,
                                       (uint8_t)0, (uint8_t)15);
    } else if ( ( height > 15 ) && ( height < 32 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xe62063,
                                       (uint32_t)0x3af8ff,
                                       (uint8_t)16, (uint8_t)31);
    } else if ( ( height > 31 ) && ( height < 128 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x3af8ff,
                                       (uint32_t)0xb7307b,
                                       (uint8_t)32, (uint8_t)127);
    } else if ( ( height > 127 ) && ( height < 160 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xb7307b,
                                       (uint32_t)0xecff3a,
                                       (uint8_t)128, (uint8_t)159);
    } else if ( ( height > 159 ) && ( height < 192 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xecff3a,
                                       (uint32_t)0x721a1a,
                                       (uint8_t)160, (uint8_t)191);
    } else if ( ( height > 191 ) && ( height < 224 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x721a1a,
                                       (uint32_t)0x00ff00,
                                       (uint8_t)192, (uint8_t)223);
    } else if ( ( height > 223 ) && ( height < 240 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x00ff00,
                                       (uint32_t)0xff00ff,
                                       (uint8_t)224, (uint8_t)239);
    } else if ( height > 239 ) {
        cpixel = linear_inter( height, (uint32_t)0xff00ff,
                                       (uint32_t)0xffffff,
                                       (uint8_t)240, (uint8_t)255);
    }

    return ( cpixel );

}

