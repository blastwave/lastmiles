
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
#include <math.h>
#include <fenv.h>
#pragma STDC FENV_ACCESS ON

#include <locale.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>

#include <pthread.h>

Window create_borderless_topwin(Display *dsp,
                         unsigned int width, unsigned int height,
                         int x, int y,
                         unsigned long bg_color);

GC create_gc(Display *dsp, Window win);

int X_error_handler(Display *dsp, XErrorEvent *errevt);

uint64_t timediff( struct timespec st, struct timespec en );

int sysinfo(void);

unsigned long mandle_col( uint8_t height );

uint32_t mbrot( double c_r, double c_i, uint32_t bail_out );

/* Patrick says cramp that value damn it! */
double cramp(double x);

/* local defs where 1044 pixels is more or less full screen
 * and 660 pixels square fits into a neat 720p res OBS setup */
#define WIN_WIDTH 1044
#define WIN_HEIGHT 1044

int main(int argc, char*argv[])
{
    /* our display and window and graphics context */
    Display *dsp;
    Window win, win2, win3;
    GC gc, gc2, gc3;
    Colormap screen_colormap;
    XEvent event;
    Font type_font;

    /* a very few colours */
    XColor red, green, blue, yellow, cyan, magenta;
    XColor cornflowerblue, royal_blue, very_dark_grey;
    XColor mandlebrot;

    /* pre-loaded 8-bit color map */
    unsigned long lsd_trippy[256];

    /* We have a whole new method of dealing with color for
     * the mandlebrot and thus we need some intermediate vars */
    double t_param, t_param_exponent, gamma, hue, rotation;
    double shift, gamma_factor;
    uint8_t red_bits, green_bits, blue_bits;
    double red_level, green_level, blue_level;

    /* we can swap back and forth on the colour method with
     * a trivial flag */
    int colour_method_flag = 1;  /* Dennis Clarke LSD trippy */
    int invert_me_dammit = 0;

    /* please see https://arxiv.org/abs/1108.5083 
     * A colour scheme for the display of astronomical intensity images
     * D. A. Green 25 Aug 2011 (v1), revised 30 Aug 2011
     *
     * I describe a colour scheme that is appropriate for the screen
     * display of intensity images. This -- unlike many currently
     * available schemes -- is designed to be monotonically increasing
     * in terms of its perceived brightness.
     */

    /* setup mouse x and y */
    int mouse_x = -1, mouse_y = -1;
    int invert_mouse_x, invert_mouse_y;
    int mouse_x_raw, mouse_y_raw;

    /* these next five are just mouse button counters where the
     * roll_up and roll_dn are mouse wheel events */
    int button = 0;
    int left_count, mid_count, right_count, roll_up, roll_dn;
    left_count = 0;
    mid_count = 0;
    right_count = 0;
    roll_up = 0;
    roll_dn = 0;

    uint64_t t_delta;

    struct timespec t0, t1, now_time;
    struct timespec vbox_t0, vbox_t1;
    struct timespec soln_t0, soln_t1;

    /* some primordial vars */
    int disp_width, disp_height;
    unsigned int width, height;
    int conn_num, screen_num, depth;
    int j, k, p, q, offset_x, offset_y;
    int lx, ly, ux, uy;
    int gc2_x, gc2_y;
    int eff_width, eff_height, vbox_w, vbox_h;
    double obs_x_width, obs_y_height;
    double sub_pixel_real, sub_pixel_imag;
    double pixel_real_width, pixel_imag_height; 
    double magnify, real_translate, imag_translate;
    double x_prime, y_prime;

    /* use the vbox lower left coords as reference */
    int vbox_ll_x, vbox_ll_y;

    /* we need to track when a vbox has been computed
     * as well as displayed via libX11. For now we just
     * don't want to recompute the same region over and
     * over and over. */
    int vbox_flag[16][16];
    /* Also we finally have use for the little box grid that we
     * lay out and thus we will need the box coordinates */
    int vbox_x, vbox_y;
    /* well sooner or later we had to deal with this and 
     * dropping it all on the stack is horrific but gets
     * the job done */
    uint32_t mandel_val[16][16][64][64];
    memset( &mandel_val, 0x00, (size_t)(64*64*16*16)* sizeof(uint32_t));

    /* pre-fill the lsd trippy color map */
    for ( k=0; k<256; k++ ) {
        lsd_trippy[k] = mandle_col((uint8_t)k);
    }

    int candidate_int = 0;
    long long unsigned int candidate_magnify = 0;
    double candidate_double = 0.0;
    int fpe_raised = 0;
    uint32_t mand_height, mand_bail;

    int mand_x_pix, mand_y_pix;

    /* These are the initial and normalized mouse fp64 values
     * from within the viewport. */
    double win_x, win_y;

    /* small general purpose char buffer */
    char *buf = calloc((size_t)128,sizeof(unsigned char));

    char *disp_name = NULL;

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

    errno = 0;
    if ( ( argc < 5 ) && ( argc > 1 ) ) {
        fprintf(stderr,"FAIL : insufficient arguments provided\n");
        fprintf(stderr,"     : usage %s bail_out_integer \\\n",argv[0]);
        fprintf(stderr,"     :          magnify_integer \\\n");
        fprintf(stderr,"     :          double_real \\\n");
        fprintf(stderr,"     :          double_imaginary\n");
        fprintf(stderr,"     : quitting.\n");
        return ( EXIT_FAILURE );
    } else if ( argc >= 5 ) {
        /* TODO 
         * check if the first char in argv[1] is a letter 'p' and then
         * assume the remaining digits represent a power of 2 */

        candidate_int = (int)strtol(argv[1], (char **)NULL, 10);
        if ( ( errno == ERANGE ) || ( errno == EINVAL ) ){
            fprintf(stderr,"FAIL : bail_out_integer not understood\n");
            perror("     ");
            return ( EXIT_FAILURE );
        }
        if ( ( candidate_int < 256 ) || ( candidate_int > 65536 ) ){
            fprintf(stderr,"WARN : mandlebrot bail out is unreasonable\n");
            fprintf(stderr,"     : we shall assume 4096 and proceed.\n");
            mand_bail = (uint32_t)4096;
        } else {
            mand_bail = (uint32_t)candidate_int;
        }

        /* TODO fix this to use SCNu64 macro with uint64_t type */
        if ( sscanf( argv[2], "%lld", &candidate_magnify ) == 0 ) {
            fprintf(stderr,"INFO : magnify not understood as long long int\n");

            candidate_int = (int)strtol(argv[2], (char **)NULL, 10);
            if ( ( errno == ERANGE ) || ( errno == EINVAL ) ){
                fprintf(stderr,"FAIL : magnify_integer not understood\n");
                perror("     ");
                return ( EXIT_FAILURE );
            }
            if ( ( candidate_int < 1 ) || ( candidate_int > ( 1<<30 ) ) ){
                fprintf(stderr,"WARN : magnify_integer is unreasonable\n");
                fprintf(stderr,"     : we shall assume 1 and proceed.\n");
                magnify = 1.0;
            } else {
                magnify = (double)candidate_int;
            }
        } else {
            fprintf(stderr,"INFO : magnify accepted as long long int\n");
            magnify = (double)candidate_magnify;
        }

        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        candidate_double = strtod(argv[3], (char **)NULL);
        fpe_raised = fetestexcept(FE_ALL_EXCEPT);
        if (fpe_raised!=0){
            printf("INFO : FP Exception raised is");
            if ( fpe_raised & FE_INEXACT ) printf(" FE_INEXACT");
            if ( fpe_raised & FE_DIVBYZERO ) printf(" FE_DIVBYZERO");
            if ( fpe_raised & FE_UNDERFLOW ) printf(" FE_UNDERFLOW");
            if ( fpe_raised & FE_OVERFLOW ) printf(" FE_OVERFLOW");
            if ( fpe_raised & FE_INVALID ) printf(" FE_INVALID");
            if ( fpe_raised & FE_INEXACT ) printf(" FE_INEXACT");
            printf("\n");
        }
        if ( fpe_raised & FE_INEXACT ) {
            printf("real : Perfectly safe to ignore FE_INEXACT\n");
        }
        if ( ( errno == ERANGE ) || ( errno == EINVAL ) ){
            fprintf(stderr,"FAIL : double real coordinate not understood\n");
            perror("     ");
            return ( EXIT_FAILURE );
        }
        if ( !isnormal(candidate_double) && ( candidate_double != 0.0 ) ) {
            fprintf(stderr,"FAIL : double real coordinate is not normal\n");
            fprintf(stderr,"     : looks like %-+18.12e\n", candidate_double);
            return ( EXIT_FAILURE );
        }
        feclearexcept(FE_ALL_EXCEPT);
        if ( ( candidate_double < -2.0 ) || ( candidate_double > 2.0 ) ){
            fprintf(stderr,"WARN : double real coordinate is out of range\n");
            fprintf(stderr,"     : value seen = %-+18.12e\n", candidate_double );
            fprintf(stderr,"     : we shall assume zero.\n");
            real_translate = 0.0;
        } else {
            real_translate = candidate_double;
        }

        errno = 0;
        feclearexcept(FE_ALL_EXCEPT);
        candidate_double = strtod(argv[4], (char **)NULL);
        fpe_raised = fetestexcept(FE_ALL_EXCEPT);
        if (fpe_raised!=0){
            printf("INFO : FP Exception raised is");
            if ( fpe_raised & FE_INEXACT ) printf(" FE_INEXACT");
            if ( fpe_raised & FE_DIVBYZERO ) printf(" FE_DIVBYZERO");
            if ( fpe_raised & FE_UNDERFLOW ) printf(" FE_UNDERFLOW");
            if ( fpe_raised & FE_OVERFLOW ) printf(" FE_OVERFLOW");
            if ( fpe_raised & FE_INVALID ) printf(" FE_INVALID");
            printf("\n");
        }
        if ( fpe_raised & FE_INEXACT ) {
            printf("imag : Perfectly safe to ignore FE_INEXACT\n");
        }
        if ( ( errno == ERANGE ) || ( errno == EINVAL ) ){
            fprintf(stderr,"FAIL : double imaginary coordinate not understood\n");
            perror("     ");
            return ( EXIT_FAILURE );
        }
        if ( !isnormal(candidate_double) && ( candidate_double != 0.0 ) ) {
            fprintf(stderr,"FAIL : double imaginary coordinate is not normal\n");
            fprintf(stderr,"     : looks like %-+18.12e\n", candidate_double);
            return ( EXIT_FAILURE );
        }
        feclearexcept(FE_ALL_EXCEPT);
        if ( ( candidate_double < -2.0 ) || ( candidate_double > 2.0 ) ){
            fprintf(stderr,"WARN : double imaginary coordinate is out of range\n");
            fprintf(stderr,"     : value seen = %-+18.12e\n", candidate_double );
            fprintf(stderr,"     : we shall assume zero.\n");
            imag_translate = 0.0;
        } else {
            imag_translate = candidate_double;
        }

    } else {
        fprintf(stderr,"WARN : No arguments received thus we have\n");
        fprintf(stderr,"     : some hard coded values ... enjoy.\n");
        mand_bail = 4096;
        magnify = 1.0;
        real_translate = 0.0;
        imag_translate = 0.0;
    }

    printf("\nmand_bail = %i\n", mand_bail);
    printf("translate = ( %-+18.12e , %-+18.12e )\n",
                                      real_translate, imag_translate );

    printf("  magnify = %-+18.12e\n\n", magnify );

    /* some values for the new color computation */
    gamma = 1.5;
    hue = 1.0;
    rotation = 5.0;
    shift = 1.0;
    t_param_exponent = 1.5;

    /* TODO perhaps use the terms real and imaginary for the
     * data axi and not just x and y. However x and y are nice
     * and short */
    obs_x_width = 4.0 / magnify;
    obs_y_height = 4.0 / magnify;

    /* ensure we start with clear vbox flags */
    for ( p=0; p<16; p++ )
        for ( q=0; q<16; q++ )
            vbox_flag[p][q] = 0;

    width = WIN_WIDTH;
    height = WIN_HEIGHT;
    printf("\n X11 : ");
    printf("default width=%4i height=%4i\n", width, height);

    XSetErrorHandler(X_error_handler);

    /* should work with a null display name */
    dsp = XOpenDisplay(disp_name);
    if (dsp == NULL) {
        fprintf(stderr, "%s: no X server?? '%s'\n",
            argv[0], disp_name);
        exit(EXIT_FAILURE);
    }
    conn_num = XConnectionNumber(dsp);
    printf("     : connection number %i\n", conn_num);

    screen_num = DefaultScreen(dsp);
    printf("     : screen number %i\n", screen_num);

    depth = XDefaultDepth(dsp,screen_num);
    printf("     : default depth is %i\n", depth);

    /* really we need to get a list of the available fonts and then
     * use one that should work in the correct size
     */
    type_font = XLoadFont(dsp, "lucidasanstypewriter-10");

    disp_width = DisplayWidth(dsp, screen_num);
    disp_height = DisplayHeight(dsp, screen_num);

    printf("     : display seems to be %i wide and %i high.\n",
                                   disp_width, disp_height);

    if ( ( disp_width < (int)width )
         ||
         ( disp_height < (int)height ) ) {
        fprintf(stderr, "ERROR: screen is too small\n\n");
        exit(EXIT_FAILURE);
    }

    /* hard coded screen offset */
    offset_x = 20;
    offset_y = 20;

    printf("     : offset x=%i y=%i\n", offset_x, offset_y);

    /* Our primary plotting windows is pale grey on the screen */
    unsigned long gc_bg = 0x0f0f0f;
    win = create_borderless_topwin(dsp, width, height,
                                        offset_x, offset_y,
                                        gc_bg);
    gc = create_gc(dsp, win);

    /* create a smaller darker window to the right */
    unsigned long gc2_bg = 0x080000;
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
     * inside the main mandlebrot computation loop. Colors to
     * be determined via a smooth function brought to use by 
     * Patrick Scheibe. */
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
    XDrawPoint(dsp, win, gc, 5, (int)height - 5);
    XDrawPoint(dsp, win, gc, (int)width - 5, 5);
    XDrawPoint(dsp, win, gc, (int)width - 5, (int)height - 5);

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
     * we also need room for the one pixel borders.
     */
    XDrawRectangle(dsp, win2, gc2, 10, 10, 202, 202);
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
    lx = (int)width - offset_x;
    ly = (int)height - offset_y;

    /* therefore we have effective box width and height */
    eff_width = lx - ux;
    eff_height = ly - uy;

    printf("     : eff_width = %5i    eff_height = %5i\n\n",
                           eff_width, eff_height);
    printf("--------------------------------------------\n");

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
        XDrawLine(dsp, win, gc, j, (int)height - 8, j, (int)height - 12);
    }
    XFlush(dsp);

    /* vertical minor tic marks at every 16th of the interior viewport
     * drawing area */
    for ( j = offset_y + vbox_h; j < ly; j += vbox_h ){
        XDrawLine(dsp, win, gc, 8, j, 12, j);
        XDrawLine(dsp, win, gc, (int)width - 8, j, (int)width - 12, j);
    }
    XFlush(dsp);

    /* now we use the very dark grey color we created */
    XSetForeground(dsp,gc,very_dark_grey.pixel);

    /* draw the vertical lines */
    for ( j= offset_x + vbox_w; j<lx; j+=vbox_w ){
        XDrawLine(dsp, win, gc, j, 13, j, (int)height - 13);
    }

    /* draw the horizontal lines */
    for ( j = offset_y + vbox_h; j<ly; j+=vbox_h ){
        XDrawLine(dsp, win, gc, 13, j, (int)width - 13, j);
    }

    /* gc3 green text as default */
    XSetForeground(dsp, gc3, green.pixel);

    /* royal blue border around the main viewport */
    XSetForeground(dsp, gc, royal_blue.pixel);
    XDrawLine(dsp, win, gc, 10, 10, (int)width - 10, 10);
    XDrawLine(dsp, win, gc, (int)width - 10, 10, (int)width - 10, (int)height - 10);
    XDrawLine(dsp, win, gc, (int)width - 10, (int)height - 10, 10, (int)height - 10);
    XDrawLine(dsp, win, gc, 10, (int)height - 10, 10, 10);

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

    sprintf(buf,"[0000] tdelta = %14lld nsec", t_delta);
    XDrawImageString( dsp, win3, gc3, 10, 20, buf, (int)strlen(buf));

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

        mouse_x_raw = mouse_x;
        mouse_y_raw = mouse_y;
        XSetForeground(dsp, gc2, red.pixel);
        sprintf(buf,"raw  [ %-4i , %-4i ]", mouse_x_raw, mouse_y_raw);
        XDrawImageString( dsp, win2, gc2, 215, 210, buf, (int)strlen(buf));
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
                XDrawImageString( dsp, win2, gc2, 10, 230, buf, (int)strlen(buf));

                sprintf(buf,"fp64( %-10.8e , %-10.8e )", win_x, win_y );
                XDrawImageString( dsp, win2, gc2, 10, 250, buf, (int)strlen(buf));

                /* a more useful value is the vbox[] coordinates for
                 * each of the 16x16 grid we previously laid out */
                vbox_x = ( mouse_x - offset_x ) / vbox_w;
                vbox_y = ( eff_height - mouse_y + offset_y ) / vbox_h;
                sprintf(buf,"vbox  [ %03i , %03i ]", vbox_x, vbox_y );
                XDrawImageString( dsp, win2, gc2, 10, 270, buf, (int)strlen(buf));
                fprintf(stderr,"%s\n", buf);

                /* Offset the floating point values such that the
                 * center point shall be ( 0.0, 0.0 ) */
                win_x = win_x * 2.0 - 1.0;
                win_y = win_y * 2.0 - 1.0;

                XSetForeground(dsp, gc2, cornflowerblue.pixel);
                sprintf(buf,"fp64( %-+10.8e , %-+10.8e )  ", win_x, win_y );
                XDrawImageString( dsp, win2, gc2, 10, 290, buf, (int)strlen(buf));

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

                x_prime = x_prime + real_translate;
                y_prime = y_prime + imag_translate;
                fprintf(stderr,"c = ( %-+18.12e , %-+18.12e )\n", x_prime, y_prime );

                XSetForeground(dsp, gc3, red.pixel);
                sprintf(buf,"   select = ( %-+16.10e , %-+16.10e )", x_prime, y_prime );
                XDrawImageString( dsp, win3, gc3, 10, 80, buf, (int)strlen(buf));
                XSetForeground(dsp, gc3, green.pixel);
                sprintf(buf,"mand_bail = %-6i        ", mand_bail);
                XDrawImageString( dsp, win3, gc3, 10, 100, buf, (int)strlen(buf));
                sprintf(buf,"  magnify = %18.12e", magnify);
                XDrawImageString( dsp, win3, gc3, 10, 120, buf, (int)strlen(buf));
                sprintf(buf,"   centre = ( %-+16.10e , %-+16.10e )", real_translate, imag_translate);
                XDrawImageString( dsp, win3, gc3, 10, 140, buf, (int)strlen(buf));

                /* what is the real and imaginary axi pixel width which
                 * gets used by the physical screen ? */
                pixel_real_width = obs_x_width / (double)eff_width;
                sprintf(buf,"pixel_real_w = %-+16.10e",pixel_real_width);
                XDrawImageString( dsp, win3, gc3, 10, 180, buf, (int)strlen(buf));
                pixel_imag_height = obs_y_height / (double)eff_height;
                sprintf(buf,"pixel_imag_h = %-+16.10e",pixel_imag_height);
                XDrawImageString( dsp, win3, gc3, 10, 200, buf, (int)strlen(buf));
                XSetForeground(dsp, gc3, cyan.pixel);

                /* time the computation */
                clock_gettime( CLOCK_MONOTONIC, &soln_t0 );

                for ( mand_y_pix = 0; mand_y_pix < vbox_h; mand_y_pix++ ) {
                    vbox_ll_y = vbox_y * vbox_h + mand_y_pix;
                    for ( mand_x_pix = 0; mand_x_pix < vbox_w; mand_x_pix++ ) {
                        vbox_ll_x = vbox_x * vbox_w + mand_x_pix;

                        win_x = ( ( ( 1.0 * vbox_ll_x ) / eff_width ) * 2.0 - 1.0 ) + 0.0;
                        win_y = ( -1.0 * ( ( ( 1.0 * ( eff_height - vbox_ll_y ) ) / eff_height ) * 2.0 - 1.0 ) ) + 0.0;

                        x_prime = obs_x_width * win_x / 2.0;
                        y_prime = obs_y_height * win_y / 2.0;

                        x_prime = x_prime + real_translate;
                        y_prime = y_prime + imag_translate;

                        if ( vbox_flag[vbox_x][vbox_y] == 1 ) {
                            mand_height = mandel_val[vbox_x][vbox_y][mand_x_pix][mand_y_pix];
                        } else {
                            mand_height = mbrot( x_prime, y_prime, mand_bail );
                            mandel_val[vbox_x][vbox_y][mand_x_pix][mand_y_pix] = mand_height;
                        }

                        if ( mand_height == mand_bail ) {
                            XSetForeground(dsp, gc, (unsigned long)0 );
                        } else {
                            mandlebrot.pixel = lsd_trippy[ (uint8_t)(mand_height & 0xff) ];
                            XSetForeground(dsp, gc, mandlebrot.pixel);
                        }

                        XDrawPoint(dsp, win, gc,
                                   vbox_ll_x + offset_x,
                                   ( eff_height - vbox_ll_y + offset_y ) );

                        /* A few manual offsets of ( 16, 13 ) pixels to centre the
                         * plot data into a subwindow of gc2 */
                        gc2_x = 16 + ( 3 * mand_x_pix );
                        gc2_y = 13 + ( 192 - ( 3 * mand_y_pix ) );

                        /* walk around the samples clock wise and begin with
                         * offset the real coord by one third of pixel width */
                        for ( p = 0; p < 3; p++ ) {
                            for ( q = 0; q < 3; q++ ) {

                                sub_pixel_real = x_prime + ( p - 1 ) * pixel_real_width / 3.0;
                                sub_pixel_imag = y_prime + ( q - 1 ) * pixel_imag_height / -3.0;

                                mand_height = mbrot( sub_pixel_real, sub_pixel_imag, mand_bail );

                                if ( mand_height == mand_bail ) {
                                    XSetForeground(dsp, gc2, (unsigned long)0 );
                                } else {
                                    mandlebrot.pixel = lsd_trippy[ (uint8_t)(mand_height & 0xff) ];
                                    XSetForeground(dsp, gc2, mandlebrot.pixel);
                                }

                                XDrawPoint( dsp, win2, gc2, gc2_x + p, gc2_y + q );

                            }
                        }
                    }
                }
                vbox_flag[vbox_x][vbox_y] = 1;

                clock_gettime( CLOCK_MONOTONIC, &soln_t1 );

                t_delta = timediff( soln_t0, soln_t1 );
                sprintf(buf,"[soln] = %14lld nsec   %08.6e sec", t_delta, ((double)t_delta)/1.0e9);
                fprintf(stderr,"%s\n",buf);
                XSetForeground(dsp, gc3, green.pixel);
                XDrawImageString( dsp, win3, gc3, 10, 290, buf, (int)strlen(buf));

            }
            /* TODO here we implement the FiggleFratz "jank" slider idea
             * for input controls to modify essential parameters */

        } else if ( button == Button2 ) {

            if (    ( mouse_x >=  offset_x ) && ( mouse_y >= offset_y )
                 && ( mouse_x < ( eff_width + offset_x ) )
                 && ( mouse_y < ( eff_height + offset_y ) ) ) {

                /* we are inside the primary window plotting region */
                win_x = ( 1.0 * ( mouse_x - offset_x ) ) / eff_width;
                win_y = ( 1.0 * ( eff_height - mouse_y + offset_y ) ) / eff_height;

                /* invert the y axis */
                invert_mouse_x = mouse_x - offset_x;
                invert_mouse_y = eff_height - mouse_y + offset_y;
                sprintf(buf,"inv  [ %4i , %4i ]  ", invert_mouse_x, invert_mouse_y );

                XSetForeground(dsp, gc2, green.pixel);
                XDrawImageString( dsp, win2, gc2, 10, 230, buf, (int)strlen(buf));

                sprintf(buf,"fp64( %-10.8e , %-10.8e )", win_x, win_y );
                XDrawImageString( dsp, win2, gc2, 10, 250, buf, (int)strlen(buf));

                /* vbox[] coordinates for the 16x16 grid */
                vbox_x = ( mouse_x - offset_x ) / vbox_w;
                vbox_y = ( eff_height - mouse_y + offset_y ) / vbox_h;
                sprintf(buf,"vbox  [ %03i , %03i ]", vbox_x, vbox_y );
                XDrawImageString( dsp, win2, gc2, 10, 270, buf, (int)strlen(buf));
                fprintf(stderr,"%s\n", buf);

                /* Offset the floating point values such that the
                 * center point shall be ( 0.0, 0.0 ) */
                win_x = win_x * 2.0 - 1.0;
                win_y = win_y * 2.0 - 1.0;

                XSetForeground(dsp, gc2, cornflowerblue.pixel);
                sprintf(buf,"fp64( %-+10.8e , %-+10.8e )  ", win_x, win_y );
                XDrawImageString( dsp, win2, gc2, 10, 290, buf, (int)strlen(buf));

                x_prime = obs_x_width * win_x / 2.0;
                y_prime = obs_y_height * win_y / 2.0;

                /* translation */
                x_prime = x_prime + real_translate;
                y_prime = y_prime + imag_translate;

                XSetForeground(dsp, gc3, red.pixel);
                sprintf(buf,"   select = ( %-+16.10e , %-+16.10e )", x_prime, y_prime );
                XDrawImageString( dsp, win3, gc3, 10, 80, buf, (int)strlen(buf));
                XSetForeground(dsp, gc3, green.pixel);
                sprintf(buf,"mand_bail = %-6i        ", mand_bail);
                XDrawImageString( dsp, win3, gc3, 10, 100, buf, (int)strlen(buf));
                sprintf(buf,"  magnify = %18.12e", magnify);
                XDrawImageString( dsp, win3, gc3, 10, 120, buf, (int)strlen(buf));
                sprintf(buf,"   centre = ( %-+16.10e , %-+16.10e )", real_translate, imag_translate);
                XDrawImageString( dsp, win3, gc3, 10, 140, buf, (int)strlen(buf));
                XSetForeground(dsp, gc3, cyan.pixel);

                clock_gettime( CLOCK_MONOTONIC, &soln_t0 );
                /* here we loop over the vbox coords */
                if ( colour_method_flag ) {
                    colour_method_flag = 0;
                } else {
                    colour_method_flag = 1;
                    if ( invert_me_dammit == 0 ) {
                        invert_me_dammit = 1;
                    } else {
                        invert_me_dammit = 0;
                    }
                }
                for ( vbox_y = 0; vbox_y < 16; vbox_y++ ) {
                    for ( vbox_x = 0; vbox_x < 16; vbox_x++ ) {
                        if ( 1 ) { /* vbox_flag[vbox_x][vbox_y] == 0 */
                            clock_gettime( CLOCK_MONOTONIC, &vbox_t0 );
                            for ( mand_y_pix = 0; mand_y_pix < vbox_h; mand_y_pix++ ) {
                                vbox_ll_y = vbox_y * vbox_h + mand_y_pix;
                                for ( mand_x_pix = 0; mand_x_pix < vbox_w; mand_x_pix++ ) {
                                    vbox_ll_x = vbox_x * vbox_w + mand_x_pix;

                                    win_x = ( ( ( 1.0 * vbox_ll_x )
                                                / eff_width ) * 2.0 - 1.0 ) + 0.0;

                                    win_y = ( -1.0 *
                                              ( (
                                                   ( 1.0 * ( eff_height - vbox_ll_y ) ) / eff_height
                                                ) * 2.0 - 1.0
                                              ) ) + 0.0;

                                    x_prime = obs_x_width * win_x / 2.0;
                                    y_prime = obs_y_height * win_y / 2.0;

                                    x_prime = x_prime + real_translate;
                                    y_prime = y_prime + imag_translate;

                                    if ( vbox_flag[vbox_x][vbox_y] == 1 ) {
                                        mand_height = mandel_val[vbox_x][vbox_y][mand_x_pix][mand_y_pix];
                                    } else {
                                        mand_height = mbrot( x_prime, y_prime, mand_bail );
                                        mandel_val[vbox_x][vbox_y][mand_x_pix][mand_y_pix] = mand_height;
                                    }

                                    if ( colour_method_flag ) {

                                        if ( invert_me_dammit == 0 ) {
                                            t_param = pow( ( (double)mand_height / (double)mand_bail ), t_param_exponent);
                                        } else {
                                            t_param = pow( 1.0 - ( (double)mand_height / (double)mand_bail ), t_param_exponent);
                                        }
    
                                        gamma_factor = pow( t_param, gamma );
                
                                        red_level  =  gamma_factor + ( hue * gamma_factor * ( 1.0 - gamma_factor )
                                                        * ( -0.14861 * cos( 2.0 * M_PI * ( shift/3.0 + rotation * t_param ) )
                                                        + 1.78277 * sin( 2.0 * M_PI * ( shift/3.0 + rotation * t_param ))))/2.0;
                
                                        green_level = gamma_factor + ( hue * gamma_factor * ( 1.0 - gamma_factor )
                                                        * ( -0.29227 * cos( 2.0 * M_PI * ( shift/3.0 + rotation * t_param ) )
                                                        - 0.90649 * sin( 2.0 * M_PI * ( shift/3.0 + rotation * t_param ))))/2.0;
                
                                        blue_level  = gamma_factor + ( hue * gamma_factor * ( 1.0 - gamma_factor)
                                                        * (1.97294 * cos( 2.0 * M_PI * (shift/3.0 + rotation * t_param ))))/2.0;
                
                                        red_bits   = (uint8_t) ( 255.0 * cramp(red_level) );
                                        green_bits = (uint8_t) ( 255.0 * cramp(green_level) );
                                        blue_bits  = (uint8_t) ( 255.0 * cramp(blue_level) );
                
                                        mandlebrot.pixel = (unsigned long)( ( red_bits<<16 ) + ( green_bits<<8 ) + blue_bits );

                                        XSetForeground(dsp, gc, mandlebrot.pixel);

                                    } else {

                                        if ( mand_height == mand_bail ) {
                                            XSetForeground(dsp, gc, (unsigned long)0 );
                                        } else {
                                            mandlebrot.pixel = lsd_trippy[ (uint8_t)(mand_height & 0xff) ];
                                            XSetForeground(dsp, gc, mandlebrot.pixel);
                                        }
                                    }

                                    XDrawPoint(dsp, win, gc, vbox_ll_x + offset_x, ( eff_height - vbox_ll_y + offset_y ) );

                                }
                            }
                            vbox_flag[vbox_x][vbox_y] = 1;
                            clock_gettime( CLOCK_MONOTONIC, &vbox_t1 );
                            t_delta = timediff( vbox_t0, vbox_t1);
                            sprintf(buf,"[vbox] = %14lld nsec   %08.6e sec", t_delta, ((double)t_delta)/1.0e9);
                            XSetForeground(dsp, gc3, yellow.pixel);
                            XDrawImageString( dsp, win3, gc3, 10, 310, buf, (int)strlen(buf));
                        }
                    }
                }
            } /* inside main plot area check */

            XSetForeground(dsp, gc, yellow.pixel);
            clock_gettime( CLOCK_MONOTONIC, &soln_t1 );
            t_delta = timediff( soln_t0, soln_t1 );
            sprintf(buf,"[mand] = %14lld nsec   %08.6e sec", t_delta, ((double)t_delta)/1.0e9);
            fprintf(stderr,"%s\n\n",buf);
            XSetForeground(dsp, gc2, red.pixel);
            XDrawImageString( dsp, win2, gc2, 10, 310, buf, (int)strlen(buf));

        } else if ( button == Button3 ) {

            printf("right click\n");
            clock_gettime( CLOCK_MONOTONIC, &t1 );
            t_delta = timediff( t0, t1 );

            sprintf(buf,"[%04i] tdelta = %14lld nsec",
                                            right_count, t_delta);

            XDrawImageString( dsp, win3, gc3, 10, 20,
                               buf, (int)strlen(buf));

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
    return EXIT_SUCCESS;
}

double cramp(double x) {
    return x > 1.0 ? 1.0 : x < 0.0 ? 0.0 : x;
}

