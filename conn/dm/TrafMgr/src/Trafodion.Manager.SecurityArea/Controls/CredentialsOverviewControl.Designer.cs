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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class CredentialsOverviewControl
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
            this.caCertificateLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this._caCertLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.selfsignedLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this._selfCertLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._headerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.SuspendLayout();
            // 
            // caCertificateLinkLabel
            // 
            this.caCertificateLinkLabel.AutoSize = true;
            this.caCertificateLinkLabel.Location = new System.Drawing.Point(22, 95);
            this.caCertificateLinkLabel.Name = "caCertificateLinkLabel";
            this.caCertificateLinkLabel.Size = new System.Drawing.Size(107, 13);
            this.caCertificateLinkLabel.TabIndex = 1;
            this.caCertificateLinkLabel.TabStop = true;
            this.caCertificateLinkLabel.Text = "CA Signed Certificate";
            this.caCertificateLinkLabel.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.caCertificateLinkLabel_LinkClicked);
            // 
            // _caCertLabel
            // 
            this._caCertLabel.AutoSize = true;
            this._caCertLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caCertLabel.Location = new System.Drawing.Point(139, 95);
            this._caCertLabel.Name = "_caCertLabel";
            this._caCertLabel.Size = new System.Drawing.Size(483, 13);
            this._caCertLabel.TabIndex = 0;
            this._caCertLabel.Text = ":   Genereate a certificate signing request and deploy the certificate signed by " +
                "Certificate Authority";
            // 
            // selfsignedLinkLabel
            // 
            this.selfsignedLinkLabel.AutoSize = true;
            this.selfsignedLinkLabel.Location = new System.Drawing.Point(22, 58);
            this.selfsignedLinkLabel.Name = "selfsignedLinkLabel";
            this.selfsignedLinkLabel.Size = new System.Drawing.Size(111, 13);
            this.selfsignedLinkLabel.TabIndex = 1;
            this.selfsignedLinkLabel.TabStop = true;
            this.selfsignedLinkLabel.Text = "Self-Signed Certificate";
            this.selfsignedLinkLabel.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.selfsignedLinkLabel_LinkClicked);
            // 
            // _selfCertLabel
            // 
            this._selfCertLabel.AutoSize = true;
            this._selfCertLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._selfCertLabel.Location = new System.Drawing.Point(139, 58);
            this._selfCertLabel.Name = "_selfCertLabel";
            this._selfCertLabel.Size = new System.Drawing.Size(256, 13);
            this._selfCertLabel.TabIndex = 0;
            this._selfCertLabel.Text = ":   Replace the self-signed certificate on the platfom";
            // 
            // _headerLabel
            // 
            this._headerLabel.AutoSize = true;
            this._headerLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._headerLabel.Location = new System.Drawing.Point(22, 17);
            this._headerLabel.Name = "_headerLabel";
            this._headerLabel.Size = new System.Drawing.Size(725, 16);
            this._headerLabel.TabIndex = 0;
            this._headerLabel.Text = "You have the option to use either a self-signed certificate or a certificate sign" +
                "ed by a Certificate Authority (CA) of your choice.";
            // 
            // CredentialsOverviewControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.caCertificateLinkLabel);
            this.Controls.Add(this._caCertLabel);
            this.Controls.Add(this.selfsignedLinkLabel);
            this.Controls.Add(this._selfCertLabel);
            this.Controls.Add(this._headerLabel);
            this.Name = "CredentialsOverviewControl";
            this.Size = new System.Drawing.Size(772, 345);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _headerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _selfCertLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel selfsignedLinkLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _caCertLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel caCertificateLinkLabel;
    }
}
