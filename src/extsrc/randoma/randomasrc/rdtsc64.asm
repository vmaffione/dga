;*************************  rdtsc64.asm  *********************************
; Author:           Agner Fog
; Date created:     
; Last modified:    2008-12-03
; Source URL:       www.agner.org/optimize
; Project:          asmlib.zip
; Description:
; This function returns the value of the time stamp counter, which counts 
; clock cycles. To count how many clock cycles a piece of code takes, call
; Rdtsc before and after the code to measure and calculate the difference.
;
; Copyright (c) 2003-2008 GNU General Public License www.gnu.org/licenses
;******************************************************************************

.code

; ********** ReadTSC function **********
; C++ prototype:
; extern "C" __int64 ReadTSC (void);


; The number of clock cycles taken by the ReadTSC function itself is approximately:
; Core 2:   730
; Pentium 4:  700
; Pentium II and Pentium III: 225
; AMD Athlon 64, Opteron: 126
; Does not work on 80386 and 80486.

; Note that clock counts may not be fully reproducible on Intel Core and
; Core 2 processors because the clock frequency can change. More reliable
; instruction timings are obtained with the performance monitor counter
; for "core clock cycles". This requires a kernel mode driver as the one
; included with www.agner.org/optimize/testp.zip.

ReadTSC PROC
        push    rbx                    ; ebx is modified by cpuid
        sub     eax, eax               ; 0
        cpuid                          ; serialize
        rdtsc                          ; read time stamp counter into edx:eax
        shl     rdx, 32
        or      rax, rdx               ; combine into 64 bit register        
        push    rax
        sub     eax, eax
        cpuid                          ; serialize
        pop     rax                    ; return value
        pop     rbx
        ret
ReadTSC ENDP

END
