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
#include "QueryAnalysisView.h"
#include "ui_QueryAnalysisView.h"

QueryAnalysisView::QueryAnalysisView(QueryAnalysis* analysis, QWidget * parent):QWidget(parent),ui(new Ui::QueryAnalysisView)
{
  ui->setupUi(this);
  queryAnalysis_ = analysis;
  UpdateView();
}

QueryAnalysisView::~QueryAnalysisView()
{
  delete ui;
}

void QueryAnalysisView::UpdateView()
{
  NAString qaText;
  const int MAX_STR_LEN = 1001;
  char validascii[MAX_STR_LEN];

  Free();

  if (queryAnalysis_ != NULL)  {
    NAString oneline(queryAnalysis_->getText());
    NAString tempascii;
    size_t idx;
    BOOL done = FALSE;
    // Newline delimited text.  Need to split it up and add one line at a time
    idx = oneline.first('\n');
    if (idx == NA_NPOS && oneline.length() > 0)  {
    // it could just be one line
      sprintf(validascii, oneline);
      AddListItem(NAString(validascii));
    }
    else if (idx == NA_NPOS && oneline.length() == 0)  {
      sprintf(validascii, "QueryAnalysis getText is NULL!");
      AddListItem(NAString(validascii));
    }
    while (idx != NA_NPOS && !done)  {
      // find first newline if any
      if (idx == NA_NPOS)  {
        done = TRUE;
      // still add the line, it may not be newline terminated
      // only add if there is something there
        if (oneline.length() > 0){
          AddListItem(oneline);
          oneline.remove(0);  // empty list
        }
      }
      else{
        tempascii.remove(0);
        tempascii.append(oneline,idx);
        AddListItem(tempascii);
        oneline.remove(0,idx+1); // remove newline too
      }
      idx = oneline.first('\n');
    }//while
  }
}

Int32 QueryAnalysisView::AddListItem(const NAString & s)
{
  if(0 == s.length())
    return -1;
  QString qline((const char*)s);
  int index = qline.lastIndexOf("\n");
  if(index >= 0)
    qline[index] = ' ';
  ui->listWidget->addItem(qline);
  return 0;
}

void QueryAnalysisView::Free()
{
    ui->listWidget->clear();
}

void QueryAnalysisView::on_btnOK_clicked()
{
   QObject* obj = parent();
   if(0==strcmp(obj->metaObject()->className(), "QMdiSubWindow"))
     ((QMdiSubWindow *)obj)->close();
}

void QueryAnalysisView::on_btnCancel_clicked()
{
   QObject* obj = parent();
   if(0==strcmp(obj->metaObject()->className(), "QMdiSubWindow"))
     ((QMdiSubWindow *)obj)->close();
}
