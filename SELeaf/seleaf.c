
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

#include <security/SELeaf/seleaf.h>
#include <security/SELeaf/kernel_prot.h>
#include <security/SELeaf/driver_prot.h>
#include <security/SELeaf/fs_prot.h>
#include <security/certs.h>
#include <vga.h>

static bool enforcing = true;

void seleaf_init(void) {
    vga_puts("[SELeaf] Initializing security subsystem...\n");
    certs_init();
    
    kernel_prot_init();
    driver_prot_init();
    fs_prot_init();
    
    vga_puts("[SELeaf] All security modules active.\n");
}

bool seleaf_is_enforcing(void) {
    return enforcing;
}

void seleaf_log(const char* module, const char* message) {
    vga_puts("[SELeaf:");
    vga_puts(module);
    vga_puts("] ");
    vga_puts(message);
    vga_puts("\n");
}