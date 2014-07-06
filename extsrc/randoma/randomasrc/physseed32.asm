;*************************  physseed32.asm  **********************************
; Author:           Agner Fog
; Date created:     
; Last modified:    2010-08-03
; Source URL:       www.agner.org/optimize
; Project:          asmlib.zip
; C++ prototype:
; extern "C" int PhysicalSeed(int seeds[], int NumSeeds);
;
; Description:
; Generates a non-deterministic random seed from a physical random number generator 
; which is available on VIA processors. 
; Uses the time stamp counter (which is less random) if no physical random number
; generator is available.
; The code is not optimized for speed because it is typically called only once.
;
; Parameters:
; int seeds[]       An array which will be filled with random numbers
; int NumSeeds      Indicates the desired number of 32-bit random numbers
;
; Return value:     0   Failure. No suitable instruction available (processor older than Pentium)
;                   1   No physical random number generator. Used time stamp counter instead
;                   2   Success. VIA physical random number generator used
; 
; The return value will indicate the availability of a physical random number generator
; even if NumSeeds = 0.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.686
.xmm
.model flat

.code

_PhysicalSeed PROC NEAR
        push    ebx
        push    esi
        push    edi
        pushfd
        pop     eax
        btc     eax, 21                ; check if CPUID bit can toggle
        push    eax
        popfd
        pushfd
        pop     ebx
        xor     ebx, eax
        xor     eax, eax               ; 0
        bt      ebx, 21
        jc      FAILURE                ; CPUID not supported
        cpuid                          ; get number of CPUID functions
        test    eax, eax
        jz      FAILURE                ; function 1 not supported
        mov     eax, 1
        cpuid                          ; get features
        bt      edx, 4                 ; check if RDTSC supported
        jnc     FAILURE
        ; test if VIA xstore instruction supported
        mov     eax, 0C0000000H
        mov     esi, eax
        cpuid
        cmp     eax, esi
        jna     USE_RDTSC              ; not a VIA processor
        lea     eax, [esi+1]
        cpuid
        bt      edx, 3
        jnc     USE_RDTSC              ; XSTORE instruction not supported or not enabled
        
;       XSTORE  supported
        mov     edi, [esp+16]          ; seeds
        mov     ecx, [esp+20]          ; NumSeeds
        mov     ebx, ecx
        and     ecx, -2                ; round down to nearest even
        jz      XSTORE_ODD             ; NumSeeds <= 1
        ; make an even number of random dwords
        shl     ecx, 2                 ; number of bytes (divisible by 8)
        mov     edx, 3                 ; quality factor
        db 0F3H, 00FH, 0A7H, 0C0H      ; rep xstore instuction
XSTORE_ODD:
        test    ebx, 1
        jz      END_XSTORE
        ; NumSeeds is odd. Make 8 bytes in temporary buffer and store 4 of the bytes
        mov     esi, edi               ; current output pointer
        push    ebp
        mov     ebp, esp
        sub     esp, 8                 ; make temporary space on stack
        and     esp, -8                ; align by 8
        mov     edi, esp
        mov     ecx, 4                 ; Will generate 4 or 8 bytes, depending on CPU
        mov     edx, 3                 ; quality factor
        db 0F3H, 00FH, 0A7H, 0C0H      ; rep xstore instuction
        mov     eax, [esp]
        mov     [esi], eax             ; store the last 4 bytes
        mov     esp, ebp
        pop     ebp
END_XSTORE:
        mov     eax, 2                 ; return value        
        pop     edi
        pop     esi
        pop     ebx
        ret        
        
USE_RDTSC:
        mov     edi, [esp+16]          ; seeds
        mov     ecx, [esp+20]          ; NumSeeds
        test    ecx, ecx
        jz      END_RDTSC
        xor     eax, eax
        mov     esi, edi
        rep stosd                      ; fill seeds with zeroes
        cpuid                          ; serialize
        rdtsc                          ; get time stamp counter
        mov     [esi], eax             ; store time stamp counter as seeds[0]
        cmp     dword ptr [esp+20], 1  ; seeds
        jna     END_RDTSC
        mov     [esi+4], edx           ; store high bits as seeds[1]        
END_RDTSC:
        mov     eax, 1                 ; return value        
EXIT1:
        pop     edi
        pop     esi
        pop     ebx
        ret 
        
FAILURE: ; No useful instruction supported
        xor     eax, eax
        jmp     EXIT1     
        
_PhysicalSeed ENDP

END
