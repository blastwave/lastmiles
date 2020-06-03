
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
#include "v.h"

/* return the magnitude of op1 */
double cplex_mag( cplex_type *op1 )
{
    /* TODO we need to change this to a two parameter function
     *      wherein the return value of the magnitude is in
     *      a point to a double whereas the function itself
     *      returns a math status MATH_OP_SUCCESS.
     *
     *      Is this even a sane idea ? 
     */

    double hyp = op1->r * op1->r + op1->i * op1->i;
    return sqrt( hyp );

}

