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
#include "ExeQueueDisplay.h"
#include "ui_ExeQueueDisplay.h"
#include "CommonSqlCmpDbg.h"
#include "TCBTreeView.h"

ExeQueueDisplay::ExeQueueDisplay(const ex_tcb *tcb,
                                 bool isUp,
                                 QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ExeQueueDisplay),
  tcb_(tcb),
  isUp_(isUp)
{
  ui->setupUi(this);

  QString s = QString("%1 queue for %2 (Id: %3 Ex: %4)").
                 arg(isUp_ ? "Up" : "Down").
                 arg(tcb->getTdb()->getNodeName()).
                 arg(tcb->getTdb()->getTdbId()).
                 arg(tcb->getTdb()->getExplainNodeId());
  setWindowTitle(s);
  s = "";
  setLabel(s);
  populate();
}

ExeQueueDisplay::~ExeQueueDisplay()
{
  delete ui;
}

void ExeQueueDisplay::setLabel(QString &lbl)
{
    ui->queueLabel->setText(lbl);
}

void ExeQueueDisplay::populate()
{
  ex_queue_pair qp = tcb_->getParentQueue();
  ex_queue *q = (isUp_ ? qp.up : qp.down);
  queue_index qLength = q->getLength();
  queue_index headIndex = q->getHeadIndex();
  QStringList headers;

  ui->tableWidget->setRowCount(qLength);
  headers << "Index";
  if (isUp_)
    {
      headers << "DownIx" << "ParentIx" << "State";
      ui->tableWidget->setColumnCount(4);
    }
  else
    {
      headers << "ParentIx" << "State";
      ui->tableWidget->setColumnCount(3);
    }
  ui->tableWidget->setHorizontalHeaderLabels(headers);
  for (queue_index ix=0; ix<qLength; ix++)
    {
      ex_queue_entry *qEntry = q->getQueueEntry(headIndex+ix);

      ui->tableWidget->setItem(ix, 0, new QTableWidgetItem(QString("%1").
                           arg(headIndex + ix)));
      if (isUp_)
        {
          up_state upState = qEntry->upState;
          QString status;

          ui->tableWidget->setItem(ix, 1, new QTableWidgetItem(QString("%1").
                                                               arg(upState.downIndex)));
          ui->tableWidget->setItem(ix, 2, new QTableWidgetItem(QString("%1").
                                                               arg(upState.parentIndex)));

          switch (upState.status)
            {
            case ex_queue::Q_NO_DATA:
              status = "Q_NO_DATA";
              break;
            case ex_queue::Q_OK_MMORE:
              status = "Q_OK_MMORE";
              break;
            case ex_queue::Q_SQLERROR:
              status = "Q_SQLERROR";
              break;
            case ex_queue::Q_INVALID:
              status = "Q_INVALID";
              break;
            case ex_queue::Q_GET_DONE:
              status = "Q_GET_DONE";
              break;
            case ex_queue::Q_REC_SKIPPED:
              status = "Q_REC_SKIPPED";
              break;
            case ex_queue::Q_STATS:
              status = "Q_STATS";
              break;
            default:
              status = QString("unknown: %1").arg((int) upState.status);
              break;
            }
          ui->tableWidget->setItem(ix, 3, new QTableWidgetItem(status));
        }
      else
        {
          down_state downState = qEntry->downState;
          QString status;

          ui->tableWidget->setItem(ix, 1, new QTableWidgetItem(QString("%1").arg(downState.parentIndex)));

          switch (downState.request)
            {
            case ex_queue::GET_N:
              status = QString("GET_N (%1)").arg(downState.requestValue);
              break;
            case ex_queue::GET_ALL:
              status = "GET_ALL";
              break;
            case ex_queue::GET_NOMORE:
              status = "GET_NOMORE";
              break;
            case ex_queue::GET_EMPTY:
              status = "GET_EMPTY";
              break;
            case ex_queue::GET_EOD:
              status = "GET_EOD";
              break;
            case ex_queue::GET_EOD_NO_ST_COMMIT:
              status = "GET_EOD_NO_ST_COMMIT";
              break;
            case ex_queue::GET_NEXT_N:
              status = QString("GET_NEXT_N (%1)").arg(downState.requestValue);
              break;
            case ex_queue::GET_NEXT_N_MAYBE_WAIT:
              status = "GET_NEXT_N_MAYBE_WAIT";
              break;
            case ex_queue::GET_NEXT_0_MAYBE_WAIT:
              status = "GET_NEXT_0_MAYBE_WAIT";
              break;
            case ex_queue::GET_NEXT_N_SKIP:
              status = "GET_NEXT_N_SKIP";
              break;
            case ex_queue::GET_NEXT_N_MAYBE_WAIT_SKIP:
              status = "GET_NEXT_N_MAYBE_WAIT_SKIP";
              break;
            case ex_queue::GET_N_RETURN_SKIPPED:
              status = "GET_N_RETURN_SKIPPED";
              break;
           default:
              status = QString("unknown: %1").arg((int) downState.request);
              break;
            }
          ui->tableWidget->setItem(ix, 2, new QTableWidgetItem(status));
        }
    }
}

void ExeQueueDisplay::on_okButton_clicked()
{
  delete this;
}

void ExeQueueDisplay::on_cancelButton_clicked()
{
  // same as OK button, just close the window
  on_okButton_clicked();
}
