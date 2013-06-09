; ----------------------------- RANDOMAD32.ASM ---------------------------
; Author:        Agner Fog
; Date created:  
; Last modified: 2008-11-04
; Project:       Any DLL from assembly source
; Source URL:    www.agner.org/random
; Description:
; Dummy DLL entry function
; Link with options /DLL /ENTRY:DllEntry
;
; Copyright 2008 by Agner Fog. 
; GNU General Public License http://www.gnu.org/licenses/gpl.html

.386
.model flat, stdcall

.code

DllEntry proc hInstance:DWORD, reason:DWORD, reserved1:DWORD
        mov     eax, 1
        ret
DllEntry endp

END
