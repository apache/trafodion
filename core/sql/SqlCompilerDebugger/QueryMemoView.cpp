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
#include "QueryMemoView.h"
#include "ui_QueryMemoView.h"
#include "QueryData.h"
#include "MainWindow.h"

// defined in MainWindow.cpp
extern MainWindow *GlobGuiMainWindow;


DirtyGrid::DirtyGrid(Int32 row, Int32 col,const QIcon & pict) :
  m_row(row), m_col(col), m_pict(pict) {}

QueryMemoView::QueryMemoView(QWidget * parent):QWidget(parent),
ui(new Ui::QueryMemoView)
{
  ui->setupUi(this);

  m_currTaskNo = 0;
  m_currTask      = NULL;
  m_currContext   = NULL;
  m_currGrpNo = 0;
  m_currExpNo = 0;
 
  m_memo = NULL;
  m_cascadesTask = NULL;

  m_hold          = TRUE;
  m_trackGridCell = FALSE;
  m_memoGridHasElem = FALSE;

  m_initialGroups = 0;
  m_oldGroups     = 0;
  m_oldTasks      = 0;   

  m_stopTaskNo    = -1;  // stops at every task until reset
  m_stopGroupNo   = -1;
  m_stopExprNo    = -1;

  //m_stopExpr      = NULL;
  //m_stopPlan      = NULL;

  m_bStopNextTask = FALSE;
  m_bStopTaskNo = FALSE;
  m_bStopNextGroup = FALSE;
  m_bStopExprOfGrp = FALSE;

  m_dirtyList.clear();
  m_taskList.clear();
  m_contextList.clear();
  m_groupList.clear();
  m_groupSize.clear(); 
  
  VIEWPORT_MAXROWS = 20;
  VIEWPORT_MAXCOLS = 50;

  RANGE_LOW = -1;
  RANGE_HIGH = 32767;
  RANGE_HIGH_TASKS = 2147483647;
  
  ui->memoWidget->setMouseTracking(true);
  ui->contextWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui->contextWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui->taskWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui->taskWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

QueryMemoView::~QueryMemoView()
{
  delete ui;
}

void QueryMemoView::ResetMemoMembers() 
{
  //------------------------------------------------------------------
  // GSH : Here we clear all components of the memo view in
  // preparation for displaying the memo for the next query. The 
  // components include the memo grid, task list, context list, and 
  // all the spin controls, In this procedure we also reset all the 
  // the member variables of CSqldbgMView thus preparing for the 
  // memo display for next query.
  //------------------------------------------------------------------

  //------------------------------------------------------------------
  // GSH : Reset the memo grid.
  //------------------------------------------------------------------
  if (m_memoGridHasElem) 
     ClearMemoGrid();
  
  //------------------------------------------------------------------
  // GSH : Reset the task list and context list.
  //------------------------------------------------------------------
  FreeTaskMemory();
  FreeContextMemory();
  
  //------------------------------------------------------------------
  // GSH : Now reset the Spin Controls.
  //------------------------------------------------------------------
  ui->group_spin->setValue(RANGE_LOW);
  ui->expr_spin->setValue(RANGE_LOW);
  ui->task_spin->setValue(RANGE_LOW);
  //ui->step_spin->setValue(RANGE_LOW);
  
  //------------------------------------------------------------------
  // GSH : Now reset the member variables.
  //------------------------------------------------------------------

  m_currTaskNo = 0;
  m_memo = NULL;
  m_cascadesTask = NULL;
  m_currTask  = NULL;
  m_currContext = NULL;
  m_hold      = TRUE;
  m_trackGridCell   = FALSE;
  m_memoGridHasElem = FALSE;

  m_initialGroups = 0;
  m_oldGroups     = 0;
  m_oldTasks      = 0;
  
  m_stopTaskNo    = -1;  // stops at every task until reset
  m_stopGroupNo   = -1;
  m_stopExprNo    = -1;

  //m_stopExpr      = NULL;
  //m_stopPlan      = NULL;

  m_bStopNextTask = FALSE;
  m_bStopTaskNo = FALSE;
  m_bStopNextGroup = FALSE;  
  m_bStopExprOfGrp = FALSE;

  m_taskList.clear();
  m_contextList.clear();
  m_groupList.clear();
  m_groupSize.clear();

}

void QueryMemoView::SetMemoData(void *memoData)
{
  m_memo = (CascadesMemo *) memoData;
}

// make Grp No and Exp No consistent with the plan in the document.
void QueryMemoView::syncToDocument() 
{
  // make Grp No and Exp No consistent with the plan in the document.
  m_memo =  (CascadesMemo*)(GlobGuiMainWindow->m_querydata->GetMemo());
  void *plan = NULL;
  void *query = NULL;
  GlobGuiMainWindow->m_querydata->GetData(&query, &plan);
  if(plan)
    syncGrpNoExpNoToPlan((CascadesPlan *)plan);
}

void QueryMemoView::syncGrpNoExpNoToPlan(CascadesPlan *targetPlan)
{
  m_currContext = targetPlan->getContext();
  RelExpr *pExpr = targetPlan->getPhysicalExpr();
  m_currGrpNo = pExpr->getGroupId();
  if(m_memo) {
    CascadesGroup *group = (*m_memo)[m_currGrpNo];
    RelExpr *expr = group->getFirstLogExpr(); 
    Int32 exprNo = 0;
    while(expr && expr->isLogical()) {
      expr = expr->getNextInGroup();
      exprNo++; 
    }
    expr = group->getFirstPhysExpr();
    while(expr && expr->isPhysical()) {
      expr = expr->getNextInGroup();
      exprNo++; 
    }
    for(Int32 planNo=0; planNo < (Int32)group->getCountOfPlans(); planNo++){
      CascadesPlan *plan = group->getPlans()[planNo];
      if(plan == targetPlan){
	m_currExpNo = exprNo;
	return;
      }
      exprNo++;
    } 
  }
}

NABoolean QueryMemoView::NeedToStop(Int32 passNo, Int32 groupNo, Int32 currentTaskNo, CascadesTask *task, ExprNode *expr, CascadesPlan *plan)
{
  // this call has the side effect of setting the current task number
  m_currTaskNo = currentTaskNo;
  m_currTask = task;
  m_currPassNo = passNo;
  m_currGrpNo = groupNo;
  calcCurrExprNo(expr, plan);
   
  if ( m_hold                                                  OR
       m_bStopNextTask                                         OR
       (m_bStopTaskNo AND (m_stopTaskNo == currentTaskNo))     OR
       (m_bStopExprOfGrp AND m_stopExprNo != -1 AND (m_stopGroupNo == groupNo) AND (m_stopExprNo == m_currExpNo)) OR
       (m_bStopExprOfGrp AND m_stopExprNo == -1 AND (m_stopGroupNo == groupNo))   OR
       (m_bStopTask AND (m_stopTask == task))
     )
  {
       logprint("m_stopTask:%p => task:%p\n", m_stopTask, task);
       m_hold = TRUE;
       m_memo =  (CascadesMemo*)(GlobGuiMainWindow->m_querydata->GetMemo());
       m_cascadesTask = (CascadesTaskList*)(GlobGuiMainWindow->m_querydata->GetTaskList());
       //------------------------------------------------------------------
       // GSH : First we need to update the memo view with the latest memo
       // and task list.
       //------------------------------------------------------------------
       m_analysis = (QueryAnalysis*)(GlobGuiMainWindow->m_querydata->GetAnalysis());

       UpdateGroupList();

       UpdateView();

       if (task != NULL AND task->getPlan() != NULL)
	 setDisplayExpr(NULL, task->getPlan());
       else if (task != NULL AND task->getExpr() != NULL)
	 setDisplayExpr(task->getExpr()->castToRelExpr(), NULL);
  }
  return m_hold;
}

/*
 * Keep m_groupList and exprNo in each group sync with memo
 * */
void QueryMemoView::UpdateGroupList()
{
  // bring the list of groups up to date
  // add new group opointer to m_groupList if a new group added in memo
  while ((CollIndex)m_memo->getCountOfUsedGroups() > m_groupList.size())
  {
      m_groupList.append((*m_memo)[(Int32) m_groupList.size()]);
      m_groupSize.append(0);
  }
  // bring the expression counts up to date
  // keep expr num in each group sync
  for (Int32 grno = 0; (CollIndex)grno < m_groupSize.size(); grno++)
  {
      m_groupSize[grno] = NumExprsInGroup(grno);
  }
}

void QueryMemoView::UpdateView()
{
  calcCurrContext();
  updateSelfFully();
}

void QueryMemoView::calcCurrContext()
{
  CascadesPlan *plan = calcCurrPlan();
  if(plan)
    m_currContext = plan->getContext();
}

void QueryMemoView::calcCurrExprNo(ExprNode *expr, CascadesPlan *plan)
{
  //calculate m_currExpNo from expr or plan passed to NeedToStop
  if(NULL == m_memo)
    return;
  Int32 exprCount = 0;
  if(NULL == expr && NULL != plan)
  {
    CascadesGroup *_group = (*m_memo)[m_currGrpNo];
    exprCount += _group->getCountOfLogicalExpr();
    exprCount += _group->getCountOfPhysicalExpr();
    for(Int32 planNo=0; planNo < (Int32)_group->getCountOfPlans(); planNo++)
    {
      CascadesPlan * _plan = _group->getPlans()[planNo];
      exprCount++;
      if(NULL != _plan && _plan == plan) goto L;
    }
  }//if(NULL == expr && NULL != plan)
  else if(NULL != expr && NULL == plan)
  {
    CascadesGroup *_group = (*m_memo)[m_currGrpNo];
    RelExpr *_expr = _group->getFirstLogExpr();
    while(_expr && _expr->isLogical())
    {
      exprCount++;
      if(_expr == expr) goto L;
      _expr = _expr->getNextInGroup();
    }
    _expr = _group->getFirstPhysExpr();
    while(_expr && _expr->isPhysical()) {
      exprCount++;
      if(_expr == expr) goto L;
      _expr = _expr->getNextInGroup();
    }
  }//else if
L:
  m_currExpNo = exprCount - 1;
}

void QueryMemoView::updateSelfFully()
{
  m_trackGridCell = FALSE;
  DrawMemo();	
  UpdateTaskList();
  UpdateSpinControls();
  UpdateContextList();
  m_trackGridCell = TRUE;
}

void QueryMemoView::clearDirtyGrids()
{
  for(UInt32 i = 0; i< m_dirtyList.size(); i++){
    struct DirtyGrid *grid = m_dirtyList[i];
    Int32 row = grid->m_row;
    Int32 col = grid->m_col;
    QIcon icon = grid->m_pict;
    QTableWidgetItem * cell = ui->memoWidget->item(row, col);
    if(cell)
      cell->setIcon(icon);
    delete grid;
  }
  m_dirtyList.clear();
}

void QueryMemoView::changeMemo()
{
  if(!m_memo || ! m_memoGridHasElem)
    return;
  clearDirtyGrids();
  drawSelectedExpr();
  drawCandidatePlanAndSolution(m_currContext);
}

void QueryMemoView::updateSelf()
{
  m_trackGridCell = FALSE;
  changeMemo();	
  UpdateTaskList();
  UpdateSpinControls();
  UpdateContextList();
  m_trackGridCell = TRUE;
}

void QueryMemoView::UpdateContextList()
{
  //---------------------------------------------------------------------
  // GSH : Don't do anything if we aren't optimizing
  //---------------------------------------------------------------------
  //ui->contextWidget->setColumnCount(5);
  if(m_memo != NULL AND m_memo->getCountOfUsedGroups() != 0)
  {
    //---------------------------------------------------------------------
    // GSH : Delete the existing context list and update it with the new 
    // contexts.
    //---------------------------------------------------------------------
    m_contextList.clear();
    //---------------------------------------------------------------------
    // GSH : Make sure that the group list is up to date.
    //--------------------------------------------------------------------
    if ((CollIndex)m_memo->getCountOfUsedGroups() > m_groupList.size())
      UpdateGroupList();
#if 1
    Context *currContext;
    for (Int32 i=0; i<m_groupList[m_currGrpNo]->getCountOfContexts(); i++)
    {
      currContext = m_groupList[m_currGrpNo]->getContext(i);
      m_contextList.insert(0,currContext);
    }
#endif
#if 0
    Context *currContext;
    for (Int32 i=0; i<(*m_memo)[m_currGrpNo]->getCountOfContexts(); i++)
    { 
      currContext = (*m_memo)[m_currGrpNo]->getContext(i);
      m_contextList->insertAt(0,currContext);
    }
#endif
    //--------------------------------------------------------------------
    // GSH : Now m_contextList has all the context that we need to display   
    // in the m_context list control. So go ahead and display. First you have
    // to delete all the existing items in the m_context list control.
    //--------------------------------------------------------------------
    FreeContextMemory();
    displayContextList();
  }
  // adjust Context table size
  QStringList header;
  header << "Group" << "Cost Limit" << "Logical Props" << "Status" << "Required Physical Properties";
  ui->contextWidget->setHorizontalHeaderLabels(header);
  for(int i=0; i<ui->contextWidget->columnCount(); i++){
    ui->contextWidget->resizeColumnToContents(i);
    int width = ui->contextWidget->columnWidth (i);
    ui->contextWidget->setColumnWidth (i,width + 30);
  }
}

void QueryMemoView::displayContextList()
{
  // clear current context if we are switching groups
  if (m_currContext &&
      m_currContext->getGroupId() != (CascadesGroupId)m_currGrpNo)
  {
      m_currContext = NULL;
  }

  ui->contextWidget->setRowCount(m_contextList.size());  
  for (Int32 i = 0; (CollIndex)i < m_contextList.size(); i ++) 
  {
    #if 0
    CONTEXTDETAILS* pContextDetails;
    pContextDetails = new CONTEXTDETAILS; 
    pContextDetails->thisContext = m_contextList[i];
    pContextDetails->costText = QString(m_contextList[i]->getCostString().data());
    pContextDetails->ilpText  = QString(m_contextList[i]->getILPString().data());
    pContextDetails->statText = QString(m_contextList[i]->getStatusString().data());
    pContextDetails->rppText  = QString(m_contextList[i]->getRPPString().data());
    pContextDetails->groupNo  = m_currGrpNo;
    #endif
    //highlight the current context
    QIcon contextIcon;
    if(m_currContext == m_contextList[i])
      contextIcon = QIcon(":/file/Resource/Main/Context_green.bmp");
    else
      contextIcon = QIcon(":/file/Resource/Main/Context.bmp");
    //add one row
    //save context pointer to cell
    QTableWidgetItem *pItem0 = new QTableWidgetItem(contextIcon,QString::number(m_currGrpNo));
    QVariant itemData = qVariantFromValue((void*)(m_contextList[i]));
    pItem0->setData(Qt::UserRole, itemData);
    QTableWidgetItem *pItem1 = new QTableWidgetItem(QString(m_contextList[i]->getCostString().data()));
    QTableWidgetItem *pItem2 = new QTableWidgetItem(QString(m_contextList[i]->getILPString().data()));
    QTableWidgetItem *pItem3 = new QTableWidgetItem(QString(m_contextList[i]->getStatusString().data()));
    QTableWidgetItem *pItem4 = new QTableWidgetItem(QString(m_contextList[i]->getRPPString().data()));
    ui->contextWidget->setItem (i, 0, pItem0);
    ui->contextWidget->setItem (i, 1, pItem1);
    ui->contextWidget->setItem (i, 2, pItem2);
    ui->contextWidget->setItem (i, 3, pItem3);
    ui->contextWidget->setItem (i, 4, pItem4);
  }//for (Int32 i = 0; (CollIndex)i < m_contextList.size(); i ++)
}

void QueryMemoView::FreeContextMemory()
{
  ui->contextWidget->clear(); 
}

void QueryMemoView::UpdateSpinControls()
{

  ui->group_spin->setValue(m_currGrpNo);
  ui->expr_spin->setValue(m_currExpNo);
  ui->task_spin->setValue(m_currTaskNo);
}

void QueryMemoView::UpdateTaskList()
{
  //---------------------------------------------------------------------
  // GSH : Get the list of Cascades Task list from the docmument and 
  // display the text describing each task in the m_task List control of
  // the Memo View.
  //---------------------------------------------------------------------
  m_cascadesTask = (CascadesTaskList*)(GlobGuiMainWindow->m_querydata->GetTaskList());
  //---------------------------------------------------------------------
  // GSH : Don't do anything if we aren't optimizing
  //---------------------------------------------------------------------
  if (m_memo != NULL AND 
      m_memo->getCountOfUsedGroups() != 0 AND
      m_cascadesTask != NULL)
  {
    //---------------------------------------------------------------------
    // GSH : Delete the existing task list and update it with the new tasks.
    //---------------------------------------------------------------------
    m_taskList.clear();
    //--------------------------------------------------------------------
    // GSH : Populate m_taskList with tasks in docmument tasklist
    //--------------------------------------------------------------------
    CascadesTask *currTask = m_cascadesTask->getFirst();
    while (currTask != NULL)  
    {
      m_taskList.insert(0,currTask);
      currTask = m_cascadesTask->getNext(currTask);
    }
    //--------------------------------------------------------------------
    // GSH : Now m_taskList has all the task that we need to display in  
    // the m_task list control. So go ahead and display. First you have
    // to delete all the existing items in the m_task list control.
    //--------------------------------------------------------------------
    FreeTaskMemory();
    displayTaskList();
  }
  //adjust task view size
  QStringList header;
  header << "Tasks"<<" ";
  ui->taskWidget->setHorizontalHeaderLabels(header);
  for(int i=0;i<ui->taskWidget->columnCount();i++){
    ui->taskWidget->resizeColumnToContents(i);
    int width = ui->taskWidget->columnWidth (i);
    ui->taskWidget->setColumnWidth (i,width + 30);
  }
  /*
 * taskWidget only need one column, 
 * the column just to make scrollbar appear.
 * */ 
  ui->taskWidget->setColumnWidth(1,1);
}

void QueryMemoView::FreeTaskMemory()
{
  ui->taskWidget->clear();
}

void QueryMemoView::displayTaskList()
{
  ui->taskWidget->setRowCount(m_taskList.size()); 
  for (Int32 i = 0; (CollIndex)i < m_taskList.size(); i ++) 
  {
      QTableWidgetItem *pItem = new QTableWidgetItem();
      pItem->setIcon(QIcon(":/file/Resource/Main/Taskdetails.bmp"));
      pItem->setText(QString(m_taskList[i]->taskText().data()));
      QVariant itemData = qVariantFromValue((void*)(m_taskList[i]));
      pItem->setData(Qt::UserRole, itemData);
      ui->taskWidget->setItem (m_taskList.size()-1-i, 0, pItem);
  }
}

Int32 QueryMemoView::NumExprsInGroup(Int32 grno)
{
  Int32 result = 0;
  
  if (m_memo != NULL AND
      grno   >= 0    AND
      (Lng32)grno < m_memo->getCountOfUsedGroups())
    {
      RelExpr *e = (*m_memo)[grno]->getFirstLogExpr();
      while (e != NULL)
	  {
	   result++;
	   e = e->getNextInGroup();
	  }

      e = (*m_memo)[grno]->getFirstPhysExpr();

      while (e != NULL)
	  {
	   result++;
	   e = e->getNextInGroup();
	  }

      CollIndex numplans = (*m_memo)[grno]->getCountOfPlans();

      if (numplans > 0)
	     result += ((Int32)numplans);
    }
  return result;
}

CascadesPlan * QueryMemoView::calcCurrPlan()
{
  CascadesPlan *p = NULL;
  if (m_memo != NULL && m_currExpNo > 0) {
    Int32 grno = m_currGrpNo;
    Int32 exprno = m_currExpNo;
    // found the group, now walk the list to expression number "exprno"
    RelExpr *e = (*m_memo)[grno]->getFirstLogExpr();
    while (e != NULL AND exprno > 0)
    {
      e = e->getNextInGroup();
      exprno--;
    }
    
    if (e == NULL)
      e = (*m_memo)[grno]->getFirstPhysExpr();
    
    while (e != NULL AND exprno > 0)
    {
      e = e->getNextInGroup();
      exprno--;
    }

    if (e == NULL)
    {
      Lng32 numPlans = (*m_memo)[grno]->getCountOfPlans();
      if (exprno >= 0 AND exprno < numPlans)
	p = (*m_memo)[grno]->getPlans()[exprno];
    }
  }//if(m_memo != NULL && m_currExpNo > 0)
  return p;
}

void QueryMemoView::setDisplayExpr(RelExpr* expr, CascadesPlan* plan)
{
  // need to update m_currGrpNo & m_currExpNo
  GlobGuiMainWindow->m_querydata->SetData(expr, plan);
  //GlobGuiMainWindow->UpdateAllViews();
  GlobGuiMainWindow->UpdateQueryTreeView();
}

void QueryMemoView::DrawMemo(void)
{ 	
  m_memo = (CascadesMemo*)(GlobGuiMainWindow->m_querydata->GetMemo());
  if(!m_memo){
    m_currGrpNo = 0;
    m_currExpNo = 0;
  }
  setMemoMaxRowsCols();
  InitMemoDisplay(22, 22);

  if (m_memoGridHasElem) 
    ClearMemoGrid();

  // now draw them
  if (m_memo != NULL)
  { 
    CascadesGroupId maxgroups = m_memo->getCountOfUsedGroups();

    if (m_initialGroups == 0)
      m_initialGroups = (Int32)maxgroups;
    //draw all the expressions and plans in memo
    for (CascadesGroupId grno=0; grno<maxgroups; grno++) 
    {
      DrawMemoGroup((Int32) grno);  
    }
    
    drawSelectedExpr();
    drawCandidatePlanAndSolution(m_currContext);

    m_memoGridHasElem = TRUE;
    m_oldGroups = maxgroups;
  }
}

void QueryMemoView::setMemoMaxRowsCols()
{
  Int32 maxRows, maxCols;
  if(m_memo){
    maxRows = m_memo->getCountOfUsedGroups();
    maxCols = 0; 
    for(Int32 grp = 0; grp < maxRows; grp++){
      Int32 numExpr = NumExprsInGroup(grp);
      if(maxCols < numExpr)
	maxCols = numExpr;
    }
    maxCols++;
  }
  else{
    maxRows = VIEWPORT_MAXROWS; 
    maxCols = VIEWPORT_MAXCOLS;
  }

  ui->memoWidget->setRowCount(maxRows);
  ui->memoWidget->setColumnCount(maxCols);
} 
/*
 * set every grid cell width and height
 * */
void QueryMemoView::InitMemoDisplay(int width, int height)
{
  Int32 row, col;
  for (row=0;row<ui->memoWidget->rowCount();row++)
  {
    ui->memoWidget->setRowHeight(row, width);
  }
  for (col=0;col<ui->memoWidget->columnCount();col++)
  {
    ui->memoWidget->setColumnWidth(col, height);
  }
}

void QueryMemoView::ClearMemoGrid()
{
  ui->memoWidget->clear(); 
  m_memoGridHasElem = FALSE;
}

void QueryMemoView::DrawMemoGroup(Int32 grno)
{
  if (m_memo == NULL OR grno < 0)
    return;
  QTableWidgetItem *pItem = new QTableWidgetItem();
  // ---------------------------------------------------------------------
  // display the icon (colored rectangle) for the group
  // ---------------------------------------------------------------------  
  if ((*m_memo)[grno]->getGroupId() != (CollIndex)grno)
    // this group is merged and no longer exists
    pItem->setIcon(QIcon(":/file/Resource/Main/Merggrp.bmp"));
  else if (grno < m_initialGroups)
    // this group was allocated as part of the original query
    pItem->setIcon(QIcon(":/file/Resource/Main/Initgrp.bmp"));
  else if (grno < m_oldGroups)
    //this group was added during optimization, but before the last step
    pItem->setIcon(QIcon(":/file/Resource/Main/Addngrp.bmp"));
  else
    // this group was added during the last step
    pItem->setIcon(QIcon(":/file/Resource/Main/Newgrp.bmp"));	
  //-------------------------------------------------------------
  // GSH : This is one of the places where we do not use the member
  // function SetGridCell to set the appropriate Grid row and 
  // column. This is to draw the icon representing the group.
  //-------------------------------------------------------------
  ui->memoWidget->setItem (grno, 0, pItem);
  DrawExprInMemoGroup(grno);
}

void QueryMemoView::DrawExprInMemoGroup(Int32 grno)
{
  Int32 exprno = 0;
  DrawLogExpr(grno,exprno);
  DrawPhyExpr(grno,exprno);
  DrawPlans(grno,exprno);
}

void QueryMemoView::DrawLogExpr(Int32 grno, Int32& exprno)
{
  CascadesGroup *group = (*m_memo)[grno];
  RelExpr   *expr     = NULL;

  expr = group->getFirstLogExpr();
  if (expr == NULL) 
    return;
  
  while ((expr != NULL) && expr->isLogical()) 
  {
    QTableWidgetItem *pItem = new QTableWidgetItem();
    pItem->setIcon(QIcon(":/file/Resource/Main/Logexp.bmp"));
    pItem->setToolTip (QString("%1,%2").arg(grno).arg(exprno)); 
    ui->memoWidget->setItem (grno, exprno + 1, pItem);
    exprno++;      // Increment exprno for next expression in group
    expr = expr->getNextInGroup(); // get next expression
  }
}

void QueryMemoView::DrawPhyExpr(Int32 grno, Int32& exprno)
{
  CascadesGroup *group = (*m_memo)[grno];
  RelExpr   *expr     = NULL;
  
  expr = group->getFirstPhysExpr();
  if (expr == NULL) 
    return;

  while (expr != NULL && expr->isPhysical()) 
  {
    QTableWidgetItem *pItem = new QTableWidgetItem();
    pItem->setIcon(QIcon(":/file/Resource/Main/Phyexp.bmp"));
    pItem->setToolTip (QString("%1,%2").arg(grno).arg(exprno));
    ui->memoWidget->setItem (grno, exprno + 1, pItem);
    exprno++; // Increment col for next expression in group
    expr = expr->getNextInGroup(); // get next expression
  }
}

void QueryMemoView::DrawPlans(Int32 grno, Int32& exprno)
{
  CascadesGroup *group = (*m_memo)[grno];
  for(Int32 planNo=0; planNo < (Int32)group->getCountOfPlans(); planNo++){
    CascadesPlan *plan = group->getPlans()[planNo];
    RelExpr *expr = plan->getPhysicalExpr();
    if(expr) {
      QTableWidgetItem *pItem = new QTableWidgetItem();
      pItem->setIcon(QIcon(":/file/Resource/Main/Plan.bmp"));
      pItem->setToolTip(QString("%1,%2").arg(grno).arg(exprno));
      ui->memoWidget->setItem (grno, exprno + 1, pItem);
      exprno++; 
    }
  } //end for
}

/*selected expression is current expression in current group*/
void QueryMemoView::drawSelectedExpr()
{
  Int32 grno = m_currGrpNo;
  Int32 exprno = m_currExpNo;
  //Scroll to current expression
  ui->memoWidget->scrollTo(ui->memoWidget->model()->index(m_currGrpNo, m_currExpNo>0?m_currExpNo:0), QAbstractItemView::EnsureVisible);

  if(-1 == m_currExpNo) //-1 == m_currExpNo means there is no selected Expr
    return;
  //find the selected expression via m_currGrpNo and m_currExpNo
  RelExpr *e = (*m_memo)[grno]->getFirstLogExpr();
  while (e != NULL AND exprno > 0)
  { 
    e = e->getNextInGroup(); 
    exprno--; 
  }
  if(e) { //exprno fall in LogExpr
    QTableWidgetItem *pItem = ui->memoWidget->item(m_currGrpNo, m_currExpNo+1);
    pItem->setIcon(QIcon(":/file/Resource/Main/SelLogExp.bmp"));
    struct DirtyGrid *grid = new DirtyGrid(m_currGrpNo,
					   m_currExpNo+1,
					   QIcon(":/file/Resource/Main/Logexp.bmp"));
    m_dirtyList.insert(0, grid);
    return;
  }
  //out of range of LogExpr find in PhysExpr
  e = (*m_memo)[grno]->getFirstPhysExpr();
  while (e != NULL AND exprno > 0)
  {
    e = e->getNextInGroup();
    exprno--;
  }
  if(e){//exprno fall in PhysExpr
    QTableWidgetItem *pItem = ui->memoWidget->item(m_currGrpNo, m_currExpNo+1);
    pItem->setIcon(QIcon(":/file/Resource/Main/SelPhyExp.bmp"));
    struct DirtyGrid *grid = new DirtyGrid(m_currGrpNo,
					   m_currExpNo+1,
					   QIcon(":/file/Resource/Main/Phyexp.bmp"));
    m_dirtyList.insert(0, grid);
    return;
  }
  //find expr in Plan range
  Lng32 numPlans = (*m_memo)[grno]->getCountOfPlans();
  if (exprno >= 0 AND exprno < numPlans){
    QTableWidgetItem *pItem = ui->memoWidget->item(m_currGrpNo, m_currExpNo+1);
    pItem->setIcon(QIcon(":/file/Resource/Main/SelPlan.bmp"));
    struct DirtyGrid *grid = new DirtyGrid(m_currGrpNo,
					   m_currExpNo+1,
					   QIcon(":/file/Resource/Main/Plan.bmp"));
    m_dirtyList.insert(0, grid);
  }
}

/*mark the plan of current group if it is a Solution or Candidate*/
void QueryMemoView::drawCandidatePlanAndSolution(Context* context)
{
  if(!context)
    return;
  Int32 exprno = 0;
  Int32 grno = m_currGrpNo;
  RelExpr *e = (*m_memo)[grno]->getFirstLogExpr();
    
  while (e)
  {
    e = e->getNextInGroup();
    exprno++;
  }
  e = (*m_memo)[grno]->getFirstPhysExpr();
  while (e)
  {
    e = e->getNextInGroup();
    exprno++;
  }
  CascadesGroup *group = (*m_memo)[grno];
  for(Int32 planNo=0; planNo < (Int32)group->getCountOfPlans(); planNo++){
    CascadesPlan *plan = group->getPlans()[planNo];
    RelExpr *expr = plan->getPhysicalExpr();
    if(expr){
      if (context->getSolution() == plan) {
	
	QTableWidgetItem *pItem = ui->memoWidget->item(grno, exprno+1);
        pItem->setIcon(QIcon(":/file/Resource/Main/Solution.bmp"));
	struct DirtyGrid *grid = new DirtyGrid(grno,
					       exprno+1,
					       QIcon(":/file/Resource/Main/Plan.bmp"));
	m_dirtyList.insert(0, grid);
      }
      else if (context->isACandidate(plan)){
        
        QTableWidgetItem *pItem = ui->memoWidget->item(grno, exprno+1);
        pItem->setIcon(QIcon(":/file/Resource/Main/CandPlan.bmp"));
        struct DirtyGrid *grid = new DirtyGrid(grno,
					       exprno+1,
					       QIcon(":/file/Resource/Main/Plan.bmp"));
	m_dirtyList.insert(0, grid);
      }
      exprno++; 
    }//if(expr)
  } //for
}

void QueryMemoView::InitTaskList()
{
  ui->taskWidget->setColumnCount(2);
  ui->taskWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
}

void QueryMemoView::InitContextList()
{
  ui->contextWidget->setColumnCount(5);
  ui->contextWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
}

void QueryMemoView::InitSpinControls()
{
  unsigned long max_range = RANGE_HIGH_TASKS > RANGE_HIGH ? RANGE_HIGH_TASKS : RANGE_HIGH;
  ui->group_spin->setRange(RANGE_LOW, max_range);
  ui->expr_spin->setRange(RANGE_LOW, max_range);
  ui->task_spin->setRange(RANGE_LOW, max_range);
}

void QueryMemoView::StopAtExpr(Int32 grno, Int32 exprno, NABoolean enabled)
{
  m_bStopExprOfGrp = enabled;
  m_stopGroupNo = grno;
  m_stopExprNo = exprno;
}

void QueryMemoView::turnOffMemoTrack()
{
  m_trackGridCell = FALSE;
}

void QueryMemoView::OnMemoStepOneTask()
{
  ResetMemoStops();
  StopAtNextTask(TRUE);
  GlobGuiMainWindow->IsBackToSqlCompiler_ = true;
  turnOffMemoTrack();
}

void QueryMemoView::OnMemoSteptasknum()
{
    //validation check
    if(ui->task_spin->value() <= m_currTaskNo)
    {
      QMessageBox::warning(this, "Warning", "Stop task number should be greater than current task number");
      return;
    }
    ResetMemoStops();
    StopAtTaskNo(ui->task_spin->value(), TRUE);		
    GlobGuiMainWindow->IsBackToSqlCompiler_ = true;
}

void QueryMemoView::OnMemoStepTask()
{
    QList<QTableWidgetItem *> selTaskList = ui->taskWidget->selectedItems();
    if(selTaskList.count() >= 1)
    {
       QVariant v = selTaskList[0]->data(Qt::UserRole);
       void * task = v.value < void * >();
       if (NULL == task)
         return;
       ResetMemoStops();
       StopAtTask(task, TRUE);
       GlobGuiMainWindow->IsBackToSqlCompiler_ = true;
    }
}

void QueryMemoView::OnMemoStepgrp() 
{
  ResetMemoStops();
  StopAtExpr(ui->group_spin->value(), -1, TRUE);	
  GlobGuiMainWindow->IsBackToSqlCompiler_ = true;
}

void QueryMemoView::OnMemoStepexpr() 
{
  ResetMemoStops();
  StopAtExpr(ui->group_spin->value(), ui->expr_spin->value(), TRUE);		
  GlobGuiMainWindow->IsBackToSqlCompiler_ = true;
}

void QueryMemoView::OnMemoFinish() 
{
  ResetMemoStops();
  GlobGuiMainWindow->IsBackToSqlCompiler_ = true;
  turnOffMemoTrack();
}

void QueryMemoView::OnMemoFinishpass() 
{
  ResetMemoStops();
  GlobGuiMainWindow->IsBackToSqlCompiler_ = true;
  turnOffMemoTrack();
}

void QueryMemoView::memoWidgetCell(int row, int col)
{
  // TODO: Add your control notification handler code here
  if(!m_trackGridCell||!m_memo)
    return;

  if(row == m_currGrpNo && col-1 == m_currExpNo)
    return;

  if(CURRSTMT_OPTGLOBALS->memo) //memo is context global
    m_currGrpNo = row < m_memo->getCountOfUsedGroups() ? row : m_memo->getCountOfUsedGroups();
  else
    m_currGrpNo = row;

  m_currExpNo = (col-1) > 0 ? (col-1) : 0;
  calcCurrContext();
  updateSelf();
  displayCurrExpr();	
}

void QueryMemoView::on_memoWidget_currentCellChanged( int currentRow, int currentColumn, int previousRow, int previousColumn )
{
  memoWidgetCell(currentRow, currentColumn);
}

void QueryMemoView::on_memoWidget_cellClicked(int row, int col)
{
  memoWidgetCell(row, col);
}

void QueryMemoView::contextWidgetCell(int row, int col)
{
  //retreive previously saved context pointer
  QList<QTableWidgetItem *> selContextList = ui->contextWidget->selectedItems();
  if(selContextList.count() >= 1)
  {
       QVariant v = selContextList[0]->data(Qt::UserRole);
       Context* context = (Context*) v.value < void * >();
       if (NULL == context)
         return;
       //now, if any, we draw the plan and solution of the context
       clearDirtyGrids();
       drawCandidatePlanAndSolution(context);
  }
}

void QueryMemoView::on_contextWidget_cellClicked (int row, int column)
{
  contextWidgetCell(row, column);
}

/*calc & display expr and plan from current grno and ExprNo*/
void QueryMemoView::displayCurrExpr()
{
  if (m_memo != NULL) {
    Int32 grno = m_currGrpNo;
    Int32 exprno = m_currExpNo;
    // found the group, now walk the list to expression number "exprno"
    RelExpr *e = (*m_memo)[grno]->getFirstLogExpr();
    
    while (e != NULL AND exprno > 0)
      {
	e = e->getNextInGroup();
	exprno--;
      }
    
    if (e == NULL)
      e = (*m_memo)[grno]->getFirstPhysExpr();
    
    while (e != NULL AND exprno > 0)
    {
	e = e->getNextInGroup();
	exprno--;
    }

    CascadesPlan *p = NULL;
    if (e == NULL)
    {
	Lng32 numPlans = (*m_memo)[grno]->getCountOfPlans();
	if (exprno >= 0 AND exprno < numPlans)
	{
	    p = (*m_memo)[grno]->getPlans()[exprno];
	    e = NULL;
	}
    }
    //set tree root of query viewer
    GlobGuiMainWindow->m_querydata->SetData(e, p);
    GlobGuiMainWindow->UpdateQueryTreeView();
  }
}
