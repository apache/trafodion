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
#ifndef TCBTREEVIEW_H
#define TCBTREEVIEW_H

#include <QtGui>
#include "CommonSqlCmpDbg.h"

namespace Ui {
class TCBTreeView;
}

class ex_tcb;
class ExScheduler;
class ExSubtask;
class ExeQueueDisplay;
class ExeSchedWindow;

struct TcbTreeDetails : public QObject
{
  const ex_tcb *currTcb;
  ExScheduler *currSched;
  ExSubtask *currTask;
  ExeQueueDisplay *currDnQueueDisplay;
  ExeQueueDisplay *currUpQueueDisplay;

  TcbTreeDetails(const ex_tcb *tcb,
                 ExScheduler *sched,
                 ExSubtask *subtask = NULL)
  {
    currTcb = tcb;
    currSched = sched;
    currTask = subtask;
    currDnQueueDisplay = NULL;
    currUpQueueDisplay = NULL;
  }

};

Q_DECLARE_METATYPE (TcbTreeDetails *);	// This is for QTreeWidgetItem.setData()

class TCBTreeView : public QWidget
{
    Q_OBJECT

public:
    explicit TCBTreeView(ExeSchedWindow *mainExeWindow,
                         QWidget *parent = 0);
    ~TCBTreeView();

  void UpdateView();
  void displayTcbTree(const ex_tcb *tcb,
                      ExSubtask *currSubtask,
                      ExScheduler *sched,
                      bool includeTasks = true,
                      QTreeWidgetItem *parentTreeItem = NULL);

private slots:
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

private:
    Ui::TCBTreeView *ui;

    void displayTcbTasks(const ex_tcb *tcb,
                         ExSubtask *currSubtask,
                         ExScheduler *sched,
                         bool expandTasks,
                         QTreeWidgetItem *tcbTreeItem);
    void getOperatorImageText(const ComTdb *tdb,
                              Int32 & bitmapIndex,
                              QString &nodeText);
    void setBreakSchedInfo(QTreeWidgetItem *treeItem,
                           int numBreakpoints,
                           const char *schedTasks);
    // currently active TCB and subtask
    const ex_tcb *currTcb_;
    ExeSchedWindow *mainExeWindow_;
};

#endif // TCBTREEVIEW_H
