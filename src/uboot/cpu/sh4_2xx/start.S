/*
 * (C) Copyright 2004 STMicroelectronics.
 *
 * Andy Sturges <andy.sturges@st.com>
 *	
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include "asm/regdef.h"
#include "asm/hardware.h"
#include "asm/asmdefs.h"

	.global _start
_start:
	/* Set SR:	MD=1, RB=0, BL=0, IMASK=0xF */
	MOV_CONST32_R0 0x400000F0 /* 0x400080F0 */ 
	ldc	r0, sr
	MOV_CONST32_R0 0x00080001 /* 0x400080F0 */ 
	lds	r0, fpscr

	/* Disable watchdog timer */
	MOV_CONST32_R0  CPG_WTCSR 
	mov     r0, r1
	MOV_CONST16_R0 0xA500
	mov.w   r0, @r1
		
	/* disable Caches */
	MOV_CONST16_R0 (CCR_OCE|CCR_ICE|CCR_EMODE)
	mov	r0, a0
	
	CALL	sh_cache_clear_op_offset

	CALL	init_ram_offset

	/* clear caches & enable icache & dcache write through */
	MOV_CONST16_R0 (CCR_OCI|CCR_ICI|CCR_ICE|CCR_OCE|CCR_EMODE|CCR_WT)
	mov	r0, a0
	CALL	sh_cache_set_op_offset

	/*
	 * relocate code if we need to
	 * 
	 */
	
	/* set up P2 mask  */
	MOV_CONST32_R0 0X20000000
	mov	r0, r4		
	mova	_start_offset, r0	/* Load source address in r1		*/
	mov	r0, r1
	or	r4, r1
	mov.l	@r1, r2
	sub	r2, r1
	
	mov.l	target_addr, r2		/* Load target address	in r2		*/
	or	r4, r2			/* make sure its P2			*/
	cmp/eq	r1, r2			/* Are we in correct place already ?	*/
	bt	init_sp

	/* relocate code: r1 = source, r2 = dest, r3 dest end */
	mov.l	bss_start, r3
	or	r4, r3
1:	mov.w	@r1+, r0
	mov.w	r0, @r2
	add	#2, r2
	cmp/hi	r2,r3
	bt	1b

	/* init stack pointer */
init_sp:	
	mov.l	stack_addr, sp  

	/* Clear bss */	
init_bss:	
	mov.l	bss_start, r1
	add	#4, r1
	mov.l	bss_end, r2
	mov	#0, r0
1:	cmp/hs	r2, r1
	bf/s	1b		! while (r1 < r2)
	mov.l	r0,@-r2

/* Load address of board init routine */		
	mov.l	start_sh4boot_addr, r1  
	jmp	@r1        /*  never returns     */
	nop

	/* Constants used above */

.balign 4
_start_offset:	.long . - _start	
target_addr:	.long TEXT_BASE 
stack_addr:	.long TEXT_BASE - (CFG_GBL_DATA_SIZE + CFG_MALLOC_LEN + CFG_BOOTPARAMS_LEN)
bss_start:	.long __bss_start 
bss_end:	.long __bss_end

start_sh4boot_addr:.long start_sh4boot

	/* offsets for PIC calls */

	.global sh_cache_clear_op_offset
			
init_ram_offset:		.long init_ram - .
sh_cache_clear_op_offset:	.long sh_cache_clear_op - .
sh_cache_set_op_offset:		.long sh_cache_set_op - .
	
TRA     = 0xff000020
EXPEVT  = 0xff000024
INTEVT  = 0xff000028
MMU_TEA = 0xff00000c       

! Exception Vector Base
!
!	Should be aligned page boundary.
!
	.balign 	4096,0,4096
vbr_base:
	.long	0
!
	.balign 	256,0,256
general_exception:

	.balign 	1024,0,1024
tlb_miss:

	.balign 	512,0,512
interrupt:

