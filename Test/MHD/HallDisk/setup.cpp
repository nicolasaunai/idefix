#include "idefix.hpp"
#include "setup.hpp"

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


// User-defined boundaries
void UserdefBoundary(DataBlock& data, int dir, BoundarySide side, real t) {
    if( (dir==IDIR) && (side == left)) {
        IdefixArray4D<real> Vc = data.Vc;
        IdefixArray4D<real> Vs = data.Vs;
        IdefixArray1D<real> x1 = data.x[IDIR];

        int ighost = data.nghost[IDIR];
        idefix_for("UserDefBoundary",0,data.np_tot[KDIR],0,data.np_tot[JDIR],0,ighost,
                    KOKKOS_LAMBDA (int k, int j, int i) {
                        Vc(RHO,k,j,i) = Vc(RHO,k,j,ighost);
                        Vc(PRS,k,j,i) = Vc(PRS,k,j,ighost);
                        Vc(VX1,k,j,i) = Vc(VX1,k,j,ighost) * sqrt(x1(i)/x1(ighost));
                        Vc(VX2,k,j,i) = Vc(VX2,k,j,ighost) * sqrt(x1(i)/x1(ighost));
                        Vc(VX3,k,j,i) = Vc(VX3,k,j,ighost) * sqrt(x1(i)/x1(ighost));
                        Vs(BX2s,k,j,i) = Vs(BX2s,k,j,ighost);
                        Vs(BX3s,k,j,i) = Vs(BX3s,k,j,ighost);

                    });
    }

}


void Potential(DataBlock& data, const real t, IdefixArray1D<real>& x1, IdefixArray1D<real>& x2, IdefixArray1D<real>& x3, IdefixArray3D<real>& phi) {

    idefix_for("Potential",0,data.np_tot[KDIR], 0, data.np_tot[JDIR], 0, data.np_tot[IDIR],
        KOKKOS_LAMBDA (int k, int j, int i) {
        phi(k,j,i) = -1.0/x1(i);
    });

}

void Hall(DataBlock& data, const real t, IdefixArray3D<real> &xH) {
  IdefixArray1D<real> x1 = data.x[IDIR];

  idefix_for("Hall",0,data.np_tot[KDIR], 0, data.np_tot[JDIR], 0, data.np_tot[IDIR],

      KOKKOS_LAMBDA (int k, int j, int i) {
        real f;
        if(x1(i)<1.5) f = 0.0;
        else if(x1(i) < 1.6) f = 10.0*(x1(i)-1.5);
        else f=1.0;

        xH(k,j,i) = 0.1*pow(x1(i),-0.5)*f;
  });
}
// Default constructor
Setup::Setup() {}

// Initialisation routine. Can be used to allocate
// Arrays or variables which are used later on
Setup::Setup(Input &input, Grid &grid, DataBlock &data, Hydro &hydro) {
    // Set the function for userdefboundary
    hydro.EnrollUserDefBoundary(&UserdefBoundary);
    hydro.EnrollGravPotential(&Potential);
    hydro.EnrollHallDiffusivity(&Hall);
}

// This routine initialize the flow
// Note that data is on the device.
// One can therefore define locally
// a datahost and sync it, if needed
void Setup::InitFlow(DataBlock &data) {
    // Create a host copy
    DataBlockHost d(data);

    real x,y,z;

    real vphi,f,r,th;
    real beta=1e4;

    for(int k = 0; k < d.np_tot[KDIR] ; k++) {
        for(int j = 0; j < d.np_tot[JDIR] ; j++) {
            for(int i = 0; i < d.np_tot[IDIR] ; i++) {
                r=d.x[IDIR](i);
                th=d.x[JDIR](j);

                d.Vc(RHO,k,j,i) = 1.0;
                d.Vc(PRS,k,j,i) = 1.0e-2;
                d.Vc(VX1,k,j,i) = 0.0;
                d.Vc(VX2,k,j,i) = pow(r,-0.5);
                d.Vc(VX3,k,j,i) = 1e-2*(0.5-randm());

                d.Vs(BX1s,k,j,i) = 0.0;
                d.Vs(BX2s,k,j,i) = 0.0;
                d.Vs(BX3s,k,j,i) = 1e-1*d.Vc(VX2,k,j,i)/sqrt(beta);
            }
        }
    }

    // Send it all, if needed
    d.SyncToDevice();
}

// Analyse data to produce an output
void Setup::MakeAnalysis(DataBlock & data, real t) {

}




// Do a specifically designed user step in the middle of the integration
void ComputeUserStep(DataBlock &data, real t, real dt) {

}