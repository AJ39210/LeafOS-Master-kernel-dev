/**
 * include/SELeaf/policy_jit.h
 */

#ifndef SELEAF_POLICY_JIT_H
#define SELEAF_POLICY_JIT_H

#include <stdint.h>

typedef enum {
    OP_NOP = 0,
    OP_LOAD_CAP,
    OP_LOAD_CONST,
    OP_COMPARE_EQ,
    OP_COMPARE_LT,
    OP_COMPARE_GT,
    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_ALLOW,
    OP_DENY,
    OP_AUDIT,
    OP_RETURN
} bytecode_opcode_t;

typedef struct {
    bytecode_opcode_t opcode;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
} bytecode_instr_t;

int policy_jit_init(void);
uint32_t policy_jit_compile(const char *policy_name, const char *policy_rules);
int policy_jit_execute(uint32_t policy_id, uint32_t capability);
int policy_jit_get_bytecode(uint32_t policy_id, bytecode_instr_t *out_instrs,
                           uint32_t max_instrs, uint32_t *instr_count);
void policy_jit_disassemble(uint32_t policy_id);
void policy_jit_get_stats(uint32_t *compilations, uint32_t *executions, uint32_t *errors);

#endif
