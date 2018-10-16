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
#include "BreakpointDialog.h"
#include "ui_BreakpointDialog.h"

BreakpointDialog::BreakpointDialog(SQLDebugBrkPts * breakpoint, QWidget * parent):QDialog(parent),
    ui_(new Ui::BreakpointDialog)

{
  ui_->setupUi(this);
  setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);  // Hide unecessary defaut window icon in the title
  //setFixedSize(this->size());   // Make it unsizable
  m_breakpoint = breakpoint;
}

BreakpointDialog::~BreakpointDialog()
{
  delete ui_;
}

void BreakpointDialog::ShowBreakpoint()
{
      /*
         Set data
      */
  ui_->chkDisplayAfterParsing->setChecked(m_breakpoint->brkAfterParsing);
  ui_->chkDisplayAfterBinding->setChecked(m_breakpoint->brkAfterBinding);
  ui_->chkDisplayAfterTransformation->
      setChecked(m_breakpoint->brkAfterTransform);
  ui_->chkDisplayAfterNormalization->
      setChecked(m_breakpoint->brkAfterNormalize);
  ui_->chkDisplayAfterSemanticQueryOpt->
      setChecked(m_breakpoint->brkAfterSemanticQueryOptimization);
  ui_->chkDisplayAfterAnalysis->setChecked(m_breakpoint->brkAfterAnalyze);
  ui_->chkDisplayAfterOptimization1->setChecked(m_breakpoint->brkAfterOpt1);
  ui_->chkDisplayAfterOptimization2->setChecked(m_breakpoint->brkAfterOpt2);
  ui_->chkDisplayAfterPrecodegen->setChecked(m_breakpoint->brkAfterPreCodegen);
  ui_->chkDisplayAfterCodegen->setChecked(m_breakpoint->brkAfterCodegen);
  ui_->chkDisplayAfterTdbGeneration->setChecked(m_breakpoint->brkAfterTDBgen);
  ui_->chkDisplayExecution->setChecked(m_breakpoint->brkDuringExecution);
  // Set center screen
  QDesktopWidget * desktop = QApplication::desktop();
  move((desktop->width() - this->width()) / 2, (desktop->height() - this->height()) / 2);
  this->exec();
}

void BreakpointDialog::on_bkptOK_clicked()
{
  m_breakpoint->brkAfterParsing = ui_->chkDisplayAfterParsing->isChecked();
  m_breakpoint->brkAfterBinding = ui_->chkDisplayAfterBinding->isChecked();
  m_breakpoint->brkAfterTransform =
      ui_->chkDisplayAfterTransformation->isChecked();
  m_breakpoint->brkAfterNormalize =
      ui_->chkDisplayAfterNormalization->isChecked();
  m_breakpoint->brkAfterSemanticQueryOptimization =
      ui_->chkDisplayAfterSemanticQueryOpt->isChecked();
  m_breakpoint->brkAfterAnalyze = ui_->chkDisplayAfterAnalysis->isChecked();
  m_breakpoint->brkAfterOpt1 = ui_->chkDisplayAfterOptimization1->isChecked();
  m_breakpoint->brkAfterOpt2 = ui_->chkDisplayAfterOptimization2->isChecked();
  m_breakpoint->brkAfterPreCodegen = ui_->chkDisplayAfterPrecodegen->isChecked();
  m_breakpoint->brkAfterCodegen = ui_->chkDisplayAfterCodegen->isChecked();
  m_breakpoint->brkAfterTDBgen = ui_->chkDisplayAfterTdbGeneration->isChecked();
  m_breakpoint->brkDuringExecution = ui_->chkDisplayExecution->isChecked();
  done(0);
}

void BreakpointDialog::on_bkptCancel_clicked()
{
  done(-1);
}

void BreakpointDialog::on_bkptSA_clicked()
{
  setall(true);
}

void BreakpointDialog::on_bkptClrA_clicked()
{
  setall(false);
}

void BreakpointDialog::setall(bool checked)
{
  ui_->chkDisplayAfterParsing->setChecked(checked);
  ui_->chkDisplayAfterBinding->setChecked(checked);
  ui_->chkDisplayAfterTransformation->setChecked(checked);
  ui_->chkDisplayAfterNormalization->setChecked(checked);
  ui_->chkDisplayAfterSemanticQueryOpt->setChecked(checked);
  ui_->chkDisplayAfterAnalysis->setChecked(checked);
  ui_->chkDisplayAfterOptimization1->setChecked(checked);
  ui_->chkDisplayAfterOptimization2->setChecked(checked);
  ui_->chkDisplayAfterPrecodegen->setChecked(checked);
  ui_->chkDisplayAfterCodegen->setChecked(checked);
  ui_->chkDisplayAfterTdbGeneration->setChecked(checked);
  ui_->chkDisplayExecution->setChecked(checked);
}
