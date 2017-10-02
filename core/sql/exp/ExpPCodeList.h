/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         ExpPCodeList.h
* RCS:          $Id: ExpPCodeList.h,v 1.1 2007/10/09 19:38:51  Exp $
* Description:  
*
* Created:      8/25/97
* Modified:     $ $Date: 2007/10/09 19:38:51 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
****************************************************************************
*/
#ifndef ExpPCodeList_h
#define ExpPCodeList_h

#include "ExpPCodeInstruction.h"

// Forward External Declaractions
//

// Forward Internal Declaractions
//
class ListLink;
class ListBase;
class ListBaseIter;
class PCILink;
class PCIList;
class PCIListIter;

class ListLink {
public:
  ListLink() { prev_ = 0; next_ = 0; };
  ListLink(ListLink *prev, ListLink *next) { prev_ = prev; next_ = next; };

  ListLink *getNext() { return next_; };
  ListLink *getPrev() { return prev_; };

  void setNext(ListLink *next) { next_ = next; };
  void setPrev(ListLink *prev) { prev_ = prev; };

private:
  ListLink *prev_, *next_;

};


class ListBase {
public:
  ListBase() { tail_ = NULL; }
  ListBase(ListLink *tail) { tail_ = tail; }

  void insert(ListLink *item) { 
    if(tail_) { 
      item->setNext(getHead()); 
      getHead()->setPrev(item); 
    }
    else { tail_ = item; }
    tail_->setNext(item);
    item->setPrev(tail_);
  };
  void append(ListLink *item) {
    insert(item);
    tail_ = getHead();
  };

  void insert(ListBase list) {
    if(!list.getHead()) return;
    if(!tail_) tail_ = list.getTail();
    else {
      ListLink *origHead = list.getHead();
      list.getTail()->setNext(getHead());
      getHead()->setPrev(list.getTail());
      getTail()->setNext(origHead);
      origHead->setPrev(getTail());
    }
  };
  void append(ListBase list) {
    if(!list.getHead()) return;
    insert(list);
    tail_ = list.getTail();
  };

  void remove(ListLink *item) { remove(item, item); };
  void remove(ListLink *fromItem, ListLink *toItem) {
    ListLink *prev = fromItem->getPrev();
    ListLink *next = toItem->getNext();
    if(prev) prev->setNext(next);
    if(next) next->setPrev(prev);
    if(tail_ == toItem) tail_ = prev;
  };

  ListLink *getHead() const { return tail_ ? tail_->getNext() : 0; };
  ListLink *getTail() const { return tail_; };

private:
  ListLink *tail_;
};


class ListBaseIter {
public:
  ListBaseIter(const ListBase &base) { 
    base_ = &base; link_ = base_->getTail(); 
  };
  
  ListBaseIter(ListBaseIter &iter) {
    base_ = iter.base_; link_ = iter.link_;
  };
  ListLink *next() { 
    if(link_ == base_->getTail()) link_ = NULL;
    if(link_) link_ = link_->getNext(); 
    return link_; 
  };
  ListLink *prev() {
    if(link_ == base_->getHead()) link_ = NULL;
    if(link_) link_ = link_->getPrev();
    return link_;
  }
  ListLink *first() { link_ = base_->getHead(); return link_; };
  ListLink *last() { link_ = base_->getTail(); return link_; };
  ListLink *curr() { return link_; };
  ListLink *prevPtr() { return (link_)? link_->getPrev(): NULL; };

private:
  ListLink *link_;
  const ListBase *base_;

};


class PCILink : public ListLink {
public:
  PCILink(PCodeInstruction *pci) : info_(pci) { ; };
  PCodeInstruction *getData() { return info_; };

private:
  PCodeInstruction *info_;

};


class PCIList : private ListBase {
  friend class PCIListIter;

public:
  PCIList(CollHeap *heap) : heap_(heap) { ; };
  PCIList(PCILink *base, CollHeap *heap) : 
                        ListBase(base), heap_(heap) { ; };

  void insert(PCodeInstruction *pci) { 
    ListBase::insert(new(heap_) PCILink(pci)); 
  };
  void append(PCodeInstruction *pci) { 
    ListBase::append(new(heap_) PCILink(pci)); 
  };
  void append(PCodeInstruction pci) {
    ListBase::append(new(heap_) PCILink(new(heap_) PCodeInstruction(&pci)));
  };
  void append(PCIList pciList) { ListBase::append(pciList); };

  void remove(PCILink *pciLink) { ListBase::remove(pciLink); };

  PCodeInstruction *getHead() { 
    return ((PCILink*)ListBase::getHead())->getData(); 
  };
  PCodeInstruction *getTail() { 
    return ((PCILink*)ListBase::getTail())->getData(); 
  };
  PCIID getHeadId() { 
    return (PCIID)((PCILink*)ListBase::getHead())->getData(); 
  };
  PCIID getTailId() { 
    return (PCIID)((PCILink*)ListBase::getTail())->getData(); 
  };
  PCILink *getList() { return (PCILink*)ListBase::getTail(); };

private:
  CollHeap *heap_;

};


class PCIListIter : private ListBaseIter {
public:
  PCIListIter(const PCIList &pcilist)
    : ListBaseIter(pcilist) { ; };
  PCIListIter(PCIListIter &pciListIter)
    : ListBaseIter(pciListIter) { ; };
  PCodeInstruction *next() { 
    PCILink *item = (PCILink*)ListBaseIter::next();
    return item ? item->getData() : NULL;
  };
  PCodeInstruction *first() { 
    PCILink *item = (PCILink*)ListBaseIter::first();
    return item ? item->getData() : NULL;
  };
  PCodeInstruction *last() {
    PCILink *item = (PCILink*)ListBaseIter::last();
    return item ? item->getData() : NULL;
  };
  PCodeInstruction *prev() {
    PCILink *item = (PCILink*)ListBaseIter::prev();
    return item ? item->getData() : NULL;
  };
  PCodeInstruction *curr() {
    PCILink *item = (PCILink*)ListBaseIter::curr();
    return item ? item->getData() : NULL;
  };

  PCILink *currItem() { return (PCILink*)ListBaseIter::curr(); };
  PCodeInstruction *prevPCI() {
    PCILink *item = (PCILink*)ListBaseIter::prevPtr();
    return item ? item->getData() : NULL;
  };
  PCILink *prevItem() { return (PCILink*)ListBaseIter::prevPtr(); };

};


#endif
