/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2016 Martin Förg

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

#ifndef _FLEXIBLE_BODY_FFR__H_
#define _FLEXIBLE_BODY_FFR__H_

#include "body.h"

namespace MBSimGUI {

  class GenericFlexibleFfrBody : public Body {
    MBSIMGUI_OBJECTFACTORY_CLASS(GenericFlexibleFfrBody, Body, MBSIMFLEX%"GenericFlexibleFfrBody", "Flexible ffr body");
    public:
      GenericFlexibleFfrBody();
      xercesc::DOMElement* getXMLFrames() override { return frames; }
      xercesc::DOMElement* getXMLContours() override { return contours; }
      void removeXMLElements() override;
      xercesc::DOMElement* createXMLElement(xercesc::DOMNode *parent) override;
      xercesc::DOMElement* processIDAndHref(xercesc::DOMElement* element) override;
      void create() override;
      void clear() override;
      void setDedicatedFileItem(FileItemData *dedicatedFileItem) override;
      void setDedicatedParameterFileItem(FileItemData *dedicatedFileItem) override;
      QMenu* createFrameContextMenu() override { return new NodeFramesContextMenu(this); }
    protected:
      xercesc::DOMElement *frames, *contours;
  };

  class FlexibleFfrBody : public GenericFlexibleFfrBody {
    MBSIMGUI_OBJECTFACTORY_CLASS(FlexibleFfrBody, GenericFlexibleFfrBody, MBSIMFLEX%"FlexibleFfrBody", "Flexible ffr body");
    friend class FlexibleFfrBodyPropertyDialog;
    public:
      xercesc::DOMElement* processIDAndHref(xercesc::DOMElement* element) override;
      PropertyDialog* createPropertyDialog() override { return new FlexibleFfrBodyPropertyDialog(this); }
  };

  class ExternalFlexibleFfrBody : public GenericFlexibleFfrBody {
    MBSIMGUI_OBJECTFACTORY_CLASS(ExternalFlexibleFfrBody, GenericFlexibleFfrBody, MBSIMFLEX%"ExternalFlexibleFfrBody", "External flexible ffr body");
    friend class ExternalFlexibleFfrBodyPropertyDialog;
    public:
      xercesc::DOMElement* processIDAndHref(xercesc::DOMElement* element) override;
      PropertyDialog* createPropertyDialog() override { return new ExternalFlexibleFfrBodyPropertyDialog(this); }
  };

}

#endif
