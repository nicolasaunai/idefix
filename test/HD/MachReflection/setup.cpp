#include "idefix.hpp"
#include "setup.hpp"

// User-defined boundaries
void UserdefBoundary(DataBlock& data, int dir, BoundarySide side, real t) {
    const real alpha = 60./180.*M_PI;

    if( (dir==IDIR) && (side == left)) {
        IdefixArray4D<real> Vc = data.Vc;
        int ighost = data.nghost[IDIR];
        idefix_for("UserDefBoundaryX1Beg",0,data.np_tot[KDIR],0,data.np_tot[JDIR],0,ighost,
                    KOKKOS_LAMBDA (int k, int j, int i) {

                        Vc(RHO,k,j,i) = 8.0;
                        Vc(VX1,k,j,i) =   8.25*sin(alpha);
                        Vc(VX2,k,j,i) = - 8.25*cos(alpha);
                        Vc(PRS,k,j,i) = 116.5;
                      });
    }

    if(dir==JDIR) {
        IdefixArray4D<real> Vc = data.Vc;
        IdefixArray1D<real> x1 = data.x[IDIR];
        IdefixArray1D<real> x2 = data.x[JDIR];
        int jghost;
        int jbeg,jend;
        if(side == left) {
            jghost = data.beg[JDIR];
            jbeg = 0;
            jend = data.beg[JDIR];
            idefix_for("UserDefBoundaryX2Beg",0,data.np_tot[KDIR],jbeg,jend,0,data.np_tot[IDIR],
                        KOKKOS_LAMBDA (int k, int j, int i) {
                            if(x1(i) < 1.0/6.0) {
                              Vc(RHO,k,j,i) = 8.0;
                              Vc(VX1,k,j,i) =   8.25*sin(alpha);
                              Vc(VX2,k,j,i) = - 8.25*cos(alpha);
                              Vc(PRS,k,j,i) = 116.5;
                            } else {
                              Vc(RHO,k,j,i) =  Vc(RHO,k,2*jend-j-1,i);
                              Vc(VX1,k,j,i) =  Vc(VX1,k,2*jend-j-1,i);
                              Vc(VX2,k,j,i) = -Vc(VX2,k,2*jend-j-1,i);
                              Vc(PRS,k,j,i) =  Vc(PRS,k,2*jend-j-1,i);
                            }

                        });
            //return;
        } else if(side==right) {
            jghost = data.end[JDIR]-1;
            jbeg=data.end[JDIR];
            jend=data.np_tot[JDIR];
            real xs = 10.0*t/sin(alpha) + 1.0/6.0 + 1.0/tan(alpha);

            idefix_for("UserDefBoundaryX2End",0,data.np_tot[KDIR],jbeg,jend,0,data.np_tot[IDIR],
                        KOKKOS_LAMBDA (int k, int j, int i) {
                            if(x1(i) < xs) {
                              Vc(RHO,k,j,i) = 8.0;
                              Vc(VX1,k,j,i) =   8.25*sin(alpha);
                              Vc(VX2,k,j,i) = - 8.25*cos(alpha);
                              Vc(PRS,k,j,i) = 116.5;
                            } else {
                              Vc(RHO,k,j,i) =  1.4;
                              Vc(VX1,k,j,i) =  0.0;
                              Vc(VX2,k,j,i) =  0.0;
                              Vc(PRS,k,j,i) =  1.0;
                            }

                        });
        }
    }

}

// Default constructor
Setup::Setup() {}

// Initialisation routine. Can be used to allocate
// Arrays or variables which are used later on
Setup::Setup(Input &input, Grid &grid, DataBlock &data, Hydro &hydro) {
    hydro.EnrollUserDefBoundary(&UserdefBoundary);
}

// This routine initialize the flow
// Note that data is on the device.
// One can therefore define locally
// a datahost and sync it, if needed
void Setup::InitFlow(DataBlock &data) {
    // Create a host copy
    DataBlockHost d(data);
    real alpha, xs, x1,x2;

    alpha = 1.0/3.0*M_PI;

    for(int k = 0; k < d.np_tot[KDIR] ; k++) {
        for(int j = 0; j < d.np_tot[JDIR] ; j++) {
            for(int i = 0; i < d.np_tot[IDIR] ; i++) {
                x1 = d.x[IDIR](i);
                x2 = d.x[JDIR](j);
                xs = 1.0/6.0 + x2/tan(alpha);
                if(x1>xs) {
                  d.Vc(RHO,k,j,i) = 1.4;
                  d.Vc(VX1,k,j,i) = 0.0;
                  d.Vc(VX2,k,j,i) = 0.0;
                  d.Vc(PRS,k,j,i) = 1.0;
                } else {
                  d.Vc(RHO,k,j,i) = 8.0;
                  d.Vc(VX1,k,j,i) = 8.25*sin(alpha);
                  d.Vc(VX2,k,j,i) = -8.25*cos(alpha);
                  d.Vc(PRS,k,j,i) = 116.5;
                }
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