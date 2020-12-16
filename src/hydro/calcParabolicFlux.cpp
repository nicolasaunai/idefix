// ***********************************************************************************************
// Idefix MHD astrophysical code
// Copyright(C) 2020 Geoffroy R. J. Lesur <geoffroy.lesur@univ-grenoble-alpes.fr
// and other code contributors
// Licensed under CeCILL 2.1 License, see COPYING for more information
// ***********************************************************************************************

#include "../idefix.hpp"
#include "hydro.hpp"

// Compute parabolic fluxes
void Hydro::CalcParabolicFlux(int dir, const real t) {
  idfx::pushRegion("Hydro::CalcParabolicFlux");

  IdefixArray3D<real> dMax = this->dMax;

  // Reset Max diffusion coefficient
  idefix_for("HydroParabolicResetStage",0,data->np_tot[KDIR],0,data->np_tot[JDIR],0,data->np_tot[IDIR],
    KOKKOS_LAMBDA (int k, int j, int i) {

        dMax(k,j,i) = ZERO_F;
      }
  );

  if(this->haveResistivity || this->haveAmbipolar) {
    this->AddNonIdealMHDFlux(dir,t);
  }

  if(this->haveViscosity) {
    this->viscosity.AddViscousFlux(dir,t);
  }

  idfx::popRegion();  
}
