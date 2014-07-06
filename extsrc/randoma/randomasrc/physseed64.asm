;*************************  physseed64.asm  **********************************
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

.code

PhysicalSeed PROC
        push    rbx
IFDEF WINDOWS
        push    rsi
        push    rdi
        mov     rdi, rcx               ; seeds
        mov     esi, edx               ; NumSeeds
ENDIF
        
        ; test if VIA xstore instruction supported
        mov     eax, 0C0000000H
        mov     r8d, eax
        cpuid
        cmp     eax, r8d
        jna     USE_RDTSC              ; not a VIA processor
        lea     eax, [r8+1]
        cpuid
        bt      edx, 3
        jnc     USE_RDTSC              ; XSTORE instruction not supported or not enabled
        
;       XSTORE  supported
        mov     ecx, esi               ; NumSeeds
        and     ecx, -2                ; round down to nearest even
        jz      XSTORE_ODD             ; NumSeeds <= 1
        ; make an even number of random dwords
        shl     ecx, 2                 ; number of bytes (divisible by 8)
        mov     edx, 3                 ; quality factor
        db 0F3H, 00FH, 0A7H, 0C0H      ; rep xstore instuction
XSTORE_ODD:        
        test    esi, 1
        jz      END_XSTORE
        ; NumSeeds is odd. Make 8 bytes in temporary buffer and store 4 of the bytes
        mov     rbx, rdi               ; current output pointer
        mov     ecx, 4                 ; Will generate 4 or 8 bytes, depending on CPU
        mov     edx, 3                 ; quality factor
        push    rcx                    ; make temporary space on stack
        mov     rdi, rsp               ; point to buffer on stack
        db 0F3H, 00FH, 0A7H, 0C0H      ; rep xstore instuction
        pop     rax
        mov     [rbx], eax             ; store the last 4 bytes
END_XSTORE:
        mov     eax, 2                 ; return value        
IFDEF WINDOWS
        pop     rdi
        pop     rsi
ENDIF        
        pop     rbx
        ret        
        
USE_RDTSC:
        test    esi, esi
        jz      END_RDTSC
        xor     eax, eax
        mov     ecx, esi
        push    rdi
        rep stosd                      ; fill seeds with zeroes
        pop     rdi
        cpuid                          ; serialize
        rdtsc                          ; get time stamp counter
        mov     [rdi], eax             ; store time stamp counter as seeds[0]
        cmp     esi, 1  ; seeds
        jna     END_RDTSC
        mov     [rdi+4], edx           ; store high bits as seeds[1]        
END_RDTSC:
        mov     eax, 1                 ; return value
EXIT1:
IFDEF WINDOWS
        pop     rdi
        pop     rsi
ENDIF        
        pop     rbx
        ret        
        
FAILURE: ; No useful instruction supported
        xor     eax, eax
        jmp     EXIT1
        
PhysicalSeed ENDP

END
