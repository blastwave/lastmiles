
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
#include <complex.h>

#include "v.h"

void check_status( int status );

int main ( int argc, char **argv)
{

    cplex_type op1, op2, op3, opr, opr2[3], quad_res[4];

    /* rh_col is right hand column for Cramer call with
     * res_vec as the result if it exists */
    vec_type v[3], rh_col, res_vec;
    int status;

    op1.i = 1.0; op1.r = 0.0;
    op2.i = 1.0; op2.r = 0.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     : op2 = ( %g, %g )\n", op2.r, op2.i);
    check_status( cplex_mult( &opr, &op1, &op2 ) );
    printf("     : opr = op1 * op2 = ( %g, %g )\n", opr.r, opr.i);
    printf("     :     should be i^2 = -1\n\n");


    op1.r = 1.0;
    op1.i = 0.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     : op2 = ( %g, %g )\n", op2.r, op2.i);
    check_status( cplex_mult( &opr, &op1, &op2 ) );

    printf("     : opr = op1 * op2 = ( %g, %g )\n", opr.r, opr.i);
    printf("     :     should be 1i\n\n");

    op1.r = -1.0; op1.i = 0.0;
    op2.r = 0.0; op2.i = 0.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     : op2 = ( %g, %g )\n", op2.r, op2.i);
    check_status( cplex_mult( &opr, &op1, &op2 ) );
    printf("     : opr = op1 * op2 = ( %g, %g )\n", opr.r, opr.i);
    printf("     :     should be just zero.\n\n");

    op1.r = 4.0;
    op1.i = 3.0;
    op2.r = 2.0;
    op2.i = -1.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     : op2 = ( %g, %g )\n", op2.r, op2.i);
    check_status( cplex_div( &opr, &op1, &op2 ) );
    printf("dbug : opr = op1 / op2 = ( %g, %g )\n", opr.r, opr.i);
    printf("     :     should be 1 + 2i\n\n");

    op2.r = -4.0;
    op2.i = -12.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     : op2 = ( %g, %g )\n", op2.r, op2.i);
    check_status( cplex_div( &opr, &op1, &op2 ) );
    printf("dbug : opr = op1 / op2 = ( %g, %g )\n", opr.r, opr.i);
    printf("     :     should be -0.325 + 0.225i\n\n");

    op1.r = 2.0;
    op1.i = 1.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     :     theta = %16.12e\n\n", cplex_theta( &op1 ) );

    op1.r = 4.0;
    op1.i = 3.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     :     theta = %16.12e\n", cplex_theta( &op1 ) );

    check_status( cplex_sq( &opr, &op1 ) );
    printf("     : opr = op1^2 = ( %g, %g )\n", opr.r, opr.i);
    printf("     :     should be 7 + 24i\n");
    printf("     :     magnitude is %g\n", cplex_mag(&opr));
    printf("     :     theta = %16.12e\n", cplex_theta( &opr));

    printf("     : now we take the square root(s) of opr\n");
    op1.r = opr.r;
    op1.i = opr.i;
    double complex z = op1.r + op1.i * I;
    check_status( cplex_sqrt( opr2, &op1 ) );
    printf("root : 1 is ( %16.12e, %16.12e )\n", opr2[0].r, opr2[0].i);
    printf("root : 2 is ( %16.12e, %16.12e )\n", opr2[1].r, opr2[1].i);
    double complex zr = csqrt(z);
    printf("     : csqrt returns ( %16.12e, %16.12e )\n",
                                                creal(zr), cimag(zr) );
    printf("     : we should get back ( 4, 3i ) and ( -4, -3i ).\n\n");

    /* square root of ( 0, 1 ) */
    printf("dbug : square root test\n");
    op1.r = 0.0;
    op1.i = 1.0;
    printf("dbug : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     :     theta = %16.12e\n", cplex_theta(&op1) );
    printf("     :     magnitude is %g\n", cplex_mag(&op1));
    check_status( cplex_sqrt( opr2, &op1 ) );
    printf("root : 1 = ( %16.12e, %16.12e )\n", opr2[0].r, opr2[0].i);
    printf("root : 2 = ( %16.12e, %16.12e )\n\n", opr2[1].r, opr2[1].i);

    /* cube roots of ( -11 + 2i ) */
    printf("dbug : cube root test\n");
    op1.r = -11.0;
    op1.i = 2.0;
    printf("     : op1 = ( %g, %g )\n", op1.r, op1.i);
    printf("     :     theta = %16.12e\n", cplex_theta(&op1) );
    printf("     :     magnitude is %g\n", cplex_mag(&op1));

    check_status( cplex_cbrt( opr2, &op1 ) );
    printf("root : 1 = ( %16.12e, %16.12e )\n", opr2[0].r, opr2[0].i);
    printf("root : 2 = ( %16.12e, %16.12e )\n", opr2[1].r, opr2[1].i);
    printf("root : 3 = ( %16.12e, %16.12e )\n\n", opr2[2].r, opr2[2].i);

    printf("Lets test vector dot product\n");
    /* < (1 + 1i), (2 + 2i), (3+3i) >
     *                    . <( -1 - 1i), ( -2 -2i), ( 3 - 3i ) >
     *
     *
     * should be ( 18, -10i ) */

    /* first vector is < (1 + 1i), (2 + 2i), (3+3i) > */
    v[0].x.r = 1.0; v[0].x.i = 1.0;
    v[0].y.r = 2.0; v[0].y.i = 2.0;
    v[0].z.r = 3.0; v[0].z.i = 3.0;
    printf("dbug : v1 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[0].x.r, v[0].x.i,
            v[0].y.r, v[0].y.i,
            v[0].z.r, v[0].z.i );

    /* second vector is <( -1 - 1i), ( -2 -2i), ( 3 - 3i ) > */
    v[1].x.r = -1.0; v[1].x.i = -1.0;
    v[1].y.r = -2.0; v[1].y.i = -2.0;
    v[1].z.r =  3.0; v[1].z.i = -3.0;
    printf("     : v2 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[1].x.r, v[1].x.i,
            v[1].y.r, v[1].y.i,
            v[1].z.r, v[1].z.i );

    check_status( cplex_vec_dot( &opr, v, v+1) );
    printf("     : dot product = ( %g, %g )\n\n", opr.r, opr.i);


    /* tw0st3p says < 12 + 12im, -6 - 6im, 0 + 0im > */
    check_status( cplex_vec_cross( v+2, v, v+1 ) );
    printf("     : v1 X v2 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[2].x.r, v[2].x.i,
            v[2].y.r, v[2].y.i,
            v[2].z.r, v[2].z.i );


    op1.r = 1.0; op1.i = 0.0;
    op2.r = -9.0; op2.i = 0.0;
    op3.r = 14.0; op3.i = 0.0;
    check_status( cplex_quadratic( quad_res, &op1, &op2, &op3 ) );
    printf("Quadratic result 1 = ( %16.12e, %16.12e )\n",
                                          quad_res[0].r, quad_res[0].i);
    printf("          result 2 = ( %16.12e, %16.12e )\n\n",
                                          quad_res[1].r, quad_res[1].i);

    op1.r = 1.0; op1.i = 0.0;
    op2.r = 5.0; op2.i = 0.0;
    op3.r = -14.0; op3.i = 0.0;
    check_status( cplex_quadratic( quad_res, &op1, &op2, &op3 ) );
    printf("Quadratic result 1 = ( %16.12e, %16.12e )\n",
                                          quad_res[0].r, quad_res[0].i);
    printf("          result 2 = ( %16.12e, %16.12e )\n\n",
                                          quad_res[1].r, quad_res[1].i);


    op1.r = 1.0; op1.i = 0.0;
    op2.r = -5.0; op2.i = 0.0;
    op3.r = 14.0; op3.i = 0.0;
    check_status( cplex_quadratic( quad_res, &op1, &op2, &op3 ) );
    printf("Quadratic result 1 = ( %16.12e, %16.12e )\n",
                                          quad_res[0].r, quad_res[0].i);
    printf("          result 2 = ( %16.12e, %16.12e )\n\n",
                                          quad_res[1].r, quad_res[1].i);


    op1.r = 2.0;  op1.i = 3.0;
    op2.r = -5.0; op2.i = 2.0;
    op3.r = -1.0; op3.i = -7.0;
    check_status( cplex_quadratic( quad_res, &op1, &op2, &op3 ) );
    printf("Quadratic result 1 = ( %16.12e, %16.12e )\n",
                                          quad_res[0].r, quad_res[0].i);
    printf("          result 2 = ( %16.12e, %16.12e )\n\n",
                                          quad_res[1].r, quad_res[1].i);


    /* Determinant of three row matrix */
    v[0].x.r = 1.0; v[0].x.i = 0.0;
    v[0].y.r = 2.0; v[0].y.i = 0.0;
    v[0].z.r = 3.0; v[0].z.i = 0.0;
    printf("dbug : row 1 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[0].x.r, v[0].x.i,
            v[0].y.r, v[0].y.i,
            v[0].z.r, v[0].z.i );

    /* second row */
    v[1].x.r = 4.0; v[1].x.i = 0.0;
    v[1].y.r = 5.0; v[1].y.i = 0.0;
    v[1].z.r = 6.0; v[1].z.i = 0.0;
    printf("     : row 2 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[1].x.r, v[1].x.i,
            v[1].y.r, v[1].y.i,
            v[1].z.r, v[1].z.i );

    /* third row */
    v[2].x.r = 7.0; v[2].x.i = 0.0;
    v[2].y.r = 8.0; v[2].y.i = 0.0;
    v[2].z.r = 9.0; v[2].z.i = 0.0;
    printf("     : row 3 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[2].x.r, v[2].x.i,
            v[2].y.r, v[2].y.i,
            v[2].z.r, v[2].z.i );

    check_status( cplex_det( &opr, &v[0], &v[1], &v[2] ) ); 
    printf("     : det = ( %g, %g )\n", opr.r, opr.i);

    printf("\n\nNew Row data on row1 \n");
    v[0].x.r = 10.0; v[0].x.i = 0.0;
    v[0].y.r = -2.0; v[0].y.i = 0.0;
    v[0].z.r = -3.0; v[0].z.i = 0.0;
    printf("dbug : row 1 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[0].x.r, v[0].x.i,
            v[0].y.r, v[0].y.i,
            v[0].z.r, v[0].z.i );
    printf("     : row 2 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[1].x.r, v[1].x.i,
            v[1].y.r, v[1].y.i,
            v[1].z.r, v[1].z.i );
    printf("     : row 3 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[2].x.r, v[2].x.i,
            v[2].y.r, v[2].y.i,
            v[2].z.r, v[2].z.i );

    check_status( cplex_det( &opr, &v[0], &v[1], &v[2] ) ); 
    printf("     : det = ( %g, %g )\n", opr.r, opr.i);

    printf("\n\nComplex row data in row1 and row 3\n");
    v[0].x.r = 0.5; v[0].x.i = -1.0;
    printf("dbug : row 1 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[0].x.r, v[0].x.i,
            v[0].y.r, v[0].y.i,
            v[0].z.r, v[0].z.i );
    v[2].y.r = -2.0; v[2].y.i = 4.0;
    printf("     : row 2 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[1].x.r, v[1].x.i,
            v[1].y.r, v[1].y.i,
            v[1].z.r, v[1].z.i );
    printf("     : row 3 = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
            v[2].x.r, v[2].x.i,
            v[2].y.r, v[2].y.i,
            v[2].z.r, v[2].z.i );

    check_status( cplex_det( &opr, &v[0], &v[1], &v[2] ) ); 
    printf("     : det = ( %g, %g )\n", opr.r, opr.i);

    /*
     *
     *  Solve for A,B,C where
     *  ( 0.5 - i ) * A  - 2 * B - 3 * C = ( 1 + 0.5i ),
     *  4 * A + 5 * B + 6 * C = ( 2 + 0.75i ),
     *  7 * A + ( -2 + 4i ) * B + 9 * C = ( 3 -0.25i ).
     *
     */
    printf("\n\nCramers rule test with existing matrix and\n");
    rh_col.x.r = 1.0; rh_col.x.i = 0.5;
    rh_col.y.r = 2.0; rh_col.y.i = 0.75;
    rh_col.z.r = 3.0; rh_col.z.i = -0.25;
    printf("rh_col = < ( %g, %g ), ( %g, %g ), ( %g, %g ) >\n",
        rh_col.x.r, rh_col.x.i,
        rh_col.y.r, rh_col.y.i,
        rh_col.z.r, rh_col.z.i);

    status = cplex_cramer( &res_vec, &v[0], &v[1], &v[2], &rh_col );
    if ( status != 0 ) {
        printf("dbug : There is no valid solution.\n");
    } else {
        printf("     : result col = < ( %16.12e, %16.12e ),\n",
                    res_vec.x.r, res_vec.x.i );
        printf("                      ( %16.12e, %16.12e ),\n",
                    res_vec.y.r, res_vec.y.i );
        printf("                      ( %16.12e, %16.12e ) >\n\n",
                    res_vec.z.r, res_vec.z.i);
    }

    return ( EXIT_SUCCESS );

}

