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

#ifndef _LEAFOS_FS_PROT_H
#define _LEAFOS_FS_PROT_H

#include <fs/vfs.h>
#include <stdbool.h>

void fs_prot_init(void);

/* Hooks into VFS operations */
bool fs_prot_check_access(fs_node_t* node, uint32_t mask);
bool fs_prot_is_immutable(fs_node_t* node);

#endif
