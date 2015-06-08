// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class SchemaPrivilegeOptionsControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._allCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._allDMLCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._allDDLCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._selectCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._insertCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._deleteCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._updateCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._executeCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._referencesCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dmlGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._usageCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._ddlGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._dropGuiGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._dropTriggerCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropSynonymCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropProcedureCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropLibraryCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropViewCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropMVGroupCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropMVCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropTableCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dropCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._alterTriggerCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterSynonymCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterLibraryCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterViewCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterMVGroupCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterMVCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterTableCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._alterCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._createTriggerCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createSynonymCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createLibraryCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createProcedureCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createViewCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createMVGroupCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createMVCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createTableCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._createCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._allUTILITYCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._utilityGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._replicateCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._dmlGroupBox.SuspendLayout();
            this._ddlGroupBox.SuspendLayout();
            this._dropGuiGroupBox.SuspendLayout();
            this._alterGroupBox.SuspendLayout();
            this._createGroupBox.SuspendLayout();
            this._utilityGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _allCheckBox
            // 
            this._allCheckBox.AutoSize = true;
            this._allCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._allCheckBox.Location = new System.Drawing.Point(24, 12);
            this._allCheckBox.Name = "_allCheckBox";
            this._allCheckBox.Size = new System.Drawing.Size(49, 18);
            this._allCheckBox.TabIndex = 0;
            this._allCheckBox.Text = "ALL";
            this._allCheckBox.UseVisualStyleBackColor = true;
            this._allCheckBox.CheckedChanged += new System.EventHandler(this._allCheckBox_CheckedChanged);
            // 
            // _allDMLCheckBox
            // 
            this._allDMLCheckBox.AutoSize = true;
            this._allDMLCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allDMLCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._allDMLCheckBox.Location = new System.Drawing.Point(87, 12);
            this._allDMLCheckBox.Name = "_allDMLCheckBox";
            this._allDMLCheckBox.Size = new System.Drawing.Size(72, 18);
            this._allDMLCheckBox.TabIndex = 2;
            this._allDMLCheckBox.Text = "ALL DML";
            this._allDMLCheckBox.UseVisualStyleBackColor = true;
            this._allDMLCheckBox.CheckedChanged += new System.EventHandler(this._allDMLCheckBox_CheckedChanged);
            // 
            // _allDDLCheckBox
            // 
            this._allDDLCheckBox.AutoSize = true;
            this._allDDLCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allDDLCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._allDDLCheckBox.Location = new System.Drawing.Point(173, 12);
            this._allDDLCheckBox.Name = "_allDDLCheckBox";
            this._allDDLCheckBox.Size = new System.Drawing.Size(71, 18);
            this._allDDLCheckBox.TabIndex = 1;
            this._allDDLCheckBox.Text = "ALL DDL";
            this._allDDLCheckBox.UseVisualStyleBackColor = true;
            this._allDDLCheckBox.CheckedChanged += new System.EventHandler(this._allDDLCheckBox_CheckedChanged);
            // 
            // _selectCheckBox
            // 
            this._selectCheckBox.AutoSize = true;
            this._selectCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._selectCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._selectCheckBox.Location = new System.Drawing.Point(19, 22);
            this._selectCheckBox.Name = "_selectCheckBox";
            this._selectCheckBox.Size = new System.Drawing.Size(61, 18);
            this._selectCheckBox.TabIndex = 0;
            this._selectCheckBox.Text = "Select";
            this._selectCheckBox.UseVisualStyleBackColor = true;
            // 
            // _insertCheckBox
            // 
            this._insertCheckBox.AutoSize = true;
            this._insertCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._insertCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._insertCheckBox.Location = new System.Drawing.Point(83, 22);
            this._insertCheckBox.Name = "_insertCheckBox";
            this._insertCheckBox.Size = new System.Drawing.Size(61, 18);
            this._insertCheckBox.TabIndex = 1;
            this._insertCheckBox.Text = "Insert";
            this._insertCheckBox.UseVisualStyleBackColor = true;
            // 
            // _deleteCheckBox
            // 
            this._deleteCheckBox.AutoSize = true;
            this._deleteCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._deleteCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._deleteCheckBox.Location = new System.Drawing.Point(217, 22);
            this._deleteCheckBox.Name = "_deleteCheckBox";
            this._deleteCheckBox.Size = new System.Drawing.Size(63, 18);
            this._deleteCheckBox.TabIndex = 3;
            this._deleteCheckBox.Text = "Delete";
            this._deleteCheckBox.UseVisualStyleBackColor = true;
            // 
            // _updateCheckBox
            // 
            this._updateCheckBox.AutoSize = true;
            this._updateCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._updateCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._updateCheckBox.Location = new System.Drawing.Point(147, 22);
            this._updateCheckBox.Name = "_updateCheckBox";
            this._updateCheckBox.Size = new System.Drawing.Size(67, 18);
            this._updateCheckBox.TabIndex = 2;
            this._updateCheckBox.Text = "Update";
            this._updateCheckBox.UseVisualStyleBackColor = true;
            // 
            // _executeCheckBox
            // 
            this._executeCheckBox.AutoSize = true;
            this._executeCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._executeCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._executeCheckBox.Location = new System.Drawing.Point(283, 22);
            this._executeCheckBox.Name = "_executeCheckBox";
            this._executeCheckBox.Size = new System.Drawing.Size(71, 18);
            this._executeCheckBox.TabIndex = 4;
            this._executeCheckBox.Text = "Execute";
            this._executeCheckBox.UseVisualStyleBackColor = true;
            // 
            // _referencesCheckBox
            // 
            this._referencesCheckBox.AutoSize = true;
            this._referencesCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._referencesCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._referencesCheckBox.Location = new System.Drawing.Point(357, 22);
            this._referencesCheckBox.Name = "_referencesCheckBox";
            this._referencesCheckBox.Size = new System.Drawing.Size(82, 18);
            this._referencesCheckBox.TabIndex = 5;
            this._referencesCheckBox.Text = "Reference";
            this._referencesCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dmlGroupBox
            // 
            this._dmlGroupBox.Controls.Add(this._usageCheckBox);
            this._dmlGroupBox.Controls.Add(this._selectCheckBox);
            this._dmlGroupBox.Controls.Add(this._insertCheckBox);
            this._dmlGroupBox.Controls.Add(this._deleteCheckBox);
            this._dmlGroupBox.Controls.Add(this._referencesCheckBox);
            this._dmlGroupBox.Controls.Add(this._updateCheckBox);
            this._dmlGroupBox.Controls.Add(this._executeCheckBox);
            this._dmlGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dmlGroupBox.Location = new System.Drawing.Point(5, 38);
            this._dmlGroupBox.Name = "_dmlGroupBox";
            this._dmlGroupBox.Size = new System.Drawing.Size(514, 55);
            this._dmlGroupBox.TabIndex = 3;
            this._dmlGroupBox.TabStop = false;
            this._dmlGroupBox.Text = "DML Privileges";
            // 
            // _usageCheckBox
            // 
            this._usageCheckBox.AutoSize = true;
            this._usageCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._usageCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._usageCheckBox.Location = new System.Drawing.Point(447, 22);
            this._usageCheckBox.Name = "_usageCheckBox";
            this._usageCheckBox.Size = new System.Drawing.Size(62, 18);
            this._usageCheckBox.TabIndex = 6;
            this._usageCheckBox.Text = "Usage";
            this._usageCheckBox.UseVisualStyleBackColor = true;
            // 
            // _ddlGroupBox
            // 
            this._ddlGroupBox.Controls.Add(this._dropGuiGroupBox);
            this._ddlGroupBox.Controls.Add(this._alterGroupBox);
            this._ddlGroupBox.Controls.Add(this._createGroupBox);
            this._ddlGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._ddlGroupBox.Location = new System.Drawing.Point(5, 102);
            this._ddlGroupBox.Name = "_ddlGroupBox";
            this._ddlGroupBox.Size = new System.Drawing.Size(514, 241);
            this._ddlGroupBox.TabIndex = 4;
            this._ddlGroupBox.TabStop = false;
            this._ddlGroupBox.Text = "DDL Privileges";
            // 
            // _dropGuiGroupBox
            // 
            this._dropGuiGroupBox.Controls.Add(this._dropTriggerCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropSynonymCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropProcedureCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropLibraryCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropViewCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropMVGroupCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropMVCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropTableCheckBox);
            this._dropGuiGroupBox.Controls.Add(this._dropCheckBox);
            this._dropGuiGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropGuiGroupBox.Location = new System.Drawing.Point(343, 12);
            this._dropGuiGroupBox.Name = "_dropGuiGroupBox";
            this._dropGuiGroupBox.Size = new System.Drawing.Size(158, 220);
            this._dropGuiGroupBox.TabIndex = 2;
            this._dropGuiGroupBox.TabStop = false;
            // 
            // _dropTriggerCheckBox
            // 
            this._dropTriggerCheckBox.AutoSize = true;
            this._dropTriggerCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropTriggerCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropTriggerCheckBox.Location = new System.Drawing.Point(6, 62);
            this._dropTriggerCheckBox.Name = "_dropTriggerCheckBox";
            this._dropTriggerCheckBox.Size = new System.Drawing.Size(92, 18);
            this._dropTriggerCheckBox.TabIndex = 2;
            this._dropTriggerCheckBox.Text = "Drop Trigger";
            this._dropTriggerCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropSynonymCheckBox
            // 
            this._dropSynonymCheckBox.AutoSize = true;
            this._dropSynonymCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropSynonymCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropSynonymCheckBox.Location = new System.Drawing.Point(6, 167);
            this._dropSynonymCheckBox.Name = "_dropSynonymCheckBox";
            this._dropSynonymCheckBox.Size = new System.Drawing.Size(102, 18);
            this._dropSynonymCheckBox.TabIndex = 7;
            this._dropSynonymCheckBox.Text = "Drop Synonym";
            this._dropSynonymCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropProcedureCheckBox
            // 
            this._dropProcedureCheckBox.AutoSize = true;
            this._dropProcedureCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropProcedureCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropProcedureCheckBox.Location = new System.Drawing.Point(6, 146);
            this._dropProcedureCheckBox.Name = "_dropProcedureCheckBox";
            this._dropProcedureCheckBox.Size = new System.Drawing.Size(107, 18);
            this._dropProcedureCheckBox.TabIndex = 6;
            this._dropProcedureCheckBox.Text = "Drop Procedure";
            this._dropProcedureCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropLibraryCheckBox
            // 
            this._dropLibraryCheckBox.AutoSize = true;
            this._dropLibraryCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropLibraryCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropLibraryCheckBox.Location = new System.Drawing.Point(6, 188);
            this._dropLibraryCheckBox.Name = "_dropLibraryCheckBox";
            this._dropLibraryCheckBox.Size = new System.Drawing.Size(91, 18);
            this._dropLibraryCheckBox.TabIndex = 5;
            this._dropLibraryCheckBox.Text = "Drop Library";
            this._dropLibraryCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropViewCheckBox
            // 
            this._dropViewCheckBox.AutoSize = true;
            this._dropViewCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropViewCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropViewCheckBox.Location = new System.Drawing.Point(6, 125);
            this._dropViewCheckBox.Name = "_dropViewCheckBox";
            this._dropViewCheckBox.Size = new System.Drawing.Size(80, 18);
            this._dropViewCheckBox.TabIndex = 5;
            this._dropViewCheckBox.Text = "Drop View";
            this._dropViewCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropMVGroupCheckBox
            // 
            this._dropMVGroupCheckBox.AutoSize = true;
            this._dropMVGroupCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropMVGroupCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropMVGroupCheckBox.Location = new System.Drawing.Point(6, 104);
            this._dropMVGroupCheckBox.Name = "_dropMVGroupCheckBox";
            this._dropMVGroupCheckBox.Size = new System.Drawing.Size(104, 18);
            this._dropMVGroupCheckBox.TabIndex = 4;
            this._dropMVGroupCheckBox.Text = "Drop MV Group";
            this._dropMVGroupCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropMVCheckBox
            // 
            this._dropMVCheckBox.AutoSize = true;
            this._dropMVCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropMVCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropMVCheckBox.Location = new System.Drawing.Point(6, 83);
            this._dropMVCheckBox.Name = "_dropMVCheckBox";
            this._dropMVCheckBox.Size = new System.Drawing.Size(140, 18);
            this._dropMVCheckBox.TabIndex = 3;
            this._dropMVCheckBox.Text = "Drop Materialized View";
            this._dropMVCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropTableCheckBox
            // 
            this._dropTableCheckBox.AutoSize = true;
            this._dropTableCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropTableCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropTableCheckBox.Location = new System.Drawing.Point(6, 41);
            this._dropTableCheckBox.Name = "_dropTableCheckBox";
            this._dropTableCheckBox.Size = new System.Drawing.Size(84, 18);
            this._dropTableCheckBox.TabIndex = 1;
            this._dropTableCheckBox.Text = "Drop Table";
            this._dropTableCheckBox.UseVisualStyleBackColor = true;
            // 
            // _dropCheckBox
            // 
            this._dropCheckBox.AutoSize = true;
            this._dropCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._dropCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dropCheckBox.Location = new System.Drawing.Point(6, 20);
            this._dropCheckBox.Name = "_dropCheckBox";
            this._dropCheckBox.Size = new System.Drawing.Size(55, 18);
            this._dropCheckBox.TabIndex = 0;
            this._dropCheckBox.Text = "Drop";
            this._dropCheckBox.UseVisualStyleBackColor = true;
            this._dropCheckBox.CheckedChanged += new System.EventHandler(this._dropCheckBox_CheckedChanged);
            // 
            // _alterGroupBox
            // 
            this._alterGroupBox.Controls.Add(this._alterTriggerCheckBox);
            this._alterGroupBox.Controls.Add(this._alterSynonymCheckBox);
            this._alterGroupBox.Controls.Add(this._alterLibraryCheckBox);
            this._alterGroupBox.Controls.Add(this._alterViewCheckBox);
            this._alterGroupBox.Controls.Add(this._alterMVGroupCheckBox);
            this._alterGroupBox.Controls.Add(this._alterMVCheckBox);
            this._alterGroupBox.Controls.Add(this._alterTableCheckBox);
            this._alterGroupBox.Controls.Add(this._alterCheckBox);
            this._alterGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterGroupBox.Location = new System.Drawing.Point(178, 12);
            this._alterGroupBox.Name = "_alterGroupBox";
            this._alterGroupBox.Size = new System.Drawing.Size(158, 220);
            this._alterGroupBox.TabIndex = 1;
            this._alterGroupBox.TabStop = false;
            // 
            // _alterTriggerCheckBox
            // 
            this._alterTriggerCheckBox.AutoSize = true;
            this._alterTriggerCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterTriggerCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterTriggerCheckBox.Location = new System.Drawing.Point(6, 62);
            this._alterTriggerCheckBox.Name = "_alterTriggerCheckBox";
            this._alterTriggerCheckBox.Size = new System.Drawing.Size(89, 18);
            this._alterTriggerCheckBox.TabIndex = 2;
            this._alterTriggerCheckBox.Text = "AlterTrigger";
            this._alterTriggerCheckBox.UseVisualStyleBackColor = true;
            // 
            // _alterSynonymCheckBox
            // 
            this._alterSynonymCheckBox.AutoSize = true;
            this._alterSynonymCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterSynonymCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterSynonymCheckBox.Location = new System.Drawing.Point(6, 146);
            this._alterSynonymCheckBox.Name = "_alterSynonymCheckBox";
            this._alterSynonymCheckBox.Size = new System.Drawing.Size(102, 18);
            this._alterSynonymCheckBox.TabIndex = 7;
            this._alterSynonymCheckBox.Text = "Alter Synonym";
            this._alterSynonymCheckBox.UseVisualStyleBackColor = true;
            // 
            // _alterLibraryCheckBox
            // 
            this._alterLibraryCheckBox.AutoSize = true;
            this._alterLibraryCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterLibraryCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterLibraryCheckBox.Location = new System.Drawing.Point(6, 167);
            this._alterLibraryCheckBox.Name = "_alterLibraryCheckBox";
            this._alterLibraryCheckBox.Size = new System.Drawing.Size(91, 18);
            this._alterLibraryCheckBox.TabIndex = 5;
            this._alterLibraryCheckBox.Text = "Alter Library";
            this._alterLibraryCheckBox.UseVisualStyleBackColor = true;
            // 
            // _alterViewCheckBox
            // 
            this._alterViewCheckBox.AutoSize = true;
            this._alterViewCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterViewCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterViewCheckBox.Location = new System.Drawing.Point(6, 125);
            this._alterViewCheckBox.Name = "_alterViewCheckBox";
            this._alterViewCheckBox.Size = new System.Drawing.Size(80, 18);
            this._alterViewCheckBox.TabIndex = 5;
            this._alterViewCheckBox.Text = "Alter View";
            this._alterViewCheckBox.UseVisualStyleBackColor = true;
            // 
            // _alterMVGroupCheckBox
            // 
            this._alterMVGroupCheckBox.AutoSize = true;
            this._alterMVGroupCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterMVGroupCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterMVGroupCheckBox.Location = new System.Drawing.Point(6, 104);
            this._alterMVGroupCheckBox.Name = "_alterMVGroupCheckBox";
            this._alterMVGroupCheckBox.Size = new System.Drawing.Size(104, 18);
            this._alterMVGroupCheckBox.TabIndex = 4;
            this._alterMVGroupCheckBox.Text = "Alter MV Group";
            this._alterMVGroupCheckBox.UseVisualStyleBackColor = true;
            // 
            // _alterMVCheckBox
            // 
            this._alterMVCheckBox.AutoSize = true;
            this._alterMVCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterMVCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterMVCheckBox.Location = new System.Drawing.Point(6, 83);
            this._alterMVCheckBox.Name = "_alterMVCheckBox";
            this._alterMVCheckBox.Size = new System.Drawing.Size(140, 18);
            this._alterMVCheckBox.TabIndex = 3;
            this._alterMVCheckBox.Text = "Alter Materialized View";
            this._alterMVCheckBox.UseVisualStyleBackColor = true;
            // 
            // _alterTableCheckBox
            // 
            this._alterTableCheckBox.AutoSize = true;
            this._alterTableCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterTableCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterTableCheckBox.Location = new System.Drawing.Point(6, 41);
            this._alterTableCheckBox.Name = "_alterTableCheckBox";
            this._alterTableCheckBox.Size = new System.Drawing.Size(84, 18);
            this._alterTableCheckBox.TabIndex = 1;
            this._alterTableCheckBox.Text = "Alter Table";
            this._alterTableCheckBox.UseVisualStyleBackColor = true;
            // 
            // _alterCheckBox
            // 
            this._alterCheckBox.AutoSize = true;
            this._alterCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._alterCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterCheckBox.Location = new System.Drawing.Point(6, 20);
            this._alterCheckBox.Name = "_alterCheckBox";
            this._alterCheckBox.Size = new System.Drawing.Size(55, 18);
            this._alterCheckBox.TabIndex = 0;
            this._alterCheckBox.Text = "Alter";
            this._alterCheckBox.UseVisualStyleBackColor = true;
            this._alterCheckBox.CheckedChanged += new System.EventHandler(this._alterCheckBox_CheckedChanged);
            // 
            // _createGroupBox
            // 
            this._createGroupBox.Controls.Add(this._createTriggerCheckBox);
            this._createGroupBox.Controls.Add(this._createSynonymCheckBox);
            this._createGroupBox.Controls.Add(this._createLibraryCheckBox);
            this._createGroupBox.Controls.Add(this._createProcedureCheckBox);
            this._createGroupBox.Controls.Add(this._createViewCheckBox);
            this._createGroupBox.Controls.Add(this._createMVGroupCheckBox);
            this._createGroupBox.Controls.Add(this._createMVCheckBox);
            this._createGroupBox.Controls.Add(this._createTableCheckBox);
            this._createGroupBox.Controls.Add(this._createCheckBox);
            this._createGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createGroupBox.Location = new System.Drawing.Point(13, 12);
            this._createGroupBox.Name = "_createGroupBox";
            this._createGroupBox.Size = new System.Drawing.Size(158, 220);
            this._createGroupBox.TabIndex = 0;
            this._createGroupBox.TabStop = false;
            // 
            // _createTriggerCheckBox
            // 
            this._createTriggerCheckBox.AutoSize = true;
            this._createTriggerCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createTriggerCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createTriggerCheckBox.Location = new System.Drawing.Point(6, 62);
            this._createTriggerCheckBox.Name = "_createTriggerCheckBox";
            this._createTriggerCheckBox.Size = new System.Drawing.Size(102, 18);
            this._createTriggerCheckBox.TabIndex = 2;
            this._createTriggerCheckBox.Text = "Create Trigger";
            this._createTriggerCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createSynonymCheckBox
            // 
            this._createSynonymCheckBox.AutoSize = true;
            this._createSynonymCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createSynonymCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createSynonymCheckBox.Location = new System.Drawing.Point(6, 167);
            this._createSynonymCheckBox.Name = "_createSynonymCheckBox";
            this._createSynonymCheckBox.Size = new System.Drawing.Size(112, 18);
            this._createSynonymCheckBox.TabIndex = 7;
            this._createSynonymCheckBox.Text = "Create Synonym";
            this._createSynonymCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createLibraryCheckBox
            // 
            this._createLibraryCheckBox.AutoSize = true;
            this._createLibraryCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createLibraryCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createLibraryCheckBox.Location = new System.Drawing.Point(6, 188);
            this._createLibraryCheckBox.Name = "_createLibraryCheckBox";
            this._createLibraryCheckBox.Size = new System.Drawing.Size(101, 18);
            this._createLibraryCheckBox.TabIndex = 6;
            this._createLibraryCheckBox.Text = "Create Library";
            this._createLibraryCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createProcedureCheckBox
            // 
            this._createProcedureCheckBox.AutoSize = true;
            this._createProcedureCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createProcedureCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createProcedureCheckBox.Location = new System.Drawing.Point(6, 146);
            this._createProcedureCheckBox.Name = "_createProcedureCheckBox";
            this._createProcedureCheckBox.Size = new System.Drawing.Size(117, 18);
            this._createProcedureCheckBox.TabIndex = 6;
            this._createProcedureCheckBox.Text = "Create Procedure";
            this._createProcedureCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createViewCheckBox
            // 
            this._createViewCheckBox.AutoSize = true;
            this._createViewCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createViewCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createViewCheckBox.Location = new System.Drawing.Point(6, 125);
            this._createViewCheckBox.Name = "_createViewCheckBox";
            this._createViewCheckBox.Size = new System.Drawing.Size(90, 18);
            this._createViewCheckBox.TabIndex = 5;
            this._createViewCheckBox.Text = "Create View";
            this._createViewCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createMVGroupCheckBox
            // 
            this._createMVGroupCheckBox.AutoSize = true;
            this._createMVGroupCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createMVGroupCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createMVGroupCheckBox.Location = new System.Drawing.Point(6, 104);
            this._createMVGroupCheckBox.Name = "_createMVGroupCheckBox";
            this._createMVGroupCheckBox.Size = new System.Drawing.Size(114, 18);
            this._createMVGroupCheckBox.TabIndex = 4;
            this._createMVGroupCheckBox.Text = "Create MV Group";
            this._createMVGroupCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createMVCheckBox
            // 
            this._createMVCheckBox.AutoSize = true;
            this._createMVCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createMVCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createMVCheckBox.Location = new System.Drawing.Point(6, 83);
            this._createMVCheckBox.Name = "_createMVCheckBox";
            this._createMVCheckBox.Size = new System.Drawing.Size(150, 18);
            this._createMVCheckBox.TabIndex = 3;
            this._createMVCheckBox.Text = "Create Materialized View";
            this._createMVCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createTableCheckBox
            // 
            this._createTableCheckBox.AutoSize = true;
            this._createTableCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createTableCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createTableCheckBox.Location = new System.Drawing.Point(6, 41);
            this._createTableCheckBox.Name = "_createTableCheckBox";
            this._createTableCheckBox.Size = new System.Drawing.Size(94, 18);
            this._createTableCheckBox.TabIndex = 1;
            this._createTableCheckBox.Text = "Create Table";
            this._createTableCheckBox.UseVisualStyleBackColor = true;
            // 
            // _createCheckBox
            // 
            this._createCheckBox.AutoSize = true;
            this._createCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._createCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createCheckBox.Location = new System.Drawing.Point(6, 20);
            this._createCheckBox.Name = "_createCheckBox";
            this._createCheckBox.Size = new System.Drawing.Size(65, 18);
            this._createCheckBox.TabIndex = 0;
            this._createCheckBox.Text = "Create";
            this._createCheckBox.UseVisualStyleBackColor = true;
            this._createCheckBox.CheckedChanged += new System.EventHandler(this._createCheckBox_CheckedChanged);
            // 
            // _allUTILITYCheckBox
            // 
            this._allUTILITYCheckBox.AutoSize = true;
            this._allUTILITYCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allUTILITYCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._allUTILITYCheckBox.Location = new System.Drawing.Point(258, 12);
            this._allUTILITYCheckBox.Name = "_allUTILITYCheckBox";
            this._allUTILITYCheckBox.Size = new System.Drawing.Size(90, 18);
            this._allUTILITYCheckBox.TabIndex = 5;
            this._allUTILITYCheckBox.Text = "ALL UTILITY";
            this._allUTILITYCheckBox.UseVisualStyleBackColor = true;
            this._allUTILITYCheckBox.CheckedChanged += new System.EventHandler(this._allUTILITYCheckBox_CheckedChanged);
            // 
            // _utilityGroupBox
            // 
            this._utilityGroupBox.Controls.Add(this._replicateCheckBox);
            this._utilityGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._utilityGroupBox.Location = new System.Drawing.Point(5, 352);
            this._utilityGroupBox.Name = "_utilityGroupBox";
            this._utilityGroupBox.Size = new System.Drawing.Size(514, 55);
            this._utilityGroupBox.TabIndex = 3;
            this._utilityGroupBox.TabStop = false;
            this._utilityGroupBox.Text = "UTILITY Privileges";
            // 
            // _replicateCheckBox
            // 
            this._replicateCheckBox.AutoSize = true;
            this._replicateCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._replicateCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._replicateCheckBox.Location = new System.Drawing.Point(19, 20);
            this._replicateCheckBox.Name = "_replicateCheckBox";
            this._replicateCheckBox.Size = new System.Drawing.Size(76, 18);
            this._replicateCheckBox.TabIndex = 4;
            this._replicateCheckBox.Text = "Replicate";
            this._replicateCheckBox.UseVisualStyleBackColor = true;
            // 
            // SchemaPrivilegeOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.Controls.Add(this._utilityGroupBox);
            this.Controls.Add(this._allUTILITYCheckBox);
            this.Controls.Add(this._ddlGroupBox);
            this.Controls.Add(this._dmlGroupBox);
            this.Controls.Add(this._allDDLCheckBox);
            this.Controls.Add(this._allDMLCheckBox);
            this.Controls.Add(this._allCheckBox);
            this.Name = "SchemaPrivilegeOptionsControl";
            this.Size = new System.Drawing.Size(547, 438);
            this._dmlGroupBox.ResumeLayout(false);
            this._dmlGroupBox.PerformLayout();
            this._ddlGroupBox.ResumeLayout(false);
            this._dropGuiGroupBox.ResumeLayout(false);
            this._dropGuiGroupBox.PerformLayout();
            this._alterGroupBox.ResumeLayout(false);
            this._alterGroupBox.PerformLayout();
            this._createGroupBox.ResumeLayout(false);
            this._createGroupBox.PerformLayout();
            this._utilityGroupBox.ResumeLayout(false);
            this._utilityGroupBox.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _allCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _allDMLCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _allDDLCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _selectCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _insertCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _deleteCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _updateCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _executeCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _referencesCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _dmlGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _ddlGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _createGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createTriggerCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createSynonymCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createProcedureCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createViewCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createMVGroupCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createMVCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _createTableCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _dropGuiGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropTriggerCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropSynonymCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropProcedureCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropViewCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropMVGroupCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropMVCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropTableCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _dropCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _alterGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _alterTriggerCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _alterSynonymCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _alterViewCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _alterMVGroupCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _alterMVCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _alterTableCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _alterCheckBox;
        private Framework.Controls.TrafodionCheckBox _allUTILITYCheckBox;
        private Framework.Controls.TrafodionGroupBox _utilityGroupBox;
        private Framework.Controls.TrafodionCheckBox _replicateCheckBox;
        private Framework.Controls.TrafodionCheckBox _dropLibraryCheckBox;
        private Framework.Controls.TrafodionCheckBox _alterLibraryCheckBox;
        private Framework.Controls.TrafodionCheckBox _createLibraryCheckBox;
        private Framework.Controls.TrafodionCheckBox _usageCheckBox;
    }
}
