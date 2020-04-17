
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include "v.h"

int cplex_mat3x3_print( vec_type *row1, vec_type *row2, vec_type *row3 );

int main ( int argc, char **argv)
{

    vec_type tmp[12];
    cplex_type c_tmp[4];
    vec_type grad, grad_norm, reflect;
    double vec_T_mag, theta_i;
    int status, intercept_cnt = -1;

    /* https://github.com/blastwave/lastmiles/blob/master/ray_trace/
     *                                  math_notes/notes_rt_math_006.png
     *
     * per our diagrams we are just solving for k in the complex
     * coefficient quadratic */
    cplex_type k_val[2];

    /* If we actually do get an intercept then we need to determine
     * the closest forward looking point which is our actual point
     * of intercept H.
     * We can call that the hit_point just to be consistent with
     * the diagrams thus far where we use H and hit_point to mean
     * the actual intercept. */
    vec_type hit_point;

    /* We must first define where our observation plane is in R3
     * as a point and a normal direction. The obs_normal direction
     * will be the direction of all rays that we trace. */
    vec_type obs_origin, obs_normal;

    /* The observation plane has its own coordinate system and
     * thus we have x_prime_hat_vec and y_prime_hat_vec as ortogonal
     * and normalized vectors. See notes_rt_math_004_m.png */
    vec_type x_prime_hat_vec, y_prime_hat_vec;

    /* To locate a given point L_0 on the observation plane we will
     * need scalar distances away from the observation plane
     * center along the directions x_prime_hat_vec and
     * y_prime_hat_vec */
    double x_prime, y_prime;

    /* All of the above merely allows us to compute a starting
     * point for our ray to trace along the direction of the
     * vector obs_normal. In the diagrams this may be called L_0
     * on the observation plane. */
    vec_type obs_point;

    /* Test case will be an observation plane at ( 12, 0, 0 ) */
    cplex_vec_set( &obs_origin, 12.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    printf("\nINFO : viewport O obs_origin = ");
    printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                      obs_origin.x.r, obs_origin.y.r, obs_origin.z.r );

    /* Observation direction is along negative i_hat basis vector
     * and this becomes the Ri later. */
    cplex_vec_set( &obs_normal, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    printf("\nINFO : viewport direction obs_normal = ");
    printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                      obs_normal.x.r, obs_normal.y.r, obs_normal.z.r );

    /* we arbitrarily choose the x_prime_hat_vec and y_prime_hat_vec */
    /* x_prime_hat_vec is < 0, 1, 0 > */
    /* y_prime_hat_vec is < 0, 0, 1 > */
    cplex_vec_set( &x_prime_hat_vec, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
    cplex_vec_set( &y_prime_hat_vec, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    printf("     : viewport x_prime basis vector = ");
    printf("< %-20.14e, %-20.14e, %-20.14e >\n",
       x_prime_hat_vec.x.r, x_prime_hat_vec.y.r, x_prime_hat_vec.z.r );

    printf("     : viewport y_prime basis vector = ");
    printf("< %-20.14e, %-20.14e, %-20.14e >\n",
       y_prime_hat_vec.x.r, y_prime_hat_vec.y.r, y_prime_hat_vec.z.r );

    /* A test point to begin with on the observation plane.
     *
     *   x_prime = 1.7;
     *   y_prime = -2.0;
     */

    x_prime = 1.7;
    y_prime = -2.0;

    printf("INFO : initial x' and y' : ( %-20.14e, %-20.14e )\n\n",
                                                    x_prime, y_prime );

    /* All of the above allows us to compute a starting point on
     * the observation plane in R3 and in the coordinate system
     * of the observation object.  This is also called L_0 in the
     * various diagrams.
     *
     * obs_point = x_prime * x_prime_hat_vec
     *           + y_prime * y_prime_hat_vec
     *           + obs_origin
     *
     */
    cplex_vec_scale( tmp,   &x_prime_hat_vec, x_prime );
    cplex_vec_scale( tmp+1, &y_prime_hat_vec, y_prime );

    cplex_vec_add( tmp+2, tmp, tmp+1);
    cplex_vec_add( tmp, tmp+2, &obs_origin );

    cplex_vec_copy( &obs_point, tmp );

    printf("\nINFO : L obs_point = ");
    printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                      obs_point.x.r, obs_point.y.r, obs_point.z.r );
    printf("\n\n");

    /* At this moment we have the observation point and the direction
     * of the plane within obs_normal. What we need is to pass all
     * this to an intercept function that will determine a
     * k index value on the ray trace line.
     *
     * See page 6 of the notes on github at :
     *     https://github.com/blastwave/lastmiles/ray_trace/math_notes
     *
     * So clearly we need :
     *
     *     Our L point is obs_point along with a ray_direction.
     *     We shall need the sign_data, object_location and the
     *     semi_major_axi of that ellipsoid.
     *
     *     At the moment for this test we shall use :
     *
     *         sign_data = < 1, 1, 1 >
     *
     *         object_location = < 0, 0, 0 >
     *
     *         semi_major_axi = < 5, 2, 6 >
     *
     *         ray_direct = obs_normal where this must be a
     *                           normalized vector
     */
    vec_type sign_data, object_location, semi_major_axi, ray_direct;

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

    /* Note that the ray direction must be normalized */
    status = cplex_vec_normalize( &ray_direct, &obs_normal );
    if ( status == EXIT_FAILURE ) return ( EXIT_FAILURE );

    printf("INFO : ray_direct = ");
    printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                      ray_direct.x.r, ray_direct.y.r, ray_direct.z.r );
    printf("\n\n");

    /* Now we call our intercept function to do most of the work */
    intercept_cnt = icept ( k_val, &sign_data,
                                &object_location, &semi_major_axi,
                                &obs_point, &obs_normal );

    printf("intercept_cnt = %i\n", intercept_cnt );
    printf("\nINFO : k_val[0] = ( %-20.14e, %-20.14e )\n",
                           k_val[0].r, k_val[0].i );
    printf("     : k_val[1] = ( %-20.14e, %-20.14e )\n",
                           k_val[1].r, k_val[1].i );

    if ( intercept_cnt > 0 ) {

        if ( surface_icept_pt( &hit_point, intercept_cnt,
                               &k_val[0], &obs_point,
                               &ray_direct) == 0 ) {

            printf("\nDBUG : We have a good intercept point\n");
            printf("INFO : H hit_point = ");
            printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                      hit_point.x.r, hit_point.y.r, hit_point.z.r );
            printf("\n");

            gradient( &grad,
                      &sign_data, &object_location,
                      &semi_major_axi, &hit_point );

            printf("\n------------------------------------------\n");
            printf("INFO : N gradient = ");
            printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                              grad.x.r, grad.y.r, grad.z.r );

            status = cplex_vec_normalize( &grad_norm, &grad );
            if ( status == EXIT_FAILURE ) return ( EXIT_FAILURE );
            printf("     : normalized = ");
            printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                       grad_norm.x.r, grad_norm.y.r, grad_norm.z.r );

            /* we should attempt to compute the T tangent vector in
             * the plane of incidence if and only if N is not parallel
             * to the incident ray_direct. We use -Ri for our vector
             * due to right-hand rule of the supposedly physical
             * universe. So T == -Ri X N here. */
            cplex_vec_scale( tmp+3, &ray_direct, -1.0 );
            printf("\nINFO : -Ri = < %-20.14e, %-20.14e, %-20.14e >\n",
                               tmp[3].x.r, tmp[3].y.r, tmp[3].z.r );

            /* what is the angle of incidence ?
             * Here we can take advantage of IEEE754-2008 specification
             * just to check for a zero that is positive or negative. */
            cplex_vec_dot( c_tmp, &grad_norm, tmp+3);
            if ( !(c_tmp->i == 0.0) ) {
                /* this should never happen */
                fprintf(stderr,"FAIL : bizarre complex dot product");
                fprintf(stderr," dot( N, -Ri )\n");
                fprintf(stderr,"     :  = ( %-20.14e, %-20.14e )\n",
                                                   c_tmp->r, c_tmp->i );
                fprintf(stderr,"BAIL : we are done.\n\n");
                return ( EXIT_FAILURE );
            } else {
                printf("     : dot( N, -Ri ) = %-20.14e\n", c_tmp->r );
            }

            theta_i = acos(c_tmp->r);
            printf("     : theta_i = %-20.14e\n", theta_i );
            printf("     :         = %-20.14e degrees\n", theta_i * 180.0/M_PI );

            if ( fabs(theta_i) < RT_ANGLE_EPSILON ) {
                if ( theta_i == 0.0 ) {
                    fprintf(stderr,"WARN : theta_i is zero!\n");
                } else {
                    fprintf(stderr,"WARN : theta_i too small.\n");
                }
            }

            cplex_vec_cross( tmp+4, tmp+3, &grad_norm );
            status = cplex_vec_normalize( &ray_direct, &obs_normal );
            if ( status == EXIT_FAILURE ) return ( EXIT_FAILURE );

            printf("\n\nINFO : -Ri X N = ");
            printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                               tmp[4].x.r, tmp[4].y.r, tmp[4].z.r );

            vec_T_mag = cplex_vec_mag( tmp+4 );
            if ( ( vec_T_mag < RT_EPSILON )
                    ||
                 ( fabs(theta_i) < RT_ANGLE_EPSILON ) ) {

                if ( vec_T_mag == 0.0 ) {
                    printf("WARN : null vector result from -Ri x N\n");
                } else {
                    printf("WARN : tiny vector result from -Ri x N\n");
                }

                /* At this point there will be no solution using Cramer's
                 * method as the denominator matrix will be determinant
                 * of zero. However geometrically we may say that the
                 * reflected vector is the same as the normal N. */
                 cplex_vec_copy( &reflect, &grad_norm );
                 printf("INFO : Rr = < %-20.14e, %-20.14e, %-20.14e > ??\n",
                                 reflect.x.r, reflect.y.r, reflect.z.r);
                 printf("     : this is just the surface gradient N\n");

            } else {

                /* Cramer's Method and may as well embrace it */
                status = cplex_vec_normalize( tmp+5, tmp+4 );
                if ( status == EXIT_FAILURE ) return ( EXIT_FAILURE );
                printf("     : this is the plane of incidence tangent\n");
                printf("     : T = ");
                printf("< %-20.14e, %-20.14e, %-20.14e >\n",
                               tmp[5].x.r, tmp[5].y.r, tmp[5].z.r );
                printf("     : Cramer\'s Method needed from here\n");

                /*****************************************************
                 * We need a left hand 3x3 matrix of rows T, N, -Ri
                 *
                 *          T   is in tmp[5]        normalized
                 *          N   is in grad_norm     normalized
                 *        -Ri   is in tmp[3]        normalized
                 *
                 * Right hand column needs 0,  Cos ( theta_i )  and
                 * lastly cos ( 2 x theta_i ).
                 *
                 * Note that theta_i = acos( dot( -Ri, N ) )
                 * and also we have c_tmp[0] = dot( -Ri, N ) wherein
                 * we only need the real component.
                 *
                 * Thus we need rt hand column 0, c_tmp[0], 2*c_tmp[0]
                 *
                 **/

                /* begin by copying the required data into tmp vectors
                 * for the left hand 3x3 matrix */

                 cplex_vec_copy( tmp+6, tmp+5 );  /*  T  */
                 cplex_vec_copy( tmp+7, &grad_norm );  /*  N  */
                 cplex_vec_copy( tmp+8, tmp+3 );  /* -Ri */

                 /* lets print out a simplified 3x3 matrix here */
                 printf ("\nLeft hand 3x3 matrix is :\n");
                 cplex_mat3x3_print( tmp+6, tmp+7, tmp+8 );

                 cplex_det( c_tmp+1, tmp+6, tmp+7, tmp+8 );

                 printf ("\nDET == ( %-+18.12e, %-+18.12e )\n\n", c_tmp[1].r, c_tmp[1].i );

                 /* slam together a right hand column with
                  * 0, cos(theta_i), cos(2 * theta_i) */
                 cplex_vec_set( tmp+9, 0.0, 0.0,
                                       c_tmp->r, 0.0,
                                       cos(2 * theta_i), 0.0);

                 printf ("     : rh_col = %-+18.12e, %-+18.12e, %-+18.12e\n",
                         tmp[9].x.r, tmp[9].y.r, tmp[9].z.r );

                 /* let us now call cramer */
                 status = cplex_cramer( tmp+10, tmp+6, tmp+7, tmp+8, tmp+9 );
                 if ( status != 0 ) {
                     printf("dbug : There is no valid solution.\n");
                 } else {

                     printf("     : result col = < ( %-+20.14e, %-+20.14e ),\n",
                                 tmp[10].x.r, tmp[10].x.i );
                     printf("                      ( %-+20.14e, %-+20.14e ),\n",
                                 tmp[10].y.r, tmp[10].y.i );
                     printf("                      ( %-+20.14e, %-+20.14e ) >\n\n",
                                 tmp[10].z.r, tmp[10].z.i);

                 }


            }

        } else {
            printf("INFO : no intercept point\n");
        }

    } else {
        printf("INFO : no real solutions\n");
    }

    return ( EXIT_SUCCESS );

}

int cplex_mat3x3_print( vec_type *row1,
                        vec_type *row2,
                        vec_type *row3 )
{

    printf ("    +------------------------");
    printf ("+------------------------");
    printf ("+------------------------+\n");

    printf ("    |                        ");
    printf ("|                        ");
    printf ("|                        |\n");

    printf (" T  |  %-+18.12e   ", row1->x.r );
    printf ("|  %-+18.12e   ", row1->y.r );
    printf ("|  %-+18.12e   |\n", row1->z.r );

    printf ("    |                        ");
    printf ("|                        ");
    printf ("|                        |\n");

    printf ("    +------------------------");
    printf ("+------------------------");
    printf ("+------------------------+\n");

    printf ("    |                        ");
    printf ("|                        ");
    printf ("|                        |\n");

    printf (" N  |  %-+18.12e   ", row2->x.r );
    printf ("|  %-+18.12e   ", row2->y.r );
    printf ("|  %-+18.12e   |\n", row2->z.r );

    printf ("    |                        ");
    printf ("|                        ");
    printf ("|                        |\n");

    printf ("    +------------------------");
    printf ("+------------------------");
    printf ("+------------------------+\n");

    printf ("    |                        ");
    printf ("|                        ");
    printf ("|                        |\n");

    printf ("-Ri |  %-+18.12e   ", row3->x.r );
    printf ("|  %-+18.12e   ", row3->y.r );
    printf ("|  %-+18.12e   |\n", row3->z.r );

    printf ("    |                        ");
    printf ("|                        ");
    printf ("|                        |\n");

    printf ("    +------------------------");
    printf ("+------------------------");
    printf ("+------------------------+\n\n");

    return ( EXIT_SUCCESS );

}

