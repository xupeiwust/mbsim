#include "system.h"
#include "mbsimFlexibleBody/flexible_body/1s_21_rcm.h"
#include "mbsimFlexibleBody/flexible_body/1s_33_rcm.h"
#include <mbsimFlexibleBody/flexible_body/linear_external_ffr.h>
#include <mbsimFlexibleBody/contours/nc/1s_neutral_linear_external_FFR.h>
#include "mbsim/objects/rigid_body.h"
#include "mbsim/links/joint.h"
#include "mbsim/links/contact.h"
#include "mbsim/frames/fixed_relative_frame.h"
#include "mbsim/frames/fixed_contour_frame.h"
#include "mbsim/contours/point.h"
#include "mbsimFlexibleBody/contours/flexible_band.h"
#include "mbsim/constitutive_laws/constitutive_laws.h"
#include "mbsim/environment.h"
#include <mbsim/functions/kinematics/kinematics.h>

#include "mbsim/contact_kinematics/point_planarcontour.h"

#include <openmbvcppinterface/spineextrusion.h>
#include <openmbvcppinterface/polygonpoint.h>
#include <openmbvcppinterface/sphere.h>

using namespace MBSimFlexibleBody;
using namespace MBSim;
using namespace fmatvec;
using namespace std;

SlidingMass::SlidingMass(const string &projectName) :
    DynamicSystemSolver(projectName) {

  bool runRCM = false;
  bool runFFR = true;
//  bool runCosserat = true;

  double l0 = 1.5; // length
  double b0 = 0.05; // width
  double E = 5.e7; // E-Modul
  double A = b0 * b0; // cross-section area
  double I1 = 1. / 12. * b0 * b0 * b0 * b0; // moment inertia
  double rho = 9.2e2; // density
  int elements = 10; // number of finite elements

  double r = b0; // radius of ball
  double mass = 1.; // mass of ball
  SymMat3 Theta;
  Theta(0, 0) = 2. / 5. * mass * r * r;
  Theta(1, 1) = 2. / 5. * mass * r * r;
  Theta(2, 2) = 2. / 5. * mass * r * r;

  double u0 = 0.1;

  Vec grav(3);
  grav(0) = -9.81;
  getMBSimEnvironment()->setAccelerationOfGravity(grav);

  if (runFFR) {
    Vec3 ffrDispl;
    ffrDispl(0) = l0 / 2.;
    FixedRelativeFrame * ffrRef = new FixedRelativeFrame("FFRReference", ffrDispl);
    ffrRef->setFrameOfReference(getFrameI());
    addFrame(ffrRef);
    ffrRef->enableOpenMBV(1e-1);

    FlexibleBodyLinearExternalFFR *beam = new FlexibleBodyLinearExternalFFR("FFRBeam");

    beam->readFEMData("FFRBeam/", true);

    int nf = 20;
    beam->setNumberOfModes(nf);

    beam->setFrameOfReference(ffrRef);

    // set a initial angle for the body reference
    Vec q0FFR = Vec(nf + 6, INIT, 0.0);
    Vec u0Beam = Vec(nf + 6, INIT, 0.0);
    //  q0(0) = 0.2;

    beam->setq0(q0FFR);
    beam->setu0(u0Beam);

    beam->getFloatingFrameOfReference()->enableOpenMBV(1e-1);

    // Fix the beam at its FFR
    Joint * fix = new Joint("FFRFix");
    fix->connect(ffrRef, beam->getFloatingFrameOfReference());
    fix->setForceDirection(Mat3x3(EYE));
    fix->setMomentDirection(Mat3x3(EYE));
    fix->setForceLaw(new BilateralConstraint);
    fix->setMomentLaw(new BilateralConstraint);
    addLink(fix);

    this->addObject(beam);

    // add neutral contour to the rod
    Contour1sNeutralLinearExternalFFR* ncc = new Contour1sNeutralLinearExternalFFR("neutralFibre");
    beam->addContour(ncc);
    ncc->readTransNodes("FFRBeam/NeutralPhase.txt");
    ncc->setFrameOfReference(beam->getFrameOfReference());

    vector<double> nodes(11);
    for (int i = 0; i <= 10; i++)
      nodes[i] = double(i)/10.;
    ncc->setEtaNodes(nodes);

    {
      std::shared_ptr<OpenMBV::SpineExtrusion> cuboid = OpenMBV::ObjectFactory::create<OpenMBV::SpineExtrusion>();
      cuboid->setNumberOfSpinePoints(elements * 4 + 1); // resolution of visualisation
      cuboid->setDiffuseColor(244./360., 1, 200./255.); // color in (minimalColorValue, maximalColorValue)
      cuboid->setScaleFactor(1.); // orthotropic scaling of cross section
      shared_ptr<vector<shared_ptr<OpenMBV::PolygonPoint> > > rectangle = make_shared<vector<shared_ptr<OpenMBV::PolygonPoint> > >(); // clockwise ordering, no doubling for closure
      shared_ptr<OpenMBV::PolygonPoint>  corner1 = OpenMBV::PolygonPoint::create(b0 * 0.5, b0 * 0.5, 1);
      rectangle->push_back(corner1);
      shared_ptr<OpenMBV::PolygonPoint>  corner2 = OpenMBV::PolygonPoint::create(b0 * 0.5, -b0 * 0.5, 1);
      rectangle->push_back(corner2);
      shared_ptr<OpenMBV::PolygonPoint>  corner3 = OpenMBV::PolygonPoint::create(-b0 * 0.5, -b0 * 0.5, 1);
      rectangle->push_back(corner3);
      shared_ptr<OpenMBV::PolygonPoint>  corner4 = OpenMBV::PolygonPoint::create(-b0 * 0.5, b0 * 0.5, 1);
      rectangle->push_back(corner4);

      cuboid->setContour(rectangle);
      ncc->setOpenMBVSpineExtrusion(cuboid);
    }

    Vec WrOS0B1(3, INIT, 0.);
    WrOS0B1(0) = l0 * 0.9 - l0 / 2.;
    FixedRelativeFrame * ball1Ref = new FixedRelativeFrame("FFR_B", WrOS0B1);
    ball1Ref->setFrameOfReference(ffrRef);
    addFrame(ball1Ref);

    RigidBody *ball1 = new RigidBody("FFR_Ball");
    addObject(ball1);
    ball1->setFrameOfReference(ball1Ref);
    ball1->setMass(mass);
    ball1->setInertiaTensor(Theta);

    ball1->setTranslation(new TranslationAlongAxesXYZ<VecV>());
    Vec u0Ball1(3, INIT, 0.);
    u0Ball1(1) = u0;
    ball1->setGeneralizedInitialVelocity(u0Ball1);

    MBSim::Point * ballContour1 = new MBSim::Point("FFR_Mass");
    ball1->addContour(ballContour1);
    ballContour1->setFrameOfReference(ball1->getFrameC());

    std::shared_ptr<OpenMBV::Sphere> sphere1 = OpenMBV::ObjectFactory::create<OpenMBV::Sphere>();
    sphere1->setRadius(r);
    sphere1->setDiffuseColor(0.5, 0.5, 1);
    sphere1->setTransparency(0.7);
    sphere1->setEnable(true);
    ball1->setOpenMBVRigidBody(sphere1);

    Contact *contact1 = new Contact("FFR_Contact");
    contact1->setNormalForceLaw(new BilateralConstraint);
    contact1->setNormalImpactLaw(new BilateralImpact);
    contact1->connect(ballContour1, ncc);
    contact1->setContactKinematics(new ContactKinematicsPointPlanarContour);
    contact1->setInitialGlobalSearch(true);
    this->addLink(contact1);

  }

  if (runRCM) {

    Vec3 rcmDispl;
    rcmDispl(2) = 5 * b0;
    FixedRelativeFrame * rcmRef = new FixedRelativeFrame("RCMReference", rcmDispl);
    rcmRef->setFrameOfReference(getFrameI());
    addFrame(rcmRef);

    FlexibleBody1s33RCM *rod = new FlexibleBody1s33RCM("RCM_Beam", true);
    rod->setLength(l0);
    double nu = 0.3;
    double G = E / (2 + 2*nu);
    rod->setEGModuls(E,G);
    rod->setCrossSectionalArea(A);
    rod->setMomentsInertia(I1, I1, I1);
    rod->setDensity(rho);
    rod->setFrameOfReference(rcmRef);
    rod->setNumberElements(elements);
    Vec q0 = Vec(10 * elements + 6, INIT, 0.);
    for (int i = 1; i <= elements; i++)
      q0(10  * i) = l0 * i / elements;
    rod->setq0(q0);
    this->addObject(rod);

    std::shared_ptr<OpenMBV::SpineExtrusion> cuboid = OpenMBV::ObjectFactory::create<OpenMBV::SpineExtrusion>();
    cuboid->setNumberOfSpinePoints(elements * 4 + 1); // resolution of visualisation
    cuboid->setDiffuseColor(56./360., 132./255., 166./255.); // color in (minimalColorValue, maximalColorValue)
    cuboid->setScaleFactor(1.); // orthotropic scaling of cross section
    shared_ptr<vector<shared_ptr<OpenMBV::PolygonPoint> > > rectangle = make_shared<vector<shared_ptr<OpenMBV::PolygonPoint> > >(); // clockwise ordering, no doubling for closure
    shared_ptr<OpenMBV::PolygonPoint>  corner1 = OpenMBV::PolygonPoint::create(b0 * 0.5, b0 * 0.5, 1);
    rectangle->push_back(corner1);
    shared_ptr<OpenMBV::PolygonPoint>  corner2 = OpenMBV::PolygonPoint::create(b0 * 0.5, -b0 * 0.5, 1);
    rectangle->push_back(corner2);
    shared_ptr<OpenMBV::PolygonPoint>  corner3 = OpenMBV::PolygonPoint::create(-b0 * 0.5, -b0 * 0.5, 1);
    rectangle->push_back(corner3);
    shared_ptr<OpenMBV::PolygonPoint>  corner4 = OpenMBV::PolygonPoint::create(-b0 * 0.5, b0 * 0.5, 1);
    rectangle->push_back(corner4);

    cuboid->setContour(rectangle);
    rod->setOpenMBVSpineExtrusion(cuboid);

    Vec WrOS0B(3, INIT, 0.);
    WrOS0B(0) = l0 * 0.9;
    FixedRelativeFrame * ball2Ref = new FixedRelativeFrame("RC_B", WrOS0B);
    ball2Ref->setFrameOfReference(rcmRef);
    addFrame(ball2Ref);

    RigidBody *ball2 = new RigidBody("RCM_Ball");
    addObject(ball2);
    ball2->setFrameOfReference(ball2Ref);
    ball2->setMass(mass);
    ball2->setInertiaTensor(Theta);
    ball2->setTranslation(new TranslationAlongAxesXYZ<VecV>());
    Vec u0Ball(3, INIT, 0.);
    u0Ball(1) = u0;
    ball2->setGeneralizedInitialVelocity(u0Ball);

    MBSim::Point * ballContour = new MBSim::Point("RCM_Mass");
    ball2->addContour(ballContour);
    ballContour->setFrameOfReference(ball2->getFrameC());

    std::shared_ptr<OpenMBV::Sphere> sphere = OpenMBV::ObjectFactory::create<OpenMBV::Sphere>();
    sphere->setRadius(r);
    sphere->setDiffuseColor(0.5, 0.5, 1);
    sphere->setTransparency(0.7);
    sphere->setEnable(true);
    ball2->setOpenMBVRigidBody(sphere);

    Contact *contact = new Contact("RCM_Contact");
    contact->setNormalForceLaw(new BilateralConstraint);
    contact->setNormalImpactLaw(new BilateralImpact);
    contact->connect(ballContour, rod->getContour("NeutralFibre"));
    this->addLink(contact);

    rod->addFrame(new Frame1s("RJ",0));

    FixedRelativeFrame * fixFrameRCM = new FixedRelativeFrame("refFrameRCMFix");
    Vec3 relRCMFixPos;
    relRCMFixPos(0) = l0/2.;
    fixFrameRCM->setRelativePosition(relRCMFixPos);
    fixFrameRCM->setFrameOfReference(rcmRef);
    addFrame(fixFrameRCM);

    Joint *joint = new Joint("RCMFix");
    joint->connect(fixFrameRCM, rod->getFrame("RJ"));
    joint->setForceDirection(Mat("[1,0; 0,1; 0,0]"));
    joint->setForceLaw(new BilateralConstraint);
    joint->setMomentDirection("[0; 0; 1]");
    joint->setMomentLaw(new BilateralConstraint);
    this->addLink(joint);
  }

  setPlotFeatureRecursive(generalizedPosition, true);
  setPlotFeatureRecursive(generalizedVelocity, true);
  setPlotFeatureRecursive(generalizedRelativePosition, true);
  setPlotFeatureRecursive(generalizedRelativeVelocity, true);
  setPlotFeatureRecursive(generalizedForce, true);

//  if (runCosserat) {
//    Vec3 displ;
//    displ(2) = -5 * b0;
//    FixedRelativeFrame * beamRef = new FixedRelativeFrame("CosseratReference", ffrDispl);
//    beamRef->setFrameOfReference(getFrameI());
//    addFrame(beamRef);
//
//    FlexibleBody1s21Cosserat *beam = new FlexibleBody1s21Cosserat("CosseratBeam");
//
//    beam->setLength(l0);
//    beam->setEModul(E);
//    beam->setCrossSectionalArea(A);
//    beam->setMomentInertia(I1);
//    beam->setDensity(rho);
//    beam->setFrameOfReference(rcmRef);
//    beam->setNumberElements(elements);
//
//    beam->setFrameOfReference(beamRef);
//
//    // set a initial angle for the body reference
//    Vec q0FFR = Vec(elements * 3, INIT, 0.0);
//    Vec u0Beam = Vec(elements * 3, INIT, 0.0);
//    //  q0(0) = 0.2;
//
//    beam->setq0(q0FFR);
//    beam->setu0(u0Beam);
//
//    // Fix the beam at its FFR
//    Joint * fix = new Joint("Fix");
//    fix->connect(getFrameI(), beam->getFloatingFrameOfReference());
//    fix->setForceDirection(Mat3x3(EYE));
//    fix->setMomentDirection(Mat3x3(EYE));
//    fix->setForceLaw(new BilateralConstraint);
//    fix->setImpactForceLaw(new BilateralImpact);
//    fix->setMomentLaw(new BilateralConstraint);
//    fix->setImpactMomentLaw(new BilateralImpact);
//    addLink(fix);
//
//    this->addObject(beam);
//
//    // add neutral contour to the rod
//    Contour1sNeutralLinearExternalFFR* ncc = new Contour1sNeutralLinearExternalFFR("neutralFibre");
//    beam->addContour(ncc);
//    ncc->readTransNodes("simpleBeam_coordinatesInCenter/MBSimData/NeutralPhase.txt");
//    ncc->setFrameOfReference(beam->getFrameOfReference());
//
//    ncc->setAlphaStart(0);
//    ncc->setAlphaEnd(1);
//
//    beam->enableFramePlot(1e-3, ncc->getTransNodes());
//
//    {
//      std::shared_ptr<OpenMBV::SpineExtrusion> cuboid = OpenMBV::ObjectFactory::create<OpenMBV::SpineExtrusion>();
//      cuboid->setNumberOfSpinePoints(nf * 4 + 1); // resolution of visualisation
//      cuboid->setDiffuseColor(0.5, 0.5, 0); // color in (minimalColorValue, maximalColorValue)
//      cuboid->setScaleFactor(1.); // orthotropic scaling of cross section
//      shared_ptr<vector<shared_ptr<OpenMBV::PolygonPoint> > > rectangle = make_shared<vector<shared_ptr<OpenMBV::PolygonPoint> > >(); // clockwise ordering, no doubling for closure
//      shared_ptr<OpenMBV::PolygonPoint>  corner1 = OpenMBV::PolygonPoint::create(b0 * 0.5, b0 * 0.5, 1);
//      rectangle->push_back(corner1);
//      shared_ptr<OpenMBV::PolygonPoint>  corner2 = OpenMBV::PolygonPoint::create(b0 * 0.5, -b0 * 0.5, 1);
//      rectangle->push_back(corner2);
//      shared_ptr<OpenMBV::PolygonPoint>  corner3 = OpenMBV::PolygonPoint::create(-b0 * 0.5, -b0 * 0.5, 1);
//      rectangle->push_back(corner3);
//      shared_ptr<OpenMBV::PolygonPoint>  corner4 = OpenMBV::PolygonPoint::create(-b0 * 0.5, b0 * 0.5, 1);
//      rectangle->push_back(corner4);
//
//      cuboid->setContour(rectangle);
//      ncc->setOpenMBVSpineExtrusion(cuboid);
//    }
//
//    Vec WrOS0B1(3, INIT, 0.);
//    WrOS0B1(0) = l0 * 0.9 - l0 / 2.;
//    FixedRelativeFrame * ball1Ref = new FixedRelativeFrame("FFR_B", WrOS0B1);
//    ball1Ref->setFrameOfReference(ffrRef);
//    addFrame(ball1Ref);
//
//    RigidBody *ball1 = new RigidBody("FFR_Ball");
//    addObject(ball1);
//    ball1->setFrameOfReference(ball1Ref);
//    ball1->setMass(mass);
//    ball1->setInertiaTensor(Theta);
//
//    ball1->setTranslation(new LinearTranslation(Mat3x3(EYE)));
//    Vec u0Ball1(3, INIT, 0.);
//    u0Ball1(1) = u0;
//    ball1->setGeneralizedInitialVelocity(u0Ball1);
//
//    MBSim::Point * ballContour1 = new MBSim::Point("FFR_Mass");
//    ball1->addContour(ballContour1);
//    ballContour1->setFrameOfReference(ball1->getFrameC());
//
//    std::shared_ptr<OpenMBV::Sphere> sphere1 = OpenMBV::ObjectFactory::create<OpenMBV::Sphere>();
//    sphere1->setRadius(r);
//    sphere1->setDiffuseColor(0.5, 0.5, 1);
//    sphere1->setTransparency(0.7);
//    sphere1->setEnable(true);
//    ball1->setOpenMBVRigidBody(sphere1);
//
//    Contact *contact1 = new Contact("FFR_Contact");
//    contact1->setContactForceLaw(new BilateralConstraint);
//    contact1->setContactImpactLaw(new BilateralImpact);
//    contact1->connect(ballContour1, ncc);
//    this->addLink(contact1);
//
//  }
}

