/*
 * LeafOS Kernel - Copyright (C) 2022-2026 [Predescu Ciprian]
 * 
 * 1. SHARE ALIKE: This program is free software. If you modify this 
 *    code and distribute it, you MUST provide the source code of your 
 *    changes under this same license. 
 * 
 * 2. NO WARRANTY: This program is distributed in the hope that it will 
 *    be useful, but WITHOUT ANY WARRANTY; without even the implied 
 *    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * 3. LIMITATION OF LIABILITY: IN NO EVENT SHALL THE AUTHOR BE LIABLE 
 *    FOR ANY DAMAGES (INCLUDING SYSTEM FAILURE, DATA LOSS, OR BRAKED 
 *    HARDWARE) ARISING OUT OF THE USE OF THIS KERNEL.
 *
 * Full License: GNU General Public License v3.0 <https://www.gnu.org/licenses/gpl-3.0.txt>
 */

#include <security/SELeaf/kernel_prot.h>
#include <security/SELeaf/seleaf.h>
#include <kernel.h>
#include <panic.h>

void kernel_prot_init(void) {
    /* Set up Write Protect (WP) in CR0 to prevent kernel code modification */
    kernel_prot_apply_readonly();
}

void kernel_prot_apply_readonly(void) {
    asm volatile("mov %cr0, %eax; or $0x00010000, %eax; mov %eax, %cr0");
}

/* GCC calls this if a stack smash is detected (requires -fstack-protector) */
void __stack_chk_fail(void) {
    seleaf_log("KERNEL", "CRITICAL: Stack smashing detected!");
    kernel_panic("Security Violation: Stack Buffer Overflow");
}