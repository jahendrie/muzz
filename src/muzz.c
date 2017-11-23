/*******************************************************************************
 *  muzz.c  |   version 1.01    |   zlib license    |   2017-11-23
 *  James Hendrie               |   hendrie.james@gmail.com
 *  ----------------------------------------------------------------------------
 *
 *  This program is primarily intended to be used to calculate the muzzle
 *  energy of a projectile given its mass and velocity.  If the user wishes,
 *  they can instead get the mass (given velocity and energy) or velocity
 *  (given mass and energy).
 *
 *  The program uses both Imperial and Si (metric) units of measure, depending
 *  on which the user prefers.
 *
 *  The program can also output results using the 'Taylor Knockout Formula', if
 *  the user wishes to do so, but will not convert from the TKOF number to get
 *  mass, velocity, etc.  All TKOF calculations MUST be provided all three
 *  parameters:  Mass, velocity, diameter.  Again, these can be Imperial or Si.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>   //  Make sure to link with -lm

#define VERSION "1.01"

#define MAX_STR 255




/*  The parameter types we'll want */
enum ParametersGiven {
    PARAM_MASS,
    PARAM_VELOCITY,
    PARAM_ENERGY,
    TOTAL_VALUES_WANTED
};


/*  The options we're going to use later */
enum OptionsEnum {
    OPT_VERBOSE,
    OPT_SI,
    OPT_PRECISE,
    OPT_TKOF,
    OPT_K,
    TOTAL_OPTIONS
};


/*  
 *  Gravitational constant ( ( (N * (distance**2))/second) ** 2 )
 *
 *  This is the 'approximated' constant, apparently used in the small arms
 *  business when calculating lbf.
 */
//  Units of measure:  feet
const double GRAVITY_IMPERIAL_APPROX = 32.163;

//  Same but not approximated
const double GRAVITY_IMPERIAL = 32.1739;

/*
 *  Note that I'm not including the Si gravity accleration constant because
 *  the constant is used to calculate a number by which we divide other numbers.
 *  For imperial, K = ( 2 * (constant) * 7000 ), while for Si, K = 1000.
 *
 *  Also, for Imperial calculations we'll later assign K to 450240, which is
 *  the standard number used when not calculating with the gravity constant.
 *  It's very close to the result given when doing so, though I don't know
 *  where they get it precisely.
 */


/*  Value 'K' (constant by which we divide (mass*(velocity**2)) */
double K;



/*  ----------------------  Optstring   -------------------------------
 *  V   Version and author info
 *  h   Help
 *  E   Print usage examples
 *  U   Print information on units and numbers used
 *  S   Silent mode
 *  q   Same
 *  s   Use Si units of measure
 *  i   Imperial units of measure (default)
 *  m   'solve' for mass
 *  v   for velocity
 *  e   for energy (default)
 *  p   Be precise (do not floor results, do not ceiling results)
 *  t   Print results for Taylor Knockout Formula instead of standard
 *  c   'Small arms standard' for earth gravitational accel. constant
 *  C   Use non-approximated standard for earth gravitational accel. constant
 *  K   Use industry standard constant (do not calculate) (default)
 *  k   Custom constant
 */
static const char *optString = "VhEHSqsimveptcCKk:";




/*==============================================================================
                                  PRINT USAGE
--------------------------------------------------------------------------------
*   Prints how to use the program
*/
void print_usage( FILE *fp )
{
    fprintf( fp, "Usage:  muzz [OPTION] MASS VELOCITY [DIAMETER]\n" );
}


/*==============================================================================
                                   PRINT HELP
--------------------------------------------------------------------------------
*   Prints help text
*/
void print_help( void )
{
    print_usage( stdout );
    printf( "\nThe program is used primarily to calculate the muzzle energy ");
    printf( "of projectiles.\n\n" );

    printf( "Imperial gravity accleration constants (K = 2 * GAC * 7000) :\n" );
    printf( "GAC-1 (industry):  32.163\t" );
    printf( "GAC-2 (standard):  32.1739\n" );

    printf( "\nOptions\n" );

    printf( "  -h\t\tPrint this help text\n" );
    printf( "  -H\t\tPrint additional information on units used, etc.\n" );
    printf( "  -E\t\tPrint example usage\n" );
    printf( "  -V\t\tPrint version and author info\n" );

    printf("  -S or q\tSilent (quiet) mode; print only the resultant number\n");
    printf( "  -s\t\tUse Si (metric) units of measure - grams, m/s, joules\n" );
    printf( "  -i\t\tUse Imperial units - grains, ft/s, lbf (default)\n" );

    printf("  -m\t\tCalculate for mass (num1 = velocity, num2 = energy)\n");
    printf("  -v\t\tCalculate for velocity (num1 = mass, num2 = energy)\n");
    printf("  -e\t\tCalculate for energy (num1 = mass, num2 = velocity) ");
    printf("(default)\n" );

    printf( "  -K\t\tUse industry standard imperial constant (450,240) ");
    printf( " (default)\n" );
    printf( "  -k [num]\tCustom user constant\n" );
    printf( "  -c\t\tCalculate constant using 'industry' GAC-1\n" );
    printf( "  -C\t\tCalculate constant using standard GAC-2\n" );

    printf( "  -p\t\tBe precise (do not round any numbers)\n  " );
    printf("-t\t\tUse Taylor Knockout Formula (give mass, velocity, diameter)");
    putchar( '\n' );
}



/*==============================================================================
                                 PRINT EXAMPLES
--------------------------------------------------------------------------------
*   Prints a number of usage examples for the user.
*/
void print_examples( void )
{
    printf("Examples:\n\n" );

    printf( "muzz 230 900\n" );
    printf( "  Returns muzzle energy of a 230 grain bullet @ 900 ft/s\n" );

    printf( "\nmuzz -s 15 270\n" );
    printf( "  Using Si units of measure, returns joules (15grams @ 270 m/s)\n");

    printf( "\nmuzz -qp 230 900\n" );
    printf( "  Same, but only the number and with nothing rounded\n");

    printf( "\nmuzz -mq 900 414\n" );
    printf( "  Given the velocity and muzzle energy, it will return only the ");
    printf( "mass\n  of the projectile.\n" );

    printf( "\nmuzz -t 230 860 .45\n" );
    printf("  Prints result using Taylor Knockout Formula, with the params");
    printf( " being\n  the mass (grains), velocity (ft/s) and diameter\n" );

    printf( "\nmuzz -ts 15 255 11.6\n" );
    printf( "  Same, but using Si units (grams, meters/second, mm)\n");
}



/*==============================================================================
                             PRINT ADDITIONAL HELP
--------------------------------------------------------------------------------
*   Prints the units used and how results are calculated
*/
void print_additional_help( void )
{
    printf("All units of measure are Imperial by default.  U-S-A!  U-S-A!\n\n");

    printf( "Weight:\n" );
    printf( "  Si:\t\tGrams (g)\n" );
    printf( "  Imperial:\tGrains (gr) (7000 per pound)\n\n" );
    
    printf( "Velocity:\n" );
    printf( "  Si:\t\tMeters per second (m/s)\n" );
    printf( "  Imperial:\tFeet per second (ft/s)\n\n" );

    printf( "Diameter:\n" );
    printf( "  Si:\t\tMillimeters (mm)\n" );
    printf( "  Imperial:\tInch caliber (fractions of inch) (ex.: .45)\n\n" );

    printf( "Energy:\n" );
    printf( "  Si:\t\tJoules (J)\n" );
    printf( "  Imperial:\tFoot-pounds (lbf)\n\n" );

    printf( "\nTo calculate the standard muzzle energy of a projectile:\n\n" );
    printf( "  Si:\t\t( 2 * mass * (velocity*velocity)) / K\n" );
    printf( "  Imperial:\t( mass * (velocity*velocity)) / K\n\n" );

    printf( "  Default values of K are 450240 (Imperial) or 1000 (Si).\n" );
    printf( "  To use different numbers to calculate K, use the '-c' or '-C' ");
    printf( "options:\n" );
    printf( "    -c:\t\tK = 2 * 32.163 * 7000\n" );
    printf( "    -C:\t\tK = 2 * 32.1739 * 7000\n\n" );

    printf( "  You can also use the '-k' option to use a custom constant.\n\n");

    printf("The Taylor Knockout Formula, if used, will return a number that's");
    printf( " roughly the\n" );
    printf( "same regardless of whether or not the user chooses Si or ");
    printf( "Imperial units\n" );
    printf( "of measure.  The formula is as follows:\n\n" );
    
    printf( "  Si:\t\t( mass * velocity * diameter ) / 3500\n" );
    printf( "  Imperial:\t( mass * velocity * diameter ) / 7000\n\n" );
}



/*==============================================================================
                                 PRINT VERSION
--------------------------------------------------------------------------------
*   Print version and author info
*/
void print_version( void )
{
    printf( "muzz.c, version %s\n", (VERSION) );
    printf( "James Hendrie <hendrie.james@gmail.com>\n" );
}




/*==============================================================================
                                   GET ENERGY
--------------------------------------------------------------------------------
*   Returns the energy, given the mass and velocity of the projectile.
*
*   Params
*       double mass     |   Mass of the projectile
*       double velocity |   Velocity of the projectile
*       int si          |   Are we using Si units of measure
*/
double get_energy( double mass, double velocity, int si )
{
    /*  Si */
    if( si == 1 )
        return( (double)(( (mass/2.0f) * (velocity*velocity)) / K) );

    /*  Imperial */
    else
        return( (double)( ( mass * (velocity*velocity)) / K ));
}



/*==============================================================================
                                    GET MASS
--------------------------------------------------------------------------------
*   Returns the mass of the projectile, given the velocity and muzzle energy.
*
*   Params
*       double velocity |   Velocity of the projectile
*       double energy   |   Energy of the projectile
*       int si          |   Are we using Si units of measure
*/
double get_mass( double velocity, double energy, int si )
{
    /*  Si */
    if( si == 1 )
        return( (double)( ( (energy*2)/(velocity*velocity) ) * K ));

    /*  Imperial */
    else
        return( (double)( (energy/(velocity*velocity)) * K ));
}



/*==============================================================================
                                  GET VELOCITY
--------------------------------------------------------------------------------
*   Returns the velocity of the projectile, given the mass and muzzle energy.
*
*   Params
*       double mass     |   Mass of the projectile
*       double energy   |   Energy of the projectile
*       int si          |   Are we using Si units of measure
*/
double get_velocity( double mass, double energy, int si )
{
    /*  Si */
    if( si == 1 )
        return( sqrt( ( (energy*2)/mass ) * K ));

    /*  Imperial */
    else
        return( sqrt( (energy/mass) * K ));
}



/*==============================================================================
                                 VERBOSE RESULT
--------------------------------------------------------------------------------
*   Prints the results to the user in a 'verbose' way; i.e., more than just the
*   number.
*
*   Params
*       double mass     |   Mass of the projectile
*       double velocity |   Velocity of the projectile
*       double energy   |   Energy of the projectile
*       int *options    |   Program options (si, verbosity, etc.)
*/
void verbose_result( double mass, double velocity, double energy, int *options )
{
    if( options[ OPT_VERBOSE ] == 1 )
    {
        /*  Si */
        if( options[ OPT_SI ] == 1 )
        {
            if( options[ OPT_PRECISE ] == 1 )
            {
                printf( "%.02lf g @ %.02lf m/s = %.02lf J\n",
                        mass, velocity, energy );
            }

            else
            {
                printf( "%.02lf g @ %.02lf m/s = %.0lf J\n",
                        mass, velocity, round(energy) );
            }

        }   //  END if si


        /*  Imperial */
        else
        {
            /*  If we're being precise */
            if( options[ OPT_PRECISE ] == 1 )
            {
                printf( "%.02lf gr @ %.02lf ft/s = %.02lf lbf\n",
                        mass, velocity, energy );
            }

            /*  Otherwise, keep the output prettier */
            else
            {
                printf( "%.0lf gr @ %.0lf ft/s = %.0lf lbf\n",
                        round( mass ), round( velocity ), round( energy ));
            }

        }   //  END else imperial
    }
}




/*==============================================================================
                                     RESULT
--------------------------------------------------------------------------------
*   This function is where we shove all of our lovely data, where it then
*   calls other functions to do the calculations and print the results.
*
*   Params
*       double mass     |   Mass of the projectile
*       double velocity |   Velocity of the projectile
*       double energy   |   Energy of the projectile
*       int param       |   Which of the three (mass, vel, energy) we solve for
*       int *options    |   Program options (si, verbose, etc.)
*/
void result( double mass, double velocity, double energy,
        int param, int *options )
{
    /*
     *  Here, we set the constant.
     */

    switch( options[ OPT_K ] )
    {
        case -1:    //  Industry standard, don't calculate in-program (default)
            K = 450240.0f;
            break;

        /* Use the industry's version of the grav. accel. constant (32.163) */
        case 1:
            K = ( 2 * GRAVITY_IMPERIAL_APPROX * 7000 );
            break;

        /*  Use standard (32.1739) */
        case 2:
            K = ( 2 * GRAVITY_IMPERIAL * 7000 );
            break;
    }

    /*  If the user is using Si units, K = 1000 unless they entered their own */
    if( options[ OPT_SI ] == 1 && options[ OPT_K ] != 0 )
        K = 1000.0f;


    double result = -1;

    switch( param )
    {
        case PARAM_ENERGY:
            result = get_energy( mass, velocity, options[ OPT_SI ] );
            verbose_result( mass, velocity, result, options );
            break;

        case PARAM_MASS:
            result = get_mass( velocity, energy, options[ OPT_SI ] );
            verbose_result( result, velocity, energy, options );
            break;

        case PARAM_VELOCITY:
            result = get_velocity( mass, energy, options[ OPT_SI ] );
            verbose_result( mass, result, energy, options );
            break;

        default:
            result = -1;
            break;
    }


    /*  Print terse result */
    if( options[ OPT_VERBOSE ] != 1 )
    {
        if( options[ OPT_PRECISE ] == 1 )
            printf( "%.02lf\n", result );
        else
            printf( "%.0lf\n", result );
    }


}



/*==============================================================================
                                      TKOF
                           (Taylor Knockout Formula)
--------------------------------------------------------------------------------
*   This function prints results using the Taylor Knockout Formula, an
*   alternative to the standard muzzle energy formula developed by African
*   big-game hunter John Taylor.  Its purpose is not to be scientific, but to
*   present the hunter with a simple number that is supposed to correspond
*   well to its real-world performance according to Taylor's experience.
*
*
*   Params
*       double mass     |   Mass of the projectile
*       double velocity |   Velocity of projectile
*       double diameter |   Diameter of the projectile
*       int *options    |   Program options
*/
void tkof( double mass, double velocity, double diameter, int *options )
{
    /*  Knockout number */
    double ko;

    /*  Si */
    if( options[ OPT_SI ] == 1 )
    {
        ko = ( mass * velocity * diameter ) / 3500.0f;

        /*  Verbose output */
        if( options[ OPT_VERBOSE ] == 1 )
        {
            printf("%.02lf g @ %.02lf m/s (%.02lf mm diameter) = %.02lf TKOF\n",
                    mass, velocity, diameter, ko );

        }

        /*  Terse */
        else
            printf( "%.02lf\n", ko );
    }

    /*  Imperial */
    else
    {
        ko = ( mass * velocity * diameter ) / 7000.0f;

        /*  Verbose output */
        if( options[ OPT_VERBOSE ] == 1 )
        {
            if( options[ OPT_PRECISE ] == 1 )
            {
                printf("%.02lf gr @ %.02lf ft/s (%.03lf\" diameter) = %.02lf ",
                        mass, velocity, diameter, ko );
                printf( "TKOF\n" );
            }

            else
            {
                printf( "%.0lf gr @ %.0lf ft/s (%.03lf\" diameter) = %.02lf ",
                        round( mass ), round( velocity ), diameter, ko );
                printf( "TKOF\n" );
            }
        }

        /*  Terse */
        else
            printf( "%.02lf\n", ko );
    }
}



/*==============================================================================
                                 MAIN FUNCTION
--------------------------------------------------------------------------------
*   The main function, duh
*
*   Params
*       int argc        |   Number of arguments, including program
*       char *argv[]    |   Array of strings, the arguments in question
*/
int main( int argc, char *argv[] )
{
    /*  Check the number of arguments */
    if( argc < 2 )  //  Too few args
    {
        fprintf( stderr, "ERROR:  Too few arguments\n" );
        print_usage( stderr );
        fprintf( stderr, "\nTo view help, run with -h argument.\n" );

        return( 1 );
    }

    /*  Init global options */
    int options[ TOTAL_OPTIONS ];
    int i;
    for( i = 0; i < TOTAL_OPTIONS; ++i )
        options[ i ] = -1;

    /*  Some options, we don't want set to -1 */
    options[ OPT_VERBOSE ] = 1;


    /*  Now, we create the variables we'll be using */
    double mass;        //  Mass of the projectile
    double velocity;    //  Velocity of the projectile
    double diameter;    //  Diameter of projectile
    double energy;      //  Energy of the projectile at the muzzle

    /*  Initialize everything to -1, which is default 'no' or 'invalid' */
    mass = velocity = diameter = energy = -1;

    /*  Which value are we solving for? */
    int valueWanted = PARAM_ENERGY;


    /*  Do our optstring thing */
    int opt = 0;
    opt = getopt( argc, argv, optString );
    while( opt != -1 )
    {
        switch( opt )
        {
            case 'h':   //  Help
                print_help();
                return( 0 );
                break;

            case 'V':   //  Version / author info
                print_version();
                return( 0 );
                break;

            case 'H':   //  Print additional help
                print_additional_help();
                return( 0 );
                break;

            case 'E':   //  Print usage examples
                print_examples();
                return( 0 );
                break;

            case 'S':   //  Silent mode
                //  FALL THROUGH
            case 'q':
                options[ OPT_VERBOSE ] = -1;
                break;

            case 's':   //  Use si units of measure
                options[ OPT_SI ] = 1;
                break;

            case 'i':   //  Imperial units of measure (default)
                options[ OPT_SI ] = -1;
                break;

            case 'm':   //  Mass of the projectile
                valueWanted = PARAM_MASS;
                break;

            case 'v':   //  Velocity of the projectile
                valueWanted = PARAM_VELOCITY;
                break;

            case 'e':   //  Energy of the projectile at the muzzle (default)
                valueWanted = PARAM_ENERGY;
                break;

            case 'c':   //  Approximate earth gravitational constant
                options[ OPT_K ] = 1;
                break;

            case 'C':   //  Do not approximate earth gravitational constant
                options[ OPT_K ] = 2;
                break;

            case 'k':   //  User wants to input a custom constant
                options[ OPT_K ] = 0;
                K = atof( optarg );
                break;

            case 'K':   //  Use industry standard constant 450240 (default)
                options[ OPT_K ] = -1;
                break;

            case 'p':
                options[ OPT_PRECISE ] = 1;
                break;

            case 't':
                options[ OPT_TKOF ] = 1;
                break;
        }

        opt = getopt( argc, argv, optString );
    }

    /*  Once outside the loop, adjust argc and argv */
    argv += ( optind - 1 );
    argc -= ( optind - 1 );


    /*
     *  If the user didn't specify mass/velocity/energy, assume they gave mass
     *  and velocity and that they want energy
     */
    if( argc > 1 )
    {
        /*  Only one arg */
        if( argc == 2 )
        {
            fprintf( stderr, "ERROR:  Need more than one parameter\n" );
            print_usage( stderr );
            fprintf( stderr, "\nTo view help, run with -h argument.\n" );
            return( 1 );
        }

        /*
         *  If they want the Taylor Knockout Formula, we need exactly three
         *  parameters:  mass, velocity and diameter of projectile.
         */
        else if( options[ OPT_TKOF ] == 1 )
        {
            /*  Too few */
            if( argc <= 3 )
            {
                fprintf(stderr,"ERROR:  The Taylor Knockout Formula requires ");
                fprintf( stderr, "three paramters:\n" );
                fprintf( stderr, "Mass, Velocity and Diameter of projectile\n");
                print_usage( stderr );
                fprintf( stderr, "\nTo view help, run with -h argument.\n" );
                return( 1 );
            }

            /*  We're good */
            else
            {
                mass = atof( argv[1] );
                velocity = atof( argv[2] );
                diameter = atof( argv[3] );
            }
        }

        /*  Two or more args, just grab the first two */
        else
        {
            mass = atof( argv[1] );
            velocity = atof( argv[2] );
        }

    }   //  END if argc > 1

    else
    {
        fprintf( stderr, "ERROR:  Parameters required\n" );
        print_usage( stderr );
        fprintf( stderr, "\nTo view help, run with -h argument.\n" );
        return( 1 );
    }



    /*  If we want the standard muzzle energy formula */
    if( options[ OPT_TKOF ] == -1 )
    {
        switch( valueWanted )
        {
            case PARAM_ENERGY:
                mass = atof( argv[1] );
                velocity = atof( argv[2] );
                break;

            case PARAM_MASS:
                velocity = atof( argv[1] );
                energy = atof( argv[2] );
                break;

            case PARAM_VELOCITY:
                mass = atof( argv[1] );
                energy = atof( argv[2] );
                break;
        }

        result( mass, velocity, energy, valueWanted, options );
    }   //  END standard muzzle energy forumla


    /*  If we want Taylor Knockout Formula */
    else
        tkof( mass, velocity, diameter, options );

    return( 0 );
}
