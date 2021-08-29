;
; rgb-led-driver-asm.asm
;
; Created: 27.03.2018
; Author : Alexandr Kolodkin  <alexandr.kolodkin@gmail.com>
;

.equ LED_R         = PORTB0
.equ LED_G         = PORTB1
.equ LED_B         = PORTB2
.equ LED_S         = PORTB3
.equ RXD           = PORTB4

.equ F_CPU         = 9600000
.equ BAUDRATE      = 115200


.equ START_CHAR    = ':'
.equ UART_DELAY    = 19

.def next          = r10
.def pos           = r11

.def rx_end        = r22
.def rx_byte       = r23
.def bitcounter    = r25

;;;;;;;;;;;;;;;;;;;;;;;;;;;; GLOBALS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.DSEG
.ORG SRAM_START

_buffer:	.byte 8
_count:     .byte 1
_red:       .byte 1
_green:     .byte 1
_blue:      .byte 1
_checksumm: .byte 1
_next:		.byte 1
_rx_flag:   .byte 1
_rx_buffer: .byte 9


;;;;;;;;;;;;;;;;;;;;;;; INTERRUPT VECTORS ;;;;;;;;;;;;;;;;;;;;;;;
.CSEG
.ORG 0x0000
	rjmp start                ; Reset vector
	reti                      ; External Interrupt INT0
	rjmp PCINT0_handler       ; External Interrupt Request PCINT0
	rjmp TIM0_OVF_handler     ; Timer/Counter0 Overflow
	reti                      ; EEPROM Ready
	reti                      ; Analog Comparator
	rjmp TIM0_COMPA_handler   ; Timer/Counter Compare Match A
	reti                      ; Timer/Counter Compare Match B
	reti                      ; Watchdog Time-out
	reti                      ; ADC Conversion Complete

;;;;;;;;;;;;;;;;;;;;;;;;;;;;; CONST ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_ocr0a: .db 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x7F
	

;;;;;;;;;;;;;;;;;;;;;;;;;; ENTRY POINT ;;;;;;;;;;;;;;;;;;;;;;;;;;
start:
	clr  r0
	out  SREG, R1              ; reset SREG
	ldi  r28, (SRAM_START + SRAM_SIZE - 1)
	out  SPL, r28              ; set stack pointer

	ldi  r30, LOW(_red)        ; z = &_red
	ldi  r28, 0xAA             ;
	st   z+, r28               ; _red = 0xAA
	ldi  r28, 0x55             ;
	st   z+, r28               ; _green = 0x55
	ldi  r28, 1                ;
	st   z, r28                ; _blue = 0x04
	
	rcall update

	; Setup gpio
	ldi  r28, 0x0F             ; set PB1 - PB4
	out  DDRB, r28             ;   as output

	; Setup rx interrupt
	ldi  r28, (1<<PCIE)
	out  GIMSK, r28
	ldi  r28, (1<<RXD)
	out  PCMSK, r28

	; Stop timer
	ldi  r26, (1<<TSM)
	out  GTCCR, r26

	; Setup timer
	out  TCCR0A, r0            
	ldi  r28, (1<<CS02)
	out  TCCR0B, r28
	out  OCR0A, r0
	out  OCR0B, r0
	ldi  r28, 0xFF
	out  TCNT0, r28
	ldi  r28, (1<<TOIE0) | (1<<OCIE0A)
	out  TIMSK0, r28

	; Enable interrupts
	sei

	; Start timer
	out  GTCCR, r0

;;;;;;;;;;;;;;;;;;;;;;;;;;; MAIN LOOP ;;;;;;;;;;;;;;;;;;;;;;;;;;;
loop:


    rjmp loop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;; UPDATE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.MACRO update_bit                   ; Start macro definition 
	clr  r16                        ; temp = 0
	sbrc r17, @0                    ; skip if _red & mask
	sbr  r16, (1<<LED_R)            ; set red led pin	 
	sbrc r18, @0                    ; skip if _red & mask
	sbr  r16, (1<<LED_G)            ; set red led pin	 
	sbrc r19, @0                    ; skip if _red & mask
	sbr  r16, (1<<LED_B)            ; set red led pin
	st   z+, r16
.ENDMACRO                           ; End macro definition


update:
	push r16
	in   r16, SREG                  ; store SREG
	cli                             ; disable interrupts
	push r16
	push r17
	push r18
	push r19
	push r30

	ldi  r30, LOW(_buffer)          ; z    = _buffer

	ldd  r17, z + 9                 ; r17  = _red
	ldd  r18, z + 10                ; r18  = _green
	ldd  r19, z + 11                ; r19  = _red

	update_bit 7
	update_bit 6
	update_bit 5
	update_bit 4
	update_bit 3
	update_bit 2
	update_bit 1
	update_bit 0

	sbr  r16, LED_S                 ; next = LED_S | _buffer[7]
	mov  next, r16                  ;  

	ldi  r16, 100                   ; _count = 100
	st   z, r16                     ;

	ldi  r16, 7                     ; pos = 7
	mov  pos, r16                   ;
	
	ldi  r26, (1<<TSM)              ; Stop timer
	out  GTCCR, r26                 ;
	ldi  r16, 0xFF                  ;
	out  TCNT0, r16                 ; TCNT0 = 0xFF
;	out  OCR0A, r16                 ; OCR0A = 0xFF	
	ldi  r16, (1<<OCF0A)            ; TIFR0 = _BV(OCF0A)
	out  TIFR0, r16                 ;
	clr  r16
	out  GTCCR, r16                 ; Start timer
	
	; restore used registers
	pop  r30
	pop  r19
	pop  r18
	pop  r17
	pop  r16
	out  SREG, r16
	pop  r16
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;; SUBROUTINES ;;;;;;;;;;;;;;;;;;;;;;;;;;
set_next:
	ld   r16, -z
	mov  next, r16
append_status:
	lds  r16, _count                ; r16 = _counttst  r16                        ; 
	tst  r16
	breq skip_led_s
	ldi  r16, (1<<LED_S)            ; r16 = _BV(LED_S)
	or   next, r16; 
skip_led_s:
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;; TIM0_COMPA ;;;;;;;;;;;;;;;;;;;;;;;;;;
TIM0_COMPA_handler:
	out  PORTB, next

	; store used registers
	push r16
	in   r16, SREG
	push r16
	push r30
	push r31
	push r0

	ldi  r16, 7                     ; sizeof(_buffer)
	dec  pos                        ;
	and  pos, r16                   ;

	; next = (_count) ? _buffer[_pos] | _BV(LED_S) : _buffer[_pos]
	ldi  r30, LOW(_buffer)          ; r30 = _buffer
	add  r30, pos                   ; r30 = _buffer + pos
	ld   next, z                    ; next = _buffer[pos]
	rcall append_status
	
	; OCR0A = _BV(7 - pos) - 1
	ldi  r30, LOW(_ocr0a<<1)        ;
	clr  r31                        ;
	add  r30, pos                   ;
	lpm                             ; r0 = _BV(7-pos) - 1
	out  OCR0A, r0                  ;

	; restore used registers
	pop  r0
	pop  r31
	pop  r30
	pop  r16
	out  SREG, r16
	pop  r16

	reti


;;;;;;;;;;;;;;;;;;;;;;;;;;;; TIM0_OVF ;;;;;;;;;;;;;;;;;;;;;;;;;;;
TIM0_OVF_handler:
	
	; store used registers
	push r16
	in   r16, SREG
	push r16
	push r30

	out  PORTB, next                ; next = _buffer[7] + status   (bit 0)

	ldi  r30, LOW(_buffer)+7        ; z = _buffer + sizeof(_buffer) - 1
	rcall set_next

b0:	in   r16, TCNT0                 ; while (TCNT<1);
	tst  r16                        ;
	breq b0                         ;

	out  PORTB, next                ; next = _buffer[6] + status   (bit 1)
	rcall set_next                  ; 

b1:	in   r16, TCNT0                 ; while (TCNT<1);
	cpi  r16, 3                     ;
	brcs b1                         ;

	out  PORTB, next                ; next = _buffer[5] + status   (bit 2)
	rcall set_next

b2:	in   r16, TCNT0                 ; while (TCNT<1);
	cpi  r16, 7                     ;
	brcs b2                         ;

	out  PORTB, next                ; next = _buffer[4] + status   (bit 3)
	rcall set_next

	ldi  r16, 15                    ; OCR0A = 15
	out  OCR0A, r16                 ;

	ldi  r16, 3                     ; pos = 3
	mov  pos, r16                   ;

	ldi  r16, (1<<OCF0A)            ; TIFR0 = _BV(OCF0A)
	out  TIFR0, r16                 ;

	lds  r16, LOW(_count)           ; if (status_count) status_count--;
	tst  r16                        ;
	breq skip_decrement             ;
	dec  r16                        ;
	sts  LOW(_count), r16           ;
skip_decrement:

	; restore used registers
	pop  r30
	pop  r16
	out  SREG, r16
	pop  r16

	reti


;;;;;;;;;;;;;;;;;;;;;;;;;;;;; PCINT0 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PCINT0_handler:

	; Skip if RXD is up (whait for start)
	sbic PINB, RXD
	reti

	push r0				            ;
	push r1				            ; 
	in   r0, SREG                   ; Store SREG
	push r0

	push r18	                    ; Store used registers
	push r30	                    ; Store Z

	ldi  r30, LOW(_rx_buffer)       ; z      = &_buffer[0]
	ldi  rx_end, LOW(_rx_buffer) + 9; rx_end = &_buffer[9]

	ldi  r18, 5                    ; delay for 1/2 of uart bit period
delay1:         
	dec  r18
	brne delay1
	nop

get_byte:
	ldi  bitcounter, 9              ; 8 data bits + 1 start bit

get_bit:
	ldi  r18, 83                    ; delay for 1 of uart bit period
delay2:
	dec  r18
	brne delay2

	clc
	sbic PINB, RXD
	sec
	ror  rx_byte

	nop
	nop
	nop
	nop

	dec  bitcounter
	breq byte_received

	rjmp get_bit

byte_received:
	st   z+, rx_byte

	cp   r30, rx_end
	breq data_reseived

wait_next_start:
	sbic PINB, RXD
	rjmp wait_next_start
	rjmp get_byte

data_reseived:

	rcall update
							        
	pop  r30                        ; Restore Z
	pop  r29                        ; Restore r19
	pop  r18                        ; Restore r18
	pop  r0                         ; Restore SREG
	out  0x3F, r0                   ; Restore old SREG state
	pop  r1                         ; Restore r1
	pop  r0                         ; Restore r0
	reti                            ; Exit Interrupt


