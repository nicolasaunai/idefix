#include "idefix.hpp"
#include "setup.hpp"
#include "analysis.hpp"

static real gammaIdeal;
static real omega;
static real shear;
static real B0y;
static real B0z;

Analysis *analysis;


//#define STRATIFIED

/*********************************************/
/**
 Customized random number generator
 Allow one to have consistant random numbers
 generators on different architectures.
 **/
/*********************************************/
real randm(void) {
    const int a    =    16807;
    const int m =    2147483647;
    static int in0 = 13763 + 2417*idfx::prank;
    int q;

    /* find random number  */
    q= (int) fmod((double) a * in0, m);
    in0=q;

    return((real) ((double) q/(double)m));
}

void BodyForce(DataBlock &data, const real t, IdefixArray4D<real> &force) {
  idfx::pushRegion("BodyForce");
  IdefixArray1D<real> x = data.x[IDIR];
  IdefixArray1D<real> z = data.x[KDIR];

  // GPUS cannot capture static variables
  real omegaLocal=omega;
  real shearLocal =shear;

  idefix_for("BodyForce",
              data.beg[KDIR] , data.end[KDIR],
              data.beg[JDIR] , data.end[JDIR],
              data.beg[IDIR] , data.end[IDIR],
              KOKKOS_LAMBDA (int k, int j, int i) {

                force(IDIR,k,j,i) = -2.0*omegaLocal*shearLocal*x(i);
                force(JDIR,k,j,i) = ZERO_F;
                #ifdef STRATIFIED
                  force(KDIR,k,j,i) = - omegaLocal*omegaLocal*z(k);
                #else
                  force(KDIR,k,j,i) = ZERO_F;
                #endif
      });


  idfx::popRegion();
}

void AnalysisFunction(DataBlock &data) {
  analysis->PerformAnalysis(data);
}

// Initialisation routine. Can be used to allocate
// Arrays or variables which are used later on
Setup::Setup(Input &input, Grid &grid, DataBlock &data, Output &output) {
  gammaIdeal=data.hydro.GetGamma();

  // Get rotation rate along vertical axis
  omega=input.GetReal("Hydro","rotation",0);
  shear=input.GetReal("Hydro","shearingBox",0);
  B0y = input.GetReal("Setup","B0y",0);
  B0z = input.GetReal("Setup","B0z",0);

  // Add our userstep to the timeintegrator
  data.gravity.EnrollBodyForce(BodyForce);

  analysis = new Analysis(input, grid, data, output,std::string("timevol.dat"));
  output.EnrollAnalysis(&AnalysisFunction);
  // Reset analysis if required
  if(!input.restartRequested) {
    analysis->ResetAnalysis();
  }
}

// This routine initialize the flow
// Note that data is on the device.
// One can therefore define locally
// a datahost and sync it, if needed
void Setup::InitFlow(DataBlock &data) {
    // Create a host copy
    DataBlockHost d(data);
    real x,y,z;

    real cs2 = gammaIdeal*omega*omega;


    for(int k = 0; k < d.np_tot[KDIR] ; k++) {
        for(int j = 0; j < d.np_tot[JDIR] ; j++) {
            for(int i = 0; i < d.np_tot[IDIR] ; i++) {
                x=d.x[IDIR](i);
                y=d.x[JDIR](j);
                z=d.x[KDIR](k);

#ifdef STRATIFIED
                d.Vc(RHO,k,j,i) = 1.0*exp(-z*z/(2.0));
#else
                d.Vc(RHO,k,j,i) = 1.0;
#endif
                d.Vc(PRS,k,j,i) = d.Vc(RHO,k,j,i)/cs2*gammaIdeal;
                d.Vc(VX1,k,j,i) = 1e-5*sin(2.0*M_PI*(y+2*z));
                d.Vc(VX2,k,j,i) = shear*x;
                d.Vc(VX3,k,j,i) = 0.0;
                #ifdef EVOLVE_VECTOR_POTENTIAL
                  d.Ve(AX1e,k,j,i) = -B0z * y + B0y * z;
                  d.Ve(AX2e,k,j,i) = 0.0;
                  d.Ve(AX3e,k,j,i) = 0.0;
                #else
                  d.Vs(BX1s,k,j,i) = 0.0;
                  d.Vs(BX2s,k,j,i) = B0y;
                  d.Vs(BX3s,k,j,i) = B0z;
                #endif

            }
        }
    }

    // Send it all, if needed
    d.SyncToDevice();
}


// Analyse data to produce an output

void MakeAnalysis(DataBlock & data) {
}
