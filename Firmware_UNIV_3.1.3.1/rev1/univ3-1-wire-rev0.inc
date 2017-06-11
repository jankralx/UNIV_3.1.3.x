	LIST
;==============================================================================
;   HAPCAN - Home Automation Project Firmware (http://hapcan.com)
;   Copyright (C) 2013 hapcan.com
;
;   This program is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program.  If not, see <http://www.gnu.org/licenses/>.
;==============================================================================
;   Filename:               univ3-1-wire-rev0.inc
;   Associated diagram:     none
;   Author:                 Jacek Siwilo, based on Dallas Semiconductor
;   Description:            UNIV 3 CPU 1-wire functions		
;==============================================================================
;   Revision History
;   Rev:  Date:     Details:
;   0     08.2012   Original version									
;==============================================================================
;   Description:
;   Processor has 8MHz clock and 0.5탎 per instruction cycle.
;   TRISDQ      - 1-wire port (TRIS reg)
;   PORTDQ      - 1-wire port (PORT reg)
;   DQ          - 1-wire pin
;   ROMCODE0    - first byte of 64bit rom code
;   TMPO        - time counter reg
;   PDBYTE      - presence reg
;   COUNT       - counter reg
;   COUNT2      - counter reg 2
;   IOBYTE      - input/output byte reg
;   IOBYTE2     - temp input/output byte reg
;   CRCREG      - CRC register
;==============================================================================


; These constants are standard 1-Wire ROM commands

		constant SRCHROM=0xF0
		constant RDROM=0x33
 		constant MTCHROM=0x55
		constant SKPROM=0xCC

;------------------------------------------------------------------------------
;       Dallas Semiconductor 1-Wire MACROS
;------------------------------------------------------------------------------
OW_HIZ:MACRO
		bsf         TRISDQ, DQ					; Make DQ pin High Z
		ENDM
;-------------------------------
OW_LO:MACRO
		bcf         PORTDQ, DQ                  ; Clear the DQ bit
		bcf         TRISDQ, DQ                  ; Make DQ pin an output
		ENDM
;-------------------------------
WAIT:MACRO TIME
;Delay for TIME 탎.
;Variable time must be in multiples of 5탎.
        nop
        nop
		movlw       (TIME/5)-1                  ;1탎
        nop
		movwf       TMP0                        ;1탎
        nop
		call        Wait5U                      ;2탎
		ENDM

;------------------------------------------------------------------------------
;       Dallas Semiconductor 1-Wire ROUTINES
;------------------------------------------------------------------------------
Wait5U:
;This takes 5킪 to complete
        nop
        nop
        nop
		nop                                     ;1탎
        nop
		nop                                     ;1탎
        nop
		decfsz      TMP0,F                      ;0.5탎 or 1탎
		goto        Wait5U                      ;1탎
		retlw 0                                 ;1탎
;-------------------------------
DSReset:
		OW_HIZ                                  ; Start with the line high
		clrf 		PDBYTE                      ; Clear the PD byte
		OW_LO
		WAIT        .500                        ; Drive Low for 500탎
		OW_HIZ
		WAIT        .70                         ; Release line and wait 70탎 for PD Pulse
		btfss       PORTDQ,DQ                   ; Read for a PD Pulse
		incf        PDBYTE,F                    ; Set PDBYTE to 1 if get a PD Pulse
		WAIT        .400                        ; Wait 400탎 after PD Pulse
		retlw 0
;-------------------------------
DSRXByte:										 ; Byte read is stored in IOBYTE
		movlw       .8
		movwf       COUNT                       ; Set COUNT equal to 8 to count the bits
DSRXLP:
		OW_LO
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop                                     ; Bring DQ low for 6탎
		OW_HIZ
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop                                     ; Change to HiZ and Wait 4탎
		bcf			STATUS,C
		btfsc		PORTDQ,DQ
		bsf			STATUS,C
		rrcf        IOBYTE,F                    ; Shift C into IOBYTE
		WAIT        .50                         ; Wait 50탎 to end of time slot
		decfsz      COUNT,F                     ; Decrement the bit counter
		goto        DSRXLP
		retlw       0
;-------------------------------
DSTXByte:                                       ; Byte to send starts in IOBYTE
		movwf       IOBYTE                      ; We send it from IOBYTE
		movlw       .8
		movwf       COUNT                       ; Set COUNT equal to 8 to count the bits
DSTXLP:
		OW_LO
		nop
		nop
		nop
		nop
		nop
		nop                                     ; Drive the line low for 3탎
		rrcf        IOBYTE,F
		btfsc       STATUS,C                    ; Check the LSB of IOBYTE for 1 or 0
		bsf         TRISDQ,DQ                   ; HiZ the line  if LSB is 1
      	WAIT        .60                         ; Continue driving line for 60탎
      	OW_HIZ                                  ; Release the line for pullup
      	nop
		nop
		nop
      	nop                                     ; Recovery time of 2탎
      	decfsz      COUNT,F                     ; Decrement the bit counter
      	goto        DSTXLP
      	retlw       0
;-------------------------------
CalcCrc:
		movlw		.8
		movwf		COUNT
CalcCrcLoop:
		movf		CRCREG,W					;find D7 bit
		xorwf		IOBYTE,W
		rrcf		WREG						;get D7 bit
		bnc		D7Zero							;invert bit  <4:3> if D7=1
		movlw		b'00011000'
		xorwf		CRCREG,F
D7Zero
		rrcf		CRCREG						;done CRCREG
		rrncf		IOBYTE						;take next bit
		decfsz		COUNT
		bra		CalcCrcLoop
		retlw		0
;-------------------------------
ReadBit											;bit is written to 7th bit of IOBYTE
		OW_LO
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop                                     ; Bring DQ low for 6탎
		OW_HIZ
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop                                     ; Change to HiZ and Wait 4탎
		bcf			STATUS,C
		btfsc		PORTDQ,DQ
		bsf			STATUS,C
		clrf		IOBYTE
		rrcf        IOBYTE,F                    ; Shift C into IOBYTE
		WAIT        .50                         ; Wait 50탎 to end of time slot
		retlw		0 
;-------------------------------
SendBit0
		OW_LO
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop                                     ; Drive the line low for 5탎
		nop
		nop
      	WAIT        .60                         ; Continue driving line for 60탎
      	OW_HIZ                                  ; Release the line for pullup
      	nop
      	nop
		nop
		nop
		retlw		0 
;------------------------------- 
SendBit1
		OW_LO
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop                                     ; Drive the line low for 5탎
		nop
		nop
		bsf         TRISDQ,DQ                   ; HiZ the line  if LSB is 1
      	WAIT        .60                         ; Continue driving line for 60탎
      	OW_HIZ                                  ; Release the line for pullup
      	nop
      	nop
      	nop
      	nop  
		retlw		0 
;-------------------------------
CheckROM:										;checks if there is only one device on network
		movlw       SRCHROM                  	;send Search ROM Command (0xF0)
		call        DSTXByte
		;read device
		movlw		.64
		movwf		COUNT
CheckROMLoop:
		;read 1 bit
		call		ReadBit
		movff		IOBYTE,IOBYTE2
		;read 2 bit
		call		ReadBit
		movf		IOBYTE,W
		cpfseq		IOBYTE2
		bra			$ + 4
	bra			ManyDevices
		btfsc		IOBYTE2,7					;check
		bra			$ + 8
		call		SendBit0
		bra			$ + 6
		call		SendBit1
		decfsz		COUNT
	bra			CheckROMLoop
		retlw		0
ManyDevices:
		retlw		1
;-------------------------------
ReadROM:									    ;reads 64 bit ROM CODE (returns 0 if ok or 1 if CRC problem)
		movlw       RDROM                  	    ;Send Search ROM Command (0x33)
		call        DSTXByte
		clrf		CRCREG					    ;init CRC
		;read device
		movlw		.8
		movwf		COUNT2
		lfsr		FSR0,ROMCODE0
ReadROMLoop:		
		call        DSRXByte           		    ;ROMCODE0-7
		call		CalcCrc
		movff       IOBYTE,POSTINC0                        
		decfsz		COUNT2
	bra			ReadROMLoop
		tstfsz		CRCREG					    ;test if CRC correct
	bra			$ + 4
		retlw		0						    ;CRC ok
		retlw		1					        ;wrong CRC