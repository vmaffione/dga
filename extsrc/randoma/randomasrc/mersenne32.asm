; ----------------------------- MERSENNE32.ASM ---------------------------
; Author:        Agner Fog
; Date created:  1998
; Last modified: 2008-11-04
; Project:       randoma library of random number generators
; Source URL:    www.agner.org/random
; Description:
; Random Number generator 'Mersenne Twister' type MT11213A or MT19937
; 32-bit mode version for x86 compatible microprocessors.
; See ran-instructions.pdf for detailed instructions
;
;  This random number generator is described in the article by
;  M. Matsumoto & T. Nishimura, in:
;  ACM Transactions on Modeling and Computer Simulation,
;  vol. 8, no. 1, 1998, pp. 3-30. See also:
;  http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
;
;  Initialization:
;  MersRandomInit must be called before the first call to any of the other
;  random number functions. The seed is any 32-bit integer.
;  You may use MersRandomInitByArray instead if you want more
;  than 32 bits for seed. NumSeeds is the number of integers in seeds[].
;  NumSeeds must be > 0, there is no upper limit for NumSeeds.
;
;  Generating random numbers:
;  MersRandom returns a floating point number in the interval 0 <= x < 1 with
;  a resolution of 32 bits.
;  MersIRandom returns an integer in the interval defined by min and max with
;  a resolution of 32 bits.
;  MersIRandomX returns an integer in the interval defined by min and max with
;  exactly equal probabilities of all values in the interval.
;  MersBRandom returns 32 random bits.
;
;  Error conditions:
;  If MersRandomInit or MersRandomInitByArray has not been called then MersRandom
;  and MersBRandom keep returning 0, and MersIRandom and MersIRandomX return min.
;  MersIRandom and MersIRandomX return a large negative number if max < min.
;
;  C++ prototypes in randoma.h, 32-bit Windows:
;
;  Thread-safe static link versions for Mersenne Twister
;  extern "C" void   MersRandomInit(void * Pthis, int ThisSize, int seed);// Re-seed
;  extern "C" void   MersRandomInitByArray(void * Pthis, int ThisSize, int const seeds[], int NumSeeds); // Seed by more than 32 bits
;  extern "C" int    MersIRandom (void * Pthis, int min, int max);        // Output random integer
;  extern "C" int    MersIRandomX(void * Pthis, int min, int max);        // Output random integer, exact
;  extern "C" double MersRandom(void * Pthis);                            // Output random float
;  extern "C" unsigned int MersBRandom(void * Pthis);                     // Output random bits
;
;  Single-threaded static link versions for Mersenne Twister, Windows only
;  extern "C" void   MersenneRandomInit(int seed);              // Re-seed
;  extern "C" void   MersenneRandomInitByArray(int const seeds[], int NumSeeds); // Seed by more than 32 bits
;  extern "C" int    MersenneIRandom (int min, int max);        // Output random integer
;  extern "C" int    MersenneIRandomX(int min, int max);        // Output random integer, exact
;  extern "C" double MersenneRandom();                          // Output random float
;  extern "C" unsigned int MersenneBRandom();                   // Output random bits
;
;  Single threaded dynamic link versions for Mersenne Twister, Windows only
;  extern "C" void   __stdcall MersenneRandomInitD(int seed);              // Re-seed
;  extern "C" void   __stdcall MersenneRandomInitByArrayD(int const seeds[], int NumSeeds); // Seed by more than 32 bits
;  extern "C" int    __stdcall MersenneIRandomD (int min, int max);        // Output random integer
;  extern "C" int    __stdcall MersenneIRandomXD(int min, int max);        // Output random integer, exact
;  extern "C" double __stdcall MersenneRandomD();                          // Output random float
;  extern "C" unsigned int __stdcall MersenneBRandomD();                   // Output random bits
;
; Copyright 1998-2008 by Agner Fog. 
; GNU General Public License http://www.gnu.org/licenses/gpl.html
; ----------------------------------------------------------------------
.686
.xmm
.model flat

; structure definition and constants:
INCLUDE randomah.asi

.DATA
align 16
; Data for single instance of random number generator
MersenneInstance CRandomMersenneA <>


.CODE

extern _InstructionSet: near


; ---------------------------------------------------------------
;  Thread-safe static link versions for Mersenne Twister
; ---------------------------------------------------------------

;  extern "C" void MersRandomInit(void * Pthis, int ThisSize, int seed); // Re-seed

_MersRandomInit PROC NEAR
        mov     ecx, [esp+4]           ; Pthis
        cmp     dword ptr [esp+8], (type CRandomMersenneA)
        jb      Error                  ; Error exit if buffer too small

        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16
assume  ecx: ptr CRandomMersenneA
        mov     eax, [esp+12]           ; seed
        
MersRandomInit_reg  LABEL NEAR         ; Entry for register parameters, used internally

        call    Mers_init0             ; initialize mt buffer with seeds
        
        ; Number of premade numbers that are lost in the initialization when the  
        ; SSE2 implementation makes up to 4 premade numbers at a time:
IF MERS_N and 3        
   PREMADELOST = (MERS_N and 3)
ELSE
   PREMADELOST = 4
ENDIF
        ; We want the C++ and the assembly implementation to give exactly the same
        ; sequence. The C++ version discards 37 random numbers after initialization.
        ; The assembly version generates a sequence that is PREMADELOST + 1 numbers
        ; behind. Therefore we discard the first 37 + PREMADELOST + 1 numbers if
        ; SSE2 is supported, otherwise 37 + 1.
        
        push    edi
        mov     edi, 37+PREMADELOST+1
        CMP     [ecx].Instset, 4       ; can we use XMM registers and SSE2 ?
        jae     M110
        sub     edi, PREMADELOST       ; SSE2 not supported
        mov     [ecx].PreInx, 0        ; reset index to premade list
M110:   ; loop
M120:   call    MersBRandom_reg
        dec     edi
        jnz     M120
        pop     edi
        ret
_MersRandomInit ENDP
        
Error   label   near                        ; Error exit
        xor     eax, eax
        div     eax                         ; Divide by 0
        ret

Mers_init0   PROC    NEAR    ; make random seeds from eax and put them into MT buffer
; Input parameters: 
; eax: seed
; ecx points to CRandomMersenneA
assume  ecx: ptr CRandomMersenneA

        push    ebx
        push    edi
        mov     ebx, eax               ; seed
        
        ; clear my buffer, except first 16 possibly nonexisting filler bytes
        push    ecx
        lea     edi, [ecx+16]          ; Pthis
        mov     ecx, (type CRandomMersenneA - 16) / 4
        xor     eax, eax
        cld
        rep     stosd
        pop     ecx                    ; Pthis
        
        ; initialize CRandomMersenneA structure
        mov     [ecx].PreInx, 4*4
        push    ecx
        call    _InstructionSet        ; detect instruction set
        pop     ecx
        mov     [ecx].Instset, eax
        mov     eax, MERS_B
        mov     [ecx].TMB, eax
        mov     [ecx].TMB+4, eax
        mov     [ecx].TMB+8, eax
        mov     [ecx].TMB+12, eax
        mov     eax, MERS_C
        mov     [ecx].TMC, eax
        mov     [ecx].TMC+4, eax
        mov     [ecx].TMC+8, eax
        mov     [ecx].TMC+12, eax
        mov     eax, 3FF00000H         ; upper dword of 1.0, double precision
        mov     dword ptr [ecx].one+4, eax
        mov     dword ptr [ecx].one+12, eax        
        mov     [ecx].LMASK, LOWER_MASK
        mov     [ecx].UMASK, UPPER_MASK
        mov     [ecx].MATA,  MERS_A

        ; put random numbers into MT buffer
        xor     edi, edi        
M210:   mov     [ecx+edi*4].MT, ebx
        mov     edx, ebx
        shr     ebx, 30
        xor     ebx, edx
        imul    ebx, 1812433253D
        inc     edi
        add     ebx, edi        
        cmp     edi, MERS_N
        jb      M210
        
        ; Set index MTI to end of list, (scaled by 4)
        ; Round up to multiple of 4 to avoid alignment error
        mov     [ecx].MTI, ((MERS_N+3) and -4) * 4
        
        pop     edi
        pop     ebx       
        ret      
Mers_init0   ENDP


;  extern "C" void   MersRandomInitByArray(void * Pthis, int ThisSize, int const seeds[], int NumSeeds); // Seed by more than 32 bits
_MersRandomInitByArray PROC NEAR

        cmp     dword ptr [esp+8], (type CRandomMersenneA)
        jb      Error                  ; Error exit if ThisSize too small
        push    ebx
        push    esi
        push    edi
        push    ebp
        mov     ecx, [esp+20]          ; Pthis
        mov     ebx, [esp+28]          ; seeds
        mov     ebp, [esp+32]          ; NumSeeds

        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomMersenneA
        
MersRandomInitByArray_reg  LABEL NEAR  ; Entry for register parameters, used internally
;public MersRandomInitByArray_reg

        push    ebp                    ; save NumSeeds
        mov     eax, 19650218
        call    Mers_init0             ; init0(19650218);
        
        test    ebp, ebp
        jle     M380                   ; error: NumSeeds <= 0
        xor     edi, edi               ; j = 0
        lea     esi, [edi+1]           ; i = 1
        cmp     ebp, MERS_N
        ja      M310
        mov     ebp, MERS_N            ; k = max (MERS_N,NumSeeds)
M310:

        ; for (; k; k--) {
M320:   mov     eax, [ecx+esi*4-4].MT  ; mt[i-1]
        mov     edx, eax
        shr     eax, 30
        xor     eax, edx               ; mt[i-1] ^ (mt[i-1] >> 30)
        imul    eax, 1664525           ; * 1664525
        xor     eax, [ecx+esi*4].MT       ; ^ mt[i]
        add     eax, [ebx+edi*4]       ; + seeds[j]
        add     eax, edi               ; + j
        mov     [ecx+esi*4].MT, eax       ; save in mt[i]
        inc     esi                    ; i++
        inc     edi                    ; j++
        cmp     esi, MERS_N
        jb      M330                   ; if (i>=MERS_N)
        mov     eax, [ecx+(MERS_N-1)*4].MT     ; mt[0] = mt[MERS_N-1];
        mov     [ecx].MT, eax
        mov     esi, 1                 ; i=1;
M330:
        cmp     edi, [esp]             ; NumSeeds
        jb      M340          ; if (j>=NumSeeds)
        xor     edi, edi               ; j = 0;
M340:
        dec     ebp                    ; k--
        jnz     M320                   ; first k loop
M350:
        mov     ebp, MERS_N-1          ; k
M360:   mov     eax, [ecx+esi*4-4].MT  ; mt[i-1]
        mov     edx, eax
        shr     eax, 30
        xor     eax, edx               ; mt[i-1] ^ (mt[i-1] >> 30)
        imul    eax, 1566083941        ; * 1566083941
        xor     eax, [ecx+esi*4].MT    ; ^ mt[i]
        sub     eax, esi               ; - i
        mov     [ecx+esi*4].MT, eax    ; save in mt[i]
        inc     esi                    ; i++
        cmp     esi, MERS_N
        jb      M370                   ; if (i>=MERS_N)
        mov     eax, [ecx+(MERS_N-1)*4].MT  ; mt[0] = mt[MERS_N-1];
        mov     [ecx].MT, eax
        mov     esi, 1                 ; i=1;
M370:
        dec     ebp                    ; k--
        jnz     M360                   ; second k loop
        mov     [ecx].MT, 80000000H    ; mt[0] = 0x80000000
M380:
        mov     [ecx].MTI, 0
        mov     [ecx].PreInx, 0

; discard first MERS_N random numbers + PREMADELOST+1 to compensate for lag
        mov     edi, MERS_N + PREMADELOST+1
        CMP     [ecx].Instset, 4       ; can we use XMM registers and SSE2 ?
        jae     M390
        sub     edi, PREMADELOST       ; SSE2 not supported
        mov     [ecx].PreInx, 0        ; reset index to premade list
M390:   ; loop
M391:   call    MersBRandom_reg
        dec     edi
        jnz     M391

        pop     ecx                    ; remove local copy of NumSeeds
        pop     ebp                    ; restore registers
        pop     edi
        pop     esi
        pop     ebx
        ret
_MersRandomInitByArray ENDP


;  extern "C" unsigned int MersBRandom(void * Pthis);                     // Output random bits

_MersBRandom PROC NEAR                 ; generate random bits
        mov     ecx, [esp+4]           ; Pthis
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomMersenneA

MersBRandom_reg  LABEL NEAR            ; Entry for register parameters, used internally
;public MersBRandom_reg

        cmp     [ecx].Instset, 4       ; can we use XMM registers and SSE2 ?
        jb      M500

        ; this version uses XMM registers and SSE2 instructions:
        mov     edx, [ecx].PreInx      ; index into premade numbers
        mov     eax, [ecx+edx*1].PreInt; fetch premade random number
        add     edx, 4
        mov     [ecx].PreInx, edx
        cmp     edx, 4*4
        jnb     M410
        ret                            ; return premade number

M410    LABEL   NEAR
; PREMADE list is empty. Make 4 more numbers ready for next call:
        mov     edx, [ecx].MTI         ; fetch 4 numbers from MT buffer
        movdqa  xmm0, xmmword ptr [ecx+edx*1].MT
        
; optional tempering algorithm
        movdqa  xmm1, xmm0
        psrld   xmm0, MERS_U
        pxor    xmm0, xmm1
        movdqa  xmm1, xmm0        
        pslld   xmm0, MERS_S
        pand    xmm0, xmmword ptr [ecx].TMB
        pxor    xmm0, xmm1
        movdqa  xmm1, xmm0        
        pslld   xmm0, MERS_T
        pand    xmm0, xmmword ptr [ecx].TMC
        pxor    xmm0, xmm1
        movdqa  xmm1, xmm0        
        psrld   xmm0, MERS_L
        pxor    xmm0, xmm1
; end of tempering

        ; Check if initialized
        cmp     [ecx].MATA, MERS_A
        jne     Error                  ; Make error if not initialized

        ; save four premade integers
        movdqa  xmmword ptr [ecx].PreInt, xmm0
        ; premake four floating point numbers
        pxor    xmm1, xmm1
        pxor    xmm2, xmm2
        punpckldq xmm1, xmm0           ; get first two numbers into bits 32-63 and 96-127
        punpckhdq xmm2, xmm0           ; get next  two numbers into bits 32-63 and 96-127
        psrlq   xmm1, 12               ; get bits into mantissa position
        psrlq   xmm2, 12               ; get bits into mantissa position
        por     xmm1, xmmword ptr [ecx].one ; set exponent for interval [1,2)
        por     xmm2, xmmword ptr [ecx].one ; set exponent for interval [1,2)
        movdqa  xmmword ptr [ecx].PreFlt, xmm1     ; store two premade numbers
        movdqa  xmmword ptr [ecx].PreFlt+16, xmm2  ; store two more premade numbers        
        mov     [ecx].PreInx, 0        ; index to premade numbers 
        add     edx, 4*4               ; increment MTI index into MT buffer by 4
        mov     [ecx].MTI, edx
        cmp     edx, MERS_N*4
        jae     M420
        ret                            ; return random number in eax

; MT buffer exhausted. Make MERS_N new numbers ready for next time
M420:                                  ; eax is the random number to return
IF      MERS_N AND 3                   ; if MERS_N is not divisible by 4
        NVALID = MERS_N AND 3          ; only NVALID of the 4 premade numbers are valid
        ; Move premade numbers (4-NVALID) positions forward
        movdqa  xmm0, [ecx].PreInt
        movdqa  xmm1, [ecx].PreFlt
        movdqa  xmm2, [ecx].PreFlt+16
        movdqu  [ecx].PreInt + (4-NVALID)*4, xmm0
        movdqu  [ecx].PreFlt + (4-NVALID)*8, xmm1
IF NVALID EQ 3        
        movq    [ecx].PreFlt+16 + 8, xmm2
ENDIF        
        ; save index to first valid premade number
        mov     [ecx].PreInx, (4-NVALID)*4  
ENDIF
        
        ; MT buffer is empty. Fill it up
        push    ebx
        movd    xmm3, [ecx].UMASK      ; load constants
        movd    xmm4, [ecx].LMASK
        movd    xmm5, [ecx].MATA
        pshufd  xmm3, xmm3, 0          ; broadcast constants
        pshufd  xmm4, xmm4, 0
        pshufd  xmm5, xmm5, 0
        xor     ebx,  ebx              ; kk = 0
        mov     edx,  MERS_M*4         ; km
        
; change ecx from pointing to CRandomMersenneA to pointing to CRandomMersenneA.MT
        add     ecx, CRandomMersenneA.MT
        assume  ecx: ptr dword

M430:   ; kk loop
        movdqa  xmm2, xmmword ptr [ecx+ebx]        ; mt[kk]
        movd    xmm6, [ecx+ebx+16]
        movdqa  xmm1, xmmword ptr [ecx+ebx]        ; mt[kk]        
        movss   xmm2, xmm6             ; faster than movdqu xmm2, [ebx+4]
        pshufd  xmm2, xmm2, 00111001B  ; mt[kk+1]
        movq    xmm0, qword ptr [ecx+edx]; mt[km]
        movhps  xmm0, qword ptr [ecx+edx+8] ; faster than movdqu xmm0, [edx]
        pand    xmm1, xmm3             ; mt[kk] & UPPER_MASK
        pand    xmm2, xmm4             ; mt[kk+1] & LOWER_MASK
        por     xmm1, xmm2             ; y        
        movdqa  xmm2, xmm1             ; y
        pslld   xmm1, 31               ; copy bit 0 into all bits
        psrad   xmm1, 31               ; -(y & 1)
        pand    xmm1, xmm5             ; & MERS_A
        psrld   xmm2, 1                ; y >> 1
        pxor    xmm0, xmm1
        pxor    xmm0, xmm2
        movdqa  xmmword ptr [ecx+ebx], xmm0        ; result into mt[kk]
        cmp     ebx, (MERS_N-4)*4
        jae     M440                   ; exit loop when kk past end of buffer
        add     ebx, 16                ; kk += 4
        add     edx, 16                ; km += 4
        cmp     edx, (MERS_N-4)*4
        jbe     M430                   ; skip unless km wraparound
        sub     edx, MERS_N*4          ; km wraparound
        movdqu  xmm0, xmmword ptr [ecx]+(MERS_N-4)*4 ; copy end to before begin for km wraparound
        movdqa  xmmword ptr [ecx]-4*4, xmm0        
        movdqa  xmm0, xmmword ptr [ecx]            ; copy begin to after end for kk wraparound
        movdqu  xmmword ptr [ecx]+MERS_N*4, xmm0
        jmp     M430

M440:   ; loop finished. discard excess part of last result

; change ecx back to pointing to CRandomMersenneA
        sub     ecx, CRandomMersenneA.MT        
assume  ecx: ptr CRandomMersenneA

        mov     [ecx].MTI, 0
        pop     ebx
        ret                            ; random number is still in eax
        
; Generic version        
; this version is for old processors without XMM support:
M500    label near
        mov     edx, [ecx].MTI
        cmp     edx, MERS_N*4
        jnb     short M520             ; buffer is empty, fill it   
M510:   mov     eax, [ecx+edx*1].MT
        add     edx, 4
        mov     [ecx].MTI, edx
        
; optional tempering
        mov     edx, eax
        shr     eax, MERS_U
        xor     eax, edx
        mov     edx, eax
        shl     eax, MERS_S
        and     eax, MERS_B
        xor     eax, edx
        mov     edx, eax
        shl     eax, MERS_T
        and     eax, MERS_C
        xor     eax, edx
        mov     edx, eax
        shr     eax, MERS_L
        xor     eax, edx
; end of tempering

        ; Check if initialized
        cmp     [ecx].MATA, MERS_A
        jne     Error                  ; Make error if not initialized

        mov     edx, [ecx].PreInt      ; previously premade number
        mov     [ecx].PreInt, eax      ; store number for next call
        shl     eax, 20                ; convert to float
        mov     dword ptr [ecx].PreFlt, eax
        mov     eax, [ecx].PreInt
        shr     eax, 12
        or      eax, 3FF00000H
        mov     dword ptr [ecx].PreFlt+4, eax
        mov     eax, edx               ; return value is premade integer
        ret

        ; fill buffer with random numbers
M520:   push    ebx
        push    esi
        xor     esi, esi               ; kk
        mov     ebx, MERS_M*4          ; km
; change ecx from pointing to CRandomMersenneA to pointing to CRandomMersenneA.MT
        add     ecx, CRandomMersenneA.MT
        assume  ecx: ptr dword
        
        ; kk loop
M530:   mov     eax, [ecx+esi]
        mov     edx, [ecx+esi+4]
        and     eax, UPPER_MASK
        and     edx, LOWER_MASK
        or      eax, edx
        shr     eax, 1
        sbb     edx, edx
        and     edx, MERS_A
        xor     eax, edx
        xor     eax, [ecx+ebx]
        mov     [ecx+esi], eax
        add     ebx, 4
        cmp     ebx, MERS_N*4
        jb      short M540
        ; copy begin of table to after end to simplify kk+1 wraparound
        mov     eax, [ecx]
        mov     [ecx+ebx], eax 
        xor     ebx, ebx
M540:   add     esi, 4
        cmp     esi, MERS_N*4
        jb      M530                   ; loop end        
        
; change ecx back to pointing to CRandomMersenneA
        sub     ecx, CRandomMersenneA.MT        
assume  ecx: ptr CRandomMersenneA

        xor     edx, edx
        mov     [ecx].MTI, edx
        pop     esi
        pop     ebx
        jmp     M510        
        
_MersBRandom ENDP


;  extern "C" double MersRandom(void * Pthis); // Output random float

_MersRandom PROC NEAR                  ; generate random float with 32 bits resolution
        mov     ecx, [esp+4]           ; Pthis
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomMersenneA

        mov     edx, [ecx].PreInx      ; index into premade numbers
        fld     [ecx+edx*2].PreFlt     ; fetch premade floating point random number
        fsub    [ecx].one              ; subtract 1.0
        jmp     MersBRandom_reg        ; random bits
_MersRandom ENDP


;  extern "C" int MersIRandom (void * Pthis, int min, int max);  // Output random integer

_MersIRandom PROC   NEAR
        mov     ecx, [esp+4]           ; Pthis
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomMersenneA
        call    MersBRandom_reg        ; random bits
        mov     edx, [esp+12]          ; max
        mov     ecx, [esp+8]           ; min
        sub     edx, ecx
        jl      short M720             ; max < min
        add     edx, 1                 ; max - min + 1
        mul     edx                    ; multiply random number by interval and truncate
        lea     eax, [edx+ecx]         ; add min
        ret
M720:   mov     eax, 80000000H         ; error exit
        ret
_MersIRandom ENDP


;  extern "C" int MersIRandomX (void * Pthis, int min, int max);        // Output random integer

_MersIRandomX PROC   NEAR
        push    edi
        mov     ecx, [esp+8]           ; Pthis
        mov     edx, [esp+12]          ; min
        mov     edi, [esp+16]          ; max
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomMersenneA

        sub     edi, edx               ; max - min
        jle     short M830             ; max <= min (signed)
        inc     edi                    ; interval = max - min + 1
        
        ; if (interval != LastInterval) {
        cmp     edi, [ecx].LastInterval
        je      M810
        ; RLimit = uint32(((uint64)1 << 32) / interval) * interval - 1;}
        xor     eax, eax               ; 0
        lea     edx, [eax+1]           ; 1
        div     edi                    ; (would give overflow if interval = 1)
        mul     edi
        dec     eax
        mov     [ecx].RLimit, eax
        mov     [ecx].LastInterval, edi
M810:
M820:   ; do { // Rejection loop
        call    MersBRandom_reg        ; random bits (ecx is preserved)
        ; longran  = (uint64)BRandom() * interval;
        mul     edi
        ; } while (remainder > RLimit);
        cmp     eax, [ecx].RLimit
        ja      M820
        
        ; return (int32)iran + min
        mov     eax, [esp+12]          ; min
        add     eax, edx
        pop     edi
        ret
        
M830:   jl      M840
        ; max = min. Return min
        mov     eax, edx
        pop     edi
        ret                            ; max = min exit
        
M840:   ; max < min: error
        mov     eax, 80000000H         ; error exit
        pop     edi
        ret
_MersIRandomX ENDP



; -------------------------------------------------------------------------
;  Single-threaded static link versions for Mersenne Twister
; -------------------------------------------------------------------------

;  extern "C" void   MersenneRandomInitByArray(int const seeds[], int NumSeeds); // Seed by more than 32 bits
_MersenneRandomInitByArray PROC NEAR
        push    ebx
        push    esi
        push    edi
        push    ebp
        mov     ebx, [esp+20]          ; seeds
        mov     ebp, [esp+24]          ; NumSeeds
        LOADOFFSET2ECX MersenneInstance  ; Get address of MersenneInstance into ecx
        jmp     MersRandomInitByArray_reg     ; jump to function in mersenne32.asm
_MersenneRandomInitByArray ENDP        


;  extern "C" void   MersenneRandomInit(int seed);  // Re-seed
_MersenneRandomInit PROC NEAR
        mov     eax, [esp+4]           ; seed
        LOADOFFSET2ECX MersenneInstance  ; Get address of MersenneInstance into ecx
        jmp     MersRandomInit_reg     ; jump to function in mersenne32.asm
_MersenneRandomInit ENDP


;  extern "C" double MersenneRandom(); // Output random float
_MersenneRandom PROC NEAR              ; generate random float with 32 bits resolution
        LOADOFFSET2ECX MersenneInstance  ; Get address of MersenneInstance into ecx
assume  ecx: ptr CRandomMersenneA
        mov     edx, [ecx].PreInx      ; index into premade numbers
        fld     [ecx+edx*2].PreFlt     ; fetch premade floating point random number
        fsub    [ecx].one              ; subtract 1.0
        jmp     MersBRandom_reg        ; random bits
_MersenneRandom ENDP


;  extern "C" int MersenneIRandom (int min, int max); // Output random integer
_MersenneIRandom PROC   NEAR
        LOADOFFSET2ECX MersenneInstance  ; Get address of MersenneInstance into ecx
        call    MersBRandom_reg        ; random bits
        mov     edx, [esp+8]           ; max
        mov     ecx, [esp+4]           ; min
        sub     edx, ecx
        jl      short S410             ; max < min
        add     edx, 1                 ; max - min + 1
        mul     edx                    ; multiply random number by interval and truncate
        lea     eax, [edx+ecx]         ; add min
        ret
S410:   mov     eax, 80000000H         ; error exit
        ret
_MersenneIRandom ENDP


;  extern "C" int MersenneIRandomX(int min, int max); // Output random integer, exact

_MersenneIRandomX PROC   NEAR
        push    edi
        LOADOFFSET2ECX MersenneInstance  ; Get address of MersenneInstance into ecx
        mov     edx, [esp+8]           ; min
        mov     edi, [esp+12]          ; max
assume  ecx: ptr CRandomMersenneA

        sub     edi, edx               ; max - min
        jle     short S530             ; max <= min (signed)
        inc     edi                    ; interval = max - min + 1
        cmp     edi, [ecx].LastInterval
        je      S510
        xor     eax, eax               ; 0
        lea     edx, [eax+1]           ; 1
        div     edi                    ; (would give overflow if interval = 1)
        mul     edi
        dec     eax
        mov     [ecx].RLimit, eax
        mov     [ecx].LastInterval, edi
S510:
S520:   call    MersBRandom_reg        ; random bits (ecx is preserved)
        mul     edi
        cmp     eax, [ecx].RLimit
        ja      S520        
        mov     eax, [esp+8]           ; min
        add     eax, edx
        pop     edi
        ret     
        
S530:   jl      S540
        ; max = min. Return min
        mov     eax, edx
        pop     edi
        ret                            ; max = min exit
        
S540:   ; max < min: error
        mov     eax, 80000000H         ; error exit
        pop     edi
        ret     
_MersenneIRandomX ENDP


;  extern "C" unsigned int MersenneBRandom();                   // Output random bits
_MersenneBRandom PROC NEAR             ; generate random float with 32 bits resolution
        LOADOFFSET2ECX MersenneInstance  ; Get address of MersenneInstance into ecx
        jmp     MersBRandom_reg        ; random bits
_MersenneBRandom ENDP



IFDEF   WINDOWS
; -----------------------------------------------------------------
;  Single-threaded DLL versions for Mersenne Twister, Windows only
; -----------------------------------------------------------------

;  extern "C" void __stdcall MersenneRandomInitByArrayD(int const seeds[], int NumSeeds); // Seed by more than 32 bits
_MersenneRandomInitByArrayD@8 PROC NEAR
        ; translate __cdecl to __stdcall calling
        mov     eax, [esp+4]           ; seeds
        mov     edx, [esp+8]           ; NumSeeds
        push    edx
        push    eax
        call    _MersenneRandomInitByArray
        pop     ecx
        pop     ecx
        ret     8
_MersenneRandomInitByArrayD@8 ENDP        


;  extern "C" void __stdcall MersenneRandomInitD(int seed); // Re-seed
_MersenneRandomInitD@4 PROC NEAR
        ; remove parameter from stack
        pop     edx                    ; return address
        pop     eax                    ; seed
        push    edx                    ; put return address back in        
        mov     ecx, offset MersenneInstance
        ; eax = seed, ecx = Pthis
        jmp     MersRandomInit_reg     ; jump to function in mersenne32.asm
_MersenneRandomInitD@4 ENDP


;  extern "C" double __stdcall MersenneRandomD(); // Output random float
_MersenneRandomD@0 PROC NEAR           ; generate random float with 32 bits resolution
        mov     ecx, offset MersenneInstance
assume  ecx: ptr CRandomMersenneA
        mov     edx, [ecx].PreInx      ; index into premade numbers
        fld     [ecx+edx*2].PreFlt     ; fetch premade floating point random number
        fsub    [ecx].one              ; subtract 1.0
        jmp     MersBRandom_reg        ; random bits
_MersenneRandomD@0 ENDP


;  extern "C" int __stdcall MersenneIRandomD (int min, int max); // Output random integer
_MersenneIRandomD@8 PROC   NEAR
        mov     ecx, offset MersenneInstance
        call    MersBRandom_reg        ; random bits
        mov     edx, [esp+8]           ; max
        mov     ecx, [esp+4]           ; min
        sub     edx, ecx
        jl      short S710             ; max < min
        add     edx, 1                 ; max - min + 1
        mul     edx                    ; multiply random number by interval and truncate
        lea     eax, [edx+ecx]         ; add min
        ret     8
S710:   mov     eax, 80000000H         ; error exit
        ret     8
_MersenneIRandomD@8 ENDP


;  extern "C" int __stdcall MersenneIRandomXD(int min, int max); // Output random integer, exact

_MersenneIRandomXD@8 PROC   NEAR
        push    edi
        mov     ecx, offset MersenneInstance
        mov     edx, [esp+8]           ; min
        mov     edi, [esp+12]          ; max
assume  ecx: ptr CRandomMersenneA

        sub     edi, edx               ; max - min
        jle     short S830             ; max <= min (signed)
        inc     edi                    ; interval = max - min + 1
        cmp     edi, [ecx].LastInterval
        je      S810
        xor     eax, eax               ; 0
        lea     edx, [eax+1]           ; 1
        div     edi                    ; (would give overflow if interval = 1)
        mul     edi
        dec     eax
        mov     [ecx].RLimit, eax
        mov     [ecx].LastInterval, edi
S810:
S820:   call    MersBRandom_reg        ; random bits (ecx is preserved)
        mul     edi
        cmp     eax, [ecx].RLimit
        ja      S820        
        mov     eax, [esp+8]           ; min
        add     eax, edx
        pop     edi
        ret     8
        
S830:   jl      S840
        ; max = min. Return min
        mov     eax, edx
        pop     edi
        ret     8                      ; max = min exit
        
S840:   ; max < min: error
        mov     eax, 80000000H         ; error exit
        pop     edi
        ret     8
_MersenneIRandomXD@8 ENDP


;  extern "C" unsigned int __stdcall MersenneBRandomD(); // Output random bits
_MersenneBRandomD@0 PROC NEAR          ; generate random float with 32 bits resolution
        mov     ecx, offset MersenneInstance
assume  ecx: ptr CRandomMersenneA
        jmp     MersBRandom_reg        ; random bits
_MersenneBRandomD@0 ENDP

ENDIF  ; WINDOWS

IFDEF   POSITIONINDEPENDENT
get_thunk_ecx: ; load caller address into ecx for position-independent code
        mov ecx, [esp]
        ret
ENDIF   ; POSITIONINDEPENDENT

END
