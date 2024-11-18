/* Copyright (C) 2004-2024 MBSim Development Team
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
 * Contact: martin.o.foerg@gmail.com
 */

#ifndef _INITIAL_CONDITION_H_
#define _INITIAL_CONDITION_H_

#include "mbsim/links/link.h"

namespace MBSim {

  using Index = int;
  class Object;

  class InitialCondition : public Link {
    protected:
      Object *object;
      std::string objectString;
      std::vector<Index> indices;
      fmatvec::VecV q0, u0;
      fmatvec::MatV JRel;
bool active{true};
    public:
      InitialCondition(const std::string &name="");
      ~InitialCondition() override { }

      void setObject(Object *object_) { object = object_; }
      void setGeneralizedInitialPosition(const fmatvec::Vec &q0_) { q0 <<= q0_; }
      void setGeneralizedInitialVelocity(const fmatvec::Vec &u0_) { u0 <<= u0_; }
      void setIndices(const std::vector<Index> &indices_) { indices = indices_; }

      void deactivate() { active = false; }
      bool isActive() const override { return active; }
      bool gActiveChanged() override { return false; }
      void calcSize() override;
      void calclaSize(int j) override;
      void calcgSize(int j) override;
      void calccorrSize(int j) override;
      void calcgdSize(int j) override;
      bool isSetValued() const override { return true; }
      bool isSingleValued() const override { return false; }

      void updateGeneralizedPositions() override;
      void updateGeneralizedVelocities() override;
 
      void updateg() override;
      void updategd() override;
      void updateW(int i=0) override;

      void updatehRef(fmatvec::Vec &hParent, int j=0) override { }
      void updaterRef(fmatvec::Vec &hParent, int j=0) override { }
      void updateWRef(fmatvec::Mat &WParent, int j=0) override;
      void updateVRef(fmatvec::Mat &WParent, int j=0) override { }

      void init(InitStage stage, const InitConfigSet &config) override;
      void initializeUsingXML(xercesc::DOMElement * element) override;
  };

}

#endif