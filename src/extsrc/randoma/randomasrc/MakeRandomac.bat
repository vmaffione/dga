rem           MakeAsmlib.bat                   2008-11-04 Agner Fog

rem  Make function library randoma and class library randomc
rem  randoma is made from assembly source with multiple
rem  versions for different operating systems


rem  Set path to assembler and objconv:
rem  You need to modify this path to fit your installation
set path=C:\Program Files\Microsoft Visual Studio 9.0\VC\bin;C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE;C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\x86_amd64;E:\Program Files\Microsoft SDKs\Windows\v6.1\Bin\x64;%path%

rem reuse functions instrset and rdtsc from asmlib:
copy C:\1_Public\asmlib\instrset??.asm .
copy C:\1_Public\asmlib\rdtsc??.asm .

rem  Make everything according to makefile asmlib.make
nmake /Frandomac.make

pause