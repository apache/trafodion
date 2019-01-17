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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

#include "AboutBox.h"
#include "CommonSqlCmpDbg.h"

class BreakpointDialog;
class QueryData;
class QueryTreeView;
namespace Ui
{
  class MainWindow;
}

class MainWindow:public QMainWindow
{
Q_OBJECT
public:
  explicit MainWindow (QWidget * parent = 0);
  ~MainWindow ();
  NABoolean Run ();
  NABoolean NeedToDisplay (Sqlcmpdbg::CompilationPhase phase);
  NABoolean NeedToStop (Int32 passNo, Int32 groupNo, Int32 currentTaskNo,
			CascadesTask * task, ExprNode * expr,
			CascadesPlan * plan);
  void SetDocumentTitle (Sqlcmpdbg::CompilationPhase phase);
  void syncMemoWithDoc ();
  void UpdateAllViews ();
  void UpdateQueryTreeView ();
  void UpdateMemoView ();
  void popUpContextMenu (const QPoint & pos);
  void CreateTDBView();
  void RestoreGeometry();
  void SaveGeometry();
  NABoolean IsDisplayExecutionEnabled();

  NABoolean IsBackToSqlCompiler_;
  static NABoolean IsQuitting;
  QueryData *m_querydata;

protected:

  void closeEvent (QCloseEvent *);

private slots:

  void on_actionBreakpoints_triggered ();
  void on_actionToolbar_toggled (bool isChecked);
  void on_actionStatus_Bar_toggled (bool isChecked);
  void on_toolBar_visibilityChanged (bool isVisible);
  void on_actionAbout_SqlDbg_triggered ();
  void on_actionNew_triggered ();
  void on_actionOpen_triggered ();
  void on_actionContinue_triggered ();
  void on_actionItemExpr_triggered ();
  void on_actionProperties_triggered ();
  void on_actionMemo_triggered ();
  void on_actionStepOneTask_triggered ();
  void on_actionStepGrp_triggered ();
  void on_actionStepExpr_triggered ();
  void on_actionStepTaskNum_triggered ();
  void on_actionStepTask_triggered();
  void on_actionFinishPass_triggered ();
  void on_actionFinish_triggered ();
  void on_actionRules_triggered();
  void on_actionQuery_Analysis_triggered();
  void on_actionUpdateMemo_triggered ();
  void on_actionTile_triggered ();
  void on_actionCascade_triggered ();
private:
  /*
     UI control member
  */
  Ui::MainWindow * ui;
  BreakpointDialog *breakpointDialog;
  AboutBox *aboutBox;
  QRect m_rect;
  /*
     Data Member
  */
  SQLDebugBrkPts *m_breakpoint;
  QMenu *m_popMenu;
  QString m_title;
  NABoolean m_FinishAllOptimizePass;
  /*
     Methd
  */
  void ActivateView (Sqlcmpdbg::CompilationPhase phase);
  void EnableMemoButton ();
  void DisableMemoButton ();
};

#endif // MAINWINDOW_H
