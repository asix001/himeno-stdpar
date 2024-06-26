/********************************************************************

 This benchmark test program is measuring a cpu performance
 of floating point operation by a Poisson equation solver.

 If you have any question, please ask me via email.
 written by Ryutaro HIMENO, November 26, 2001.
 Version 3.0
 ----------------------------------------------
 Ryutaro Himeno, Dr. of Eng.
 Head of Computer Information Division,
 RIKEN (The Institute of Pysical and Chemical Research)
 Email : himeno@postman.riken.go.jp
 ---------------------------------------------------------------
 You can adjust the size of this benchmark code to fit your target
 computer. In that case, please chose following sets of
 (mMIMAX - 1,mMJMAX - 1,mMKMAX - 1):
 small : 33,33,65
 small : 65,65,129
 midium: 129,129,257
 large : 257,257,513
 ext.large: 513,513,1025
 This program is to measure a computer performance in MFLOPS
 by using a kernel which appears in a linear solver of pressure
 Poisson eq. which appears in an incompressible Navier-Stokes solver.
 A point-Jacobi method is employed in this solver as this method can 
 be easyly vectrized and be parallelized.
 ------------------
 Finite-difference method, curvilinear coodinate system
 Vectorizable and parallelizable on each grid point
 No. of grid points : MIMAX - 1 x MJMAX - 1 x MKMAX - 1 including boundaries
 ------------------
 A,B,C : coefficient matrix, wrk1: source term of Poisson equation
 wrk2 : working area, OMEGA : relaxation parameter
 BND : control variable for boundaries and objects ( = 0 or 1)
 P : pressure
********************************************************************/

#include <cmath>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <execution>
#include <numeric>

#define SEQ std::execution::seq,
#define PAR std::execution::par,
#define PAR_UNSEQ std::execution::par_unseq,

#ifdef SSMALL
#define MIMAX            33
#define MJMAX            33
#define MKMAX            65
#endif

#ifdef SMALL
#define MIMAX            65
#define MJMAX            65
#define MKMAX            129
#endif

#ifdef MIDDLE
#define MIMAX            129
#define MJMAX            129
#define MKMAX            257
#endif

#ifdef LARGE
#define MIMAX            257
#define MJMAX            257
#define MKMAX            513
#endif

#ifdef ELARGE
#define MIMAX            513
#define MJMAX            513
#define MKMAX            1025
#endif

float jacobi(int nn, float (*a)[MIMAX][MJMAX][MKMAX], 
                     float(*b)[MIMAX][MJMAX][MKMAX], 
                     float (*c)[MIMAX][MJMAX][MKMAX], 
                     float (*p)[MJMAX][MKMAX], 
                     float (*bnd)[MJMAX][MKMAX], 
                     float(*wrk1)[MJMAX][MKMAX], 
                     float (*wrk2)[MJMAX][MKMAX]);

void initmt(float (*a)[MIMAX][MJMAX][MKMAX], 
            float(*b)[MIMAX][MJMAX][MKMAX], 
            float (*c)[MIMAX][MJMAX][MKMAX], 
            float (*p)[MJMAX][MKMAX], 
            float (*bnd)[MJMAX][MKMAX], 
            float(*wrk1)[MJMAX][MKMAX], 
            float (*wrk2)[MJMAX][MKMAX]);

double fflop(int,int,int);
double mflops(int,double,double);
double second();

/* From BabelStream */
// A lightweight counting iterator which will be used by the STL algorithms
// NB: C++ <= 17 doesn't have this built-in, and it's only added later in ranges-v3 (C++2a) which this
// implementation doesn't target
template <typename N>
class ranged 
{
  N from, to;
public:
  ranged(N from, N to ): from(from), to(to) {}
    class iterator {
      N num;
    public:
      using difference_type = N;
      using value_type = N;
      using pointer = const N*;
      using reference = const N&;
      using iterator_category = std::random_access_iterator_tag;
      explicit iterator(N _num = 0) : num(_num) {}

      iterator& operator++() { num++; return *this; }
      iterator operator++(int) { iterator retval = *this; ++(*this); return retval; }
      iterator operator+(const value_type v) const { return iterator(num + v); }

      bool operator==(iterator other) const { return num == other.num; }
      bool operator!=(iterator other) const { return *this != other; }
      bool operator<(iterator other) const { return num < other.num; }

      reference operator*() const { return num;}
      difference_type operator-(const iterator &it) const { return num - it.num; }
      value_type operator[](const difference_type &i) const { return num + i; }

    };
    iterator begin() { return iterator(from); }
    iterator end() { return iterator(to >= from? to+1 : to-1); }
};
/*********************/

int main()
{
  float (*a)[MIMAX][MJMAX][MKMAX] = new float[4][MIMAX][MJMAX][MKMAX];
  float (*b)[MIMAX][MJMAX][MKMAX] = new float[3][MIMAX][MJMAX][MKMAX];
  float (*c)[MIMAX][MJMAX][MKMAX] = new float[3][MIMAX][MJMAX][MKMAX];

  float (*p)[MJMAX][MKMAX] = new float[MIMAX][MJMAX][MKMAX];
  float (*bnd)[MJMAX][MKMAX] = new float[MIMAX][MJMAX][MKMAX];
  float (*wrk1)[MJMAX][MKMAX] = new float[MIMAX][MJMAX][MKMAX];
  float (*wrk2)[MJMAX][MKMAX] = new float[MIMAX][MJMAX][MKMAX];

  int    nn;
  float  gosa;
  double cpu,cpu0,cpu1,flop,target;

  target = 60.0;

  /*
   *    Initializing matrixes
   */
  initmt(a, b, c, p, bnd, wrk1, wrk2);
  printf("mimax = %d mjmax = %d mkmax = %d\n",MIMAX, MJMAX, MKMAX);
  printf("imax = %d jmax = %d kmax = %d\n",MIMAX - 1,MJMAX - 1,MKMAX - 1);

  nn = 3;
  printf(" Start rehearsal measurement process.\n");
  printf(" Measure the performance in %d times.\n\n",nn);

  cpu0 = second();
  gosa = jacobi(nn, a, b, c, p, bnd, wrk1, wrk2);
  cpu1 = second();
  cpu = cpu1 - cpu0;

  flop = fflop(MIMAX - 1,MJMAX - 1,MKMAX - 1);
  
  printf(" MFLOPS: %f time(s): %f %e\n\n",
         mflops(nn,cpu,flop),cpu,gosa);

  nn = 2000;//(int)(target/(cpu/3.0));
  printf(" Now, start the actual measurement process.\n");
  printf(" The loop will be excuted in %d times\n", nn);
  printf(" This will take about one minute.\n");
  printf(" Wait for a while\n\n");

  /*
   *    Start measuring
   */
  cpu0 = second();
  gosa = jacobi(nn, a, b, c, p, bnd, wrk1, wrk2);;
  cpu1 = second();

  cpu = cpu1 - cpu0;
  
  printf(" Loop executed for %d times\n",nn);
  printf(" Gosa : %e \n",gosa);
  printf(" MFLOPS measured : %f\tcpu : %f\n",mflops(nn,cpu,flop),cpu);
  printf(" Score based on Pentium III 600MHz : %f\n",
         mflops(nn,cpu,flop)/82,84);
  
  return (0);
}

void initmt(float (*a)[MIMAX][MJMAX][MKMAX], 
            float(*b)[MIMAX][MJMAX][MKMAX], 
            float (*c)[MIMAX][MJMAX][MKMAX], 
            float (*p)[MJMAX][MKMAX], 
            float (*bnd)[MJMAX][MKMAX], 
            float(*wrk1)[MJMAX][MKMAX], 
            float (*wrk2)[MJMAX][MKMAX])
{
  ranged<int> range_m(0,(MIMAX) * (MJMAX) * (MKMAX));

  std::for_each(PAR_UNSEQ range_m.begin(), range_m.end(), [=](int ijk) 
  {
    int i = ijk / ((MJMAX) * (MKMAX));
    int jk = ijk % ((MJMAX)*(MKMAX));
    int j = jk / (MKMAX);
    int k = jk % (MKMAX);
    
      a[0][i][j][k]=0.0;
      a[1][i][j][k]=0.0;
      a[2][i][j][k]=0.0;
      a[3][i][j][k]=0.0;
      b[0][i][j][k]=0.0;
      b[1][i][j][k]=0.0;
      b[2][i][j][k]=0.0;
      c[0][i][j][k]=0.0;
      c[1][i][j][k]=0.0;
      c[2][i][j][k]=0.0;
      p[i][j][k]=0.0;
      wrk1[i][j][k]=0.0;
      bnd[i][j][k]=0.0;
    });

  ranged<int> range_n(0,(MIMAX - 1) * (MJMAX - 1) * (MKMAX - 1));

  std::for_each(PAR_UNSEQ range_n.begin(), range_n.end(), [=](int ijk) 
  {
    int i = ijk / ((MJMAX - 1) * (MKMAX - 1));
    int jk = ijk % ((MJMAX - 1)*(MKMAX - 1));
    int j = jk / (MKMAX - 1);
    int k = jk % (MKMAX - 1);

      a[0][i][j][k]=1.0;
      a[1][i][j][k]=1.0;
      a[2][i][j][k]=1.0;
      a[3][i][j][k]=1.0/6.0;
      b[0][i][j][k]=0.0;
      b[1][i][j][k]=0.0;
      b[2][i][j][k]=0.0;
      c[0][i][j][k]=1.0;
      c[1][i][j][k]=1.0;
      c[2][i][j][k]=1.0;
      p[i][j][k]=(float)(i*i)/(float)((MIMAX - 1-1)*(MIMAX - 1-1));
      wrk1[i][j][k]=0.0;
      bnd[i][j][k]=1.0;
  });
}

float jacobi(int nn, float (*a)[MIMAX][MJMAX][MKMAX], 
                     float(*b)[MIMAX][MJMAX][MKMAX], 
                     float (*c)[MIMAX][MJMAX][MKMAX], 
                     float (*p)[MJMAX][MKMAX], 
                     float (*bnd)[MJMAX][MKMAX], 
                     float(*wrk1)[MJMAX][MKMAX], 
                     float (*wrk2)[MJMAX][MKMAX])
{
  float gosa;
  ranged<int> range(0,(MIMAX - 1-2) * (MJMAX - 1-2) * (MKMAX - 1-2));

 for(int n=0 ; n<nn ; ++n)
 {
    gosa = 0.0;

    // REDUCTION
    gosa = std::transform_reduce(PAR_UNSEQ range.begin(), range.end() , 0.0, std::plus{}, [=](int ijk) 
    {
      int i = ijk / ((MJMAX - 1-2) * (MKMAX - 1-2)) + 1;
      int jk = ijk % ((MJMAX - 1-2)*(MKMAX - 1-2));
      int j = jk / (MKMAX - 1-2) + 1;
      int k = jk % (MKMAX - 1-2) + 1;

      float omega = 0.8;
      float s0, ss;

      s0 = a[0][i][j][k] * p[i+1][j  ][k  ]
          + a[1][i][j][k] * p[i  ][j+1][k  ]
          + a[2][i][j][k] * p[i  ][j  ][k+1]
          + b[0][i][j][k] * ( p[i+1][j+1][k  ] - p[i+1][j-1][k  ]
                          - p[i-1][j+1][k  ] + p[i-1][j-1][k  ] )
          + b[1][i][j][k] * ( p[i  ][j+1][k+1] - p[i  ][j-1][k+1]
                          - p[i  ][j+1][k-1] + p[i  ][j-1][k-1] )
          + b[2][i][j][k] * ( p[i+1][j  ][k+1] - p[i-1][j  ][k+1]
                          - p[i+1][j  ][k-1] + p[i-1][j  ][k-1] )
          + c[0][i][j][k] * p[i-1][j  ][k  ]
          + c[1][i][j][k] * p[i  ][j-1][k  ]
          + c[2][i][j][k] * p[i  ][j  ][k-1]
          + wrk1[i][j][k];

      ss = ( s0 * a[3][i][j][k] - p[i][j][k] ) * bnd[i][j][k];
      wrk2[i][j][k] = p[i][j][k] + omega * ss;

      return ss*ss;
    });

    std::for_each(PAR_UNSEQ range.begin(), range.end(), [=](int ijk) 
    {
      int i = ijk / ((MJMAX - 1-2) * (MKMAX - 1-2)) + 1;
      int jk = ijk % ((MJMAX - 1-2)*(MKMAX - 1-2));
      int j = jk / (MKMAX - 1-2) + 1;
      int k = jk % (MKMAX - 1-2) + 1;                  
      p[i][j][k] = wrk2[i][j][k];
    });
  } /* end n loop */

  return(gosa);
}

double fflop(int mx,int my, int mz)
{
  return((double)(mz-2)*(double)(my-2)*(double)(mx-2)*34.0);
}

double mflops(int nn,double cpu,double flop)
{
  return(flop/cpu*1.e-6*(double)nn);
}

double second()
{
  static bool started = false;
  static std::chrono::high_resolution_clock::time_point t1;
  static std::chrono::high_resolution_clock::time_point t2;
  double t = 0.0;
  
  if (!started)
  {
    started = true;
    t1 = std::chrono::high_resolution_clock::now();
  } else {
    t2 = std::chrono::high_resolution_clock::now();
    t = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  }
  return t;
}

// nvc++ -O3 -std=c++17 -stdpar=gpu -DLARGE himenoBMTxps.cpp -o himenoBMTxps && ./himenoBMTxps
// nvc++ -O3 -std=c++17 -stdpar=gpu -DMIDDLE himenoBMTxps.cpp -o himenoBMTxps && ./himenoBMTxps
// nvc++ -O3 -std=c++17 -stdpar=gpu -DSMALL himenoBMTxps.cpp -o himenoBMTxps && ./himenoBMTxps

