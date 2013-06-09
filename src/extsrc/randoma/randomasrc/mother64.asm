; ----------------------------- MOTHER64.ASM -----------------------------
; Author:        Agner Fog
; Date created:  1998
; Last modified: 2008-12-03
; Project:       randoma library of random number generators
; Source URL:    www.agner.org/random
; Description:
; Mother-of-All random number generator by Agner Fog
; 64-bit mode version for x86-64 compatible microprocessors.
; See ran-instructions.pdf for detailed instructions
;
; This is a multiply-with-carry type of random number generator
; invented by George Marsaglia.  The algorithm is:             
; S = 2111111111*X[n-4] + 1492*X[n-3] + 1776*X[n-2] + 5115*X[n-1] + C
; X[n] = S modulo 2^32
; C = floor(S / 2^32) 
;
; C++ prototypes:
; extern "C" {
; // Single-threaded static link versions for Mother-of-all
; void   MotherRandomInit(int seed);                              // Re-seed
; int    MotherIRandom (int min, int max);                        // Output random integer
; double MotherRandom();                                          // Output random float
; uint32_t MotherBRandom();                                       // Output random bits
;
; // Single-threaded dynamic link versions for Mother-of-all
; void   __stdcall MotherRandomInitD(int seed);                 // Re-seed
; int    __stdcall MotherIRandomD (int min, int max);           // Output random integer
; double __stdcall MotherRandomD();                             // Output random float
; uint32_t __stdcall MotherBRandomD();                          // Output random bits
;
; // Thread-safe library functions for Mother-of-all
; // The thread-safe versions have as the first parameter a pointer to a 
; // private memory buffer. These functions are intended to be called from
; // the class CRandomMotherA defined below. 
; // If calling from C rather than C++ then supply a private memory buffer
; // as Pthis. The necessary size of the buffer is given in the class 
; // definition below. The buffer must be aligned by 16 because of the use 
; // of XMM registers.
; void   MotRandomInit(void * Pthis, int ThisSize, int seed);                   // Initialization
; int    MotIRandom(void * Pthis, int min, int max);              // Get integer random number in desired interval
; double MotRandom (void * Pthis);                                // Get floating point random number
; uint32_t MotBRandom(void * Pthis);                              // Output random bits
;
; Copyright 1998-2008 by Agner Fog. 
; GNU General Public License http://www.gnu.org/licenses/gpl.html
; ----------------------------------------------------------------------


; structure definition and constants:
INCLUDE randomah.asi

.DATA    ; data segment
align 16

; Data for single instance of random number generator
MotherInstance CRandomMotherA <>


.CODE    ; code segment

; Single threaded version:
; extern "C" unsigned int MotherRandomInit(int seed);  // Initialization

MotherRandomInit PROC
        mov     par3d, par1d                ; seed
        lea     par1, [MotherInstance]      ; Pthis = point to instance
        jmp     Register_MotRandomInit
MotherRandomInit ENDP
       

; Thread-safe version:
; extern "C" void MotRandomInit(void * Pthis, int ThisSize, int seed); // Initialization
MotRandomInit PROC
        ; Align buffer
        and     par1, -16
        
        ; check buffer size
        cmp     par2d, (type CRandomMotherA)
        jb      Error                       ; make error if buffer too small

Register_MotRandomInit LABEL NEAR           ; internal entry.
; par1 = aligned pointer to buffer
; par3d = seed

        ; clear my buffer, except 16 bytes filler
        push    rdi
        push    rcx
        lea     rdi, [par1+16]               ; Pthis
        mov     ecx, (type CRandomMotherA - 16) / 4
        xor     eax, eax
        cld
        rep     stosd
        pop     rcx
        pop     rdi
        
        ; insert constants
        mov     dword ptr [par1].CRandomMotherA.one + 4, 3FF00000H  ; high dword of 1.0       
        mov     [par1].CRandomMotherA.MF0, MOTHERF0         ; factors
        mov     [par1].CRandomMotherA.MF1, MOTHERF1
        mov     [par1].CRandomMotherA.MF2, MOTHERF2
        mov     [par1].CRandomMotherA.MF3, MOTHERF3
        
        ; initialize from seed
        mov     eax, par3d                                   ; seed        
        ; make random numbers and put them into buffer
        mov     par2d, 29943829
        imul    eax, par2d
        dec     eax
        mov     [par1].CRandomMotherA.M0, eax
        imul    eax, par2d
        dec     eax
        mov     [par1].CRandomMotherA.M1, eax
        imul    eax, par2d
        dec     eax
        mov     [par1].CRandomMotherA.M2, eax
        imul    eax, par2d
        dec     eax
        mov     [par1].CRandomMotherA.M3, eax
        imul    eax, par2d
        dec     eax
        mov     [par1].CRandomMotherA.MC, eax

        ; randomize some more
        call    Mother_Generate
        mov     r9d, 19                                    ; loop counter
R90:    call    Register_MotBRandom                        ; r9 unchanged
        dec     r9d
        jnz     R90
        ret
MotRandomInit ENDP

Error   label   near                   ; Error exit
        xor     eax, eax
        div     eax                    ; Make error by dividing by 0
        ud2
                
align 16

Mother_Generate proc private
; Fill state array with new random numbers
; Don't change eax, st(0), xmm0

        ;assume  par1: ptr CRandomMotherA

        movdqa  xmm3, xmmword ptr [par1].CRandomMotherA.M0       ; M3,M2,M1,M0
        movd    xmm4, dword   ptr [par1].CRandomMotherA.MC       ; MC
        mov     edx,  4                          ; loop counter

L1:     ; loop 4 times
        movdqa  xmm1, xmm3                       ; M3,M2,M1,M0
        pshufd  xmm2, xmm3, 00110001B            ; --,M3,--,M1
        pmuludq xmm1, xmmword ptr [par1].CRandomMotherA.MF0 ; M2*MF2, M0*MF0
        pmuludq xmm2, xmmword ptr [par1].CRandomMotherA.MF1 ; M3*MF3, M1*MF1
        paddq   xmm1, xmm2                       ; sum
        movhlps xmm2, xmm1                       ; Get high sum
        paddq   xmm4, xmm1                       ; add carry
        paddq   xmm4, xmm2                       ; add sums = MC,M0
        pslldq  xmm3, 4                          ; M3=M2,M2=M1,M1=M0
        movss   xmm3, xmm4                       ; insert new M0, rest unchanged
        psrlq   xmm4, 32                         ; shift down new MC
        dec     edx
        jnz     L1                               ; loop
        
        movdqa  xmmword ptr [par1].CRandomMotherA.M0, xmm3  ; M3,M2,M1,M0
        movd    dword   ptr [par1].CRandomMotherA.MC, xmm4  ; MC
        mov     dword   ptr [par1].CRandomMotherA.IM, 3     ; point index to M3
        cmp     [par1].CRandomMotherA.MF3, MOTHERF3         ; Check if initialized
        jne     Error                            ; Make error if not
        ret                                      ; return value in eax, st(0) or xmm0
Mother_Generate endp        
        

; Single threaded version:
; extern "C" unsigned int MotherBRandom(); // Output random bits

MotherBRandom PROC                          ; entry for both Windows and Linux call
        lea     par1, [MotherInstance]      ; Point to instance
        ;jmp     Register_MotBRandom
        ; continue in MotBRandom      

; Thread-safe version:
; extern "C" unsigned int MotBRandom(void * Pthis); // Output random bits

MotBRandom LABEL NEAR
public MotBRandom 
        and     par1, -16                   ; align buffer
        
Register_MotBRandom LABEL NEAR              ; internal entry. par1 = aligned pointer to CRandomMotherA
        mov     edx, [par1].CRandomMotherA.IM               ; index into state buffer
        mov     eax, [par1+rdx*4].CRandomMotherA.M0         ; read random number from state buffer
        dec     edx
        js      Mother_Generate             ; 4 values used, make next 4. eax has result
        mov     [par1].CRandomMotherA.IM, edx               ; store next index
        ret                                 ; return random number
MotherBRandom ENDP 

        
; Single threaded version:
; extern "C" unsigned int MotherRandom();  // Get floating point random number

MotherRandom PROC
        lea     par1, [MotherInstance]         ; Point to instance
        ; continue in MotRandom

; Thread-safe version:
; extern "C" double MotRandom(void * Pthis);  // Get floating point random number
MotRandom LABEL NEAR
PUBLIC MotRandom
        
        and     par1, -16                        ; align buffer
        mov     edx,  [par1].CRandomMotherA.IM              ; index into state buffer
        movd    xmm0, [par1+rdx*4].CRandomMotherA.M0        ; read random number from state buffer
        psllq   xmm0, 20                    ; put mantissa in place
        movsd   xmm1, [par1].CRandomMotherA.one             ; 1.0
        orpd    xmm0, xmm1                  ; insert exponent to get 1 <= x < 2
        subsd   xmm0, xmm1                  ; subtract one to get 0 <= x < 1
        dec     edx                         ; next index
        js      Mother_Generate             ; 4 values used, make next 4. st(0) has result
        mov     [par1].CRandomMotherA.IM, edx               ; store next index
        ret
MotherRandom ENDP


; Single threaded version:
; extern "C" unsigned int MotherIRandom(int min, int max); // Get integer random number in desired interval

MotherIRandom PROC
        mov     par3d, par2d                     ; max
        mov     par2d, par1d                     ; min
        lea     par1, [MotherInstance]           ; Pthis = point to instance
        ;jmp     Register_MotIRandom
        ; continue in MotIRandom

; Thread-safe version:
; extern "C" int MotIRandom(void * Pthis, int min, int max); // Get integer random number in desired interval
MotIRandom LABEL NEAR
PUBLIC MotIRandom
        and     par1, -16                   ; align buffer
        push    par2                        ; min
        push    par3                        ; max

        mov     edx, [par1].CRandomMotherA.IM               ; index into state buffer
        mov     eax, [par1+rdx*4].CRandomMotherA.M0         ; read random number from state buffer
        dec     edx
        mov     [par1].CRandomMotherA.IM, edx               ; store next index
        jns     L2
        call    Mother_Generate             ; 4 values used, make next 4. eax has random number
L2:        
        pop     rcx                         ; max
        pop     r8                          ; min
        sub     ecx, r8d
        jl      short rerror                ; max < min
        inc     ecx                         ; interval = max - min + 1
        mul     ecx                         ; multiply random number eax by interval and truncate
        lea     eax, [rdx+r8]               ; add min to interval*BRandom >> 32
        ret                                 ; ret 8 if not _cdecl calling

rerror: mov     eax, 80000000h              ; error exit   
        ret                                 ; ret 8 if not _cdecl calling
MotherIRandom ENDP       

        END
