Option Explicit

Dim shell, fileSystem, processEnv
Dim runtimeDir, pythonDir, vcpkgBin, cudaDir, exePath
Dim requiredFiles, item, missing

runtimeDir = "E:\sfm\runtime\Release"
pythonDir = "E:\sfm\envs\feedback310"
vcpkgBin = "E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\bin"
cudaDir = "D:\CUDA\v12.2"
exePath = runtimeDir & "\UI.exe"

Set shell = CreateObject("WScript.Shell")
Set fileSystem = CreateObject("Scripting.FileSystemObject")
Set processEnv = shell.Environment("PROCESS")

requiredFiles = Array( _
    exePath, _
    "E:\sfm\runtime\settings.ini", _
    runtimeDir & "\python310.dll", _
    runtimeDir & "\ffi-8.dll", _
    runtimeDir & "\tbb12.dll", _
    runtimeDir & "\platforms\qwindows.dll", _
    cudaDir & "\bin\cudart64_12.dll" _
)

missing = ""
For Each item In requiredFiles
    If Not fileSystem.FileExists(item) Then
        missing = missing & vbCrLf & item
    End If
Next

If missing <> "" Then
    MsgBox "Required runtime files are missing:" & missing, 16, "OntheflySfMFeedback"
    WScript.Quit 1
End If

If WScript.Arguments.Count > 0 Then
    If LCase(WScript.Arguments(0)) = "check" Then
        WScript.Echo "PASS: all required runtime files exist."
        WScript.Quit 0
    End If
End If

processEnv("PATH") = runtimeDir & ";" & _
    pythonDir & ";" & _
    pythonDir & "\DLLs;" & _
    pythonDir & "\Library\bin;" & _
    vcpkgBin & ";" & _
    cudaDir & "\bin;" & _
    processEnv("PATH")

processEnv("CUDA_PATH") = cudaDir
processEnv("QT_QPA_PLATFORM_PLUGIN_PATH") = runtimeDir & "\platforms"
processEnv("GLOG_logtostderr") = "1"

On Error Resume Next
processEnv.Remove "PYTHONHOME"
processEnv.Remove "PYTHONPATH"
processEnv.Remove "QT_PLUGIN_PATH"
On Error GoTo 0

shell.CurrentDirectory = runtimeDir
shell.Run Chr(34) & exePath & Chr(34), 1, False
