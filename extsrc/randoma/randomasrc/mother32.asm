; ----------------------------- MOTHER32.ASM -----------------------------
; Author:        Agner Fog
; Date created:  1998
; Last modified: 2008-12-02
; Project:       randoma library of random number generators
; Source URL:    www.agner.org/random
; Description:
; Mother-of-All random number generator by Agner Fog,
; 32-bit mode version for 80x86 and compatible microprocessors.
; See ran-instructions.pdf for detailed instructions
;
; This is a multiply-with-carry type of random number generator
; invented by George Marsaglia.  The algorithm is:             
; S = 2111111111*X[n-4] + 1492*X[n-3] + 1776*X[n-2] + 5115*X[n-1] + C
; X[n] = S modulo 2^32
; C = floor(S / 2^32) 
;
; C++ prototypes:
;
; Thread-safe versions:
; extern "C" void         MotRandomInit(void * Pthis, int ThisSize, int seed);// Initialization
; extern "C" int          MotIRandom(void * Pthis, int min, int max); // Get integer random number in desired interval
; extern "C" double       MotRandom(void * Pthis);                    // Get floating point random number
; extern "C" unsigned int MotBRandom(void * Pthis);                   // Output random bits
;
; Single-threaded static link versions
; extern "C" void         MotherRandomInit(int seed);      // Initialization
; extern "C" int          MotherIRandom(int min, int max); // Get integer random number in desired interval
; extern "C" double       MotherRandom();                  // Get floating point random number
; extern "C" unsigned int MotherBRandom();                 // Output random bits
;
; Single-threaded dynamic link versions
; extern "C" void         __stdcall MotherRandomInitD(int seed);      // Initialization
; extern "C" int          __stdcall MotherIRandomD(int min, int max); // Get integer random number in desired interval
; extern "C" double       __stdcall MotherRandomD();                  // Get floating point random number
; extern "C" unsigned int __stdcall MotherBRandomD();                 // Output random bits
;
; Copyright 1998-2008 by Agner Fog. 
; GNU General Public License http://www.gnu.org/licenses/gpl.html
; ----------------------------------------------------------------------

.686
.xmm
.model flat

extern _InstructionSet: near

; structure definition and constants:
INCLUDE randomah.asi

.DATA
align 16
; Instance of random number generator for single-threaded version
MotherInstance CRandomMotherA <>


.CODE    ; code segment


; extern "C" void MotRandomInit(void * Pthis, int ThisSize, int seed);  // Initialization
_MotRandomInit PROC NEAR

        mov     ecx, [esp+4]                ; Pthis
        ; ThisSize = buffer size. Check if big enough
        cmp     dword ptr [esp+8], (type CRandomMotherA)
        jb      Error                       ; Error exit if buffer too small
        push    edi

        ; Align by 16. Will overlap part of Fill if Pthis unaligned        
        and     ecx, -16        
        assume  ecx: ptr CRandomMotherA
        
        ; Clear my buffer, except first 16 possibly nonexisting filler bytes
        lea     edi, [ecx+16]
        push    ecx 
        mov     ecx, (type CRandomMotherA - 16) / 4
        xor     eax, eax
        cld
        rep     stosd
        pop     ecx

        ; insert constants
        mov     dword ptr [ecx].one + 4, 3FF00000H  ; high dword of 1.0       
        mov     [ecx].MF0, MOTHERF0             ; factors
        mov     [ecx].MF1, MOTHERF1             ; factors
        mov     [ecx].MF2, MOTHERF2             ; factors
        mov     [ecx].MF3, MOTHERF3             ; factors
        
        ; get instruction set
        push    ecx
        call    _InstructionSet
        pop     ecx
        mov     [ecx].Instset, eax
        
        ; initialize from seed
        mov     eax, [esp+16]               ; seed
        mov     edx, 29943829               ; factor
        ; make random numbers and put them into buffer
        imul    eax, edx
        dec     eax
        mov     [ecx].M0, eax
        imul    eax, edx
        dec     eax
        mov     [ecx].M1, eax
        imul    eax, edx
        dec     eax
        mov     [ecx].M2, eax
        imul    eax, edx
        dec     eax
        mov     [ecx].M3, eax
        imul    eax, edx
        dec     eax
        mov     [ecx].MC, eax

        ; randomize some more
        call    Mother_Generate
        mov     edi, 19                     ; loop counter
R90:    call    MotBRandom_reg
        dec     edi
        jnz     R90
        pop     edi
        ret     0                           ; ret 4 if not _cdecl calling
_MotRandomInit ENDP

Error   label   near                        ; Error exit
        xor     eax, eax
        div     eax                         ; Divide by 0
        ret

align 16

Mother_Generate proc near private
; Fill state array with new random numbers
; Don't change eax, st(0), xmm0

        assume  ecx: ptr CRandomMotherA

; check if SSE2 instruction set supported
        cmp     [ecx].Instset, 4
        jb      Mother_Generate_386

Mother_Generate_SSE2 label near                  ; alternative entry when SSE2 known to be supported
        
        movdqa  xmm3, xmmword ptr [ecx].M0       ; M3,M2,M1,M0
        movd    xmm4, dword   ptr [ecx].MC       ; MC
        mov     edx,  4                          ; loop counter

L1:     ; loop 4 times
        movdqa  xmm1, xmm3                       ; M3,M2,M1,M0
        pshufd  xmm2, xmm3, 00110001B            ; --,M3,--,M1
        pmuludq xmm1, xmmword ptr [ecx].MF0      ; M2*MF2, M0*MF0
        pmuludq xmm2, xmmword ptr [ecx].MF1      ; M3*MF3, M1*MF1
        paddq   xmm1, xmm2                       ; sum
        movhlps xmm2, xmm1                       ; Get high sum
        paddq   xmm4, xmm1                       ; add carry
        paddq   xmm4, xmm2                       ; add sums = MC,M0
        pslldq  xmm3, 4                          ; M3=M2,M2=M1,M1=M0
        movss   xmm3, xmm4                       ; insert new M0, rest unchanged
        psrlq   xmm4, 32                         ; shift down new MC
        dec     edx
        jnz     L1                               ; loop
        
        movdqa  xmmword ptr [ecx].M0, xmm3       ; M3,M2,M1,M0
        movd    dword   ptr [ecx].MC, xmm4       ; MC
        mov     dword   ptr [ecx].IM, 3          ; point index to M3
        cmp     [ecx].MF3, MOTHERF3              ; Check if initialized
        jne     Error                            ; Make error if not
        ret                                      ; return value in eax, st(0) or xmm0
        
Mother_Generate_386:
        push    eax                              ; eax may contain return value
        push    ebx
        push    esi
        push    edi
        mov     ebx,  4                          ; loop counter
        
L2:     ; loop 4 times
        mov     eax, [ecx].MF3
        mul     [ecx].M3                    ; x[n-4]
        mov     esi, eax
        mov     eax, [ecx].M2               ; x[n-3]
        mov     edi, edx
        mov     [ecx].M3, eax
        mul     [ecx].MF2
        add     esi, eax
        mov     eax, [ecx].M1               ; x[n-2]
        adc     edi, edx
        mov     [ecx].M2, eax
        mul     [ecx].MF1
        add     esi, eax
        mov     eax, [ecx].M0               ; x[n-1]
        adc     edi, edx
        mov     [ecx].M1, eax
        mul     [ecx].MF0
        add     eax, esi
        adc     edx, edi
        add     eax, [ecx].MC
        adc     edx, 0
        ; store next random number and carry
        mov     [ecx].M0, eax
        mov     [ecx].MC, edx
        dec     ebx
        jnz     L2                               ; loop

        mov     dword   ptr [ecx].IM, 3          ; point index to M3
        pop     edi
        pop     esi
        pop     ebx
        pop     eax
        cmp     [ecx].MF3, MOTHERF3              ; Check if initialized
        jne     Error                            ; Make error if not
        ret
Mother_Generate endp


_MotBRandom PROC NEAR
        mov     ecx, [esp+4]                ; Pthis
        and     ecx, -16                    ; Align by 16
        assume  ecx: ptr CRandomMotherA
MotBRandom_reg  label near                  ; Alternative entry for Pthis in ecx
        mov     edx, [ecx].IM               ; index into state buffer
        mov     eax, [ecx+edx*4].M0         ; read random number from state buffer
        dec     edx
        js      Mother_Generate             ; 4 values used, make next 4. eax has result
        mov     [ecx].IM, edx               ; store next index
        ret                                 ; return random number
_MotBRandom endp

        
; extern "C" double MotRandom(void * Pthis);  // Get floating point random number
_MotRandom PROC NEAR
        mov     ecx, [esp+4]                ; Pthis
        and     ecx, -16                    ; Align by 16
        assume  ecx: ptr CRandomMotherA
MotRandom_reg label near                    ; alternative entry for ecx = Pthis

; check if SSE2 instruction set supported
        cmp     [ecx].Instset, 4
        jb      MotRandom_386
        mov     edx,  [ecx].IM              ; index into state buffer
        movd    xmm0, [ecx+edx*4].M0        ; read random number from state buffer
        psllq   xmm0, 20                    ; put mantissa in place
        movsd   xmm1, [ecx].one             ; 1.0
        orpd    xmm0, xmm1                  ; insert exponent to get 1 <= x < 2
        subsd   xmm0, xmm1                  ; subtract one to get 0 <= x < 1
        movsd   [ecx].temp, xmm0            ; transfer to st(0) register
        fld     [ecx].temp
        dec     edx                         ; next index
        js      Mother_Generate_SSE2        ; 4 values used, make next 4. st(0) has result
        mov     [ecx].IM, edx               ; store next index
        ret

MotRandom_386:
        call    MotBRandom_reg              ; random bits
        ; convert to float
        mov     edx, eax
        shr     eax, 12
        or      eax, 3ff00000h
        shl     edx, 20
        mov     dword ptr [ecx].temp+4, eax
        mov     dword ptr [ecx].temp, edx
        fld     qword ptr [ecx].temp
        fsub    [ecx].one        
        ret
_MotRandom ENDP


; extern "C" int MotIRandom(void * Pthis, int min, int max); // Get integer random number in desired interval
_MotIRandom PROC NEAR                       ; make random integer in desired interval

        mov     ecx, [esp+4]                ; Pthis
        and     ecx, -16                    ; Align by 16
        call    MotBRandom_reg              ; make random number
        mov     edx, [esp+12]               ; max
        mov     ecx, [esp+8]                ; min
        sub     edx, ecx
        jl      short rerror                ; max < min
        inc     edx                         ; max - min + 1
        mul     edx                         ; multiply random number by interval and truncate
        lea     eax, [edx+ecx]              ; add min
        ret                                 ; ret 8 if not _cdecl calling

rerror: mov     eax, 80000000h              ; error exit   
        ret                                 ; ret 8 if not _cdecl calling
_MotIRandom ENDP


; ------------------------------------------------------------------
; Single-threaded static link versions
; ------------------------------------------------------------------

; extern "C" void MotherRandomInit(int seed); // Initialization
_MotherRandomInit PROC NEAR
        push    [esp+4]                ; seed
        push    type (CRandomMotherA)  ; Tell size of buffer
        LOADOFFSET2ECX MotherInstance  ; Get address of MotherInstance into ecx        
        push    ecx
        call    _MotRandomInit
        add     esp, 12
        ret
_MotherRandomInit ENDP


; extern "C" double MotherRandom(); // Get floating point random number
_MotherRandom PROC NEAR
        LOADOFFSET2ECX MotherInstance  ; Get address of MotherInstance into ecx        
        assume  ecx: ptr CRandomMotherA
        jmp     MotRandom_reg
_MotherRandom ENDP


; extern "C" int MotherIRandom(int min, int max); // Get integer random number in desired interval
_MotherIRandom PROC   NEAR             ; make random integer in desired interval
        LOADOFFSET2ECX MotherInstance  ; Get address of MotherInstance into ecx        
        call    MotBRandom_reg         ; make random number
        mov     edx, [esp+8]           ; max
        mov     ecx, [esp+4]           ; min
        sub     edx, ecx
        jl      RR100                  ; max < min
        inc     edx                    ; max - min + 1
        mul     edx                    ; multiply random number by interval and truncate
        lea     eax, [edx+ecx]         ; add min
        ret                            ; ret 8 if not _cdecl calling
        
RR100:  mov     eax, 80000000H         ; error exit   
        ret                            ; ret 8 if not _cdecl calling
_MotherIRandom ENDP


; extern "C" unsigned int MotherBRandom(); // Output random bits
_MotherBRandom PROC NEAR
        LOADOFFSET2ECX MotherInstance  ; Get address of MotherInstance into ecx        
        jmp     MotBRandom_reg
_MotherBRandom ENDP
       
       
IFDEF   WINDOWS
; ------------------------------------------------------------------
; Single-threaded dynamic link versions
; ------------------------------------------------------------------

; extern "C" void __stdcall MotherRandomInitD(int seed); // Initialization
_MotherRandomInitD@4 PROC NEAR
        push    [esp+4]                ; seed
        push    type (CRandomMotherA)  ; tell size of buffer
        push    offset MotherInstance  ; this pointer
        call    _MotRandomInit
        add     esp, 12
        ret     4
_MotherRandomInitD@4 ENDP


; extern "C" double __stdcall MotherRandomD(); // Get floating point random number
_MotherRandomD@0 PROC NEAR
        mov     ecx, offset MotherInstance
        assume  ecx: ptr CRandomMotherA
        jmp     MotRandom_reg
_MotherRandomD@0 ENDP


; extern "C" int __stdcall MotherIRandomD(int min, int max); // Get integer random number in desired interval
_MotherIRandomD@8 PROC NEAR            ; make random integer in desired interval
        mov     ecx, offset MotherInstance
        call    MotBRandom_reg         ; make random number
        mov     edx, [esp+8]           ; max
        mov     ecx, [esp+4]           ; min
        sub     edx, ecx
        jl      RR200                  ; max < min
        inc     edx                    ; max - min + 1
        mul     edx                    ; multiply random number by interval and truncate
        lea     eax, [edx+ecx]         ; add min
        ret     8
RR200:  mov     eax, 80000000h         ; error exit   
        ret     8
_MotherIRandomD@8 ENDP


; extern "C" unsigned int __stdcall MotherBRandomD(); // Output random bits
_MotherBRandomD@0 PROC NEAR
        mov     ecx, offset MotherInstance
        jmp     MotBRandom_reg
_MotherBRandomD@0 ENDP       
ENDIF ; WINDOWS


IFDEF   POSITIONINDEPENDENT
get_thunk_ecx: ; load caller address into ecx for position-independent code
        mov ecx, [esp]
        ret
ENDIF

END
