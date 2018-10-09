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
#include "TDBTreeView.h"
#include "ui_TDBTreeView.h"
#include "MainWindow.h"
#include "QueryData.h"
#include "TDBDlgMdamNet.h"
#include "TDBDlgExprList.h"

// defined in MainWindow.cpp
extern MainWindow *GlobGuiMainWindow;

TDBTreeView::TDBTreeView(QWidget * parent):
QWidget(parent), ui(new Ui::TDBTreeView)
{
  ui->setupUi(this);
  QStringList header;
  header << "TDB Display" << "Expressions" << "MDAM Disjuncts";
  QTreeWidgetItem *hdrItem = new QTreeWidgetItem(header);
  hdrItem->setIcon(0, QIcon(":/file/R_SQLNODE"));
  hdrItem->setIcon(1, QIcon(":/file/R_SQLNODE"));
  hdrItem->setIcon(2, QIcon(":/file/R_SQLNODE"));
  ui->treeWidget->setHeaderItem(hdrItem);
  ui->treeWidget->setIconSize(QSize(32, 32));
  ui->treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->treeWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui->treeWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

TDBTreeView::~TDBTreeView()
{
  delete ui;
}

/* Public Method Begin */

NAString TDBTreeView::TDBNodeTypeToString(ComTdb::ex_node_type nodeType)
{
  NAString tdbName;

  switch (nodeType)
    {
    case ComTdb::ex_DDL:
      tdbName = "ex_ddl_tdb";
      break;

    case ComTdb::ex_DESCRIBE:
      tdbName = "ex_describe_tdb";
      break;

    case ComTdb::ex_ROOT:
      tdbName = "ex_root_tdb";
      break;

    case ComTdb::ex_ONLJ:
      tdbName = "ex_onlj_tdb";
      break;

    case ComTdb::ex_MJ:
      tdbName = "ex_mj_tdb";
      break;

    case ComTdb::ex_FIRST_N:
      tdbName = "ExFirstNTdb";
      break;

    case ComTdb::ex_HASHJ:
      tdbName = "ex_hashj_tdb";
      break;

    case ComTdb::ex_HASH_GRBY:
      tdbName = "ex_hash_grby_tdb";
      break;

    case ComTdb::ex_LOCK:
      tdbName = "ExLockTdb";
      break;

    case ComTdb::ex_SORT:
      tdbName = "ExSortTdb";
      break;

    case ComTdb::ex_SORT_GRBY:
      tdbName = "ex_sort_grby_tdb";
      break;

    case ComTdb::ex_TRANSACTION:
      tdbName = "ExTransTdb";
      break;

    case ComTdb::ex_UNION:
      tdbName = "ex_union_tdb";
      break;

    case ComTdb::ex_LEAF_TUPLE:
      tdbName = "ex_leaf_tuple_tdb";
      break;

    case ComTdb::ex_NON_LEAF_TUPLE:
      tdbName = "ex_nonleaf_tuple_tdb";
      break;

   case ComTdb::ex_TUPLE_FLOW:
      tdbName = "ex_tuple_flow_tdb";
      break;

   case ComTdb::ex_SPLIT_TOP:
      tdbName = "ex_split_top_tdb";
      break;

    case ComTdb::ex_SPLIT_BOTTOM:
      tdbName = "ex_split_bottom_tdb";
      break;

    case ComTdb::ex_PARTN_ACCESS:
      tdbName = "ex_partn_access_tdb";
      break;

    case ComTdb::ex_SEND_TOP:
      tdbName = "ex_send_top_tdb";
      break;

    case ComTdb::ex_SEND_BOTTOM:
      tdbName = "ex_send_bottom_tdb";
      break;

    case ComTdb::ex_EXPLAIN:
      tdbName = "exExplainTdb";
      break;

    case ComTdb::ex_SEQUENCE_FUNCTION:
      tdbName = "ExSequenceTdb";
      break;

    case ComTdb::ex_UDR:
      tdbName = "ExUdrTdb";
      break;

    default:
      tdbName = "NULL";

    }
  return tdbName;
}

void TDBTreeView::UpdateView()
{
  //CSqldbgDoc* pDoc = (CSqldbgDoc*) GetDocument();
  //ASSERT_VALID(pDoc);
  //------------------------------------------------------------------
  // GSH : Get the Tree to be displayed from the document.
  //------------------------------------------------------------------
  //void* tree;
  //void* plan;
  //pDoc->GetDocumentData(&tree, &plan);

  ComTdb* tdb;
  ExFragDir* fragDir;
  Lng32 baseAddr;

  GlobGuiMainWindow->m_querydata->GetTDBData((void**) &tdb, (void**) &fragDir, baseAddr);
  //------------------------------------------------------------------
  // GSH : Delete the existing Tree.
  //------------------------------------------------------------------
  FreeItemMemory();

  //------------------------------------------------------------------
  // GSH : Now we do the tree walk to display the tree.
  //------------------------------------------------------------------
  DisplayTDBTree(tdb, fragDir, baseAddr, NULL);

  //adjust column width
  ui->treeWidget->expandAll();
  for (int i = 0; i < ui->treeWidget->columnCount(); i++)
  {
      ui->treeWidget->resizeColumnToContents(i);
      int width = ui->treeWidget->columnWidth(i);
      ui->treeWidget->setColumnWidth(i, width + 30);
  }

  //------------------------------------------------------------------
  // GSH :Once ALL TREE CONTROL ITEMS HAVE BEEN ADDED, you can set
  // additional tree control attributes.
  //------------------------------------------------------------------

  //------------------------------------------------------------------
  // GSH : Make all column widths optimal, so text and bitmaps are
  // not clipped horizontally.
  //------------------------------------------------------------------
  //m_Tree.MakeColumnOptimal(1);        // Make column widths optimal
  //m_Tree.MakeColumnOptimal(2);        // Make column widths optimal
  //m_Tree.RecalcHorizontalExtent();     // Update horizontal scroll bar

}

void TDBTreeView::FreeItemMemory(void)
{
  ui->treeWidget->clear();
}


void TDBTreeView::DisplayTDBTree(const class ComTdb* curr,
				    ExFragDir* fragDir,
				    Lng32 baseAddr,
				    QTreeWidgetItem * parentTreeItem)
{
//Int32 treeItemIndex;
  char labelString[20];

  if (curr != NULL) {
    //col 0 string
    //ComTdb::ex_node_type t = curr->getNodeType();
    //NAString textString = TDBNodeTypeToString(t);
    NAString textString = curr->getNodeName();
    //col 1 string
    sprintf(labelString,"%d Expressions.",
               ((ComTdb*)curr)->numExpressions());

    QStringList rowValues;
    rowValues << QString(QLatin1String(textString.data())) << labelString;
    //new and fill item
    QTreeWidgetItem *treeItem = new QTreeWidgetItem(rowValues);
    treeItem->setTextAlignment (1, Qt::AlignRight|Qt::AlignVCenter);
    treeItem->setIcon(0, QIcon(":/file/Resource/Main/Tdbnodes.bmp"));
    QVariant itemData = qVariantFromValue((void*)curr);
    treeItem->setData(0, Qt::UserRole, itemData);

    //add Item into tree
    if (parentTreeItem == NULL)
    {
        ui->treeWidget->addTopLevelItem(treeItem);
    }
    else
    {
        parentTreeItem->addChild(treeItem);
    }

    for (Int32 j = ((ComTdb*)curr)->numChildren()-1; j >= 0; j--) {
           DisplayTDBTree(curr->getChildForGUI(j, baseAddr, fragDir),
						fragDir,
						baseAddr,
						treeItem);
    }
  }//if(curr != NULL)
}

/* Public Method End */
void TDBTreeView::closeEvent(QCloseEvent * event)
{
  event->ignore();
}

void TDBTreeView::on_treeWidget_itemClicked ( QTreeWidgetItem * item, int column )
{
#if 1
  class ComTdb* currTdb;
  TDBDlgExprList* ExprDlg_;
  TDBDlgMdamNet* MdamDlg_;
  QVariant v = item->data(0, Qt::UserRole);
  currTdb = (ComTdb*)v.value <void *>();
  //ComTdb::ex_node_type t = currTdb->getNodeType();
  //NAString textString = TDBNodeTypeToString(t);
  NAString textString = currTdb->getNodeName();
  switch (column) {
    case 1 :  // Expr
            ExprDlg_ = new TDBDlgExprList(this, currTdb, textString);
            ExprDlg_->setAttribute(Qt::WA_DeleteOnClose);
            ExprDlg_->show();
            break;
    case 2 :  // MDAM
            MdamDlg_ = new TDBDlgMdamNet(this);
            MdamDlg_->setAttribute(Qt::WA_DeleteOnClose);
            MdamDlg_->show();
            break;
    default : ;
  }//switch
#endif
}
