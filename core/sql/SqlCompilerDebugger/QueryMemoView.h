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
#ifndef QUERYMEMOVIEW_H
#define QUERYMEMOVIEW_H

#include <QtGui>

#include "CommonSqlCmpDbg.h"

namespace Ui
{
  class QueryMemoView;
}

typedef struct tagContextDetails
{
  Context *thisContext;
  QString costText;
  QString ilpText;
  QString statText;
  QString rppText;
  Int32 groupNo;
} CONTEXTDETAILS;

Q_DECLARE_METATYPE (tagContextDetails *);

struct DirtyGrid
{
  Int32 m_row;
  Int32 m_col;
  QIcon m_pict;
  DirtyGrid (Int32 row, Int32 col, const QIcon & pict);
};

class QueryMemoView:public QWidget
{
Q_OBJECT
public:
  explicit QueryMemoView (QWidget * parent = 0);
  ~QueryMemoView ();
  void ResetMemoMembers (void);
  void SetMemoData (void *memoData);
  void syncToDocument ();
  void syncGrpNoExpNoToPlan (CascadesPlan * targetPlan);
  NABoolean NeedToStop (Int32 passNo, Int32 groupNo, Int32 currentTaskNo,
			CascadesTask * task, ExprNode * expr,
			CascadesPlan * plan);
  void UpdateView ();
  void UpdateGroupList ();
  void calcCurrContext ();
  void updateSelfFully ();
  void displayContextList ();
  void FreeContextMemory ();
  void UpdateSpinControls ();
  void UpdateTaskList ();
  void UpdateContextList ();
  void FreeTaskMemory ();
  void displayTaskList ();
  Int32 NumExprsInGroup (Int32 grno);
  CascadesPlan *calcCurrPlan ();
  void setDisplayExpr (RelExpr * expr, CascadesPlan * plan);
  void DrawMemo (void);
  void setMemoMaxRowsCols ();
  void ClearMemoGrid ();
  void DrawMemoGroup (Int32 grno);
  void DrawExprInMemoGroup (Int32 grno);
  void DrawLogExpr (Int32 grno, Int32 & exprno);
  void DrawPhyExpr (Int32 grno, Int32 & exprno);
  void DrawPlans (Int32 grno, Int32 & exprno);
  void drawSelectedExpr ();
  void drawCandidatePlanAndSolution (Context* context);
  void InitTaskList ();
  void InitContextList ();
  void InitSpinControls ();
  void updateSelf ();
  //-----------------------------------------------------------------
  // GSH : To set and reset various stop points in the memo.
  //-----------------------------------------------------------------
  inline void Hold (NABoolean enabled = TRUE)
  {
    m_hold = enabled;
  }

  inline void StopAtNextTask (NABoolean enabled = FALSE)
  {
    m_bStopNextTask = enabled;
  }

  inline void StopAtTaskNo (Int32 v = -1, NABoolean enabled = FALSE)
  {
    m_stopTaskNo = v;
    m_bStopTaskNo = enabled;
  }
  inline void StopAtTask (void* task = NULL, NABoolean enabled = FALSE)
  {
    m_stopTask = task;
    m_bStopTask = enabled;
  }
  inline void StopAtNextGroup (NABoolean enabled = FALSE)
  {
    m_bStopNextGroup = enabled;
    //save current group number to m_stopGroupNo
    m_stopGroupNo = m_currGrpNo;
  }

  void StopAtExpr (Int32 grno = -1, Int32 exprno = -1, NABoolean enabled = FALSE);
/*
  inline void StopAtPass (Int32 v = -1)
  {
    m_stopPassNo = v;
  }
*/
  inline void ResetMemoStops ()
  {
    m_hold = FALSE;
    StopAtNextTask ();
    StopAtTaskNo ();
    StopAtTask ();
    StopAtNextGroup();
    StopAtExpr();
  }

  void turnOffMemoTrack ();
  void OnMemoStepOneTask ();
  void OnMemoStepexpr ();
  void OnMemoStepgrp ();
  void OnMemoSteptasknum ();
  void OnMemoStepTask();
  void OnMemoFinish ();
  void OnMemoFinishpass ();

private:
  void InitMemoDisplay (int width, int height);
  void calcCurrExprNo(ExprNode *expr, CascadesPlan *plan);
  void clearDirtyGrids ();
  void changeMemo ();
  void displayCurrExpr ();

private slots:
  void memoWidgetCell(int row, int col);
  void on_memoWidget_currentCellChanged( int currentRow, int currentColumn, int previousRow, int previousColumn );
  void on_memoWidget_cellClicked (int row, int column);

  void contextWidgetCell(int row, int column);
  void on_contextWidget_cellClicked (int row, int column);
private:
  Ui::QueryMemoView * ui;

  Int32 m_currTaskNo;
  Int32 m_currExpNo;
  Int32 m_currGrpNo;
  Int32 m_currPassNo;
  CascadesTask *m_currTask;
  Context *m_currContext;

  //----------------------------------------------------------------
  // GSH : The key data structures associated with the display of
  // view (Memo View).
  //----------------------------------------------------------------
  CascadesMemo *m_memo;
  CascadesTaskList *m_cascadesTask;
  QueryAnalysis *m_analysis;
  QList <Int32> m_groupSize;
  QList <CascadesTask *> m_taskList;
  QList <Context *> m_contextList;
  QList <CascadesGroup *> m_groupList;
    //----------------------------------------------------------------
  // GSH : these data members are used to set breakpoints during the
  // optimization process.
  //----------------------------------------------------------------
  Int32 m_stopGroupNo;		// stop at tasks within group
  Int32 m_stopTaskNo;		// stop at task > stopTaskNo_
  Int32 m_stopExprNo;           // stop at specified expr of specified group
  void* m_stopTask;             // stop at specified task in tasklist

  NABoolean m_bStopNextTask;
  NABoolean m_bStopTaskNo;
  NABoolean m_bStopTask;
  NABoolean m_bStopNextGroup;
  NABoolean m_bStopExprOfGrp;

  //----------------------------------------------------------------
  // GSH : Variables for storing some old state related values.
  //----------------------------------------------------------------
  Int32 m_initialGroups;
  Int32 m_oldGroups;
  Int32 m_oldTasks;
  //----------------------------------------------------------------
  // GSH : Variables for storing some current state related values.
  //----------------------------------------------------------------
  NABoolean m_trackGridCell;
  NABoolean m_memoGridHasElem;
  NABoolean m_hold;

  Int32 VIEWPORT_MAXROWS;
  Int32 VIEWPORT_MAXCOLS;
  Int32 RANGE_LOW;
  Int32 RANGE_HIGH;
  Int32 RANGE_HIGH_TASKS;
  QList <struct DirtyGrid *> m_dirtyList;
};

#endif // QUERYMEMOVIEW_H
