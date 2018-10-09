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
#ifndef EXEQUEUEDISPLAY_H
#define EXEQUEUEDISPLAY_H

#include <QtGui>
#include <QDialog>
#include "CommonSqlCmpDbg.h"
#include "ex_globals.h"
#include "ex_tcb.h"

namespace Ui {
  class ExeQueueDisplay;
  }

class QTreeWidgetItem;

class ExeQueueDisplay : public QDialog
{
  Q_OBJECT

  public:
  explicit ExeQueueDisplay(const ex_tcb *tcb,
                           bool isUp,
                           QWidget *parent);
  ~ExeQueueDisplay();

  void setLabel(QString & lbl);
  void populate();

  private slots:
  void on_okButton_clicked();

  void on_cancelButton_clicked();

  private:
  Ui::ExeQueueDisplay *ui;
  const ex_tcb *tcb_;
  bool isUp_;
};

#endif // EXEQUEUEDISPLAY_H
