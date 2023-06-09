// Random.h: Definition and Implementation of Random Number Distribution Class
//           Ref: Richard Saucier, "Computer Generation of Statistical 
//                Distributions," ARL-TR-2168, US Army Research Laboratory,
//                Aberdeen Proving Ground, MD, 21005-5068, March 2000.

#ifndef RANDOM_H
#define RANDOM_H

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <cassert>
#include <cmath>
#include <cfloat>     // for FLT_MIN and FLT_MAX
#include <unistd.h>   // for getpid
#include <climits>
using namespace std;

// Constants for Tausworthe random bit generator
// Ref: Tausworthe, Robert C., "Random Numbers Generated by Linear Recurrence
//      Modulo Two," Mathematics of Computation, vol. 19, pp. 201-209, 1965.

static const unsigned DEGREE_MAX = 32;   // max degree (bits per word)

static const unsigned BIT[ 1 + DEGREE_MAX ] = {

// Hexidecimal       Value      Degree   
// -----------       -----      ------
   0x00000000,    // 0          0
   0x00000001,    // 2^0        1
   0x00000002,    // 2^1        2
   0x00000004,    // 2^2        3
   0x00000008,    // 2^3        4
   0x00000010,    // 2^4        5
   0x00000020,    // 2^5        6
   0x00000040,    // 2^6        7
   0x00000080,    // 2^7        8
   0x00000100,    // 2^8        9
   0x00000200,    // 2^9        10
   0x00000400,    // 2^10       11
   0x00000800,    // 2^11       12
   0x00001000,    // 2^12       13
   0x00002000,    // 2^13       14
   0x00004000,    // 2^14       15
   0x00008000,    // 2^15       16
   0x00010000,    // 2^16       17
   0x00020000,    // 2^17       18
   0x00040000,    // 2^18       19
   0x00080000,    // 2^19       20
   0x00100000,    // 2^20       21
   0x00200000,    // 2^21       22
   0x00400000,    // 2^22       23
   0x00800000,    // 2^23       24
   0x01000000,    // 2^24       25
   0x02000000,    // 2^25       26
   0x04000000,    // 2^26       27
   0x08000000,    // 2^27       28
   0x10000000,    // 2^28       29
   0x20000000,    // 2^29       30
   0x40000000,    // 2^30       31 
   0x80000000     // 2^31       32
};

// Coefficients that define a primitive polynomial (mod 2)
// Ref: Watson, E. J., "Primitive Polynomials (Mod 2),"
//      Mathematics of Computation, vol. 16, pp. 368-369, 1962.

static const unsigned MASK[ 1 + DEGREE_MAX ] = {

   BIT[0],                                      // 0
   BIT[0],                                      // 1
   BIT[1],                                      // 2
   BIT[1],                                      // 3
   BIT[1],                                      // 4
   BIT[2],                                      // 5
   BIT[1],                                      // 6
   BIT[1],                                      // 7
   BIT[4] + BIT[3] + BIT[2],                    // 8
   BIT[4],                                      // 9
   BIT[3],                                      // 10
   BIT[2],                                      // 11
   BIT[6] + BIT[4] + BIT[1],                    // 12
   BIT[4] + BIT[3] + BIT[1],                    // 13
   BIT[5] + BIT[3] + BIT[1],                    // 14
   BIT[1],                                      // 15
   BIT[5] + BIT[3] + BIT[2],                    // 16
   BIT[3],                                      // 17
   BIT[5] + BIT[2] + BIT[1],                    // 18
   BIT[5] + BIT[2] + BIT[1],                    // 19
   BIT[3],                                      // 20
   BIT[2],                                      // 21
   BIT[1],                                      // 22
   BIT[5],                                      // 23
   BIT[4] + BIT[3] + BIT[1],                    // 24
   BIT[3],                                      // 25
   BIT[6] + BIT[2] + BIT[1],                    // 26
   BIT[5] + BIT[2] + BIT[1],                    // 27
   BIT[3],                                      // 28
   BIT[2],                                      // 29
   BIT[6] + BIT[4] + BIT[1],                    // 30
   BIT[3],                                      // 31
   BIT[7] + BIT[5] + BIT[3] + BIT[2] + BIT[1]   // 32
};

// for convenience, define some data structures for bivariate distributions

struct cartesianCoord {   // cartesian coordinates in 2-D

   double x, y;
   cartesianCoord& operator+=( const cartesianCoord& p ) {
      x += p.x;
      y += p.y;
      return *this;
   }
   cartesianCoord& operator-=( const cartesianCoord& p ) {
      x -= p.x;
      y -= p.y;
      return *this;
   }
   cartesianCoord& operator*=( double scalar ) {
      x *= scalar;
      y *= scalar;
      return *this;
   }
   cartesianCoord& operator/=( double scalar ) {
      x /= scalar;
      y /= scalar;
      return *this;
   }
};

struct sphericalCoord {  // spherical coordinates on unit sphere

   double theta, phi;
   double x( void ) { return sin( theta ) * cos( phi ); }   // x-coordinate
   double y( void ) { return sin( theta ) * sin( phi ); }   // y-coordinate
   double z( void ) { return cos( theta ); }                // z-coordinate
};

class Random {

// friends list
// overloaded relational operators
   
   friend bool operator==( const Random& p, const Random& q )
   {
      bool equal = ( p._seed == q._seed ) && ( p._next == q._next );
      for ( int i = 0; i < p._NTAB; i++ )
         equal = equal && ( p._table[ i ] == q._table[ i ] );
      return equal;
   }
   
   friend bool operator!=( const Random& p, const Random& q )
   {
      return !( p == q );
   }
   
// overloaded stream operator
   
   friend istream& operator>>( istream& is, Random& rv )
   {
      cout << "Enter a random number seed "
           << "(between 1 and " << LONG_MAX - 1 << ", inclusive): " << endl;
        
      is >> rv._seed;

      assert( rv._seed != 0 && rv._seed != LONG_MAX );
   
      rv._seedTable();
   
      return is;
   }
  
public:

   Random( long seed )   // constructor to set the seed
   {
      assert( seed != 0 && seed != LONG_MAX );
      _seed = seed;
      _seedTable();
      
      _seed2 = _seed | 1;  // for tausworthe random bit generation
   }
   
   Random( void )   // default constructor uses process id to set the seed
   {
      _seed = long( getpid() );
      _seedTable();
      _seed2 = _seed | 1;  // for tausworthe random bit generation
   }

  ~Random( void )   // default destructor
   {
   }
   
   Random( const Random& r )   // copy constructor (copies current state)
   {
      _seed  = r._seed;
      _seed2 = r._seed2;

      // copy the current state of the shuffle table

      _next = r._next;
      for ( int i = 0; i < _NTAB; i++ ) _table[ i ] = r._table[ i ];
   }
   
   Random& operator=( const Random& r )   // overloaded assignment
   {
      if ( *this == r ) return *this;

      _seed  = r._seed;
      _seed2 = r._seed2;
   
      // copy the current state of the shuffle table

      _next = r._next;
      for ( int i = 0; i < _NTAB; i++ ) _table[ i ] = r._table[ i ];

      return *this;
   }
   
// utility functions

   void reset( long seed )   // reset the seed explicitly
   {
      assert( seed != 0 && seed != LONG_MAX );
      _seed = seed;
      _seedTable();
      _seed2 = _seed | 1;   // so that all bits cannot be zero
   }
   
   void reset( void )   // reset seed from current process id
   {
      _seed = long( getpid() );
      _seedTable();
      _seed2 = _seed | 1;   // so that all bits cannot be zero
   }

// Continuous Distributions

   double arcsine( double xMin = 0., double xMax = 1. )   // Arc Sine
   {
      double q = sin( M_PI_2 * _u() );
      return xMin + ( xMax - xMin ) * q * q;
   }
   
   double beta( double v, double w,                    // Beta
                double xMin = 0., double xMax = 1. )   // (v > 0. and w > 0.)
   {
      if ( v < w ) return xMax - ( xMax - xMin ) * beta( w, v );
      double y1 = gamma( 0., 1., v );
      double y2 = gamma( 0., 1., w );
      return xMin + ( xMax - xMin ) * y1 / ( y1 + y2 );
   }
   
   double cauchy( double a = 0., double b = 1. )   // Cauchy (or Lorentz)
   {
      // a is the location parameter and b is the scale parameter
      // b is the half width at half maximum (HWHM) and variance doesn't exist
   
      assert( b > 0. );
   
      return a + b * tan( M_PI * uniform( -0.5, 0.5 ) );
   }

   double chiSquare( int df )   // Chi-Square
   {
      assert( df >= 1 );
   
      return gamma( 0., 2., 0.5 * double( df ) );
   }

   double cosine( double xMin = 0., double xMax = 1. )   // Cosine
   {
      assert( xMin < xMax );
   
      double a = 0.5 * ( xMin + xMax );    // location parameter
      double b = ( xMax - xMin ) / M_PI;   // scale parameter
   
      return a + b * asin( uniform( -1., 1. ) );
   }
   
   double doubleLog( double xMin = -1., double xMax = 1. )   // Double Log
   {
      assert( xMin < xMax );
   
      double a = 0.5 * ( xMin + xMax );    // location parameter
      double b = 0.5 * ( xMax - xMin );    // scale parameter
   
      if ( bernoulli( 0.5 ) ) return a + b * _u() * _u();
      else                    return a - b * _u() * _u();
   }

   double erlang( double b, int c )   // Erlang (b > 0. and c >= 1)
   {
      assert( b > 0. && c >= 1 );
  
      double prod = 1.;
      for ( int i = 0; i < c; i++ ) prod *= _u();
      
      return -b * log( prod );
   }
   
   double exponential( double a = 0., double c = 1. )   // Exponential
   {                                                    // location a, shape c
      assert( c > 0.0 );
   
      return a - c * log( _u() );
   }
   
   double extremeValue( double a = 0., double c = 1. )   // Extreme Value
   {                                                     // location a, shape c
      assert( c > 0. );
   
      return a + c * log( -log( _u() ) );
   }
   
   double fRatio( int v, int w )   // F Ratio (v and w >= 1)
   {
      assert( v >= 1 && w >= 1 );
   
      return ( chiSquare( v ) / v ) / ( chiSquare( w ) / w );
   }
   
   double gamma( double a, double b, double c )  // Gamma
   {                                             // location a, scale b, shape c
      assert( b > 0. && c > 0. );
   
      const double A = 1. / sqrt( 2. * c - 1. );
      const double B = c - log( 4. );
      const double Q = c + 1. / A;
      const double T = 4.5;
      const double D = 1. + log( T );
      const double C = 1. + c / M_E;
   
      if ( c < 1. ) {  
         while ( true ) {
            double p = C * _u();      
            if ( p > 1. ) {
               double y = -log( ( C - p ) / c );
               if ( _u() <= pow( y, c - 1. ) ) return a + b * y;
            }
            else {
               double y = pow( p, 1. / c );
               if ( _u() <= exp( -y ) ) return a + b * y;
            }
         }
      }
      else if ( c == 1.0 ) return exponential( a, b );
      else {
         while ( true ) {
            double p1 = _u();
            double p2 = _u();
            double v = A * log( p1 / ( 1. - p1 ) );
            double y = c * exp( v );
            double z = p1 * p1 * p2;
            double w = B + Q * v - y;
            if ( w + D - T * z >= 0. || w >= log( z ) ) return a + b * y;
         }
      }
   }
   
   double laplace( double a = 0., double b = 1. )   // Laplace
   {                                                // (or double exponential)
      assert( b > 0. );

      // composition method
  
      if ( bernoulli( 0.5 ) ) return a + b * log( _u() );
      else                    return a - b * log( _u() );
   }
   
   double logarithmic( double xMin = 0., double xMax = 1. )   // Logarithmic
   {
      assert( xMin < xMax );
   
      double a = xMin;          // location parameter
      double b = xMax - xMin;   // scale parameter
   
      // use convolution formula for product of two IID uniform variates

      return a + b * _u() * _u();
   }
   
   double logistic( double a = 0., double c = 1. )   // Logistic
   {
      assert( c > 0. );

      return a - c * log( 1. / _u() - 1. );
   }
   
   double lognormal( double a, double mu, double sigma )   // Lognormal
   {
      return a + exp( normal( mu, sigma ) );
   }
   
   double normal( double mu = 0., double sigma = 1. )   // Normal
   {
      assert( sigma > 0. );
   
      static bool f = true;
      static double p, p1, p2;
      double q;
   
      if ( f ) {
         do {
            p1 = uniform( -1., 1. );
            p2 = uniform( -1., 1. );
            p = p1 * p1 + p2 * p2;
         } while ( p >= 1. );
         q = p1;
      }
      else
         q = p2;
      f = !f;

      return mu + sigma * q * sqrt( -2. * log( p ) / p );
   }
   
   double parabolic( double xMin = 0., double xMax = 1. )   // Parabolic
   {  
      assert( xMin < xMax );
   
      double a    = 0.5 * ( xMin + xMax );        // location parameter
      double yMax = _parabola( a, xMin, xMax );   // maximum function range
   
      return userSpecified( _parabola, xMin, xMax, 0., yMax );
   }
   
   double pareto( double c )   // Pareto
   {                           // shape c
      assert( c > 0. );
   
      return pow( _u(), -1. / c );
   }
   
   double pearson5( double b, double c )   // Pearson Type 5
   {                                       // scale b, shape c
      assert( b > 0. && c > 0. );
   
      return 1. / gamma( 0., 1. / b, c );
   }
   
   double pearson6( double b, double v, double w )   // Pearson Type 6
   {                                                 // scale b, shape v & w
      assert( b > 0. && v > 0. && w > 0. );
   
      return gamma( 0., b, v ) / gamma( 0., b, w );
   }
   
   double power( double c )   // Power
   {                          // shape c
      assert( c > 0. );
   
      return pow( _u(), 1. / c );
   }
   
   double rayleigh( double a, double b )   // Rayleigh
   {                                       // location a, scale b
      assert( b > 0. );
   
      return a + b * sqrt( -log( _u() ) );
   }
   
   double studentT( int df )   // Student's T
   {                           // degres of freedom df
      assert( df >= 1 );
   
      return normal() / sqrt( chiSquare( df ) / df );
   }
   
   double triangular( double xMin = 0.,     // Triangular
                      double xMax = 1.,     // with default interval [0,1)
                      double c    = 0.5 )   // and default mode 0.5
   {
      assert( xMin < xMax && xMin <= c && c <= xMax );
      
      double p = _u(), q = 1. - p;
      
      if ( p <= ( c - xMin ) / ( xMax - xMin ) )
         return xMin + sqrt( ( xMax - xMin ) * ( c - xMin ) * p );
      else
         return xMax - sqrt( ( xMax - xMin ) * ( xMax - c ) * q );
   }
   
   double uniform( double xMin = 0., double xMax = 1. )   // Uniform
   {                                                      // on [xMin,xMax)
      assert( xMin < xMax );
   
      return xMin + ( xMax - xMin ) * _u();
   }
   
   double userSpecified(                // User-Specified Distribution
        double( *usf )(                 // pointer to user-specified function
             double,                    // x
             double,                    // xMin
             double ),                  // xMax
        double xMin, double xMax,       // function domain
        double yMin, double yMax )      // function range
   {
      assert( xMin < xMax && yMin < yMax );
   
      double x, y, areaMax = ( xMax - xMin ) * ( yMax - yMin ); 

      // acceptance-rejection method
   
      do {   
         x = uniform( 0.0, areaMax ) / ( yMax - yMin ) + xMin;  
         y = uniform( yMin, yMax );
   
      } while ( y > usf( x, xMin, xMax ) );
   
      return x;
   }
   
   double weibull( double a, double b, double c )   // Weibull
   {                                                // location a, scale b,
      assert( b > 0. && c > 0. );                   // shape c
   
      return a + b * pow( -log( _u() ), 1. / c );
   }

    // nitin
    /*
   double inverse_poly (double x, double offset, int N) 
   {    
       assert(x>0);
       return (N-1) * pow(offset, N-1) * pow((x+offset), -N);
   }

   double inverse_polyical(double offset , int poly)
   {
       
       const double X_MIN = 1.;
       //const double X_MAX = ;
       const double Y_MIN = 0.;
       const double Y_MAX = 1.;
       
       return userSpecified(inverse_poly(offset, poly), X_MIN, 100 , Y_MIN, Y_MAX);
    }
*/
   
                   
// Discrete Distributions

   bool bernoulli( double p = 0.5 )   // Bernoulli Trial
   {
      assert( 0. <= p && p <= 1. );
   
      return _u() < p;
   }
   
   int binomial( int n, double p )   // Binomial
   {
      assert( n >= 1 && 0. <= p && p <= 1. );
   
      int sum = 0;
      for ( int i = 0; i < n; i++ ) sum += bernoulli( p );
      return sum;
   }
   
   int geometric( double p )   // Geometric
   {
      assert( 0. < p && p < 1. );

      return int( log( _u() ) / log( 1. - p ) );
   }
   
   int hypergeometric( int n, int N, int K )            // Hypergeometric
   {                                                    // trials n, size N,
      assert( 0 <= n && n <= N && N >= 1 && K >= 0 );   // successes K
      
      int count = 0;
      for ( int i = 0; i < n; i++, N-- ) {
   
         double p = double( K ) / double( N );
         if ( bernoulli( p ) ) { count++; K--; }
      }
      return count;
   }
   
   void multinomial( int    n,            // Multinomial
                     double p[],          // trials n, probability vector p,
                     int    count[],      // success vector count,
                     int    m )           // number of disjoint events m
   {
      assert( m >= 2 );   // at least 2 events
      double sum = 0.;
      for ( int bin = 0; bin < m; bin++ ) sum += p[ bin ];    // probabilities
      assert( sum == 1. );                                    // must sum to 1
      
      for ( int bin = 0; bin < m; bin++ ) count[ bin ] = 0;   // initialize
   
      // generate n uniform variates in the interval [0,1) and bin the results
   
      for ( int i = 0; i < n; i++ ) {

         double lower = 0., upper = 0., u = _u();

         for ( int bin = 0; bin < m; bin++ ) {

         // locate subinterval, which is of length p[ bin ],
         // that contains the variate and increment the corresponding counter
      
            lower = upper;
            upper += p[ bin ];
            if ( lower <= u && u < upper ) { count[ bin ]++; break; }
         }
      }
   }
   
   int negativeBinomial( int s, double p )   // Negative Binomial
   {                                         // successes s, probability p
      assert( s >= 1 && 0. < p && p < 1. );
   
      int sum = 0;
      for ( int i = 0; i < s; i++ ) sum += geometric( p );
      return sum;
   }
   
   int pascal( int s, double p )              // Pascal
   {                                          // successes s, probability p
      return negativeBinomial( s, p ) + s;
   }
   
   int poisson( double mu )   // Poisson
   {
      assert ( mu > 0. );
   
      double a = exp( -mu );
      double b = 1.;
   
      int i;
      for ( i = 0; b >= a; i++ ) b *= _u();   
      return i - 1;
   }
   
   int uniformDiscrete( int i, int j )   // Uniform Discrete
   {                                     // inclusive i to j
      assert( i < j );

      return i + int( ( j - i + 1 ) * _u() );
   }

// Empirical and Data-Driven Distributions

   double empirical( void )   // Empirical Continuous
   {
      static vector< double > x, cdf;
      static int              n;
      static bool             init = false;
   
      if ( !init ) {
         ifstream in( "empiricalDistribution" );
         if ( !in ) {
            cerr << "Cannot open \"empiricalDistribution\" input file" << endl;
            exit( 1 );
         }
         double value, prob;
         while ( in >> value >> prob ) {   // read in empirical distribution
            x.push_back( value );
            cdf.push_back( prob );
         }
         n = x.size();
         init = true;
      
         // check that this is indeed a cumulative distribution

         assert( 0. == cdf[ 0 ] && cdf[ n - 1 ] == 1. );
         for ( int i = 1; i < n; i++ ) assert( cdf[ i - 1 ] < cdf[ i ] );
      }

      double p = _u();
      for ( int i = 0; i < n - 1; i++ )
         if ( cdf[ i ] <= p && p < cdf[ i + 1 ] )
            return x[ i ] + ( x[ i + 1 ] - x[ i ] ) *
                            ( p - cdf[ i ] ) / ( cdf[ i + 1 ] - cdf[ i ] );
      return x[ n - 1 ];
   }
   
   int empiricalDiscrete( void )   // Empirical Discrete
   {
      static vector< int >    k;
      static vector< double > f[ 2 ];   // f[ 0 ] is pdf and f[ 1 ] is cdf
      static double           max;
      static int              n;
      static bool             init = false;
   
      if ( !init ) {
         ifstream in ( "empiricalDiscrete" );
         if ( !in ) {
            cerr << "Cannot open \"empiricalDiscrete\" input file" << endl;
            exit( 1 );
         }
         int value;
         double freq;
         while ( in >> value >> freq ) {   // read in empirical data
            k.push_back( value );
            f[ 0 ].push_back( freq );
         }
         n = k.size();
         init = true;
   
         // form the cumulative distribution

         f[ 1 ].push_back( f[ 0 ][ 0 ] );
         for ( int i = 1; i < n; i++ )
            f[ 1 ].push_back( f[ 1 ][ i - 1 ] + f[ 0 ][ i ] );

         // check that the integer points are in ascending order

         for ( int i = 1; i < n; i++ ) assert( k[ i - 1 ] < k[ i ] );
      
         max = f[ 1 ][ n - 1 ];
      }
   
      // select a uniform variate between 0 and the max value of the cdf

      double p = uniform( 0., max );

      // locate and return the corresponding index

      for ( int i = 0; i < n; i++ ) if ( p <= f[ 1 ][ i ] ) return k[ i ];
      return k[ n - 1 ];
   }
   
   double sample( bool replace = true )  // Sample w or w/o replacement from a
   {                                     // distribution of 1-D data in a file
      static vector< double > v;         // vector for sampling with replacement
      static bool init = false;          // flag that file has been read in
      static int n;                      // number of data elements in the file
      static int index = 0;              // subscript in the sequential order
   
      if ( !init ) {
         ifstream in( "sampleData" );
         if ( !in ) {
            cerr << "Cannot open \"sampleData\" file" << endl;
            exit( 1 );
         }
         double d;
         while ( in >> d ) v.push_back( d );
         in.close();
         n = v.size();
         init = true;
         if ( replace == false ) {   // sample without replacement
         
            // shuffle contents of v once and for all
            // Ref: Knuth, D. E., The Art of Computer Programming, Vol. 2:
            //      Seminumerical Algorithms. London: Addison-Wesley, 1969.

            for ( int i = n - 1; i > 0; i-- ) {
               int j = int( ( i + 1 ) * _u() );
               swap( v[ i ], v[ j ] );
            }
         }
      }

      // return a random sample

      if ( replace )                                // sample w/ replacement
         return v[ uniformDiscrete( 0, n - 1 ) ];
      else {                                        // sample w/o replacement
         assert( index < n );                       // retrieve elements
         return v[ index++ ];                       // in sequential order
      }
   }
   
   void sample( double x[], int ndim )   // Sample from a given distribution
   {                                     // of multi-dimensional data
      static const int N_DIM = 6;
      assert( ndim <= N_DIM );
   
      static vector< double > v[ N_DIM ];
      static bool init = false;
      static int n;
   
      if ( !init )  {
         ifstream in( "sampleData" );
         if ( !in ) {
            cerr << "Cannot open \"sampleData\" file" << endl;
            exit( 1 );
         }
         double d;
         while ( !in.eof() ) {
            for ( int i = 0; i < ndim; i++ ) {
               in >> d;
               v[ i ].push_back( d );
            }
         }
         in.close();
         n = v[ 0 ].size();
         init = true;
      }
      int index = uniformDiscrete( 0, n - 1 );
      for ( int i = 0; i < ndim; i++ ) x[ i ] = v[ i ][ index ];
   }

   // comparison functor for determining the neighborhood of a data point

   struct dSquared :
   public binary_function< cartesianCoord, cartesianCoord, bool > {
      bool operator()( cartesianCoord p, cartesianCoord q ) {
         return p.x * p.x + p.y * p.y < q.x * q.x + q.y * q.y;
      }
   };

   cartesianCoord stochasticInterpolation( void )   // Stochastic Interpolation

   // Refs: Taylor, M. S. and J. R. Thompson, Computational Statistics & Data 
   //       Analysis, Vol. 4, pp. 93-101, 1986; Thompson, J. R., Empirical Model
   //       Building, pp. 108-114, Wiley, 1989; Bodt, B. A. and M. S. Taylor,
   //       A Data Based Random Number Generator for A Multivariate Distribution
   //       - A User's Manual, ARBRL-TR-02439, BRL, APG, MD, Nov. 1982.
   {
      static vector< cartesianCoord > data;
      static cartesianCoord           min, max;
      static int                      m;
      static double                   lower, upper;
      static bool                     init = false;

      if ( !init ) {
         ifstream in( "stochasticData" );
         if ( !in ) {
            cerr << "Cannot open \"stochasticData\" input file" << endl;
            exit( 1 );
         }
   
         // read in the data and set min and max values
   
         min.x = min.y = FLT_MAX;
         max.x = max.y = FLT_MIN;
         cartesianCoord p;
         while ( in >> p.x >> p.y ) {

            min.x = ( p.x < min.x ? p.x : min.x );
            min.y = ( p.y < min.y ? p.y : min.y );
            max.x = ( p.x > max.x ? p.x : max.x );
            max.y = ( p.y > max.y ? p.y : max.y );
      
            data.push_back( p );
         }
         in.close();
         init = true;
   
         // scale the data so that each dimension will have equal weight

         for ( int i = 0; i < data.size(); i++ ) {
       
            data[ i ].x = ( data[ i ].x - min.x ) / ( max.x - min.x );
            data[ i ].y = ( data[ i ].y - min.y ) / ( max.y - min.y );
         }
   
         // set m, the number of points in a neighborhood of a given point
   
         m = data.size() / 20;       // 5% of all the data points
         if ( m < 5  ) m = 5;        // but no less than 5
         if ( m > 20 ) m = 20;       // and no more than 20
   
         lower = ( 1. - sqrt( 3. * ( double( m ) - 1. ) ) ) / double( m );
         upper = ( 1. + sqrt( 3. * ( double( m ) - 1. ) ) ) / double( m );
      }
   
      // uniform random selection of a data point (with replacement)
      
      cartesianCoord origin = data[ uniformDiscrete( 0, data.size() - 1 ) ];

      // make this point the origin of the coordinate system

      for ( int n = 0; n < data.size(); n++ ) data[ n ] -= origin;
      
      // sort the data with respect to its distance (squared) from this origin
      
      sort( data.begin(), data.end(), dSquared() );
      
      // find the mean value of the data in the neighborhood about this point
      
      cartesianCoord mean;
      mean.x = mean.y = 0.;
      for ( int n = 0; n < m; n++ ) mean += data[ n ];
      mean /= double( m );
   
      // select a random linear combination of the points in this neighborhood

      cartesianCoord p;
      p.x = p.y = 0.;
      for ( int n = 0; n < m; n++ ) {
         
         double rn;
         if ( m == 1 ) rn = 1.;
         else          rn = uniform( lower, upper );
         p.x += rn * ( data[ n ].x - mean.x );
         p.y += rn * ( data[ n ].y - mean.y );
      }
      
      // restore the data to its original form

      for ( int n = 0; n < data.size(); n++ ) data[ n ] += origin;
      
      // use mean and original point to translate the randomly-chosen point

      p += mean;
      p += origin;

      // scale randomly-chosen point to the dimensions of the original data
      
      p.x = p.x * ( max.x - min.x ) + min.x;
      p.y = p.y * ( max.y - min.y ) + min.y;

      return p;
   }

// Multivariate Distributions

   cartesianCoord bivariateNormal( double muX    = 0.,   // Bivariate Gaussian
                                   double sigmaX = 1.,
                                   double muY    = 0., 
                                   double sigmaY = 1. )
   {
      assert( sigmaX > 0. && sigmaY > 0. );
   
      cartesianCoord p;
      p.x = normal( muX, sigmaX );
      p.y = normal( muY, sigmaY );
      return p;
   }
   
   cartesianCoord bivariateUniform( double xMin = -1.,    // Bivariate Uniform
                                    double xMax =  1.,
                                    double yMin = -1.,
                                    double yMax =  1. )
   {
      assert( xMin < xMax && yMin < yMax );
   
      double x0 = 0.5 * ( xMin + xMax );
      double y0 = 0.5 * ( yMin + yMax );
      double a  = 0.5 * ( xMax - xMin );
      double b  = 0.5 * ( yMax - yMin );
      double x, y;
   
      do {
         x = uniform( -1., 1. );
         y = uniform( -1., 1. );
      
      } while( x * x + y * y > 1. );
      
      cartesianCoord p;
      p.x = x0 + a * x;
      p.y = y0 + b * y;
      return p;
   }
   
   cartesianCoord corrNormal( double r,              // Correlated Normal
                              double muX    = 0.,
                              double sigmaX = 1.,
                              double muY    = 0.,
                              double sigmaY = 1. )
   {
      assert( -1. <= r && r <= 1. &&          // bounds on correlation coeff
              sigmaX > 0. && sigmaY > 0. );   // positive std dev
   
      double x = normal();
      double y = normal();
   
      y = r * x + sqrt( 1. - r * r ) * y;     // correlate the variables
   
      cartesianCoord p;
      p.x = muX + sigmaX * x;                 // translate and scale
      p.y = muY + sigmaY * y;
      return p;
   }
   
   cartesianCoord corrUniform( double r,        // Correlated Uniform
                               double xMin = 0.,
                               double xMax = 1.,
                               double yMin = 0.,
                               double yMax = 1. )
   {
      assert( -1. <= r && r <= 1. &&          // bounds on correlation coeff
              xMin < xMax && yMin < yMax );   // bounds on domain

      double x0 = 0.5 * ( xMin + xMax );
      double y0 = 0.5 * ( yMin + yMax );
      double a  = 0.5 * ( xMax - xMin );
      double b  = 0.5 * ( yMax - yMin );
      double x, y;
   
      do {
         x = uniform( -1., 1. );
         y = uniform( -1., 1. );
      
      } while ( x * x + y * y > 1. );
   
      y = r * x + sqrt( 1. - r * r ) * y;   // correlate the variables
   
      cartesianCoord p;
      p.x = x0 + a * x;                     // translate and scale
      p.y = y0 + b * y;
      return p;
   }
   
   sphericalCoord spherical( double thMin = 0.,     // Uniform Spherical
                             double thMax = M_PI,
                             double phMin = 0.,
                             double phMax = 2. * M_PI )
   {
      assert( 0. <= thMin && thMin < thMax && thMax <= M_PI &&
              0. <= phMin && phMin < phMax && phMax <= 2. * M_PI );
   
      sphericalCoord p;                       // azimuth
      p.theta = acos( uniform( cos( thMax ), cos( thMin ) ) );   // polar angle
      p.phi   = uniform( phMin, phMax );                         // azimuth
      return p;
   }
   
   void sphericalND( double x[], int n )   // Uniform over the surface of
                                           // an n-dimensional unit sphere
   {
      // generate a point inside the unit n-sphere by normal polar method

      double r2 = 0.;
      for ( int i = 0; i < n; i++ ) {
         x[ i ] = normal();
         r2 += x[ i ] * x[ i ];
      }
   
      // project the point onto the surface of the unit n-sphere by scaling

      const double A = 1. / sqrt( r2 );
      for ( int i = 0; i < n; i++ ) x[ i ] *= A;
   }

// Number Theoretic Distributions

   double avoidance( void )   // Maximal Avoidance (1-D)
   {                          // overloaded for convenience
      double x[ 1 ];
      avoidance( x, 1 );
      return x[ 0 ];
   }
   
   void avoidance( double x[], int ndim )   // Maximal Avoidance (N-D)
   {
      static const int MAXBIT = 30;
      static const int MAXDIM = 6;
   
      assert( ndim <= MAXDIM );

      static unsigned long ix[ MAXDIM + 1 ] = { 0 };
      static unsigned long *u[ MAXBIT + 1 ];
      static unsigned long mdeg[ MAXDIM + 1 ] = { // degree of
         0,                                       // primitive polynomial
         1, 2, 3, 3, 4, 4
      };
      static unsigned long p[ MAXDIM + 1 ] = {   // decimal encoded
         0,                                      // interior bits
         0, 1, 1, 2, 1, 4
      };
      static unsigned long v[ MAXDIM * MAXBIT + 1 ] = {
          0,
          1,  1, 1,  1,  1,  1,
          3,  1, 3,  3,  1,  1,
          5,  7, 7,  3,  3,  5,
         15, 11, 5, 15, 13,  9
      };

      static double fac;
      static int in = -1;
      int j, k;
      unsigned long i, m, pp;
      
      if ( in == -1 ) {
         in = 0;
         fac = 1. / ( 1L << MAXBIT );
         for ( j = 1, k = 0; j <= MAXBIT; j++, k += MAXDIM ) u[ j ] = &v[ k ];
         for ( k = 1; k <= MAXDIM; k++ ) {
            for ( j = 1; j <= mdeg[ k ]; j++ ) u[ j ][ k ] <<= ( MAXBIT - j );
            for ( j = mdeg[ k ] + 1; j <= MAXBIT; j++ ) {
               pp = p[ k ];
               i = u[ j - mdeg[ k ] ][ k ];
               i ^= ( i >> mdeg[ k ] );
               for ( int n = mdeg[ k ] - 1; n >= 1; n-- ) {
                  if ( pp & 1 ) i ^= u[ j - n ][ k ];
                  pp >>= 1;
               }
               u[ j ][ k ] = i;
            }
         }
      }
      m = in++;
      for ( j = 0; j < MAXBIT; j++, m >>= 1 ) if ( !( m & 1 ) ) break;
      if ( j >= MAXBIT ) exit( 1 );
      m = j * MAXDIM;
      for ( k = 0; k < ndim; k++ ) {
         ix[ k + 1 ] ^= v[ m + k + 1 ];
         x[ k ] = ix[ k + 1 ] * fac;
      }
   }
   
   bool tausworthe( unsigned n )   // Tausworthe random bit generator
   {                               // returns a single random bit
      assert( 1 <= n && n <= 32 );

      if ( _seed2 & BIT[ n ] ) {
         _seed2 = ( ( _seed2 ^ MASK[ n ] ) << 1 ) | BIT[ 1 ];
         return true;
      }
      else {
         _seed2 <<= 1;
         return false;
      }
   }
   
   void tausworthe( bool*    bitvec,   // Tausworthe random bit generator
                    unsigned n )       // returns a bit vector of length n
                    
   // It is guaranteed to cycle through all possible combinations of n bits
   // (except all zeros) before repeating, i.e., cycle has maximal length 2^n-1.
   // Ref: Press, W. H., B. P. Flannery, S. A. Teukolsky and W. T. Vetterling,
   //      Numerical Recipes in C, Cambridge Univ. Press, Cambridge, 1988.
   {
      assert( 1 <= n && n <= 32 );   // length of bit vector

      if ( _seed2 & BIT[ n ] )
         _seed2 = ( ( _seed2 ^ MASK[ n ] ) << 1 ) | BIT[ 1 ];
      else
         _seed2 <<= 1;
      for ( int i = 0; i < n; i++ ) bitvec[ i ] = _seed2 & ( BIT[ n ] >> i );
   }
                                 
private:

   static const long   _M    = 0x7fffffff; // 2147483647 (Mersenne prime 2^31-1)
   static const long   A_    = 0x10ff5;    // 69621
   static const long   Q_    = 0x787d;     // 30845
   static const long   R_    = 0x5d5e;     // 23902
   const double _F    = 1. / _M;
   static const short  _NTAB = 32;         // arbitrary length of shuffle table
   static const long   _DIV  = 1+(_M-1)/_NTAB;
   long         _table[ _NTAB ];          // shuffle table of seeds
   long         _next;                    // seed to be used as index into table
   long         _seed;                    // current random number seed
   unsigned     _seed2;                   // seed for tausworthe random bit
   
   void _seedTable( void )                          // seeds the shuffle table
   {
      for ( int i = _NTAB + 7; i >= 0; i-- ) {      // first perform 8 warm-ups
      
         long k = _seed / Q_;                       // seed = ( A * seed ) % M
         _seed = A_ * ( _seed - k * Q_ ) - k * R_;  // without overflow by
         if ( _seed < 0 ) _seed += _M;              // Schrage's method
         
         if ( i < _NTAB ) _table[ i ] = _seed;      // store seeds into table
      }
      _next = _table[ 0 ];                          // used as index next time
   }

   double _u( void )                                // uniform rng
   { 
      long k = _seed / Q_;                          // seed = ( A*seed ) % M
      _seed = A_ * ( _seed - k * Q_ ) - k * R_;     // without overflow by
      if ( _seed < 0 ) _seed += _M;                 // Schrage's method

      int index = _next / _DIV;                     // Bays-Durham shuffle
      _next = _table[ index ];                      // seed used for next time
      _table[ index ] = _seed;                      // replace with new seed
   
      return _next * _F;                            // scale value within [0,1)
   }
   
   static double _parabola( double x, double xMin, double xMax )  // parabola
   {
      if ( x < xMin || x > xMax ) return 0.0;
   
      double a    = 0.5 * ( xMin + xMax );   // location parameter
      double b    = 0.5 * ( xMax - xMin );   // scale parameter
      double yMax = 0.75 / b;
      
      return yMax * ( 1. - ( x - a ) * ( x - a ) / ( b * b ) );
   }
};

#endif

