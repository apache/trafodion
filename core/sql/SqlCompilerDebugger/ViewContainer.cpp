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
#include "ViewContainer.h"
#include "ui_ViewContainer.h"

#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
ViewContainer::ViewContainer(QWidget * parent):QWidget(parent),
ui(new Ui::ViewContainer)
{
  ui->setupUi(this);
  /*
     Add splitter and set its style.
   */
  QSplitter *splitterView = new QSplitter(Qt::Vertical, this);
  splitterView->setStretchFactor(0, 1);
  splitterView->setStretchFactor(1, 2);
  splitterView->setOpaqueResize(false);
  splitterView->setStyleSheet
      ("QSplitter::handle{background-color:gray; border:1px solid black;}");
  splitterView->setHandleWidth(3);
  m_queryTreeView = new QueryTreeView(splitterView);
  m_queryMemoView = new QueryMemoView(splitterView);
  m_queryMemoView->hide();
  m_queryMemoView->InitTaskList();
  m_queryMemoView->InitContextList();
  m_queryMemoView->InitSpinControls();
  /*
     Add Layout to QSplitter,
     so that the QSplitter can adapt the window size automatically,
     when minimizing, maximizing and resizing.
     Also configure the Layout,
     so as to remove too much blank area between controls, otherwise the UI looks ugly.
   */
  QVBoxLayout *verticalLayout = new QVBoxLayout(this);
  verticalLayout->addWidget(splitterView);
  verticalLayout->setMargin(0);
  verticalLayout->setSpacing(0);
}

ViewContainer::~ViewContainer()
{
  delete ui;
}

QSize ViewContainer::minimumSizeHint() const
{
  return QSize(480, 320);
}

void ViewContainer::SetMemoData(void *memoData)
{
  m_queryMemoView->SetMemoData(memoData);
}

void ViewContainer::ResetMemoMembers()
{
  m_queryMemoView->ResetMemoMembers();
}

void ViewContainer::syncMemoWithDoc()
{
  m_queryMemoView->syncToDocument();
}

void ViewContainer::memo_syncGrpNoExpNoToPlan(CascadesPlan *targetPlan)
{
  m_queryMemoView->syncGrpNoExpNoToPlan(targetPlan);
}

void ViewContainer::memo_updateSelf()
{
  m_queryMemoView->updateSelf();
}

void ViewContainer::UpdateView()
{
  m_queryMemoView->UpdateView();
  m_queryTreeView->UpdateView();
}

void ViewContainer::UpdateQueryTreeView()
{
  m_queryTreeView->UpdateView();
}

void ViewContainer::UpdateMemoView()
{
  m_queryMemoView->UpdateView();
}

void ViewContainer::toggleMemoDisplay(bool bDisplay)
{
  m_queryMemoView->setVisible(bDisplay);
}

bool ViewContainer::isMemoVisible()
{
  return m_queryMemoView->isVisible();
}

NABoolean ViewContainer::NeedToStop(Int32 passNo, Int32 groupNo, Int32 currentTaskNo, CascadesTask *task, ExprNode *expr, CascadesPlan *plan)
{
  return m_queryMemoView->NeedToStop(passNo,groupNo,currentTaskNo,task,expr,plan);
}
void ViewContainer::ResetMemoStops()
{
  m_queryMemoView->ResetMemoStops();
}

void ViewContainer::turnOffMemoTrack()
{
  m_queryMemoView->turnOffMemoTrack();
}

void ViewContainer::OnMemoStepOneTask()
{
  m_queryMemoView->OnMemoStepOneTask();
}

void ViewContainer::OnMemoStepexpr()
{
  m_queryMemoView->OnMemoStepexpr();
}
void ViewContainer::OnMemoStepgrp()
{
  m_queryMemoView->OnMemoStepgrp();
}
void ViewContainer::OnMemoSteptasknum()
{
  m_queryMemoView->OnMemoSteptasknum();
}

void ViewContainer::OnMemoStepTask()
{
  m_queryMemoView->OnMemoStepTask();
}

void ViewContainer::OnMemoFinish()
{
  m_queryMemoView->OnMemoFinish();
}

void ViewContainer::OnMemoFinishpass()
{
  m_queryMemoView->OnMemoFinishpass();
}

void ViewContainer::closeEvent(QCloseEvent * event)
{
  event->ignore();
}

