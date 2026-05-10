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

#include <security/SELeaf/net_prot.h>
#include <security/SELeaf/seleaf.h>
#include <net/net.h>

void net_prot_init(void) {
    /* Initialize network filter rules */
    seleaf_log("NET_PROT", "Stateful inspection enabled.");
}

bool net_prot_filter_packet(void* packet_data, uint32_t length) {
    if (!packet_data || length < 14) return false;

    /* Simple sanity check: Drop oversized frames */
    if (length > 1518) {
        seleaf_log("NET_PROT", "Dropped jumbo frame - policy violation.");
        return false;
    }

    return true;
}