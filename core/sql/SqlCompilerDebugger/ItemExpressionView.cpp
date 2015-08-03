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
#include "ItemExpressionView.h"
#include "ui_ItemExpressionView.h"
#define MARGIN 30
#define ICON_H 32
#define ICON_W 32
ItemExpressionView::ItemExpressionView(QWidget * parent):QWidget(parent), ui(new Ui::ItemExpressionView),
m_expr(NULL)
{
  ui->setupUi(this);
  QStringList header;
  header << "Item Expression." << "Value ID" << "Type";
  QTreeWidgetItem* hdrItem = new QTreeWidgetItem(header);
  hdrItem->setIcon(0, QIcon(":/file/R_SQLNODE"));
  hdrItem->setIcon(1, QIcon(":/file/R_SQLNODE"));
  hdrItem->setIcon(2, QIcon(":/file/R_SQLNODE"));
  ui->treeWidget->setHeaderItem(hdrItem);
  ui->treeWidget->setIconSize(QSize(ICON_W, ICON_H));
  ui->treeWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui->treeWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

ItemExpressionView::~ItemExpressionView()
{
  delete ui;
}

void ItemExpressionView::setNewExpr(void *expr)
{
  m_expr = expr;
}

void ItemExpressionView::UpdateView()
{

  //------------------------------------------------------------------
  // GSH : Delete the existing Tree.
  //------------------------------------------------------------------
  Free();
  //------------------------------------------------------------------
  // GSH : Now we do the tree walk to display the tree.
  //------------------------------------------------------------------
  DisplayItemExprRoot(m_expr);
  ui->treeWidget->expandAll();
  for(int i=0;i<ui->treeWidget->columnCount();i++){
    ui->treeWidget->resizeColumnToContents(i);
    int width = ui->treeWidget->columnWidth (i);
    ui->treeWidget->setColumnWidth(i,width + MARGIN);
  }
}

void ItemExpressionView::Free(void)
{
  ui->treeWidget->clear();
}

void ItemExpressionView::DisplayItemExprRoot(void *tree)
{
  ExprNode *qTree = (ExprNode *) tree;
  if (qTree != NULL)
  {
      LIST(ExprNode *) localExpList;
      LIST(NAString) localLabelList;
      ExprNode *currExpr;
      NAString currLabel;
      CollIndex numEntries;
      qTree->addLocalExpr(localExpList, localLabelList);

      // remove NULL pointers from the end of the list
      // (just looks nicer that way)
      //while ((numEntries = localExpList.entries()) > 0
      //       && localExpList[numEntries - 1] == NULL)
      //  localExpList.getLast(currExpr);

      // add the remaining list elements to the list of
      // children of this expression widget
      while (localExpList.getFirst(currExpr))
      {
          localLabelList.getFirst(currLabel);
          QStringList rowValues;
          rowValues.append(QString(QLatin1String(currLabel.data())));
          QTreeWidgetItem *treeItem = new QTreeWidgetItem(rowValues);
          treeItem->setIcon(0,QIcon(":/file/Resource/Main/Ienodes.bmp"));
          ui->treeWidget->addTopLevelItem(treeItem);
          //set column alignment
          treeItem->setTextAlignment (1, Qt::AlignRight|Qt::AlignVCenter);
          //TODO: SetItemBitmap
          FillItemExprDetails(currExpr, treeItem);
          DisplayItemExprChild(currExpr, treeItem);
       }
    }// if (qTree != NULL)
}

void ItemExpressionView::DisplayItemExprChild(void *tree,
                                              QTreeWidgetItem * parentTreeItem)
{
  ExprNode *qTree = (ExprNode *) tree;
  if (qTree != NULL && parentTreeItem != NULL)
    {
      NAString nodeType;
      nodeType = qTree->getText();
      QStringList rowValues;
      rowValues.append(QString(QLatin1String(nodeType.data())));
      QTreeWidgetItem *treeItem = new QTreeWidgetItem(rowValues);
      treeItem->setIcon(0,QIcon(":/file/Resource/Main/Ienodes.bmp"));
      parentTreeItem->addChild(treeItem);
      //set column alignment for each row
      treeItem->setTextAlignment (1, Qt::AlignRight|Qt::AlignVCenter);

      FillItemExprDetails(qTree, treeItem);
      for (Int32 i = qTree->getArity() - 1; i >= 0; i--)
        {
          DisplayItemExprChild(qTree->getChild(i), treeItem);
        }
    }
}

void ItemExpressionView::FillItemExprDetails(ExprNode * en,
                                             QTreeWidgetItem * treeItem)
{
  //char labelString[100];
  QString labelString;
  // -----------------------------------------------------------------------
  // en might be NULL, for instance when there is a
  // MDAM key with disjunct with no key predicates...
  // -----------------------------------------------------------------------
  if (en != NULL)
  {
      if (en->castToItemExpr() != NULL)
      {
          labelString = QString("%1").arg((int)(CollIndex) en->castToItemExpr()->getValueId());
          treeItem->setText(1, labelString);
      }
      if (en->castToItemExpr() !=
          NULL AND en->castToItemExpr()->getValueId() !=
          NULL_VALUE_ID AND & (en->castToItemExpr()->getValueId().getType()) !=
          NULL)
      {
          labelString = QString("%1").arg(en->castToItemExpr()->getValueId().getType().getTypeSQLname().data());
          treeItem->setText(2, labelString);
      }
  }
}
