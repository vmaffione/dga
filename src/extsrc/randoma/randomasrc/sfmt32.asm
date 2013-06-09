; ----------------------------- SFMT32.ASM ---------------------------
; Author:        Agner Fog
; Date created:  2008-11-01
; Last modified: 2008-11-26
; Project:       randoma library of random number generators
; Source URL:    www.agner.org/random
; Description:
; Random number generator of type SIMD-oriented Fast Mersenne Twister (SFMT)
; (Mutsuo Saito and Makoto Matsumoto: "SIMD-oriented Fast Mersenne Twister:
; a 128-bit Pseudorandom Number Generator", Monte Carlo and Quasi-Monte 
; Carlo Methods 2006, Springer, 2008, pp. 607-622).
;
; 32-bit mode version for x86 compatible microprocessors.
; See ran-instructions.pdf for detailed instructions
; Copyright (c) 2008 Agner Fog. 
; GNU General Public License http://www.gnu.org/licenses
; ----------------------------------------------------------------------

.686
.xmm
.model flat

; structure definition and constants:
INCLUDE randomah.asi

extern _InstructionSet: near

.DATA
align 16
; Data for single instance of random number generator
SFMTInstance CRandomSFMTA <>

align 16
; Initialization constants for Mother-Of-All:
InitMother DD 2111111111, 0, 1492, 0, 1776, 0, 5115, 0
; Initialization Mask for SFMT:
InitMask   DD SFMT_MASK
; Period certification vector for SFMT:
InitParity DD SFMT_PARITY


.CODE


; ---------------------------------------------------------------
;  Thread-safe static link versions for SFMT
; ---------------------------------------------------------------

; extern "C" void SFMTRandomInit(void * Pthis, int ThisSize, int seed, int IncludeMother = 1);
; Parameters:
; [esp+4]  = Pthis
; [esp+8]  = ThisSize
; [esp+12] = seed
; [esp+16] = IncludeMother

_SFMTRandomInit PROC NEAR
        mov     ecx, [esp+4]           ; Pthis
        cmp     dword ptr [esp+8], (type CRandomSFMTA)
        jb      Error                  ; Error exit if buffer too small
        push    edi

        ; Align by 16. Will overlap part of Fill if Pthis unaligned        
        and     ecx, -16
assume  ecx: ptr CRandomSFMTA

        xor     eax, eax
        cmp     dword ptr [esp+16+4], eax ; IncludeMother
        setnz   al                     ; convert any nonzero value to 1
        ; Store USEMOTHER
        mov     [ecx].USEMOTHER, eax
        
        mov     eax, [esp+12+4]        ; seed
        xor     edi, edi               ; loop counter i
        jmp     L2                     ; go into seeding loop

L1:     ; seeding loop for SFMT
        ; y = factor * (y ^ (y >> 30)) + (++i);
        call    InitSubf0              ; randomization subfunction
L2:     mov     [ecx+edi*4].STATE,eax  ; initialize state
        cmp     edi, SFMT_N*4 - 1
        jb      L1

        ; Put 5 more values into Mother-Of-All generator
        call    InitSubf0
        mov     [ecx].M0, eax
        call    InitSubf0
        mov     [ecx].M1, eax
        call    InitSubf0
        mov     [ecx].M2, eax
        call    InitSubf0
        mov     [ecx].M3, eax
        call    InitSubf0
        mov     [ecx].MC, eax
        
        ; more initialization and period certification
        call    InitAndPeriod
        
        pop     edi
        ret
_SFMTRandomInit ENDP
        
Error   label   near                        ; Error exit
        xor     eax, eax
        div     eax                         ; Divide by 0
        ret
        
; Subfunction used by _SFMTRandomInit
InitSubf0 proc near private
; y = 1812433253 * (y ^ (y >> 30)) + (++i);
; input parameters:
; eax = y
; edi = i
; output:
; eax = new y
; edi = i+1
; edx modified
        mov     edx, eax
        shr     eax, 30
        xor     eax, edx
        imul    eax, 1812433253
        inc     edi
        add     eax, edi
        ret
InitSubf0 endp 
       
; Subfunction used by _SFMTRandomInitByArray
InitSubf1 proc near private
; r = 1664525U * (r ^ (r >> 27));
; input parameters:
; eax = r
; output:
; eax = new r
; edx modified
        mov     edx, eax
        shr     eax, 27
        xor     eax, edx
        imul    eax, 1664525
        ret
InitSubf1 endp

; Subfunction used by _SFMTRandomInitByArray
InitSubf2 proc near private
; r = 1566083941U * (r ^ (r >> 27));
; input parameters:
; eax = r
; output:
; eax = new r
; edx modified
        mov     edx, eax
        shr     eax, 27
        xor     eax, edx
        imul    eax, 1566083941
        ret
InitSubf2 endp

; Subfunciton for initialization and period certification, except seeding
; ecx = aligned pointer to CRandomSFMTA
InitAndPeriod   proc    near private
        push    ebx
        push    edi
        ; initialize constants for Mother-Of-All and SFMT
        LOADOFFSET2EDI InitMother      ; edi points to InitMother
        
        xor     edx, edx
L1:     ; Loop fills MF3 - MF0
        mov     eax, [edi+edx]         ; load from InitMother
        mov     [ecx+edx].MF3, eax
        add     edx, 4
        cmp     edx, 32
        jb      L1
        xor     edx, edx
L2:     ; Loop fills AMASK
        mov     eax, [edi+edx+32]      ; load from InitMask
        mov     [ecx+edx].AMASK, eax
        add     edx, 4
        cmp     edx, 4*4
        jb      L2        
        
        ; get instruction set
        push    ecx
        call    _InstructionSet
        pop     ecx
        mov     [ecx].Instset, eax
        xor     eax, eax
        mov     dword ptr [ecx].one, eax
        mov     dword ptr [ecx+4].one, 3FF00000H        
        
        ; Period certification
        ; Compute parity of STATE[0-4] & InitParity
        xor     edx, edx               ; parity
        xor     ebx, ebx               ; loop counter
L4:     mov     eax, [ecx+ebx*4].STATE
        and     eax, [edi+(InitParity-InitMother)+ebx*4] ; and InitParity[i]
        xor     edx, eax
        inc     ebx
        cmp     ebx, 4
        jb      L4
        
        ; get parity of edx
        mov     eax, edx
        shr     edx, 16
        xor     eax, edx
        xor     al, ah
        jpo     L8                     ; parity odd: period OK
        
        ; parity even: period not OK
        ; Find a nonzero dword in period certification vector
        xor     ebx, ebx               ; loop counter
L5:     mov     eax, [edi+(InitParity-InitMother)+ebx*4]  ; InitParity[i]
        test    eax, eax
        jnz     L6
        inc     ebx
        ; assume that there is a nonzero dword in InitParity
        jmp     L5                     ; loop until nonzero found
        
L6:     ; find first nonzero bit in eax
        bsf     edx, eax
        ; flip the corresponding bit in STATE
        btc     [ecx+ebx*4].STATE, edx

L8:     cmp     [ecx].USEMOTHER, 0
        je      L9
        call    Mother_Next                 ; Make first random number ready

L9:     ; Generate first random numbers and set IX = 0
        call    SFMT_Generate
        pop     edi
        pop     ebx
        ret
InitAndPeriod   endp


;  extern "C" void SFMTRandomInitByArray
; (void * Pthis, int ThisSize, int const seeds[], int NumSeeds, int IncludeMother = 1);
; // Seed by more than 32 bits
_SFMTRandomInitByArray PROC NEAR
; Parameters
; [esp+ 4] = Pthis
; [esp+ 8] = ThisSize
; [esp+12] = seeds
; [esp+16] = NumSeeds
; [esp+20] = IncludeMother

; define constants:
SFMT_SIZE equ SFMT_N*4                 ; number of 32-bit integers in state

IF SFMT_SIZE GE 623
   SFMT_LAG equ 11
ELSEIF SFMT_SIZE GE 68
   SFMT_LAG equ  7
ELSEIF SFMT_SIZE GE 39
   SFMT_LAG equ  5
ELSE
   SFMT_LAG equ  3
ENDIF

SFMT_MID equ ((SFMT_SIZE - SFMT_LAG) / 2)

        push    ebx
        push    esi
        push    edi
        push    ebp
        cmp     dword ptr [esp+8+16], (type CRandomSFMTA); ThisSize
        jb      Error                  ; Error exit if buffer too small
        mov     ecx, [esp+4+16]        ; Pthis
        mov     ebx, [esp+12+16]       ; seeds
        mov     ebp, [esp+16+16]       ; NumSeeds

        ; Align by 16. Will overlap part of Fill if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomSFMTA

        xor     eax, eax
        cmp     dword ptr [esp+20+16], eax ; IncludeMother
        setnz   al                     ; convert any nonzero value to 1
        ; Store USEMOTHER
        mov     [ecx].USEMOTHER, eax

; 1. loop: Fill state vector with random numbers from NumSeeds
; r = NumSeeds;
; for (i = 0; i < SFMT_N*4; i++) {
;    r = factor * (r ^ (r >> 30)) + i;
;    sta[i] = r;}

        mov     eax, ebp               ; r = NumSeeds
        xor     esi, esi               ; i
L0:     mov     edx, eax
        shr     eax, 30
        xor     eax, edx
        imul    eax, 1812433253
        add     eax, esi
        mov     [ecx+esi*4].STATE, eax
        inc     esi
        cmp     esi, SFMT_SIZE
        jb      L0        

        ; count = max(NumSeeds,size-1)
        mov     eax, SFMT_SIZE - 1
        cmp     ebp, eax
        cmovb   ebp, eax
        push    ebp                    ; save count as local variable
        
; 2. loop: Fill state vector with random numbers from seeds[]
; for (i = 1, j = 0; j < count; j++) {
;    r = func1(sta[i] ^ sta[(i + mid) % size] ^ sta[(i + size - 1) % size]);
;    sta[(i + mid) % size] += r;
;    if (j < NumSeeds) r += seeds[j]
;    r += i;
;    sta[(i + mid + lag) % size] += r;
;    sta[i] = r;
;    i = (i + 1) % size;
; }
        xor     edi, edi
        lea     esi, [edi+1]

        ; ecx = Pthis
        ; ebx = seeds
        ; esi = i
        ; edi = j
        ; eax = r
        ; [esp] = count
        ; [esp+20+16] = NumSeeds

L1:     ; r = sta[i] ^ sta[(i + mid) % size] ^ sta[(i + size - 1) % size];
        mov     eax, [ecx+esi*4].STATE           ; sta[i]
        lea     ebp, [esi+SFMT_MID]
        cmp     ebp, SFMT_SIZE
        jb      L2
        sub     ebp, SFMT_SIZE
L2:     xor     eax, [ecx+ebp*4].STATE           ; sta[(i + mid) % size]
        lea     edx, [esi+SFMT_SIZE-1]
        cmp     edx, SFMT_SIZE
        jb      L3
        sub     edx, SFMT_SIZE
L3:     xor     eax, [ecx+edx*4].STATE           ; sta[(i + size - 1) % size]

        ; r = func1(r) = (r ^ (r >> 27)) * 1664525U;
        call    InitSubf1
        
        ; sta[(i + mid) % size] += r;
        add     [ecx+ebp*4].STATE, eax
        
        ; if (j < NumSeeds) r += seeds[j]
        cmp     edi, [esp+20+16]
        jnb     L4
        add     eax, [ebx+edi*4]        
L4:
        ; r += i;
        add     eax, esi
        
        ; sta[(i + mid + lag) % size] += r;
        lea     edx, [esi+SFMT_MID+SFMT_LAG]
        cmp     edx, SFMT_SIZE
        jb      L5
        sub     edx, SFMT_SIZE
L5:     add     [ecx+edx*4].STATE, eax
        
        ;sta[i] = r;
        mov     [ecx+esi*4].STATE, eax
        
        ; i = (i + 1) % size;
        inc     esi
        cmp     esi, SFMT_SIZE
        jb      L6
        sub     esi, SFMT_SIZE
L6:
        ; j++, loop while j < count
        inc     edi
        cmp     edi, [esp]
        jb      L1
        
; 3. loop: Randomize some more
; for (j = 0; j < size; j++) {
;   r = func2(sta[i] + sta[(i + mid) % size] + sta[(i + size - 1) % size]);
;   sta[(i + mid) % size] ^= r;
;   r -= i;
;   sta[(i + mid + lag) % size] ^= r;
;   sta[i] = r;
;   i = (i + 1) % size;
; }
        ; j = 0
        xor     edi, edi

L10:    ; r = sta[i] + sta[(i + mid) % size] + sta[(i + size - 1) % size]
        mov     eax, [ecx+esi*4].STATE           ; sta[i]
        lea     ebp, [esi+SFMT_MID]
        cmp     ebp, SFMT_SIZE
        jb      L11
        sub     ebp, SFMT_SIZE
L11:    add     eax, [ecx+ebp*4].STATE           ; sta[(i + mid) % size]
        lea     edx, [esi+SFMT_SIZE-1]
        cmp     edx, SFMT_SIZE
        jb      L12
        sub     edx, SFMT_SIZE
L12:    add     eax, [ecx+edx*4].STATE           ; sta[(i + size - 1) % size]

        ; r = func2(r) = (x ^ (x >> 27)) * 1566083941U;
        call    InitSubf2
        
        ; sta[(i + mid) % size] ^= r;
        xor     [ecx+ebp*4].STATE, eax
        
        ; r -= i;
        sub     eax, esi
        
        ; sta[(i + mid + lag) % size] ^= r;
        lea     edx, [esi+SFMT_MID+SFMT_LAG]
        cmp     edx, SFMT_SIZE
        jb      L13
        sub     edx, SFMT_SIZE
L13:    xor     [ecx+edx*4].STATE, eax

        ; sta[i] = r;
        mov     [ecx+esi*4].STATE, eax
        
        ; i = (i + 1) % size;
        inc     esi
        cmp     esi, SFMT_SIZE
        jb      L14
        sub     esi, SFMT_SIZE
L14:
        ; j++, loop while j < size
        inc     edi
        cmp     edi, SFMT_SIZE
        jb      L10
    
        pop     ebp                              ; remove local variable count

        ; if (UseMother) {
        cmp     [ecx].USEMOTHER, 0
        jz      L20
        
; 4. loop: Initialize MotherState
; for (j = 0; j < 5; j++) {
;    r = func2(r) + j;
;    MotherState[j] = r + sta[2*j];
; }
        call    InitSubf2
        mov     edx, [ecx].STATE
        add     edx, eax
        mov     [ecx].M0, edx
        call    InitSubf2
        inc     eax
        mov     edx, [ecx+8].STATE
        add     edx, eax
        mov     [ecx].M1, edx
        call    InitSubf2
        add     eax, 2
        mov     edx, [ecx+16].STATE
        add     edx, eax        
        mov     [ecx].M2, edx
        call    InitSubf2
        add     eax, 3
        mov     edx, [ecx+24].STATE
        add     edx, eax        
        mov     [ecx].M3, edx
        call    InitSubf2
        add     eax, 4
        mov     edx, [ecx+32].STATE
        add     edx, eax        
        mov     [ecx].MC, edx
        
L20:    ; More initialization and period certification
        call    InitAndPeriod
        
        pop     ebp
        pop     edi
        pop     esi
        pop     ebx
        ret
_SFMTRandomInitByArray ENDP


align 16
Mother_Next proc near private
; Internal procedure: advance Mother-Of-All generator
; The random value is in M0
; ecx = pointer to structure CRandomSFMTA
; eax, ecx, xmm0 unchanged
assume  ecx: ptr CRandomSFMTA
        cmp     [ecx].Instset, 4
        jb      Mother_Next_386
        movdqa  xmm1, xmmword ptr [ecx].M3  ; load M3,M2
        movdqa  xmm2, xmmword ptr [ecx].M1  ; load M1,M0
        movhps  qword ptr [ecx].M3, xmm1    ; M3=M2
        movq    qword ptr [ecx].M2, xmm2    ; M2=M1
        movhps  qword ptr [ecx].M1, xmm2    ; M1=M0
        pmuludq xmm1, xmmword ptr [ecx].MF3 ; M3*MF3, M2*MF2
        pmuludq xmm2, xmmword ptr [ecx].MF1 ; M1*MF1, M0*MF0
        paddq   xmm1, xmm2                  ; P3+P1, P2+P0
        movhlps xmm2, xmm1                  ; Get high qword
        movq    xmm3, qword ptr [ecx].MC    ; +carry
        paddq   xmm1, xmm3
        paddq   xmm1, xmm2                  ; P0+P1+P2+P3
        movq    qword ptr [ecx].M0, xmm1    ; Store new M0 and carry
        ret
        
Mother_Next_386: ; same, no SSE2
        push    eax
        push    esi
        push    edi
        ; prepare new random number
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
        mov     eax,[ecx].M0                ; x[n-1]
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
        pop     edi
        pop     esi
        pop     eax
        ret

Mother_Next endp


align 16
SFMT_Generate proc near private
; void CRandomSFMT::Generate() {
; Fill state array with new random numbers
assume  ecx: ptr CRandomSFMTA

; check if SSE2 instruction set supported
        cmp     [ecx].Instset, 4
        jb      SFMT_Generate_386
        push    ebx
        
        ; register use
        ; ecx = Pthis
        ; edx = i*16 + offset state
        ; eax, ebx = loop end
        ; xmm1 = r1
        ; xmm2 = r2 = r
        ; xmm0, xmm3 = scratch
        
        ; r1 = state[SFMT_N*16 - 2];
        ; r2 = state[SFMT_N*16 - 1];
        movdqa  xmm1, xmmword ptr [ecx+(SFMT_N-2)*16].STATE
        movdqa  xmm2, xmmword ptr [ecx+(SFMT_N-1)*16].STATE
        mov     edx, CRandomSFMTA.STATE        
        
;static inline __m128i sfmt_recursion(__m128i const &a, __m128i const &b, 
;__m128i const &c, __m128i const &d, __m128i const &mask) {
;    __m128i a1, b1, c1, d1, z1, z2;
;    b1 = _mm_srli_epi32(b, SFMT_SR1);
;    a1 = _mm_slli_si128(a, SFMT_SL2);
;    c1 = _mm_srli_si128(c, SFMT_SR2);
;    d1 = _mm_slli_epi32(d, SFMT_SL1);
;    b1 = _mm_and_si128(b1, mask);
;    z1 = _mm_xor_si128(a, a1);
;    z2 = _mm_xor_si128(b1, d1);
;    z1 = _mm_xor_si128(z1, c1);
;    z2 = _mm_xor_si128(z1, z2);
;    return z2;}

; for (i = 0; i < SFMT_N - SFMT_M; i++) {
;    r = sfmt_recursion(state[i], state[i + SFMT_M], r1, r2, mask);
;    state[i] = r;
;    r1 = r2;
;    r2 = r;
; }

        mov      eax, (SFMT_N-SFMT_M)*16 + CRandomSFMTA.STATE ; first loop end
        mov      ebx, SFMT_N*16 + CRandomSFMTA.STATE          ; second loop end

; first i loop from 0 to SFMT_N - SFMT_M
align 8
L1:     movdqa   xmm0, xmmword ptr [ecx+edx+SFMT_M*16]        ; b
        psrld    xmm0, SFMT_SR1                               ; b1
        pand     xmm0, xmmword ptr [ecx].AMASK                ; b1
        movdqa   xmm3, xmmword ptr [ecx+edx]                  ; a
        pxor     xmm0, xmm3
        pslldq   xmm3, SFMT_SL2                               ; a1
        psrldq   xmm1, SFMT_SR2                               ; c1, c = r1
        pxor     xmm0, xmm3
        pxor     xmm0, xmm1
        movdqa   xmm1, xmm2                                   ; r1 = r2
        pslld    xmm2, SFMT_SL1                               ; d1, d = r2
        pxor     xmm2, xmm0                                   ; r2 = r
        ; state[i] = r;
        movdqa   xmmword ptr [ecx+edx], xmm2
        
        ; i++ while i < SFMT_N - SFMT_M
        add      edx, 16
        cmp      edx, eax
        jb       L1
        
;align 16
L2:     ; second i loop from SFMT_N - SFMT_M + 1 to SFMT_N
        movdqa   xmm0, xmmword ptr [ecx+edx+(SFMT_M-SFMT_N)*16]; b
        psrld    xmm0, SFMT_SR1                               ; b1
        pand     xmm0, xmmword ptr [ecx].AMASK                ; b1
        movdqa   xmm3, xmmword ptr [ecx+edx]                  ; a
        pxor     xmm0, xmm3
        pslldq   xmm3, SFMT_SL2                               ; a1
        psrldq   xmm1, SFMT_SR2                               ; c1, c = r1
        pxor     xmm0, xmm3
        pxor     xmm0, xmm1
        movdqa   xmm1, xmm2                                   ; r1 = r2
        pslld    xmm2, SFMT_SL1                               ; d1, d = r2
        pxor     xmm2, xmm0                                   ; r2 = r
        ; state[i] = r;
        movdqa   xmmword ptr [ecx+edx], xmm2
        
        ; i++ while i < SFMT_N
        add      edx, 16
        cmp      edx, ebx
        jb       L2
        
        ; Check if initialized
L8:     cmp     [ecx].AMASK, SFMT_MASK1
        jne     Error                  ; Make error if not initialized

        ; ix = 0;
        mov      [ecx].IX, 0 ; point to start of STATE buffer
        pop      ebx
        ret

; Same, SSE2 instruction set not supported:
SFMT_Generate_386:
        push    ebx
        push    esi
        push    edi
        push    ebp
        sub     esp, 32

        ; register use
        ; ecx = Pthis
        ; edx = i*16
        ; ebx = ((i+SFMT_M) mod SFMT_N) * 16
        ; ebp = accumulator
        ; eax = temporary
        ; esi, edi = previous state[i]
        
        RR1 equ [esp]               ; r1
        RR2 equ [esp+16]            ; r2 = r
        
        ; r1 = state[SFMT_N - 2];
        ; r2 = state[SFMT_N - 1];
        lea     esi, [ecx+(SFMT_N-2)*16].STATE
        mov     edi, esp
        push    ecx
        mov     ecx, 8
        rep     movsd
        pop     ecx
        
; The two loops from i = 0 to SFMT_N - SFMT_M - 1 and 
; from SFMT_N - SFMT_M to SFMT_N - 1 are joined together here:
; for (i = 0; i < SFMT_N; i++) {
;    r = sfmt_recursion(state[i], state[(i+SFMT_M)%SFMT_N], r1, r2, mask);
;    state[i] = r;
;    r1 = r2;
;    r2 = r;

        xor     edx, edx               ; i = 0
        mov     ebx, SFMT_M * 16       ; j = ((i+SFMT_M)%SFMT_N)*16
        
M1:     ; loop start
        ; 1. dword:
        mov     ebp, [ecx+ebx].STATE[0]
        shr     ebp, SFMT_SR1                    ; 32-bit shifts right
        and     ebp, [ecx].AMASK[0]
        mov     eax, [ecx+edx].STATE[0]
        xor     ebp, eax
        mov     esi, eax                         ; save for 2. dword
        shl     eax, SFMT_SL2*8                  ; 128-bit shift left
        xor     ebp, eax
        mov     eax, [RR1][0]
        mov     edi, [RR1][4]
        shrd    eax, edi, SFMT_SR2*8 ; 128-bit shift right
        xor     ebp, eax
        mov     eax, [RR2][0]
        mov     [RR1][0], eax                    ; r1 = r2
        shl     eax, SFMT_SL1                    ; 32-bit shifts left
        xor     ebp, eax
        mov     [RR2][0], ebp                    ; r2 = r
        mov     [ecx+edx].STATE[0], ebp          ; state[i] = r
        
        ; 2. dword:
        mov     ebp, [ecx+ebx].STATE[4]
        shr     ebp, SFMT_SR1                    ; 32-bit shifts right
        and     ebp, [ecx].AMASK[4]
        mov     eax, [ecx+edx].STATE[4]
        xor     ebp, eax        
        mov     edi, eax                         ; save for 3. dword
        ; esi = [ecx+edx].STATE[0] before change
        shld    eax, esi, SFMT_SL2*8             ; 128-bit shift left
        xor     ebp, eax
        mov     eax, [RR1][4]
        mov     esi, [RR1][8]
        shrd    eax, esi, SFMT_SR2*8 ; 128-bit shift right
        xor     ebp, eax
        mov     eax, [RR2][4]
        mov     [RR1][4], eax                    ; r1 = r2
        shl     eax, SFMT_SL1                    ; 32-bit shifts left
        xor     ebp, eax
        mov     [RR2][4], ebp                    ; r2 = r
        mov     [ecx+edx].STATE[4], ebp          ; state[i] = r
        
        ; 3. dword:
        mov     ebp, [ecx+ebx].STATE[8]
        shr     ebp, SFMT_SR1                    ; 32-bit shifts right
        and     ebp, [ecx].AMASK[8]
        mov     eax, [ecx+edx].STATE[8]
        mov     esi, eax                         ; save for 4. dword
        xor     ebp, eax
        ; edi = [ecx+edx].STATE[4] before change
        shld    eax, edi, SFMT_SL2*8             ; 128-bit shift left
        xor     ebp, eax
        mov     eax, [RR1][8]
        mov     edi, [RR1][12]
        shrd    eax, edi, SFMT_SR2*8             ; 128-bit shift right
        xor     ebp, eax
        mov     eax, [RR2][8]
        mov     [RR1][8], eax                    ; r1 = r2
        shl     eax, SFMT_SL1                    ; 32-bit shifts left
        xor     ebp, eax
        mov     [RR2][8], ebp                    ; r2 = r
        mov     [ecx+edx].STATE[8], ebp          ; state[i] = r
        
        ; 4. dword:
        mov     ebp, [ecx+ebx].STATE[12]
        shr     ebp, SFMT_SR1                    ; 32-bit shifts right
        and     ebp, [ecx].AMASK[12]
        mov     eax, [ecx+edx].STATE[12]
        xor     ebp, eax
        ; esi = [ecx+edx].STATE[8] before change        
        shld    eax, esi, SFMT_SL2*8             ; 128-bit shift left
        xor     ebp, eax
        mov     eax, [RR1][12]
        shr     eax, SFMT_SR2*8                  ; 128-bit shift right
        xor     ebp, eax
        mov     eax, [RR2][12]
        mov     [RR1][12], eax                   ; r1 = r2
        shl     eax, SFMT_SL1                    ; 32-bit shifts left
        xor     ebp, eax
        mov     [RR2][12], ebp                   ; r2 = r
        mov     [ecx+edx].STATE[12], ebp         ; state[i] = r
        
        ; increment i, j
        add     ebx, 16
        cmp     ebx, SFMT_N*16
        jb      M4
        sub     ebx, SFMT_N*16                   ; modulo SFMT_N
M4:     add     edx, 16
        cmp     edx, SFMT_N*16
        jb      M1
        
        ; free r1, r2 from stack
        add     esp, 32
        pop     ebp
        pop     edi
        pop     esi
      ; pop     ebx
        jmp     L8

SFMT_Generate endp


;  extern "C" unsigned int SFMTBRandom(void * Pthis);                     // Output random bits

_SFMTBRandom PROC NEAR                 ; generate random bits
        mov     ecx, [esp+4]           ; Pthis
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomSFMTA

SFMTBRandom_reg  LABEL NEAR            ; Entry for register parameters, used internally

; if (ix >= SFMT_N*4) Generate();
        mov     edx, [ecx].IX
        cmp     edx, SFMT_N*16
        jnb     NeedGenerate
        
; y = ((uint32_t*)state)[ix++];
        mov     eax, dword ptr [ecx+edx].STATE
        add     edx, 4
        mov     [ecx].IX, edx

AfterGenerate:
; if (UseMother) y += MotherBits();
        cmp     [ecx].USEMOTHER, 0
        jz      NoMother
        
        ; add mother bits
        add     eax,  [ecx].M0              ; Add Mother random number        
        call    Mother_Next                 ; Make next Mother random number ready
        
NoMother: ; return y;
        ret
        
NeedGenerate: 
        call    SFMT_Generate          ; generate SFMT_N*4 random dwords
        mov     eax, [ecx].STATE
        mov     [ecx].IX, 4
        jmp     AfterGenerate
        
_SFMTBRandom ENDP


;  extern "C" double SFMTRandom  (void * Pthis); // Output random float

_SFMTRandom PROC NEAR                  ; generate random float with 52 bits resolution
        mov     ecx, [esp+4]           ; Pthis
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16
        
SFMTRandom_reg  label near             ; internal entry point        
assume  ecx: ptr CRandomSFMTA

; check if there are at least 64 random bits in state buffer
; if (ix >= SFMT_N*4-1) Generate();
        mov     edx, [ecx].IX
        cmp     edx, SFMT_N*16-4
        jnb     NeedGenerateR

        ; check instruction set
L1:     cmp     [ecx].Instset, 4
        jb      NoSSE2
        
        ; read 64 random bits
        movq    xmm0, qword ptr [ecx+edx].STATE
        add     edx, 8
        mov     [ecx].IX, edx

        ; combine with Mother-Of-All generator?
        cmp     [ecx].USEMOTHER, 0
        jz      ConvertToFloat
        
        ; add mother bits
        movq    xmm1, qword ptr [ecx].M0    ; Mother random number MC and M0
        pshuflw xmm1, xmm1, 01001011B       ; Put M0 before MC, and swap the words in MC
        paddq   xmm0, xmm1                  ; Add SFMT and Mother outputs
        call    Mother_Next                 ; Make next Mother random number ready
        
ConvertToFloat:
        psrlq	xmm0, 12			   ; align with mantissa field of double precision float
        movsd   xmm1, [ecx].one        ; 1.0 double precision
        por     xmm0, xmm1             ; insert exponent to get 1.0 <= x < 2.0
        subsd   xmm0, xmm1             ; subtract 1.0 to get 0.0 <= x < 1.0
        movsd   [ecx].TempRan, xmm0
        fld     [ecx].TempRan          ; transfer to st(0) register
        ret                            ; return value        
        
NeedGenerateR:
        call    SFMT_Generate          ; generate SFMT_N*4 random dwords
        xor     edx, edx
        jmp     L1

NoSSE2: ; Use old 386 instruction set:
        push    ebx
        ; read 64 random bits
        mov     eax, [ecx+edx].STATE
        mov     ebx, [ecx+edx+4].STATE
        add     edx, 8
        mov     [ecx].IX, edx

        ; combine with Mother-Of-All generator?
        cmp     [ecx].USEMOTHER, 0
        jz      ToFloatNoSSE2
        
        ; add mother bits
        mov     edx, [ecx].MC               ; Mother random number MC
        ror     edx, 16                     ; rotate
        add     eax, edx                    ; 64 bit add
        adc     ebx, [ecx].M0               ; Mother random number M0
        call    Mother_Next                 ; next Mother. eax, ebx unchanged
        
ToFloatNoSSE2:
        shrd    eax, ebx, 12		   ; align with mantissa field of double precision float
        shr     ebx, 12
        or      ebx, 3FF00000H         ; insert exponent to get 1.0 <= x < 2.0
        mov     dword ptr [ecx].TempRan, eax
        mov     dword ptr [ecx+4].TempRan, ebx
        fld     [ecx].TempRan          ; transfer to st(0) register
        fsub    [ecx].one              ; subtract 1.0 to get 0.0 <= x < 1.0
        pop     ebx
        ret                            ; return value        
        
_SFMTRandom ENDP


; extern "C" long double SFMTRandomL (void * Pthis);

_SFMTRandomL PROC NEAR                  ; generate random float with 63 bits resolution
        mov     ecx, [esp+4]           ; Pthis
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16
        
SFMTRandomL_reg  label near             ; internal entry point        
assume  ecx: ptr CRandomSFMTA

; check if there are at least 64 random bits in state buffer
; if (ix >= SFMT_N*4-1) Generate();
        mov     edx, [ecx].IX
        cmp     edx, SFMT_N*16-4
        jnb     NeedGenerateR

        ; check instruction set
L1:     cmp     [ecx].Instset, 4
        jb      NoSSE2
        
        ; read 64 random bits
        movq    xmm0, qword ptr [ecx+edx].STATE
        add     edx, 8
        mov     [ecx].IX, edx

        ; combine with Mother-Of-All generator?
        cmp     [ecx].USEMOTHER, 0
        jz      ConvertToFloat
        
        ; add mother bits
        movq    xmm1, qword ptr [ecx].M0    ; Mother random number MC and M0
        pshuflw xmm1, xmm1, 01001011B       ; Put M0 before MC, and swap the words in MC
        paddq   xmm0, xmm1                  ; Add SFMT and Mother outputs
        call    Mother_Next                 ; Make next Mother random number ready
        
ConvertToFloat:
        sub     esp, 16                     ; make space for long double
        psrlq	xmm0, 1                     ; align with mantissa field of long double
        pcmpeqw xmm1, xmm1                  ; all 1's
        psllq   xmm1, 63                    ; create a 1 in bit 63
        por     xmm0, xmm1                  ; bit 63 is always 1 in long double
        movq    qword ptr [esp], xmm0       ; store mantissa
L2:     mov     dword ptr [esp+8], 3FFFH    ; exponent
        fld     tbyte ptr [esp]             ; load long double
        fsub    [ecx].one                   ; subtract 1.0 to get 0.0 <= x < 1.0
        add     esp, 16
        ret                                 ; return value        
        
NeedGenerateR:
        call    SFMT_Generate          ; generate SFMT_N*4 random dwords
        xor     edx, edx
        jmp     L1

NoSSE2: ; Use old 386 instruction set:
        push    ebx
        ; read 64 random bits
        mov     eax, [ecx+edx].STATE
        mov     ebx, [ecx+edx+4].STATE
        add     edx, 8
        mov     [ecx].IX, edx

        ; combine with Mother-Of-All generator?
        cmp     [ecx].USEMOTHER, 0
        jz      ToFloatNoSSE2
        
        ; add mother bits
        mov     edx, [ecx].MC               ; Mother random number MC
        ror     edx, 16                     ; rotate
        add     eax, edx                    ; 64 bit add
        adc     ebx, [ecx].M0               ; Mother random number M0
        call    Mother_Next                 ; next Mother. eax, ebx unchanged
        
ToFloatNoSSE2:
        mov     edx, ebx                    ; now random bits are in edx:eax
        pop     ebx                         ; clean stack
        sub     esp, 16                     ; make room for long double
        shrd    eax, edx, 1                 ; align with mantissa field of long double
        stc
        rcr     edx, 1                      ; bit 63 is always 1
        mov     [esp], eax
        mov     [esp+4], edx
        jmp     L2                          ; the rest is the same as above
        
_SFMTRandomL ENDP


;  extern "C" int SFMTIRandom (void * Pthis, int min, int max);  // Output random integer

_SFMTIRandom PROC   NEAR
        mov     ecx, [esp+4]           ; Pthis
        ; Align by 16. Will overlap part of Fill if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomSFMTA
        call    SFMTBRandom_reg        ; random bits
        mov     edx, [esp+12]          ; max
        mov     ecx, [esp+8]           ; min
        sub     edx, ecx
        jl      short WrongInterval    ; max < min
        add     edx, 1                 ; max - min + 1
        mul     edx                    ; multiply random number by interval and truncate
        lea     eax, [edx+ecx]         ; add min
        ret
WrongInterval:
        mov     eax, 80000000H         ; error exit
        ret
_SFMTIRandom ENDP


;  extern "C" int SFMTIRandomX (void * Pthis, int min, int max); // Output random integer

_SFMTIRandomX   PROC   NEAR
        push    edi
        mov     ecx, [esp+8]           ; Pthis
        mov     edx, [esp+12]          ; min
        mov     edi, [esp+16]          ; max
        ; Align by 16. Will overlap part of Fill1 if Pthis unaligned        
        and     ecx, -16        
assume  ecx: ptr CRandomSFMTA

        sub     edi, edx               ; max - min
        jle     short M30              ; max <= min (signed)
        inc     edi                    ; interval = max - min + 1
        
        ; if (interval != LastInterval) {
        cmp     edi, [ecx].LASTINTERVAL
        je      M10
        ; need to calculate new rejection limit
        ; RLimit = uint32(((uint64)1 << 32) / interval) * interval - 1;}
        xor     eax, eax               ; 0
        lea     edx, [eax+1]           ; 1
        div     edi                    ; (would give overflow if interval = 1)
        mul     edi
        dec     eax
        mov     [ecx].RLIMIT, eax       
        mov     [ecx].LASTINTERVAL, edi
M10:
M20:    ; do { // Rejection loop
        call    SFMTBRandom_reg        ; random bits (ecx is preserved)
        ; longran  = (uint64)BRandom() * interval;
        mul     edi
        ; } while (remainder > RLimit);
        cmp     eax, [ecx].RLIMIT
        ja      M20
        
        ; return (int32)iran + min
        mov     eax, [esp+12]          ; min
        add     eax, edx
        pop     edi
        ret
        
M30:    jl      M40
        ; max = min. Return min
        mov     eax, edx
        pop     edi
        ret                            ; max = min exit
        
M40:    ; max < min: error
        mov     eax, 80000000H         ; error exit
        pop     edi
        ret
_SFMTIRandomX ENDP



; -------------------------------------------------------------------------
;  Single-threaded static link versions for SFMT generator
; -------------------------------------------------------------------------

;  extern "C" void SFMTgenRandomInit(int seed, int IncludeMother = 1); 
_SFMTgenRandomInit PROC NEAR
        mov     eax, [esp+4]           ; seed
        mov     edx, [esp+8]           ; IncludeMother
        LOADOFFSET2ECX SFMTInstance    ; Get address of SFMTInstance into ecx
        
        ; call _SFMTRandomInit with Pthis pointing to SFMTInstance
        push    edx					   ; IncludeMother
        push    eax					   ; seed
        push    type CRandomSFMTA	   ; ThisSize
        push    ecx					   ; Pthis
        call    _SFMTRandomInit
        add     esp, 16
        ret
_SFMTgenRandomInit ENDP


;  extern "C" void SFMTgenRandomInitByArray(int const seeds[], int NumSeeds, int IncludeMother = 1);
_SFMTgenRandomInitByArray PROC NEAR
        mov     eax, [esp+4]           ; seeds
        mov     ecx, [esp+8]           ; NumSeeds
        mov     edx, [esp+12]          ; IncludeMother
        push    edx
        push    ecx
        push    eax
        push    type CRandomSFMTA	   ; ThisSize
        LOADOFFSET2ECX SFMTInstance    ; Get address of SFMTInstance into ecx
        push    ecx
        call	_SFMTRandomInitByArray
        add     esp, 20
        ret
_SFMTgenRandomInitByArray ENDP  


;  extern "C" double SFMTgenRandom();
_SFMTgenRandom PROC NEAR               ; generate random float with 52 bits resolution
        LOADOFFSET2ECX SFMTInstance    ; Get address of SFMTInstance into ecx
        jmp     SFMTRandom_reg         ; random bits
_SFMTgenRandom ENDP


;  extern "C" double SFMTgenRandom();
_SFMTgenRandomL PROC NEAR               ; generate random float with 63 bits resolution
        LOADOFFSET2ECX SFMTInstance     ; Get address of SFMTInstance into ecx
        jmp     SFMTRandomL_reg         ; random bits
_SFMTgenRandomL ENDP


;  extern "C" int SFMTgenIRandom (int min, int max);
_SFMTgenIRandom PROC   NEAR
        mov     eax, [esp+4]                ; min
        mov     edx, [esp+8]                ; max
        LOADOFFSET2ECX SFMTInstance         ; Get address of SFMTInstance into ecx
        push    edx
        push    eax
        push    ecx                         ; Pthis
        call	_SFMTIRandom				; continue in _SFMTIRandom
        add     esp, 12
        ret
_SFMTgenIRandom ENDP


;  extern "C" int SFMTgenIRandomX (int min, int max);
_SFMTgenIRandomX PROC   NEAR
        mov     eax, [esp+4]                ; min
        mov     edx, [esp+8]                ; max
        LOADOFFSET2ECX SFMTInstance         ; Get address of SFMTInstance into ecx
        push    edx
        push    eax
        push    ecx                         ; Pthis
        call	_SFMTIRandomX               ; continue in _SFMTIRandomX
        add     esp, 12
        ret
_SFMTgenIRandomX ENDP


;  extern "C" uint32_t SFMTgenBRandom();
_SFMTgenBRandom PROC NEAR             ; generate random float with 32 bits resolution
        LOADOFFSET2ECX SFMTInstance    ; Get address of SFMTInstance into ecx
        jmp     SFMTBRandom_reg        ; random bits
_SFMTgenBRandom ENDP



IFDEF   WINDOWS
; -----------------------------------------------------------------
;  Single-threaded DLL versions for SFMT generator, Windows only
; -----------------------------------------------------------------

;  extern "C" void __stdcall SFMTgenRandomInitD(int seed, int IncludeMother = 1);
_SFMTgenRandomInitD@8 PROC NEAR
        mov     eax, [esp+4]           ; seed
        mov     edx, [esp+8]           ; IncludeMother
        push    edx
        push    eax
        push    type CRandomSFMTA	   ; ThisSize
        push    offset SFMTInstance	   ; Pthis
        call    _SFMTRandomInit
        add     esp, 16
        ret     8
_SFMTgenRandomInitD@8  ENDP



;  extern "C" void __stdcall SFMTgenRandomInitByArrayD
; (int const seeds[], int NumSeeds, int IncludeMother = 1);
_SFMTgenRandomInitByArrayD@12 PROC NEAR
        mov     eax, [esp+4]           ; seeds
        mov     ecx, [esp+8]           ; NumSeeds
        mov     edx, [esp+12]          ; IncludeMother
        push    edx
        push    ecx
        push    eax
        push    type CRandomSFMTA	   ; ThisSize
        push    offset SFMTInstance
        call	_SFMTRandomInitByArray
        add     esp, 20
        ret     12
_SFMTgenRandomInitByArrayD@12 ENDP        



;  extern "C" double __stdcall SFMTgenRandomD(); // Output random float
_SFMTgenRandomD@0 PROC NEAR           ; generate random float with 52 bits resolution
        mov     ecx, offset SFMTInstance
        jmp     SFMTRandom_reg        ; random bits
_SFMTgenRandomD@0 ENDP


;  extern "C" long double __stdcall SFMTgenRandomLD();
_SFMTgenRandomLD@0 PROC NEAR           ; generate random float with 63 bits resolution
        mov     ecx, offset SFMTInstance
        jmp     SFMTRandomL_reg       ; random bits
_SFMTgenRandomLD@0 ENDP


;  extern "C" int __stdcall SFMTgenIRandomD (int min, int max);
_SFMTgenIRandomD@8 PROC   NEAR
        mov     eax, [esp+4]                ; min
        mov     edx, [esp+8]                ; max
        push    edx
        push    eax
        push    offset SFMTInstance
        call	_SFMTIRandom				; continue in _SFMTIRandom
        add     esp, 12
        ret     8
_SFMTgenIRandomD@8 ENDP


;  extern "C" int __stdcall SFMTgenIRandomD (int min, int max);
_SFMTgenIRandomXD@8 PROC   NEAR
        mov     eax, [esp+4]                ; min
        mov     edx, [esp+8]                ; max
        push    edx
        push    eax
        push    offset SFMTInstance
        call	_SFMTIRandomX               ; continue in _SFMTIRandom
        add     esp, 12
        ret     8
_SFMTgenIRandomXD@8 ENDP



;  extern "C" int __stdcall SFMTgenIRandomDX (int min, int max);
_SFMTgenIRandomDX@8 PROC   NEAR
        mov     eax, [esp+4]                ; min
        mov     edx, [esp+8]                ; max
        push    edx
        push    eax
        push    offset SFMTInstance
        call	_SFMTIRandomX				; continue in _SFMTIRandomX
        add     esp, 12
        ret     8
_SFMTgenIRandomDX@8 ENDP


;  extern "C" unsigned int __stdcall SFMTgenBRandomD();
_SFMTgenBRandomD@0 PROC NEAR          ; generate random float with 32 bits resolution
        mov     ecx, offset SFMTInstance
        jmp     SFMTBRandom_reg        ; random bits
_SFMTgenBRandomD@0 ENDP

ENDIF  ; WINDOWS

IFDEF   POSITIONINDEPENDENT
get_thunk_ecx: ; load caller address into ecx for position-independent code
        mov ecx, [esp]
        ret

get_thunk_edi: ; load caller address into edi for position-independent code
        mov edi, [esp]
        ret
ENDIF   ; POSITIONINDEPENDENT

END
