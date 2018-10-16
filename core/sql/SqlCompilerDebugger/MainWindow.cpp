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
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ExprNode.h"
#include "BreakpointDialog.h"
#include "ViewContainer.h"
#include "ItemExpressionView.h"
#include "QueryData.h"
#include "PropDialog.h"
#include "RulesDialog.h"
#include "QueryAnalysisView.h"
#include "TDBTreeView.h"

/*
  The one and only QApplication object.
  The QApplication should be constructed before any UI control variables, e.g. the MainWindow.
  Otherwise, the UI cannot be launch due to errors.
*/
QApplication* GlobGuiApplication = NULL;

/*
  The MainWindow must be constucted after the QApplication.
  Otherwise, the UI cannot be launch due to errors.

  For every display query session, the MainWindow global
  object will be created when SqldbgSetCmpPointers is called
  for the first time.
*/

MainWindow *GlobGuiMainWindow = NULL;
SqlcmpdbgExpFuncs GlobGuiExportFunctions;
NABoolean MainWindow::IsQuitting = false;


MainWindow::MainWindow(QWidget * parent):QMainWindow(parent), ui(new Ui::MainWindow),
m_popMenu(NULL)
{
    ui->setupUi(this);
    //Initialize
    m_FinishAllOptimizePass = FALSE;
    IsQuitting = FALSE;
    // Set center screen
    QDesktopWidget *desktop = QApplication::desktop();
    move((desktop->width() - this->width()) / 2,
         (desktop->height() - this->height()) / 2);
    SaveGeometry();
    // When the visibility of toolbar changes, reflect to the checkability of toolbar menu item
    connect(ui->mainToolBar, SIGNAL(visibilityChanged(bool)), this,
            SLOT(on_toolBar_visibilityChanged(bool)));

    // Initialzie dialogs
    m_breakpoint = new SQLDebugBrkPts();
    breakpointDialog = new BreakpointDialog(m_breakpoint);
    breakpointDialog->setModal(true);
    aboutBox = new AboutBox();
    m_querydata = new QueryData;
    // Create first MDI window, as the action triggered by clicking New menu item
    ViewContainer *viewContainer = new ViewContainer();
    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(viewContainer);
    subWindow->resize(ui->mdiArea->width() / 2, ui->mdiArea->height() / 2);
    subWindow->showMaximized();
    //init context menu
    m_popMenu = new QMenu(this);
    QApplication::setStyle(new QWindowsStyle);
}

NABoolean MainWindow::Run()
{
    IsBackToSqlCompiler_ = FALSE;
    IsQuitting = FALSE;
    while (!IsBackToSqlCompiler_)
    {
        if (!IsQuitting)
        {
            GlobGuiApplication->processEvents(
                 QEventLoop::WaitForMoreEvents |
                 QEventLoop::EventLoopExec);
        }
        else
        {
            break;
        }
    }
    return FALSE;
}

MainWindow::~MainWindow()
{
    delete m_querydata;
    delete aboutBox;
    delete breakpointDialog;
    delete m_breakpoint;
}

void MainWindow::RestoreGeometry()
{
  setGeometry(m_rect);
}

void MainWindow::SaveGeometry()
{
  m_rect = geometry();
}

/* Public Method Begin*/
NABoolean MainWindow::NeedToDisplay(Sqlcmpdbg::CompilationPhase phase)
{
    BOOL retval = FALSE;
    if (phase == Sqlcmpdbg::AFTER_PARSING)
    {
        breakpointDialog->ShowBreakpoint();
    }
    //------------------------------------------------------------------------
    // Depending on the phase make the appropriate frame active.
    //----------------------------------------------------------------------
    ActivateView(phase);
    //--------------------------------------------------------------------------
    // If we need to display at this phase of compilation then we should
    // also change the document title to specify the phase of compilation.
    // This would be helpful to the user in determining which compilation phase
    // this display is for.
    //--------------------------------------------------------------------------
    switch (phase)
    {
    case Sqlcmpdbg::AFTER_PARSING:
        retval = m_breakpoint->brkAfterParsing;
        break;
    case Sqlcmpdbg::AFTER_BINDING:
        retval = m_breakpoint->brkAfterBinding;
        break;
    case Sqlcmpdbg::AFTER_TRANSFORMATION:
        retval = m_breakpoint->brkAfterTransform;
        break;
    case Sqlcmpdbg::AFTER_NORMALIZATION:
        retval = m_breakpoint->brkAfterNormalize;
        m_querydata->SetMemoSteppingFlag(retval);
        break;
    case Sqlcmpdbg::AFTER_SEMANTIC_QUERY_OPTIMIZATION:
        retval = m_breakpoint->brkAfterSemanticQueryOptimization;
        m_querydata->SetMemoSteppingFlag(retval);
        break;
        // Display of MVQR candidate replacement trees goes along with Analyzer, but has separate caption.
    case Sqlcmpdbg::DURING_MVQR:
    case Sqlcmpdbg::AFTER_ANALYZE:
        retval = m_breakpoint->brkAfterAnalyze;
        m_querydata->SetMemoSteppingFlag(retval);
        EnableMemoButton();
        break;
    case Sqlcmpdbg::AFTER_OPT1:
        retval = m_breakpoint->brkAfterOpt1;
        m_querydata->SetMemoSteppingFlag(retval);
        //if finish all pass in memo button clicked skip this stop
        if(m_FinishAllOptimizePass)
          retval = FALSE;
        break;
    case Sqlcmpdbg::AFTER_OPT2:
        retval = m_breakpoint->brkAfterOpt2;
        DisableMemoButton();
        break;
    case Sqlcmpdbg::AFTER_PRECODEGEN:
        retval = m_breakpoint->brkAfterPreCodegen;
        break;
    case Sqlcmpdbg::AFTER_CODEGEN:
        retval = m_breakpoint->brkAfterCodegen;
        break;
    case Sqlcmpdbg::AFTER_TDBGEN:
        retval = m_breakpoint->brkAfterTDBgen;
        break;
    case Sqlcmpdbg::DURING_EXECUTION:
        retval = m_breakpoint->brkDuringExecution;
        break;
    case Sqlcmpdbg::DURING_MEMOIZATION:
        retval = TRUE;
        break;
    case Sqlcmpdbg::FROM_MSDEV:
        retval = TRUE;
        break;
    default:
        break;
    }
    return retval;
}

//--------------------------------------------------------------------------
// If we need to display at this phase of compilation then we should
// also change the document title to specify the phase of compilation.
// This would be helpful to the user in determining which compilation phase
// this display is for.
//--------------------------------------------------------------------------
void MainWindow::SetDocumentTitle(Sqlcmpdbg::CompilationPhase phase)
{
    switch (phase)
    {
    case Sqlcmpdbg::AFTER_PARSING:
        m_title = "After Parsing";
        break;
    case Sqlcmpdbg::AFTER_BINDING:
        m_title = "After Binding";
        break;
    case Sqlcmpdbg::AFTER_TRANSFORMATION:
        m_title = "After Transformation";
        break;
    case Sqlcmpdbg::AFTER_NORMALIZATION:
        m_title = "After Normalization";
        break;
    case Sqlcmpdbg::AFTER_SEMANTIC_QUERY_OPTIMIZATION:
        m_title = "After Semantic Query Optimization";
        break;
    case Sqlcmpdbg::DURING_MVQR:
        m_title = "MVQR Candidate";
        break;
    case Sqlcmpdbg::AFTER_ANALYZE:
        m_title = "After Analysis";
        break;
    case Sqlcmpdbg::AFTER_OPT1:
        m_title = "After phase 1 Optimization";
        break;
    case Sqlcmpdbg::AFTER_OPT2:
        m_title = "After phase 2 Optimization";
        break;
    case Sqlcmpdbg::AFTER_PRECODEGEN:
        m_title = "After PreCodeGen";
        break;
    case Sqlcmpdbg::AFTER_CODEGEN:
        m_title = "After CodeGen";
        break;
    case Sqlcmpdbg::AFTER_TDBGEN:
        m_title = "After TDBGen";
        break;
    case Sqlcmpdbg::DURING_EXECUTION:
        m_title = "During Execution";
        break;
    case Sqlcmpdbg::DURING_MEMOIZATION:
        m_title = "During Memoization";
        break;
    case Sqlcmpdbg::FROM_MSDEV:
        m_title = "From MSDEV";
        break;
    default:
        break;
    }
}

void MainWindow::UpdateAllViews()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ViewContainer* container = (ViewContainer *) subWindows[i]->widget();
            container->setWindowTitle(m_title);
            container->UpdateView();
            container->show();
        }
        else if (subWindows[i]->widget()->inherits("PropDialog"))
        {
            ((PropDialog *) subWindows[i]->widget())->FreezeDisplay();
        }
        else if (subWindows[i]->widget()->inherits("TDBTreeView"))
        {
            TDBTreeView * tdb = (TDBTreeView *) subWindows[i]->widget();
            tdb->UpdateView();
            tdb->show();
        }
    } //end for (int i = 0; i < length; i++)
}

void MainWindow::UpdateQueryTreeView()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->widget())->
                UpdateQueryTreeView();
        }
    }
}

void MainWindow::UpdateMemoView()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->widget())->UpdateMemoView();
        }
    }
}

void MainWindow::syncMemoWithDoc()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->widget())->syncMemoWithDoc();
            break;
        }
    }
}

void MainWindow::ActivateView(Sqlcmpdbg::CompilationPhase phase)
{
    QList < QMdiSubWindow * >subWindows;
    int length;
    switch (phase)
    {
    case Sqlcmpdbg::AFTER_PARSING:
        m_querydata->ResetData();
        subWindows = ui->mdiArea->subWindowList();
        length = subWindows.length();
        for (int i = 0; i < length; i++)
        {
            if (subWindows[i]->widget()->inherits("ViewContainer"))
            {
                subWindows[i]->activateWindow();
                ViewContainer *viewContainer =
                    (ViewContainer *) subWindows[i]->widget();
                viewContainer->ResetMemoMembers();
                break;
            }
        }
        break;
    case Sqlcmpdbg::AFTER_TDBGEN:
        m_querydata->ResetData();
        subWindows = ui->mdiArea->subWindowList();
        length = subWindows.length();
        for (int i = 0; i < length; i++)
        {
            if (subWindows[i]->widget()->inherits("TDBView"))
            {
                subWindows[i]->activateWindow();
                break;
            }
        }
        break;
    default:
        break;
    }
}

void MainWindow::popUpContextMenu(const QPoint & pos)
{
    if (NULL != m_popMenu)
    {
        m_popMenu->addAction(ui->actionItemExpr);
        m_popMenu->addAction(ui->actionProperties);
        m_popMenu->addAction(ui->actionUpdateMemo);
        m_popMenu->exec(pos);
    }
}

NABoolean
    MainWindow::NeedToStop(Int32 passNo, Int32 groupNo,
                           Int32 currentTaskNo, CascadesTask * task,
                           ExprNode * expr, CascadesPlan * plan)
{
    BOOL retval = FALSE;
    //current phase is in breakpoint structure
    switch (m_querydata->GetPhase())
    {
    case Sqlcmpdbg::AFTER_ANALYZE:
        retval = m_breakpoint->brkAfterAnalyze;
        break;
    case Sqlcmpdbg::AFTER_OPT1:
        retval = m_breakpoint->brkAfterOpt1;
        break;
    default:
        break;
    }
    if (!retval)
        return retval;
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ViewContainer *Container =
                (ViewContainer *) subWindows[i]->widget();
            if (Container)
                return Container->NeedToStop(passNo, groupNo,
                                             currentTaskNo, task, expr,
                                             plan);
        }
    }
}

void
 MainWindow::EnableMemoButton()
{
    ui->actionStepOneTask->setEnabled(true);
    ui->actionStepGrp->setEnabled(true);
    ui->actionStepExpr->setEnabled(true);
    ui->actionStepTaskNum->setEnabled(true);
    ui->actionStepTask->setEnabled(true);
    ui->actionFinishPass->setEnabled(true);
    ui->actionFinish->setEnabled(true);
}

void MainWindow::DisableMemoButton()
{
    ui->actionStepOneTask->setEnabled(false);
    ui->actionStepGrp->setEnabled(false);
    ui->actionStepExpr->setEnabled(false);
    ui->actionStepTaskNum->setEnabled(false);
    ui->actionStepTask->setEnabled(false);
    ui->actionFinishPass->setEnabled(false);
    ui->actionFinish->setEnabled(false);
}

/* Private Method End */
void MainWindow::on_actionBreakpoints_triggered()
{
    breakpointDialog->activateWindow();
    breakpointDialog->ShowBreakpoint();
}

void MainWindow::on_actionToolbar_toggled(bool isChecked)
{
    ui->mainToolBar->setVisible(isChecked);
}

void MainWindow::on_actionStatus_Bar_toggled(bool isChecked)
{
    ui->statusBar->setVisible(isChecked);
}

void MainWindow::on_toolBar_visibilityChanged(bool isVisible)
{
    ui->actionToolbar->setChecked(isVisible);
}

void MainWindow::on_actionAbout_SqlDbg_triggered()
{
    aboutBox->show();
}

void MainWindow::on_actionNew_triggered()
{

}

void MainWindow::on_actionOpen_triggered()
{

}

void MainWindow::on_actionContinue_triggered()
{
    if (m_querydata->GetPhase() == Sqlcmpdbg::AFTER_NORMALIZATION ||
        m_querydata->GetPhase() ==
        Sqlcmpdbg::AFTER_SEMANTIC_QUERY_OPTIMIZATION
        || m_querydata->GetPhase() == Sqlcmpdbg::AFTER_ANALYZE
        || m_querydata->GetPhase() == Sqlcmpdbg::AFTER_OPT1)
    {
        QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
        int length = subWindows.length();
        for (int i = 0; i < length; i++)
        {
            if (subWindows[i]->widget()->inherits("ViewContainer"))
            {
                ((ViewContainer *) subWindows[i]->widget())->
                    ResetMemoStops();
                ((ViewContainer *) subWindows[i]->widget())->
                    turnOffMemoTrack();
                break;
            }
        }
    }
    IsBackToSqlCompiler_ = TRUE;
}

void MainWindow::on_actionItemExpr_triggered()
{
    ExprNode *expr = (ExprNode *) m_querydata->GetCurrExprNode();
    if(expr)
    {
      ItemExpressionView *IEView = new ItemExpressionView();
      NAString enText = expr->getText();
      IEView->setNewExpr(expr);
      IEView->UpdateView();
      QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(IEView);
      subWindow->setAttribute(Qt::WA_DeleteOnClose);
      QString title = QString("Item Exprs for %1 , %2").arg(enText.data()).arg(m_title);
      subWindow->setWindowTitle(title);
      subWindow->show();
    }
}

void MainWindow::on_actionProperties_triggered()
{
    if (NULL != m_querydata->GetCurrExprNode())
    {
        ExprNode *en = (ExprNode *) m_querydata->GetCurrExprNode();
        NAString enText = en->getText();
        PropDialog *prop =
            new PropDialog((CascadesPlan *) m_querydata->GetCurrPlan(),
                           en,
                           enText);
        QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(prop);
        subWindow->setAttribute(Qt::WA_DeleteOnClose);
        subWindow->show();
    }
}

void MainWindow::on_actionMemo_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ViewContainer * container = (ViewContainer *) subWindows[i]->widget();
            container->toggleMemoDisplay(!container->isMemoVisible());
            break;
        }
    }
}

void MainWindow::on_actionStepOneTask_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->widget())->OnMemoStepOneTask();
            break;
        }
    }
}

void MainWindow::on_actionStepGrp_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->widget())->OnMemoStepgrp();
            break;
        }
    }
}

void MainWindow::on_actionStepExpr_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->widget())->OnMemoStepexpr();
            break;
        }
    }
}

void MainWindow::on_actionStepTaskNum_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->
             widget())->OnMemoSteptasknum();
            break;
        }
    }
}

void MainWindow::on_actionStepTask_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->
             widget())->OnMemoStepTask();
            break;
        }
    }
}

void MainWindow::on_actionFinish_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            m_FinishAllOptimizePass = TRUE;
            ((ViewContainer *) subWindows[i]->widget())->OnMemoFinish();
            break;
        }
    }
}

void MainWindow::on_actionFinishPass_triggered()
{
    QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
    int length = subWindows.length();
    for (int i = 0; i < length; i++)
    {
        if (subWindows[i]->widget()->inherits("ViewContainer"))
        {
            ((ViewContainer *) subWindows[i]->
             widget())->OnMemoFinishpass();
            break;
        }
    }
}

void MainWindow::on_actionRules_triggered()
{
    RulesDialog* rdlg = new RulesDialog(this);
    rdlg->setAttribute(Qt::WA_DeleteOnClose);
    rdlg->exec();

}

void MainWindow::on_actionQuery_Analysis_triggered()
{
  if(m_querydata->GetPhase() == Sqlcmpdbg::AFTER_ANALYZE ||
       m_querydata->GetPhase() == Sqlcmpdbg::AFTER_OPT1 ||
       m_querydata->GetPhase() == Sqlcmpdbg::AFTER_OPT2 )
  {
    QueryAnalysisView *QaView = new QueryAnalysisView((QueryAnalysis*)m_querydata->GetAnalysis());
    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(QaView);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->show();
  }
}

void MainWindow::on_actionUpdateMemo_triggered()
{
    if (m_querydata->GetCurrPlan() != NULL)
    {
        CascadesPlan *plan = (CascadesPlan *) m_querydata->GetCurrPlan();
        QList < QMdiSubWindow * >subWindows = ui->mdiArea->subWindowList();
        int length = subWindows.length();
        for (int i = 0; i < length; i++)
        {
            if (subWindows[i]->widget()->inherits("ViewContainer"))
            {
                ViewContainer* container = (ViewContainer *) subWindows[i]->widget();
                container->toggleMemoDisplay(true);
                container->memo_syncGrpNoExpNoToPlan(plan);
                container->memo_updateSelf();
                return;
            }
        }
    }
}

void MainWindow::on_actionTile_triggered ()
{
  ui->mdiArea->tileSubWindows();
}

void MainWindow::on_actionCascade_triggered ()
{
  ui->mdiArea->cascadeSubWindows();
}

/*create a tdb view and add it as subWindow*/
void MainWindow::CreateTDBView()
{
  TDBTreeView* tdb = new TDBTreeView();
  QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(tdb);
}

void MainWindow::closeEvent(QCloseEvent * event)
{
    //when phase==Sqlcmpdbg::FROM_MSDEV,
    //you can't exit GUI Debugger by click system close,
    //"continue" is the only way out.
    if (m_querydata->GetPhase() == Sqlcmpdbg::FROM_MSDEV)
    {
      event->ignore();
    }
    else
    {
      IsQuitting = TRUE;
      event->accept();
    }
}
