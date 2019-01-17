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
#include <typeinfo>
#include "TCBTreeView.h"
#include "ExeSchedWindow.h"
#include "ExeQueueDisplay.h"
#include "ui_TCBTreeView.h"
#include "CommonSqlCmpDbg.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExScheduler.h"

TCBTreeView::TCBTreeView(ExeSchedWindow *mainExeWindow,
                         QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TCBTreeView),
    mainExeWindow_(mainExeWindow)
{
    currTcb_ = NULL;
    ui->setupUi(this);
    QStringList header;
    header << "TCB/Task Tree" << "Break" << "Scheduled" << "Down" << "Up";
    QTreeWidgetItem *hdrItem = new QTreeWidgetItem(header);
    hdrItem->setIcon(0, QIcon(":/file/Resource/Main/Sqlnode.bmp"));
    ui->treeWidget->setHeaderItem(hdrItem);
    ui->treeWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidget->setIconSize(QSize(32, 32));
    ui->treeWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->treeWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

TCBTreeView::~TCBTreeView()
{
    delete ui;
}

void TCBTreeView::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
  QVariant itemData = item->data(0, Qt::UserRole);
  TcbTreeDetails *itemDetails = itemData.value <TcbTreeDetails *>();

  switch (column)
    {
    case 0:
      if (itemDetails->currTask)
        {
          // clicking on a task name:
          // set the next task to be executed to this one
          ExSubtask **subtaskPtr = mainExeWindow_->getSubtaskPtr();

          if (subtaskPtr && *subtaskPtr)
            *subtaskPtr = itemDetails->currTask;
        }
      break;

    case 1:
      if (itemDetails->currTask)
        {
          // clicking on a breakpoint cell of a task:
          // toggle the breakpoint between on/off
          int brkpt = itemDetails->currTask->getBreakPoint();

          if (brkpt)
            brkpt = 0;
          else
            brkpt = 1;
          itemDetails->currTask->setBreakPoint(brkpt);
        }
      break;

    case 2:
      if (itemDetails->currTask)
        // clicking on the "scheduled" field of a task: schedule
        itemDetails->currTask->schedule();
      break;

    case 3:
      if (itemDetails->currTask == NULL)
        {
          // clicking on the number of rows in the down queue:
          if (!itemDetails->currDnQueueDisplay)
            {
              itemDetails->currDnQueueDisplay = new ExeQueueDisplay(
                   itemDetails->currTcb,
                   false,
                   this);
              itemDetails->currDnQueueDisplay->show();
            }
        }
      break;

    case 4:
      if (itemDetails->currTask == NULL)
        {
          // clicking on the number of rows in the up queue:
          if (!itemDetails->currUpQueueDisplay)
            {
              itemDetails->currUpQueueDisplay = new ExeQueueDisplay(
                   itemDetails->currTcb,
                   true,
                   this);
              itemDetails->currUpQueueDisplay->show();
            }
        }
      break;

    default:
      break;
    }

  if (itemDetails->currTask)
    setBreakSchedInfo(item,
                      (itemDetails->currTask->getBreakPoint() > 0 ? 1 : 0),
                      (itemDetails->currTask->isScheduled() ? "*" : ""));
}

void TCBTreeView::UpdateView()
{
    //------------------------------------------------------------------
    // Delete the existing Tree.
    //------------------------------------------------------------------
}

void TCBTreeView::displayTcbTree(const ex_tcb *tcb,
                                 ExSubtask *currSubtask,
                                 ExScheduler *sched,
                                 bool includeTasks,
                                 QTreeWidgetItem * parentTreeItem)
{
    if (parentTreeItem == NULL)
      {
        // for the top-level caller, start with the fragment root TCB
        currTcb_ = tcb;
        tcb = sched->getLocalRootTcbForGui();
      }

    if (tcb != NULL)
      {
        const ComTdb *tdb = tcb->getTdb();
        int arity = tcb->numChildren();
        Int32 bitmapIndex;
        QString nodeText;
        const char *breakText = " ";
        const char *scheduledText = " ";
        TcbTreeDetails *itemDetails = new TcbTreeDetails(tcb, sched);

        getOperatorImageText(tdb, bitmapIndex, nodeText);

        QStringList rowValues;
        ex_queue_pair parentQueue = tcb->getParentQueue();
        rowValues.append(nodeText);
        QTreeWidgetItem *tcbTreeItem = new QTreeWidgetItem(rowValues);

        if (parentQueue.down)
          {
            tcbTreeItem->setText(3, QString("%1").arg(parentQueue.down->getLength()));
            if (parentQueue.down->isFull())
              tcbTreeItem->setBackgroundColor(3, QColor("mistyrose"));
          }
        if (parentQueue.up)
          {
            tcbTreeItem->setText(4, QString("%1").arg(parentQueue.up->getLength()));
            if (parentQueue.up->isFull())
              tcbTreeItem->setBackgroundColor(4, QColor("mistyrose"));
          }

        tcbTreeItem->setIcon(0, QIcon(":/file/Resource/Main/Sqlnode.bmp"));
        QString tip = QString("%1@0x%2").arg(typeid(*tcb).name()).arg((ulong)tcb, 0, 16);
        tcbTreeItem->setToolTip(0, tip);
        if (tcb == currTcb_ && !includeTasks)
          tcbTreeItem->setBackgroundColor(0, QColor("salmon"));
        if (parentTreeItem == NULL)
          {
            ui->treeWidget->clear();
            ui->treeWidget->addTopLevelItem(tcbTreeItem);
          }
        else
          {
            parentTreeItem->addChild(tcbTreeItem);
          }

        itemDetails->currTcb = tcb;
        itemDetails->currSched = sched;
        itemDetails->currTask = NULL;
        QVariant itemData = qVariantFromValue(itemDetails);
        tcbTreeItem->setData(0, Qt::UserRole, itemData);
        //set column alignment for each row
        tcbTreeItem->setTextAlignment(1, Qt::AlignCenter | Qt::AlignVCenter);
        tcbTreeItem->setTextAlignment(2, Qt::AlignCenter | Qt::AlignVCenter);

        if (nodeText.compare(QString("NULL")))
          {
            tcbTreeItem->setText(1, QString(breakText));
            tcbTreeItem->setText(2, QString(scheduledText));
          }

        displayTcbTasks(tcb, currSubtask, sched, includeTasks, tcbTreeItem);

        for (Int32 i = arity - 1; i >= 0; i--)
          {
            const ex_tcb *childTcb = tcb->getChild(i);

            displayTcbTree(childTcb, currSubtask, sched, includeTasks, tcbTreeItem);
          }
        if (parentTreeItem == NULL)
          {
            // make some final adjustments
            ui->treeWidget->expandAll();
            for (int i = 0; i < ui->treeWidget->columnCount(); i++)
              {
                ui->treeWidget->resizeColumnToContents(i);
                int width = ui->treeWidget->columnWidth(i);
                if (i == 2 && width > 70)
                  // restrict the last column (breakpoints)
                  // to a small size, otherwise the resize
                  // process expands it all the way
                  width = 70;
                ui->treeWidget->setColumnWidth(i, width + 20);
              }
          }
      }  //tdbTree != NULL
}

void TCBTreeView::displayTcbTasks(const ex_tcb *tcb,
                                  ExSubtask *currSubtask,
                                  ExScheduler *sched,
                                  bool expandTasks,
                                  QTreeWidgetItem *tcbTreeItem)
{
   ExSubtask *subtask = sched->getSubtasksForGui();
   int numBreakpoints = 0;
   std::string isScheduled;
   while (subtask)
     {
       if (subtask->getTcb() == tcb)
         {
           if (expandTasks)
             {
               QString nodeTextData = QString(QLatin1String(subtask->getTaskName()));
               QStringList rowValues;
               rowValues.append(nodeTextData);
               QTreeWidgetItem *taskTreeItem = new QTreeWidgetItem(rowValues);
               TcbTreeDetails *itemDetails = new TcbTreeDetails(tcb, sched, subtask);

               taskTreeItem->setIcon(0, QIcon(":/file/Resource/Main/T.bmp"));
               QString tip = QString("%1@0x%2").arg(typeid(*subtask).name()).arg((ulong)subtask, 0, 16);
               taskTreeItem->setToolTip(0, tip);
               tcbTreeItem->addChild(taskTreeItem);

               itemDetails->currTcb = tcb;
               itemDetails->currSched = sched;
               itemDetails->currTask = subtask;
               QVariant itemData = qVariantFromValue(itemDetails);
               taskTreeItem->setData(0, Qt::UserRole, itemData);
               //set column alignment for each row
               taskTreeItem->setTextAlignment(1, Qt::AlignCenter | Qt::AlignVCenter);
               taskTreeItem->setTextAlignment(2, Qt::AlignCenter | Qt::AlignVCenter);

               if (nodeTextData.compare(QString("NULL")))
                 {
                   setBreakSchedInfo(taskTreeItem,
                                     (subtask->getBreakPoint() > 0 ? 1 : 0),
                                     (subtask->isScheduled() ? "*" : ""));
                   if (subtask == currSubtask)
                     taskTreeItem->setBackgroundColor(0, QColor("salmon"));
                 }
             } // expandTasks
           else
             {
               // aggregate breakpoint and scheduled information at the task level
               if (subtask->isScheduled())
                 numBreakpoints++;
               if (subtask->getBreakPoint() > 0)
                 {
                   std::string taskText(subtask->getTaskName());

                   if (taskText.length() == 0 || taskText.at(0) == ' ')
                     taskText = "*";
                   taskText.append(",");
                   if (isScheduled.find(taskText) != std::string::npos)
                     {
                       isScheduled.append(subtask->getTaskName());
                       isScheduled.append(",");
                     }
                 }
             } // don't list tasks in the tree view
         } // task is for the current TCB
       subtask = subtask->getNextForGUI();
     } // loop over all subtasks
   setBreakSchedInfo(tcbTreeItem, numBreakpoints, isScheduled.c_str());
}

void TCBTreeView::getOperatorImageText (const ComTdb *tdb,
                                        Int32 & bitmapIndex,
                                        QString &nodeText)
{
    nodeText = QString("%1 (Id: %2 Ex: %3)").
                arg(tdb->getNodeName()).
                arg(tdb->getTdbId()).
                arg(tdb->getExplainNodeId());
    ComTdb::ex_node_type opType = tdb->getNodeType();
    switch (opType)
      {
      case ComTdb::ex_ROOT:
        bitmapIndex = IDX_ROOT;
        break;
      case ComTdb::ex_ONLJ:
      case ComTdb::ex_MJ:
      case ComTdb::ex_HASHJ:
      case ComTdb::ex_TUPLE_FLOW:
        bitmapIndex = IDX_JOIN;
        break;
      case ComTdb::ex_HBASE_ACCESS:
      case ComTdb::ex_HDFS_SCAN:
        bitmapIndex = IDX_SCAN;
        break;
      default:
        bitmapIndex = IDX_GENERIC;
      }
}

void TCBTreeView::setBreakSchedInfo(QTreeWidgetItem *treeItem,
                                    int numBreakpoints,
                                    const char *schedTasks)
{
    std::string schedText(schedTasks);

    if (numBreakpoints > 0)
      {
        treeItem->setText(1, QString("*"));
        treeItem->setBackgroundColor(1,"orangered");
      }
    else
      {
        treeItem->setText(1, QString(" "));
      }
    if (schedText.length() > 0 && schedText.at(schedText.length()-1) == ',')
      schedText.erase(schedText.length()-1);
    if (schedText.length() > 16)
      schedText = "*";
    treeItem->setText(2, QString(schedText.c_str()));
    if (schedText.length() > 0)
      treeItem->setBackgroundColor(2,"lightblue");
}

