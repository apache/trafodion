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
#ifndef VIEWCONTAINER_H
#define VIEWCONTAINER_H

#include "QueryTreeView.h"
#include "QueryMemoView.h"

#include <QWidget>
#include <QMdiSubWindow>
#include <QueryData.h>
namespace Ui
{
  class ViewContainer;
} class ViewContainer:public QWidget
{
Q_OBJECT public:
    explicit ViewContainer (QWidget * parent = 0);
   ~ViewContainer ();
  void SetMemoData (void *memoData);
  void ResetMemoMembers (void);
  void ResetMemoStops ();
  void turnOffMemoTrack ();
  void syncMemoWithDoc ();
  void memo_syncGrpNoExpNoToPlan (CascadesPlan * targetPlan);
  void memo_updateSelf ();

  void OnMemoStepOneTask ();
  void OnMemoStepexpr ();
  void OnMemoStepgrp ();
  void OnMemoSteptasknum ();
  void OnMemoStepTask();
  void OnMemoFinish ();
  void OnMemoFinishpass ();

  void UpdateView ();
  void UpdateQueryTreeView ();
  void UpdateMemoView ();
  void toggleMemoDisplay (bool bDisplay);
  bool isMemoVisible();

  NABoolean NeedToStop (Int32 passNo, Int32 groupNo, Int32 currentTaskNo,
			CascadesTask * task, ExprNode * expr,
			CascadesPlan * plan);
  virtual QSize minimumSizeHint() const;
protected:
  void closeEvent (QCloseEvent *);

private:
  Ui::ViewContainer * ui;
  QueryTreeView *m_queryTreeView;
  QueryMemoView *m_queryMemoView;
  QueryData *m_queryData;
};

#endif // VIEWCONTAINER_H
