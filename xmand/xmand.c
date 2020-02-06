
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

#include <X11/Xlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>

#include <pthread.h>

/* we shall need the complex math functions */
#include "v.h"

Window create_borderless_topwin(Display *dsp,
                         int width, int height,
                         int x, int y, int bg_color);

GC create_gc(Display *dsp, Window win);

int X_error_handler(Display *dsp, XErrorEvent *errevt);

uint64_t timediff( struct timespec st, struct timespec en );

int sysinfo(void);
uint32_t mandle_col ( uint8_t height );
uint32_t linear_inter( uint8_t  in_val,
                       uint32_t low_col, uint32_t high_col,
                       uint8_t  low_val, uint8_t upper_val);

/* local defs where 1044 pixels is more or less full screen
 * and 660 pixels square fits into a neat 720p res OBS setup */
#define WIN_WIDTH 1044
#define WIN_HEIGHT 1044
#define OFFSET 10

int main(int argc, char*argv[])
{
    /* our display and window and graphics context */
    Display *dsp;
    Window win, win2, win3;
    GC gc, gc2, gc3;
    Colormap screen_colormap;
    XEvent event;
    Font fixed_font, type_font;

    /* a very few colours */
    XColor red, green, blue, yellow, cyan, magenta;
    XColor cornflowerblue, royal_blue, very_dark_grey;
    XColor mandlebrot;

    Status retcode0;  /* not really needed for trivial stuff */
    int retcode1, status;     /* curious what some funcs return */

    /* setup mouse x and y */
    int mouse_x = -1, mouse_y = -1;
    int invert_mouse_x,  invert_mouse_y;

    /* these next five are just mouse button counters where the
     * roll_up and roll_dn are mouse wheel events */
    int button, left_count, mid_count, right_count, roll_up, roll_dn;
    left_count = 0;
    mid_count = 0;
    right_count = 0;
    roll_up = 0;
    roll_dn = 0;

    uint64_t t_delta;

    struct timespec t0, t1, now_time;
    struct timespec soln_t0, soln_t1;

    /* some primordial vars */
    int disp_width, disp_height, width, height;
    int conn_num, screen_num, depth;
    int j, k, p, q, offset_x, offset_y, lx, ly, ux, uy, px, py;
    int eff_width, eff_height, vbox_w, vbox_h;
    double obs_x_width, obs_y_height;
    double magnify, real_translate, imag_translate;
    double x_prime, y_prime;

    int mand_height, mand_bail = 255; /* some initial bail out value */

    cplex_type mand_tmp, mand_z, mand_c;
    int mand_x_pix, mand_y_pix;
    double mand_mag;



    /* These are the initial and normalized mouse fp64 values
     * from within the observation viewport. */
    double win_x, win_y;

    /* Also we finally have use for the little box grid that we
     * lay out and thus we will need the box coordinates */
    int vbox_x, vbox_y;

    /* small general purpose char buffer */
    char *buf = calloc((size_t)128,sizeof(unsigned char));
    char *buf0 = calloc((size_t)128,sizeof(unsigned char));

    char *disp_name = NULL;

    /* a visual box region of pixel data for screen render */
    pixel_type vb[64][64];

    setlocale( LC_ALL, "C" );

    /* Get the REALTIME_CLOCK time in a timespec struct */
    if ( clock_gettime( CLOCK_REALTIME, &now_time ) == -1 ) {
        /* We could not get the clock. Bail out. */
        fprintf(stderr,"ERROR : could not attain CLOCK_REALTIME\n");
        return(EXIT_FAILURE);
    } else {
        /* call srand48() with the sub-second time data */
        srand48( (long) now_time.tv_nsec );
    }
    sysinfo();

    /* TODO hack at this with 4x4 viewport or whatever scale we want */
    magnify = 256.0;
    obs_x_width = 4.0 / magnify;
    obs_y_height = 4.0 / magnify;
    /*    real_translate = -0.0730285645;
     *    imag_translate = -0.9643554690; */
    real_translate = -7.368164062500e-01;
    imag_translate = -1.818847656250e-01;

    width = WIN_WIDTH;
    height = WIN_HEIGHT;
    fprintf(stdout,"INFO : ");
    fprintf(stdout,"default width=%4i height=%4i\n", width, height);

    XSetErrorHandler(X_error_handler);

    /* should work with a null display name */
    dsp = XOpenDisplay(disp_name);
    if (dsp == NULL) {
        fprintf(stderr, "%s: no X server?? '%s'\n",
            argv[0], disp_name);
        exit(EXIT_FAILURE);
    }
    conn_num = XConnectionNumber(dsp);
    printf("connection number %i\n", conn_num);

    screen_num = DefaultScreen(dsp);
    printf("screen number %i\n", screen_num);

    depth = XDefaultDepth(dsp,screen_num);
    printf("default depth is %i\n", depth);

    fixed_font = XLoadFont(dsp, "fixed");

    /* really we need to get a list of the available fonts and then
     * use one that should work in the correct size
     */
    type_font = XLoadFont(dsp, "lucidasanstypewriter-10");

    disp_width = DisplayWidth(dsp, screen_num);
    disp_height = DisplayHeight(dsp, screen_num);

    printf("display seems to be %i wide and %i high.\n",
                                   disp_width, disp_height);

    if ((disp_width<width)||(disp_height<height)){
        fprintf(stderr, "ERROR: screen is too small\n\n");
        exit(EXIT_FAILURE);
    }

    /* hard coded screen offset */
    offset_x = 20;
    offset_y = 20;

    printf("INFO : offset x=%i y=%i\n", offset_x, offset_y);

    /* Our primary plotting windows is pale grey on the screen */
    win = create_borderless_topwin(dsp, width, height,
                                        offset_x, offset_y,
                                        0x0f0f0f);
    gc = create_gc(dsp, win);

    /* create a smaller darker window to the right */
    unsigned long gc2_bg = 0x101000;
    win2 = create_borderless_topwin(dsp, 400, 330, 1070, 740, gc2_bg );
    gc2 = create_gc(dsp, win2);
    XSetBackground(dsp, gc2, gc2_bg);

    /* create another small window below that */
    unsigned long gc3_bg = 0x001010;
    win3 = create_borderless_topwin(dsp, 440, 330, 1470, 740, gc3_bg );
    gc3 = create_gc(dsp, win2);
    XSetBackground(dsp, gc3, gc3_bg);

    XSync(dsp, False);

    screen_colormap = XDefaultColormap(dsp, DefaultScreen(dsp));
    if (XAllocNamedColor(dsp,
                         screen_colormap,
                         "red", &red, &red) == 0) {
        fprintf(stderr, "XAllocNamedColor - no red color?\n");
        exit(EXIT_FAILURE);
    }
    if (XAllocNamedColor(dsp,
                         screen_colormap,
                         "green", &green, &green) == 0) {
        fprintf(stderr, "XAllocNamedColor - red works but green no??\n");
        exit(EXIT_FAILURE);
    }
    if (XAllocNamedColor(dsp,
                         screen_colormap,
                         "blue", &blue, &blue) == 0) {
        fprintf(stderr, "XAllocNamedColor - red and green okay but blue??\n");
        exit(EXIT_FAILURE);
    }
    if (XAllocNamedColor(dsp,
                         screen_colormap,
                         "yellow", &yellow, &yellow) == 0) {
        fprintf(stderr, "XAllocNamedColor - yellow bork bork bork!\n");
        exit(EXIT_FAILURE);
    }

    if (XAllocNamedColor(dsp,
                         screen_colormap,
                         "cyan", &cyan, &cyan) == 0) {
        fprintf(stderr, "XAllocNamedColor - cyan bork bork bork!\n");
        exit(EXIT_FAILURE);
    }

    if (XAllocNamedColor(dsp,
                         screen_colormap,
                         "magenta", &magenta, &magenta) == 0) {
        fprintf(stderr, "XAllocNamedColor - magenta bork bork!\n");
        exit(EXIT_FAILURE);
    }

    /* cornflowerblue is #6495ED */
    if (XAllocNamedColor(dsp,
                         screen_colormap,
                         "cornflowerblue",
                         &cornflowerblue, &cornflowerblue) == 0) {
        fprintf(stderr, "XAllocNamedColor - cornflowerblue fails.\n");
        exit(EXIT_FAILURE);
    }

    /* request Royal Blue which should be #4169E1 however we
     * will get whatever teh hardware can map closest to the
     * request */
    royal_blue.flags= DoRed | DoGreen | DoBlue;
    royal_blue.red = 0x4100;
    royal_blue.green = 0x6900;
    royal_blue.blue = 0xe100;
    if ( XAllocColor(dsp, screen_colormap, &royal_blue) == 0 ) {
        fprintf(stderr, "XAllocColor - royal_blue fails.\n");
        exit(EXIT_FAILURE);
    }

    /* We need an inner grid which in our main plot window
     * which should be a subtle very very dark grey.
     * Here we manually define the rgb components using 16bit
     * values and then create the new color */
    very_dark_grey.flags= DoRed | DoGreen | DoBlue;
    very_dark_grey.red = 0x1f00;
    very_dark_grey.green = 0x1f00;
    very_dark_grey.blue = 0x1f00;
    if ( XAllocColor(dsp, screen_colormap, &very_dark_grey) == 0 ) {
        fprintf(stderr, "XAllocColor - very_dark_grey fails.\n");
        exit(EXIT_FAILURE);
    }

    /* this is a hack color data value that we will abuse later
     * inside the mouse Xevent loop */
    mandlebrot.flags= DoRed | DoGreen | DoBlue;
    /* some dummy values */
    mandlebrot.green = 0x0000;
    mandlebrot.blue  = 0x0000;
    mandlebrot.red   = 0xff00;

    if ( XAllocColor(dsp, screen_colormap, &mandlebrot) == 0 ) {
        fprintf(stderr, "XAllocColor - gee .. mandlebrot fail.\n");
        exit(EXIT_FAILURE);
    }

    /* main plot window yellow pixel at each corner 5 pixels indent */
    XSetForeground(dsp, gc, yellow.pixel);
    XDrawPoint(dsp, win, gc, 5, 5);
    XDrawPoint(dsp, win, gc, 5, height - 5);
    XDrawPoint(dsp, win, gc, width - 5, 5);
    XDrawPoint(dsp, win, gc, width - 5, height - 5);

    /* draw a blue box inside the second window */
    XSetForeground(dsp, gc2, blue.pixel);

    XSetLineAttributes(dsp, gc2, 1, LineSolid,
                                    CapButt,
                                    JoinMiter);

    XDrawRectangle(dsp, win2, gc2, 5, 5, 390, 320);
    XSetForeground(dsp, gc2, cyan.pixel);

    /* a little window to plot the vbox data into with a 3x3 grid
     * for each pixel we sample.  This shall be the 64x64 actual
     * vbox region with room to plot each of the 3x3 samples and
     * we also need room for the one pixel borders.  This gives
     * us a 192+2 width and height with an offset at 10,10. */
    XDrawRectangle(dsp, win2, gc2, 10, 10, 204, 204);
    XSetForeground(dsp, gc2, red.pixel);

    /* draw a blue box inside the third window */
    XSetForeground(dsp, gc3, blue.pixel);
    XSetLineAttributes(dsp, gc3, 1, LineSolid,
                                    CapButt,
                                    JoinMiter);

    XDrawRectangle(dsp, win3, gc3, 5, 5, 430, 320);

    /* set our graph box inside by OFFSET pixels
     *
     * reuse the offset_foo vars from above as we do not
     * need them for window location anymore. So we can
     * use them as interior offset distances for our plot.
     *
     * TODO : maybe make these a unique name and not just
     * redefine the values we used earlier. Maybe. */
    offset_x = 10;
    offset_y = 10;

    /* upper left point */
    ux = offset_x;
    uy = offset_y;

    /* lower right point */
    lx = width - offset_x;
    ly = height - offset_y;

    /* therefore we have effective box width and height */
    eff_width = lx - ux;
    eff_height = ly - uy;

    fprintf(stdout,"     : eff_width = %5i    eff_height = %5i\n",
                           eff_width, eff_height);

    XSetLineAttributes(dsp, gc, 1, LineSolid,
                                   CapButt,
                                   JoinMiter);

    XSetForeground(dsp, gc, WhitePixel(dsp, screen_num));
    XSetForeground(dsp, gc2, green.pixel);
    XSetFont(dsp, gc2, type_font);
    XSetFont(dsp, gc3, type_font);

    /****************************************************************
     *
     * The viewport is made up of a neat grid of 16 x 16 little box
     * areas and we can lay down a lightly colored dashed lines to
     * indicate where they are. We may as well refer to these little
     * boxes as view box regions. Starting from the lower left at
     * vbox [0] [0] upwards to the upper most right corner which
     * we can call vbox [15] [15].
     *
     * Each of these vbox elements has a height and width in the
     * on screen pixels of :
     *
     *     vbox_w = eff_width/16
     *
     *     vbox_h = eff_height/16
     *
     * These may come in handy later to identify where the user has
     * clicked and to perhaps identify a small region that can be
     * computed without the burder of computing the entire viewport.
     *
     ****************************************************************/
    vbox_w = eff_width/16;
    vbox_h = eff_height/16;

    for ( j=offset_x + vbox_w; j<lx; j+=vbox_w ){
        XDrawLine(dsp, win, gc, j, 8, j, 12);
        XDrawLine(dsp, win, gc, j, height - 8, j, height - 12);
    }
    XFlush(dsp);

    /* vertical minor tic marks at every 16th of the interior viewport
     * drawing area */
    for ( j = offset_y + vbox_h; j < ly; j += vbox_h ){
        XDrawLine(dsp, win, gc, 8, j, 12, j);
        XDrawLine(dsp, win, gc, width - 8, j, width - 12, j);
    }
    XFlush(dsp);

    /* now we use the very dark grey color we created */
    XSetForeground(dsp,gc,very_dark_grey.pixel);

    /* draw the vertical lines */
    for ( j= offset_x + vbox_w; j<lx; j+=vbox_w ){
        XDrawLine(dsp, win, gc, j, 13, j, height-13);
    }

    /* draw the horizontal lines */
    for ( j = offset_y + vbox_h; j<ly; j+=vbox_h ){
        XDrawLine(dsp, win, gc, 13, j, width-13, j);
    }

    /* gc3 green text as default */
    XSetForeground(dsp, gc3, green.pixel);

    /* royal blue border around the main viewport */
    XSetForeground(dsp, gc, royal_blue.pixel);
    XDrawLine(dsp, win, gc, 10, 10, width - 10, 10);
    XDrawLine(dsp, win, gc, width - 10, 10, width - 10, height - 10);
    XDrawLine(dsp, win, gc, width - 10, height - 10, 10, height - 10);
    XDrawLine(dsp, win, gc, 10, height - 10, 10, 10);

    XFlush(dsp);

    /* TODO : at the moment the only events we are trapping are
     * the mouse buttons but in the future we will want to redraw
     * and re-expose the window if there other event types */

    XGrabPointer(dsp, win, False, ButtonPressMask, GrabModeAsync,
                           GrabModeAsync, None, None, CurrentTime);

    XSelectInput(dsp, win, ButtonPressMask);

    /* some initial time data before anyone clicks anything */
    clock_gettime( CLOCK_MONOTONIC, &t0 );
    clock_gettime( CLOCK_MONOTONIC, &t1 );
    t_delta = timediff( t0, t1 );
    /* this t_delta is a baseline offset value wherein we at least
     * know how long the clock_gettime takes. Mostly. */

    sprintf(buf,"[0000] tdelta = %16lld nsec", t_delta);
    XDrawImageString( dsp, win3, gc3, 10, 20, buf, strlen(buf));

    /* plot some points on the grid that we created */
    XSetForeground(dsp, gc, yellow.pixel);
    XDrawPoint(dsp, win, gc, 5, 5);
    /* TODO at some point check for why we are fetching the x and y
     * values over and over and over inside the switch-case */
    while(1){

        XNextEvent(dsp,&event);

        switch(event.type){
            case ButtonPress:
                switch(event.xbutton.button){
                    case Button1: /* left mouse button */
                        mouse_x=event.xbutton.x;
                        mouse_y=event.xbutton.y;
                        button=Button1;
                        left_count += 1;
                        break;

                    case Button2: /* middle mouse scroll button */
                        mouse_x=event.xbutton.x;
                        mouse_y=event.xbutton.y;
                        button=Button2;
                        mid_count += 1;
                        break;

                    case Button3: /* right mouse button */
                        mouse_x=event.xbutton.x;
                        mouse_y=event.xbutton.y;
                        button=Button3;
                        right_count += 1;
                        break;

                    case Button4: /* mouse scroll wheel up */
                        mouse_x=event.xbutton.x;
                        mouse_y=event.xbutton.y;
                        button=Button4;
                        roll_up += 1;
                        break;

                    case Button5: /* mouse scroll wheel down */
                        mouse_x=event.xbutton.x;
                        mouse_y=event.xbutton.y;
                        button=Button5;
                        roll_dn += 1;
                        break;

                    default:
                        break;
                }
            break;
        default:
            break;
        }

        /* adjustment of one or two pixels */
        mouse_x = mouse_x - 1;
        mouse_y = mouse_y - 2;

        if ( button == Button1 ){

            /* printf("leftclick"); */

            if (    ( mouse_x >=  offset_x ) && ( mouse_y >= offset_y )
                 && ( mouse_x < ( eff_width + offset_x ) )
                 && ( mouse_y < ( eff_height + offset_y ) ) ) {

                /* we are inside the primary window plotting region
                 * so lets try to create floating point values for
                 * the coordinates selected. We start with just a
                 * normalized value from zero to one. */

                win_x = ( 1.0 * ( mouse_x - offset_x ) ) / eff_width;
                win_y = ( 1.0 * ( eff_height - mouse_y + offset_y ) ) / eff_height;

                /* lets try to invert the y axis */
                invert_mouse_x = mouse_x - offset_x;
                invert_mouse_y = eff_height - mouse_y + offset_y;
                sprintf(buf,"inv  [ %4i , %4i ]  ", invert_mouse_x, invert_mouse_y );

                XSetForeground(dsp, gc2, green.pixel);
                XDrawImageString( dsp, win2, gc2, 10, 230, buf, strlen(buf));

                sprintf(buf,"fp64( %-10.8e , %-10.8e )", win_x, win_y );
                XDrawImageString( dsp, win2, gc2, 10, 250, buf, strlen(buf));

                /* a more useful value is the vbox[] coordinates for
                 * each of the 16x16 grid we previously laid out */
                vbox_x = ( mouse_x - offset_x ) / vbox_w;
                vbox_y = ( eff_height - mouse_y + offset_y ) / vbox_h;
                sprintf(buf,"vbox  [ %03i , %03i ]", vbox_x, vbox_y );
                XDrawImageString( dsp, win2, gc2, 10, 270, buf, strlen(buf));
                fprintf(stderr,"%s\n", buf);

                /* Offset the floating point values such that the
                 * center point shall be ( 0.0, 0.0 ) */
                win_x = win_x * 2.0 - 1.0;
                win_y = win_y * 2.0 - 1.0;

                XSetForeground(dsp, gc2, cornflowerblue.pixel);
                sprintf(buf,"fp64( %-+10.8e , %-+10.8e )  ", win_x, win_y );
                XDrawImageString( dsp, win2, gc2, 10, 290, buf, strlen(buf));

                /* At this moment we have normalized values for a
                 * location within the observation viewport. We can
                 * scale those values by half of the viewport width
                 * and height to get actual x_prime and y_prime
                 * values.
                 *
                 * All of the above allows us to compute a starting
                 * point on the observation plane
                 *
                 * Note the x_prime = obs_x_width * win_x / 2.0
                 *          y_prime = obs_y_height * win_y / 2.0
                 */

                x_prime = obs_x_width * win_x / 2.0;
                y_prime = obs_y_height * win_y / 2.0;

                /* TODO hack in a translation */
                x_prime = x_prime + real_translate;
                y_prime = y_prime + imag_translate;

                XSetForeground(dsp, gc3, red.pixel);
                sprintf(buf,"c = ( %-10.8e , %-10.8e )  ", x_prime, y_prime );
                /* fprintf(stderr,"\n%s\n",buf); */
                fprintf(stderr,"c = ( %-+18.12e , %-+18.12e )\n", x_prime, y_prime );
                XDrawImageString( dsp, win3, gc3, 10, 80, buf, strlen(buf));
                XSetForeground(dsp, gc3, cyan.pixel);

                /* time the computation of the intercepts etc */
                clock_gettime( CLOCK_MONOTONIC, &soln_t0 );

                /* TODO : may want to actually do something here with
                 *
                 *    int mand_bail = 100;
                 *    cplex_type mand_z, mand_c;
                 *    int mand_x_pix, mand_y_pix;
                 * vbox_w = eff_width/16; ??
                 * */

                /* use the vbox lower left coords as reference */
                int foo_x, foo_y;
                for ( mand_y_pix = 0; mand_y_pix < vbox_h; mand_y_pix++ ) {
                    foo_y = vbox_y * vbox_h + mand_y_pix;
                    for ( mand_x_pix = 0; mand_x_pix < vbox_w; mand_x_pix++ ) {
                        foo_x = vbox_x * vbox_w + mand_x_pix;
                        sprintf(buf,"foo = %-4i , %-4i", foo_x, foo_y);
                        /* fprintf(stderr,"%s    ",buf); */
                        XDrawImageString( dsp, win3, gc3, 10, 40, buf, strlen(buf));
                        /* sad little hackary here to deal with negative zeros */
                        win_x = ( ( ( 1.0 * foo_x ) / eff_width ) * 2.0 - 1.0 ) + 0.0;
                        win_y = ( -1.0 * ( ( ( 1.0 * ( eff_height - foo_y ) ) / eff_height ) * 2.0 - 1.0 ) ) + 0.0;

                        x_prime = obs_x_width * win_x / 2.0;
                        y_prime = obs_y_height * win_y / 2.0;

                        /* TODO hack in a translation */
                        x_prime = x_prime + real_translate;
                        y_prime = y_prime + imag_translate;

                        sprintf(buf,"c = ( %-10.8e , %-10.8e )", x_prime, y_prime );
                        /* fprintf(stderr,"%s    ",buf); */
                        XDrawImageString( dsp, win3, gc3, 10, 60, buf, strlen(buf));

                        /* point c belongs to the Mandelbrot set if and only if
                         * the magnitude of the f(c) <= 2.0 */
                        mand_height = 0;
                        mand_c.r = x_prime;
                        mand_c.i = y_prime;
                        mand_z.r = 0.0;
                        mand_z.i = 0.0;
                        mand_mag = 0.0;
                        while ( ( mand_height < mand_bail ) && ( mand_mag < 2.0 ) ) {
                            mand_tmp.r = mand_z.r * mand_z.r - ( mand_z.i * mand_z.i ) + 0.0;
                            mand_tmp.i = mand_z.r * mand_z.i + ( mand_z.r * mand_z.i ) + 0.0;
                            mand_z.r = mand_tmp.r + mand_c.r;
                            mand_z.i = mand_tmp.i + mand_c.i;
                            mand_mag = sqrt( mand_z.r * mand_z.r + mand_z.i * mand_z.i );
                            mand_height += 1;
                        }
                        sprintf(buf,"mand_height = %-4i", mand_height);
                        /* fprintf(stderr,"%s\n",buf); */
                        XDrawImageString( dsp, win3, gc3, 10, 100, buf, strlen(buf));

                        mandlebrot.pixel = (unsigned long)mandle_col ( (uint8_t) mand_height );

                        XSetForeground(dsp, gc, mandlebrot.pixel);
                        XDrawPoint(dsp, win, gc, foo_x + offset_x, ( eff_height - foo_y + offset_y ) );
                    }
                }

                clock_gettime( CLOCK_MONOTONIC, &soln_t1 );

                t_delta = timediff( soln_t0, soln_t1 );
                sprintf(buf,"[soln] = %16lld nsec", t_delta);
                fprintf(stderr,"%s\n",buf);
                XSetForeground(dsp, gc3, green.pixel);
                XDrawImageString( dsp, win3, gc3, 10, 290,
                                  buf, strlen(buf));

            }

        } else if ( button == Button2 ) {

            /* TODO hack *
             *  lets blame cas_9 for this :
             * XColor.pixel = (((unsigned long)XColor.red) << 16)
             *               + (((unsigned long)XColor.green) << 8)
             *               + (unsigned long)XColor.blue;
             */
            clock_gettime( CLOCK_MONOTONIC, &soln_t0 );
            /* X11 load test where we fire a ton of XLib calls */

            double radius, angle, some_x, some_y, pi2 = M_PI * 2.0;

            for ( int radius = 0; radius < vbox_w; radius++ ) {
                for ( int p=0; p<720; p++ ) {

                     /* quick hack convert from tens of degrees to
                     * radians should be (p)( ( 2 x pi )/360 ) */

                    angle = pi2 * ( (1.0 * p) / 2.0 ) / 360.0;
                    some_x = radius * cos(angle);
                    some_y = radius * sin(angle);

                    mandlebrot.pixel = ( ( (unsigned long)p & 0xff ) << 16 )
                                     + ( ( (unsigned long)radius ) << 8 );

                    XSetForeground(dsp, gc, mandlebrot.pixel);

                    XDrawPoint(dsp, win, gc, mouse_x + some_x, mouse_y + some_y);

                }
            }

            XSetForeground(dsp, gc, yellow.pixel);
            clock_gettime( CLOCK_MONOTONIC, &soln_t1 );
            t_delta = timediff( soln_t0, soln_t1 );
            sprintf(buf,"[load] = %16lld nsec", t_delta);
            fprintf(stderr,"%s\n",buf);
            XSetForeground(dsp, gc2, red.pixel);
            XDrawImageString( dsp, win2, gc2, 10, 310,
                                       buf, strlen(buf));

        } else if ( button == Button3 ) {

            printf("right click\n");
            clock_gettime( CLOCK_MONOTONIC, &t1 );
            t_delta = timediff( t0, t1 );

            sprintf(buf,"[%04i] tdelta = %16lld nsec",
                                            right_count, t_delta);

            /* throw the tdelta in the same place always */
            XDrawImageString( dsp, win3, gc3, 10, 20,
                               buf, strlen(buf));

            t0.tv_sec = t1.tv_sec;
            t0.tv_nsec = t1.tv_nsec;
            /* If a 200ms right double click anywhere then quit */
            if ( t_delta < 200000000 ) {
                printf("\n\n");
                /* If we allocate memory for any purpose whatsoever
                 * then we had better free() it. */
                break;
            }

        } else if ( button == Button4 ) {

            /* TODO note that a mouse wheel event being used here to
             * track observation plane position will result in all
             * data being redrawn. */
            printf("roll up\n");

        } else if ( button == Button5 ) {

            printf("roll down\n");

        } else {

            printf("\n ??? unknown button ???\n");

        }

        printf("click at %d %d \n", mouse_x, mouse_y);

    }

    XCloseDisplay(dsp);

    printf("\n");

    free(buf);
    free(buf0);
    return EXIT_SUCCESS;
}

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
    } else {
        /* should never happen once this all works */
        cpixel = ( ( (uint32_t)( 255 - height ) ) << 16 )
               + ( ( (uint32_t)( 255 - height ) ) << 8 )
               +   ( (uint32_t)( 255 - height ) );
    }

    return ( cpixel );

}

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
    uint8_t lower_red, upper_red;
    uint8_t lower_green, upper_green;
    uint8_t lower_blue, upper_blue;
    uint32_t cpixel;

    if (    ( high_col & (uint32_t)0xff0000 )  
         <= (  low_col & (uint32_t)0xff0000 ) ) {

        lower_red = (uint8_t)( ( high_col & (uint32_t)0xff0000 ) >> 16 );
        upper_red = (uint8_t)( (  low_col & (uint32_t)0xff0000 ) >> 16 );

    } else {

        upper_red = (uint8_t)( ( high_col & (uint32_t)0xff0000 ) >> 16 );
        lower_red = (uint8_t)( (  low_col & (uint32_t)0xff0000 ) >> 16 );

    }

    if (    ( high_col & (uint32_t)0x00ff00 )  
         <= (  low_col & (uint32_t)0x00ff00 ) ) {

        lower_green = (uint8_t)( ( high_col & (uint32_t)0x00ff00 ) >> 8 );
        upper_green = (uint8_t)( (  low_col & (uint32_t)0x00ff00 ) >> 8 );

    } else {

        upper_green = (uint8_t)( ( high_col & (uint32_t)0x00ff00 ) >> 8 );
        lower_green = (uint8_t)( (  low_col & (uint32_t)0x00ff00 ) >> 8 );

    }

    if (    ( high_col & (uint32_t)0x0000ff )  
         <= (  low_col & (uint32_t)0x0000ff ) ) {

        lower_blue = (uint8_t)( high_col & (uint32_t)0x0000ff );
        upper_blue = (uint8_t)(  low_col & (uint32_t)0x0000ff );

    } else {

        upper_blue = (uint8_t)( high_col & (uint32_t)0x0000ff );
        lower_blue = (uint8_t)(  low_col & (uint32_t)0x0000ff );

    }

    red = lower_red
           + ( upper_red - lower_red )
             * ( in_val - low_val ) / ( upper_val - low_val );

    green = lower_green
           + ( upper_green - lower_green )
             * ( in_val - low_val ) / ( upper_val - low_val );

    blue = lower_blue
           + ( upper_blue - lower_blue )
             * ( in_val - low_val ) / ( upper_val - low_val );

    cpixel = ( (uint32_t)red << 16 )
           | ( (uint32_t)green << 8 )
           |   (uint32_t)blue;

    return ( cpixel );

}

