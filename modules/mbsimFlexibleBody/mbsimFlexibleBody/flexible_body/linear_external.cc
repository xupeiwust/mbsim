/* Copyright (C) 2004-2015 MBSim Development Team
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
 * Contact: thorsten.schindler@mytum.de
 *          rzander@users.berlios.de
 */

#include <config.h>
#include "mbsimFlexibleBody/flexible_body/linear_external.h"
#include "mbsimFlexibleBody/flexible_body/fe/superelement_linear_external.h"
#include "mbsim/frames/frame.h"
#include "mbsim/contours/contour.h"
#include "mbsim/dynamic_system_solver.h"

#include <iostream>

using namespace fmatvec;
using namespace std;
using namespace MBSim;

namespace MBSimFlexibleBody {

  FlexibleBodyLinearExternal::FlexibleBodyLinearExternal(const string &name) 
	:FlexibleBody(name),
	nContours(0), WrON00(Vec(3)) {
//	  JT << DiagMat(3,INIT,1.0);
	  discretization.push_back(new SuperElementLinearExternal());
	  qElement.push_back(Vec(0));
	  uElement.push_back(Vec(0));
	}

  void FlexibleBodyLinearExternal::GlobalVectorContribution(int i, const fmatvec::Vec& locVec, fmatvec::Vec& gloVec) {
    gloVec += locVec;
  }

  void FlexibleBodyLinearExternal::GlobalMatrixContribution(int i, const fmatvec::Mat& locMat, fmatvec::Mat& gloMat) {
    gloMat += locMat;
  }

  void FlexibleBodyLinearExternal::GlobalMatrixContribution(int i, const fmatvec::SymMat& locMat, fmatvec::SymMat& gloMat) {
    gloMat += locMat;
  }

  void FlexibleBodyLinearExternal::setProportionalDamping(const double &a, const double &b) {
	static_cast<SuperElementLinearExternal*>(discretization[0])->setProportionalDamping(a,b);
  }
  ContourPointData FlexibleBodyLinearExternal::addInterface(const Mat &J, const Vec &r) {
	return static_cast<SuperElementLinearExternal*>(discretization[0])->addInterface(J,r);
  }

  void FlexibleBodyLinearExternal::updateStateDependentVariables(double t) {
    if(nrm2(qElement[0]-q)>0) qElement[0] >> q;
    if(nrm2(uElement[0]-u)>0) uElement[0] >> u;

//	updateKinematicsForFrame(t); 
//	updateContours(t); 
  }

  void FlexibleBodyLinearExternal::updateKinematicsForFrame(ContourPointData &cp, Frame::Feature ff, Frame *frame) {
	Mat J = static_cast<SuperElementLinearExternal*>(discretization[0])->computeJacobianOfMotion(qElement[0],cp);
//TODO  frame[i]->setWrOP(WrON00  + JT *  static_cast<SuperElementLinearExternal*>(discretization[0])->computeTranslation(qElement[0],cp)            );
//TODO  frame[i]->setWvP (          JT *  static_cast<SuperElementLinearExternal*>(discretization[0])->computeTranslationalVelocity(qElement[0],uElement[0],cp));
	if(frame!=NULL) {
	    // TODO
	}
  }

  void FlexibleBodyLinearExternal::updateJacobiansForFrame(ContourPointData &cp, Frame *frame) {
	Mat Jacobian;
	if(cp.getContourParameterType() == ContourPointData::node) {
	  Jacobian = discretization[0]->computeJacobianOfMotion(qElement[0],cp);
	} else if(cp.getContourParameterType() == ContourPointData::extInterpol) {
//	  for(unsigned int i=0;i<(cp.iPoints).size();i++) {
//		ContourPointData cpTemp; cpTemp.getContourParameterType()=ContourPointData::node;
//		cpTemp.getNodeNumber() = cp.iPoints[i]->getID();
//		if(!Jacobian.rows()) Jacobian  = cp.iWeights(i) * ( computeJacobianMatrix (cpTemp) );
//		else                 Jacobian += cp.iWeights(i) * ( computeJacobianMatrix (cpTemp) );
//	  }
	}

    cp.getFrameOfReference().setJacobianOfTranslation(R->getOrientation()(RangeV(0,2),RangeV(0,1))*Jacobian(RangeV(0,qSize-1),RangeV(0,1)).T());// TODO dimensionen der internen Jacobis
    cp.getFrameOfReference().setJacobianOfRotation   (R->getOrientation()(RangeV(0,2),RangeV(2,2))*Jacobian(RangeV(0,qSize-1),RangeV(2,2)).T());
    // S_.getFrameOfReference().setGyroscopicAccelerationOfTranslation(TODO)
    // S_.getFrameOfReference().setGyroscopicAccelerationOfRotation(TODO)

    if(frame!=NULL) { // frame should be linked to contour point data
      frame->setJacobianOfTranslation(cp.getFrameOfReference().getJacobianOfTranslation());
      frame->setJacobianOfRotation   (cp.getFrameOfReference().getJacobianOfRotation());
      frame->setGyroscopicAccelerationOfTranslation(cp.getFrameOfReference().getGyroscopicAccelerationOfTranslation());
      frame->setGyroscopicAccelerationOfRotation   (cp.getFrameOfReference().getGyroscopicAccelerationOfRotation());
    }   
  }


  void FlexibleBodyLinearExternal::updateContours(double t) {
////        Vec WrHS(3);
////    //    RHitSphere = 0;
////    
////        for(unsigned int i=0; i<contour.size(); i++)
////    	  if(contourType[i].getContourParameterType()==ContourPointData::node) {
////    	  ContourPointData cp;
////    	  cp.getNodeNumber() = contour[i]->getID();
////    	  Mat J = static_cast<SuperElementLinearExternal*>(discretization[0])->computeJacobianOfMinimalRepresentationRegardingPhysics(qElement[0],cp);
////    //TODO		contour[i]->setWrOP(WrON00  + JT *  static_cast<SuperElementLinearExternal*>(discretization[0])->computeTranslation(qElement[0],cp)            );
////    //	TODO	contour[i]->setWvP (          JT *  static_cast<SuperElementLinearExternal*>(discretization[0])->computeTranslationalVelocity (qElement[0],uElement[0],cp));
////    
////    //	WrHS += contour[i]->getWrOP();
////    //	double R = nrm2(WrOHitSphere-contour[i]->getWrOP());
////    //	if( R > RHitSphere)
////    //	  RHitSphere = R;	
////        }
////    //    WrOHitSphere =  WrHS/nContours;
  }

 
  void FlexibleBodyLinearExternal::setMassMatrix(const SymMat &mat) {
	static_cast<SuperElementLinearExternal*>(discretization[0])->setM(mat);
	uSize[0] = discretization[0]->getuSize();
	uSize[1] = discretization[0]->getuSize(); // TODO
  }
  void FlexibleBodyLinearExternal::readMassMatrix(const string &massfilename) {
	fstream datafile(massfilename,ios::in);
	if (!datafile.is_open()) {
          stringstream error;
	  error << "File " << massfilename << " containing massmatrix not found." << endl;
	  throw runtime_error(error.str());
	}
	Mat MTemp;
	datafile >> MTemp;
	setMassMatrix((SymMat)(MTemp));
	datafile.close();
  }

  void FlexibleBodyLinearExternal::setStiffnessMatrix(const SqrMat &mat) {
	static_cast<SuperElementLinearExternal*>(discretization[0])->setK(mat);
	qSize = discretization[0]->getqSize();
  }
  void FlexibleBodyLinearExternal::readStiffnessMatrix(const string &stiffnessfilename) {
    fstream datafile(stiffnessfilename,ios::in);
    if (!datafile.is_open()) {
      stringstream error;
      error << "File " << stiffnessfilename << " containing stiffnessmatrix not found." << endl;
      throw runtime_error(error.str());
    }
    SqrMat KTemp;
    datafile >> KTemp;
    setStiffnessMatrix( KTemp );
    datafile.close();
  }

  //----------------------------------------------------------------------------------------

//  void FlexibleBodyLinearExternal::addFrame(const string &name, const Mat &J_, const Vec &r_) {
//	ContourPointData cp = addInterface(J_,r_);
//    Port *frame_ = new Port(name);
//    frame_->setID(cp.getNodeNumber()); // Stelle, an der die Jacobi steht
//    FlexibleBody::addFrame(frame_, cp );
//  }
//
//  void FlexibleBodyLinearExternal::addFrame(const string &name, const string &jacobifile) {
//    ContourPointData cp = addInterface(jacobifile);
//    Port *frame_ = new Port(name);
//    frame_->setID(cp.getNodeNumber()); // Stelle, an der die Jacobi steht
//    FlexibleBody::addFrame(frame_, cp );
//  }
//
//  void FlexibleBodyLinearExternal::addContour(Contour *contour_, const Mat &J_, const Vec &r_) {
//	ContourPointData cp = addInterface(J_,r_);
//	contourType.push_back(cp);
//    contour_->setID(cp.getNodeNumber()); // Stelle, an der die Jacobi steht
//    FlexibleBody::addContour(contour_, cp );
//    nContours++;
//  }
//  void FlexibleBodyLinearExternal::addContour(Contour *contour_, const string &jacobifile) {
//	ContourPointData cp = addInterface(jacobifile);
//	contourType.push_back(cp);
//    contour_->setID(cp.getNodeNumber()); // Stelle, an der die Jacobi steht
//    FlexibleBody::addContour(contour_, cp );
//    nContours++;
//  }
//
//  void FlexibleBodyLinearExternal::addContourInterpolation(ContourInterpolation *contour_) {
//    contour_->setID(contourType.size()); // Stelle, an der die Jacobi steht
//    ContourPointData cpData;
//    cpData.getContourParameterType()=ContourPointData::extInterpol;
//	contourType.push_back(cpData);
//    FlexibleBody::addContour(contour_,cpData,false); 
//  }


  ContourPointData FlexibleBodyLinearExternal::addInterface(const string &jacobifile) {

    fstream datafile(jacobifile,ios::in);
    if (!datafile.is_open()) {
      stringstream error;
      error << "File " << jacobifile << " containing Jacobimatrix not found." << endl;
      throw runtime_error(error.str());
    }

    //      msg(Debug) << "reading from " << jacobifile << endl;

    Mat JTemp;
    datafile >> JTemp;

    //      msg(Debug) << "Jacobian: " << JTemp << endl;

    char buffer[100]; // fmatvec-read method does not read end of line
    datafile.getline(buffer,100);

    Mat KrPTemp;
    datafile >> KrPTemp;


    return addInterface(JTemp,KrPTemp.col(0));
  }


//  void FlexibleBodyLinearExternal::plotParameters() {
//    parafile << "FlexibleBodyLinearExternal\n---------------------------\n"  << endl;
//    parafile << "# MassMatrix\n" << M << endl;
//    parafile << "\n# StiffnessMatrix\n" << -discretization[0]->getJacobianForImplicitIntegrationRegardingPosition() << endl;
//
////    parafile << "\n# JT\n"      << JT   << endl;
////    parafile << "\n# JR\n"      << JR   << endl;
//
//    if(frame.size()>0) parafile << "\nframes:" <<endl;
//    for(unsigned int i=0; i<frame.size(); i++) { 
//	  ContourPointData cp; cp.getNodeNumber()=frame[i]->getID();
//      parafile << "# J: (frame:  name= "<< frame[i]->getName()<<",  ID= "<<frame[i]->getID()<<") \n"<<discretization[0]->computeJacobianOfMinimalRepresentationRegardingPhysics(qElement[0],cp)<<endl;
//    }
//
//	if(contour.size()>0) parafile << "\ncontours:" <<endl;
//	for(unsigned int i=0; i<contour.size(); i++) {
//	  if(contourType[i].getContourParameterType()!=ContourPointData::extInterpol) {
//		parafile << "# J: (contour:  name= "<< contour[i]->getName()<<",  ID= "<<contour[i]->getID()<<")"<< endl;
//		parafile << discretization[0]->computeJacobianOfMinimalRepresentationRegardingPhysics(qElement[0],contourType[i])<<endl;
//	  } else {
//		parafile << "# extern -> contour:  name= "<< contour[i]->getName()<<",  ID= "<<contour[i]->getID() << endl;
//	  }
//	}
//  }

  void FlexibleBodyLinearExternal::updateJh_internal(double t) {
//    Mat Jh = mbs->getJh()(Iu,RangeV(0,mbs->getzSize()-1));
//    Jh(RangeV(0,uSize[0]-1),RangeV(    0,qSize         -1)) << -K;
//    Jh(RangeV(0,uSize[0]-1),RangeV(qSize,qSize+uSize[0]-1)) << -D;
  }
}
