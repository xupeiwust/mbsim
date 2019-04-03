/* Copyright (C) 2004-2019 MBSim Development Team
 *
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
#include "gearwheel_gearrack.h"
#include "mbsim/frames/contour_frame.h"
#include "mbsim/contours/gear_wheel.h"
#include "mbsim/contours/gear_rack.h"
#include <mbsim/utils/rotarymatrices.h>

using namespace fmatvec;
using namespace std;

namespace MBSim {

  void ContactKinematicsGearWheelGearRack::assignContours(const vector<Contour*> &contour) {
    if(dynamic_cast<GearWheel*>(contour[0])) {
      igearwheel = 0; igearrack = 1;
      gearwheel = static_cast<GearWheel*>(contour[0]);
      gearrack = static_cast<GearRack*>(contour[1]);
    }
    else {
      igearwheel = 1; igearrack = 0;
      gearwheel = static_cast<GearWheel*>(contour[1]);
      gearrack = static_cast<GearRack*>(contour[0]);
    }
    m = static_cast<GearWheel*>(contour[0])->getModule()/cos(static_cast<GearWheel*>(contour[0])->getHelixAngle());
    al0 = atan(tan(static_cast<GearWheel*>(contour[0])->getPressureAngle())/cos(static_cast<GearWheel*>(contour[0])->getHelixAngle()));
    double phi0 = tan(al0)-al0;
    double s0 = m*M_PI/2;
    double dk[2];
    z[0] = gearwheel->getNumberOfTeeth();
    d0[0] = m*z[0];
    db[0] = d0[0]*cos(al0);
    dk[0] = d0[0]+2*m;
    rb[0] = db[0]/2;
    sb[0] = db[0]*(s0/d0[0]+phi0)-gearwheel->getBacklash();
    ga[0] = sb[0]/rb[0]/2;
    beta[0] = gearwheel->getHelixAngle();
    beta[1] = gearrack->getHelixAngle();
    delmin[0] = -al0-ga[0];
    delmax[0] = tan(acos(db[0]/dk[0]))-al0-ga[0];
    delmin[1] = -m/cos(al);
    delmax[1] = m/cos(al);
    z[1] = gearrack->getNumberOfTeeth();
    sb[1] = m*(M_PI/2 + 2*tan(al0)) - gearrack->getBacklash(); 
  }

  void ContactKinematicsGearWheelGearRack::updateg(SingleContact &contact, int ii) {
    contact.getGeneralizedRelativePosition(false)(0) = 1e10;
    double eta[2], del[2];
    al = al0;
    Vec3 r = gearwheel->getFrame()->evalPosition() - gearrack->getFrame()->evalPosition();
    Vec3 e = gearrack->getFrame()->getOrientation().col(1);
    double rp = r.T()*e;
    double rs = sqrt(r.T()*r-rp*rp);
    double s0h = sb[1]/2-m*tan(al);
    for(int i=0; i<2; i++) {
      int signi = i?-1:1;
      Vec3 rOP[2], rSP[2];
      vector<int> v[2];
      if(maxNumContacts==1) {
        int k = 0;
        double rsi = 1e10;
        for(int k_=0; k_<z[1]; k_++) {
          double rsi_ = fabs(rs - k_*M_PI*m + signi*s0h);
          if(rsi_<rsi) {
            rsi = rsi_;
            k = k_;
          }
        }
        v[1].push_back(k);

        Vec3 rP1S2 = gearrack->getFrame()->getPosition() + gearrack->getFrame()->getOrientation().col(0)*(k*M_PI*m-signi*s0h) - gearwheel->getFrame()->getPosition();
        double cdel = 0;
        k = 0;
        for(int k_=0; k_<z[0]; k_++) {
          double ep = k_*2*M_PI/z[0]+signi*ga[0];
          rSP[0](0) = -sin(ep);
          rSP[0](1) = cos(ep);
          rSP[0] = gearwheel->getFrame()->getOrientation()*rSP[0];
          double cdel_ = rSP[0].T()*rP1S2/nrm2(rP1S2);
          if(cdel_>cdel) {
            cdel = cdel_;
            k = k_;
          }
        }
        v[0].push_back(k);
      }
      else {
        for(int k_=0; k_<z[1]; k_++) {
          double rsi = rs - k_*M_PI*m;
          double rpi = rp - d0[0]/2;
          double l = sin(al)*(s0h+signi*rsi)+rpi*cos(al); 
          if(l>delmin[1] and l<delmax[1])
            v[1].push_back(k_);
        }
        for(int k_=0; k_<z[0]; k_++) {
          double ep = k_*2*M_PI/z[0];
          rSP[0](0) = -sin(ep);
          rSP[0](1) = cos(ep);
          rSP[0] = gearwheel->getFrame()->getOrientation()*rSP[0];
          double cdel = -(rSP[0].T()*gearrack->getFrame()->getOrientation().col(1));
          double del = signi*(gearwheel->getFrame()->getOrientation().col(2).T()*crossProduct(gearrack->getFrame()->getOrientation().col(1),rSP[0])>=0?-acos(cdel):acos(cdel));
          if(del>delmin[0] and del<delmax[0])
            v[0].push_back(k_);
        }
      }

      double k[2];
      for (auto & i0 : v[0]) {
        for (auto & i1 : v[1]) {
          k[0] = i0;
          k[1] = i1;
          if(ii==0 or not(k[0]==ksave[0][0] and k[1]==ksave[0][1])) {
            double rsi = rs - k[1]*M_PI*m;
            double rpi = rp - d0[0]/2;
            double l = sin(al)*(s0h+signi*rsi)+rpi*cos(al); 
            rSP[1](0) = signi*l*sin(al);
            rSP[1](1) = l*cos(al);
            rSP[1] = gearrack->getFrame()->getOrientation()*rSP[1];
            rOP[1] = gearrack->getFrame()->getPosition() + gearrack->getFrame()->getOrientation().col(0)*(k[1]*M_PI*m-signi*s0h) + rSP[1];

            rSP[0](0) = -sin(k[0]*2*M_PI/z[0]);
            rSP[0](1) = cos(k[0]*2*M_PI/z[0]);
            rSP[0] = gearwheel->getFrame()->getOrientation()*rSP[0];
            double cdel = -(rSP[0].T()*gearrack->getFrame()->getOrientation().col(1));
            del[0] = signi*(gearwheel->getFrame()->getOrientation().col(2).T()*crossProduct(gearrack->getFrame()->getOrientation().col(1),rSP[0])>=0?-acos(cdel):acos(cdel));
            eta[0] = ga[0] + del[0] + al;
            rSP[0](0) = signi*rb[0]*(sin(eta[0])-cos(eta[0])*eta[0]);
            rSP[0](1) = rb[0]*(cos(eta[0])+sin(eta[0])*eta[0]);
            rSP[0] = gearwheel->getFrame()->getOrientation()*BasicRotAIKz(signi*ga[0]+k[0]*2*M_PI/z[0])*rSP[0];
            rOP[0] = gearwheel->getFrame()->getPosition() + rSP[0];

            Vec3 n2(NONINIT);
            n2(0) = -signi*cos(al);
            n2(1) = sin(al);
            n2(2) = -signi*cos(al)*tan(beta[1]);
            n2 /= sqrt(1+pow(cos(al)*tan(beta[1]),2));
            n2 = gearrack->getFrame()->getOrientation()*n2;

            Vec3 u2(NONINIT);
            u2(0) = signi*sin(al);
            u2(1) = cos(al);
            u2(2) = 0;

            double g = n2.T()*(rOP[2]-rOP[1]);
            if(g>-0.5*M_PI*d0[0]/z[0] and g<contact.getGeneralizedRelativePosition(false)(0)) {
              ksave[ii][0] = k[0];
              ksave[ii][1] = k[1];
              etasave[ii] = eta[0];
              signisave[ii] = signi;
              delsave = del[0];

              contact.getContourFrame(igearrack)->setPosition(rOP[1]);
              contact.getContourFrame(igearrack)->getOrientation(false).set(0,n2);
              contact.getContourFrame(igearrack)->getOrientation(false).set(1,u2);
              contact.getContourFrame(igearrack)->getOrientation(false).set(2,crossProduct(n2,contact.getContourFrame(igearrack)->getOrientation(false).col(1)));

              contact.getContourFrame(igearwheel)->setPosition(rOP[0]);
              contact.getContourFrame(igearwheel)->getOrientation(false).set(0, -contact.getContourFrame(igearrack)->getOrientation(false).col(0));
              contact.getContourFrame(igearwheel)->getOrientation(false).set(1, -contact.getContourFrame(igearrack)->getOrientation(false).col(1));
              contact.getContourFrame(igearwheel)->getOrientation(false).set(2, contact.getContourFrame(igearrack)->getOrientation(false).col(2));

              contact.getGeneralizedRelativePosition(false)(0) = g;
            }
          }
        }
      }
    }
  }

  void ContactKinematicsGearWheelGearRack::updatewb(SingleContact &contact, int ii) {
    const Vec3 n1 = contact.getContourFrame(igearwheel)->evalOrientation().col(0);
    const Vec3 vC1 = contact.getContourFrame(igearwheel)->evalVelocity();
    const Vec3 vC2 = contact.getContourFrame(igearrack)->evalVelocity();
    const Vec3 u1 = contact.getContourFrame(igearwheel)->evalOrientation().col(1);
    const Vec3 u2 = contact.getContourFrame(igearrack)->evalOrientation().col(1);
    Vec3 R[2], N1;

    R[0](0) = signisave[ii]*rb[0]*sin(etasave[ii])*etasave[ii];
    R[0](1) = rb[0]*cos(etasave[ii])*etasave[ii];
    R[0] = gearwheel->getFrame()->getOrientation()*BasicRotAIKz(signisave[ii]*ga[0]+ksave[ii][0]*2*M_PI/z[0])*R[0];

    R[1](0) = signisave[ii]*sin(al);
    R[1](1) = cos(al);
    R[1] = gearrack->getFrame()->getOrientation()*R[1];

    N1(0) = signisave[ii]*sin(etasave[ii])/sqrt(1+pow(cos(al)*tan(beta[0]),2));
    N1(1) = cos(etasave[ii])/sqrt(1+pow(cos(al)*tan(beta[0]),2));
    N1 = gearwheel->getFrame()->getOrientation()*BasicRotAIKz(signisave[ii]*ga[0]+ksave[ii][0]*2*M_PI/z[0])*N1;

    const Vec3 parnPart1 = crossProduct(gearwheel->getFrame()->evalAngularVelocity(),n1);
    const Vec3 paruPart2 = crossProduct(gearrack->getFrame()->evalAngularVelocity(),u2);
    const Vec3 parWvCParZeta1 = crossProduct(gearwheel->getFrame()->evalAngularVelocity(),R[0]);
    const Vec3 parWvCParZeta2 = crossProduct(gearrack->getFrame()->evalAngularVelocity(),R[1]);

    SqrMat A(2,NONINIT);
    A(0,0)=-u1.T()*R[0].col(0);
    A(0,1)=u1.T()*R[1].col(0);
    A(1,0)=u2.T()*N1.col(0);
    A(1,1)=0;

    Vec b(2,NONINIT);
    b(0)=-u1.T()*(vC2-vC1);
    b(1)=-u2.T()*parnPart1-n1.T()*paruPart2;

    Vec zetad = slvLU(A,b);

    if(contact.isNormalForceLawSetValued())
      contact.getwb(false)(0) += (N1*zetad(0)+parnPart1).T()*(vC2-vC1)+n1.T()*(parWvCParZeta2*zetad(1)-parWvCParZeta1*zetad(0));
    if(contact.isTangentialForceLawSetValuedAndActive())
      throw runtime_error("Tangential force law must be single valued for gear wheel to gear wheel contacts");
  }

}