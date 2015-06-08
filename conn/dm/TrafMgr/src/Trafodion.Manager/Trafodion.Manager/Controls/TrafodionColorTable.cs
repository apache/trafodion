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
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    public class TrafodionColorTable : ProfessionalColorTable
    {
        #region ColorTable Overrides

        public override Color ButtonCheckedGradientBegin
        {
            get { return ButtonCheckedGradientBeginColor; }
        }
        public override Color ButtonCheckedGradientEnd
        {
            get { return ButtonCheckedGradientEndColor; }
        }
        public override Color ButtonCheckedGradientMiddle
        {
            get { return ButtonCheckedGradientMiddleColor; }
        }
        public override Color ButtonCheckedHighlight
        {
            get { return ButtonCheckedHighlightColor; }
        }
        public override Color ButtonCheckedHighlightBorder
        {
            get { return ButtonCheckedHighlightBorderColor; }
        }
        public override Color ButtonPressedBorder
        {
            get { return ButtonPressedBorderColor; }
        }
        public override Color ButtonPressedGradientBegin
        {
            get { return ButtonPressedGradientBeginColor; }
        }
        public override Color ButtonPressedGradientEnd
        {
            get { return ButtonPressedGradientEndColor; }
        }
        public override Color ButtonPressedGradientMiddle
        {
            get { return ButtonPressedGradientMiddleColor; }
        }
        public override Color ButtonPressedHighlight
        {
            get { return ButtonPressedHighlightColor; }
        }
        public override Color ButtonPressedHighlightBorder
        {
            get { return ButtonPressedHighlightBorderColor; }
        }
        public override Color ButtonSelectedBorder
        {
            get { return ButtonSelectedBorderColor; }
        }
        public override Color ButtonSelectedGradientBegin
        {
            get { return ButtonSelectedGradientBeginColor; }
        }
        public override Color ButtonSelectedGradientEnd
        {
            get { return ButtonSelectedGradientEndColor; }
        }
        public override Color ButtonSelectedGradientMiddle
        {
            get { return ButtonSelectedGradientMiddleColor; }
        }
        public override Color ButtonSelectedHighlight
        {
            get { return ButtonSelectedHighlightColor; }
        }
        public override Color ButtonSelectedHighlightBorder
        {
            get { return ButtonSelectedHighlightBorderColor; }
        }
        public override Color CheckBackground
        {
            get { return CheckBackgroundColor; }
        }
        public override Color CheckPressedBackground
        {
            get { return CheckPressedBackgroundColor; }
        }
        public override Color CheckSelectedBackground
        {
            get { return CheckSelectedBackgroundColor; }
        }
        public override Color GripDark
        {
            get { return GripDarkColor; }
        }
        public override Color GripLight
        {
            get { return GripLightColor; }
        }
        public override Color ImageMarginGradientBegin
        {
            get { return ImageMarginGradientBeginColor; }
        }
        public override Color ImageMarginGradientEnd
        {
            get { return ImageMarginGradientEndColor; }
        }
        public override Color ImageMarginGradientMiddle
        {
            get { return ImageMarginGradientMiddleColor; }
        }
        public override Color ImageMarginRevealedGradientBegin
        {
            get { return ImageMarginRevealedGradientBeginColor; }
        }
        public override Color ImageMarginRevealedGradientEnd
        {
            get { return ImageMarginRevealedGradientEndColor; }
        }
        public override Color ImageMarginRevealedGradientMiddle
        {
            get { return ImageMarginRevealedGradientMiddleColor; }
        }
        public override Color MenuBorder
        {
            get { return MenuBorderColor; }
        }
        public override Color MenuItemBorder
        {
            get { return MenuItemBorderColor; }
        }
        public override Color MenuItemPressedGradientBegin
        {
            get { return MenuItemPressedGradientBeginColor; }
        }
        public override Color MenuItemPressedGradientEnd
        {
            get { return MenuItemPressedGradientEndColor; }
        }
        public override Color MenuItemPressedGradientMiddle
        {
            get { return MenuItemPressedGradientMiddleColor; }
        }
        public override Color MenuItemSelected
        {
            get { return MenuItemSelectedColor; }
        }
        public override Color MenuItemSelectedGradientBegin
        {
            get { return MenuItemSelectedGradientBeginColor; }
        }
        public override Color MenuItemSelectedGradientEnd
        {
            get { return MenuItemSelectedGradientEndColor; }
        }
        public override Color MenuStripGradientBegin
        {
            get { return MenuStripGradientBeginColor; }
        }
        public override Color MenuStripGradientEnd
        {
            get { return MenuStripGradientEndColor; }
        }
        public override Color OverflowButtonGradientBegin
        {
            get { return OverflowButtonGradientBeginColor; }
        }
        public override Color OverflowButtonGradientEnd
        {
            get { return OverflowButtonGradientEndColor; }
        }
        public override Color OverflowButtonGradientMiddle
        {
            get { return OverflowButtonGradientMiddleColor; }
        }
        public override Color RaftingContainerGradientBegin
        {
            get { return RaftingContainerGradientBeginColor; }
        }
        public override Color RaftingContainerGradientEnd
        {
            get { return RaftingContainerGradientEndColor; }
        }
        public override Color SeparatorDark
        {
            get { return SeparatorDarkColor; }
        }
        public override Color SeparatorLight
        {
            get { return SeparatorLightColor; }
        }
        public override Color StatusStripGradientBegin
        {
            get { return StatusStripGradientBeginColor; }
        }
        public override Color StatusStripGradientEnd
        {
            get { return StatusStripGradientEndColor; }
        }
        public override Color ToolStripBorder
        {
            get { return ToolStripBorderColor; }
        }
        public override Color ToolStripContentPanelGradientBegin
        {
            get { return ToolStripContentPanelGradientBeginColor; }
        }
        public override Color ToolStripContentPanelGradientEnd
        {
            get { return ToolStripContentPanelGradientEndColor; }
        }
        public override Color ToolStripDropDownBackground
        {
            get { return ToolStripDropDownBackgroundColor; }
        }
        public override Color ToolStripGradientBegin
        {
            get { return ToolStripGradientBeginColor; }
        }
        public override Color ToolStripGradientEnd
        {
            get { return ToolStripGradientEndColor; }
        }
        public override Color ToolStripGradientMiddle
        {
            get { return ToolStripGradientMiddleColor; }
        }
        public override Color ToolStripPanelGradientBegin
        {
            get { return ToolStripPanelGradientBeginColor; }
        }
        public override Color ToolStripPanelGradientEnd
        {
            get { return ToolStripPanelGradientEndColor; }
        }
        #endregion ColorTable Overrides

        #region ColorTable Statics

        public static bool IsXPStyle
        {
            get
            {
                return System.Windows.Forms.VisualStyles.VisualStyleInformation.DisplayName.Contains("XP");
            }
        }

        public static Color ButtonCheckedGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonCheckedGradientBegin : Color.FromArgb(255, 255, 223, 154); }
        }
        public static Color ButtonCheckedGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonCheckedGradientEnd : Color.FromArgb(255, 255, 166, 76); }
        }
        public static Color ButtonCheckedGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonCheckedGradientMiddle : Color.FromArgb(255, 255, 195, 116); }
        }
        public static Color ButtonCheckedHighlightColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonCheckedHighlight : Color.FromArgb(255, 195, 211, 237); }
        }
        public static Color ButtonCheckedHighlightBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonCheckedHighlightBorder : Color.FromArgb(255, 49, 106, 197); }
        }
        public static Color ButtonPressedBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonPressedBorder : Color.FromArgb(255, 0, 0, 128); }
        }
        public static Color ButtonPressedGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonPressedGradientBegin : Color.FromArgb(255, 254, 128, 62); }
        }
        public static Color ButtonPressedGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonPressedGradientEnd : Color.FromArgb(255, 255, 223, 154); }
        }
        public static Color ButtonPressedGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonPressedGradientMiddle : Color.FromArgb(255, 255, 177, 109); }
        }
        public static Color ButtonPressedHighlightColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonPressedHighlight : Color.FromArgb(255, 150, 179, 225); }
        }
        public static Color ButtonPressedHighlightBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonPressedHighlightBorder : Color.FromArgb(255, 49, 106, 197); }
        }
        public static Color ButtonSelectedBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonSelectedBorder : Color.FromArgb(255, 0, 0, 128); }
        }
        public static Color ButtonSelectedGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonSelectedGradientBegin : Color.FromArgb(255, 255, 255, 222); }
        }
        public static Color ButtonSelectedGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonSelectedGradientEnd : Color.FromArgb(255, 255, 203, 136); }
        }
        public static Color ButtonSelectedGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonSelectedGradientMiddle : Color.FromArgb(255, 255, 225, 172); }
        }
        public static Color ButtonSelectedHighlightColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonSelectedHighlight : Color.FromArgb(255, 195, 211, 237); }
        }
        public static Color ButtonSelectedHighlightBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.ButtonSelectedHighlightBorder : Color.FromArgb(255, 0, 0, 128); }
        }
        public static Color CheckBackgroundColor
        {
            get { return IsXPStyle ? ProfessionalColors.CheckBackground : Color.FromArgb(255, 255, 192, 111); }
        }
        public static Color CheckPressedBackgroundColor
        {
            get { return IsXPStyle ? ProfessionalColors.CheckPressedBackground : Color.FromArgb(255, 254, 128, 62); }
        }
        public static Color CheckSelectedBackgroundColor
        {
            get { return IsXPStyle ? ProfessionalColors.CheckSelectedBackground : Color.FromArgb(255, 254, 128, 62); }
        }
        public static Color GripDarkColor
        {
            get { return IsXPStyle ? ProfessionalColors.GripDark : Color.FromArgb(255, 39, 65, 118); }
        }
        public static Color GripLightColor
        {
            get { return IsXPStyle ? ProfessionalColors.GripLight : Color.FromArgb(255, 255, 255, 255); }
        }
        public static Color ImageMarginGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ImageMarginGradientBegin : Color.FromArgb(255, 227, 239, 255); }
        }
        public static Color ImageMarginGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ImageMarginGradientEnd : Color.FromArgb(255, 123, 164, 224); }
        }
        public static Color ImageMarginGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.ImageMarginGradientMiddle : Color.FromArgb(255, 203, 225, 252); }
        }
        public static Color ImageMarginRevealedGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ImageMarginRevealedGradientBegin : Color.FromArgb(255, 203, 221, 246); }
        }
        public static Color ImageMarginRevealedGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ImageMarginRevealedGradientEnd : Color.FromArgb(255, 114, 155, 215); }
        }
        public static Color ImageMarginRevealedGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.ImageMarginRevealedGradientMiddle : Color.FromArgb(255, 161, 197, 249); }
        }
        public static Color MenuBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuBorder : Color.FromArgb(255, 0, 45, 150); }
        }
        public static Color MenuItemBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuItemBorder : Color.FromArgb(255, 0, 0, 128); }
        }
        public static Color MenuItemPressedGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuItemPressedGradientBegin : Color.FromArgb(255, 227, 239, 255); }
        }
        public static Color MenuItemPressedGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuItemPressedGradientEnd : Color.FromArgb(255, 123, 164, 224); }
        }
        public static Color MenuItemPressedGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuItemPressedGradientMiddle : Color.FromArgb(255, 161, 197, 249); }
        }
        public static Color MenuItemSelectedColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuItemSelected : Color.FromArgb(255, 255, 238, 194); }
        }
        public static Color MenuItemSelectedGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuItemSelectedGradientBegin : Color.FromArgb(255, 255, 255, 222); }
        }
        public static Color MenuItemSelectedGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuItemSelectedGradientEnd : Color.FromArgb(255, 255, 203, 136); }
        }
        public static Color MenuStripGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuStripGradientBegin : Color.FromArgb(255, 158, 190, 245); }
        }
        public static Color MenuStripGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.MenuStripGradientEnd : Color.FromArgb(255, 196, 218, 250); }
        }
        public static Color OverflowButtonGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.OverflowButtonGradientBegin : Color.FromArgb(255, 127, 177, 250); }
        }
        public static Color OverflowButtonGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.OverflowButtonGradientEnd : Color.FromArgb(255, 0, 53, 145); }
        }
        public static Color OverflowButtonGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.OverflowButtonGradientMiddle : Color.FromArgb(255, 82, 127, 208); }
        }
        public static Color RaftingContainerGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.RaftingContainerGradientBegin : Color.FromArgb(255, 158, 190, 245); }
        }
        public static Color RaftingContainerGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.RaftingContainerGradientEnd : Color.FromArgb(255, 196, 218, 250); }
        }
        public static Color SeparatorDarkColor
        {
            get { return IsXPStyle ? ProfessionalColors.SeparatorDark : Color.FromArgb(255, 106, 140, 203); }
        }
        public static Color SeparatorLightColor
        {
            get { return IsXPStyle ? ProfessionalColors.SeparatorLight : Color.FromArgb(255, 241, 249, 255); }
        }
        public static Color StatusStripGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.StatusStripGradientBegin : Color.FromArgb(255, 158, 190, 245); }
        }
        public static Color StatusStripGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.StatusStripGradientEnd : Color.FromArgb(255, 196, 218, 250); }
        }
        public static Color ToolStripBorderColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripBorder : Color.FromArgb(255, 59, 97, 156); }
        }
        public static Color ToolStripContentPanelGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripContentPanelGradientBegin : Color.FromArgb(255, 158, 190, 245); }
        }
        public static Color ToolStripContentPanelGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripContentPanelGradientEnd : Color.FromArgb(255, 196, 218, 250); }
        }
        public static Color ToolStripDropDownBackgroundColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripDropDownBackground : Color.FromArgb(255, 246, 246, 246); }
        }
        public static Color ToolStripGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripGradientBegin : Color.FromArgb(255, 227, 239, 255); }
        }
        public static Color ToolStripGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripGradientEnd : Color.FromArgb(255, 123, 164, 224); }
        }
        public static Color ToolStripGradientMiddleColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripGradientMiddle : Color.FromArgb(255, 203, 225, 252); }
        }
        public static Color ToolStripPanelGradientBeginColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripPanelGradientBegin : Color.FromArgb(255, 158, 190, 245); }
        }
        public static Color ToolStripPanelGradientEndColor
        {
            get { return IsXPStyle ? ProfessionalColors.ToolStripPanelGradientEnd : Color.FromArgb(255, 196, 218, 250); }
        }
        #endregion ColorTable Statics

        #region SystemColors

        public static Color ActiveBorder 
        { 
            get { return IsXPStyle ? SystemColors.ActiveBorder : Color.FromArgb(255, 212, 208, 200); } 
        }
        public static Color ActiveCaption 
        { 
            get { return IsXPStyle ? SystemColors.ActiveCaption : Color.FromArgb(255, 0, 84, 227); } 
        }
        public static Color ActiveCaptionText 
        { 
            get { return IsXPStyle ? SystemColors.ActiveCaptionText : Color.FromArgb(255, 255, 255, 255); } 
        }
        public static Color AppWorkspace 
        { 
            get { return IsXPStyle ? SystemColors.AppWorkspace : Color.FromArgb(255, 128, 128, 128); } 
        }
        public static Color ButtonFace 
        { 
            get { return IsXPStyle ? SystemColors.ButtonFace : Color.FromArgb(255, 236, 233, 216); } 
        }
        public static Color ButtonHighlight 
        { 
            get { return IsXPStyle ? SystemColors.ButtonHighlight : Color.FromArgb(255, 255, 255, 255); } 
        }
        public static Color ButtonShadow 
        { 
            get { return IsXPStyle ? SystemColors.ButtonShadow : Color.FromArgb(255, 172, 168, 153); } 
        }
        public static Color Control 
        { 
            get { return IsXPStyle ? SystemColors.Control : Color.FromArgb(255, 236, 233, 216); } 
        }
        public static Color ControlDark 
        { 
            get { return IsXPStyle ? SystemColors.ControlDark : Color.FromArgb(255, 172, 168, 153); } 
        }
        public static Color ControlDarkDark 
        { 
            get { return IsXPStyle ? SystemColors.ControlDarkDark : Color.FromArgb(255, 113, 111, 100); } 
        }
        public static Color ControlLight 
        { 
            get { return IsXPStyle ? SystemColors.ControlLight : Color.FromArgb(255, 241, 239, 226); } 
        }
        public static Color ControlLightLight 
        { 
            get { return IsXPStyle ? SystemColors.ControlLightLight : Color.FromArgb(255, 255, 255, 255); } 
        }
        public static Color ControlText 
        { 
            get { return IsXPStyle ? SystemColors.ControlText : Color.FromArgb(255, 0, 0, 0); } 
        }
        public static Color Desktop 
        { 
            get { return IsXPStyle ? SystemColors.Desktop : Color.FromArgb(255, 0, 78, 152); } 
        }
        public static Color GradientActiveCaption 
        { 
            get { return IsXPStyle ? SystemColors.GradientActiveCaption : Color.FromArgb(255, 61, 149, 255); } 
        }
        public static Color GradientInactiveCaption 
        { 
            get { return IsXPStyle ? SystemColors.GradientInactiveCaption : Color.FromArgb(255, 157, 185, 235); } 
        }
        public static Color GrayText 
        { 
            get { return IsXPStyle ? SystemColors.GrayText : Color.FromArgb(255, 172, 168, 153); } 
        }
        public static Color Highlight 
        { 
            get { return IsXPStyle ? SystemColors.Highlight : Color.FromArgb(255, 49, 106, 197); } 
        }
        public static Color HighlightText 
        { 
            get { return IsXPStyle ? SystemColors.HighlightText : Color.FromArgb(255, 255, 255, 255); } 
        }
        public static Color HotTrack 
        { 
            get { return IsXPStyle ? SystemColors.HotTrack : Color.FromArgb(255, 0, 0, 128); } 
        }
        public static Color InactiveBorder 
        { 
            get { return IsXPStyle ? SystemColors.InactiveBorder : Color.FromArgb(255, 212, 208, 200); } 
        }
        public static Color InactiveCaption 
        { 
            get { return IsXPStyle ? SystemColors.InactiveCaption : Color.FromArgb(255, 122, 150, 223); } 
        }
        public static Color InactiveCaptionText 
        { 
            get { return IsXPStyle ? SystemColors.InactiveCaptionText : Color.FromArgb(255, 216, 228, 248); } 
        }
        public static Color Info 
        { 
            get { return IsXPStyle ? SystemColors.Info : Color.FromArgb(255, 255, 255, 225); } 
        }
        public static Color InfoText 
        { 
            get { return IsXPStyle ? SystemColors.InfoText : Color.FromArgb(255, 0, 0, 0); } 
        }
        public static Color Menu 
        { 
            get { return IsXPStyle ? SystemColors.Menu : Color.FromArgb(255, 255, 255, 255); } 
        }
        public static Color MenuBar 
        { 
            get { return IsXPStyle ? SystemColors.MenuBar : Color.FromArgb(255, 236, 233, 216); } 
        }
        public static Color MenuHighlight 
        { 
            get { return IsXPStyle ? SystemColors.MenuHighlight : Color.FromArgb(255, 49, 106, 197); } 
        }
        public static Color MenuText 
        { 
            get { return IsXPStyle ? SystemColors.MenuText : Color.FromArgb(255, 0, 0, 0); } 
        }
        public static Color ScrollBar 
        { 
            get { return IsXPStyle ? SystemColors.ScrollBar : Color.FromArgb(255, 212, 208, 200); } 
        }
        public static Color Window 
        { 
            get { return IsXPStyle ? SystemColors.Window : Color.FromArgb(255, 255, 255, 255); } 
        }
        public static Color WindowFrame 
        { 
            get { return IsXPStyle ? SystemColors.WindowFrame : Color.FromArgb(255, 0, 0, 0); } 
        }
        public static Color WindowText 
        { 
            get { return IsXPStyle ? SystemColors.WindowText : Color.FromArgb(255, 0, 0, 0); } 
        }

        #endregion SystemColors
    }
}
