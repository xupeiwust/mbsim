/* Copyright (C) 2004-2009  MBSim Development Team

 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *  
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 *  
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Contact: martin.o.foerg@googlemail.com
 */

#include <config.h>
#include "mbsim/dynamic_system_solver.h"
#include "mbsim/links/link.h"
#include "time_stepping_ssc_integrator.h"
#include "mbsim/utils/eps.h"
#include "mbsim/utils/stopwatch.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef NO_ISO_14882
using namespace std;
#endif


using namespace fmatvec;
using namespace MBSim;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSim {

  MBSIM_OBJECTFACTORY_REGISTERCLASS(MBSIM, TimeSteppingSSCIntegrator)

  TimeSteppingSSCIntegrator::~TimeSteppingSSCIntegrator() {
    SetValuedLinkListT1.clear();
    SetValuedLinkListT2.clear();
    SetValuedLinkListT3.clear();
  }

  void TimeSteppingSSCIntegrator::integrate() {
    integrate(*system, *system, *system,1); 
  }

  void TimeSteppingSSCIntegrator::integrate(DynamicSystemSolver& systemT1_, DynamicSystemSolver& systemT2_, DynamicSystemSolver& systemT3_, int Threads) { 
    numThreads = Threads;
    preIntegrate(systemT1_, systemT2_, systemT3_);
    subIntegrate(tEnd);
    postIntegrate();
  }

  void TimeSteppingSSCIntegrator::preIntegrate() {
    preIntegrate(*system, *system, *system);
  }

  void TimeSteppingSSCIntegrator::preIntegrate(DynamicSystemSolver& systemT1_, DynamicSystemSolver& systemT2_, DynamicSystemSolver& systemT3_) {
    if(method==unknownMethod)
      throwError("(TimeSteppingSSCIntegrator::integrate): method unknown");
    if(GapControlStrategy==unknownGapControl)
      throwError("(TimeSteppingSSCIntegrator::integrate): gap control unknown");
    if(FlagErrorTest==unknownErrorTest)
      throwError("(TimeSteppingSSCIntegrator::integrate): error test unknown");

    debugInit();

    assert(method>=0);
    assert(method<3);
    assert(maxOrder>0);
    assert(maxOrder<5);

    assert(FlagErrorTest>=0);		// =0	control errors locally on all variables
    assert(FlagErrorTest<4);		// =2   u is scaled with stepsize
    assert(FlagErrorTest!=1);           // =3   exclude u from error test

    order = maxOrder;

    // FlagGapControl = GapControlStrategy >= 0; temporary deactivated

    sysT1 = &systemT1_;
    sysT2 = &systemT2_;
    sysT3 = &systemT3_;

    t = tStart;

    if (dtMin<=0) {
      dtMin = epsroot;
      if (!(method==extrapolation && maxOrder==1 && !FlagSSC)) dtMin =2.0*epsroot;
      if (maxOrder>=2) dtMin = 4.0*epsroot;
      if (maxOrder>=3) dtMin = 6.0*epsroot;
      if (method==extrapolation && maxOrder==3 && !FlagSSC) dtMin = 3.0*epsroot;
      if (method==extrapolation && maxOrder==2 && !FlagSSC) dtMin = 2.0*epsroot;
    }

    if ( safetyFactorGapControl <0) {
      safetyFactorGapControl= 1.0 + nrmInf(rTol)*100;
      if (safetyFactorGapControl>1.001) safetyFactorGapControl=1.001;
    }
    if (FlagSSC && method) safetyFactorSSC = pow(0.3,1.0/(maxOrder+1));
    if (!FlagSSC) method=extrapolation;
    if ((!method) && (maxOrder>=3) && (FlagSSC)) maxOrder=3;
    if (!method && maxOrder<1) maxOrder=1;

    qSize = sysT1->getqSize(); // size of positions, velocities, state
    uSize = sysT1->getuSize();
    xSize = sysT1->getxSize();
    zSize = qSize + uSize + xSize;

    zi.resize(zSize); 	// starting value for ith step
    ze.resize(zSize,NONINIT);
    z1d.resize(zSize,NONINIT);
    z2b.resize(zSize,NONINIT);
    z2d.resize(zSize,NONINIT);
    z2dRE.resize(zSize,NONINIT);
    z3b.resize(zSize,NONINIT);
    z3d.resize(zSize,NONINIT);
    z4d.resize(zSize,NONINIT);
    z6d.resize(zSize,NONINIT);

    SetValuedLinkListT1 = sysT1->getSetValuedLinks();
    SetValuedLinkListT2 = sysT2->getSetValuedLinks();
    SetValuedLinkListT3 = sysT3->getSetValuedLinks();

    maxIter = sysT1->getMaxIter();
    iter= 0;
    integrationSteps= 0;
    integrationStepsOrder1= 0;
    integrationStepsOrder2= 0;
    integrationStepswithChange =0;
    refusedSteps = 0;
    wrongAlertGapControl =0;
    stepsOkAfterGapControl =0;
    Penetration=0;
    PenetrationLog=0;
    PenetrationCounter=0;
    PenetrationMax = 0;
    PenetrationMin = -1;
    stepsRefusedAfterGapControl =0;
    maxIterUsed = 0;
    sumIter = 0;
    StepTrials=0;
    singleStepsT1 = 0;
    singleStepsT2 = 0;
    singleStepsT3 = 0;
    maxdtUsed = dtMin;
    mindtUsed = dtMax;

    if(aTol.size() == 0)
      aTol.resize(1,INIT,1e-6);
    if(rTol.size() == 0)
      rTol.resize(1,INIT,1e-4);

    if (aTol.size() < zSize) {
      double aTol0=aTol(0);
      aTol.resize(zSize,INIT,aTol0);
    } 
    if (rTol.size() < zSize) {
      double rTol0=rTol(0);
      rTol.resize(zSize,INIT,rTol0);
    }
    assert(aTol.size() == zSize);
    assert(rTol.size() == zSize);

    if (dtPlot<=0) FlagPlotEveryStep = true;
    else FlagPlotEveryStep = false;

    if(z0.size()) {
      if(z0.size() != system->getzSize()+system->getisSize())
        throwError("(ThetaTimeSteppingIntegrator::integrate): size of z0 does not match, must be " + to_string(system->getzSize()+system->getisSize()));
      zi = z0(RangeV(0,system->getzSize()-1));
      system->setInternalState(z0(RangeV(system->getzSize(),z0.size()-1)));
    }
    else
      zi = sysT1->evalz0();

    // Perform a projection of generalized positions at time t=0
    if(sysT1->getInitialProjection()) {
      sysT1->checkActive(1);
      if (sysT1->gActiveChanged()) resize(sysT1);
      sysT1->projectGeneralizedPositions(3,true);
    }

    sysT1->setTime(t);
    sysT1->setState(zi);
    sysT1->resetUpToDate();
    sysT1->setUpdatela(false);
    sysT1->setUpdateLa(false);
    sysT1->plot();

    tPlot = t;
  }

  void TimeSteppingSSCIntegrator::subIntegrate(double tStop) { // system: only dummy!
    Timer.start();

    lae <<= system->getla(false);

    qUncertaintyByExtrapolation=0;

    int StepFinished = 0;
    bool ExitIntegration = (t>=tStop);
    int UnchangedSteps =0;
    double dtHalf;
    double dtQuarter;
    double dtThird;
    double dtSixth;

    bool ConstraintsChangedA;
    bool ConstraintsChangedB;
    bool ConstraintsChangedC;
    bool ConstraintsChangedD;

    bool calcBlock2;
    bool Block2IsOptional = true;
    bool calcJobBT1 = false;                   // B: dt/2;  C: dt/4;  D: dt/6;  E: dt/3;
    bool calcJobBT2 = false;
    bool calcJobB2RET1 = false;
    bool calcJobB2RET2 = false;
    bool calcJobC = false;
    bool calcJobD = false;
    bool calcJobE1T1 = false;
    bool calcJobE23T1= false;
    bool calcJobE12T2= false;
    bool calcJobE3T2 = false;

    if (method == 0) {
      if (FlagSSC) {
        if (maxOrder==1) {Block2IsOptional= false; calcJobBT2 = true;}
        if (maxOrder==2) {
          calcJobBT1 = true;
          calcJobB2RET1 = true;
          calcJobC = true;
        }
        if (maxOrder==3) {
          calcJobBT1 = true;
          calcJobB2RET2 = true;
          calcJobC = true;
          calcJobD = true;
          calcJobE1T1 = true;
          calcJobE23T1= true;
        }
      }
      else {            // FlagSSC==false
        if (maxOrder==2) 
          calcJobBT2 = true;
        if (maxOrder==3) {
          calcJobBT1 = true;
          calcJobE12T2 = true;
          calcJobE3T2  = true;
        }
        if (maxOrder==4) {
          calcJobBT1 = true;
          calcJobC = true;
          calcJobD = true;
        }
      }
    }

    if (method>0) {
      if (maxOrder==1)  {calcJobBT2 = true; Block2IsOptional=false;}
      if (maxOrder > 1) {calcJobBT1 = true;  calcJobC = true;}
      if (maxOrder > 2) calcJobD = true;
      if (maxOrder > 3) {calcJobE1T1 = true; calcJobE23T1 = true; }
    } 

    if (FlagSSC==false) Block2IsOptional=false;

    if (numThreads ==0) {
      numThreads=2;
      if (calcJobD) numThreads++;
      if (method==extrapolation && !FlagSSC && maxOrder==1) numThreads=1;
    }

    sysT1->getLinkStatus(LStmp_T1);
    LS <<= LStmp_T1;
    while(! ExitIntegration) 
    {
      system->resetUpToDate();

      while(StepFinished==0) 
      {
        dtHalf     = dt/2.0;
        dtQuarter  = dt/4.0;
        dtThird    = dt/3.0;
        dtSixth    = dt/6.0;
        ConstraintsChangedB = false;
        ConstraintsChangedC = false;
        ConstraintsChangedD = false;

        // Block 1 
#pragma omp parallel num_threads(numThreads)
        {
#pragma omp sections

          {
#pragma omp section           //thread 1
            {
              // one step integration (A)
              sysT1->setTime(t);
              sysT1->setState(zi);
              sysT1->setStepSize(dt);
              sysT1->getq() += sysT1->evaldq();
              sysT1->getTime() += dt;
              sysT1->resetUpToDate();
              sysT1->checkActive(1);
              if (sysT1->gActiveChanged()) resize(sysT1);
              sysT1->getbi(false) <<= sysT1->evalgd() + sysT1->evalW().T()*slvLLFac(sysT1->evalLLM(),sysT1->evalh())*dt;
              sysT1->setUpdatebi(false);
              sysT1->getu() += sysT1->evaldu();
              sysT1->getx() += sysT1->evaldx();
              iterA  = sysT1->getIterI();
              la1d <<= sysT1->getLa()/dt;
              sysT1->getLinkStatus(LStmp_T1);
              sysT1->resetUpToDate();
              // wird jetzt von testTolerances() direkt aufgerufen; nach jedem erfolgreichen Schritt!
              //              if (FlagGapControl) {
              //                sysT1->updateStateDependentVariables(t+dt);
              //                getDataForGapControl(SetValuedLinkListT1);
              //              }
              LSA <<= LStmp_T1;
              ConstraintsChangedA = changedLinkStatus(LSA,LS,indexLSException);
              singleStepsT1++;
              z1d <<= sysT1->getState();

              // two step integration (first step) (B1) 
              if (calcJobBT1) {
                sysT1->setState(zi);
                sysT1->setTime(t);
                sysT1->setStepSize(dtHalf);
                sysT1->getq() += sysT1->evaldq();
                sysT1->setTime(t+dtHalf);
                sysT1->resetUpToDate();
                sysT1->checkActive(1);
                if (sysT1->gActiveChanged()) resize(sysT1);
                sysT1->getbi(false) <<= sysT1->evalgd() + sysT1->evalW().T()*slvLLFac(sysT1->evalLLM(),sysT1->evalh())*dtHalf;
                sysT1->setUpdatebi(false);
                sysT1->getu() += sysT1->evaldu();
                sysT1->getx() += sysT1->evaldx();
                iterB1  = sysT1->getIterI();
                la2b <<= sysT1->getLa()/dtHalf;
                sysT1->getLinkStatus(LStmp_T1);
                sysT1->resetUpToDate();
                LSB1 <<= LStmp_T1;
                ConstraintsChangedB = changedLinkStatus(LSB1,LS,indexLSException);
                singleStepsT1++;
                z2b <<= sysT1->getState();
              }
              // three step integration (first step) (E1) 
              if (calcJobE1T1) {

                sysT1->setState(zi);
                sysT1->setTime(t);
                sysT1->setStepSize(dtThird);
                sysT1->getq() += sysT1->evaldq();
                sysT1->setTime(t+dtThird);
                sysT1->resetUpToDate();
                sysT1->checkActive(1);
                if (sysT1->gActiveChanged()) resize(sysT1);
                sysT1->getbi(false) <<= sysT1->evalgd() + sysT1->evalW().T()*slvLLFac(sysT1->evalLLM(),sysT1->evalh())*dtThird;
                sysT1->setUpdatebi(false);
                sysT1->getu() += sysT1->evaldu();
                sysT1->getx() += sysT1->evaldx();
                sysT1->resetUpToDate();
                singleStepsT1++;
                z3b <<= sysT1->getState();
              }
            }  // close omp thread 1

#pragma omp section	        //thread 2
            {
              // two step integration (first step) (B1) 
              if (calcJobBT2) {
                sysT2->setState(zi);
                sysT2->setTime(t);
                sysT2->setStepSize(dtHalf);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dtHalf);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd() + sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtHalf;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                iterB1  = sysT2->getIterI();
                la2b <<= sysT2->getLa()/dtHalf;
                sysT2->getLinkStatus(LStmp_T2);
                sysT2->resetUpToDate();
                LSB1 <<= LStmp_T2;
                ConstraintsChangedB = changedLinkStatus(LSB1,LS,indexLSException);
                singleStepsT2++;
                z2b <<= sysT2->getState();
              }

              // four step integration (first two steps) (C12)
              if (calcJobC) {
                sysT2->setState(zi);
                sysT2->setTime(t);
                sysT2->setStepSize(dtQuarter);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dtQuarter);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd()+sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtQuarter;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                iterC1 = sysT2->getIterI();
                sysT2->getLinkStatus(LStmp_T2);
                sysT2->resetUpToDate();
                LSC1 <<= LStmp_T2;
                ConstraintsChangedC =  changedLinkStatus(LSC1,LS,indexLSException);

                sysT2->setTime(t+dtQuarter);
                sysT2->setStepSize(dtQuarter);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dtHalf);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd()+sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtQuarter;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                iterC2 = sysT2->getIterI();
                sysT2->getLinkStatus(LStmp_T2);
                sysT2->resetUpToDate();
                LSC2 <<= LStmp_T2;
                ConstraintsChangedC = ConstraintsChangedC || changedLinkStatus(LSC2,LSC1,indexLSException);
                singleStepsT2+=2;
                z4b <<= sysT2->getState();
              }
              // three step integration (first two steps ) (E12)
              if (calcJobE12T2) {
                sysT2->setState(zi);
                sysT2->setTime(t);
                sysT2->setStepSize(dtThird);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dtThird);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd() + sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtThird;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                sysT2->resetUpToDate();

                sysT2->setTime(t+dtThird);
                sysT2->setStepSize(dtThird);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+2.0*dtThird);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd() + sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtThird;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                sysT2->resetUpToDate();
                singleStepsT2+=2;
                z3b <<= sysT2->getState();
              }
            }  // close omp thread 2

#pragma omp section	        //thread 3
            {
              // six step integration (first three steps)   
              if(calcJobD) {
                sysT3->setState(zi);
                sysT3->setTime(t);
                sysT3->setStepSize(dtSixth);
                sysT3->getq() += sysT3->evaldq();
                sysT3->setTime(t+dtSixth);
                sysT3->resetUpToDate();
                sysT3->checkActive(1);
                if (sysT3->gActiveChanged()) resize(sysT3);
                sysT3->getbi(false) <<= sysT3->evalgd() + sysT3->evalW().T()*slvLLFac(sysT3->evalLLM(),sysT3->evalh())*dtSixth;
                sysT3->setUpdatebi(false);
                sysT3->getu() += sysT3->evaldu();
                sysT3->getx() += sysT3->evaldx();
                sysT3->getLinkStatus(LStmp_T3);
                sysT3->resetUpToDate();
                LSD1 <<= LStmp_T3;
                ConstraintsChangedD = changedLinkStatus(LSD1,LS,indexLSException);

                sysT3->setTime(t+dtSixth);
                sysT3->setStepSize(dtSixth);
                sysT3->getq() += sysT3->evaldq();
                sysT3->setTime(t+dtThird);
                sysT3->resetUpToDate();
                sysT3->checkActive(1);
                if (sysT3->gActiveChanged()) resize(sysT3);
                sysT3->getbi(false) <<= sysT3->evalgd() + sysT3->evalW().T()*slvLLFac(sysT3->evalLLM(),sysT3->evalh())*dtSixth;
                sysT3->setUpdatebi(false);
                sysT3->getu() += sysT3->evaldu();
                sysT3->getx() += sysT3->evaldx();
                sysT3->getLinkStatus(LStmp_T3);
                sysT3->resetUpToDate();
                LSD2 <<= LStmp_T3;
                ConstraintsChangedD = ConstraintsChangedD || changedLinkStatus(LSD2,LSD1,indexLSException);

                sysT3->setTime(t+dtThird);
                sysT3->setStepSize(dtSixth);
                sysT3->getq() += sysT3->evaldq();
                sysT3->setTime(t+dtHalf);
                sysT3->resetUpToDate();
                sysT3->checkActive(1);
                if (sysT3->gActiveChanged()) resize(sysT3);
                sysT3->getbi(false) <<= sysT3->evalgd() + sysT3->evalW().T()*slvLLFac(sysT3->evalLLM(),sysT3->evalh())*dtSixth;
                sysT3->setUpdatebi(false);
                sysT3->getu() += sysT3->evaldu();
                sysT3->getx() += sysT3->evaldx();
                sysT3->getLinkStatus(LStmp_T3);
                sysT3->resetUpToDate();
                LSD3 <<= LStmp_T3;
                ConstraintsChangedD = ConstraintsChangedD || changedLinkStatus(LSD3,LSD2,indexLSException);
                singleStepsT3+=3;
                z6b <<= sysT3->getState();
              }
            }  // close omp thread 3

          }  // close omp sections (first sections)
        }
#pragma omp single
        {
          ConstraintsChangedBlock1 = ConstraintsChangedB || ConstraintsChangedC || ConstraintsChangedD;
          ConstraintsChanged       = ConstraintsChangedA || ConstraintsChangedBlock1;
          calcBlock2 = (Block2IsOptional==false) || (ConstraintsChangedBlock1==false);
          ConstraintsChangedB = false;
          ConstraintsChangedC = false;
          ConstraintsChangedD = false;
          ConstraintsChangedBlock2 = false;        

          if (FlagSSC && method==extrapolation && maxOrder==3) zStern <<= 0.5*z2b - 4.0*z4b + 4.5*z6b;
          if (FlagSSC && method==extrapolation && maxOrder==2) zStern <<= 2.0*z4b - z2b;
        }
        // Block 2
#pragma omp parallel num_threads(numThreads)
        {
#pragma omp sections
          {
#pragma omp section                     // thread 1
            {
              // two step integration (B2)
              if (calcJobBT1 && calcBlock2) {
                sysT1->setState(z2b);
                sysT1->setTime(t+dtHalf);
                sysT1->setStepSize(dtHalf);
                sysT1->getq() += sysT1->evaldq();
                sysT1->setTime(t+dt);
                sysT1->resetUpToDate();
                sysT1->checkActive(1);
                if (sysT1->gActiveChanged()) resize(sysT1);
                sysT1->getbi(false) <<= sysT1->evalgd() + sysT1->evalW().T()*slvLLFac(sysT1->evalLLM(),sysT1->evalh())*dtHalf;
                sysT1->setUpdatebi(false);
                sysT1->getu() += sysT1->evaldu();
                sysT1->getx() += sysT1->evaldx();
                iterB2  = sysT1->getIterI();
                sysT1->getLinkStatus(LStmp_T1);
                sysT1->resetUpToDate();
                LSB2 <<= LStmp_T1;
                ConstraintsChangedB = changedLinkStatus(LSB2,LSB1,indexLSException);
                singleStepsT1++;
                z2d <<= sysT1->getState();
              }
              if (calcJobB2RET1 && calcBlock2) { //B2RE
                sysT1->setState(zStern);
                sysT1->setTime(t+dtHalf);
                sysT1->setStepSize(dtHalf);
                sysT1->getq() += sysT1->evaldq();
                sysT1->setTime(t+dt);
                sysT1->resetUpToDate();
                sysT1->checkActive(1);
                if (sysT1->gActiveChanged()) resize(sysT1);
                sysT1->getbi(false) <<= sysT1->evalgd() + sysT1->evalW().T()*slvLLFac(sysT1->evalLLM(),sysT1->evalh())*dtHalf;
                sysT1->setUpdatebi(false);
                sysT1->getu() += sysT1->evaldu();
                sysT1->getx() += sysT1->evaldx();
                iterB2RE  = sysT1->getIterI();
                sysT1->resetUpToDate();
                singleStepsT1++;
                z2dRE <<= sysT1->getState();
              }
              // three step integration (E23) last two steps
              if (calcJobE23T1 && calcBlock2) {
                sysT1->setState(z3b);
                sysT1->setTime(t+dtThird);
                sysT1->setStepSize(dtThird);
                sysT1->getq() += sysT1->evaldq();
                sysT1->setTime(t+2.0*dtThird);
                sysT1->resetUpToDate();
                sysT1->checkActive(1);
                if (sysT1->gActiveChanged()) resize(sysT1);
                sysT1->getbi(false) <<= sysT1->evalgd() + sysT1->evalW().T()*slvLLFac(sysT1->evalLLM(),sysT1->evalh())*dtThird;
                sysT1->setUpdatebi(false);
                sysT1->getu() += sysT1->evaldu();
                sysT1->getx() += sysT1->evaldx();
                sysT1->resetUpToDate();

                sysT1->setTime(t+2.0*dtThird);
                sysT1->setStepSize(dtThird);
                sysT1->getq() += sysT1->evaldq();
                sysT1->setTime(t+dt);
                sysT1->resetUpToDate();
                sysT1->checkActive(1);
                if (sysT1->gActiveChanged()) resize(sysT1);
                sysT1->getbi(false) <<= sysT1->evalgd() + sysT1->evalW().T()*slvLLFac(sysT1->evalLLM(),sysT1->evalh())*dtThird;
                sysT1->setUpdatebi(false);
                sysT1->getu() += sysT1->evaldu();
                sysT1->getx() += sysT1->evaldx();
                sysT1->resetUpToDate();
                singleStepsT1+=2;
                z3d <<= sysT1->getState();
              } 
            }  // close omp thread 1
#pragma omp section                   // thread 2
            { 
              // four step integration (C3 and C4) (last two steps )
              if (calcJobC && calcBlock2) {
                if (method) sysT2->setState(z4b);
                else sysT2->setState(zStern);
                sysT2->setTime(t+dtHalf);
                sysT2->setStepSize(dtQuarter);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dtHalf+dtQuarter);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd()+sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtQuarter;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                iterC3  = sysT2->getIterI();
                sysT2->getLinkStatus(LStmp_T2);
                sysT2->resetUpToDate();
                LSC3 <<= LStmp_T2;

                sysT2->setTime(t+dtHalf+dtQuarter);
                sysT2->setStepSize(dtQuarter);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dt);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd()+sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtQuarter;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                iterC4 = sysT2->getIterI();
                sysT2->getLinkStatus(LStmp_T2);
                sysT2->resetUpToDate();
                LSC4 <<= LStmp_T2;
                ConstraintsChangedC = changedLinkStatus(LSC2,LSC3,indexLSException);
                ConstraintsChangedC = ConstraintsChangedC || changedLinkStatus(LSC3,LSC4,indexLSException);
                singleStepsT2+=2;
                z4d <<= sysT2->getState();
              }

              // two step integration (B2)
              if (calcJobBT2 && calcBlock2) {
                sysT2->setState(z2b);
                sysT2->setTime(t+dtHalf);
                sysT2->setStepSize(dtHalf);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dt);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd() + sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtHalf;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                iterB2  = sysT2->getIterI();
                la2b <<= sysT2->getLa()/dtHalf;
                sysT2->getLinkStatus(LStmp_T2);
                sysT2->resetUpToDate();
                LSB2 <<= LStmp_T2;
                ConstraintsChangedB = changedLinkStatus(LSB2,LSB1,indexLSException);

                singleStepsT2++;
                z2d <<= sysT2->getState();
              }
              if (calcJobB2RET2 && calcBlock2) { //B2RE
                sysT2->setState(zStern);
                sysT2->setTime(t+dtHalf);
                sysT2->setStepSize(dtHalf);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dt);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd() + sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtHalf;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                iterB2RE  = sysT2->getIterI();
                sysT2->resetUpToDate();
                singleStepsT2++;
                z2dRE <<= sysT2->getState();
              }
              // three step integration (E3) last step
              if (calcJobE3T2 && calcBlock2) {
                sysT2->setState(z3b);
                sysT2->setTime(t+2.0*dtThird);
                sysT2->setStepSize(dtThird);
                sysT2->getq() += sysT2->evaldq();
                sysT2->setTime(t+dt);
                sysT2->resetUpToDate();
                sysT2->checkActive(1);
                if (sysT2->gActiveChanged()) resize(sysT2);
                sysT2->getbi(false) <<= sysT2->evalgd() + sysT2->evalW().T()*slvLLFac(sysT2->evalLLM(),sysT2->evalh())*dtThird;
                sysT2->setUpdatebi(false);
                sysT2->getu() += sysT2->evaldu();
                sysT2->getx() += sysT2->evaldx();
                sysT2->resetUpToDate();
                singleStepsT2++;
                z3d <<= sysT2->getState();
              }
            }  // close omp thread 2
#pragma omp section	        //thread 3
            {
              // six step integration (last three steps) 
              if (calcJobD && calcBlock2) {
                if (method) sysT3->setState(z6b);
                else sysT3->setState(zStern);
                sysT3->setTime(t+dtHalf);
                sysT3->setStepSize(dtSixth);
                sysT3->getq() += sysT3->evaldq();
                sysT3->setTime(t+4.0*dtSixth);
                sysT3->resetUpToDate();
                sysT3->checkActive(1);
                if (sysT3->gActiveChanged()) resize(sysT3);
                sysT3->getbi(false) <<= sysT3->evalgd() + sysT3->evalW().T()*slvLLFac(sysT3->evalLLM(),sysT3->evalh())*dtSixth;
                sysT3->setUpdatebi(false);
                sysT3->getu() += sysT3->evaldu();
                sysT3->getx() += sysT3->evaldx();
                sysT3->getLinkStatus(LStmp_T3);
                sysT3->resetUpToDate();
                LSD4 <<= LStmp_T3;
                ConstraintsChangedD =  changedLinkStatus(LSD4,LSD3,indexLSException);

                sysT3->setTime(t+4.0*dtSixth);
                sysT3->setStepSize(dtSixth);
                sysT3->getq() += sysT3->evaldq();
                sysT3->setTime(t+5.0*dtSixth);
                sysT3->resetUpToDate();
                sysT3->checkActive(1);
                if (sysT3->gActiveChanged()) resize(sysT3);
                sysT3->getbi(false) <<= sysT3->evalgd() + sysT3->evalW().T()*slvLLFac(sysT3->evalLLM(),sysT3->evalh())*dtSixth;
                sysT3->setUpdatebi(false);
                sysT3->getu() += sysT3->evaldu();
                sysT3->getx() += sysT3->evaldx();
                sysT3->getLinkStatus(LStmp_T3);
                sysT3->resetUpToDate();
                LSD5 <<= LStmp_T3;
                ConstraintsChangedD = ConstraintsChangedD || changedLinkStatus(LSD4,LSD5,indexLSException);

                sysT3->setTime(t+5.0*dtSixth);
                sysT3->setStepSize(dtSixth);
                sysT3->getq() += sysT3->evaldq();
                sysT3->setTime(t+dt);
                sysT3->resetUpToDate();
                sysT3->checkActive(1);
                if (sysT3->gActiveChanged()) resize(sysT3);
                sysT3->getbi(false) <<= sysT3->evalgd() + sysT3->evalW().T()*slvLLFac(sysT3->evalLLM(),sysT3->evalh())*dtSixth;
                sysT3->setUpdatebi(false);
                sysT3->getu() += sysT3->evaldu();
                sysT3->getx() += sysT3->evaldx();
                sysT3->getLinkStatus(LStmp_T3);
                sysT3->resetUpToDate();
                LSD6 <<= LStmp_T3;
                ConstraintsChangedD = ConstraintsChangedD || changedLinkStatus(LSD5,LSD6,indexLSException);

                singleStepsT3+=3;
                z6d <<= sysT3->getState();
              }
            }  // close omp thread 3



          }  // close omp sections (second sections)
        }  // close omp parallel

        ConstraintsChangedBlock2 = ConstraintsChangedB || ConstraintsChangedC || ConstraintsChangedD;
        ConstraintsChanged = ConstraintsChanged || ConstraintsChangedBlock2;
        if (integrationSteps>0 && ChangeByGapControl && !ConstraintsChanged) wrongAlertGapControl++;

        dtOld = dt;
        if (testTolerances()) {
          StepFinished = 1;
          sumIter += iter;
          if(iter > maxIterUsed) maxIterUsed = iter;
          if (maxdtUsed < dtOld) maxdtUsed = dtOld;
          if (mindtUsed > dtOld) mindtUsed = dtOld;
        }
        else {
          refusedSteps++;
          if (ConstraintsChanged) refusedStepsWithImpact++;
          StepTrials++;
          if (dtOld<=dtMin+macheps) {
            StepFinished = -1;
            msg(Warn) << " TimeStepperSSC reached minimum stepsize dt= "<<dt<<" at t= "<<t<<endl;
            //exit(StepFinished);
          }
        }
      }

      StepTrials=0;
      StepFinished = 0;
      integrationSteps++; 
      if(order==1)integrationStepsOrder1++;
      if(order>=2)integrationStepsOrder2++;
      t += dte;
      plot();
      zi = ze;
      LS <<= LSe;
      if (outputInterpolation) {
        la <<= lae;
        laSizes = laeSizes;
      }

      if(ConstraintsChanged) integrationStepswithChange++;
      if (t+t*10.0*macheps>tStop) ExitIntegration = 1;
      else ExitIntegration=0;
      if (t+dt+dtMin>tStop) dt= tStop-t;
      if (StepsWithUnchangedConstraints>=0) {
        if (! ConstraintsChanged) UnchangedSteps++;
        else UnchangedSteps =0;
        if (UnchangedSteps >= StepsWithUnchangedConstraints) ExitIntegration = true;
      }

      sysT1->updateInternalState();
    }
  }

  void TimeSteppingSSCIntegrator::plot() {
    bool FlagtPlot = (t>=tPlot);
    if ((FlagPlotEveryStep) || ((t>=tPlot)&&(outputInterpolation==false))) {
      tPlot+=dtPlot;
      sysT1->setTime(t);
      sysT1->setState(ze);
      sysT1->calclaSize(2);
      if(lae.size() == sysT1->getlaSize()) sysT1->getla(false) = lae;
      sysT1->resetUpToDate();
      sysT1->setUpdatela(false);
      sysT1->setUpdateLa(false);
      sysT1->plot();
    }
    if ((t>=tPlot) && outputInterpolation && !FlagPlotEveryStep) {
      throwError("Not implemented");

      // Dimension/size von la und lae synchronisieren
      Vec laSynchron;
      Vec laeSynchron;
      VecInt laSizesSynchron;
      if(t>tPlot) {
        int nLinks = laSizes.size();
        if (nLinks>laeSizes.size()) nLinks=laeSizes.size();
        int nlaAll=0;
        for(int i=0; i<nLinks; i++) {
          if (laSizes(i)>laeSizes(i)) nlaAll+=laeSizes(i);
          else nlaAll+=laSizes(i);
        }
        laSynchron.resize(nlaAll,NONINIT);
        laeSynchron.resize(nlaAll,NONINIT);
        laSizesSynchron.resize(nlaAll,NONINIT);
        int ila=0;
        int ilae=0;
        int iSynchron=0;
        for(int i=0; i<nLinks;i++) {
          laSizesSynchron(i) = laSizes(i);
          if (laSizesSynchron(i)>laeSizes(i)) laSizesSynchron(i) = laeSizes(i);
          laSynchron.set(RangeV(iSynchron,iSynchron+laSizesSynchron(i)-1), la(RangeV(ila,ila+laSizesSynchron(i)-1)));
          laeSynchron.set(RangeV(iSynchron,iSynchron+laSizesSynchron(i)-1), lae(RangeV(ilae,ilae+laSizesSynchron(i)-1)));
          ilae+=laeSizes(i);
          ila+=laSizes(i);
          iSynchron+=laSizesSynchron(i);
        }
      }

      while (t>tPlot) {
        double ratio = (tPlot -(t-dte))/dte;
        sysT1->setTime(tPlot);
        sysT1->setState(zi + (ze-zi)*ratio);
        sysT1->setla(laSynchron+(laeSynchron-laSynchron)*ratio);
        sysT1->resetUpToDate();
        sysT1->setUpdatela(false);
        sysT1->setUpdateLa(false);
        sysT1->plot();
        tPlot += dtPlot;
      }
    }

    if (msgAct(Status) and (not FlagOutputOnlyAtTPlot or (FlagOutputOnlyAtTPlot and FlagtPlot) ))
        msg(Status) << "   t = " << t << ",\tdt = " << dtOld << ",\titer = " << setw(5) << setiosflags(ios::left) << iter << ",\torder = " << order << flush;
  }

  void TimeSteppingSSCIntegrator::postIntegrate() {           // system: only dummy!
    time += Timer.stop();
    int maxStepsPerThread = singleStepsT1;
    if (maxStepsPerThread<singleStepsT2) maxStepsPerThread = singleStepsT2;
    if (maxStepsPerThread<singleStepsT3) maxStepsPerThread = singleStepsT3;
    msg(Info) << "Summary Integration with TimeStepperSSC: "<<endl;
    msg(Info) << "Integration time:   " << time << endl;
    msg(Info) << "Integration steps:  " << integrationSteps << endl;
    msg(Info) << "Evaluations MBS:    " << (singleStepsT1+singleStepsT2+singleStepsT3)<<"   (max/Thread: "<<maxStepsPerThread<<")"<<endl;
    msg(Info) << "Steps with events: " << integrationStepswithChange<< endl;
    if (maxOrder>=2) {
      msg(Info)<<"Integration steps order 1: "<<integrationStepsOrder1<<endl;
      msg(Info)<<"Integration steps order "<<maxOrder<<": "<<integrationStepsOrder2<<endl;
    }
    if (FlagSSC) {
      msg(Info) << "Refused steps: " << refusedSteps << endl;
      msg(Info) << "Refused steps with events: " << refusedStepsWithImpact << endl;
      msg(Info) << "Maximum step size: " << maxdtUsed << endl;
      msg(Info) << "Minimum step size: " << mindtUsed << endl;
      msg(Info) << "Average step size: " << (t-tStart)/integrationSteps << endl;
    }
    if (FlagGapControl) {
      if (GapControlStrategy>0) {
        msg(Info) << "Steps accepted after GapControl  :"<<stepsOkAfterGapControl<<endl;
        msg(Info) << "Steps refused after GapControl   :"<<stepsRefusedAfterGapControl<<endl;
        msg(Info) << "No impact after GapControl alert :"<<wrongAlertGapControl<<endl;
      }
      msg(Info) << "Average Penetration  (arithm.)     :"<<Penetration/PenetrationCounter<<endl;
      msg(Info) << "Average Penetration  (geom.)       :"<<exp(PenetrationLog/PenetrationCounter)<<endl;
      msg(Info) << "PenetrationCounter "<<PenetrationCounter<<endl;
      msg(Info) << "Penetration Min    "<<PenetrationMin<<endl;
      msg(Info) << "Penetration Max    "<<PenetrationMax<<endl;
    }
    msg(Info) << "Maximum number of iterations: " << maxIterUsed << endl;
    msg(Info) << "Average number of iterations: " << double(sumIter)/integrationSteps << endl;
  }


  bool TimeSteppingSSCIntegrator::testTolerances() 
  {
    double dtNewRel = 1; 
    bool testOK = true;
    bool IterConvergenceBlock1=true;
    bool IterConvergenceBlock2=true;
    IterConvergence = (iterA<maxIter);
    iter =iterA;

    Vec EstErrorLocal(z1d.size(),NONINIT);

    if (method==extrapolation) {
      if (!FlagSSC) {
        dte =dt;
        if (maxOrder==1) {LSe <<= LSA;     ze = z1d;          }
        if (maxOrder==2) {
          LSe <<= LSB2;
          if (!ConstraintsChanged) {
            ze = 2.0*z2d - z1d; order=2;}
          else {ze=z2d; order=1;}
        }
        if (maxOrder==3) {
          LSe <<= LSB2;
          if (!ConstraintsChanged) {
            ze = 0.5*z1d - 4.0*z2d + 4.5*z3d; order=3;}
          else {ze=z3d; order=1;}
        }
        if (maxOrder==4) {
          LSe <<= LSD6;
          if (!ConstraintsChanged) {
            ze = -1.0/15.0*z1d + z2d + 27.0/5.0*z6d - 16.0/3.0*z4d; order=4;}
          else {ze=z6d; order=1;}
        }

      }
      else {
        if (maxOrder==1) {
          IterConvergenceBlock1 = (iterB1<maxIter);
          IterConvergenceBlock2 = (iterB2<maxIter);
          EstErrorLocal = z2d-z1d;
          dtNewRel = calculatedtNewRel(EstErrorLocal,dt);
          testOK = (dtNewRel>=1.0);
          dtNewRel = sqrt(dtNewRel);
          dte = dt;
          ze  = z2d;
          LSe <<= LSB2;
        }
        if (maxOrder>=2) {
          IterConvergenceBlock1 = (iterB1<maxIter) && (iterC1<maxIter) && (iterC2<maxIter);
          if (calcBlock2) IterConvergenceBlock2 = (iterB2<maxIter) && (iterB2RE<maxIter) && (iterC3<maxIter) && (iterC4<maxIter);
          if (ConstraintsChanged) {  // Prüfe auf Einhaltung der Toleranz im Block 1
            order=1;
            if (maxOrder==2) EstErrorLocal = z2b-z4b;
            if (maxOrder==3 && ConstraintsChangedBlock1) EstErrorLocal = 2.0*(z4b - z6b);
            if (maxOrder==3 && !ConstraintsChangedBlock1) EstErrorLocal = z2b -z4b;
            dtNewRel = calculatedtNewRel(EstErrorLocal,dt);
            testOK = (dtNewRel>=1.0);
            dtNewRel = sqrt(dtNewRel);
            dte= dt/2.0;
            if (ConstraintsChangedBlock1) {
              if (maxOrder==2) {ze = z4b;  LSe <<= LSC2;}
              if (maxOrder==3) {ze = z6b;  LSe <<= LSD3;}
            }
            else {
              ze <<= zStern; order=maxOrder;
              if (maxOrder==2) {LSe <<= LSC2;}
              if (maxOrder==3) {LSe <<= LSD3;}
            }
          }

          if (ConstraintsChanged && !ConstraintsChangedBlock1) {
            if (testOK) {
              if (maxOrder==2) EstErrorLocal = z2dRE-z4d;
              if (maxOrder==3) EstErrorLocal = 2.0*(z4d-z6d);
              double dtNewRelTmp = calculatedtNewRel(EstErrorLocal,dt);
              if (dtNewRelTmp>=1) {
                dtNewRel = sqrt(dtNewRelTmp);
                testOK = true;
                dte = dt;
                if (maxOrder==2) { ze = z4d; LSe <<= LSC4;}
                if (maxOrder==3) { ze = z6d; LSe <<= LSD6;}
                order =1;
              }
            }
          }

          if (!ConstraintsChanged) {
            order = maxOrder;
            if (maxOrder==2) EstErrorLocal = (2.0*z2d-z1d - 2.0*z4d+z2dRE)/3.0;
            if (maxOrder==3) EstErrorLocal = ((4.5*z3d+0.5*z1d-4.0*z2d) - (4.5*z6d+0.5*z2dRE-4.0*z4d))/5.0;
            dtNewRel = calculatedtNewRel(EstErrorLocal,dt);
            testOK = (dtNewRel>=1.0);
            dtNewRel = pow(dtNewRel,1.0/(maxOrder+1));  
            if (maxOrder==2) { ze = 2.0*z4d - z2dRE; LSe <<= LSC4;}
            if (maxOrder==3) { ze = 4.5*z6d+0.5*z2dRE-4.0*z4d; LSe <<= LSD6;}
            dte=dt;
            // order >=2 hat mehrfach versagt! Teste auf order 1
            if (!testOK && (StepTrials>2)) {
              double dtNewRelTmp1=0;
              double dtNewRelTmp2=0;
              if (maxOrder==2) { dtNewRelTmp1 = calculatedtNewRel(z2b - z4b, dt);
                dtNewRelTmp2 = calculatedtNewRel(z4d-z2dRE, dt);}
                if (maxOrder==3) { dtNewRelTmp1 = calculatedtNewRel(2.0*(z4b-z6b), dt);
                  dtNewRelTmp2 = calculatedtNewRel(2.0*(z4d-z6d), dt);}

                  if (dtNewRelTmp1>=1.0) {
                    order = 1;
                    testOK= true;
                    if (dtNewRelTmp2>=1.0) {
                      dtNewRel= sqrt(dtNewRelTmp2);
                      if (maxOrder==2) {ze = z4d; LSe <<= LSC4;}
                      if (maxOrder==3) {ze = z6d; LSe <<= LSD6;}
                    }
                    else {
                      dtNewRel= sqrt(dtNewRelTmp1);  
                      if (maxOrder==2) {ze = z4b; LSe <<= LSC2;}
                      if (maxOrder==3) {ze = z6b; LSe <<= LSD3;}
                      dte= dt/2.0;
                    }
                  }   
                  if (testOK) msg(Warn)<<"High order refused; order 1 accepted."<<endl; 
            }

          }     
        } //endif maxOrder>=2
      }   //endelse !FlagSSC
    } // endif method==extrapolation

    if (method) {
      order = maxOrder;

      if(maxOrder==1) {
        IterConvergenceBlock1 = (iterB1<maxIter);
        IterConvergenceBlock2 = (iterB2<maxIter);
        EstErrorLocal = z2d-z1d;
        if (!ConstraintsChanged) EstErrorLocal = EstErrorLocal*2.0;
        dtNewRel = calculatedtNewRel(EstErrorLocal,dt);
        testOK = (dtNewRel>=1.0);
        dte = dt;
        LSe <<= LSB2;
        if (ConstraintsChanged) ze = z2d;
        if (!ConstraintsChanged && method==embedded) ze=z1d;
        if (!ConstraintsChanged && method==embeddedHigherOrder) ze=2.0*z2d - z1d;
      }

      if (maxOrder>1) {
        IterConvergenceBlock1 = (iterB1<maxIter) && (iterC1<maxIter) && (iterC2<maxIter);
        if(calcBlock2) IterConvergenceBlock2 = (iterB2<maxIter) && (iterC3<maxIter) && (iterC4<maxIter); 

        if(ConstraintsChanged) {  // Pruefe Block1
          order =1;
          if (maxOrder==2) EstErrorLocal = z2b-z4b;
          if (maxOrder>=3) EstErrorLocal = 2.0*(z4b - z6b);
          dtNewRel = calculatedtNewRel(EstErrorLocal,dt);
          testOK = (dtNewRel>=1.0);
          dtNewRel = sqrt(dtNewRel);
          dte= dt/2.0;
          if (maxOrder==2)  {ze = z4b;  LSe <<= LSC2;}
          if (maxOrder>=3)  {ze = z6b;  LSe <<= LSD3;}
        }

        if (ConstraintsChanged && !ConstraintsChangedBlock1) { // Pruefe Block2
          if (testOK) {
            if (maxOrder==2) EstErrorLocal = z2d-z4d;
            if (maxOrder>=3) EstErrorLocal = 2.0*(z4d-z6d);
            double dtNewRelTmp = calculatedtNewRel(EstErrorLocal,dt);
            if (dtNewRelTmp>=1) {
              dtNewRel = sqrt(dtNewRelTmp);
              testOK = true;
              dte = dt;
              order=1;
              if (maxOrder==2) { ze = z4d; LSe <<= LSC4;}
              if (maxOrder>=3) { ze = z6d; LSe <<= LSD6;}
            }
          }
        }
        if (!ConstraintsChanged) {
          Vec zOp, zOp1;
          if (maxOrder==2) { 
            zOp= 2.0*z2d - z1d;  
            zOp1= z1d/3.0 - 2.0*z2d + 8.0/3.0*z4d;
          }
          if (maxOrder==3) { 
            zOp1= -1.0/15.0*z1d + z2d + 27.0/5.0*z6d - 16.0/3.0*z4d; 
            zOp = z1d/3.0 - 2.0*z2d + 8.0/3.0*z4d;
          }
          if (maxOrder==4) { 
            zOp = -1.0/6.0*z1d + 4.0*z2d -27.0/2.0*z3d + 32.0/3.0*z4d;
            zOp1= 1.0/30.0*z1d - 2.0*z2d +27.0/2.0*z3d - 64.0/3.0*z4d  + 54.0/5.0*z6d; 
          }

          LSe <<= LSC4;
          dte=dt;
          if (method==embedded) ze = zOp;
          if (method==embeddedHigherOrder) ze = zOp1;

          EstErrorLocal = zOp -zOp1;
          dtNewRel = calculatedtNewRel(EstErrorLocal,dt);
          testOK = (dtNewRel>=1.0);
          dtNewRel = pow(dtNewRel,1.0/(maxOrder+1)); 

          if (FlagSSC && !testOK && (StepTrials>2)) {
            double dtNewRelTmp1=0;
            double dtNewRelTmp2=0;
            if (maxOrder==2) { dtNewRelTmp1 = calculatedtNewRel(z2b - z4b, dt);
              dtNewRelTmp2 = calculatedtNewRel(z2d - z4d, dt);}
              if (maxOrder>=3) { dtNewRelTmp1 = calculatedtNewRel(2.0*(z4b-z6b), dt);
                dtNewRelTmp2 = calculatedtNewRel(2.0*(z4d-z6d), dt);}

                if (dtNewRelTmp1>=1.0) {
                  order = 1;
                  testOK= true;
                  if (dtNewRelTmp2>=1.0) {
                    dtNewRel= sqrt(dtNewRelTmp2);
                    if (maxOrder==2) {ze = z4d; LSe <<= LSC4;}
                    if (maxOrder>=3) {ze = z6d; LSe <<= LSD6;}
                  }
                  else {
                    dtNewRel= sqrt(dtNewRelTmp1);  
                    if (maxOrder==2) {ze = z4b; LSe <<= LSC2;}
                    if (maxOrder>=3) {ze = z6b; LSe <<= LSD3;}
                    dte= dt/2.0;
                  }
                }  
                if (testOK) msg(Warn)<<"Hohe Ordnung abgeleht aber dafuer Order 1 akzeptiert!!!"<<endl; 
          }

        } //endif !ConstraintsChanged

      }

    } //endif method>0

    if (dte<dt) {		// nur halber Integrationsschritt wurde akzeptiert dte==dt/2
      lae <<= la2b;
      laeSizes = la2bSizes;
      iter = iterB1;
      ConstraintsChanged = ConstraintsChangedBlock1;
      IterConvergenceBlock2 = true;
    }
    else {
      lae <<= la1d;
      laeSizes = la1dSizes;
      iter = iterA;
    }

    if (FlagSSC) { 
      IterConvergence = IterConvergence && IterConvergenceBlock1 && IterConvergenceBlock2;
      if ((! IterConvergence)&&(testOK)) {
        if (dt/2.0>dtMin) {
          testOK= false; 
          dt=dt/2.0;
          //msg(Warn)<<"step size halved because of failed convergence"<<endl;
        }
        else {
          msg(Warn)<<"No convergence despite minimum stepsize("<<maxIter<<" iterations) Anyway, continuing integration..."<<endl;
        }
      }
      else {
        if(dtNewRel<0.5/safetyFactorSSC) dtNewRel = 0.5/safetyFactorSSC;
        double dtRelGap = (FlagGapControl && GapControlStrategy>0) ? dtRelGapControl*0.7 + 0.3 : 1.0;
        // erlaubt groeseres dtNewRel, falls vorher dt durch GapControl verkleinert wurde
        if(dtNewRel > maxGainSSC/dtRelGap) dtNewRel=maxGainSSC/dtRelGap;   //  0.6 oder 0.7 und 2.5 oder 2  (0.7/2.2 alt)
        dt = dt*dtNewRel*safetyFactorSSC;
      }  

      if (dt > dtMax) dt = dtMax;
      if (dt < dtMin) dt = dtMin;
      if (testOK && FlagGapControl) getDataForGapControl();  // verwendet wird sysT1, SetValuedLinkListT1, ze t, dte
      if (FlagGapControl && GapControlStrategy>0) {
        if (ChangeByGapControl && testOK) stepsOkAfterGapControl++;
        if (ChangeByGapControl && !testOK)stepsRefusedAfterGapControl++;
        if (testOK) { 
          // Mittelwert berechnen
          Vec Dq;
          Dq= z1d(RangeV(0,qSize)) - ze(RangeV(0,qSize));
          double mDq=0;
          for(int i=0; i<qSize; i++) mDq +=fabs(Dq(i));
          mDq=mDq/qSize;
          // Standardabweichung
          double sDq=0;
          for(int i=0; i<qSize; i++) sDq +=(fabs(Dq(i))-mDq)*(fabs(Dq(i))-mDq);
          sDq=sqrt(sDq)/qSize;
          qUncertaintyByExtrapolation = 1.05*mDq+sDq;
        }
        ChangeByGapControl = GapControl(qUncertaintyByExtrapolation, testOK);
      }
    }
    else testOK=true;
    if (FlagGapControl && testOK) {
      for(int i=0; i<gUniActive.size(); i++){
        Penetration -= gUniActive(i);
        PenetrationLog += log(fabs(gUniActive(i)));
        PenetrationCounter+=1.0;
        if(gUniActive(i)>PenetrationMin) PenetrationMin=gUniActive(i);
        if(gUniActive(i)<PenetrationMax) PenetrationMax=gUniActive(i);
      }
    }
    return testOK;
  }

  double TimeSteppingSSCIntegrator::calculatedtNewRel(const fmatvec::Vec &ErrorLocal, double H) {
    if (!FlagSSC) return 1.0;
    double dtNewRel=1.0e10; 
    double dtNewRel_i;
    double ResTol_i;
    double Hscale=1;
    bool includeVelocities = (FlagErrorTest!=3);
    if ((FlagErrorTest==3) && (!FlagErrorTestAlwaysValid) && (!ConstraintsChanged)) includeVelocities=true;
    if ((FlagErrorTest==2) && (FlagErrorTestAlwaysValid || ConstraintsChanged)) Hscale=H;

    for (int i=0; i< zSize; i++) {
      if((i<qSize) || (i>qSize+uSize) || includeVelocities) { 
        if ((i>=qSize)&&(i<qSize+uSize))
          ResTol_i = aTol(i)/Hscale +  rTol(i)*fabs(zi(i));
        else
          ResTol_i = aTol(i) + rTol(i)*fabs(zi(i));
        if(ErrorLocal(i)!=0) {
          dtNewRel_i = ResTol_i / fabs(ErrorLocal(i));
          if (dtNewRel_i < dtNewRel) dtNewRel = dtNewRel_i;
        }
      }
    }
    return dtNewRel;
  }

  bool TimeSteppingSSCIntegrator::GapControl(double qUnsafe, bool SSCTestOK) 
  {

    // Strategien unterscheiden sich, falls mehr als ein moeglicher stoss im Intervall [0 dt]

    // Strategie  1: Weiter hinter groessten Nullstelle, aber mindestens 0.3*dt_SSC (schnellster Integrationsfortschritt)
    // Strategie  2: Scoreverfahren: weiter hinter NS mit groesstem Score 
    // Strategie  3: Groesste NS so dass gapTol eingehalten wird
    // Strategie  4: Weiter hinter der kleinsten Nullstelle (geringste Penetration)
    // Strategie  5: event detection: 
    //               Integration bis kurz vor NS, dann mit kleiner dt ueber event hinweg und weiter mit alter dt
    // Strategie  0: GapControl wieder deaktiviert (z.B. um Penetration auszugeben)
    double dtOrg=dt;    

    if (GapControlStrategy==noGapControl) {dtRelGapControl = 1; return false;}
    if (statusGapControl>2) {statusGapControl=0; indexLSException=-1; }
    if (statusGapControl && !SSCTestOK) statusGapControl=0;
    if (statusGapControl==2) { 
      if (ConstraintsChanged && SSCTestOK) 
        dt= 0.75*dt_SSC_vorGapControl;
      else statusGapControl=0;
    }

    double NSi=-1;              // groesste Nullstelle im Bereich [0,dt]     (innen)
    double NSiMin=2*dt;         // kleinste Nullstelle im Bereich [dtmin; dt](innen)
    double NSa=-1;              // groesste Nullstelle im Bereich ]dt 1.5dt] (aussen) 
    double NS_=-1;              // zum Speichern der NS fuer z.B. Strategie 1 oder 2
    double NS;
    double Hold=dt;
    double ScoreMax = -1;
    double tTolMin = dt;
    int gInActiveSize = gInActive.size();
    int IndexNS=-1;
    int IndexNSMin=-1;

    for(int i=0; i<gInActiveSize; i++) {
      if (gdInActive(i)>0.0 || gdInActive(i)<0.0) NS = -gInActive(i)/gdInActive(i);
      else NS = -1;
      if (NS>0) {
        if (NS<=dt && NS>NSi) {NSi=NS;  IndexNS=i;}
        if (NS>dtMin && NS<=dt && NS<NSiMin) {NSiMin=NS; IndexNSMin=i;}
        if (NS>dt && NS<=1.5*dt && NS>NSa) NSa=NS;  

        if (GapControlStrategy==scoring && NS<=dt) {
          double Score = fabs(gdInActive(i))*(dt-NS)*NS;
          if (Score>ScoreMax) {ScoreMax=Score; NS_= NS;}
        }
        if(GapControlStrategy==gapTolerance && NS<=dt) {
          double tTol = NS + fabs(gapTol/gdInActive(i));
          if (tTol<tTolMin) {tTolMin= tTol;  NS_=NS;}
        }
      }
    }

    if (statusGapControl==2 && ConstraintsChanged && SSCTestOK) {
      // identifiziere Eintrag in LS Vektor der zu diesem Stoss gehoert; falls sich nur ein Eintrag geaendert hat
      // wird maxOrder erzwungen dadurch dass dieser Eintrag im Vergleich der LS unberuecksichtigt bleibt
      statusGapControl=0;
      int n=0;
      int ni=-1;
      if(LS.size()==LSe.size()) {
        for(int i=0; i<LS.size(); i++) if (LSe(i)!=LS(i)) {n++; ni=i;}
      }
      if(n==1) indexLSException=ni;
      if (n==1 && NSi<=0) statusGapControl=3;
    }

    if (NSi>0){ 
      if (GapControlStrategy<=4 && GapControlStrategy) dt= safetyFactorGapControl*NSi+1e-15;
      if (GapControlStrategy==biggestRoot && dt/Hold<0.3) dt =0.3*Hold;
      if (GapControlStrategy==scoring && NS_>0) dt= safetyFactorGapControl*NS_ +1e-15;
      if (GapControlStrategy==gapTolerance && NS_>0) dt= safetyFactorGapControl*NS_ +1e-15;
      if (GapControlStrategy==smallestRoot && NSiMin<NSi) dt=safetyFactorGapControl*NSiMin+1e-15;
      if (GapControlStrategy==5) {
        if(NSi<NSiMin) {NSiMin=NSi; IndexNSMin=IndexNS;}
        double dtUnsafe = fabs(qUnsafe/gdInActive(IndexNSMin));
        dtUnsafe *= NSiMin/dte; //pow(NS/dte,order);
        if(dtUnsafe*10<dte) {  // Falls Unsicherheit zu gross macht gapContol keinen Sinn !
          if(dtUnsafe<0.4*dtMin) dtUnsafe=0.4*dtMin;
          if (statusGapControl==0) {
            dt_SSC_vorGapControl= dt;
            statusGapControl=1;
            dt = NSiMin- (safetyFactorGapControl-1)*NSiMin;
            if (dt<=dtMin) {dt=dtMin; statusGapControl=2;}
            else {
              dt = dt -dtUnsafe;
              if (dt<=dtMin) dt=dtMin;
            }
          }
          else {  //statusGapControl==1
            statusGapControl=2;
            dt = NSiMin*safetyFactorGapControl+dtUnsafe;
            if (dt<dtMin) dt=dtMin; 
          }
        } else statusGapControl=0;
      }
    }
    else {
      if(NSa>0 && GapControlStrategy && GapControlStrategy<=4){ 
        dt = NSa*0.5;}
      if (statusGapControl && statusGapControl<3) statusGapControl=0;
    }
    if (dt<0.75*dtMin) dt=0.75*dtMin;

    dtRelGapControl = dt/Hold;

    return (dt<dtOrg);
  }

  // SetValuedLinkLists are build up within preIntegrate 
  // The Lagragian Multiplier of all Links stored in SetValuedLinkList of the corresponding DynamicSystemSolver (sysT1)
  // are collecte and stored in Vec la
  // Vector<int> laSizes contains the singel la-size of each link

  void TimeSteppingSSCIntegrator::getDataForGapControl() {
    throwError("TimeSteppingSSCIntegrator::getDataForGapControl not implemented");
//    int nInActive=0;
//    int nActive=0;
//    throw;
//    for(unsigned int i=0; i<SetValuedLinkListT1.size(); i++){ 
//      SetValuedLinkListT1[i]->checkActive(1);
//      SetValuedLinkListT1[i]->SizeLinearImpactEstimation(&nInActive, &nActive);
//    }
//    gInActive.resize(nInActive,NONINIT);
//    gdInActive.resize(nInActive,NONINIT);
//    gUniActive.resize(nActive,NONINIT);
//    nInActive=0;
//    nActive=0;
//    for(unsigned int i=0; i<SetValuedLinkListT1.size(); i++) 
//      SetValuedLinkListT1[i]->LinearImpactEstimation(t,gInActive,gdInActive,&nInActive,gUniActive,&nActive);
  }

  bool TimeSteppingSSCIntegrator::changedLinkStatus(const VecInt &L1, const VecInt &L2, int ex) {
    if (ex<0) return (L1!=L2);
    int n=L1.size();
    if (n!=L2.size()) return true;
    for(int i=0; i<n; i++) 
      if (i!=ex && L1(i)!=L2(i)) return true;
    return false;
  }

  void TimeSteppingSSCIntegrator::initializeUsingXML(DOMElement *element) {

    Integrator::initializeUsingXML(element);
    DOMElement *e;

    e=E(element)->getFirstElementChildNamed(MBSIM%"method");
    if (e) {
      string methodStr=string(X()%E(e)->getFirstTextChild()->getData()).substr(1,string(X()%E(e)->getFirstTextChild()->getData()).length()-2);
      if (methodStr=="extrapolation") setMethod(extrapolation);
      else if (methodStr=="embedded") setMethod(embedded);
      else if (methodStr=="embeddedHigherOrder") setMethod(embeddedHigherOrder);
      else setMethod(unknownMethod);
    }

    e=E(element)->getFirstElementChildNamed(MBSIM%"stepSizeControl");
    if (e) setStepSizeControl(E(e)->getText<bool>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"absoluteTolerance");
    if (e) setAbsoluteTolerance(E(e)->getText<Vec>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"absoluteToleranceScalar");
    if (e) setAbsoluteTolerance(E(e)->getText<double>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"relativeTolerance");
    if (e) setRelativeTolerance(E(e)->getText<Vec>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"relativeToleranceScalar");
    if (e) setRelativeTolerance(E(e)->getText<double>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"initialStepSize");
    if (e) setInitialStepSize(E(e)->getText<double>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"maximumStepSize");
    if (e) setMaximumStepSize(E(e)->getText<double>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"minimumStepSize");
    if (e) setMinimumStepSize(E(e)->getText<double>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"maximumOrder");
    if (e) setMaximumOrder(E(e)->getText<int>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"outputInterpolation");
    if (e) setOutputInterpolation(E(e)->getText<bool>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"gapControl");
    if (e) {
      string gapControlStr=string(X()%E(e)->getFirstTextChild()->getData()).substr(1,string(X()%E(e)->getFirstTextChild()->getData()).length()-2);
      if (gapControlStr=="noGapControl") setGapControl(noGapControl);
      else if (gapControlStr=="biggestRoot") setGapControl(biggestRoot);
      else if (gapControlStr=="scoring") setGapControl(scoring);
      else if (gapControlStr=="gapTolerance") setGapControl(gapTolerance);
      else if (gapControlStr=="smallestRoot") setGapControl(smallestRoot);
      else setGapControl(unknownGapControl);
    }

    e=E(element)->getFirstElementChildNamed(MBSIM%"gapTolerance");
    if (e) setgapTolerance(E(e)->getText<double>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"errorTest");
    if (e) {
      string errorTestStr=string(X()%E(e)->getFirstTextChild()->getData()).substr(1,string(X()%E(e)->getFirstTextChild()->getData()).length()-2);
      if (errorTestStr=="all") setErrorTest(all);
      else if (errorTestStr=="scale") setErrorTest(scale);
      else if (errorTestStr=="exclude") setErrorTest(exclude);
      else setErrorTest(unknownErrorTest);
    }

    e=E(element)->getFirstElementChildNamed(MBSIM%"maximumGain");
    if (e) setMaximumGain(E(e)->getText<double>());

    e=E(element)->getFirstElementChildNamed(MBSIM%"safetyFactor");
    if (e) setSafetyFactor(E(e)->getText<double>());
  }

  void TimeSteppingSSCIntegrator::resize(DynamicSystemSolver *system) {
    system->calcgdSize(2); // contacts which stay closed
    system->calclaSize(2); // contacts which stay closed
    system->calcrFactorSize(2); // contacts which stay closed

    system->updateWRef(system->getWParent(0));
    system->updateVRef(system->getVParent(0));
    system->updatelaRef(system->getlaParent());
    system->updateLaRef(system->getLaParent());
    system->updategdRef(system->getgdParent());
    if (system->getImpactSolver() == DynamicSystemSolver::rootfinding)
      system->updateresRef(system->getresParent());
    system->updaterFactorRef(system->getrFactorParent());
  }

}
