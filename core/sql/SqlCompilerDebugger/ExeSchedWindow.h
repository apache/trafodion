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
#ifndef EXESCHEDWINDOW_H
#define EXESCHEDWINDOW_H

#include <QMainWindow>
#include "TCBTreeView.h"

namespace Ui {
class ExeSchedWindow;
}

class ExSubtask;
class ExScheduler;

class ExeSchedWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ExeSchedWindow(QWidget *parent = 0);
    ~ExeSchedWindow();

    // initialize the info at the top of the window
    void initializeStaticInfo(ExScheduler *sched);

    // drive the event loop for the window until we
    // are ready to resume with the executor tasks
    void run(ExSubtask **subtaskPtr);
    // ensure the window is visible/invisible
    void ensureVisible();
    void ensureInvisible();
    // check whether the user requested to see the
    // GUI for this particular subtask
    bool needToStop(void *subtask, void *scheduler);

    // should the run() method continue to process events?
    bool keepProcessingEvents() const;
    void setKeepProcessingEvents(bool keepProcessingEvents);

    bool stopAtBreakpoints() const;
    void setStopAtBreakpoints(bool stopAtBreakpoints);

    bool stopAtAllTasks() const;
    void setStopAtAllTasks(bool stopAtAllTasks);

    // find the instance id of a TCB tree, given the
    // scheduler
    static ExeSchedWindow *findInstance(ExScheduler *sched);
    static void deleteInstance(void *scheduler);
    static void deleteAllInstances();

    // get the main window
    ExSubtask **getSubtaskPtr() { return subtaskPtr_; }

private slots:
    void on_finishButton_clicked();

    void on_nextBptButton_clicked();

    void on_nextTaskButton_clicked();

private:
    Ui::ExeSchedWindow *ui;
    bool headerIsInitialized_;
    bool isVisible_;
    bool keepProcessingEvents_;
    bool stopAtBreakpoints_;
    bool stopAtAllTasks_;
    QRect savedGeometry_;
    bool hasSavedGeometry_;
    ExSubtask **subtaskPtr_;
    TCBTreeView *tcbTreeView_;
};

#endif // EXESCHEDWINDOW_H
