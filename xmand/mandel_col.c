
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

uint32_t linear_inter( uint8_t  in_val,
                       uint32_t low_col, uint32_t high_col,
                       uint8_t  low_val, uint8_t upper_val);

uint32_t mandle_col ( uint8_t height )
{
    uint32_t cpixel;
    /* the idea on the table is to compute a reasonable
     * 32 bit value for RGBA data based on a range of
     * possible mandlebrot evaluations :
     *
     *  range val
     *   0 - 31  : dark blue     ->  light blue
     *             0x0b104b          0x6973ee
     *
     *  32 - 63  : light blue    ->  light red
     *             0x6973ee          0xf73f3f
     *
     *  64 - 127 : light red     ->  dark cyan
     *             0xf73f3f          0xb7307b
     *
     * 128 - 159 : dark cyan     ->  bright yellow
     *             0xb7307b          0xecff3a
     *
     * 160 - 191 : bright yellow -> dark red
     *             0xecff3a         0x721a1a
     *
     * 192 - 223 : dark red      -> green
     *             0x721a1a         0x00ff00
     *
     * 224 - 239 : green         -> magenta
     *             0x00ff00         0xff00ff
     *
     * 240 - 255 : magenta       -> white
     *             0xff00ff         0xffffff
     */

    if ( height < 32 ) {
        cpixel = linear_inter( height, (uint32_t)0x0b104b,
                                       (uint32_t)0x6973ee,
                                       (uint8_t)0, (uint8_t)31);
    } else if ( ( height > 31 ) && ( height < 64 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x6973ee,
                                       (uint32_t)0xf73f3f,
                                       (uint8_t)32, (uint8_t)63);
    } else if ( ( height > 63 ) && ( height < 128 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xf73f3f,
                                       (uint32_t)0xb7307b,
                                       (uint8_t)64, (uint8_t)127);
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
    } else {
        /* should never happen once this all works */
        cpixel = ( ( (uint32_t)( 255 - height ) ) << 16 )
               + ( ( (uint32_t)( 255 - height ) ) << 8 )
               +   ( (uint32_t)( 255 - height ) );
    }

    return ( cpixel );

}

