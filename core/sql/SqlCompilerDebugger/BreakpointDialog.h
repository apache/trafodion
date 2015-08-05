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
#ifndef BREAKPOINTDIALOG_H
#define BREAKPOINTDIALOG_H

#include "CommonSqlCmpDbg.h"
#include <QDialog>
#include<QDesktopWidget>
namespace Ui
{
  class BreakpointDialog;
}
class BreakpointDialog:public QDialog
{
Q_OBJECT public:  explicit BreakpointDialog (SQLDebugBrkPts * breakPoint =
			       0, QWidget * parent = 0);
  void ShowBreakpoint ();
   ~BreakpointDialog ();
  private slots:void on_bkptOK_clicked ();
  void on_bkptCancel_clicked ();
  void on_bkptSA_clicked ();
  void on_bkptClrA_clicked ();
private:
  Ui::BreakpointDialog * ui_;
  SQLDebugBrkPts *m_breakpoint;
private:
  void setall(bool checked);
};

#endif // BREAKPOINTDIALOG_H
