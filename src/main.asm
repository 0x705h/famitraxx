;----------------------------------------------------------------
; famitraxx - main.asm
; http://www.github.com/alekhin0w/famitraxx
;----------------------------------------------------------------
; Main Variables
;----------------------------------------------------------------

scroll_h = $a0
scroll_v = $a1
frame_counter = $a2

;----------------------------------------------------------------
; program bank(s)
;----------------------------------------------------------------

   .base $10000-(PRG_COUNT*$4000)

;----------------------------------------------------------------
; Reset: init code
; this happens every time NES starts up 
; or reset button is pressed
;----------------------------------------------------------------

Reset:
	sei	; disable irq
	cld	; disable decimal mode
	ldx #$00
	stx $2000
	stx $2001	; turn off screen
	dex
	txs
	ldx #0
	txa
clearMem:
	sta 0, x
	sta $100, x
	sta $200, x
	sta $300, x
	sta $400, x
	sta $500, x
	sta $600, x
	sta $700, x
	sta $800, x
	sta $900, x
	inx
	bne clearMem

	lda #CHRBANK1	; select the first chr bank defined in header.asm
	jsr bankswitch 	; and switch to it!

;----------------------------------------------------------------
; Set Variables in memory
;----------------------------------------------------------------
	lda #$00

	sta scroll_h
	sta scroll_v
	sta frame_counter

;----------------------------------------------------------------

	ldx #$02
WarmUp:
	bit $2002		; PPUSTATUS check vblank 0: not in vblank, 1: vblank (bpl looks at bit 7)
	bpl WarmUp		; if not in vblank, loop.
	dex
	bne WarmUp		; loop 2 times more to sync

	lda #$3F		; PPUADDR writing VRAM address to access
	sta $2006		; upper byte first
	lda #$00
	sta $2006

;----------------------------------------------------------------
; Load palette
;----------------------------------------------------------------
	ldx	#$00
load_pal:                     
	lda palette,x
	sta $2007
	inx
	cpx #$20
	bne load_pal

	lda #$20
	sta $2006
	lda #$00
	sta $2006

;----------------------------------------------------------------
; Wupe out nametables
;----------------------------------------------------------------

	ldy #$04                
wipeNametables:
	ldx #$00
	lda #$00
wipeppuloop:
	sta $2007	; PPUDATA
	dex
	bne wipeppuloop

	dey
	bne wipeNametables

;----------------------------------------------------------------
; Music setup
;----------------------------------------------------------------

music_setup:
	jmp included_music
		.include "music.asm"		;sample music data (ref to music_module label)	
		;.include "music_dpcm.asm"	;dpcm code
included_music:		
	lda #$00 					; PAL music
	jsr FamiToneInit
	
	;ldx #<music_dpcm			;set DPCM sample table location
	;ldy #>music_dpcm
	;jsr FamiToneSampleInit


music_start:
	ldx #<music_module			; x,y holds 16-bit address where music code is.
	ldy #>music_module
	jsr FamiToneMusicStart


;----------------------------------------------------------------
; Text on screen
;----------------------------------------------------------------

		;bit $2002	; read ppu_status to reset address latch

		lda #$20
		sta $2006
		lda #$82
		sta $2006
		ldx #$00

	write_message:
		lda message,x
		cmp #$0d
		beq end_write_message
		sta $2007
		inx
		jmp write_message
	end_write_message:	

	lda #%00011110		; enable video
	sta $2001

	lda #%10001000		; generate nmi + sprite pattern table
	sta $2000			; PPUCTRL
	lda #%00001110		; show left background, sprites, background
	sta $2001			; PPUMASK

	jsr vblankwait      ; last vblankwait before loop

;----------------------------------------------------------------
; Main Loop
;----------------------------------------------------------------

die:
	jsr waitNMI
	
	;ldy frame_counter	; these three
	;ldx palette 		; lines will glitch
	;jsr glitch 		; PPU (must disable waitNMI in this loop)
	
	jmp die 	; ad infinitum

;----------------------------------------------------------------
; Glitch it!
;----------------------------------------------------------------

glitch:
	iny
	iny	
	iny
	inx
	sty palette
	sty $2007
	iny
	iny
	iny
	iny
	sty $2007
	sty $2007
	sty $2007

	iny
	iny
	iny
	iny
	iny
	sty $2007
	sty $2007

	rts
	
;----------------------------------------------------------------
; CHR bank switching routines for CNROM avoiding bus conflicts
;----------------------------------------------------------------

bankswitch:
	tax					; A register holds the bank number to use
	sta bankvalues, x	; doing this, we force the rom and the cpu
	rts 				; to give the same value looking up at the table
						; this way, we avoid bankswitching conflicts.
bankvalues:
	.db $00, $01, $02, $03	; bank values table, 4 CHR banks for CNROM

vblankwait:
	bit $2002
	bpl vblankwait

	rts	

waitNMI:
	lda frame_counter
@notYet:
	cmp frame_counter
	beq @notYet
	rts

;----------------------------------------------------------------
; Non-Maskable Interrupt code / runs every video frame
;----------------------------------------------------------------

NMI:
	inc frame_counter	; increment the frame counter
						; for waitNMI

	ldx scroll_h	; set position
	stx $2005		; of scroll
	ldx scroll_v	; managed by scroll_h/v vars.
	stx $2005

	jsr FamiToneUpdate
   
   rti

;----------------------------------------------------------------
; Non-Maskable Interrupt code / runs every video frame
;----------------------------------------------------------------

IRQ:
   rti


;----------------------------------------------------------------
; Text data
;----------------------------------------------------------------

message:
        .db "FAMITRAXX - BSAS ARG",$0D

;----------------------------------------------------------------
; Palette data
;----------------------------------------------------------------

palette:
	.db $0F,$00,$10,$30,$0F,$00,$10,$30,$0F,$00,$10,$30,$0F,$00,$10,$30

;----------------------------------------------------------------
; DPCM data
;----------------------------------------------------------------

;.org FT_DPCM_OFF
;.incbin "music_dpcm.bin"
