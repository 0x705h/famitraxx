;----------------------------------------------------------------
; famitraxx - header.asm
;----------------------------------------------------------------
; this rom uses CNROM / iNES mapper 3
;----------------------------------------------------------------
; Memory map 
;----------------------------------------------------------------
; $0000-0800 - Internal RAM, 2KB chip in the NES
; $2000-2007 - PPU access ports
; $4000-4017 - Audio and controller access ports
; $6000-7FFF - Optional WRAM inside the game cart
; $8000-FFFF - Game cart ROM
;----------------------------------------------------------------
; constants
;----------------------------------------------------------------

PRG_COUNT = 2 ;1 = 16KB, 2 = 32KB
CHR_COUNT = 4 ;1 = 8KB, 2 = 16KB, 4 = 32KB
MIRRORING = %0001 ;%0000 = horizontal, %0001 = vertical, %1000 = four-screen

CHRBANK1 = $00
CHRBANK2 = $01
CHRBANK3 = $02
CHRBANK4 = $03

;----------------------------------------------------------------
; iNES header
; recommended order of rom organization
; in http://forums.nesdev.com/viewtopic.php?t=6160
; iNES header -> define variables -> code 
;			  -> data tables -> interrupt addresses -> chr data.
;----------------------------------------------------------------

.db "NES",$1a		;iNES identifier

.db PRG_COUNT      	;number of PRG-ROM blocks

.db CHR_COUNT      	;number of CHR-ROM blocks

.db $30|MIRRORING  	;mapper 3, mirroring

.db $00,$00,$00,$00,$00,$00,$00,$00,$00	;filler

;PRG-ROM

;----------------------------------------------------------------
; FamiTone settings
;----------------------------------------------------------------

FT_BASE_ADR		= $0300	;page in RAM, should be $xx00
FT_TEMP			= $00	;7 bytes in zeropage
FT_DPCM_OFF		= $c000	;$c000 or higher, 64-byte steps
FT_SFX_STREAMS	= 4		;number of sound effects played at once, can be 4 or less
FT_DPCM_PTR		= (FT_DPCM_OFF&$3fff)>>6

;FT_DPCM_ENABLE			;undefine to exclude all the DMC code
;FT_SFX_ENABLE			;undefine to exclude all the sound effects code
FT_THREAD				;undefine if you call sound effects in the same thread as sound update


;----------------------------------------------------------------
; Includes
;----------------------------------------------------------------


.include "main.asm"		;Famitraxx main code
.include "famitone.asm"	;FamiTone audio driver by Shiru


;----------------------------------------------------------------
; Vectors
;----------------------------------------------------------------

.pad $fffa				;fill any remaining space with zeroes
.dw NMI					;set interrupt addresses (defined in main.asm)
.dw Reset
.dw IRQ

;----------------------------------------------------------------
; CHR-ROM 4 banks
;----------------------------------------------------------------

.incbin "basic.chr"
.incbin "tileset.chr"
.incbin "basic.chr"
.incbin "tileset.chr"