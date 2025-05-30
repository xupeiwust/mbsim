/*
 * neutral_contour_1s_cosserat.h
 *
 *  Created on: 25.10.2013
 *      Author: zwang
 */

#ifndef CONTOUR_1S_NEUTRAL_COSSERAT_H_
#define CONTOUR_1S_NEUTRAL_COSSERAT_H_

#include <mbsimFlexibleBody/flexible_body/1s_cosserat.h>
#include "mbsimFlexibleBody/pointer.h"
#include <mbsimFlexibleBody/contours/contour_1s_neutral_factory.h>
#include "ncc/nurbs_velocity_1s.h"
#include "ncc/nurbs_position_1s.h"
#include "ncc/nurbs_angle_1s.h"
#include "ncc/nurbs_dotangle_1s.h"

namespace MBSimFlexibleBody {
  
  class Contour1sNeutralCosserat : public MBSimFlexibleBody::Contour1sNeutralFactory {
    public:
      Contour1sNeutralCosserat(const std::string &name_);
       ~Contour1sNeutralCosserat() override;
       void init(InitStage stage, const MBSim::InitConfigSet &config) override;
      virtual NeutralNurbsPosition1s* createNeutralPosition();
      virtual NeutralNurbsVelocity1s* createNeutralVelocity();
      virtual NeutralNurbsAngle1s* createNeutralAngle();
      virtual NeutralNurbsDotangle1s* createNeutralDotangle();

       fmatvec::Vec3 evalPosition(const fmatvec::Vec2 &zeta) override;
       fmatvec::Vec3 evalWs(const fmatvec::Vec2 &zeta) override;
       fmatvec::Vec3 evalWt(const fmatvec::Vec2 &zeta) override;
       fmatvec::Vec3 evalWu(const fmatvec::Vec2 &zeta) override { return evalWs(zeta); }
       fmatvec::Vec3 evalWv(const fmatvec::Vec2 &zeta) override { return evalWt(zeta); }

      void updatePositions(MBSim::Frame *frame) override;
      void updateVelocities(MBSim::Frame *frame) override;
      void updateJacobians(MBSim::Frame *frame, int j=0) override;

       MBSim::ContactKinematics * findContactPairingWith(const std::type_info &type0, const std::type_info &type1) override;

      void setTransNodes(const fmatvec::VecInt & transNodes_);
      void setRotNodes(const fmatvec::VecInt & rotNodes_);
      void setNodeOffest(const double nodeOffset_);

      double getuMax() const {
        return uMax;
      }
      
      double getuMin() const {
        return uMin;
      }

      void setFrameOfReference(MBSim::Frame *frame) { R = frame; }

      void resetUpToDate() override;
      
    protected:
      /*!
       * \brief index of the translational Nodes
       */
      fmatvec::VecInt transNodes;

      /*!
       * \brief index of the rotational Nodes
       */
      fmatvec::VecInt rotNodes;

      /*!
       * \brief offset between translationa and rotational nodes
       */
      double nodeOffset;

      CardanPtr ANGLE;
      NeutralNurbsPosition1s* NP;
      NeutralNurbsVelocity1s* NV;
      NeutralNurbsAngle1s* NA;
      NeutralNurbsDotangle1s* NDA;

      MBSim::Frame *R;

  };

} /* namespace MBSimFlexibleBody */
#endif
