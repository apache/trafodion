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
Set objFSO = CreateObject("Scripting.FileSystemObject")

'---------------------------------------------------------------------------
'START Assign a freshly generated GUID to the application.
'---------------------------------------------------
Set objFile = objFSO.OpenTextFile(strFileName, ForReading)

Set regEx = New RegExp
regEx.Pattern = "<row><td>ProductCode</td><td>{\w{8}-\w{4}-\w{4}-\w{4}-\w{12}}</td><td/></row>"
regEx.IgnoreCase = True
regEx.Global = True
regEx.MultiLine = False

'search string by line
do
str=objFile.readline
If regEx.Test(str) Then
Exit Do
End if
loop until objFile.atendofstream
objFile.Close

'get guid from the line
regEx.Pattern = "{\w{8}-\w{4}-\w{4}-\w{4}-\w{12}}"
Set Matches = regEx.Execute(str)
For Each Match in Matches
   strOldGuid = Match.Value
Next

'---------------------------------------------------------------------------
'START Update the product code GUID in the TrafodionManager.ism
'---------------------------------------------------------------------------
Set objFile = objFSO.OpenTextFile(strFileName, ForReading)
strText = objFile.ReadAll
objFile.Close

NewString=Replace(strText,strOldGuid,strNewGuid)
NewString=Replace(NewString,"C:\TrafMgrBuild", Wscript.Arguments(1))
Set objFile = objFSO.OpenTextFile(strFileName, ForWriting)

'Write the new project file to disk.
objFile.WriteLine NewString

objFile.Close