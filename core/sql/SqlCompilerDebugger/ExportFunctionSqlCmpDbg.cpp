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
#include <QtGui>
#include "ExportFunctionSqlCmpDbg.h"
#include "MainWindow.h"
#include "QueryData.h"
#include "ExeSchedWindow.h"

#define DISPLAY_WARNING 1032

// defined in MainWindow.cpp
extern MainWindow *GlobGuiMainWindow;
extern SqlcmpdbgExpFuncs GlobGuiExportFunctions;
extern QApplication* GlobGuiApplication;

static int argc = 1;
static char **argv;	

SqlcmpdbgExpFuncs * GetSqlcmpdbgExpFuncs()
{
  if (GlobGuiApplication == NULL)
     GlobGuiApplication = new QApplication(argc, argv);

  GlobGuiExportFunctions.fpDisplayQueryTree = DisplayQueryTree;
  GlobGuiExportFunctions.fpSqldbgSetCmpPointers = SqldbgSetCmpPointers;
  GlobGuiExportFunctions.fpDoMemoStep = DoMemoStep;
  GlobGuiExportFunctions.fpHideQueryTree = HideQueryTree;
  GlobGuiExportFunctions.fpDisplayTDBTree = DisplayTDBTree;
  GlobGuiExportFunctions.fpExecutionDisplayIsEnabled = ExecutionDisplayIsEnabled;
  GlobGuiExportFunctions.fpSqldbgSetExePointers = SqldbgSetExePointers;
  GlobGuiExportFunctions.fpDisplayExecution = DisplayExecution;
  GlobGuiExportFunctions.fpCleanUp = CleanUp;
  return &GlobGuiExportFunctions;
}

void DisplayQueryTree(Sqlcmpdbg::CompilationPhase phase, 
                        void *tree, void *plan) 
{
  if (MainWindow::IsQuitting)
    return;

  GlobGuiMainWindow->SetDocumentTitle(phase);
  GlobGuiMainWindow->show();
  if (!GlobGuiMainWindow->NeedToDisplay(phase)) 
  {
      GlobGuiMainWindow->hide();
      return;
  }
  GlobGuiMainWindow->m_querydata->SetPhase(phase);
  GlobGuiMainWindow->m_querydata->SetData(tree, plan);
  GlobGuiMainWindow->syncMemoWithDoc();
  
  GlobGuiMainWindow->UpdateAllViews();
  GlobGuiMainWindow->Run();
  GlobGuiMainWindow->hide();
}

void SqldbgSetCmpPointers(void *memoptr , void *tasklist ,
                          void *analysis , void *currentContext ,
                          void *ClusterInfo ) 
{
  if (MainWindow::IsQuitting)
    return;

  ExeSchedWindow::deleteAllInstances();

  if (GlobGuiMainWindow == NULL)
    GlobGuiMainWindow = new MainWindow();

  GlobGuiMainWindow->m_querydata->SetMemo(memoptr);
  GlobGuiMainWindow->m_querydata->SetAnalysis(analysis);
  GlobGuiMainWindow->m_querydata->SetTaskList(tasklist);
  cmpCurrentContext = (CmpContext *) currentContext;

  // --------------------------------------------------------------
  // initialize optimization defaults
  // This is needed to initialize re-calibration constants
  // --------------------------------------------------------------
  CURRSTMT_OPTDEFAULTS->initializeCostInfo();
}

void DoMemoStep(Int32 passNo, Int32 groupNo, Int32 taskNo, void *task,
                  void *expr, void *plan) 
{
  if (MainWindow::IsQuitting)
    return;
  if (GlobGuiMainWindow->NeedToStop(passNo,groupNo,taskNo,(CascadesTask *)task, (ExprNode *)expr,(CascadesPlan *)plan))
  {
     GlobGuiMainWindow->show();
     GlobGuiMainWindow->Run();
  }
}

void HideQueryTree(BOOL flag) 
{
  if (MainWindow::IsQuitting)
    return;
  GlobGuiMainWindow->setVisible(flag);
}

void DisplayTDBTree(Sqlcmpdbg::CompilationPhase phase, 
                      void *tdb, void *fragDir) 
{
  if (MainWindow::IsQuitting)
    return;

  if (!GlobGuiMainWindow->NeedToDisplay(phase)) 
    return;
  //-----------------------------------------------------------------------------
  // GSH : The document class of the SQL/debug MDI DLL holds all the data that 
  // is displayed by the multiple view of the application. In particular it
  // has data members like queryTree_ and cascadesPlan_ which need to be set using 
  // the parameters passed to this function (ie. DisplayQueryTree(...)). 
  // The overall logic is to get a pointer to the document class and then use the 
  // member function setDocumentData(ExprNode* tree = NULL, CascadesPlan* plan=NULL) 
  // function to set the private data members of the document object.
  //-----------------------------------------------------------------------------
  GlobGuiMainWindow->CreateTDBView();
  GlobGuiMainWindow->m_querydata->SetTDBData(tdb, fragDir);
  GlobGuiMainWindow->m_querydata->SetPhase(phase);
  GlobGuiMainWindow->show();
  GlobGuiMainWindow->UpdateAllViews();
  GlobGuiMainWindow->Run();
  GlobGuiMainWindow->hide();
}

int  ExecutionDisplayIsEnabled(void)
{
  return GlobGuiMainWindow->MainWindow::NeedToDisplay(
       Sqlcmpdbg::DURING_EXECUTION);
}

void SqldbgSetExePointers(void *rootTcb,
                          void *cliGlobals,
                          void *dummy)
{
  // TODO: determine if this is needed, scheduler has all ptrs
}

void DisplayExecution(ExSubtask **subtask, ExScheduler *scheduler)
{
  ExeSchedWindow *myWindow = ExeSchedWindow::findInstance(scheduler);

  if (GlobGuiMainWindow)
    {
      delete GlobGuiMainWindow;
      GlobGuiMainWindow = NULL;
    }

  if (subtask == NULL)
    {
      // passing a NULL subtask is a sign that
      // we are done, delete the window
      if (scheduler)
        ExeSchedWindow::deleteInstance(scheduler);
      return;
    }

  if (!myWindow->needToStop(*subtask, scheduler))
    // no need to stop at this task
    return;

  myWindow->run(subtask);
}

void CleanUp(void)
{
  if(GlobGuiMainWindow)
    delete GlobGuiMainWindow;
  GlobGuiMainWindow = NULL;
}

