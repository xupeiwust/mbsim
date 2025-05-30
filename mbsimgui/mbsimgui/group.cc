/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2012 Martin Förg

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
#include "element_view.h"
#include "group.h"
#include "frame.h"
#include "contour.h"
#include "object.h"
#include "link_.h"
#include "constraint.h"
#include "observer.h"
#include "objectfactory.h"
#include "utils.h"
#include "embed.h"
#include "mainwindow.h"
#include "mbxmlutilshelper/dom.h"

using namespace std;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimGUI {

  MBSIMGUI_REGOBJECTFACTORY(Group);

  Group::Group() : constraints(nullptr), observers(nullptr) {
    icon = Utils::QIconCached(QString::fromStdString((MainWindow::getInstallPath()/"share"/"mbsimgui"/"icons"/"group.svg").string()));
    InternalFrame *I = new InternalFrame("I",MBSIM%"enableOpenMBVFrameI",MBSIM%"plotFeatureFrameI");
    addFrame(I);
  }

  Group::~Group() {
    for(auto & i : frame)
      delete i;
    for(auto & i : contour)
      delete i;
    for(auto & i : group)
      delete i;
    for(auto & i : object)
      delete i;
    for(auto & i : link)
      delete i;
    for(auto & i : constraint)
      delete i;
    for(auto & i : observer)
      delete i;
  }

  void Group::addFrame(Frame* frame_) {
    frame.push_back(frame_);
    frame_->setParent(this);
    if(not frame_->getDedicatedFileItem()) frame_->setDedicatedFileItem(dedicatedFileItem);
    if(not frame_->getDedicatedParameterFileItem()) frame_->setDedicatedParameterFileItem(dedicatedFileItem);
    frame_->updateStatus();
  }

  void Group::addContour(Contour* contour_) {
    contour.push_back(contour_);
    contour_->setParent(this);
    if(not contour_->getDedicatedFileItem()) contour_->setDedicatedFileItem(dedicatedFileItem);
    if(not contour_->getDedicatedParameterFileItem()) contour_->setDedicatedParameterFileItem(dedicatedFileItem);
    contour_->updateStatus();
  }

  void Group::addGroup(Group* group_) {
    group.push_back(group_);
    group_->setParent(this);
    if(not group_->getDedicatedFileItem()) group_->setDedicatedFileItem(dedicatedFileItem);
    if(not group_->getDedicatedParameterFileItem()) group_->setDedicatedParameterFileItem(dedicatedFileItem);
    group_->updateStatus();
  }

  void Group::addObject(Object* object_) {
    object.push_back(object_);
    object_->setParent(this);
    if(not object_->getDedicatedFileItem()) object_->setDedicatedFileItem(dedicatedFileItem);
    if(not object_->getDedicatedParameterFileItem()) object_->setDedicatedParameterFileItem(dedicatedFileItem);
    object_->updateStatus();
  }

  void Group::addLink(Link* link_) {
    link.push_back(link_);
    link_->setParent(this);
    if(not link_->getDedicatedFileItem()) link_->setDedicatedFileItem(dedicatedFileItem);
    if(not link_->getDedicatedParameterFileItem()) link_->setDedicatedParameterFileItem(dedicatedFileItem);
    link_->updateStatus();
  }

  void Group::addConstraint(Constraint* constraint_) {
    constraint.push_back(constraint_);
    constraint_->setParent(this);
    if(not constraint_->getDedicatedFileItem()) constraint_->setDedicatedFileItem(dedicatedFileItem);
    if(not constraint_->getDedicatedParameterFileItem()) constraint_->setDedicatedParameterFileItem(dedicatedFileItem);
    constraint_->updateStatus();
  }

  void Group::addObserver(Observer* observer_) {
    observer.push_back(observer_);
    observer_->setParent(this);
    if(not observer_->getDedicatedFileItem()) observer_->setDedicatedFileItem(dedicatedFileItem);
    if(not observer_->getDedicatedParameterFileItem()) observer_->setDedicatedParameterFileItem(dedicatedFileItem);
    observer_->updateStatus();
  }

  void Group::removeElement(Element* element) {
    if(dynamic_cast<Frame*>(element)) {
      for(auto it = frame.begin(); it != frame.end(); ++it)
        if(*it==element) {
          frame.erase(it);
          break;
        }
    }
    else if(dynamic_cast<Contour*>(element)) {
      for(auto it = contour.begin(); it != contour.end(); ++it)
        if(*it==element) {
          contour.erase(it);
          break;
        }
    }
    else if(dynamic_cast<Group*>(element)) {
      for(auto it = group.begin(); it != group.end(); ++it)
        if(*it==element) {
          group.erase(it);
          break;
        }
    }
    else if(dynamic_cast<Object*>(element)) {
      for(auto it = object.begin(); it != object.end(); ++it)
        if(*it==element) {
          object.erase(it);
          break;
        }
    }
    else if(dynamic_cast<Link*>(element)) {
      for(auto it = link.begin(); it != link.end(); ++it)
        if(*it==element) {
          link.erase(it);
          break;
        }
    }
    else if(dynamic_cast<Constraint*>(element)) {
      for(auto it = constraint.begin(); it != constraint.end(); ++it)
        if(*it==element) {
          constraint.erase(it);
          break;
        }
    }
    else if(dynamic_cast<Observer*>(element)) {
      for(auto it = observer.begin(); it != observer.end(); ++it)
        if(*it==element) {
          observer.erase(it);
          break;
        }
    }
    delete element;
  }

  void Group::removeXMLElements() {
    DOMNode *e = element->getFirstChild();
    while(e) {
      DOMNode *en=e->getNextSibling();
      if((e != frames) and (e != contours) and (e != groups) and (e != objects) and (e != links) and (e != constraints) and (e != observers) and (E(e)->getTagName() != MBSIM%"enableOpenMBVFrameI") and (E(e)->getTagName() != MBSIM%"plotFeatureFrameI"))
        element->removeChild(e);
      e = en;
    }
  }

  DOMElement* Group::createXMLElement(DOMNode *parent) {
    DOMElement *ele0 = Element::createXMLElement(parent);
    DOMDocument *doc=ele0->getOwnerDocument();
    frames = D(doc)->createElement( MBSIM%"frames" );
    ele0->insertBefore( frames, nullptr );
    contours = D(doc)->createElement( MBSIM%"contours" );
    ele0->insertBefore( contours, nullptr );
    groups = D(doc)->createElement( MBSIM%"groups" );
    ele0->insertBefore( groups, nullptr );
    objects = D(doc)->createElement( MBSIM%"objects" );
    ele0->insertBefore( objects, nullptr );
    links = D(doc)->createElement( MBSIM%"links" );
    ele0->insertBefore( links, nullptr );
    constraints = D(doc)->createElement( MBSIM%"constraints" );
    ele0->insertBefore( constraints, nullptr );
    observers = D(doc)->createElement( MBSIM%"observers" );
    ele0->insertBefore( observers, nullptr );

    DOMElement *ele1 = D(doc)->createElement( MBSIM%"enableOpenMBVFrameI" );
    ele0->insertBefore( ele1, nullptr );

    return ele0;
  }

  DOMElement* Group::processIDAndHref(DOMElement *element) {
    element = Element::processIDAndHref(element);

    DOMElement *ELE=E(element)->getFirstElementChildNamed(MBSIM%"frames")->getFirstElementChild();
    for(size_t i=1; i<frame.size(); i++) {
      frame[i]->processIDAndHref(ELE);
      ELE=ELE->getNextElementSibling();
    }

    ELE=E(element)->getFirstElementChildNamed(MBSIM%"contours")->getFirstElementChild();
    for(auto & i : contour) {
      i->processIDAndHref(ELE);
      ELE=ELE->getNextElementSibling();
    }

    ELE=E(element)->getFirstElementChildNamed(MBSIM%"groups")->getFirstElementChild();
    for(auto & i : group) {
      i->processIDAndHref(ELE);
      ELE=ELE->getNextElementSibling();
    }

    ELE=E(element)->getFirstElementChildNamed(MBSIM%"objects")->getFirstElementChild();
    for(auto & i : object) {
      i->processIDAndHref(ELE);
      ELE=ELE->getNextElementSibling();
    }

    ELE=E(element)->getFirstElementChildNamed(MBSIM%"links")->getFirstElementChild();
    for(auto & i : link) {
      i->processIDAndHref(ELE);
      ELE=ELE->getNextElementSibling();
    }

    ELE=E(element)->getFirstElementChildNamed(MBSIM%"constraints");
    if(ELE) {
      ELE=ELE->getFirstElementChild();
      for(auto & i : constraint) {
        i->processIDAndHref(ELE);
        ELE=ELE->getNextElementSibling();
      }
    }

    ELE=E(element)->getFirstElementChildNamed(MBSIM%"observers");
    if(ELE) {
      ELE=ELE->getFirstElementChild();
      for(auto & i : observer) {
        i->processIDAndHref(ELE);
        ELE=ELE->getNextElementSibling();
      }
    }

    ELE=E(element)->getFirstElementChildNamed(MBSIM%"enableOpenMBVFrameI");
    if(ELE)
      E(ELE)->addProcessingInstructionChildNamed("OPENMBV_ID", getFrame(0)->getID());

    return element;
  }

  void Group::create() {
    Element::create();

    frames = E(element)->getFirstElementChildNamed(MBSIM%"frames");
    DOMElement *ELE=frames->getFirstElementChild();
    Frame *f;
    while(ELE) {
      f = Embed<FixedRelativeFrame>::create(ELE,this);
      if(f) {
        addFrame(f);
        f->create();
      }
      ELE=ELE->getNextElementSibling();
    }

    contours = E(element)->getFirstElementChildNamed(MBSIM%"contours");
    ELE=contours->getFirstElementChild();
    Contour *c;
    while(ELE) {
      c = Embed<Contour>::create(ELE,this);
      if(c) {
        addContour(c);
        c->create();
      }
      ELE=ELE->getNextElementSibling();
    }

    groups = E(element)->getFirstElementChildNamed(MBSIM%"groups");
    ELE=groups->getFirstElementChild();
    Group *g;
    while(ELE) {
      g = Embed<Group>::create(ELE,this);
      if(g) {
        addGroup(g);
        g->create();
      }
      ELE=ELE->getNextElementSibling();
    }

    objects = E(element)->getFirstElementChildNamed(MBSIM%"objects");
    ELE=objects->getFirstElementChild();
    Object *o;
    while(ELE) {
      o = Embed<Object>::create(ELE,this);
      if(o) {
        addObject(o);
        o->create();
      }
      ELE=ELE->getNextElementSibling();
    }

    links = E(element)->getFirstElementChildNamed(MBSIM%"links");
    ELE=links->getFirstElementChild();
    Link *l;
    while(ELE) {
      l = Embed<Link>::create(ELE,this);
      if(l) {
        addLink(l);
        l->create();
      }
      ELE=ELE->getNextElementSibling();
    }

    constraints = E(element)->getFirstElementChildNamed(MBSIM%"constraints");
    ELE=constraints->getFirstElementChild();
    Constraint *constraint;
    while(ELE) {
      constraint = Embed<Constraint>::create(ELE,this);
      if(constraint) {
        addConstraint(constraint);
        constraint->create();
      }
      ELE=ELE->getNextElementSibling();
    }

    observers = E(element)->getFirstElementChildNamed(MBSIM%"observers");
    ELE=observers->getFirstElementChild();
    Observer *obsrv;
    while(ELE) {
      obsrv = Embed<Observer>::create(ELE,this);
      if(obsrv) {
        addObserver(obsrv);
        obsrv->create();
      }
      ELE=ELE->getNextElementSibling();
    }
  }

  void Group::clear() {
    for (auto it = frame.begin()+1; it != frame.end(); ++it)
      delete *it;
    for (auto it = contour.begin(); it != contour.end(); ++it)
      delete *it;
    for (auto it = group.begin(); it != group.end(); ++it)
      delete *it;
    for (auto it = object.begin(); it != object.end(); ++it)
      delete *it;
    for (auto it = link.begin(); it != link.end(); ++it)
      delete *it;
    for (auto it = constraint.begin(); it != constraint.end(); ++it)
      delete *it;
    for (auto it = observer.begin(); it != observer.end(); ++it)
      delete *it;
    frame.erase(frame.begin()+1,frame.end());
    contour.erase(contour.begin(),contour.end());
    group.erase(group.begin(),group.end());
    object.erase(object.begin(),object.end());
    link.erase(link.begin(),link.end());
    constraint.erase(constraint.begin(),constraint.end());
    observer.erase(observer.begin(),observer.end());
  }

  void Group::setDedicatedFileItem(FileItemData *dedicatedFileItem) {
    Element::setDedicatedFileItem(dedicatedFileItem);
    frame[0]->setDedicatedFileItem(dedicatedFileItem);
  }

  void Group::setDedicatedParameterFileItem(FileItemData* dedicatedParameterFileItem) {
    Element::setDedicatedParameterFileItem(dedicatedParameterFileItem);
    frame[0]->setDedicatedParameterFileItem(dedicatedFileItem);
  }

  Frame* Group::getFrame(const QString &name) const {
    size_t i;
    for(i=0; i<frame.size(); i++) {
      if(frame[i]->getName() == name)
        return frame[i];
    }
    return nullptr;
  }

  Contour* Group::getContour(const QString &name) const {
    size_t i;
    for(i=0; i<contour.size(); i++) {
      if(contour[i]->getName() == name)
        return contour[i];
    }
    return nullptr;
  }

  Group* Group::getGroup(const QString &name) const {
    size_t i;
    for(i=0; i<group.size(); i++) {
      if(group[i]->getName() == name)
        return group[i];
    }
    return nullptr;
  }

  Object* Group::getObject(const QString &name) const {
    size_t i;
    for(i=0; i<object.size(); i++) {
      if(object[i]->getName() == name)
        return object[i];
    }
    return nullptr;
  }

  Link* Group::getLink(const QString &name) const {
    size_t i;
    for(i=0; i<link.size(); i++) {
      if(link[i]->getName() == name)
        return link[i];
    }
    return nullptr;
  }

  Constraint* Group::getConstraint(const QString &name) const {
    size_t i;
    for(i=0; i<constraint.size(); i++) {
      if(constraint[i]->getName() == name)
        return constraint[i];
    }
    return nullptr;
  }

  Observer* Group::getObserver(const QString &name) const {
    size_t i;
    for(i=0; i<observer.size(); i++) {
      if(observer[i]->getName() == name)
        return observer[i];
    }
    return nullptr;
  }

  Element * Group::getChildByContainerAndName(const QString &container, const QString &name) const {
    if (container == "Object")
      return getObject(name);
    else if (container == "Link")
      return getLink(name);
    else if (container == "Constraint")
      return getConstraint(name);
    else if (container == "Group")
      return getGroup(name);
    else if (container == "Frame")
      return getFrame(name);
    else if (container == "Contour")
      return getContour(name);
    else if (container == "Observer")
      return getObserver(name);
    else
      return nullptr;
  }

  int Group::getIndexOfFrame(Frame *frame_) {
    for(size_t i=0; i<frame.size(); i++)
      if(frame[i] == frame_)
        return i;
    return -1;
  }

  int Group::getIndexOfContour(Contour *contour_) {
    for(size_t i=0; i<contour.size(); i++)
      if(contour[i] == contour_)
        return i;
    return -1;
  }

  int Group::getIndexOfGroup(Group *group_) {
    for(size_t i=0; i<group.size(); i++)
      if(group[i] == group_)
        return i;
    return -1;
  }

  int Group::getIndexOfObject(Object *object_) {
    for(size_t i=0; i<object.size(); i++)
      if(object[i] == object_)
        return i;
    return -1;
  }

  int Group::getIndexOfLink(Link *link_) {
    for(size_t i=0; i<link.size(); i++)
      if(link[i] == link_)
        return i;
    return -1;
  }

  int Group::getIndexOfConstraint(Constraint *constraint_) {
    for(size_t i=0; i<constraint.size(); i++)
      if(constraint[i] == constraint_)
        return i;
    return -1;
  }

  int Group::getIndexOfObserver(Observer *observer_) {
    for(size_t i=0; i<observer.size(); i++)
      if(observer[i] == observer_)
        return i;
    return -1;
  }

  void Group::updateStatus() {
    Element::updateStatus();
    for(auto & i : frame)
      i->updateStatus();
    for(auto & i : contour)
      i->updateStatus();
    for(auto & i : group)
      i->updateStatus();
    for(auto & i : object)
      i->updateStatus();
    for(auto & i : link)
      i->updateStatus();
    for(auto & i : constraint)
      i->updateStatus();
    for(auto & i : observer)
      i->updateStatus();
    emitDataChangedOnChildren();
  }

  UnknownGroup::UnknownGroup() {
    icon = QIcon(new OverlayIconEngine((MainWindow::getInstallPath()/"share"/"mbsimgui"/"icons"/"group.svg").string(),
                                       (MainWindow::getInstallPath()/"share"/"mbsimgui"/"icons"/"unknownelement.svg").string()));
  }

}
