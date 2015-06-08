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
dim revisionVersion

Set TypeLib = CreateObject("Scriptlet.TypeLib")
strNewGuid=left(TypeLib.GUID,38)

strFileName = Wscript.Arguments(0)
revisionVersion = Wscript.Arguments(1)

Set objFSO = CreateObject("Scripting.FileSystemObject")

'---------------------------------------------------------------------------
'START (If not devBuild) increment the build version number.
'---------------------------------------------------
WScript.StdOut.WriteLine "Updating installer version..."

Set objFile = objFSO.OpenTextFile(strFileName, ForReading)

strText = objFile.ReadAll
objFile.Close

Set myRegExp = New RegExp
    myRegExp.Pattern =  """ProductVersion""\s=\s""8:((?:(?:\d*\.)|(\d*))*)"""
    myRegExp.IgnoreCase = True
    myRegExp.Global = True
    myRegExp.MultiLine = False

Set myMatches = myRegExp.Execute(strText)

If myMatches.count > 0 Then	

    'String to replace the old version string
	ReplacementString = "[assembly: AssemblyVersion(""" 

	'Store version number as an array of integers
	dim AssemblyVersionArray
	AssemblyVersionArray = Split(myMatches(0).SubMatches(0),".")

	minorVersion = AssemblyVersionArray(1)
	majorVersion = AssemblyVersionArray(0)

	newVersion = majorVersion & "." & minorVersion & "." & revisionVersion		
	
    ReplacementString = """ProductVersion"" = ""8:" 
    ReplacementString = ReplacementString & newVersion & """"

    strText = myRegExp.Replace(strText, ReplacementString)
End If

WScript.StdOut.WriteLine "Installer Version: " & newVersion

'Open the project file for writing
Set objFile = objFSO.OpenTextFile(strFileName, ForWriting)

'Write the new project file to disk.
objFile.Write strText

objFile.Close

'---------------------------------------------------
'END increment the build version number.
'---------------------------------------------------------------------------
