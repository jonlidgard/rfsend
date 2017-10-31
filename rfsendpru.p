
#define PRU0_ARM_INTERRUPT 19

.setcallreg r11.w0

.origin 0
.entrypoint RepeatLoop

/*
;.struct SignalDesc
;.u32 result   ;		0
;.u32 syncHigh ; r0 	4
;.u32 syncLow ; r1		8
;.u32 zeroHigh ; r2		12
;.u32 zeroLow ; r3		16
;.u32 oneHigh ; r4		20
;.u32 oneLow ; r5		24
;.u32 command ; r6		28
;.u8 length ; r7.b0		32
;.u8 repeats ; r7.b1	33
;.u8 inverted ; r7.b2	34
;.u8 spare				35
;.ends
 
;.assign SignalDesc R0, R7, signal
*/

RepeatLoop:
  MOV r8, 0
  LBBO r0, r8, 4, 32 ; load 8 32bit words into registers
  SUB r8, r7.b0, 1 ; r8 is length
  MOV R11.w2, SendHighLowPulse
  QBEQ SendLoop, R7.b2, 0
  MOV R11.w2, SendLowHighPulse  
SendLoop:
  QBBC ZeroBit, r6, r8
  ADD r9, r4, 0
  ADD r10, r5, 0
  CALL R11.w2
  QBA NextBit 
ZeroBit:
  ADD r9, r2, 0
  ADD r10, r3, 0
  CALL R11.w2
  QBA NextBit 
NextBit:
  SUB r8, r8, 1
  QBNE SendLoop, r8.b0, 0xff ; r8 < 0
  ADD r9, r0, 0
  ADD R10, r1, 0
  CALL R11.w2
  SUB r7.b1, r7.b1, 1
  MOV r0, 0
  SBBO r7.b1, r0, 33, 1 
  QBNE RepeatLoop, r7.b1, 0
Finished:
  MOV r31.b0, PRU0_ARM_INTERRUPT+16   
  HALT 

SendHighLowPulse:
  SET r30, r30, 15 ; set GPIO output 15 
HighPulseDelay1:
  SUB r9, r9, 1
  QBNE HighPulseDelay1, r9, 0
  CLR r30, r30, 15 ; clear GPIO output 15 
LowPulseDelay1:
  SUB r10, r10, 1
  QBNE LowPulseDelay1, r10, 0
  RET

SendLowHighPulse:
  CLR r30, r30, 15 ; clear GPIO output 15 
LowPulseDelay2:
  SUB r9, r9, 1
  QBNE LowPulseDelay2, r9, 0
  SET r30, r30, 15 ; set GPIO output 15 
HighPulseDelay2:
  SUB r10, r10, 1
  QBNE HighPulseDelay2, r10, 0
  RET