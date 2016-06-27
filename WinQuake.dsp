# Microsoft Developer Studio Project File - Name="winquake" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=winquake - Win32 D3D Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WinQuake.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WinQuake.mak" CFG="winquake - Win32 D3D Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "winquake - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 GL Debug" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 GL Release" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 Debug Unicode" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 D3D Debug" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 D3D Release" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 DX8 Release" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 Pure WinQuake Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "WinQuake"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release_soft"
# PROP Intermediate_Dir ".\release_soft"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /GX /O2 /I ".\scitech\include" /I ".\dxsdk\sdk7\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 .\scitech\lib\win32\vc\mgllt.lib advapi32.lib comdlg32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib libcurl.lib ole32.lib oleaut32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"c:\quake1\wqpro466.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /ML /GX /ZI /Od /I ".\scitech\include" /I ".\dxsdk\sdk7\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 .\scitech\lib\win32\vc\mgllt.lib opengl32.lib glu32.lib .\dxsdk\sdk7\lib\dxguid.lib winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\winquake"
# PROP BASE Intermediate_Dir ".\winquake"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\debug_gl"
# PROP Intermediate_Dir ".\debug_gl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /GX /Zi /Od /I ".\scitech\include" /I ".\dxsdk\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /ML /GX /ZI /Od /I ".\dxsdk\sdk7\include" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /FR /YX /FD /QIfist /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib .\scitech\lib\win32\vc\mgllt.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 comctl32.lib opengl32.lib glu32.lib .\dxsdk\sdk7\lib\dxguid.lib winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\debug_gl\glquake.exe"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\winquak0"
# PROP BASE Intermediate_Dir ".\winquak0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release_gl"
# PROP Intermediate_Dir ".\release_gl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /GX /Ox /Ot /Ow /I ".\scitech\include" /I ".\dxsdk\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# SUBTRACT BASE CPP /Oa /Og
# ADD CPP /nologo /G5 /W3 /GX /O2 /I ".\dxsdk\sdk7\include" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib .\scitech\lib\win32\vc\mgllt.lib /nologo /subsystem:windows /profile /machine:I386
# SUBTRACT BASE LINK32 /map /debug
# ADD LINK32 advapi32.lib comctl32.lib dsound.lib dxguid.lib gdi32.lib libcurl.lib kernel32.lib ole32.lib oleaut32.lib opengl32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"c:\quake1\glpro466.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "winquake___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "winquake___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "winquake___Win32_Debug_Unicode"
# PROP Intermediate_Dir "winquake___Win32_Debug_Unicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /GX /ZI /Od /I ".\scitech\include" /I ".\dxsdk\sdk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /FR /YX /FD /c
# ADD CPP /nologo /G5 /ML /Gm /Gi /GX /ZI /Od /I ".\scitech\include" /I ".\dxsdk\sdk7\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /D "UNICODE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 .\dxsdk\sdk\lib\dxguid.lib .\scitech\lib\win32\vc\mgllt.lib winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 .\scitech\lib\win32\vc\mgllt.lib opengl32.lib glu32.lib .\dxsdk\sdk7\lib\dxguid.lib winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "winquake___Win32_D3D_Debug"
# PROP BASE Intermediate_Dir "winquake___Win32_D3D_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\debug_d3d"
# PROP Intermediate_Dir ".\debug_d3d"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /GX /ZI /Od /I ".\dxsdk\sdk\inc" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /FR /YX /FD /c
# ADD CPP /nologo /G5 /ML /Gm /Gi /GX /ZI /Od /I ".\dxsdk\sdk7\include" /I ".\D3DFrame" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /D "D3DQUAKE" /FR /YX /FD /QIfist /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 .\dxsdk\sdk\lib\dxguid.lib comctl32.lib winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\debug_gl\glquake.exe"
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 comctl32.lib .\dxsdk\sdk7\lib\dxguid.lib .\dxsdk\sdk7\lib\d3dxd.lib .\dxsdk\sdk7\lib\ddraw.lib winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\debug_d3d\D3DQuake.exe"
# SUBTRACT LINK32 /profile /nodefaultlib

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "winquake___Win32_D3D_Release"
# PROP BASE Intermediate_Dir "winquake___Win32_D3D_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release_d3d"
# PROP Intermediate_Dir ".\release_d3d"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /GX /Ot /Ow /I ".\dxsdk\sdk7\include" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /FR /YX /FD /c
# ADD CPP /nologo /G5 /GX /O2 /I ".\dxsdk\sdk7\include" /I ".\D3DFrame" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "D3DQUAKE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib opengl32.lib glu32.lib .\dxsdk\sdk7\lib\dxguid.lib winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /machine:I386 /out:".\release_gl\glquake.exe"
# SUBTRACT BASE LINK32 /map /debug
# ADD LINK32 d3dxd.lib ddraw.lib dxguid.lib advapi32.lib comctl32.lib gdi32.lib kernel32.lib libcurl.lib ole32.lib oleaut32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"c:\quake1\d3dpro466.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "winquake___Win32_DX8_Release"
# PROP BASE Intermediate_Dir "winquake___Win32_DX8_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release_dx8"
# PROP Intermediate_Dir ".\release_dx8"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /GX /O2 /I ".\dxsdk\sdk7\include" /I ".\D3DFrame" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "NO_ASSEMBLY" /D "NO_MGRAPH" /D "D3DQUAKE" /FR /YX /FD /c
# ADD CPP /nologo /G5 /GX /O2 /I ".\dxsdk\sdk7\include" /I ".\D3DFrame" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "DX8QUAKE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 d3dxd.lib ddraw.lib dxguid.lib advapi32.lib comctl32.lib gdi32.lib kernel32.lib libcurl.lib ole32.lib oleaut32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /incremental:yes /machine:I386 /out:"e:\quakedm3\d3dpro421.exe"
# SUBTRACT BASE LINK32 /profile /map /debug
# ADD LINK32 advapi32.lib comctl32.lib dxguid.lib gdi32.lib kernel32.lib libcurl.lib ole32.lib oleaut32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"c:\quake1\dx8pro466.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "winquake___Win32_Pure_WinQuake_Release"
# PROP BASE Intermediate_Dir "winquake___Win32_Pure_WinQuake_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release_soft_api"
# PROP Intermediate_Dir ".\release_soft_api"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /GX /O2 /I ".\scitech\include" /I ".\dxsdk\sdk7\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /G5 /GX /O2 /I ".\scitech\include" /I ".\dxsdk\sdk7\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 .\scitech\lib\win32\vc\mgllt.lib advapi32.lib comdlg32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib libcurl.lib ole32.lib oleaut32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"e:\quakedm3\wqpro451.exe"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 .\scitech\lib\win32\vc\mgllt.lib advapi32.lib comdlg32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib libcurl.lib ole32.lib oleaut32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"c:\quake1\wqpro_api_466.exe"
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "winquake - Win32 Release"
# Name "winquake - Win32 Debug"
# Name "winquake - Win32 GL Debug"
# Name "winquake - Win32 GL Release"
# Name "winquake - Win32 Debug Unicode"
# Name "winquake - Win32 D3D Debug"
# Name "winquake - Win32 D3D Release"
# Name "winquake - Win32 DX8 Release"
# Name "winquake - Win32 Pure WinQuake Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\cd_win.c
# End Source File
# Begin Source File

SOURCE=.\cl_chase.c
# End Source File
# Begin Source File

SOURCE=.\cl_demo.c
# End Source File
# Begin Source File

SOURCE=.\cl_input.c
# End Source File
# Begin Source File

SOURCE=.\cl_main.c
# End Source File
# Begin Source File

SOURCE=.\cl_parse.c
# End Source File
# Begin Source File

SOURCE=.\cl_sbar.c
# End Source File
# Begin Source File

SOURCE=.\cl_tent.c
# End Source File
# Begin Source File

SOURCE=.\cl_view.c
# End Source File
# Begin Source File

SOURCE=.\cmd.c
# End Source File
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\conproc.c
# End Source File
# Begin Source File

SOURCE=.\console.c
# End Source File
# Begin Source File

SOURCE=.\crc.c
# End Source File
# Begin Source File

SOURCE=.\cvar.c
# End Source File
# Begin Source File

SOURCE=.\d_draw.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\release\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\release\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_draw16.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_edge.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_fill.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_init.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_modech.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_part.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_parta.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_polysa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_polyse.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_scan.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_scana.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_sky.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_spr8.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_sprite.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_surf.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_vars.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_varsa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_zpoint.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dx8_gl_fakegl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dx8_libci.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_draw.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_fakegl.cpp

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_fullbright.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_mesh.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_model.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_refrag.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rlight.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rmain.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rmisc.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rsurf.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_screen.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_test.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_warp.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\host.c
# End Source File
# Begin Source File

SOURCE=.\host_cmd.c
# End Source File
# Begin Source File

SOURCE=.\in_win.c
# End Source File
# Begin Source File

SOURCE=.\iplog.c
# End Source File
# Begin Source File

SOURCE=.\keys.c
# End Source File
# Begin Source File

SOURCE=.\location.c
# End Source File
# Begin Source File

SOURCE=.\math.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mathlib.c
# End Source File
# Begin Source File

SOURCE=.\matrix.c
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\movie.c
# End Source File
# Begin Source File

SOURCE=.\movie_avi.c
# End Source File
# Begin Source File

SOURCE=.\net_dgrm.c
# End Source File
# Begin Source File

SOURCE=.\net_loop.c
# End Source File
# Begin Source File

SOURCE=.\net_main.c
# End Source File
# Begin Source File

SOURCE=.\net_vcr.c
# End Source File
# Begin Source File

SOURCE=.\net_win.c
# End Source File
# Begin Source File

SOURCE=.\net_wins.c
# End Source File
# Begin Source File

SOURCE=.\net_wipx.c
# End Source File
# Begin Source File

SOURCE=.\nonintel.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pr_cmds.c
# End Source File
# Begin Source File

SOURCE=.\pr_edict.c
# End Source File
# Begin Source File

SOURCE=.\pr_exec.c
# End Source File
# Begin Source File

SOURCE=.\qmp3.cpp
# End Source File
# Begin Source File

SOURCE=.\r_aclip.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_aclipa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_alias.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_aliasa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_bsp.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_draw.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_drawa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_edge.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_edgea.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_efrag.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_light.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_main.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_misc.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_model.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_part.c
# End Source File
# Begin Source File

SOURCE=.\r_rast.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_screen.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_sky.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_sprite.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_surf.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_vars.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_varsa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\security.c
# End Source File
# Begin Source File

SOURCE=.\snd_dma.c
# End Source File
# Begin Source File

SOURCE=.\snd_mem.c
# End Source File
# Begin Source File

SOURCE=.\snd_mix.c
# End Source File
# Begin Source File

SOURCE=.\snd_mixa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\snd_win.c
# End Source File
# Begin Source File

SOURCE=.\surf16.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\surf8.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\sv_move.c
# End Source File
# Begin Source File

SOURCE=.\sv_phys.c
# End Source File
# Begin Source File

SOURCE=.\sv_user.c
# End Source File
# Begin Source File

SOURCE=.\sv_world.c
# End Source File
# Begin Source File

SOURCE=.\sys_win.c
# End Source File
# Begin Source File

SOURCE=.\sys_wina.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft
InputPath=.\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# Begin Custom Build - mycoolbuild
OutDir=.\release_soft_api
InputPath=.\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# Begin Source File

SOURCE=.\vid_common_gl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_wgl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_win.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_win_pure.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_wince.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /WX

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /WX
# SUBTRACT CPP /WX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wad.c
# End Source File
# Begin Source File

SOURCE=.\webdownload.c
# End Source File
# Begin Source File

SOURCE=.\winquake.rc
# End Source File
# Begin Source File

SOURCE=.\zone.c
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icon3.ico
# End Source File
# Begin Source File

SOURCE=.\qe3.ico
# End Source File
# Begin Source File

SOURCE=.\quake.ico
# End Source File
# End Group
# Begin Group "D3DFrame"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\D3DFrame\d3denum.cpp

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\D3DFrame\d3denum.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\D3DFrame\d3dframe.cpp

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\D3DFrame\d3dframe.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\D3DFrame\d3dres.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\D3DFrame\d3dutil.cpp

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\D3DFrame\d3dutil.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 D3D Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Pure WinQuake Release"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=C:\cygwin\home\user\proquake_dev\WinQuake\psp\Cygwin_normal_make.bat
# End Source File
# Begin Source File

SOURCE=.\progdefs.q1
# End Source File
# Begin Source File

SOURCE=.\progdefs.q2
# End Source File
# Begin Source File

SOURCE="..\proquake-changes.txt"
# End Source File
# Begin Source File

SOURCE="..\proquake-credits.txt"
# End Source File
# Begin Source File

SOURCE=..\todo.txt
# End Source File
# End Target
# End Project
