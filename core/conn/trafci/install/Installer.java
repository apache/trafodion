//
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
//

import java.awt.BorderLayout;
import java.awt.CardLayout;
import java.awt.Color;
import java.awt.Container;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilePermission;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintStream;
import java.io.StringReader;
import java.io.StringWriter;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Enumeration;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import javax.swing.*;
import javax.swing.border.Border;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.filechooser.FileFilter;

public class Installer
{
   /* change this to true when debugging */
   private boolean DEBUG = false; 
   
   static String MANIFEST = "META-INF/MANIFEST.MF";
   private static final String vprocStr = "@@@VPROC@@@";
   private String myClassName;

   final static String productName="trafci";
   final static String productTitle="Trafodion Command Interface";
   final static String productScriptName="trafci";
      
   private static String cardCurrent = "";
   private static boolean standardInstall = true;
   private static boolean silentInstall = false;
   private static boolean enableOpenSource = false;
   final static String cardInstallerChoices = "INSTALLER CHOICES";
   final static String cardInstallerPath = "INSTALLER PATHS";
   final static String cardOptionalComponents = "OPTIONAL COMPONENTS";
   final static String cardLegal = "LEGAL DISCLAIMER";
   final static String cardInstallationComplete = "INSTALLATION COMPLETE";
   
   final static String ERROR_INVALID_NCI_DIR = "Invalid " + productTitle + " Installation Directory"; 
   
   final String NEW_JDBC = "JDBC Type 4 Driver JAR File";
   final String NEW_JDBC_PKG = "org/trafodion/jdbc/t4/";
   
   String installerFilesStartWith="$";
   String productJarFileName="trafci.jar";
   String productJarLoc="lib";
   String productJarFile=productJarLoc+File.separator+productJarFileName;
   String classVarToReplace="##TRAFCI_CLASSPATH##";
   String perlClassVarToReplace="##TRAFCI_PERL_CLASSPATH##";
   String perlLibPathToReplace = "##TRAFCI_PERL_LIB_CLASSPATH##";
   String pythonClassVarToReplace="##TRAFCI_PYTHON_CLASSPATH##";
   String pythonLibPathToReplace = "##TRAFCI_PYTHON_LIB_CLASSPATH##";
   String jythonJarFile="jython.jar";
   String perlJarFile="JavaServer.jar";
   String t4jdbcIdentifier= NEW_JDBC_PKG + "TrafT4PreparedStatement.class";
   String ciJarIdentifier="org/trafodion/ci/Vproc.class"; 
   BufferedReader bufReader= new BufferedReader(new InputStreamReader(System.in));

   private static String nixDefaultInstallDir="/usr/local/";
   private static String wDefaultInstallDir="\\Trafodion\\" + productTitle;
   private static String defaultInstallDir=wDefaultInstallDir;
   
   private static String wDefaultInstallJDBC="";
   private static String nixDefaultInstallJDBC="/usr/local/trafci/jdbcT4.jar";
   private static String defaultInstallJDBC=wDefaultInstallJDBC;
   
   static String jdbcFileLoc=null;
   static String targetDirectory=null;
   static String osName=null;
   
   String defaultPerlJavaURL = "http://search.cpan.org/src/METZZO/Java-4.7/";
   String defaultPerlSAXURL = "http://search.cpan.org/src/KMACLEOD/libxml-perl-0.08/lib/XML/Parser/PerlSAX.pm";
   String defaultPythonJythonURL = "http://fr.sourceforge.jp/frs/g_redir.php?m=jaist&f=%2Fjython%2Fjython%2F2.2%2Fjython_installer-2.2.jar";
   

   public class InstallerGUIMode extends JFrame {

	  private static final long serialVersionUID = -8158111325005975799L;
	  JTextField file_txt =null;
      JButton fileBrowseButton =null;
      JButton dirBrowseButton =null;
      JButton dirBrowseButtonOSS =null;
      JButton choiceStandardInstall =null;
      JButton choiceCoreComponents =null;
      JButton choiceOptionalButton =null;
      JButton nextButtonInstallerPath =null;
      JButton nextButtonOptionalComponents =null;
      JButton autoDetectProxy =null;
      JButton nextButtonLegal =null;
      JButton finishedButton =null;
      JButton perlJavaURL =null;
      JButton perlSAXURL =null;
      JButton pythonJythonURL =null;
      
      JFileChooser filechooser=null;
      JTextField jdbcFileText =null;
      JTextField targetDirText =null;
      JTextField ossTargetDirText =null;
      JTextField proxyServerText =null;
      JTextField proxyPortText =null;
      ProxyBox proxyCombo =null;
      
      JCheckBox checkRequiredComponents =null;
      JCheckBox checkPerl =null;
      JCheckBox checkPython =null;
      JCheckBox checkPerlSAX =null;
      JCheckBox checkLegal =null;
      
      ButtonGroup proxyButtonGroup =null;
      JRadioButton noProxyRadio =null;
      JRadioButton useProxyRadio =null;
      
      JLabel statusLabel =null;
      JLabel proxyPortLabel =null;
      JLabel proxyServerLabel =null;
      JLabel downloadLabel =null;
      
      JPanel panelProxy =null;
      JPanel panelDownload =null;
      JPanel panelComponentsProxyContainer =null;
      JPanel finishedContainer =null;
      JPanel headPanelFinished =null;
      JPanel controlPanelFinished =null;
      JPanel targetSelectPanel =null;
      JPanel legalPanelContainer =null;
      JPanel pathHolder =null;
      JPanel optComponentContainer =null;
      JPanel panelComponents =null;
      JProgressBar progressBar =null;
      JTextArea downloadTextArea =null;

      Container content = getContentPane();

      boolean downloadError = false;
      
      int getJDBCLocation()
      {
         JFileChooser t4c = new JFileChooser();
         t4c.setCurrentDirectory(new File("."));
         t4c.setDialogTitle("Select " + NEW_JDBC);
         t4c.setMultiSelectionEnabled(false);
         t4c.setFileSelectionMode(JFileChooser.FILES_ONLY);
         t4c.setBackground(new Color(230,228,223));

         File jdbcFile =null;
         boolean jarFound = false;

         t4c.addChoosableFileFilter(new nameFilter());
         t4c.setAcceptAllFileFilterUsed(false);

         while (true)
         {
            if (t4c.showDialog(InstallerGUIMode.this, "Select")
               != JFileChooser.APPROVE_OPTION)
            {
               return JFileChooser.CANCEL_OPTION;  //only when user select valid dir, it can return approve_option
            }
            jdbcFile = t4c.getSelectedFile();
            if (jdbcFile.getName().endsWith(".jar") && jdbcFile.canRead())
            {
               try
               {
                  ZipFile zf = new ZipFile(jdbcFile);
                  int size = zf.size();
                  Enumeration<?> entries = zf.entries();
                  for (int i=0; i<size; i++)
                  {
                     ZipEntry entry = (ZipEntry) entries.nextElement();
                     if (!entry.isDirectory() && entry.getName().equals(t4jdbcIdentifier))
                     {
                        jarFound= true;
                        break;
                     }
                  }
               } catch (Exception e)
               {
               }
            }
            if (jarFound)
            {
               break;
            }
            else
               JOptionPane.showMessageDialog(null,"Invalid JDBC jar file specified", "Error", JOptionPane.ERROR_MESSAGE);
         }
         jdbcFileLoc=jdbcFile.getAbsolutePath();
         return JFileChooser.APPROVE_OPTION;
      }

      int getTargetInstallLocation()
      {
         JFileChooser installDir = new JFileChooser();
         installDir.setCurrentDirectory(new File("."));
         installDir.setDialogTitle("Select "+productTitle+" Installation Directory ");
         installDir.setMultiSelectionEnabled(false);
         installDir.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);


         File installDirFile =null;

         installDir.addChoosableFileFilter(new dirFilter());
         installDir.setAcceptAllFileFilterUsed(false);
         while (true)
         {
            if (installDir.showDialog(InstallerGUIMode.this, "Select")
               != JFileChooser.APPROVE_OPTION)
            {
               return JFileChooser.CANCEL_OPTION;  //only when user select valid dir, it can return approve_option
            }
            installDirFile = installDir.getSelectedFile();
            
            if (installDirFile.isDirectory() && installDirFile.exists() && installDirFile.canWrite())
            {
               if(cardCurrent == cardLegal && !validateNCIPath(installDirFile.getAbsolutePath()))
                   JOptionPane.showMessageDialog(null,ERROR_INVALID_NCI_DIR, "Error", JOptionPane.ERROR_MESSAGE);
               else{    
                   targetDirectory=installDirFile.getAbsolutePath();
                   if (!targetDirectory.equalsIgnoreCase(wDefaultInstallDir))
                   {
                	   if (osName.startsWith("window"))
                	   {
                		   if(!targetDirectory.endsWith(("\\")))
                			   targetDirectory +="\\";              	   
                	   
                	   }
                	   else
                	   {
                		   if(!targetDirectory.endsWith(("/")))
                               targetDirectory +="/";                  	   
                    	   
                	   }
                	   if(cardCurrent == cardInstallerPath)
                	       targetDirectory += productName;
                   }

                   break;
               }
            }
            else
               JOptionPane.showMessageDialog(null,"Could not find the directory specified or invalid permissions ", "Error", JOptionPane.ERROR_MESSAGE);
         }
         return JFileChooser.APPROVE_OPTION;
      }

      class nameFilter extends FileFilter
      {

         public boolean accept(File file)
         {
        	
            boolean jarFound=false;
            if (file.isDirectory() && file.canRead()) 
               return true;

            if (file.getName().endsWith(".jar") && file.canRead())
            {
               try
               {
                  ZipFile zf = new ZipFile(file);
                  int size = zf.size();
                  Enumeration<?> entries = zf.entries();
                  for (int i=0; i<size; i++)
                  {
                     ZipEntry entry = (ZipEntry) entries.nextElement();
                     if (!entry.isDirectory() && entry.getName().equals(t4jdbcIdentifier))
                        jarFound=true;
                  }
               } catch (Exception e)
               {
               }
            }
            if (jarFound) return true;
            else return false;
         }

         //The description of this filter
         public String getDescription()
         {
            return "JAR File";
         }
      }

      class dirFilter extends FileFilter
      {
         public boolean accept(File file)
         {
            if (file.exists() && file.isDirectory() && file.canWrite())
               return true;
            else
               return false;
         }

         //The description of this filter
         public String getDescription()
         {
            return "Installation Directory";
         }
      }

    @SuppressWarnings("unused")
	void process(String zipfile)
      {
         File currentArchive = new File(zipfile);
         SimpleDateFormat formatter = new SimpleDateFormat ("MM/dd/yyyy hh:mma",Locale.getDefault());
         //ProgressMonitor pm = null;
         boolean overwrite = false;
         ZipFile zf = null;
         FileOutputStream out = null;
         InputStream in = null;
         File outputDir=new File(targetDirectory);
         File jdbcFile=new File(jdbcFileLoc);

         if (outputDir != null && !outputDir.exists())
         {
            outputDir.mkdirs();
         }

         try
         {
            zf = new ZipFile(currentArchive);
            int size = zf.size();
            int extracted = 0;

            Enumeration<?> entries = zf.entries();
            progressBar.setMaximum(size);
            installLogOutput("Extracting files to " + targetDirectory);
            
            ArrayList<String> installerFiles = new ArrayList<String>();
            
            installerFiles.add("Downloader.class");
            installerFiles.add("DownloaderError.class");
            installerFiles.add("DownloaderTest.class"); 
            installerFiles.add("Ebcdic2Ascii.class");
            installerFiles.add("ControlCSignalHandler.class");
            installerFiles.add("ControlCSignalHandler$1.class");
            
            for (int i=0; i<size; i++)
            {
               progressBar.setValue(i+1);
               
               ZipEntry entry = (ZipEntry) entries.nextElement();
               if (entry.isDirectory())
                  continue;

               String pathname = entry.getName();
               if (myClassName.equals(pathname) || MANIFEST.equals(pathname.toUpperCase()) || 
                       pathname.startsWith(installerFilesStartWith) || installerFiles.contains(pathname))
                  continue;

               extracted ++;
               //pm.setProgress(i);
               //pm.setNote(pathname);
               //if (pm.isCanceled())
               //   return;

               in = zf.getInputStream(entry);
               File outFile = new File(outputDir, pathname);
               Date archiveTime = new Date(entry.getTime());
               if (overwrite==false)
               {
                  if (outFile.exists())
                  {
                     Object[] options = {"Yes", "Yes To All", "No"};
                     String msg = checkFileExists (outFile,entry,formatter,archiveTime);
                     int result = JOptionPane.showOptionDialog(InstallerGUIMode.this,
                        msg, "Warning", JOptionPane.DEFAULT_OPTION,
                        JOptionPane.WARNING_MESSAGE, null, options,options[0]);
                     if (result == 2)
                     { // No
                        extracted --;
                        continue;
                     }
                     else if (result == 1)
                     { //YesToAll
                        overwrite = true;
                     }
                     
                     optComponentContainer.repaint();
                     optComponentContainer.revalidate();
                  }
               }
               handleSamplesDir(archiveTime, in, outFile, out, jdbcFile, outputDir);
            }

            handlePermission(targetDirectory);
          
            zf.close();
            getToolkit().beep();
            JOptionPane.showMessageDialog (InstallerGUIMode.this,
               extractedMessage(extracted, zipfile, outputDir, selectedOptionalComponents()),
               productTitle+" Installation Status"
               ,
               JOptionPane.INFORMATION_MESSAGE);
            
            optComponentContainer.repaint();
            
            installLogOutput("Extraction Complete: Successfully installed core TRAFCI files");
            
         } catch (Exception e)
         {
            installLogOutput("INSTALLATION ERROR:\n" + e.toString());
             
            System.out.println(e);
            if (zf!=null)
            {
               try
               {
                  zf.close();
               } catch (IOException ioe)
               {
                  ;
               } 
            }
            if (out!=null)
            {
               try
               {
                  out.close();
               } catch (IOException ioe)
               {
                  ;
               } 
            }
            if (in!=null)
            {
               try
               {
                  in.close();
               } catch (IOException ioe)
               {
                  ;
               } 
            }
         }
      }

      public void installLogOutput(String logtext){
          downloadTextArea.setText(downloadTextArea.getText() + "\n" + "* " + logtext);
          downloadTextArea.setCaretPosition(downloadTextArea.getDocument().getLength());
          downloadTextArea.paintImmediately(downloadTextArea.getVisibleRect());
      }
      
      class ComboBoxListener  implements ActionListener
      {
          public void actionPerformed(ActionEvent event)
          {
              if(event.getSource() == proxyCombo)
              {
                  if(proxyCombo.getSelectedItem() != null)
                  {   
                      String tmp[] = proxyCombo.getSelectedItem().toString().split(":");
                      if(tmp.length>1)
                      {
                          proxyServerText.setText(tmp[0]);
                          proxyPortText.setText(tmp[1]);
                      }
                  }
              }
          }
      }
      
      class RadioButtonListener  implements ActionListener
      {
          public void actionPerformed(ActionEvent event){
              if(event.getSource() == useProxyRadio || event.getSource() == noProxyRadio){
                  toggleProxyComponents();
              }
          }
      }
      
      class CheckBoxListener  implements ActionListener
      {
          public void actionPerformed(ActionEvent event){
              if(event.getSource() == checkPerl || event.getSource() == checkPerlSAX || event.getSource() == checkPython){
                  
                  perlJavaURL.setEnabled(checkPerl.isSelected());
                  perlSAXURL.setEnabled(checkPerlSAX.isSelected());
                  pythonJythonURL.setEnabled(checkPython.isSelected());
                      
                  if(!selectedOptionalComponents()){
                      if(!standardInstall)
                          nextButtonOptionalComponents.setEnabled(false);
                      
                      panelProxy.setVisible(false);
                  }else{
                      panelProxy.setVisible(true);
                      nextButtonOptionalComponents.setEnabled(true);
                  }
              }else if(event.getSource() == checkLegal){
                  
                  if(checkLegal.isSelected())
                      nextButtonLegal.setEnabled(true);
                  else
                      nextButtonLegal.setEnabled(false);
                  
                  if(!standardInstall)
                  {
                      if(checkLegal.isSelected()){
                          legalPanelContainer.remove(targetSelectPanel);
                          legalPanelContainer.add(targetSelectPanel, BorderLayout.PAGE_END);
                          BorderLayout BL = (BorderLayout)legalPanelContainer.getLayout();
                          BL.setHgap(20);
                          legalPanelContainer.revalidate();
                      }else{
                          legalPanelContainer.remove(targetSelectPanel);
                          legalPanelContainer.revalidate();
                         
                      }
                  }
                  
              }
          }
      }

      class ButtonListener  implements ActionListener
      {
         public void actionPerformed(ActionEvent event)
         {
            if (((JButton)event.getSource()).getText().equals("Cancel"))
            {
               System.exit(0);
            }
            else if (event.getSource() == nextButtonInstallerPath)
            {  
                if(enableOpenSource && !nextButtonInstallerPath.getText().equals("Install"))
                    cardCurrent = cardLegal;
                else
                    cardCurrent = cardOptionalComponents; 
                        
                checkRequiredComponents.setSelected(true);
                checkLegal.setSelected(false);
                nextButtonLegal.setEnabled(false);
                
                CardLayout cl = (CardLayout)(content.getLayout());
                cl.show(content, cardCurrent);
                
                if(enableOpenSource)
                    checkLegal.requestFocusInWindow();
                else
                    nextButtonOptionalComponents.requestFocusInWindow();
                
                
                if(nextButtonInstallerPath.getText().equals("Install")){
                    hideOptionalComponents();
                    new ButtonListener().actionPerformed(new ActionEvent(nextButtonOptionalComponents,0,"click"));
                }
            }
            else if(event.getSource() == nextButtonOptionalComponents)
            {
                if(validateProxy()){
                    panelComponentsProxyContainer.remove(panelProxy);
                    panelComponentsProxyContainer.add(panelDownload);
                    panelComponentsProxyContainer.revalidate();
                    
                    downloadTextArea.setText("Installation Log\n----------------------");
                    downloadTextArea.setCaretPosition(downloadTextArea.getText().length());
                    
                    if(standardInstall)
                    {
                        // first install then download
                        downloadLabel.setText("Installing Core TRAFCI Files...");
                        String jarFileName = getJarFileName();
                        process(jarFileName);
                    }
                    
                    if(selectedOptionalComponents())
                        SwingUtilities.invokeLater(new DownloadComponentsWrapper());
                
                    SwingUtilities.invokeLater(new installComplete());
                }
                
            }
            else if (event.getSource() == fileBrowseButton)
            {
               if (getJDBCLocation()==JFileChooser.APPROVE_OPTION)
                  jdbcFileText.setText(jdbcFileLoc);
               if (jdbcFileLoc != null && targetDirectory != null)
                  nextButtonInstallerPath.setEnabled(true);
              
            }
            else if (event.getSource() == dirBrowseButton)
            {
               if (getTargetInstallLocation()==JFileChooser.APPROVE_OPTION)
                  targetDirText.setText(targetDirectory);
               if (jdbcFileLoc != null && targetDirectory != null)
                  nextButtonInstallerPath.setEnabled(true);
            }
            else if (event.getSource() == choiceStandardInstall)
            {
                /* reset defaults */
                Color borderCol=new Color(167,160,143);
                targetSelectPanel.setBorder(BorderFactory.createTitledBorder(BorderFactory.createLineBorder(borderCol),"                                            "));
                targetDirText.setText(defaultInstallDir);
                
                pathHolder.remove(targetSelectPanel);
                pathHolder.add(targetSelectPanel, BorderLayout.PAGE_END);
                pathHolder.revalidate();
                
                CardLayout cl = (CardLayout)(content.getLayout());
                cardCurrent = cardInstallerPath;
                cl.show(content, cardInstallerPath);
                standardInstall = true;
                
                fileBrowseButton.requestFocusInWindow();
            }
            else if (event.getSource() == choiceCoreComponents)
            {
                /* reset defaults */
                Color borderCol=new Color(167,160,143);
                targetSelectPanel.setBorder(BorderFactory.createTitledBorder(BorderFactory.createLineBorder(borderCol),"                                            "));
                targetDirText.setText(defaultInstallDir);
                
                pathHolder.remove(targetSelectPanel);
                pathHolder.add(targetSelectPanel, BorderLayout.PAGE_END);
                pathHolder.revalidate();
                
                CardLayout cl = (CardLayout)(content.getLayout());
                cardCurrent = cardInstallerPath;         
                cl.show(content, cardInstallerPath);
                
                nextButtonInstallerPath.setText("Install");
                
                fileBrowseButton.requestFocusInWindow();
            }
            else if (event.getSource() == choiceOptionalButton)
            {
                Color borderCol=new Color(167,160,143);
                targetSelectPanel.setBorder(BorderFactory.createTitledBorder(BorderFactory.createLineBorder(borderCol),""));
                
                cardCurrent = cardLegal;
                CardLayout cl = (CardLayout)(content.getLayout());
                cl.show(content, cardCurrent);
                standardInstall = false;
                checkRequiredComponents.setSelected(false);
                checkLegal.setSelected(false);
                nextButtonLegal.setEnabled(false);
                legalPanelContainer.remove(targetSelectPanel);
                legalPanelContainer.revalidate();
                
                checkLegal.requestFocusInWindow();
                
            }
            else if (((JButton)event.getSource()).getText().equals("Back"))
            {
                if(cardCurrent.equals(cardInstallerPath)){
                    nextButtonInstallerPath.setText("Next");
                    cardCurrent = cardInstallerChoices;
                }else if(cardCurrent.equals(cardOptionalComponents)){
                    if(enableOpenSource)
                        cardCurrent = cardLegal;
                    else
                        cardCurrent = cardInstallerPath;
                }
                else if(cardCurrent.equals(cardLegal)){
                    if(standardInstall)
                        cardCurrent = cardInstallerPath;
                    else
                        cardCurrent = cardInstallerChoices;
                }else if(cardCurrent.equals(cardInstallationComplete)){
                    cardCurrent = cardOptionalComponents;
                   
                    panelComponentsProxyContainer.removeAll();
                    setupDownloadPanel(false);
                    panelComponentsProxyContainer.add(panelComponents);
                    panelComponentsProxyContainer.add(panelProxy);
                    panelComponentsProxyContainer.revalidate();
                }

                CardLayout cl = (CardLayout)(content.getLayout());
                cl.show(content, cardCurrent);
                
            }
            else if(event.getSource() == autoDetectProxy){
                setCursor(new Cursor(Cursor.WAIT_CURSOR));
                autoDetectProxy.setEnabled(false);
                Downloader dl = new Downloader();
                populateProxyComboBox(dl.findProxy());
                autoDetectProxy.setEnabled(true);
                setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
            
            }
            else if(event.getSource() == nextButtonLegal){
                
                if(!standardInstall && !validateNCIPath(targetDirText.getText()))
                    JOptionPane.showMessageDialog(null, ERROR_INVALID_NCI_DIR, "Error", JOptionPane.ERROR_MESSAGE);
                else{
                    // if user went to optional components only screen
                    // unchecked everything, then came back to here,
                    // lets re-enable everything
                    if(!nextButtonOptionalComponents.isEnabled())
                        resetOptionalComponentsScreen();
                    
                    cardCurrent = cardOptionalComponents;
                    CardLayout cl = (CardLayout)(content.getLayout());
                    cl.show(content, cardCurrent);
                    
                    nextButtonOptionalComponents.requestFocusInWindow();
                }
            }
            else if(event.getSource() == finishedButton){
                System.exit(0);
            }
            else if(event.getSource() == dirBrowseButtonOSS){
                if (getTargetInstallLocation()==JFileChooser.APPROVE_OPTION)
                    ossTargetDirText.setText(targetDirectory);
            }
            else if(event.getSource() == perlJavaURL){
                Object result = JOptionPane.showInputDialog(content, "URL of the Folder to Download Perl JavaServer (Java.pm, JavaArray.pm, amd JavaServer.jar):","Type a URL", JOptionPane.QUESTION_MESSAGE, null, null, defaultPerlJavaURL);
                if(result != null && !result.toString().trim().equals("")){
                    defaultPerlJavaURL = result.toString();
                    /* we are looking for a folder here */
                    if(!defaultPerlJavaURL.endsWith("/"))
                        defaultPerlJavaURL += "/";
                } 
            }
            else if(event.getSource() == perlSAXURL){
                Object result = JOptionPane.showInputDialog(content, "URL to Download Perl SAX XML Module:                                                                                ","Type a URL", JOptionPane.QUESTION_MESSAGE, null, null, defaultPerlSAXURL);
                if(result != null && !result.toString().trim().equals(""))
                    defaultPerlSAXURL = result.toString();
            }
            else if(event.getSource() == pythonJythonURL){
                Object result = JOptionPane.showInputDialog(content, "URL to Jython:                                                                                                                    ","Type a URL", JOptionPane.QUESTION_MESSAGE, null, null, defaultPythonJythonURL);
                if(result != null && !result.toString().trim().equals(""))
                    defaultPythonJythonURL = result.toString();
            }
            
         }// action performed
      }
      
      
      class ProxyBox extends JComboBox
      {
		private static final long serialVersionUID = -1623914485707120601L;

		public ProxyBox(){
              
          }
          
          public Dimension getPreferredSize(){
              return getMaximumSize();
          }
          
          public Dimension getMaximumSize(){
              return new Dimension(260,25);
          } 
          
          public Dimension getMinimumSize(){
              return getMaximumSize();
          }
          
          public Dimension getSize(){
              return getMaximumSize();
          }
          
          public Dimension getSize(Dimension arg0){
              return getMaximumSize();
          }
      }
      
      void populateProxyComboBox(ArrayList<String> list){
          if(list != null && proxyCombo != null)
          {
              proxyCombo.removeAllItems();
              String[] tmp;
              for(int i=0;i<list.size();i++)
              {
                  proxyCombo.addItem(list.get(i));
                  if(i==0)
                  {
                      tmp = list.get(0).split(":");
                      if(tmp.length>1)
                      {
                          proxyServerText.setText(tmp[0]);
                          proxyPortText.setText(tmp[1]);
                      }
                  }
              }
              
              if(list.size() > 0)
                  proxyCombo.setVisible(true);
              else
                  proxyCombo.setVisible(false);
              
          }
      }

      void resetOptionalComponentsScreen(){
          nextButtonOptionalComponents.setEnabled(true);
          
          checkPerl.setSelected(true);
          checkPerlSAX.setSelected(true);
          checkPython.setSelected(true);

          panelComponentsProxyContainer.remove(panelDownload); 
          panelComponentsProxyContainer.add(panelProxy);
          panelComponentsProxyContainer.revalidate();
          panelProxy.setVisible(true);
          
          noProxyRadio.setSelected(true);
          toggleProxyComponents();

      }
      
      void hideOptionalComponents(){
          checkPerl.setSelected(false);
          checkPerl.setVisible(false);
          checkPerlSAX.setSelected(false);
          checkPerlSAX.setVisible(false);
          checkPython.setSelected(false);
          checkPython.setVisible(false);
          
          perlJavaURL.setVisible(false);
          perlSAXURL.setVisible(false);
          pythonJythonURL.setVisible(false);
      }
      
      void initialize(String productTitle)
      {
          int winWidth = 550;
          int winHeight = 525;//475;
    
          content.setBackground(Color.white);
          content.setLayout(new CardLayout());
          content.setBackground(new Color(255,255,255));
          
          // Set up first window
          setTitle(productTitle+" Installer Wizard");
          
          setupInstallerChoices();
          setupLegalDisclaimer();
          setupInstallPathPage();
          setupOptionalComponents();
          setupInstallationComplete();
          
          CardLayout cl = (CardLayout)(content.getLayout());
          cl.show(content, cardInstallerChoices);
          cardCurrent = cardInstallerChoices;
          
          //Display the window
          pack();
          addWindowListener(new ExitListener());
          
          setSize(winWidth,winHeight);
          Dimension screen = getToolkit().getScreenSize();
          setBounds( (screen.width-getWidth())/2, (screen.height-getHeight())/2, getWidth(), getHeight() );

          setResizable(false);
          setVisible(true);
      }
      
      KeyAdapter escapeHandler = new KeyAdapter()
      {
         public void keyPressed(KeyEvent ke)
         {
            if (ke.getKeyCode() == KeyEvent.VK_ESCAPE)
            {
               System.exit(0);
            }
         }
      };
      
      KeyAdapter installPathKeyHandler = new KeyAdapter()
      {
         public void keyPressed(KeyEvent ke)
         {
             if(ke.getKeyCode() == KeyEvent.VK_ENTER)
             {
                 if(fileBrowseButton.isFocusOwner() && jdbcFileText.getText().trim().equals(""))
                     new ButtonListener().actionPerformed(new ActionEvent(fileBrowseButton,0,"click"));
                 else if(dirBrowseButton.isFocusOwner())
                     new ButtonListener().actionPerformed(new ActionEvent(dirBrowseButton,0,"click"));
                 else if(nextButtonInstallerPath.isEnabled())
                     new ButtonListener().actionPerformed(new ActionEvent(nextButtonInstallerPath,0,"click"));
             }
         }
      };
      
      KeyAdapter legalKeyHandler = new KeyAdapter()
      {
          public void keyPressed(KeyEvent ke)
          {
              switch(ke.getKeyCode()){
                  case KeyEvent.VK_ESCAPE:     System.exit(0);
                      break;
                  case KeyEvent.VK_ENTER:
                          if(checkLegal.isSelected())
                              new ButtonListener().actionPerformed(new ActionEvent(nextButtonLegal,0,"click"));
                          else{
                              checkLegal.setSelected(!checkLegal.isSelected());
                              new CheckBoxListener().actionPerformed(new ActionEvent(checkLegal,0,"click"));
                          }
                      break;
              }
          }
      };

      class ExitListener extends WindowAdapter
      {
         public void windowClosing(WindowEvent we)
         {
            System.exit(0);
         }
      }
      
       void setupInstallerChoices(){
          
          Color bgCol=new Color(255,255,255);
          Color borderCol=new Color(167,160,143);
          
          Font labelFont=new Font("SansSerif",Font.PLAIN,13);
          Font headLabelFont=new Font(null,Font.BOLD,15);
        
          JPanel headPanel = new JPanel(new GridBagLayout());
 
          GridBagConstraints c = new GridBagConstraints();
        
          
          JLabel headLabel=new JLabel("Welcome to the "+productTitle+" Installer Wizard");
          c.fill = GridBagConstraints.PAGE_START;
          c.gridwidth=10;
          c.gridheight=1;
          c.weightx = 0;
          c.gridx = 0;
          c.gridy = 0;
          //c.insets = new Insets(0,30,5,0);
          c.insets = new Insets(10,0,10,0);
          headPanel.add(headLabel,c);
          
          c.fill = GridBagConstraints.HORIZONTAL;
          
          JLabel headLabel1=new JLabel("This wizard helps you install and configure "+productTitle +".");
          c.gridwidth=10;
          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 1;
          c.insets = new Insets(0,0,20,0);
          headPanel.add(headLabel1,c);
          
          
          JSeparator hRule=new JSeparator();
          c.gridwidth=10;
          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 2;
          c.insets = new Insets(0,0,5,0);
          hRule.setForeground(borderCol);
          headPanel.add(hRule,c);
          
          JLabel noteLabel=new JLabel("Note:");
          c.weightx = 0.0;
          c.gridx = 1;
          c.gridy = 3;
          c.gridwidth=1;
          c.ipadx= 10;
          c.insets = new Insets(0,50,0,20);
          noteLabel.setFont(new Font(null,Font.BOLD,13));
          headPanel.add(noteLabel,c);
          
          JLabel headLabel2=new JLabel("The wizard requires the " + NEW_JDBC);
          c.weightx = 0.0;
          c.gridx = 3;
          c.gridy = 3;
          c.ipadx =0;
          c.gridwidth=11;
          c.insets = new Insets(0,0,0,0);
          headPanel.add(headLabel2,c);
          
          JLabel headLabel3=new JLabel("to be installed on your workstation."
             );
          c.weightx = 0.0;
          c.gridx = 3;
          c.gridy = 4;
          c.ipadx=0;
          c.gridwidth=4;
          c.insets = new Insets(0,0,0,0);
          headPanel.add(headLabel3,c);
          
          JSeparator hEndRule=new JSeparator();
          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 5;
          c.gridwidth=10;
          c.insets = new Insets(10,0,5,0);
          headPanel.add(hEndRule,c);
          hEndRule.setForeground(borderCol);
          
          
          Color headTxtColor=new Color(000,000,000);
          headLabel.setForeground(new Color(000,000,128));
       
          headLabel1.setForeground(headTxtColor);
          headLabel2.setForeground(headTxtColor);
          headLabel3.setForeground(headTxtColor);
          headLabel.setFont(headLabelFont);
          headLabel1.setFont(labelFont);
          headLabel2.setFont(labelFont);
          headLabel3.setFont(labelFont);

          headPanel.setBackground(bgCol);
          
          /* button code goes here */
          choiceStandardInstall = new JButton("Standard Installation");
          c.fill = GridBagConstraints.HORIZONTAL;
          c.weightx = 0.0;
          c.gridx = 5;
          c.gridy = 10;
          c.gridheight = 2;
          c.gridwidth = 2;
          c.ipady = 15;
          c.insets = new Insets(40,0,0,0);

          headPanel.add(choiceStandardInstall, c);
          
          choiceCoreComponents = new JButton("Core Components");
          c.fill = GridBagConstraints.HORIZONTAL;
          c.weightx = 0.0;
          c.gridx = 5;
          c.gridy = 15;
          c.gridheight = 2;
          c.gridwidth = 2;
          c.ipady = 15;
          c.insets = new Insets(10,0,0,0);

          headPanel.add(choiceCoreComponents, c);
          
          choiceOptionalButton = new JButton("Optional Components");
          c.weightx = 0.0;
          c.gridx = 5;
          c.gridy = 20;
          c.gridheight = 2;
          c.gridwidth = 2;
          c.ipady = 15;
          c.insets = new Insets(10,0,30,0);
          
    
          if(enableOpenSource)
              headPanel.add(choiceOptionalButton, c);
          
          c.ipady = 0;

          choiceStandardInstall.addKeyListener(
                  new KeyAdapter()
                  {
                     public void keyPressed(KeyEvent ke)
                     {
                        if (ke.getKeyCode() == KeyEvent.VK_ESCAPE)
                           System.exit(0);
                        else if(ke.getKeyCode() == KeyEvent.VK_ENTER)
                           new ButtonListener().actionPerformed(new ActionEvent(choiceStandardInstall,0,"click"));
                     }
                  }
          );
          choiceStandardInstall.addActionListener(new ButtonListener());
          choiceCoreComponents.addActionListener(new ButtonListener());
          choiceCoreComponents.addKeyListener(escapeHandler);
          choiceOptionalButton.addKeyListener(escapeHandler);
          choiceOptionalButton.addActionListener(new ButtonListener());
          
          getContentPane().add(headPanel, cardInstallerChoices);
      }
      
       void setupInstallPathPage(){
          
          JPanel pathContainer = new JPanel(new BorderLayout());
          Color bgCol=new Color(255,255,255);
          Color borderCol=new Color(167,160,143);
          pathContainer.setBackground(bgCol);
          
          
          file_txt =new JTextField("",20);
          fileBrowseButton=new JButton("Browse");
          dirBrowseButton=new JButton("Browse");
         
          nextButtonInstallerPath=new JButton("Next");
          JButton cancelButton=new JButton("Cancel");
          JButton backButton=new JButton("Back");
          statusLabel=new JLabel("Not Started");
          
          filechooser=new JFileChooser();
          jdbcFileText =new JTextField("                       ",10);
          targetDirText =new JTextField("                       ",10);
          Font titleFont=new Font(null,Font.BOLD,13);
          Color textCol=new Color(230,230,230);

          Font labelFont=new Font("SansSerif",Font.PLAIN,13);
          Font headLabelFont=new Font(null,Font.BOLD,15);
        
          JPanel headPanel = new JPanel(new GridBagLayout());
          
          GridBagConstraints c = new GridBagConstraints();
        
          
          JLabel headLabel=new JLabel("Welcome to the "+productTitle+" Installer Wizard");
          c.fill = GridBagConstraints.PAGE_START;
          c.gridwidth=10;
          c.gridheight=1;
          c.weightx = 0;
          c.gridx = 0;
          c.gridy = 0;
          //c.insets = new Insets(0,30,5,0);
          c.insets = new Insets(10,0,10,0);
          headPanel.add(headLabel,c);
          
          c.fill = GridBagConstraints.HORIZONTAL;
          
          JLabel headLabel1=new JLabel("This wizard helps you install and configure "+productTitle +".");
          c.gridwidth=10;
          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 1;
          c.insets = new Insets(0,0,20,0);
          headPanel.add(headLabel1,c);
          
          
          JSeparator hRule=new JSeparator();
          c.gridwidth=10;
          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 2;
          c.insets = new Insets(0,0,5,0);
          hRule.setForeground(borderCol);
          headPanel.add(hRule,c);
          
          JLabel noteLabel=new JLabel("Note:");
          c.weightx = 0.0;
          c.gridx = 1;
          c.gridy = 3;
          c.gridwidth=1;
          c.ipadx= 10;
          c.insets = new Insets(0,50,0,20);
          noteLabel.setFont(new Font(null,Font.BOLD,13));
          headPanel.add(noteLabel,c);
          
          JLabel headLabel2=new JLabel("The wizard requires the " + NEW_JDBC + " ");
          c.weightx = 0.0;
          c.gridx = 3;
          c.gridy = 3;
          c.ipadx =0;
          c.gridwidth=11;
          c.insets = new Insets(0,0,0,0);
          headPanel.add(headLabel2,c);
          
          JLabel headLabel3=new JLabel("to be installed on your workstation."
             );
          c.weightx = 0.0;
          c.gridx = 3;
          c.gridy = 4;
          c.ipadx=0;
          c.gridwidth=4;
          c.insets = new Insets(0,0,0,0);
          headPanel.add(headLabel3,c);
          
          JSeparator hEndRule=new JSeparator();
          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 5;
          c.gridwidth=10;
          c.insets = new Insets(10,0,5,0);
          headPanel.add(hEndRule,c);
          hEndRule.setForeground(borderCol);
    
          Color headTxtColor=new Color(000,000,000);
          headLabel.setForeground(new Color(000,000,128));
       
          headLabel1.setForeground(headTxtColor);
          headLabel2.setForeground(headTxtColor);
          headLabel3.setForeground(headTxtColor);
          headLabel.setFont(headLabelFont);
          headLabel1.setFont(labelFont);
          headLabel2.setFont(labelFont);
          headLabel3.setFont(labelFont);

          headPanel.setBackground(bgCol);
          
          pathContainer.add(headPanel, BorderLayout.PAGE_START);
          /****************/
          JPanel pathInternalContainer = new JPanel(new GridBagLayout());
          /****************/
          
          /************************************************************/
          GridLayout gl = new GridLayout(2,1);
          gl.setVgap(-10);
          
          pathHolder = new JPanel(gl);
          pathHolder.setBackground(bgCol);
          
          /************************************************************/
          
          JPanel jdbcFilePanel = new JPanel(new GridBagLayout());

          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 0;
          c.ipadx = 0;
          c.ipady = 0;
          c.gridwidth = 3;
          c.insets = new Insets(10,20,0,0);
          
          JLabel jdbcFileLabel=new JLabel(NEW_JDBC + ":");
          jdbcFileLabel.setFont(titleFont);
          fileBrowseButton.addActionListener(new ButtonListener());
          fileBrowseButton.addKeyListener(escapeHandler);
          fileBrowseButton.addKeyListener(installPathKeyHandler);
         
          jdbcFilePanel.setBackground(Color.white);
          jdbcFilePanel.add(jdbcFileLabel,c);
          c.weightx = 0.5;
          c.gridx = 1;
          c.gridy = 1;
          c.gridwidth = 1;
          c.insets = new Insets(0,20,10,0);
          c.ipadx=375;
          jdbcFilePanel.add(jdbcFileText,c);
          
          jdbcFileText.setCaretPosition(jdbcFileText.getColumns());
          jdbcFileText.addKeyListener(new KeyAdapter()
          {
             public void keyPressed(KeyEvent ke)
             {
                if (ke.getKeyCode() == KeyEvent.VK_ESCAPE)
                   System.exit(0);

                if (ke.getKeyCode() == KeyEvent.VK_RIGHT)
                {
                   int currentPos = jdbcFileText.getCaretPosition();
                   if (currentPos < jdbcFileText.getDocument().getLength()) jdbcFileText.setCaretPosition(currentPos+1);
                }
             }
          });

          jdbcFileText.setBackground(textCol);
          jdbcFileText.setEditable(false);
          c.weightx = 0.5;
          c.gridx = 2;
          c.gridy = 1;
          c.gridwidth = 1;
          c.insets = new Insets(0,10,10,40);
          c.ipadx=75;
          jdbcFilePanel.add(fileBrowseButton,c);
          jdbcFilePanel.setSize(500,80);
          jdbcFilePanel.setBorder(BorderFactory.createTitledBorder(BorderFactory.createLineBorder(borderCol)," Select these locations, and then click Next."));
          
          pathHolder.add(jdbcFilePanel);
          /************************************************************/
         
          targetSelectPanel = new JPanel(new GridBagLayout());
          c.weightx = 0.0;
          c.gridx = 0;
          c.gridy = 0;
          c.gridwidth = 3;
          c.insets = new Insets(10,20,0,0);
          JLabel targetDirLabel=new JLabel(productTitle+" installation directory:");
          targetDirLabel.setFont(titleFont);
          jdbcFilePanel.setLocation(25,175);
          jdbcFilePanel.setBackground(bgCol);
          targetSelectPanel.setBackground(Color.white);
          targetSelectPanel.add(targetDirLabel,c);
          c.weightx = 0.5;
          c.gridx = 1;
          c.gridy = 1;
          c.gridwidth = 1;
          c.insets = new Insets(0,20,10,0);
          c.ipadx=375;

          targetSelectPanel.add(targetDirText,c);
          targetDirectory = defaultInstallDir;
          targetDirText.setText(defaultInstallDir);
          targetDirText.setCaretPosition(targetDirText.getColumns());
          targetDirText.addKeyListener(new KeyAdapter()
          {
             public void keyPressed(KeyEvent ke)
             {
                if (ke.getKeyCode() == KeyEvent.VK_ESCAPE)
                   System.exit(0);
                if (ke.getKeyCode() == KeyEvent.VK_RIGHT)
                {
                   int currentPos = targetDirText.getCaretPosition();
                   if (currentPos < targetDirText.getDocument().getLength()) targetDirText.setCaretPosition(currentPos+1);
                }
             }
          });

          targetDirText.setBackground(textCol);
          targetDirText.setEditable(false);
          c.weightx = 0.5;
          c.gridx = 2;
          c.gridy = 1;
          c.gridwidth = 1;
          c.insets = new Insets(0,10,10,40);
          c.ipadx=75;
          targetSelectPanel.add(dirBrowseButton,c);
          dirBrowseButton.addActionListener(new ButtonListener());
          dirBrowseButton.addKeyListener(escapeHandler);
          dirBrowseButton.addKeyListener(installPathKeyHandler);
          targetSelectPanel.setSize(500,100);

          targetSelectPanel.setBorder(BorderFactory.createTitledBorder(BorderFactory.createLineBorder(borderCol),"                                            "));
          
          targetSelectPanel.setBackground(bgCol);

          pathHolder.add(targetSelectPanel);
          targetSelectPanel.setLocation(25,265);

          /************************************************************/
          //pathContainer.add(pathHolder, BorderLayout.CENTER);

          c.gridwidth = 450;
          c.gridheight = 100;
          c.insets = new Insets(0,25,0,25);
          c.ipady = 28;
          pathInternalContainer.setBackground(bgCol);
          pathInternalContainer.add(pathHolder,c);
          pathContainer.add(pathInternalContainer, BorderLayout.CENTER);
          /************************************************************/
          
          JPanel controlPanel = new JPanel();
          controlPanel.add(backButton);
          backButton.addActionListener(new ButtonListener());
          backButton.addKeyListener(escapeHandler);
          controlPanel.add(cancelButton);
          cancelButton.addActionListener(new ButtonListener());
          cancelButton.addKeyListener(escapeHandler);
          controlPanel.add(nextButtonInstallerPath);
          nextButtonInstallerPath.addActionListener(new ButtonListener());
          nextButtonInstallerPath.addKeyListener(installPathKeyHandler);
          nextButtonInstallerPath.addKeyListener(escapeHandler);
          nextButtonInstallerPath.setEnabled(false);
          
          controlPanel.setSize(500,50);
          controlPanel.setLocation(25,385);
          controlPanel.setBackground(bgCol);
          
          pathContainer.add(controlPanel, BorderLayout.SOUTH);
          /************************************************************/
          
          getContentPane().add(pathContainer, cardInstallerPath);

       }
     
       void setupOptionalComponents(){

           optComponentContainer = new JPanel(new BorderLayout());
           
           Color bgCol=new Color(255,255,255);
           Color borderCol=new Color(167,160,143);
           
           Font labelFont=new Font("SansSerif",Font.PLAIN,13);
           Font headLabelFont=new Font(null,Font.BOLD,15);
         
           /****************** Header Panel ***********************************/
           JPanel headPanel = new JPanel(new GridBagLayout());
  
           GridBagConstraints c = new GridBagConstraints();
         
           
           JLabel headLabel=new JLabel("Welcome to the "+productTitle+" Installer Wizard");
           c.fill = GridBagConstraints.PAGE_START;
           c.gridwidth=10;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 0;
           
           c.insets = new Insets(10,0,10,0);
           headPanel.add(headLabel,c);
           
           c.fill = GridBagConstraints.HORIZONTAL;
           
           JLabel headLabel1=new JLabel("This wizard helps you install and configure "+productTitle +".");
           c.gridwidth=10;
           c.weightx = 0.0;
           c.gridx = 0;
           c.gridy = 1;
           c.insets = new Insets(0,0,20,0);
           headPanel.add(headLabel1,c);
           
           
           JSeparator hRule=new JSeparator();
           c.gridwidth=10;
           c.weightx = 0.0;
           c.gridx = 0;
           c.gridy = 2;
           c.insets = new Insets(0,0,5,0);
           hRule.setForeground(borderCol);
           headPanel.add(hRule,c);
   
 
           Color headTxtColor=new Color(000,000,000);
           headLabel.setForeground(new Color(000,000,128));
        
           headLabel1.setForeground(headTxtColor);

           headLabel.setFont(headLabelFont);
           headLabel1.setFont(labelFont);

           headPanel.setBackground(bgCol);
           
           optComponentContainer.add(headPanel, BorderLayout.PAGE_START); 
           /***** New Panel w/ Options *********************************/
           
           GridLayout gl = new GridLayout(2,1);
           panelComponentsProxyContainer = new JPanel(gl);
           gl.setVgap(10);

           panelComponents = new JPanel(new GridBagLayout());
      
           c = new GridBagConstraints();    // reset c
           c.fill = GridBagConstraints.HORIZONTAL;
           c.anchor = GridBagConstraints.NORTH;
  
           JLabel componentsLabel=new JLabel("Please check the components you wish to download and install:");
           c.gridwidth=13;
           c.weightx = 0.0;
           c.gridx = 0;
           c.gridy = 0;
           c.insets = new Insets(0,0,5,0);
           panelComponents.add(componentsLabel,c);
           
           checkRequiredComponents = new JCheckBox("Core " + productTitle + " Components");
           c.gridwidth=10;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 1;
           c.gridy = 1;
           
           c.insets = new Insets(5,0,5,0);
           panelComponents.add(checkRequiredComponents,c);
 
       
           
           checkPerl = new JCheckBox("Perl JavaServer extension");
           c.gridwidth=5;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 1;
           c.gridy = 2;
           
           c.insets = new Insets(5,0,5,0);
           
           if(enableOpenSource)
               panelComponents.add(checkPerl,c);
           
           
           perlJavaURL = new JButton("Edit URL");
           c.gridwidth = 1;
           c.gridheight = 1;
           c.gridx = 0;
           c.gridy = 2;
           c.insets = new Insets(8,0,0,5);
           c.ipady = -5;
           if(enableOpenSource)
               panelComponents.add(perlJavaURL,c);
                      
           
           checkPerlSAX = new JCheckBox("Perl XML SAX Module");
           c.gridwidth=5;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 1;
           c.gridy = 3;
           c.ipady =0;
           //c.insets = new Insets(0,30,5,0);
           c.insets = new Insets(5,0,5,0);
           
           if(enableOpenSource)
               panelComponents.add(checkPerlSAX,c);
           
           perlSAXURL = new JButton("Edit URL");
           c.gridwidth = 1;
           c.gridheight = 1;
           c.gridx = 0;
           c.gridy = 3;
           c.ipady = -5;
           c.insets = new Insets(8,0,0,5);
           
           if(enableOpenSource)
               panelComponents.add(perlSAXURL,c);
           
           
           checkPython = new JCheckBox("Jython, a Java implementation of Python");
           c.gridwidth=8;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 1;
           c.gridy = 4;
           c.ipady =0;
           c.insets = new Insets(5,0,5,0);
               
           if(enableOpenSource)
               panelComponents.add(checkPython,c);


           pythonJythonURL = new JButton("Edit URL");
           c.gridwidth = 1;
           c.gridheight = 1;
           c.gridx = 0;
           c.gridy = 4;
           c.ipady = -5;
           c.insets = new Insets(8,0,0,5);
           
           if(enableOpenSource)
               panelComponents.add(pythonJythonURL,c);
           
           checkRequiredComponents.setSelected(true);
           
           if(enableOpenSource){
               checkPerl.setSelected(true);
               checkPerlSAX.setSelected(true);
               checkPython.setSelected(true);
           }
           
           checkPerl.addActionListener(new CheckBoxListener());
           checkPerl.addKeyListener(escapeHandler);
           checkPerlSAX.addActionListener(new CheckBoxListener());
           checkPerlSAX.addKeyListener(escapeHandler);
           checkPython.addActionListener(new CheckBoxListener());
           checkPython.addKeyListener(escapeHandler);
           perlJavaURL.addActionListener(new ButtonListener());
           perlJavaURL.addKeyListener(escapeHandler);
           perlSAXURL.addActionListener(new ButtonListener());
           perlSAXURL.addKeyListener(escapeHandler);
           pythonJythonURL.addActionListener(new ButtonListener());
           pythonJythonURL.addKeyListener(escapeHandler);
           
           checkRequiredComponents.setEnabled(false);
           checkRequiredComponents.setBackground(bgCol);
           checkPerl.setBackground(bgCol);
           checkPerlSAX.setBackground(bgCol);
           checkPython.setBackground(bgCol);
           
           panelComponents.setBackground(bgCol);
           panelComponents.setFont(labelFont);
           componentsLabel.setFont(headLabelFont);
        
           
           /************** proxy panel ***********/
           panelProxy = new JPanel(new GridBagLayout());
           
           c = new GridBagConstraints();    // reset c
           c.fill = GridBagConstraints.HORIZONTAL;
           c.anchor = GridBagConstraints.NORTH;

           noProxyRadio = new JRadioButton("Do not use a proxy server.");
           c.gridwidth=5;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 0;
           
           if(enableOpenSource)
               panelProxy.add(noProxyRadio,c);
           
           useProxyRadio = new JRadioButton("Use the following proxy settings:");
           c.gridwidth=7;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 2;
           
           if(enableOpenSource)
               panelProxy.add(useProxyRadio,c);
           
           JLabel blankLabel = new JLabel("              ");
           c.gridwidth=4;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 3;
           panelProxy.add(blankLabel,c);
           
           proxyServerLabel=new JLabel("Proxy Server: ");
           c.gridwidth=4;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 4;
           
           if(enableOpenSource)
               panelProxy.add(proxyServerLabel,c);
                   
           proxyServerText = new JTextField("",10);
           c.gridwidth=10;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 5;
           c.gridy = 4;
           
           if(enableOpenSource)
               panelProxy.add(proxyServerText,c);
           
           proxyPortLabel=new JLabel("Proxy Port: ");
           c.gridwidth=4;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 5;
           
           if(enableOpenSource)
               panelProxy.add(proxyPortLabel,c);
           
           proxyPortText = new JTextField("",4);
           c.gridwidth=5;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 5;
           c.gridy = 5;
           
           if(enableOpenSource)
               panelProxy.add(proxyPortText,c);
           
           autoDetectProxy = new JButton("Detect Proxy Server(s)");
           c.gridwidth=5;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 6;
           c.insets = new Insets(0,0,8,0);
           
           if(enableOpenSource)
               panelProxy.add(autoDetectProxy,c);
             
           proxyCombo = new ProxyBox();
           c.gridwidth=4;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 7;
           c.gridy = 6;
           c.insets = new Insets(0,0,8,0);
                      
           if(enableOpenSource)
               panelProxy.add(proxyCombo,c);
           
           proxyButtonGroup = new ButtonGroup();
           proxyButtonGroup.add(noProxyRadio);
           proxyButtonGroup.add(useProxyRadio);
           noProxyRadio.setSelected(true);
           
           toggleProxyComponents();
           
           proxyCombo.setBackground(bgCol);
           noProxyRadio.setBackground(bgCol);
           useProxyRadio.setBackground(bgCol);
           
           autoDetectProxy.addActionListener(new ButtonListener());
           autoDetectProxy.addKeyListener(escapeHandler);
           proxyCombo.addActionListener(new ComboBoxListener());
           proxyCombo.addKeyListener(escapeHandler);
           noProxyRadio.addActionListener(new RadioButtonListener());
           noProxyRadio.addKeyListener(escapeHandler);
           useProxyRadio.addActionListener(new RadioButtonListener());
           useProxyRadio.addKeyListener(escapeHandler);
           
           panelProxy.setSize(500,100);
           
           if(enableOpenSource)
               panelProxy.setBorder(BorderFactory.createTitledBorder(BorderFactory.createLineBorder(borderCol),"Proxy Settings"));
       
           panelProxy.setBackground(bgCol);
           /**************************************/
           
           /**************************************/
           setupDownloadPanel(false);
           /**************************************/
           
           panelComponentsProxyContainer.add(panelComponents);
           
           panelComponentsProxyContainer.add(panelProxy);
           Border margin = new EmptyBorder(0, 10, 0, 10);
           panelComponentsProxyContainer.setBorder(margin);
           
           panelComponentsProxyContainer.setBackground(bgCol);
           optComponentContainer.add(panelComponentsProxyContainer, BorderLayout.CENTER);
           /***********************************************************/
           
           JPanel panelButtons = new JPanel(new GridBagLayout());
           c = new GridBagConstraints();    // reset c
           c.fill = GridBagConstraints.HORIZONTAL;
           c.anchor = GridBagConstraints.NORTH;
 
           c.gridx = 0;
           c.gridy = 0;
           JLabel blankLabelBottom = new JLabel(" ");
           panelButtons.add(blankLabelBottom, c);
 
           c.gridy = 1;
           JButton backButton = new JButton("Back");
           JButton cancelButton = new JButton("Cancel");
           nextButtonOptionalComponents = new JButton("Install");
           
           c.insets = new Insets(0,0,5,5);
           panelButtons.add(backButton, c);
           c.gridx = 1;
           panelButtons.add(cancelButton, c);
           c.gridx = 2;
           panelButtons.add(nextButtonOptionalComponents, c);
           
           backButton.addActionListener(new ButtonListener());
           backButton.addKeyListener(escapeHandler);
           cancelButton.addActionListener(new ButtonListener());
           cancelButton.addKeyListener(escapeHandler);
           nextButtonOptionalComponents.addActionListener(new ButtonListener());
           nextButtonOptionalComponents.addKeyListener(
                   new KeyAdapter(){
                       public void keyPressed(KeyEvent ke)
                       {
                           switch(ke.getKeyCode()){
                               case KeyEvent.VK_ESCAPE:     System.exit(0);
                                   break;
                               case KeyEvent.VK_ENTER:      new ButtonListener().actionPerformed(new ActionEvent(nextButtonOptionalComponents,0,"click"));
                                   break;
                           }
                       }
                   }
           );
           
           panelButtons.setBackground(bgCol);
                     
           optComponentContainer.add(panelButtons, BorderLayout.PAGE_END);
           /***********************************************************/
           
           getContentPane().add(optComponentContainer, cardOptionalComponents);
           
           /***********************************************************/
       }
       
       void setupDownloadPanel(boolean installComplete){
           GridBagConstraints c = new GridBagConstraints();
           Color bgCol=new Color(255,255,255);
           
           panelDownload = new JPanel(new GridBagLayout());
           
           c = new GridBagConstraints();    // reset c
           c.fill = GridBagConstraints.HORIZONTAL;
           c.anchor = GridBagConstraints.NORTH;
           
           if(!installComplete)
               downloadLabel = new JLabel("Downloading: ");
                     
           c.gridwidth=20;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 1;
           c.gridy = 0;
           panelDownload.add(downloadLabel,c);     
           
           progressBar = new JProgressBar();
           c.gridwidth=20;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 1;
           c.gridy = 3;
           //c.ipadx = 130;
           
           if(installComplete)
               progressBar.setValue(100);
           
           panelDownload.add(progressBar,c);
           
           c.ipadx = 475;
           if(!installComplete){
               downloadTextArea = new JTextArea("");
               c.ipady = 100;
           }else
               c.ipady = 250;
           
           c.gridwidth=20;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 1;
           c.gridy = 7;
           
           JScrollPane scrollPane = new JScrollPane(downloadTextArea);
           panelDownload.add(scrollPane, c);
           
           downloadTextArea.setColumns(15);
           downloadTextArea.setLineWrap(true);
           downloadTextArea.setRows(7);
           downloadTextArea.setWrapStyleWord(true);
           downloadTextArea.setEditable(false);
           downloadTextArea.setMargin(new Insets(5,5,5,5));
           if(!installComplete)
               progressBar.addChangeListener(new pbChangeListener());
           
           downloadLabel.setBounds(new Rectangle(500,100));
           
           panelDownload.setBackground(bgCol);
       }
       
       void setupLegalDisclaimer(){

           JPanel legalContainer = new JPanel(new BorderLayout());
           
           Color bgCol=new Color(255,255,255);
           Color borderCol=new Color(167,160,143);
           
           Font labelFont=new Font("SansSerif",Font.PLAIN,13);
           Font headLabelFont=new Font(null,Font.BOLD,15);
         
           /****************** Header Panel ***********************************/
           JPanel headPanel = new JPanel(new GridBagLayout());
  
           GridBagConstraints c = new GridBagConstraints();
         
           
           JLabel headLabel=new JLabel("Welcome to the "+productTitle+" Installer Wizard");
           c.fill = GridBagConstraints.PAGE_START;
           c.gridwidth=10;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 0;
           
           c.insets = new Insets(10,0,10,0);
           headPanel.add(headLabel,c);
           
           c.fill = GridBagConstraints.HORIZONTAL;
           
           JLabel headLabel1=new JLabel("This wizard helps you install and configure "+productTitle +".");
           c.gridwidth=10;
           c.weightx = 0.0;
           c.gridx = 0;
           c.gridy = 1;
           c.insets = new Insets(0,0,20,0);
           headPanel.add(headLabel1,c);
           
           
           JSeparator hRule=new JSeparator();
           c.gridwidth=10;
           c.weightx = 0.0;
           c.gridx = 0;
           c.gridy = 2;
           c.insets = new Insets(0,0,5,0);
           hRule.setForeground(borderCol);
           headPanel.add(hRule,c);
   
 
           Color headTxtColor=new Color(000,000,000);
           headLabel.setForeground(new Color(000,000,128));
        
           headLabel1.setForeground(headTxtColor);

           headLabel.setFont(headLabelFont);
           headLabel1.setFont(labelFont);

           headPanel.setBackground(bgCol);
           
           legalContainer.add(headPanel, BorderLayout.PAGE_START); 
        
           /***** New Panel w/ Options *********************************/
           Border margin = new EmptyBorder(8, 20, 15, 20);
           legalPanelContainer = new JPanel(new BorderLayout());
           legalPanelContainer.setBorder(margin);
           
           JPanel legalPanel = new JPanel(new GridBagLayout());
           
           c = new GridBagConstraints();    // reset c
           c.fill = GridBagConstraints.HORIZONTAL;
           c.anchor = GridBagConstraints.NORTH;
  
           JLabel componentsLabel=new JLabel("Open Source Legal Disclaimer:");
           c.gridwidth=13;
           c.weightx = 0.0;
           c.gridx = 0;
           c.gridy = 0;
           c.insets = new Insets(0,0,5,0);
           legalPanel.add(componentsLabel,c);
           
           JTextArea disclaimerText = new JTextArea("The ability to download open source extensions is provided for your convenience only. Software provided under any open source licensing model is governed solely by such open source licensing terms.");
           c.gridheight = 15;
           c.gridwidth=20;
           c.ipadx = 150;
           c.weightx = 0.0;
           c.gridx = 0;
           c.gridy = 2;
           
           disclaimerText.setMargin(new Insets(5,5,5,5));
           JScrollPane scrollPane = new JScrollPane(disclaimerText);
           legalPanel.add(scrollPane, c);
           
           checkLegal = new JCheckBox("I agree to the above Terms & Conditions");
           c.gridwidth=10;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 18;
           c.insets = new Insets(5,0,12,0);
           legalPanel.add(checkLegal,c);
           
           disclaimerText.setColumns(20);
           disclaimerText.setLineWrap(true);
           disclaimerText.setRows(12);
           disclaimerText.setWrapStyleWord(true);
           disclaimerText.setEditable(false);
           
           legalPanel.setBackground(bgCol);
           checkLegal.setBackground(bgCol);
           
           legalPanelContainer.add(legalPanel, BorderLayout.CENTER);
           legalPanelContainer.setBackground(bgCol);
           legalContainer.add(legalPanelContainer, BorderLayout.CENTER); 
           /***********************************************************/
           
           JPanel panelButtons = new JPanel();
           JButton backButton = new JButton("Back");
           JButton cancelButton = new JButton("Cancel");
           nextButtonLegal = new JButton("Next");
           nextButtonLegal.setEnabled(false);
           
           panelButtons.add(backButton);
           panelButtons.add(cancelButton);
           panelButtons.add(nextButtonLegal);
           
           backButton.addActionListener(new ButtonListener());
           backButton.addKeyListener(escapeHandler);
           cancelButton.addActionListener(new ButtonListener());
           cancelButton.addKeyListener(escapeHandler);
           nextButtonLegal.addActionListener(new ButtonListener());
           nextButtonLegal.addKeyListener(legalKeyHandler);
           checkLegal.addActionListener(new CheckBoxListener());
           checkLegal.addKeyListener(legalKeyHandler);
           panelButtons.setBackground(bgCol);
           legalContainer.add(panelButtons, BorderLayout.PAGE_END);
           /***********************************************************/
           
           getContentPane().add(legalContainer, cardLegal);
           
           /***********************************************************/
       }
       
       void setupInstallationComplete(){

           finishedContainer = new JPanel(new BorderLayout());
           
           Color bgCol=new Color(255,255,255);
           Font headLabelFont=new Font(null,Font.BOLD,15);
         
           /****************** Header Panel ***********************************/
           headPanelFinished = new JPanel(new GridBagLayout());
  
           GridBagConstraints c = new GridBagConstraints();
         
           
           JLabel headLabel=new JLabel(productTitle+" Installation Wizard Complete");
           c.fill = GridBagConstraints.PAGE_START;
           c.gridwidth=10;
           c.gridheight=1;
           c.weightx = 0;
           c.gridx = 0;
           c.gridy = 0;
           
           c.insets = new Insets(10,0,10,0);
           headPanelFinished.add(headLabel,c);
           headPanelFinished.setBackground(bgCol);
           headLabel.setFont(headLabelFont);
           
           /************************/
           controlPanelFinished = new JPanel();
           finishedButton = new JButton("Exit");
           controlPanelFinished.add(finishedButton);
           finishedButton.addActionListener(new ButtonListener());
           finishedButton.addKeyListener(
                      new KeyAdapter(){ 
                          public void keyPressed(KeyEvent ke){
                              if(ke.getKeyCode() == KeyEvent.VK_ENTER || ke.getKeyCode() == KeyEvent.VK_ESCAPE)
                                  System.exit(0);
                          }
                      }
           );
           controlPanelFinished.setBackground(bgCol);
          
           /**************************/
           // middle panel will be panelDownload
           
           getContentPane().add(finishedContainer, cardInstallationComplete);
       }
       
       void toggleProxyComponents(){
           boolean val = useProxyRadio.isSelected();
           proxyServerText.setVisible(val);
           proxyPortText.setVisible(val);
           autoDetectProxy.setVisible(val);
           proxyServerLabel.setVisible(val);
           proxyPortLabel.setVisible(val);
           
           if(proxyCombo.getItemCount()>0 && val)
               proxyCombo.setVisible(val);
           else {
        	   proxyCombo.setVisible(false);
        	   /*
        	    * Fix for bug#3280: reset the proxy server and port fields when no
        	    * proxy is selected
        	    */
        	   proxyServerText.setText("");
        	   proxyPortText.setText("");
           }
       }
       
       boolean validateProxy()
       {
           if(useProxyRadio.isSelected() && panelProxy.isVisible() && selectedOptionalComponents())
           {
               if(!proxyServerText.getText().trim().equals("") && !proxyPortText.getText().trim().equals("")){
                   try{
                       Integer.parseInt(proxyPortText.getText());
                       return true;
                   }catch (NumberFormatException ex){
                       JOptionPane.showMessageDialog(this,"Enter a valid proxy server and port to continue.");
                       return false;
                   }
               }else{
                   JOptionPane.showMessageDialog(this,"Enter a valid proxy server and port to continue.");
                   return false;
               }
           }
           else
           {
               return true;
           }    
       }

       boolean selectedOptionalComponents(){
           return (checkPerl.isSelected() || checkPerlSAX.isSelected() || checkPython.isSelected());
       }
       
       class installComplete implements Runnable{
           
           public void run(){
               /* Installation Complete */
               downloadLabel.setText("Installation Complete");
               
               finishedContainer.removeAll();
               finishedContainer.add(headPanelFinished, BorderLayout.PAGE_START);
               
               setupDownloadPanel(true);
               panelDownload.revalidate();
               
               if(downloadError){
                   // show back button
                   controlPanelFinished.removeAll();
                   JButton backButton=new JButton("Back");
                   backButton.addActionListener(new ButtonListener());
                   backButton.addKeyListener(escapeHandler);
                   controlPanelFinished.add(backButton);
                   controlPanelFinished.add(finishedButton);
               }else{
                   controlPanelFinished.removeAll();
                   controlPanelFinished.add(finishedButton);
               }
                     
               finishedContainer.add(controlPanelFinished, BorderLayout.PAGE_END);
               finishedContainer.revalidate();
               finishedContainer.add(panelDownload, BorderLayout.CENTER);
               
               cardCurrent = cardInstallationComplete;
               CardLayout cl = (CardLayout)(content.getLayout());
               cl.show(content, cardCurrent);
               
               finishedButton.requestFocusInWindow();
           }
       }
       
       class DownloadComponentsWrapper implements Runnable{
           
           public DownloadComponentsWrapper(){
           }
           
           public void run(){
               downloadOptionalComponents();
           }
           
           void downloadOptionalComponents(){
               downloadError = false;
               ArrayList<String[]> fileList = new ArrayList<String[]>();

               if(checkPerl.isSelected()){
                   fileList.add(new String[]{"Perl","Java.pm", defaultPerlJavaURL + "Java.pm"});
                   fileList.add(new String[]{"Perl","JavaArray.pm", defaultPerlJavaURL + "JavaArray.pm"});
                   fileList.add(new String[]{"Perl","JavaServer.jar", defaultPerlJavaURL + "JavaServer.jar"});
               }
               
               File saxDir = new File(targetDirectory + File.separator + "lib" + File.separator + "perl" + File.separator + "XML" + File.separator + "Parser");
               if(checkPerlSAX.isSelected()){
                   fileList.add(new String[]{"Perl SAX","PerlSAX.pm", defaultPerlSAXURL});
                   File installDir = new File(targetDirectory);
                                 
                   if(installDir.exists() && !saxDir.exists())
                       saxDir.mkdirs();
                   
               }
               
               if(checkPython.isSelected())
                   fileList.add(new String[]{"Python","jython_installer-2.2.jar", defaultPythonJythonURL});
               
               Downloader dl = new Downloader(targetDirectory);
               
               progressBar.setMaximum(100);
               progressBar.setMinimum(0);
               
               dl.setProxy(proxyServerText.getText(), proxyPortText.getText());
               dl.setProgressBar(progressBar);
               
               String[] tmp;
               String libDir = targetDirectory + File.separator + "lib";
               for(int i=0; i<fileList.size(); i++){
                   tmp = (String[])fileList.get(i);
                   
                   downloadLabel.setText("Downloading: " + tmp[0] + " - " + tmp[1]);
                   if(i==0)
                       panelDownload.revalidate();
                   
                   panelDownload.paintImmediately(panelDownload.getVisibleRect());
                  
                   if(tmp[0].equalsIgnoreCase("Perl"))
                       dl.setFilename(libDir + File.separator + "perl" + File.separator + tmp[1]);
                   else if(tmp[0].equalsIgnoreCase("Perl SAX"))
                       dl.setFilename(saxDir.getAbsolutePath() + File.separator + tmp[1]);         
                   else if(tmp[0].equalsIgnoreCase("Python"))
                       dl.setFilename(libDir + File.separator + "python" + File.separator + tmp[1]);
                   
                   dl.setURL(tmp[2]);
                   
                   progressBar.setValue(0);
                   
                   Thread dlTh=new Thread(dl);
                   dlTh.start(); 
                   do{
                       
                       if(dl.getError()!=""){
                           installLogOutput("Error Downloading " + tmp[1] + ": " + dl.getError());
                           downloadError = true;
                           break;
                       }
                       
                       try{
                           Thread.sleep(100);
                       }catch(InterruptedException ie){
                           ie.printStackTrace();
                       }
                       
                   }while(progressBar.getValue() < 100);
                   
                   while (dlTh.isAlive()){
                	  // Download may be done, but dl thread may still have file open,
                	  // wait until thread exits 
                   }
                   
                   if(progressBar.getValue()==100){
                       installLogOutput("Successfully downloaded " + tmp[1]);
                       if(tmp[0].equals("Python"))
                       {
                            
                           if(installJython(dl.getFilename()))
                           {
                               installLogOutput("Successfully installed Jython");
                           
                               if(setEnvironmentVariable("Python"))
                                   installLogOutput("Successfully added settings.py");
                               else
                                   installLogOutput("Error: cannot create settings.py, manually add TRAFCI_PYTHON_JSERVER to the environment, please see README in the samples directory.");
                           
                           }else
                               installLogOutput("Error installing Jython, please try manually. See the README in the samples directory for more information.");
                       }
                       else
                       {
                           if(tmp[0].equals("Perl") && tmp[1].endsWith("jar"))
                           {
                               if(setEnvironmentVariable("Perl"))
                                   installLogOutput("Successfully added settings.pl");
                               else
                                   installLogOutput("Error: cannot create settings.pl, manually add TRAFCI_PERL_JSERVER to environment, please see README in the samples directory.");
                           }
                       }
                   }
               }
               
               // done
               
           }
       };
       
       class pbChangeListener implements ChangeListener
       {
               public void stateChanged(ChangeEvent e) 
               {
                   JProgressBar bar = (JProgressBar)e.getSource();
                   bar.paintImmediately(bar.getVisibleRect());
               }
       };

   }
   
   void installInCmdMode()
   {
		try {
			Class.forName("ControlCSignalHandler").newInstance();
		} catch (Throwable e) {
		}
      

      String line=null;
      File targetDir=null;
      File jdbcFile=null;
      
      if(enableOpenSource){
          
          if(!silentInstall){
              System.out.println("\nType Y for a standard installation, or N for optional components only.\n");
              
              do{
                  try
                  {
                      System.out.print("Standard Installation [Y]: ");
                      line=bufReader.readLine();
                      Thread.sleep(100);
                  } catch (IOException ioe)
                  {
                      System.err.println(ioe);
                  }catch(Exception ex){}
              }while( !(line != null && (line.equalsIgnoreCase("Y") || line.equalsIgnoreCase("N") || line.equalsIgnoreCase(""))) );
              
              if(line.equalsIgnoreCase("N"))
                  standardInstall = false; // true by default
          }
      }
      
      
      while (standardInstall)
      {
          
          if(!silentInstall){
             try
             {
                System.out.print("\n"+ NEW_JDBC +
                   "\n--------------------------------------"+
                   "\nEnter the location and file name [" + defaultInstallJDBC +"]:");
                line=bufReader.readLine();
                Thread.sleep(100);
             } catch (IOException ioe)
             {
                System.err.println(ioe);
             }
             catch(Exception ex){}
         }else
             line = defaultInstallJDBC;
          
         if (line != null)
         {
            if(line.trim().equals(""))
                line = defaultInstallJDBC;
            
            jdbcFile=new File(line.toString().trim());
            if (jdbcFile.exists() && jdbcFile.getName().endsWith(".jar") && jdbcFile.canRead())
            {
               boolean jarFound=false;
               try
               {
                  ZipFile zf = new ZipFile(jdbcFile);
                  int size = zf.size();
                  Enumeration<?> entries = zf.entries();
                  for (int i=0; i<size; i++)
                  {
                     ZipEntry entry = (ZipEntry)entries.nextElement();
                     if (!entry.isDirectory() && entry.getName().equals(t4jdbcIdentifier))
                        jarFound=true;
                  }
               } catch (Exception e)
               {
               }
               if (jarFound)
               {
                  jdbcFileLoc=jdbcFile.getAbsolutePath();
                  break;
               }
            }
            else
            {
                System.out.println("\nCould not find the file specified or invalid JAR file ");
                if(silentInstall)
                    System.exit(1);
            }
         }
      }
      
      line = null;
      if(!silentInstall)
          System.out.println();

		while (standardInstall) {

			if (!silentInstall) {
				System.out.print("\n" + productTitle
						+ "\n--------------------------------"
						+ "\nEnter the installation directory: ");

				try {
					line = bufReader.readLine();
					if (line == null
							||  "".equals(line.trim())) {
						line = defaultInstallDir;
					}
					Thread.sleep(100);
				} catch (IOException ioe) {
					System.err.println(ioe);
				} catch (Exception ex) {
				}
			} else
				line = defaultInstallDir;

			if (line != null) {
				if (!line.endsWith(File.separator))
					line += File.separator;

				if (line.equalsIgnoreCase(""))
					targetDir = new File(defaultInstallDir.trim());
				else
					targetDir = new File(line.toString().trim());

				if (!targetDir.exists())
					targetDir.mkdirs();
				if (!targetDir.isDirectory() && !targetDir.canWrite()) {
					System.out
							.println("\nCould not find the directory specified or invalid permissions ");
					if (silentInstall)
						System.exit(1);
				} else {
					targetDirectory = targetDir.getAbsolutePath();
					// If target directory is not the same as the default
					// directory and does not contain the product name or title
					// then we append the product name to the target directory
					if (!targetDirectory.equalsIgnoreCase(wDefaultInstallDir)
							&& (targetDirectory.toUpperCase().indexOf(
									"traci".toUpperCase()) < 0)
							&& (targetDirectory.toUpperCase().indexOf(
									"Trafodion Command Interface"
											.toUpperCase()) < 0)) {
						targetDirectory += (osName.startsWith("window") ? "\\"
								: "/") + productName;
					}
					break;
				}
			}
		}

      if(standardInstall)
      {    
          String jarFileName = "";
          try{
              jarFileName = getJarFileName();
          }catch(Exception ex){
              System.out.println("Fatal Error: could not access installation jar, quitting...");
              System.exit(1);
          }   
          processCmdMode(jarFileName);
      }
      
      if(enableOpenSource && !silentInstall){
          if(standardInstall){
              
              System.out.println();
              do{
                  System.out.print("Do you want to install the optional components? [Y]: ");
                  try
                  {
                     line=bufReader.readLine();
                     Thread.sleep(100);
                  } catch (IOException ioe)
                  {
                     System.err.println(ioe);
                  }
                  catch(Exception ex){}
              }while( !(line != null && (line.equalsIgnoreCase("Y") || line.equalsIgnoreCase("N") || line.equalsIgnoreCase(""))) );
          }
              
          
          if((line!=null && !line.equalsIgnoreCase("N")) || !standardInstall)
              processOptionalComponentsCmdMode();
      }
      
      System.out.println("\n" + productTitle + " Installation Complete.");
      
      System.exit(0);
   }
   
   String resolvePath(File f){
       String retval = "";
       try{
           retval = f.getCanonicalPath();
       }
       catch(IOException ex)
       {
           retval = f.getAbsolutePath();
       }
       
       return retval;
   }
   
   String resolvePath(String filename){
       File f = new File(filename);
       
       String retval = "";
       try{
           retval = f.getCanonicalPath();
       }
       catch(IOException ex)
       {
           retval = f.getAbsolutePath();
       }
       
       return retval;
   }
   
   void processOptionalComponentsCmdMode()
   {
       
       Downloader dl = new Downloader();
       String installPath = "";
       String line = "";
       String perlJava = "", perlSAX = "", pythonJython = "";
       
       
       System.out.println();
       System.out.println("******************************************************************");
       System.out.println("***                                                            ***");
       System.out.println("***   Trafodion Command Interface, Optional Components         ***");
       System.out.println("***                                                            ***");
       System.out.println("***                                                            ***");
       System.out.println("***                  Terms and Conditions                      ***");
       System.out.println("***                                                            ***");
       System.out.println("*** The ability to download open source extensions is provided ***");
       System.out.println("*** for your convenience only. Software provided under any     ***");
       System.out.println("*** open source licensing model is governed solely by such     ***");
       System.out.println("*** open source licensing terms.                               ***");
       System.out.println("***                                                            ***");
       System.out.println("******************************************************************");
       System.out.println();
       
       String terms = "";
       do{
           try{
               System.out.print("Do you agree to these terms? (Y or N): ");
                terms = bufReader.readLine();
                if(terms == null)
                    System.exit(0);
                
           }catch(IOException ioe){
               System.err.println(ioe);
               System.exit(1);
           }
       }while(! (terms != null && (terms.equalsIgnoreCase("Y") || terms.equalsIgnoreCase("N"))) );
       
       if(!terms.equalsIgnoreCase("Y"))
       {    
           if(!standardInstall)
               System.out.println("\nQuitting " + productTitle + " Installation.");
           else
               System.out.println("\n" + productTitle + " Installation Complete.");
           
           System.exit(0);
       }
           
       if(!standardInstall){
       
           do{
               try{
                   System.out.print("Enter your installation directory [" + defaultInstallDir + "]: ");
                   installPath = bufReader.readLine();
                   if(installPath != null){
                       installPath = (installPath.trim().equals("")) ? defaultInstallDir : installPath.trim();
                       if(!validateNCIPath(installPath))
                           System.out.println("\n" + ERROR_INVALID_NCI_DIR + "\n");
                   }else{
                       System.exit(0);
                   }
               }catch(IOException ioe){
                   System.err.println(ioe);
               }
           }while(! (installPath != null && validateNCIPath(installPath)));
           
       }else
           installPath = targetDirectory;

       installPath = (installPath.trim().equals("")) ? defaultInstallDir : resolvePath(installPath.trim());
       targetDirectory = installPath;
       
       dl.setAsciiMode(true);
       
       try{
           
           System.out.print("Use a proxy server? [N]: ");
           line = bufReader.readLine();
           Thread.sleep(100);
           if(line != null && line.trim().equalsIgnoreCase("Y")){
               proxyCmdMode(dl);
           }
           
           System.out.print("\nInstall Perl JavaServer extensions? [Y]: ");
           perlJava = bufReader.readLine();
           Thread.sleep(100);
           if(cmdModeOptionTest(perlJava)){
               System.out.println("Perl JavaServer requires 3 files: Java.pm, JavaArray.pm, and JavaServer.jar");
               System.out.print("URL of the folder which contains these files\n[" + defaultPerlJavaURL + "]: ");
               String tmpURL = bufReader.readLine();
               Thread.sleep(100);
               if(tmpURL != null && !tmpURL.trim().equals("")){
                   /* this must be a URL directory */
                   if(!tmpURL.endsWith("/"))
                       tmpURL += "/";
                   
                   defaultPerlJavaURL = tmpURL;
               }
           }
           
           System.out.print("\nInstall Perl XML SAX Module? [Y]: ");
           perlSAX = bufReader.readLine();
           Thread.sleep(100);
           if(cmdModeOptionTest(perlSAX)){
               System.out.print("Perl SAX XML Module URL (PerlSAX.pm)\n[" + defaultPerlSAXURL + "]: ");
               String tmpURL = bufReader.readLine();
               Thread.sleep(100);
               if(tmpURL != null && !tmpURL.trim().equals(""))
                   defaultPerlSAXURL = tmpURL;
           }
           
           
           System.out.print("\nInstall Jython, a Java implementation of Python? [Y]: ");
           pythonJython = bufReader.readLine();
           Thread.sleep(100);
           if(cmdModeOptionTest(pythonJython)){
               System.out.print("Jython URL (jython_installer-2.2.jar)\n[" + defaultPythonJythonURL + "]: ");
               String tmpURL = bufReader.readLine();
               Thread.sleep(100);
               if(tmpURL != null && !tmpURL.trim().equals(""))
                   defaultPythonJythonURL = tmpURL;
           }
      
           
           if(cmdModeOptionTest(perlJava)){
               System.out.println("\nDownloading Perl JavaServer [1 of 3] - Java.pm");
               dl.setFilename(installPath + File.separator + "lib" + File.separator + "perl" + File.separator + "Java.pm");
               dl.setURL(defaultPerlJavaURL + "Java.pm");
               downloadFileCmdMode(dl);
               
               System.out.println("Downloading Perl JavaServer [2 of 3] - JavaArray.pm");
               dl.setFilename(installPath + File.separator + "lib" + File.separator + "perl" + File.separator + "JavaArray.pm");
               dl.setURL(defaultPerlJavaURL + "JavaArray.pm");
               downloadFileCmdMode(dl);
               
               System.out.println("Downloading Perl JavaServer [3 of 3] - JavaServer.jar");
               dl.setFilename(installPath + File.separator + "lib" + File.separator + "perl" + File.separator + "JavaServer.jar");
               dl.setURL(defaultPerlJavaURL + "JavaServer.jar");
               if(downloadFileCmdMode(dl)){
                   if(setEnvironmentVariable("Perl"))
                       System.out.println("Successfully added settings.pl");
                   else
                       System.out.println("Error: cannot add settings.pl, manually add TRAFCI_PERL_JSERVER to environment, please see README in the samples directory.");
               }
           }

           if(cmdModeOptionTest(perlSAX)){
               System.out.println("\nDownloading Perl XML SAX Module [1 of 1] - PerlSAX.pm");
               File installDir = new File(installPath);
               File saxDir = new File(installPath + File.separator + "lib" + File.separator + "perl" + File.separator + "XML" + File.separator + "Parser");
                              
               if(installDir.exists() && !saxDir.exists())
                   saxDir.mkdirs();
               
               dl.setFilename(saxDir.getAbsolutePath() + File.separator + "PerlSAX.pm");
               dl.setURL(defaultPerlSAXURL);
               downloadFileCmdMode(dl);
           }
           
           if(cmdModeOptionTest(pythonJython)){
               System.out.println("\nDownloading Jython [1 of 1] - jython_installer-2.2.jar");
               dl.setFilename(installPath + File.separator + "lib" + File.separator + "python" + File.separator + "jython_installer-2.2.jar");
               dl.setURL(defaultPythonJythonURL);
               downloadFileCmdMode(dl);
               if(installJython(dl.getFilename())){
                   System.out.println("Successfully Installed Jython.");
                   
                   if(setEnvironmentVariable("Python"))
                       System.out.println("Successfully added settings.py");
                   else
                       System.out.println("Error: cannot add settings.py, manually add TRAFCI_PYTHON_JSERVER to environment, please see README in the samples directory.");
                   
               }else
                   System.out.println("Error installing Jython, please try manually. See the README in the samples directory for more information.");
               
           }

       }catch(Exception ex){
           System.err.println(ex.toString());
       }

   }
   
   boolean cmdModeOptionTest(String result){
       if((result != null && result.trim().equals("")) || (result != null && result.trim().equalsIgnoreCase("Y")))
           return true;
       else
           return false;
   }
   
   boolean installJython(String jarFile){
       Enumeration<?> entries;
       ZipFile zipFile;
       byte[] buffer = new byte[1024];
       int len;

       ArrayList<String> requiredFiles = new ArrayList<String>();
       requiredFiles.add("jython.jar");
       requiredFiles.add("re.py");
       requiredFiles.add("sre.py");
       requiredFiles.add("sre_compile.py");
       requiredFiles.add("sre_parse.py");
       requiredFiles.add("string.py");
       requiredFiles.add("sre_constants.py");
       requiredFiles.add("copy_reg.py");
       int extractCount = 0;
       
       if(!(new File(jarFile).exists())) {
    	   return false;
       }
           
       
       try
       {
           
           zipFile = new ZipFile(jarFile);
      
           entries = zipFile.entries();

           while(entries.hasMoreElements()) {
             ZipEntry entry = (ZipEntry)entries.nextElement();             
             
             if(requiredFiles.contains(new File(entry.getName().toLowerCase()).getName())){
                 OutputStream out = new BufferedOutputStream(new FileOutputStream((new File(jarFile).getParent()) + File.separator + new File(entry.getName()).getName()));
                 InputStream in = zipFile.getInputStream(entry);
                 while((len = in.read(buffer)) >= 0)
                   out.write(buffer, 0, len);
    
                 in.close();
                 out.close();
                 extractCount++;
             }
           }
           zipFile.close();
           new File(jarFile).delete();
           if(extractCount == requiredFiles.size())
               return true;
           
       }catch (IOException ioe) {
           //System.err.println(ioe.toString());
       }
       return false;
   }
   
   boolean downloadFileCmdMode(Downloader dl){
       dl.download();
       
       if(dl.getError() != "")
           System.out.println("Could not complete download, error:\n" + dl.getError());
       else
       {
           System.out.println(" 100%");
           return true;
       }
       
       return false;
   }
   
   boolean validateNCIPath(String path){
       // build path
       if(!path.endsWith(File.separator))
           path += File.separator;
       File ciJar = new File(path + "lib" + File.separator + "trafci.jar");
       
       if(ciJar.exists() && ciJar.isFile()){
           try
           {
              ZipFile zf = new ZipFile(ciJar);
              int size = zf.size();
              Enumeration<?> entries = zf.entries();
              for (int i=0; i<size; i++)
              {
                 ZipEntry entry = (ZipEntry) entries.nextElement();
                 if (!entry.isDirectory() && entry.getName().equals(ciJarIdentifier))
                 {
                    return true;
                 }
              }
           } catch (Exception e)
           {
               ;
           }
       }
       
       return false;
   }
   
   /* java has no way to do this, but in windows we can exec reg.exe */
   boolean setEnvironmentVariable(String type){     

       if(!targetDirectory.endsWith(File.separator))
           targetDirectory += File.separator;
       
       if(type.equalsIgnoreCase("Python")){
           try{
               FileOutputStream pythonSettings = new FileOutputStream(targetDirectory + File.separator + "bin" + File.separator + "settings.py");
               PrintStream p = new PrintStream( pythonSettings );
               p.println("import os");
               
               
               if(osName.startsWith("window")){
                   targetDirectory = targetDirectory.replaceAll("\\\\", "\\\\\\\\");
                   p.println("os.environ[\"TRAFCI_PYTHON_JSERVER\"] = \"" + targetDirectory + "lib\\\\python\\\\jython.jar\"");
               }else
                   p.println("os.environ[\"TRAFCI_PYTHON_JSERVER\"] = \"" + targetDirectory + "lib/python/jython.jar\"");
               
               
               p.println("def getJythonClasspath():");
               p.println("    return os.environ[\"TRAFCI_PYTHON_JSERVER\"]");
               
               p.close(); 
               return true;
           }catch(Exception ex){
               System.err.println(ex);
               ;
           }
       }else{
           try{
               FileOutputStream perlSettings = new FileOutputStream(targetDirectory + File.separator + "bin" + File.separator + "settings.pl");
               PrintStream p = new PrintStream( perlSettings );
               
               p.println("#!/usr/bin/perl");
               p.println("$ENV{\'TRAFCI_PERL_JSERVER\'} = '" + targetDirectory + "lib" + File.separator + "perl" + File.separator + "JavaServer.jar';");
               p.println("print $ENV{'TRAFCI_PERL_JSERVER'};");
               p.close();
               return true;
           }catch(Exception ex){
               System.err.println(ex);
               ;
           }
       }
       
       return false;
   }
   
   String runCommand(String command){
       String result = "";
       try{
           String tmp;
           Process p = Runtime.getRuntime().exec(command);
           BufferedReader buffer = new BufferedReader(new InputStreamReader(p.getInputStream()));
           
           while ((tmp = buffer.readLine()) != null) {
               result += tmp;
           }
                           
           buffer.close();
           p.destroy();
           
       }catch(Exception ex){
           //ex.printStackTrace();
           result = "Error";
       }    
       
        return result;
   }
   
   void proxyCmdMode(Downloader dl){
       String line = "";
       String defaultProxyServer = "";
       String defaultProxyPort = "";
       
       try{
           System.out.print("Attempt to auto-detect proxy server(s)? [Y]: ");
           line = bufReader.readLine();
          
           if(line != null && line.trim().equals("") || (line != null && line.trim().equalsIgnoreCase("Y"))){
               ArrayList<String> proxyList = dl.findProxy();
               if(proxyList != null && proxyList.size() > 0)
               {
                   System.out.println("\nFound the following proxy server(s):");
                   String[] tmp;
                   for(int i=0;i<proxyList.size();i++){
                       
                       tmp = proxyList.get(i).split(":");
                       if(i==0){
                           defaultProxyServer = tmp[0];
                           defaultProxyPort = tmp[1];
                       }
                       System.out.println(proxyList.get(i));
                   }
                   System.out.println();
                   
               }else
                   System.out.println("Auto detection did not find any proxy servers, please enter one below.");
           }
           
           if(line != null){
               /* Get Proxy Server */
               do{
                   line = "Enter the proxy server (do not include the port)";
                   line += (defaultProxyServer.trim().equals("")) ? ": " : " [" + defaultProxyServer + "]: ";
                   
                   System.out.print(line);
                   line = bufReader.readLine();
                   Thread.sleep(100);
                   if(line != null && !line.trim().equals(""))
                       defaultProxyServer = line;
               }while(defaultProxyServer.equals(""));
               
               do{
                   /* Get Proxy Port */
                   line = "Enter the proxy port";
                   line += (defaultProxyPort.trim().equals("")) ? ": " : " [" + defaultProxyPort + "]: ";
                   
                   System.out.print(line);
                   line = bufReader.readLine();
                   Thread.sleep(100);
                   if(line != null && !line.trim().equals("")){
                       defaultProxyPort = line;
                   
                       try{
                           Integer.parseInt(defaultProxyPort);
                       }catch(Exception ex){
                           defaultProxyPort = "";
                           System.out.println("Error: Please enter a valid port number");
                       }
                   }
               }while(defaultProxyPort.equals(""));
               
               dl.setProxy(defaultProxyServer, defaultProxyPort);
           }else
               System.exit(0);
       }catch(IOException ex){
           ;
       }catch(Exception ex){}
   }
   
   @SuppressWarnings("unused")
void processCmdMode(String zipfile)
   {
      File currentArchive = new File(zipfile);
      SimpleDateFormat formatter = new SimpleDateFormat ("MM/dd/yyyy hh:mma",Locale.getDefault());
      boolean overwrite = false;
      ZipFile zf = null;
      FileOutputStream out = null;
      InputStream in = null;
      File outputDir=new File(targetDirectory);
      File jdbcFile=new File(jdbcFileLoc);

      if (outputDir != null && !outputDir.exists())
      {
         outputDir.mkdirs();
      }
      
      ArrayList<String> installerFiles = new ArrayList<String>();
      installerFiles.add("Downloader.class");
      installerFiles.add("DownloaderError.class");
      installerFiles.add("Ebcdic2Ascii.class");
      installerFiles.add("ControlCSignalHandler.class");
      installerFiles.add("ControlCSignalHandler$1.class");

      try
      {
         zf = new ZipFile(currentArchive);
       
         int size = zf.size();
         int extracted = 0;
         Enumeration<?> entries = zf.entries();
        
         for (int i=0; i<size; i++)
         {
            ZipEntry entry = (ZipEntry) entries.nextElement();
          
            if (entry.isDirectory())
               continue;
            
            String pathname = entry.getName();
           
            if (myClassName.equals(pathname) || MANIFEST.equals(pathname.toUpperCase()) || 
                    pathname.startsWith(installerFilesStartWith) || installerFiles.contains(pathname))
               continue;
            
            extracted ++;
            in = zf.getInputStream(entry);
            File outFile = new File(outputDir, pathname);
            
            Date archiveTime = new Date(entry.getTime());
            if (overwrite==false)
            {
               if (outFile.exists())
               {
                  String msg = checkFileExists (outFile,entry,formatter,archiveTime);
                  String line=null;
                  do
                  {
                     if(!silentInstall){
                         try
                         {
                            System.out.println();
                            System.out.println("-------------FILE OVERWRITE WARNING-----------------");
                            System.out.print(msg+"[Y = Yes, N = No, A = Yes to All] :");
                            line=bufReader.readLine();
                         } catch (IOException ioe)
                         {
                            System.err.println(ioe);
                         }
                     }else
                         line = "A";
                  } while (line != null && !line.trim().equalsIgnoreCase("A") && !line.trim().equalsIgnoreCase("Y") && !line.trim().equalsIgnoreCase("N"));

                  int result=0;
                  
                  if(line == null){
                      System.out.println("An error has occurred, please re-install the product.");
                      System.exit(0);
                  }
                  
                  if (line.trim().equalsIgnoreCase("A")) result = 1;
                  else if (line.trim().equalsIgnoreCase("N")) result = 2;
                  if (result == 2)
                  { // No
                     extracted --;
                     continue;
                  }
                  else if (result == 1)
                  { //YesToAll
                     overwrite = true;
                  }
               }
            }
            handleSamplesDir(archiveTime, in, outFile, out, jdbcFile, outputDir);
         }
        
         handlePermission(targetDirectory);
         zf.close();
         
         if(silentInstall)
             System.out.println();
         
         if (osName.equalsIgnoreCase("z/OS") )
         {
            String fileEncoding = System.getProperty("file.encoding");
            if (fileEncoding.equalsIgnoreCase("Cp1047"))
            {
               System.out.println();
               System.out.println("******************************************************************");
               System.out.println("***    Detected encoding : " + fileEncoding  +"                              ***");
               System.out.println("***    Converting Core files from EBCDIC to ASCII              ***");
               System.out.println("******************************************************************");

               Ebcdic2Ascii ebcdic2ascii = new Ebcdic2Ascii();
               ebcdic2ascii.listContents(new File(targetDirectory));
            }
         }        
         
         System.out.println( extractedMessage(extracted, zipfile, outputDir, false) ) ;
        
      } catch (Exception e)
      {
        System.err.println(e);
         
         if (zf!=null)
         {
            try
            {
               zf.close();
            } catch (IOException ioe)
            {
               ;
            } 
         }
         if (out!=null)
         {
            try
            {
               out.close();
            } catch (IOException ioe)
            {
               ;
            } 
         }
         if (in!=null)
         {
            try
            {
               in.close();
            } catch (IOException ioe)
            {
               ;
            } 
         }
      }
   }
   
   private String getJarFileName()
   {
      myClassName = this.getClass().getName();
      installerFilesStartWith=myClassName+"$";
      myClassName = myClassName + ".class";

      if(DEBUG)
        return "C:\\Temp\\trafci-Build\\regress\\trafciInstaller.jar";
      else{
    	  //URL urlJar = this.getClass().getClassLoader().getSystemResource(myClassName);
          URL urlJar = ClassLoader.getSystemResource(myClassName);
          
           String urlStr = urlJar.toString().replaceAll("%20"," ");
          
           urlStr = urlStr.replaceAll("%22","\"");
           urlStr = urlStr.replaceAll("%23","#");
           urlStr = urlStr.replaceAll("%25","%");
           urlStr = urlStr.replaceAll("%2a","*");
           urlStr = urlStr.replaceAll("%3b",";");
           urlStr = urlStr.replaceAll("%3c","<");
           urlStr = urlStr.replaceAll("%3e",">");
           urlStr = urlStr.replaceAll("%3f","?");
           urlStr = urlStr.replaceAll("%5b","[");
           urlStr = urlStr.replaceAll("%5d","]");
           urlStr = urlStr.replaceAll("%5e","^");
           urlStr = urlStr.replaceAll("%60","`");
           urlStr = urlStr.replaceAll("%7b","{");
           urlStr = urlStr.replaceAll("%7c","|");
           urlStr = urlStr.replaceAll("%7d","}");
           
          int from = "jar:file:".length();
          int to = urlStr.indexOf("!/");
          return urlStr.substring(from, to);
      }
   }

   public static void main(String args[])
   {

      jdbcFileLoc=null;
      targetDirectory=null;
      
 
      Installer inst=new Installer();
      
      enableOpenSource = true;
      
      osName = System.getProperty("os.name");
      if (osName != null)
      {
         osName = osName.trim().toLowerCase();
      }
      else
      {
         osName= "window";
      }

      if (osName.startsWith("window"))
      {
          /*try {
              String line;
              Process p = Runtime.getRuntime().exec("cmd /c set programfiles") ;
              BufferedReader input =
                 new BufferedReader
                   (new InputStreamReader(p.getInputStream()));
              line = input.readLine();
              line = line.substring(line.indexOf("=")+1);
              input.close();
              p.destroy();
              wDefaultInstallDir = line + wDefaultInstallDir;
           } catch (Exception e) {
              wDefaultInstallDir = "C:\\Program Files" + wDefaultInstallDir;
           }*/

         defaultInstallDir=wDefaultInstallDir;
         defaultInstallJDBC=wDefaultInstallJDBC;
      }
      else
      {
         defaultInstallDir=nixDefaultInstallDir;
         defaultInstallJDBC=nixDefaultInstallJDBC;
      }

      
      if (args.length == 1 && args[0].equalsIgnoreCase("v"))
      {
         System.out.println(vprocStr);
      }
      else if(args.length > 0)
      {

          boolean silentFlag = false;
          boolean cmFlag = false;
          boolean helpFlag = false;
      
          for(int i=0; i<args.length; i++){
              
              if(!args[i].startsWith("-"))
                  args[i] = "-"+args[i];
          

              if(args[i].equalsIgnoreCase("-help") || args[i].equalsIgnoreCase("--help"))
              {
                  helpFlag = true;
              }
              
              if(args[i].equalsIgnoreCase("-silent") || args[i].equalsIgnoreCase("--silent"))
              {
                  silentFlag = true;
              }
              
              if(args[i].equalsIgnoreCase("-cm") || args[i].equalsIgnoreCase("--cm"))
              {
                  cmFlag = true;
              }
              
              if(args[i].equalsIgnoreCase("-jdbcFile") || args[i].equalsIgnoreCase("--jdbcFile"))
              {
                  if(i+1<args.length){
                      defaultInstallJDBC = args[i+1].trim();
                      i++;
                  }
              }
              else if(args[i].equalsIgnoreCase("-installDir") || args[i].equalsIgnoreCase("--installDir"))
              {
                  if(i+1<args.length){
                      defaultInstallDir = args[i+1].trim();
                      i++;
                  }
              }
          }
          
          
          if(helpFlag){
              inst.printUsage();
              System.exit(0);
          }
          else if(silentFlag)
          {
              if(!defaultInstallJDBC.trim().equals("") && !defaultInstallDir.trim().equals(""))
              {
                  silentInstall = true;
                  inst.installInCmdMode();
              }
              else
              {
                  if(!defaultInstallJDBC.trim().equals(""))
                      System.out.println("Please specify an installation directory.");
                  else if(!defaultInstallDir.trim().equals(""))
                      System.out.println("Please specify the JDBC filename.");
                     
                  inst.printUsage();
                  System.exit(1);
                  
              }
          }else if(cmFlag){
              
              System.out.println();
              System.out.println("******************************************************************");
              System.out.println("***                                                            ***");
              System.out.println("*** Welcome to Trafodion Command Interface Installer           ***");
              System.out.println("***                                                            ***");
              System.out.println("***                                                            ***");
              System.out.println("***    NOTE: The installer requires the JDBC Type 4            ***");
              System.out.println("***          Driver to be installed on your workstation.       ***");
              System.out.println("***                                                            ***");
              System.out.println("******************************************************************");
              inst.installInCmdMode();
              
          }          
          else
          {
              inst.printUsage();
              System.exit(1);           
          }
              
      }
      else
      {
         inst.new InstallerGUIMode().initialize(productTitle);
      }

   }
   
   public void printUsage(){
       System.out.println("Usage: java -jar <installer jar> [ -help | <-cm|-silent> [-jdbcFile <jdbc filename>] [-installDir <install dir>] ]");
   }
   
   Installer()
   {
   }

   public String checkFileExists (File outFile, ZipEntry entry, SimpleDateFormat formatter, Date archiveTime)
   {

      String msg = "";
      Date existTime = new Date(outFile.lastModified());
      Long archiveLen = new Long(entry.getSize());
      msg = "File name conflict: "
         + "There is already a file with "
         + "that name on the disk!\n"
         + "\nFile name: " + outFile.getName()
         + "\nExisting file: "
         + formatter.format(existTime) + ",  "
         + outFile.length() + "Bytes"
         + "\nFile in archive:"
         + formatter.format(archiveTime) + ",  "
         + archiveLen + "Bytes"
         +"\n\nWould you like to overwrite the file?";
      return msg;
   }

   public void handleSamplesDir(Date archiveTime, InputStream in, File outFile, FileOutputStream out, File jdbcFile, File outputDir) throws FileNotFoundException, IOException
   {
	  byte[] buf = new byte[1024];
      StringWriter sw=null;
      File parent = new File(outFile.getParent());
      if (parent != null && !parent.exists())
      {
         parent.mkdirs();
      }
      out = new FileOutputStream(outFile);

      if (outFile.getName().equals(productScriptName+".cmd") || outFile.getName().equals(productScriptName+".sh") ||
          outFile.getName().endsWith("py") || outFile.getName().endsWith("pl"))
      {
         sw = new StringWriter();
         while (true)
         {
            int readbyte = in.read();
            if (readbyte <=0)
               break;
            if ((readbyte == 13) && !(osName.startsWith("window")))
            {
            }
            else
               sw.write(readbyte);
         }
         String batchFile = handleDirectorySlash(sw, jdbcFile,outputDir, out);
         StringReader sr=new StringReader(batchFile);
         if (sr != null)
         {
            int ch;
            while ((ch=sr.read()) != -1)
               out.write(ch);
         }
      }
      else
      {
         while (true)
         {
            int nRead = in.read(buf, 0, buf.length);
            if (nRead <= 0)
               break;
            out.write(buf, 0, nRead);
         }
      }
      out.close();
      outFile.setLastModified(archiveTime.getTime());
      new FilePermission(outFile.getName(),"read,write,execute,delete");
   }

   public String handleDirectorySlash(StringWriter sw, File jdbcFile, File outputDir, FileOutputStream out)
   {
	  String batchfile=sw.toString();
      
      String replace=jdbcFile.getAbsolutePath()+File.pathSeparator+outputDir.getAbsolutePath()+File.separator+productJarFile;
      String replacePython=jdbcFile.getAbsolutePath()+File.pathSeparator+outputDir.getAbsolutePath()+File.separator+productJarFile+File.pathSeparator;
      String replacePerl=jdbcFile.getAbsolutePath()+File.pathSeparator+outputDir.getAbsolutePath()+File.separator+productJarFile+File.pathSeparator;
      String replacePerlLib = outputDir.getAbsolutePath() + File.separator + "lib" + File.separator + "perl";
      String replacePythonLib = outputDir.getAbsolutePath() + File.separator + "lib" +File.separator + "python";

      replace=replace.replaceAll(File.separator+File.separator,"#####PATH#####");
      replacePython=replacePython.replaceAll(File.separator+File.separator,"#####PATH#####");
      replacePerl=replacePerl.replaceAll(File.separator+File.separator,"#####PATH#####");
      replacePerlLib=replacePerlLib.replaceAll(File.separator+File.separator,"#####PATH#####");
      replacePythonLib=replacePythonLib.replaceAll(File.separator + File.separator, "#####PATH#####");
       
    
      // $ is special character for regex, need to escape it before doing replaceAll
      // so replace any $ in directory names with \$
      replace=replace.replaceAll("\\$","\\\\\\$");
      replacePython=replacePython.replaceAll("\\$","\\\\\\$");
      replacePerl=replacePerl.replaceAll("\\$","\\\\\\$");
      replacePerlLib=replacePerlLib.replaceAll("\\$","\\\\\\$");
      replacePythonLib=replacePythonLib.replaceAll("\\$","\\\\\\$");
           
      batchfile=batchfile.replaceAll(classVarToReplace,replace).replaceAll("#####PATH#####",File.separator+File.separator+File.separator+File.separator);
      batchfile=batchfile.replaceAll(pythonClassVarToReplace,replacePython).replaceAll("#####PATH#####",File.separator+File.separator+File.separator+File.separator);
      batchfile=batchfile.replaceAll(perlClassVarToReplace,replacePerl).replaceAll("#####PATH#####",File.separator+File.separator+File.separator+File.separator);
      batchfile =batchfile.replaceAll(perlLibPathToReplace, replacePerlLib).replaceAll("#####PATH#####", File.separator + File.separator + File.separator + File.separator);
      batchfile =batchfile.replaceAll(pythonLibPathToReplace, replacePythonLib).replaceAll("#####PATH#####", File.separator + File.separator + File.separator + File.separator);
      return batchfile;
   }

   private void handlePermission(String directoryName) throws IOException, InterruptedException
   {

      if (!osName.startsWith("window"))
      {
     	 Runtime rt = Runtime.getRuntime();
         Process proc = rt.exec("chmod -R 755 "+ directoryName +"/bin");
         proc.waitFor();
      }
   }

   private String extractedMessage(int extracted, String zipfile, File outputDir, boolean optComponents)
   {
      String extraNotice = "";
      if(optComponents)
          extraNotice = " Installation will now attempt to download and install open source extensions.";
      
      String installCompleteMsg = "Extracted " + extracted +
      " file" + ((extracted > 1) ? "s": "") +
      " from the\n";
      
      try{
          installCompleteMsg +=
            new File(zipfile).getCanonicalPath() + "\narchive into the\n" +
            outputDir.getCanonicalPath();
      }
      catch(IOException ex)
      {
          installCompleteMsg +=
            new File(zipfile).getAbsolutePath() + "\narchive into the\n" +
            outputDir.getAbsolutePath();
      }
      
      installCompleteMsg += "\ndirectory.\n\n";
      
      if(!silentInstall)
          installCompleteMsg +="Core TRAFCI files installed." + extraNotice;
      
      return installCompleteMsg;
   }
}
