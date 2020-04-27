
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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

uint32_t mbrot_subpixel ( Display *d, Window *w, GC *g, XColor *clr,
                          int mand_x_pix, int mand_y_pix,
                          double x_prime, double y_prime,
                          double pixel_width, double pixel_height,
                          uint32_t mand_bail )
{

    int j, k, gc2_x, gc2_y;
    uint32_t sub_pixel_height, sub_pixel[3][3];
    uint32_t red, green, blue, color_avg;
    double x, y, delta_x, delta_y;

    delta_x = pixel_width / 3.0;
    delta_y = pixel_height / 3.0;

    gc2_x = 16 + ( 3 * mand_x_pix );
    gc2_y = 13 + ( 192 - ( 3 * mand_y_pix ) );

    red = 0;
    green = 0;
    blue = 0;
    color_avg = 0;

    for ( j=0; j<3; j++ ) {
        for ( k=0; k<3; k++ ) {
            x = ( (double)( j - 1 ) * delta_x ) + x_prime;
            y = ( (double)( k - 1 ) * delta_y ) + y_prime;
            sub_pixel_height = mbrot( x, y, mand_bail );

            if ( sub_pixel_height == mand_bail ) {
                sub_pixel[j][k] = 0;
            } else {
                sub_pixel[j][k] = mandle_col( (uint8_t)(sub_pixel_height & 0xff) );
            }

            red   += ( sub_pixel[j][k] & 0xff0000 ) >> 16;
            green += ( sub_pixel[j][k] & 0x00ff00 ) >> 8;
            blue  += ( sub_pixel[j][k] & 0x0000ff );

            clr->pixel = (unsigned long)sub_pixel[j][k];
            XSetForeground( d, *g, clr->pixel );
            XDrawPoint( d, *w, *g, gc2_x + j, gc2_y + k );
            /*
             *  mand_height = mbrot( x_prime, y_prime, mand_bail );
             *  if ( mand_height == mand_bail ) {
             *      XSetForeground(dsp, gc, (unsigned long)0 );
             *  } else {
             *      mandlebrot.pixel = (unsigned long)mandle_col ( (uint8_t)(mand_height & 0xff) );
             *      XSetForeground(dsp, gc, mandlebrot.pixel);
             *  }
             *  XDrawPoint(dsp, win, gc, vbox_ll_x + offset_x, ( eff_height - vbox_ll_y + offset_y ) );
             */

        }
    }

    color_avg = ( ( ( red / 9 ) & 0xff ) << 16 )
                ||
                ( ( ( green / 9 ) & 0xff ) << 8 )
                ||
                ( ( blue / 9 ) & 0xff );

    return ( color_avg );

}

