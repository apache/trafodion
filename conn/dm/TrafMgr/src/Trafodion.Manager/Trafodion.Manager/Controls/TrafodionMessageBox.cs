//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//

using System;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;
using HPOneVoice;

namespace Trafodion.Manager.Framework.Controls
{
  /// <summary>
  /// Wrapper class extending the HPMessageBox class. It has hooks to change the look and feel
  /// when the look and feel of the framework is changed.
  /// </summary>
    public abstract class TrafodionMessageBox : HPMessageBox
    {
        /// <summary>
        /// Look and Feel change handleer
        /// </summary>
        private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;


        /// <summary>
        /// Converts Windows.Form.MessageBoxIcon enum used in the caller into HPOneVoice.MessageBoxIcon enum
        /// </summary>
        /// <param name="icon">System.Windows.Forms.MessageBoxIcon enum</param>
        /// <returns>HPOneVoice.HPMessageBoxIcon</returns>
        private static HPMessageBoxIcon EnumConverter(MessageBoxIcon icon)
        {
            // return value variable
            HPMessageBoxIcon HpIconEnum = HPMessageBoxIcon.NONE;

            switch (icon)
            {
                //
                // Summary:
                //     The message box contain no symbols.
                case MessageBoxIcon.None:
                    HpIconEnum = HPMessageBoxIcon.NONE;
                    break;
                //
                // Summary:
                //     The message box contains a symbol consisting of white X in a circle with
                //     a red background.
                case MessageBoxIcon.Error:  //MessageBoxIcon.Hand; MessageBoxIcon.Stop:
                    HpIconEnum = HPMessageBoxIcon.ERROR;
                    break;
                //
                // Summary:
                //     The message box contains a symbol consisting of a question mark in a circle.
                case MessageBoxIcon.Question:
                    HpIconEnum = HPMessageBoxIcon.QUESTION;
                    break;
                //
                // Summary:
                //     The message box contains a symbol consisting of an exclamation point in a
                //     triangle with a yellow background.
                case MessageBoxIcon.Warning:    //MessageBoxIcon.Exclamation
                    HpIconEnum = HPMessageBoxIcon.WARNING;
                    break;
                //
                // Summary:
                //     The message box contains a symbol consisting of a lowercase letter i in a
                //     circle.
                case MessageBoxIcon.Information:    //MessageBoxIcon.Asterisk
                    HpIconEnum = HPMessageBoxIcon.INFORMATION;
                    break;
                //
                // Summary:
                //     Invalid icon name, return error!!.
                default:
                    HpIconEnum = HPMessageBoxIcon.NONE;
                    break;

            }

            return HpIconEnum;

        }

        /// <summary>
        /// Overloaded. Displays a message box with specified text, caption, buttons, and icon.
        /// </summary>
        /// <param name="text">(string)</param>
        /// <param name="caption">(string)</param>
        /// <param name="buttons">(MessageBoxButtons)</param>
        /// <param name="icon">(System.Windows.Forms.MessageBoxIcon)</param>
        /// <returns>DialogResult</returns>
        // this method is used to take care of the caller pass in System.Windows.Forms.MessageBoxIcon enum that
        // is not same values used for HPOneVoice.MessageBox
        public static DialogResult Show (string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon)
        {
            return HPMessageBox.Show(Utilities.GetForegroundControl(), text, caption, buttons, EnumConverter(icon));
        }

        /// <summary>
        /// Overloaded. Displays a message box in front of the specified object and with the specified text, caption, buttons, and icon.
        /// </summary>
        /// <param name="owner">(IWin32Window)</param>
        /// <param name="text">(string)</param>
        /// <param name="caption">(string)</param>
        /// <param name="buttons">(MessageBoxButtons)</param>
        /// <param name="icon">(System.Windows.Forms.MessageBoxIcon)</param>
        /// <returns>DialogResult</returns>
        // this method is used to take care of the caller pass in System.Windows.Forms.MessageBoxIcon enum that
        // is not same values used for HPOneVoice.MessageBox
        public static DialogResult Show(IWin32Window owner, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon)
        {
            return HPMessageBox.Show(owner, text, caption, buttons, EnumConverter(icon));
        }


        /// <summary>
        /// Overloaded. Displays a message box with the specified text, caption, buttons, icon, and default button.
        /// </summary>
        /// <param name="text">(string)</param>
        /// <param name="caption">(string)</param>
        /// <param name="buttons">(MessageBoxButtons)</param>
        /// <param name="icon">(System.Windows.Forms.MessageBoxIcon)</param>
        /// <param name="defaultButton">(MessageBoxDefaultButtons)</param>
        /// <returns></returns>
        // this method is used to take care of the caller pass in System.Windows.Forms.MessageBoxIcon enum that
        // is not same values used for HPOneVoice.MessageBox
        public static DialogResult Show(string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton)
        {
            return HPMessageBox.Show(text, caption, buttons, EnumConverter(icon), defaultButton);
        }

        
        /// <summary>
        /// Overloaded. Displays a message box in front of the specified object and with the specified text, caption, buttons, icon, and default button.
        /// </summary>
        /// <param name="owner">(IWin32Window)</param>
        /// <param name="text">(string)</param>
        /// <param name="caption">(string)</param>
        /// <param name="buttons">(MessageBoxButtons)</param>
        /// <param name="icon">(System.Windows.Forms.MessageBoxIcon)</param>
        /// <param name="defaultButton">(MessageBoxDefaultButtons)</param>
        /// <returns>DialogResult</returns>
        // this method is used to take care of the caller pass in System.Windows.Forms.MessageBoxIcon enum that
        // is not same values used for HPOneVoice.MessageBox
        public static DialogResult Show(IWin32Window owner, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton)
        {
            return HPMessageBox.Show(owner, text, caption, buttons, EnumConverter(icon), defaultButton);
        }
    }    

}
