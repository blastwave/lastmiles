
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

uint32_t linear_inter( uint8_t  in_val,
                       uint32_t low_col, uint32_t high_col,
                       uint8_t  low_val, uint8_t upper_val)
{
    /* in_val is some number that should fall between
     *        the low_val and upper_val. If not then
     *        just assume low_val or upper_val as
     *        needed.
     *
     * low_col is the actual RGB value at the low end
     *         of the scale
     *
     * high_col is the RGB colour at the high end of the
     *           scale
     *
     * low_val and upper_val are the possible range values
     *          for the in_val
     *
     * How to do a linear interpolation between two 32-bit colour
     * values?  We need a smooth function :
     *
     *    uint32_t cpixel = ( uint8_t   red_val << 16 )
     *                       + ( uint8_t green_val << 8 )
     *                       +   uint8_t  blue_val
     **/

    uint8_t red, green, blue;
    int red_direction = 1;
    int green_direction = 1;
    int blue_direction = 1;
    int lower_red, upper_red;
    int lower_green, upper_green;
    int lower_blue, upper_blue;
    uint32_t cpixel;

    if (    ( high_col & (uint32_t)0xff0000 )
         <= (  low_col & (uint32_t)0xff0000 ) ) {

        lower_red = (uint8_t)( ( high_col & (uint32_t)0xff0000 ) >> 16 );
        upper_red = (uint8_t)( (  low_col & (uint32_t)0xff0000 ) >> 16 );
        /* the red color decreases as the input index increases */
        red_direction = 0;

    } else {

        upper_red = (uint8_t)( ( high_col & (uint32_t)0xff0000 ) >> 16 );
        lower_red = (uint8_t)( (  low_col & (uint32_t)0xff0000 ) >> 16 );

    }
    printf ( "upper_red   = %02" PRIx8 "  %3i\n", upper_red, upper_red );
    printf ( "lower_red   = %02" PRIx8 "  %3i\n", lower_red, lower_red );

    if (    ( high_col & (uint32_t)0x00ff00 )
         <= (  low_col & (uint32_t)0x00ff00 ) ) {

        lower_green = (uint8_t)( ( high_col & (uint32_t)0x00ff00 ) >> 8 );
        upper_green = (uint8_t)( (  low_col & (uint32_t)0x00ff00 ) >> 8 );
        /* the green color decreases as the input index increases */
        green_direction = 0;

    } else {

        upper_green = (uint8_t)( ( high_col & (uint32_t)0x00ff00 ) >> 8 );
        lower_green = (uint8_t)( (  low_col & (uint32_t)0x00ff00 ) >> 8 );

    }
    printf ( "upper_green = %02" PRIx8 "  %3i\n", upper_green, upper_green );
    printf ( "lower_green = %02" PRIx8 "  %3i\n", lower_green, lower_green );

    if (    ( high_col & (uint32_t)0x0000ff )
         <= (  low_col & (uint32_t)0x0000ff ) ) {

        lower_blue = (uint8_t)( high_col & (uint32_t)0x0000ff );
        upper_blue = (uint8_t)(  low_col & (uint32_t)0x0000ff );
        /* the blue color decreases as the input index increases */
        blue_direction = 0;

    } else {

        upper_blue = (uint8_t)( high_col & (uint32_t)0x0000ff );
        lower_blue = (uint8_t)(  low_col & (uint32_t)0x0000ff );

    }
    printf ( "upper_blue  = %02" PRIx8 "  %3i\n", upper_blue, upper_blue );
    printf ( "lower_blue  = %02" PRIx8 "  %3i\n", lower_blue, lower_blue );

    if ( red_direction ) {
        red = (uint8_t)( lower_red
                 + ( upper_red - lower_red )
                   * ( in_val - low_val ) / ( upper_val - low_val ) );
    } else {
        red = (uint8_t)( upper_red
                - ( upper_red - lower_red )
                   * ( in_val - low_val ) / ( upper_val - low_val ) );
    }
    printf ( "\n        red = %02" PRIx8 "  %3i\n", red, red );

    if ( green_direction ) {
        green = (uint8_t)( lower_green
                 + ( upper_green - lower_green )
                    * ( in_val - low_val ) / ( upper_val - low_val ) );
    } else {
        green = (uint8_t)( upper_green
                - ( upper_green - lower_green )
                   * ( in_val - low_val ) / ( upper_val - low_val ) );
    }
    printf ( "      green = %02" PRIx8 "  %3i\n", green, green );

    if ( blue_direction ) {
        blue = (uint8_t)( lower_blue
                 + ( upper_blue - lower_blue )
                    * ( in_val - low_val ) / ( upper_val - low_val ) );
    } else {
        blue = (uint8_t)( upper_blue
                - ( upper_blue - lower_blue )
                   * ( in_val - low_val ) / ( upper_val - low_val ) );
    }
    printf ( "       blue = %02" PRIx8 "  %3i\n", blue, blue );

    cpixel = ( (uint32_t)red << 16 )
           | ( (uint32_t)green << 8 )
           |   (uint32_t)blue;

    return ( cpixel );

}

