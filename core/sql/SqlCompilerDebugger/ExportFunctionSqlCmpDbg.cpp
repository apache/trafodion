// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
#include <QtGui>
#include "ExportFunctionSqlCmpDbg.h"
#include "MainWindow.h"
#include "QueryData.h" 

#define DISPLAY_WARNING 1032
 
extern MainWindow *mainWindow_;
extern SqlcmpdbgExpFuncs exportFunctions_;
extern QApplication* application_;
static int argc = 1;
static char **argv;	

SqlcmpdbgExpFuncs * GetSqlcmpdbgExpFuncs(void) 
{
  if (application_ == NULL)
     application_ = new QApplication(argc, argv);
     
  if(mainWindow_)
    delete mainWindow_;
  mainWindow_ = new MainWindow();
  exportFunctions_.fpDisplayQueryTree = DisplayQueryTree;
  exportFunctions_.fpSqldbgSetPointers = SqldbgSetPointers;
  exportFunctions_.fpDoMemoStep = DoMemoStep;
  exportFunctions_.fpHideQueryTree = HideQueryTree;
  exportFunctions_.fpDisplayTDBTree = DisplayTDBTree;
  exportFunctions_.fpDisplayExecution = DisplayExecution;
  exportFunctions_.fpCleanUp = CleanUp;
  return &exportFunctions_;
}

void DisplayQueryTree(Sqlcmpdbg::CompilationPhase phase, 
                        void *tree, void *plan) 
{
  if (MainWindow::IsQuiting)
    return;
  mainWindow_->SetDocumentTitle(phase);
  mainWindow_->show();
  if (!mainWindow_->NeedToDisplay(phase)) 
  {
      mainWindow_->hide();
      return;
  }
  mainWindow_->m_querydata->SetPhase(phase);
  mainWindow_->m_querydata->SetData(tree, plan);
  mainWindow_->syncMemoWithDoc();
  
  mainWindow_->UpdateAllViews();
  mainWindow_->Run();
  mainWindow_->hide();
}

void SqldbgSetPointers(void *memoptr , void *tasklist ,
                         void *analysis , void *currentContext ,
                         void *ClusterInfo ) 
{
  if (MainWindow::IsQuiting)
    return;
  mainWindow_->m_querydata->SetMemo(memoptr);
  mainWindow_->m_querydata->SetAnalysis(analysis);
  mainWindow_->m_querydata->SetTaskList(tasklist);
  cmpCurrentContext = (CmpContext *) currentContext;

  CURRSTMT_OPTGLOBALS->memo = (CascadesMemo *) memoptr;
  
  // Initialize the GUI pointer to the compiler's global cluster info:
  gpClusterInfo = (NAClusterInfo *) ClusterInfo;
  
  // --------------------------------------------------------------
  // initialize optimization defaults
  // This is needed to initialize re-calibration constants
  // --------------------------------------------------------------
  CURRSTMT_OPTDEFAULTS->initializeCostInfo();
}

void DoMemoStep(Int32 passNo, Int32 groupNo, Int32 taskNo, void *task,
                  void *expr, void *plan) 
{
  if (MainWindow::IsQuiting)
    return;
  if (mainWindow_->NeedToStop(passNo,groupNo,taskNo,(CascadesTask *)task, (ExprNode *)expr,(CascadesPlan *)plan))
  {
     mainWindow_->show();
     mainWindow_->Run();
  }
}

void HideQueryTree(BOOL flag) 
{
  if (MainWindow::IsQuiting)
    return;
  mainWindow_->setVisible(flag);
}

void DisplayTDBTree(Sqlcmpdbg::CompilationPhase phase, 
                      void *tdb, void *fragDir) 
{
  if (MainWindow::IsQuiting)
    return;

  if (!mainWindow_->NeedToDisplay(phase)) 
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
  mainWindow_->CreateTDBView();
  mainWindow_->m_querydata->SetTDBData(tdb, fragDir);
  mainWindow_->m_querydata->SetPhase(phase);
  mainWindow_->show();
  mainWindow_->UpdateAllViews();
  mainWindow_->Run();
  mainWindow_->hide();
}

NABoolean DisplayExecution(void)
{
  *CmpCommon::diags() << DgSqlCode(DISPLAY_WARNING);
  return FALSE;
}

void CleanUp(void)
{
  if(mainWindow_)
    delete mainWindow_;
  mainWindow_ = NULL;
}

