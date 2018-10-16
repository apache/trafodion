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
#include "ExeSchedWindow.h"
#include "ui_ExeSchedWindow.h"
#include "CommonSqlCmpDbg.h"

// defined in MainWindow.cpp
extern QApplication* GlobGuiApplication;

const int GlobGuiExeMaxFragInstances = 32;
ExeSchedWindow *GlobGuiExeSchedWindow[GlobGuiExeMaxFragInstances];
int GlobGuiExeFragInstancesLen = 0;

ExeSchedWindow::ExeSchedWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ExeSchedWindow)
{
    ui->setupUi(this);
    headerIsInitialized_ = false;
    isVisible_ = false;
    keepProcessingEvents_ = true;
    stopAtBreakpoints_ = true;
    stopAtAllTasks_ = true;
    hasSavedGeometry_ = false;
    subtaskPtr_ = NULL;
    tcbTreeView_ = new TCBTreeView(this);

    // ViewContainer *viewContainer = new ViewContainer();
    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(tcbTreeView_);
    subWindow->resize(ui->mdiArea->width() / 2, ui->mdiArea->height() / 2);
    subWindow->showMaximized();

}

ExeSchedWindow::~ExeSchedWindow()
{
    delete ui;
}

void ExeSchedWindow::initializeStaticInfo(ExScheduler *sched)
{
  if (!headerIsInitialized_)
    {
      int frag, inst, numInst, nid, pid;
      char buf[100];
      char procName[100];
      int xOffset = 0;
      int yOffset = 0;
      int xDelta = 75;
      int yDelta = 75;
      QDesktopWidget *desktop = QApplication::desktop();
      int maxXOffset = (desktop->width() - this->width()) / 4;
      int maxYOffset = (desktop->height() - this->height()) / 4;
      sched->getProcInfoForGui(frag, inst, numInst, nid, pid, procName, sizeof(procName));

      if (frag == 0)
        snprintf(buf, sizeof(buf), "Master Executor");
      else
        snprintf(buf, sizeof(buf), "Fragment %d, instance %d of %d",
                 frag, inst, numInst);
      ui->centralwidget->parentWidget()->setWindowTitle(buf);

      snprintf(buf, sizeof(buf), "%d, %d, %s", nid, pid, procName);
      ui->nidPidLabel->setText(buf);
      // position the windows such that they are staggered
      // and their titles are visible, fragments at the same
      // height and instances arranged left to right
      xOffset = inst * xDelta;
      yOffset = frag * yDelta;
      if (xOffset > maxXOffset)
        xOffset = maxXOffset;
      if (yOffset > maxYOffset)
        yOffset = maxYOffset;
      move(xOffset, yOffset);

      headerIsInitialized_ = true;
    }
}

void ExeSchedWindow::run(ExSubtask **subtaskPtr)
{
    ensureVisible();
    keepProcessingEvents_ = true;
    subtaskPtr_ = subtaskPtr;

    while (keepProcessingEvents_)
    {
        GlobGuiApplication->sendPostedEvents();
        GlobGuiApplication->processEvents(
             QEventLoop::WaitForMoreEvents |
             QEventLoop::EventLoopExec);
    }
    subtaskPtr_ = NULL;
    ensureInvisible();
}

void ExeSchedWindow::ensureVisible()
{
  if (!isVisible_)
    {
      if (hasSavedGeometry_)
        setGeometry(savedGeometry_);
      show();
      isVisible_ = true;
    }
}

void ExeSchedWindow::ensureInvisible()
{
  if (isVisible_)
    {
      savedGeometry_ = geometry();
      hasSavedGeometry_ = true;
      hide();
      isVisible_ = false;
    }
}

bool ExeSchedWindow::needToStop(void *subtask, void *scheduler)
{
    ExSubtask *st = static_cast<ExSubtask *>(subtask);
    ExScheduler *sch = static_cast<ExScheduler *>(scheduler);

    if (scheduler != NULL &&
        (stopAtAllTasks_ ||
         (st && st->getBreakPoint() && stopAtBreakpoints_)))
      {
        initializeStaticInfo(sch);

        if (tcbTreeView_ && st && st->getTcb())
          tcbTreeView_->displayTcbTree(st->getTcb(), st, sch);
        return true;
      }

    return false;
}

void ExeSchedWindow::on_finishButton_clicked()
{
    keepProcessingEvents_ = false;
    stopAtBreakpoints_ = false;
    stopAtAllTasks_ = false;
}

void ExeSchedWindow::on_nextBptButton_clicked()
{
    keepProcessingEvents_ = false;
    stopAtBreakpoints_ = true;
    stopAtAllTasks_ = false;
}

void ExeSchedWindow::on_nextTaskButton_clicked()
{
    keepProcessingEvents_ = false;
    stopAtBreakpoints_ = true;
    stopAtAllTasks_ = true;
}
bool ExeSchedWindow::stopAtAllTasks() const
{
    return stopAtAllTasks_;
}

void ExeSchedWindow::setStopAtAllTasks(bool stopAtAllTasks)
{
    stopAtAllTasks_ = stopAtAllTasks;
  }

ExeSchedWindow *ExeSchedWindow::findInstance(ExScheduler *sched)
{
    int fragInstId = sched->getFragInstIdForGui();

    if (fragInstId >= GlobGuiExeFragInstancesLen)
      {
        if (fragInstId >= GlobGuiExeMaxFragInstances)
          // this is not handled right now
          return NULL;

        // fill the array that is now used with NULLs
        for (int i=GlobGuiExeFragInstancesLen; i<=fragInstId; i++)
          GlobGuiExeSchedWindow[i] = NULL;
        GlobGuiExeFragInstancesLen = fragInstId+1;
      }
    if (GlobGuiExeSchedWindow[fragInstId] == NULL)
      GlobGuiExeSchedWindow[fragInstId] = new ExeSchedWindow();

    return GlobGuiExeSchedWindow[fragInstId];
  }

void ExeSchedWindow::deleteInstance(void *scheduler)
{
  ExScheduler *sched = static_cast<ExScheduler *>(scheduler);
  int fragInstId = sched->getFragInstIdForGui();

  if (fragInstId >= 0 && fragInstId < GlobGuiExeFragInstancesLen)
    {
      delete GlobGuiExeSchedWindow[fragInstId];
      GlobGuiExeSchedWindow[fragInstId] = NULL;
    }
  }

void ExeSchedWindow::deleteAllInstances()
{
  for (int i=0; i<GlobGuiExeFragInstancesLen; i++)
    if (GlobGuiExeSchedWindow[i])
      delete GlobGuiExeSchedWindow[i];
  GlobGuiExeFragInstancesLen = 0;
}

bool ExeSchedWindow::stopAtBreakpoints() const
{
    return stopAtBreakpoints_;
}

void ExeSchedWindow::setStopAtBreakpoints(bool stopAtBreakpoints)
{
    stopAtBreakpoints_ = stopAtBreakpoints;
}

bool ExeSchedWindow::keepProcessingEvents() const
{
    return keepProcessingEvents_;
}

void ExeSchedWindow::setKeepProcessingEvents(bool keepProcessingEvents)
{
    keepProcessingEvents_ = keepProcessingEvents;
}

