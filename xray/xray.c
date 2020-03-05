
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

/* pure hack to get past LLVM/Clang pissing on me */

int line_plane_icept( vec_type *icept_pt,
                      vec_type *plun,
                      vec_type *plvn,
                      vec_type *kst,
                      vec_type *lp0, vec_type *lpr,
                      vec_type *pl0, vec_type *pn,
                      vec_type *plu, vec_type *plv);

Window create_borderless_topwin(Display *dsp,
                         int width, int height,
                         int x, int y, int bg_color);

GC create_gc(Display *dsp, Window win);

int X_error_handler(Display *dsp, XErrorEvent *errevt);

uint64_t timediff( struct timespec st, struct timespec en );

int sysinfo(void);

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
    XColor whatever; /* just a hack */

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

    uint64_t t_delta, t_ray_total = 0;
    uint64_t t_sample_count = 0;
    double vec_T_mag;

    struct timespec t0, t1, now_time;
    struct timespec soln_t0, soln_t1;

    /* some primordial vars */
    int disp_width, disp_height, width, height;
    int conn_num, screen_num, depth;
    int j, k, p, q, offset_x, offset_y, lx, ly, ux, uy, px, py;
    int eff_width, eff_height, vbox_w, vbox_h;
    double obs_x_width, obs_y_height;
    double x_prime, y_prime;
    double theta_i;

    /* These are the initial and normalized mouse fp64 values
     * from within the observation viewport. */
    double win_x, win_y;

    /* Also we finally have use for the little box grid that we
     * lay out and thus we will need the box coordinates */
    int box_x, box_y;

    /* small general purpose char buffer */
    char *buf = calloc((size_t)128,sizeof(unsigned char));

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

    /* The vbox idea on the table is to create an array of
     * pointers to some sort of a magic struct wherein we
     * can gather up the data for a view box region.
     *
     * At this time we have no idea what that struct looks
     * like or how we are going to dispatch out to POSIX
     * thread type workers.
     *
     * However, having said all that, it is clear that we
     * need to take advantage of parallel processing in some
     * manner. My preference is to look at NVidia CUDA style
     * workers. Eventually.
     *
     *
     * The whole objective here is to map a point on the display
     * to the observation plane for ray tracing.
     *
     * Therefore we need some very basic data about the actual
     * observation plane such as the location and the plane
     * normal vector. We need the dimensions of the observation
     * viewport which is really just a part of the plane.
     *
     *     obs_loc = < 12, 0, 0 >
     *     obs_vec = < -1, 0, 0 >
     *
     *     This is merely example data for the display viewport :
     *     obs_x_width  = 8.0
     *     obs_y_height = 8.0
     */

    int intercept_cnt = -1;
    int intercept_point_flag = -1;

    /* per our diagrams we are just solving for k in the complex
     * cooeficient quadratic */
    cplex_type k_val[2];

    /* be able to compute the dot product of -Ri and N */
    cplex_type dot_negRi_N;

    /* we are going to try a line plane interception and need a 
     * few data elements */
    vec_type line_point, line_direction, plane_point,
             plane_normal, plane_u, plane_v;

    vec_type plane_u_norm, plane_v_norm, lp_intercept_param;
    vec_type lp_intercept_point;

    int lp_status;

    /* for the sake of looking at the Cramer denominator det */
    cplex_type cramer_denom_det;

    vec_type obs_origin, obs_normal_dir, obs_normal, obs_point;
    vec_type hit_point, grad, grad_norm, neg_Ri, T, T_norm;

    vec_type x_prime_vec_dir, x_prime_vec;
    vec_type y_prime_vec_dir, y_prime_vec;
    vec_type tmp[8];

    /* Test case will be an observation plane at ( 12, 0, 0 ) */
    cplex_vec_set( &obs_origin, 12.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    /* Observation direction is along negative i_hat basis vector */
    cplex_vec_set( &obs_normal_dir, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    /* Try a rotation where the observation plane and viewport
     * are somewhere between the x and y axi but still sitting
     * in the xy-plane
     *
     *    cplex_vec_set( &obs_origin, 8.0, 0.0, 8.0, 0.0, 0.0, 0.0 );
     *
     * look towards the origin as per usual
     *
     *    cplex_vec_set( &obs_normal_dir, -1.0, 0.0, -1.0, 0.0, 0.0, 0.0 );
     */
    cplex_vec_normalize( &obs_normal, &obs_normal_dir );

    /* 8x8 viewport */
    obs_x_width = 8.0;
    obs_y_height = 8.0;

    /* TODO compute the reasonable plank constant of this
     * observation viewport. Strictly a lowest level epsilon
     * to be used for resolution. */

    /*
     * We will also require the coordinate basis vectors within
     * the observation plane.
     *
     *     obs_x_hat_prime = < 0, 1, 0 >
     *     obs_y_hat_prime = < 0, 0, 1 >
     *
     * It should be reasonably clear that a click of the mouse
     * within the active display region of our primary window
     * will result in an x_prime and y_prime pair. The Xll xlib
     * return mouse click coordinates referenced to the graphics
     * context and the window therein. This means we get possibly
     * negative coordinates from a mouse click that is left and
     * above the upper left corner of our grey window win.
     *
     */

    /* x_prime_vec is < 0, 1, 0 > */
    cplex_vec_set( &x_prime_vec_dir, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
    cplex_vec_normalize( &x_prime_vec, &x_prime_vec_dir );

    /* y_prime_vec is < 0, 0, 1 > */
    cplex_vec_set( &y_prime_vec, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    /*
     *         sign_data = < 1, 1, 1 >
     *
     *         object_location = < 0, 0, 0 >
     *
     *         semi_major_axi = < 5, 2, 6 >
     *
     *         ray_direct = obs_normal where this must be a
     *                           normalized vector
     *
     */
    vec_type sign_data, object_location, semi_major_axi, ray_direct;
    vec_type reflect;

    /* Within the set of signs Sx, Sy, and Sz we do not care about
     * the complex component and merely want the real. The same
     * may be said for object_location, semi_major_axi and the
     * direction of our ray ray_direct */
    cplex_vec_set( &sign_data, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0);

    /* by default we were using an object at the origin but we can
     * shift around for testing purposes. */
    cplex_vec_set( &object_location, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    /* Again the diagrams we used had a=5, b=2 and c=6 */
    cplex_vec_set( &semi_major_axi, 5.0, 0.0, 2.0, 0.0, 6.0, 0.0);

    /* We may need to change the sight line direction in the future
     * thus we will make a copy of the viewport plane normal vector */
    cplex_vec_copy( &ray_direct, &obs_normal );

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
    unsigned long gc2_bg = 0x0f0f00;
    win2 = create_borderless_topwin(dsp, 400, 330, 1070, 740, gc2_bg );
    gc2 = create_gc(dsp, win2);
    XSetBackground(dsp, gc2, gc2_bg);

    /* create another small window below that */
    unsigned long gc3_bg = 0x00000f;
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
    whatever.flags= DoRed | DoGreen | DoBlue;
    /* some dummy values */
    whatever.green = 0x0000;
    whatever.blue  = 0x0000;
    whatever.red   = 0xff00;

    if ( XAllocColor(dsp, screen_colormap, &whatever) == 0 ) {
        fprintf(stderr, "XAllocColor - gee .. whatever fail.\n");
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

    /* initial line characteristics on the gc are set in the XCreateGC
     * calls that we do.
     *
     * https://www.x.org/archive/X11R7.7/doc/
     *              man/man3/XSetLineAttributes.3.xhtml
     */

    /* set our graph box inside by OFFSET pixels
     *
     * reuse the offset_foo vars from above as we do not
     * need them for window location anymore. So we can
     * use them as interior offset distances for our plot.
     */
    offset_x = 10;
    offset_y = 10;

    /* upper left point */
    ux = offset_x;
    uy = offset_y;

    /* lower right point */
    lx = width - offset_x;
    ly = height - offset_y;
    fprintf(stdout,"     : ( %5i , %5i ) to ( %5i , %5i )\n",
                                                       ux, uy, lx, ly);


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


    /* horizontal minor tic marks at every 16th of the viewport
     * drawing area
     * printf("\n-------------------------------------------------\n");
     * printf("\n\nINFO : horizontal minor tic marks at every 16th\n");
     * p = 0;
     */

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
     * rendered without the burder of computing the entire viewport.
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

    /* display the ray_direct on gc3 as capital Ri cap */
    /* hack a litle vector cap over top of the letter R */
    XSetForeground(dsp, gc3, cornflowerblue.pixel);
    sprintf(buf,"^");
    XDrawImageString( dsp, win3, gc3, 9, 42, buf, strlen(buf));

    /* observation plane and viewport normalized direction are
     * the same as the ray direction.  At least for now. */
    sprintf(buf,"Ri = < %-+10.6e, %-+10.6e, %-+10.6e >",
                    ray_direct.x.r, ray_direct.y.r, ray_direct.z.r );

    XDrawImageString( dsp, win3, gc3, 10, 50, buf, strlen(buf));

    /* plot some points on the grid that we created */
    XSetForeground(dsp, gc, yellow.pixel);
    XDrawPoint(dsp, win, gc, 5, 5);


    /* before we have tears over our loved Deloris O'Reirdon from The Cranberries 
     *
     * ../complex_vector/check_dot.c 
     * */
     cplex_vec_set( &plane_point, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0);
     cplex_vec_set( &plane_normal, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0);
     cplex_vec_set( &plane_u, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 );
     cplex_vec_set( &plane_v, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );

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

                win_y = ( 1.0 * ( eff_height - mouse_y + offset_y ) )
                                                           / eff_height;

                /* lets try to invert the y axis */
                invert_mouse_x = mouse_x - offset_x;
                invert_mouse_y = eff_height - mouse_y + offset_y;
                sprintf(buf,"inv  [ %4i , %4i ]  ",
                                       invert_mouse_x, invert_mouse_y );

                XSetForeground(dsp, gc2, green.pixel);
                XDrawImageString( dsp, win2, gc2, 10, 230,
                                                      buf, strlen(buf));

                sprintf(buf,"fp64( %-10.8e , %-10.8e )", win_x, win_y );
                XDrawImageString( dsp, win2, gc2, 10, 250,
                                                      buf, strlen(buf));

                /* a more useful value is the vbox[] coordinates for
                 * each of the 16x16 grid we previously laid out */
                box_x = ( mouse_x - offset_x ) / vbox_w;
                box_y = ( eff_height - mouse_y + offset_y ) / vbox_h;
                sprintf(buf,"box  [ %03i , %03i ]", box_x, box_y );
                XDrawImageString( dsp, win2, gc2, 10, 270,
                                                      buf, strlen(buf));

                /* TODO dispatch out a worker thread to process all
                 * pixel data in the vbox[ box_x, box_y ] */

                /* Offset the floating point values such that the
                 * center point shall be ( 0.0, 0.0 ) */
                win_x = win_x * 2.0 - 1.0;
                win_y = win_y * 2.0 - 1.0;

                XSetForeground(dsp, gc2, red.pixel);
                sprintf(buf,"fp64( %-+10.8e , %-+10.8e )  ",
                                                          win_x, win_y );


                XDrawImageString( dsp, win2, gc2, 10, 290,
                                                       buf, strlen(buf));

                /* At this moment we have normalized values for a
                 * location within the observation viewport. We can
                 * scale those values by half of the viewport width
                 * and height to get actual x_prime and y_prime
                 * values.
                 *
                 * All of the above allows us to compute a starting
                 * L_0 point on the observation plane in R3 and in
                 * the coordinate system of the observation object.
                 *
                 * Note the x_prime = obs_x_width * win_x / 2.0
                 *          y_prime = obs_y_height * win_y / 2.0
                 *
                 * obs_point = x_prime * x_prime_vec
                 *           + y_prime * y_prime_vec
                 *           + obs_origin
                 *
                 */

                x_prime = obs_x_width * win_x / 2.0;
                y_prime = obs_y_height * win_y / 2.0;

                XSetForeground(dsp, gc3, cyan.pixel);

                sprintf(buf,"x', y' = ( %-10.8e , %-10.8e )  ",
                              x_prime, y_prime );

                fprintf(stderr,"\n%s\n",buf);

                XDrawImageString( dsp, win3, gc3, 10, 70,
                                  buf, strlen(buf));

                /* compute the observation point */
                cplex_vec_scale( tmp, &x_prime_vec, x_prime );
                cplex_vec_scale( tmp+1, &y_prime_vec, y_prime );
                cplex_vec_add( tmp+2, tmp, tmp+1);
                cplex_vec_add( tmp, tmp+2, &obs_origin );
                cplex_vec_copy( &obs_point, tmp );

                XSetForeground(dsp, gc3, cornflowerblue.pixel);

                sprintf(buf,"L = < %-8.6e, %-8.6e, %-8.6e >   ",
                        obs_point.x.r, obs_point.y.r, obs_point.z.r );

                fprintf(stderr,"\n%s\n",buf);

                XDrawImageString( dsp, win3, gc3, 10, 90,
                                  buf, strlen(buf));

                XSetForeground(dsp, gc3, cyan.pixel);

                /* time the computation of the intercepts etc */
                clock_gettime( CLOCK_MONOTONIC, &soln_t0 );

                intercept_cnt = -1;
                intercept_cnt = icept ( k_val, &sign_data,
                                &object_location, &semi_major_axi,
                                &obs_point, &obs_normal );

                fprintf(stderr,"\nintercept_cnt = %i\n", intercept_cnt );

                /* check that we have a valid intercept */
                if ( intercept_cnt > 0 ) {

                    /* note that this function returns a zero value
                     * if all goes well and we get an intercept point
                     * in R3 space. */
                    intercept_point_flag = surface_icept_pt(
                                  &hit_point, intercept_cnt, &k_val[0],
                                              &obs_point, &obs_normal);

                    if ( intercept_point_flag == 0 ) {
                        /* we have an intercept point H */
                        XDrawPoint(dsp, win, gc, mouse_x, mouse_y);

                        sprintf(buf,
                                "H = ( %-+10.6e, %-+10.6e, %-+10.6e )",
                                                   hit_point.x.r,
                                                   hit_point.y.r,
                                                   hit_point.z.r );

                        fprintf(stderr,"\n%s\n",buf);

                        XSetForeground(dsp, gc3, green.pixel);
                        XDrawImageString( dsp, win3, gc3, 10, 150,
                                          buf, strlen(buf));

                        /* surface gradient normal vector */
                        gradient( &grad, &sign_data, &object_location,
                                         &semi_major_axi, &hit_point );

                        cplex_vec_normalize( &grad_norm, &grad );

                        sprintf(buf,"^"); /* vector hat hack */
                        XSetForeground(dsp, gc3, cyan.pixel);
                        XDrawImageString( dsp, win3, gc3, 9, 162,
                                                       buf, (size_t)1);

                        sprintf(buf,
                                "N = < %-+10.6e, %-+10.6e, %-+10.6e )",
                                                   grad_norm.x.r,
                                                   grad_norm.y.r,
                                                   grad_norm.z.r );

                        fprintf(stderr,"\n%s\n",buf);

                        XDrawImageString( dsp, win3, gc3, 10, 170,
                                          buf, strlen(buf));

                        cplex_vec_scale( &neg_Ri, &ray_direct, -1.0 );
                        cplex_vec_dot( &dot_negRi_N, &neg_Ri, &grad_norm);

                        theta_i = acos(dot_negRi_N.r);

                        fprintf(stderr,"theta_i = %-+20.14e\n",
                                                             theta_i );

                        fprintf(stderr,"        = %-+20.14e degrees\n",
                                                theta_i * 180.0/M_PI );
          
                        if ( fabs(theta_i) < RT_ANGLE_EPSILON ) {
                            XSetForeground(dsp, gc3, red.pixel);
                        } else {
                            XSetForeground(dsp, gc3, magenta.pixel);
                        }

                        sprintf(buf,"theta_i = %-+20.14e", theta_i );

                        XDrawImageString( dsp, win3, gc3, 10, 190,
                                                   buf, strlen(buf));

                        sprintf(buf,"theta_i = %-+20.14e degrees",
                                              theta_i * 180.0/M_PI );

                        XDrawImageString( dsp, win3, gc3, 10, 210,
                                                   buf, strlen(buf));

                        cplex_vec_cross( &T, &neg_Ri, &grad_norm );
                        vec_T_mag = cplex_vec_mag( &T );

                        /* okay time for Cramer perhaps but we need to
                         * check for silly small angles and possible 
                         * null vector results from -Ri X N to throw
                         * the plane of incidence out the window. We
                         * should avoid that.
                         */

                        if ( ( vec_T_mag < RT_EPSILON )
                                ||
                             ( fabs(theta_i) < RT_ANGLE_EPSILON ) ) {
            
                            XSetForeground(dsp, gc3, red.pixel);
                            sprintf(buf," ");
                            XDrawImageString( dsp, win3, gc3, 9, 222,
                                                       buf, (size_t)1);

                            if ( vec_T_mag == 0.0 ) {
                                fprintf(stderr,"WARN : null vector\n");
                                sprintf(buf,"T = < 0.0, 0.0, 0.0 >");
                                strncat(buf,"            ",(size_t)18);
                            } else {
                                fprintf(stderr,"WARN : tiny vector result from -Ri x N\n");
                                sprintf(buf,"T = < %-+10.6e, %-+10.6e, %-+10.6e >",
                                                                     T.x.r, T.y.r, T.z.r );
                            }

                            XDrawImageString( dsp, win3, gc3, 10, 230,
                                                    buf, strlen(buf) );
            
                            sprintf(buf,"det = zero determinant                         ");

                            fprintf(stderr,"%s\n",buf);
                            XDrawImageString( dsp, win3, gc3, 10, 250, buf, strlen(buf) );

                            /* At this point there will be no solution using Cramer's
                             * method as the denominator matrix will be determinant
                             * of zero. However geometrically we may say that the
                             * reflected vector is the same as the normal N. */
                            cplex_vec_copy( &reflect, &grad_norm );
                            fprintf(stderr,"ASSM : Rr = < %-20.14e, %-20.14e, %-20.14e > ??\n",
                                             reflect.x.r, reflect.y.r, reflect.z.r);
                            fprintf(stderr,"     : this is just the surface gradient N\n");
            
                            sprintf(buf,
                                "Rr = < %-+10.6e, %-+10.6e, %-+10.6e >",
                                       reflect.x.r, reflect.y.r, reflect.z.r );
                            XDrawImageString( dsp, win3, gc3, 10, 270,
                                                             buf, strlen(buf) );

                            /* TODO : regardless of the assumption that R_r vector is in
                             * fact the same as the -R_i we may need to deal with an line
                             * plane interception. */

                        } else {

                            /* Cramer's Method possible */
                            cplex_vec_normalize( &T_norm, &T );

                            sprintf(buf,"^"); /* hat trick */
                            XDrawImageString( dsp, win3, gc3, 9, 222, buf, (size_t)1);
                            sprintf(buf,"T = < %-+10.6e, %-+10.6e, %-+10.6e >",
                                       T_norm.x.r, T_norm.y.r, T_norm.z.r );
                            XDrawImageString( dsp, win3, gc3, 10, 230,
                                                             buf, strlen(buf) );
                            /*
                             * We need a left hand 3x3 matrix of rows T, N, -Ri
                             * Right hand column needs 0,  Cos ( theta_i )  and
                             * lastly cos ( 2 x theta_i ).
                             */
                            cplex_vec_copy( tmp+3, &T_norm );  /*  T  */
                            cplex_vec_copy( tmp+4, &grad_norm );  /*  N  */
                            cplex_vec_copy( tmp+5, &neg_Ri );  /* -Ri */

                            cplex_det( &cramer_denom_det, tmp+3, tmp+4, tmp+5 );

                            sprintf(buf,"det = ( %-+14.10e, %-+14.10e )",
                                        cramer_denom_det.r, cramer_denom_det.i);

                            fprintf(stderr,"%s\n",buf);

                            XDrawImageString( dsp, win3, gc3, 10, 250,
                                                             buf, strlen(buf) );

                            cplex_vec_set( tmp+6, 0.0, 0.0,
                                           dot_negRi_N.r, 0.0,
                                           cos(2.0 * theta_i), 0.0);

                            status = cplex_cramer( tmp+7, tmp+3, tmp+4, tmp+5, tmp+6 );

                            if ( status != 0 ) {

                                fprintf(stderr,"dbug : There is no valid solution.\n");

                            } else {
            
                                sprintf(buf,
                                    "Rr = < %-+10.6e, %-+10.6e, %-+10.6e >",
                                           tmp[7].x.r, tmp[7].y.r, tmp[7].z.r );

                                XDrawImageString( dsp, win3, gc3, 10, 270,
                                                             buf, strlen(buf) );

                                fprintf(stderr,"Rr = < ( %-+20.14e, %-+20.14e ),\n",
                                             tmp[7].x.r, tmp[7].x.i );
                                fprintf(stderr,"       ( %-+20.14e, %-+20.14e ),\n",
                                             tmp[7].y.r, tmp[7].y.i );
                                fprintf(stderr,"       ( %-+20.14e, %-+20.14e ) >\n\n",
                                             tmp[7].z.r, tmp[7].z.i);

                                /* TODO we need to hack in a line plane intercept 
                                 * situation. Let us hack in a sample plane at the
                                 * y=10 location and give it a normal vector as -j
                                 * which is < 0, -1, 0 >.  This opens the problem 
                                 * of what "face" of a plane we are dealing with. */

                                cplex_vec_set( &line_point, hit_point.x.r, 0.0, hit_point.y.r, 0.0, hit_point.z.r, 0.0);
                                cplex_vec_set( &line_direction, tmp[7].x.r, 0.0, tmp[7].y.r, 0.0, tmp[7].z.r, 0.0);
                            
                                lp_status = line_plane_icept( &lp_intercept_point,
                                                              &plane_u_norm, &plane_v_norm,
                                                              &lp_intercept_param,
                                                              &line_point, &line_direction,
                                                              &plane_point, &plane_normal,
                                                              (vec_type*)(NULL), &plane_v);
                            
                                printf("     : line_plane_icept() returns %i\n\n", lp_status);
                            
                                if ( lp_status != 0 ) {
                                    printf("     : line plane intercept = ( %-+16.9e, %-+16.9e, %-+16.9e )\n",
                                                             lp_intercept_point.x.r,
                                                             lp_intercept_point.y.r,
                                                             lp_intercept_point.z.r);

                                    printf("     : plane_u norm = <  %g  ,  %g  ,  %g  >\n",
                                                              plane_u_norm.x.r, 
                                                              plane_u_norm.y.r, 
                                                              plane_u_norm.z.r ); 

                                    printf("     : plane_v norm = <  %g  ,  %g  ,  %g  >\n",
                                                              plane_v_norm.x.r, 
                                                              plane_v_norm.y.r, 
                                                              plane_v_norm.z.r ); 

                                    /*TODO lets find the coordinates within the plane */

                                }
                            }
                        }
                    }
                } else {

                    /* we have no intercept and no surface normal
                     * so clear those areas on gc3 */
                    fprintf(stderr,"\n we have no intercept and no surface normal\n");
                    XSetForeground(dsp, gc3, red.pixel);
                    sprintf(buf,"i = no intercept point                             ");
                    XDrawImageString( dsp, win3, gc3, 10, 150, buf, strlen(buf));
                    sprintf(buf,"^"); /* vector hat hack */
                    XDrawImageString( dsp, win3, gc3, 9, 162, buf, strlen(buf));
                    sprintf(buf,"N = no possible gradient normal vector             ");
                    XDrawImageString( dsp, win3, gc3, 10, 170, buf, strlen(buf));
                    sprintf(buf,"theta_i = does not exist                 ");
                    XDrawImageString( dsp, win3, gc3, 10, 190, buf, strlen(buf));
                    XDrawImageString( dsp, win3, gc3, 10, 210, buf, strlen(buf));
                    sprintf(buf,"T = does not exist                                 ");
                    XDrawImageString( dsp, win3, gc3, 10, 230, buf, strlen(buf));
                    sprintf(buf,"det =                                               ");
                    XDrawImageString( dsp, win3, gc3, 10, 250, buf, strlen(buf));
                    sprintf(buf,"Rr = does not exist                                 ");
                    XDrawImageString( dsp, win3, gc3, 10, 270, buf, strlen(buf));

                }

                clock_gettime( CLOCK_MONOTONIC, &soln_t1 );

                t_delta = timediff( soln_t0, soln_t1 );
                sprintf(buf,"[soln] = %16lld nsec", t_delta);
                fprintf(stderr,"%s\n",buf);
                XSetForeground(dsp, gc3, green.pixel);
                XDrawImageString( dsp, win3, gc3, 10, 290,
                                  buf, strlen(buf));

                t_ray_total += t_delta;

                t_sample_count += 1;

                sprintf(buf,"         %16lld avg nsec", t_ray_total/t_sample_count);
                fprintf(stderr,"%s\n",buf);

                XDrawImageString( dsp, win3, gc3, 10, 310, buf, strlen(buf));

                XSetForeground(dsp, gc3, cyan.pixel);

                sprintf(buf,"root 0 = ( %-+10.6e + %-+10.6e i )",
                        k_val[0].r, k_val[0].i );

                XDrawImageString( dsp, win3, gc3, 10, 110,
                                  buf, strlen(buf));

                sprintf(buf,"root 1 = ( %-+10.6e + %-+10.6e i )",
                        k_val[1].r, k_val[1].i );

                XDrawImageString( dsp, win3, gc3, 10, 130,
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

                    whatever.pixel = ( ( (unsigned long)p & 0xff ) << 16 )
                                     + ( ( (unsigned long)radius ) << 8 );

                    XSetForeground(dsp, gc, whatever.pixel);

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

        /* printf(" at %d %d \n", mouse_x, mouse_y); */

    }

    XCloseDisplay(dsp);

    printf("\n");

    free(buf);
    return EXIT_SUCCESS;
}

