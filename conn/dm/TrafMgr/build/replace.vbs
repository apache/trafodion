'    @@@ START COPYRIGHT @@@                                                 
'                                                                            
'    (C) Copyright 2015 Hewlett-Packard Development Company, L.P.            
'                                                                            
'    Licensed under the Apache License, Version 2.0 (the "License");         
'    you may not use this file except in compliance with the License.        
'    You may obtain a copy of the License at                                 
'                                                                            
'         http://www.apache.org/licenses/LICENSE-2.0                         
'                                                                            
'    Unless required by applicable law or agreed to in writing, software     
'    distributed under the License is distributed on an "AS IS" BASIS,       
'    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
'    See the License for the specific language governing permissions and     
'    limitations under the License.                                          
'                                                                            
'    @@@ END COPYRIGHT @@@                                                   
Const ForReading = 1
Const ForWriting = 2

Set TypeLib = CreateObject("Scriptlet.TypeLib")
strNewGuid=left(TypeLib.GUID,38)

strFileName = Wscript.Arguments(0)
strUninstallTemplateFile = Wscript.Arguments(1)
strUninstallScriptFile = Wscript.Arguments(2)

Set objFSO = CreateObject("Scripting.FileSystemObject")


'---------------------------------------------------------------------------
'START Assign a freshly generated GUID to the application.
'---------------------------------------------------------------------------

strNewText = """ProductCode"" = ""8:" + strNewGuid + """"

Set objFile = objFSO.OpenTextFile(strFileName, ForReading)

strText = objFile.ReadAll
objFile.Close

Set regEx = New RegExp
regEx.Pattern = """ProductCode""\s=\s""8:\{\w{8}-\w{4}-\w{4}-\w{4}-\w{12}\}"""
regEx.IgnoreCase = True
regEx.Global = True
regEx.MultiLine = False
StringToExtract = regEx.Replace(strText, strNewText)

'Open the project file for writing
Set objFile = objFSO.OpenTextFile(strFileName, ForWriting)

'Write the new project file to disk.
objFile.WriteLine StringToExtract

objFile.Close


'---------------------------------------------------------------------------
'END Assign a freshly generated GUID to the application.
'---------------------------------------------------


'---------------------------------------------------------------------------
'START Update the GUID reference in the uninstaller.
'---------------------------------------------------

Set objFile = objFSO.OpenTextFile(strUninstallTemplateFile, ForReading)
strText = objFile.ReadAll
objFile.Close

Set regEx = New RegExp
    regEx.Pattern = "\{INSERT_NEW_GUID\}"
    regEx.IgnoreCase = True
    regEx.Global = False
    regEx.MultiLine = False
NewString = regEx.Replace(strText, strNewGuid)


'Open the Uninstall File
Set objFile = objFSO.OpenTextFile(strUninstallScriptFile, ForWriting)

'Write the new project file to disk.
objFile.WriteLine NewString

objFile.Close


