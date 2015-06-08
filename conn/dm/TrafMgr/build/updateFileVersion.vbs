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
dim buildVersion
dim revisionVersion

Set TypeLib = CreateObject("Scriptlet.TypeLib")
strNewGuid=left(TypeLib.GUID,38)

strAssemblyInfoFile = Wscript.Arguments(0)
buildVersion = Wscript.Arguments(1)
revisionVersion = Wscript.Arguments(2)

Set objFSO = CreateObject("Scripting.FileSystemObject")

'---------------------------------------------------------------------------
'START Increment the File's AssemblyFileVersion
'---------------------------------------------------

Set objFile = objFSO.OpenTextFile(strAssemblyInfoFile, ForReading)

strText = objFile.ReadAll
objFile.Close

Set myRegExp = New RegExp
    myRegExp.Pattern =  "\[assembly:\sAssemblyFileVersion\(""((?:(?:\d*\.)|(\d*))*)""\)\]"
    myRegExp.IgnoreCase = True
    myRegExp.Global = True
    myRegExp.MultiLine = False

Set myMatches = myRegExp.Execute(strText)

If myMatches.count > 0 Then
	'String to replace the old version string
	ReplacementString = "[assembly: AssemblyFileVersion(""" 

	'Store version number as an array of integers
	dim AssemblyVersionArray
	AssemblyVersionArray = Split(myMatches(0).SubMatches(0),".")

	minorVersion = AssemblyVersionArray(1)
	majorVersion = AssemblyVersionArray(0)

	newVersion = majorVersion & "." & minorVersion & "." & buildVersion & "." & revisionVersion

	WScript.StdOut.WriteLine "New Assembly Version: " & newVersion
	
	ReplacementString = ReplacementString & newVersion & """)]"
	strText = myRegExp.Replace(strText, ReplacementString)
End If
'---------------------------------------------------
'END Increment the Application's AssemblyVersion
'---------------------------------------------------------------------------

'Open the project file for writing
Set objFile = objFSO.OpenTextFile(strAssemblyInfoFile, ForWriting)

'Write the new project file to disk.
objFile.Write strText
objFile.Close
