
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
        cpixel = linear_inter( height, (uint32_t)0x2002ff,
                                       (uint32_t)0x7802ff,
                                       (uint8_t)0, (uint8_t)15);
    } else if ( ( height > 15 ) && ( height < 31 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x7802ff,
                                       (uint32_t)0xd102ff,
                                       (uint8_t)16, (uint8_t)30);
    } else if ( ( height > 30 ) && ( height < 46 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xd102ff,
                                       (uint32_t)0xff01d5,
                                       (uint8_t)31, (uint8_t)45);
    } else if ( ( height > 45 ) && ( height < 61 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xff01d5,
                                       (uint32_t)0xff0077,
                                       (uint8_t)46, (uint8_t)60);
    } else if ( ( height > 60 ) && ( height < 76 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xff0077,
                                       (uint32_t)0xff0022,
                                       (uint8_t)61, (uint8_t)75);
    } else if ( ( height > 75 ) && ( height < 91 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xff0022,
                                       (uint32_t)0xff1500,
                                       (uint8_t)76, (uint8_t)90);
    } else if ( ( height > 90 ) && ( height < 106 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xff1500,
                                       (uint32_t)0xff4400,
                                       (uint8_t)91, (uint8_t)105);
    } else if ( ( height > 105 ) && ( height < 121 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xff4400,
                                       (uint32_t)0xff8400,
                                       (uint8_t)106, (uint8_t)120);
    } else if ( ( height > 120 ) && ( height < 136 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xff8400,
                                       (uint32_t)0xffb700,
                                       (uint8_t)121, (uint8_t)135);
    } else if ( ( height > 135 ) && ( height < 151 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xffb700,
                                       (uint32_t)0xffe100,
                                       (uint8_t)136, (uint8_t)150);
    } else if ( ( height > 150 ) && ( height < 166 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xffe100,
                                       (uint32_t)0xdaff07,
                                       (uint8_t)151, (uint8_t)165);
    } else if ( ( height > 165 ) && ( height < 181 ) ) {
        cpixel = linear_inter( height, (uint32_t)0xdaff07,
                                       (uint32_t)0x80ff00,
                                       (uint8_t)166, (uint8_t)180);
    } else if ( ( height > 180 ) && ( height < 196 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x80ff00,
                                       (uint32_t)0x2fff00,
                                       (uint8_t)181, (uint8_t)195);
    } else if ( ( height > 195 ) && ( height < 211 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x2fff00,
                                       (uint32_t)0x01ff41,
                                       (uint8_t)196, (uint8_t)210);
    } else if ( ( height > 210 ) && ( height < 226 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x01ff41,
                                       (uint32_t)0x03ffd5,
                                       (uint8_t)211, (uint8_t)225);
    } else if ( ( height > 225 ) && ( height < 241 ) ) {
        cpixel = linear_inter( height, (uint32_t)0x03ffd5,
                                       (uint32_t)0x03a744,
                                       (uint8_t)226, (uint8_t)240);
    } else if ( height > 240 ) {
        cpixel = linear_inter( height, (uint32_t)0x03a744,
                                       (uint32_t)0x0320ff,
                                       (uint8_t)241, (uint8_t)255);
    }

    return ( cpixel );

}

