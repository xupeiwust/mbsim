/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2024 Martin Förg

  This library is free software; you can redistribute it and/or 
  modify it under the terms of the GNU Lesser General Public 
  License as published by the Free Software Foundation; either 
  version 2.1 of the License, or (at your option) any later version. 
   
  This library is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
  Lesser General Public License for more details. 
   
  You should have received a copy of the GNU Lesser General Public 
  License along with this library; if not, write to the Free Software 
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <config.h>
#include "observer_property_dialog.h"
#include "variable_widgets.h"
#include "ombv_widgets.h"
#include "frame.h"
#include "constraint.h"
#include "rigid_body.h"
#include "signal_.h"

using namespace std;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimGUI {

  class FrameChaserObserverWidgetFactory : public WidgetFactory {
    public:
      FrameChaserObserverWidgetFactory(Element* observer_, bool rot_, const QChar &dir, QWidget *parent_);
      Widget* createWidget(int i=0) override;
      QString getName(int i=0) const override { return name[i]; }
      MBXMLUtils::FQN getXMLName(int i=0) const override { return xmlName[i]; }
      int getSize() const override { return name.size(); }
    protected:
      Element *observer;
      bool rot;
      vector<QString> name;
      vector<MBXMLUtils::FQN> xmlName;
      QWidget *parent;
  };

  FrameChaserObserverWidgetFactory::FrameChaserObserverWidgetFactory(Element* observer_, bool rot_, const QChar &dir, QWidget *parent_) : observer(observer_), rot(rot_), name(2), xmlName(2), parent(parent_) {
    QString midName = rot?" rotation ":" ";
    QString xmlMidName = rot?"Rotation":"";
    name[0] = "Constant" + midName + dir;
    name[1] = "Signal" + midName + dir;
    xmlName[0] = MBSIMCONTROL%("constant" + xmlMidName + dir.toUpper()).toStdString();
    xmlName[1] = MBSIMCONTROL%("signal" + xmlMidName + dir.toUpper()).toStdString();
  }

  Widget* FrameChaserObserverWidgetFactory::createWidget(int i) {
    if(i==0) {
      if(rot)
	return new ChoiceWidget(new ScalarWidgetFactory("0",vector<QStringList>(2,angleUnits())),QBoxLayout::RightToLeft,5);
      else
	return new ChoiceWidget(new ScalarWidgetFactory("0",vector<QStringList>(2,lengthUnits()),vector<int>(2,4)),QBoxLayout::RightToLeft,5);
    }
    if(i==1)
      return new ElementOfReferenceWidget<Signal>(observer,nullptr,parent);
    return nullptr;
  }

  ObserverPropertyDialog::ObserverPropertyDialog(Element *observer) : ElementPropertyDialog(observer) {
  }

  MechanicalLinkObserverPropertyDialog::MechanicalLinkObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    link = new ExtWidget("Mechanical link",new ElementOfReferenceWidget<Link>(observer,nullptr,this),false,false,MBSIM%"mechanicalLink");
    addToTab("General", link);

    outputFrame = new ExtWidget("Output frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIM%"outputFrame");
    addToTab("General", outputFrame);

    forceArrow = new ExtWidget("Enable openMBV force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVForce");
    addToTab("Visualization",forceArrow);

    momentArrow = new ExtWidget("Enable openMBV moment",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVMoment");
    addToTab("Visualization",momentArrow);
  }

  DOMElement* MechanicalLinkObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    link->initializeUsingXML(item->getXMLElement());
    outputFrame->initializeUsingXML(item->getXMLElement());
    forceArrow->initializeUsingXML(item->getXMLElement());
    momentArrow->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* MechanicalLinkObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    link->writeXMLFile(item->getXMLElement(),ref);
    outputFrame->writeXMLFile(item->getXMLElement(),ref);
    forceArrow->writeXMLFile(item->getXMLElement(),ref);
    momentArrow->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  MechanicalConstraintObserverPropertyDialog::MechanicalConstraintObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    constraint = new ExtWidget("Mechanical constraint",new ElementOfReferenceWidget<Constraint>(observer,nullptr,this),false,false,MBSIM%"mechanicalConstraint");
    addToTab("General", constraint);

    outputFrame = new ExtWidget("Output frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIM%"outputFrame");
    addToTab("General", outputFrame);

    forceArrow = new ExtWidget("Enable openMBV force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVForce");
    addToTab("Visualization",forceArrow);

    momentArrow = new ExtWidget("Enable openMBV moment",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVMoment");
    addToTab("Visualization",momentArrow);
  }

  DOMElement* MechanicalConstraintObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    constraint->initializeUsingXML(item->getXMLElement());
    outputFrame->initializeUsingXML(item->getXMLElement());
    forceArrow->initializeUsingXML(item->getXMLElement());
    momentArrow->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* MechanicalConstraintObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    constraint->writeXMLFile(item->getXMLElement(),ref);
    outputFrame->writeXMLFile(item->getXMLElement(),ref);
    forceArrow->writeXMLFile(item->getXMLElement(),ref);
    momentArrow->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  ContactObserverPropertyDialog::ContactObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    link = new ExtWidget("Mechanical link",new ElementOfReferenceWidget<Link>(observer,nullptr,this),false,false,MBSIM%"contact");
    addToTab("General", link);

    outputFrame = new ExtWidget("Output frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIM%"outputFrame");
    addToTab("General", outputFrame);

    forceArrow = new ExtWidget("Enable openMBV force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVForce");
    addToTab("Visualization",forceArrow);

    momentArrow = new ExtWidget("Enable openMBV force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVMoment");
    addToTab("Visualization",momentArrow);

    contactPoints = new ExtWidget("Enable openMBV contact points",new FrameMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVContactPoints");
    addToTab("Visualization",contactPoints);

    normalForceArrow = new ExtWidget("Enable openMBV normal force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVNormalForce");
    addToTab("Visualization",normalForceArrow);

    frictionArrow = new ExtWidget("Enable openMBV tangential force",new FrictionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVTangentialForce");
    addToTab("Visualization",frictionArrow);
  }

  DOMElement* ContactObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    link->initializeUsingXML(item->getXMLElement());
    outputFrame->initializeUsingXML(item->getXMLElement());
    forceArrow->initializeUsingXML(item->getXMLElement());
    momentArrow->initializeUsingXML(item->getXMLElement());
    contactPoints->initializeUsingXML(item->getXMLElement());
    normalForceArrow->initializeUsingXML(item->getXMLElement());
    frictionArrow->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* ContactObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    link->writeXMLFile(item->getXMLElement(),ref);
    outputFrame->writeXMLFile(item->getXMLElement(),ref);
    forceArrow->writeXMLFile(item->getXMLElement(),ref);
    momentArrow->writeXMLFile(item->getXMLElement(),ref);
    contactPoints->writeXMLFile(item->getXMLElement(),ref);
    normalForceArrow->writeXMLFile(item->getXMLElement(),ref);
    frictionArrow->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  TyreContactObserverPropertyDialog::TyreContactObserverPropertyDialog(Element *observer) : MechanicalLinkObserverPropertyDialog(observer) {
    contactPoints = new ExtWidget("Enable openMBV contact points",new FrameMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVContactPoints");
    addToTab("Visualization",contactPoints);

    normalForceArrow = new ExtWidget("Enable openMBV normal force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVNormalForce");
    addToTab("Visualization",normalForceArrow);

    longitudinalForceArrow = new ExtWidget("Enable openMBV longitudinal force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVLongitudinalForce");
    addToTab("Visualization",longitudinalForceArrow);

    lateralForceArrow = new ExtWidget("Enable openMBV lateral force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVLateralForce");
    addToTab("Visualization",lateralForceArrow);

    overturningMomentArrow = new ExtWidget("Enable openMBV overturning moment",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVOverturningMoment");
    addToTab("Visualization",overturningMomentArrow);

   rollingResistanceMomentArrow = new ExtWidget("Enable openMBV rolling resistance moment",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVRollingResistanceMoment");
    addToTab("Visualization",rollingResistanceMomentArrow);

    aligningMomentArrow = new ExtWidget("Enable openMBV aligning moment",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVAligningMoment");
    addToTab("Visualization",aligningMomentArrow);
  }

  DOMElement* TyreContactObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    MechanicalLinkObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    contactPoints->initializeUsingXML(item->getXMLElement());
    normalForceArrow->initializeUsingXML(item->getXMLElement());
    longitudinalForceArrow->initializeUsingXML(item->getXMLElement());
    lateralForceArrow->initializeUsingXML(item->getXMLElement());
    overturningMomentArrow->initializeUsingXML(item->getXMLElement());
    rollingResistanceMomentArrow->initializeUsingXML(item->getXMLElement());
    aligningMomentArrow->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* TyreContactObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    MechanicalLinkObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    contactPoints->writeXMLFile(item->getXMLElement(),ref);
    normalForceArrow->writeXMLFile(item->getXMLElement(),ref);
    longitudinalForceArrow->writeXMLFile(item->getXMLElement(),ref);
    lateralForceArrow->writeXMLFile(item->getXMLElement(),ref);
    overturningMomentArrow->writeXMLFile(item->getXMLElement(),ref);
    rollingResistanceMomentArrow->writeXMLFile(item->getXMLElement(),ref);
    aligningMomentArrow->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  FrameObserverPropertyDialog::FrameObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    frame = new ExtWidget("Frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),false,false,MBSIM%"frame");
    addToTab("General", frame);

    outputFrame = new ExtWidget("Output frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIM%"outputFrame");
    addToTab("General", outputFrame);

    position = new ExtWidget("Enable openMBV position",new ArrowMBSOMBVWidget(getGreenColor()),true,false,MBSIM%"enableOpenMBVPosition");
    addToTab("Visualization",position);

    velocity = new ExtWidget("Enable openMBV velocity",new ArrowMBSOMBVWidget(getGreenColor()),true,false,MBSIM%"enableOpenMBVVelocity");
    addToTab("Visualization",velocity);

    angularVelocity = new ExtWidget("Enable openMBV angular velocity",new ArrowMBSOMBVWidget(getGreenColor()),true,false,MBSIM%"enableOpenMBVAngularVelocity");
    addToTab("Visualization",angularVelocity);

    acceleration = new ExtWidget("Enable openMBV acceleration",new ArrowMBSOMBVWidget(getGreenColor()),true,false,MBSIM%"enableOpenMBVAcceleration");
    addToTab("Visualization",acceleration);

    angularAcceleration = new ExtWidget("Enable openMBV angular acceleration",new ArrowMBSOMBVWidget(getGreenColor()),true,false,MBSIM%"enableOpenMBVAngularAcceleration");
    addToTab("Visualization",angularAcceleration);
  }

  DOMElement* FrameObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    frame->initializeUsingXML(item->getXMLElement());
    outputFrame->initializeUsingXML(item->getXMLElement());
    position->initializeUsingXML(item->getXMLElement());
    velocity->initializeUsingXML(item->getXMLElement());
    angularVelocity->initializeUsingXML(item->getXMLElement());
    acceleration->initializeUsingXML(item->getXMLElement());
    angularAcceleration->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* FrameObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    frame->writeXMLFile(item->getXMLElement(),ref);
    outputFrame->writeXMLFile(item->getXMLElement(),ref);
    position->writeXMLFile(item->getXMLElement(),ref);
    velocity->writeXMLFile(item->getXMLElement(),ref);
    angularVelocity->writeXMLFile(item->getXMLElement(),ref);
    acceleration->writeXMLFile(item->getXMLElement(),ref);
    angularAcceleration->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  RigidBodyObserverPropertyDialog::RigidBodyObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    body = new ExtWidget("Rigid body",new ElementOfReferenceWidget<RigidBody>(observer,nullptr,this),false,false,MBSIM%"rigidBody");
    addToTab("General", body);

    frameOfReference = new ExtWidget("Frame of reference",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIM%"frameOfReference");
    addToTab("General", frameOfReference);

    outputFrame = new ExtWidget("Output frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIM%"outputFrame");
    addToTab("General", outputFrame);

    weight = new ExtWidget("Enable openMBV weight",new ArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVWeight");
    addToTab("Visualization",weight);

    jointForce = new ExtWidget("Enable openMBV joint force",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVJointForce");
    addToTab("Visualization",jointForce);

    jointMoment = new ExtWidget("Enable openMBV joint moment",new InteractionArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVJointMoment");
    addToTab("Visualization",jointMoment);

    axisOfRotation = new ExtWidget("Enable openMBV axis of rotation",new ArrowMBSOMBVWidget(getBlueColor()),true,false,MBSIM%"enableOpenMBVAxisOfRotation");
    addToTab("Visualization",axisOfRotation);

    momentum = new ExtWidget("Enable openMBV momentum",new ArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVMomentum");
    addToTab("Visualization",momentum);

    angularMomentum = new ExtWidget("Enable openMBV angular momentum",new ArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVAngularMomentum");
    addToTab("Visualization",angularMomentum);

    derivativeOfMomentum = new ExtWidget("Enable openMBV derivative of momentum",new ArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVDerivativeOfMomentum");
    addToTab("Visualization",derivativeOfMomentum);

    derivativeOfAngularMomentum = new ExtWidget("Enable openMBV derivative of angular momentum",new ArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBVDerivativeOfAngularMomentum");
    addToTab("Visualization",derivativeOfAngularMomentum);
  }

  DOMElement* RigidBodyObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    body->initializeUsingXML(item->getXMLElement());
    frameOfReference->initializeUsingXML(item->getXMLElement());
    outputFrame->initializeUsingXML(item->getXMLElement());
    weight->initializeUsingXML(item->getXMLElement());
    jointForce->initializeUsingXML(item->getXMLElement());
    jointMoment->initializeUsingXML(item->getXMLElement());
    axisOfRotation->initializeUsingXML(item->getXMLElement());
    momentum->initializeUsingXML(item->getXMLElement());
    angularMomentum->initializeUsingXML(item->getXMLElement());
    derivativeOfMomentum->initializeUsingXML(item->getXMLElement());
    derivativeOfAngularMomentum->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* RigidBodyObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    body->writeXMLFile(item->getXMLElement(),ref);
    frameOfReference->writeXMLFile(item->getXMLElement(),ref);
    outputFrame->writeXMLFile(item->getXMLElement(),ref);
    weight->writeXMLFile(item->getXMLElement(),ref);
    jointForce->writeXMLFile(item->getXMLElement(),ref);
    jointMoment->writeXMLFile(item->getXMLElement(),ref);
    axisOfRotation->writeXMLFile(item->getXMLElement(),ref);
    momentum->writeXMLFile(item->getXMLElement(),ref);
    angularMomentum->writeXMLFile(item->getXMLElement(),ref);
    derivativeOfMomentum->writeXMLFile(item->getXMLElement(),ref);
    derivativeOfAngularMomentum->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  InverseKinematicsConstraintObserverPropertyDialog::InverseKinematicsConstraintObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    constraint = new ExtWidget("Inverse kinematics constraint",new ElementOfReferenceWidget<InverseKinematicsConstraint>(observer,nullptr,this),false,false,MBSIM%"inverseKinematicsConstraint");
    addToTab("General", constraint);

    outputFrame = new ExtWidget("Output frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIM%"outputFrame");
    addToTab("General", outputFrame);

    ombv = new ExtWidget("Enable openMBV",new ArrowMBSOMBVWidget,true,false,MBSIM%"enableOpenMBV");
    addToTab("Visualization",ombv);
  }

  DOMElement* InverseKinematicsConstraintObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    constraint->initializeUsingXML(item->getXMLElement());
    outputFrame->initializeUsingXML(item->getXMLElement());
    ombv->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* InverseKinematicsConstraintObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    constraint->writeXMLFile(item->getXMLElement(),ref);
    outputFrame->writeXMLFile(item->getXMLElement(),ref);
    ombv->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  SignalObserverPropertyDialog::SignalObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    signal = new ExtWidget("Signal",new ElementOfReferenceWidget<Signal>(observer,nullptr,this),false,false,MBSIMCONTROL%"signal");
    addToTab("General", signal);

    position = new ExtWidget("Position",new ElementOfReferenceWidget<Signal>(observer,nullptr,this),true,false,MBSIMCONTROL%"position");
    addToTab("General", position);

    enableOmbv = new ExtWidget("Enable openMBV",new ArrowMBSOMBVWidget,true,false,MBSIMCONTROL%"enableOpenMBV");
    addToTab("Visualization", enableOmbv);

    ombvIVSA = new ExtWidget("Open Inventor screen annotation",new ChoiceWidget(new OMBVIvScreenAnnotationWidgetFactory,QBoxLayout::TopToBottom,0),true,false,MBSIMCONTROL%"openMBVIvScreenAnnotation");
    addToTab("Visualization", ombvIVSA);
  }

  DOMElement* SignalObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    signal->initializeUsingXML(item->getXMLElement());
    position->initializeUsingXML(item->getXMLElement());
    enableOmbv->initializeUsingXML(item->getXMLElement());
    ombvIVSA->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* SignalObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    signal->writeXMLFile(item->getXMLElement(),ref);
    position->writeXMLFile(item->getXMLElement(),ref);
    enableOmbv->writeXMLFile(item->getXMLElement(),ref);
    ombvIVSA->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  FrameChaserObserverPropertyDialog::FrameChaserObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    frame = new ExtWidget("Frame",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),false,false,MBSIMCONTROL%"frame");
    addToTab("General", frame);

    transX = new ExtWidget("Translation along x axis",new ChoiceWidget(new FrameChaserObserverWidgetFactory(observer,false,'x',this),QBoxLayout::TopToBottom,3),true,false,"");
    addToTab("General",transX);

    transY = new ExtWidget("Translation along y axis",new ChoiceWidget(new FrameChaserObserverWidgetFactory(observer,false,'y',this),QBoxLayout::TopToBottom,3),true,false,"");
    addToTab("General",transY);

    transZ = new ExtWidget("Translation along z axis",new ChoiceWidget(new FrameChaserObserverWidgetFactory(observer,false,'z',this),QBoxLayout::TopToBottom,3),true,false,"");
    addToTab("General",transZ);

    rotX = new ExtWidget("Rotation about x axis",new ChoiceWidget(new FrameChaserObserverWidgetFactory(observer,true,'x',this),QBoxLayout::TopToBottom,3),true,false,"");
    addToTab("General",rotX);

    rotY = new ExtWidget("Rotation about y axis",new ChoiceWidget(new FrameChaserObserverWidgetFactory(observer,true,'y',this),QBoxLayout::TopToBottom,3),true,false,"");
    addToTab("General",rotY);

    rotZ = new ExtWidget("Rotation about z axis",new ChoiceWidget(new FrameChaserObserverWidgetFactory(observer,true,'z',this),QBoxLayout::TopToBottom,3),true,false,"");
    addToTab("General",rotZ);

    enableOmbv = new ExtWidget("Enable openMBV",new FrameMBSOMBVWidget,true,false,MBSIMCONTROL%"enableOpenMBV");
    addToTab("Visualization",enableOmbv);
  }

  DOMElement* FrameChaserObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    frame->initializeUsingXML(item->getXMLElement());
    transX->initializeUsingXML(item->getXMLElement());
    transY->initializeUsingXML(item->getXMLElement());
    transZ->initializeUsingXML(item->getXMLElement());
    rotX->initializeUsingXML(item->getXMLElement());
    rotY->initializeUsingXML(item->getXMLElement());
    rotZ->initializeUsingXML(item->getXMLElement());
    enableOmbv->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* FrameChaserObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    frame->writeXMLFile(item->getXMLElement(),ref);
    transX->writeXMLFile(item->getXMLElement(),ref);
    transY->writeXMLFile(item->getXMLElement(),ref);
    transZ->writeXMLFile(item->getXMLElement(),ref);
    rotX->writeXMLFile(item->getXMLElement(),ref);
    rotY->writeXMLFile(item->getXMLElement(),ref);
    rotZ->writeXMLFile(item->getXMLElement(),ref);
    enableOmbv->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

  MotionObserverPropertyDialog::MotionObserverPropertyDialog(Element *observer) : ObserverPropertyDialog(observer) {

    addTab("Visualization",1);

    frame = new ExtWidget("Frame of reference",new ElementOfReferenceWidget<Frame>(observer,nullptr,this),true,false,MBSIMCONTROL%"frameOfReference");
    addToTab("General", frame);

    position = new ExtWidget("Position signal",new ElementOfReferenceWidget<Signal>(observer,nullptr,this),true,false,MBSIMCONTROL%"positionSignal");
    addToTab("General",position);

    orientation = new ExtWidget("Orientation signal",new ElementOfReferenceWidget<Signal>(observer,nullptr,this),true,false,MBSIMCONTROL%"orientationSignal");
    addToTab("General",orientation);

    ombv = new ExtWidget("OpenMBV body",new ChoiceWidget(new OMBVRigidBodyWidgetFactory,QBoxLayout::TopToBottom,0),true,true,MBSIMCONTROL%"openMBVRigidBody");
    addToTab("Visualization",ombv);
  }

  DOMElement* MotionObserverPropertyDialog::initializeUsingXML(DOMElement *parent) {
    ObserverPropertyDialog::initializeUsingXML(item->getXMLElement());
    frame->initializeUsingXML(item->getXMLElement());
    position->initializeUsingXML(item->getXMLElement());
    orientation->initializeUsingXML(item->getXMLElement());
    ombv->initializeUsingXML(item->getXMLElement());
    return parent;
  }

  DOMElement* MotionObserverPropertyDialog::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    ObserverPropertyDialog::writeXMLFile(item->getXMLElement(),ref);
    frame->writeXMLFile(item->getXMLElement(),ref);
    position->writeXMLFile(item->getXMLElement(),ref);
    orientation->writeXMLFile(item->getXMLElement(),ref);
    ombv->writeXMLFile(item->getXMLElement(),ref);
    return nullptr;
  }

}
