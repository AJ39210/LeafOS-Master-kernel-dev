/**
 * kernel/SELeaf/policy_jit.c
 * 
 * SELeaf Policy JIT Compiler
 * 
 * Converts high-level policies to bytecode for fast execution
 * 
 * Bytecode Operations:
 *   OP_LOAD_CAP    - Load capability
 *   OP_CHECK       - Check condition
 *   OP_JUMP        - Jump if condition
 *   OP_ALLOW       - Allow operation
 *   OP_DENY        - Deny operation
 *   OP_AUDIT       - Log event
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Bytecode instruction opcodes */
typedef enum {
    OP_NOP = 0,         /* No operation */
    OP_LOAD_CAP,        /* Load capability (arg = cap type) */
    OP_LOAD_CONST,      /* Load constant */
    OP_COMPARE_EQ,      /* Compare equal */
    OP_COMPARE_LT,      /* Compare less than */
    OP_COMPARE_GT,      /* Compare greater than */
    OP_JUMP,            /* Unconditional jump */
    OP_JUMP_IF_TRUE,    /* Jump if true */
    OP_JUMP_IF_FALSE,   /* Jump if false */
    OP_ALLOW,           /* Allow operation */
    OP_DENY,            /* Deny operation */
    OP_AUDIT,           /* Log audit event */
    OP_RETURN           /* Return from policy */
} bytecode_opcode_t;

/* Bytecode instruction */
typedef struct {
    bytecode_opcode_t opcode;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
} bytecode_instr_t;

/* Compiled policy */
typedef struct {
    uint32_t id;
    char name[32];
    bytecode_instr_t instructions[256];
    uint32_t instr_count;
} compiled_policy_t;

/* JIT Compiler state */
#define COMPILED_POLICIES_MAX   32

typedef struct {
    compiled_policy_t policies[COMPILED_POLICIES_MAX];
    uint32_t policy_count;
    
    uint32_t compilations;
    uint32_t executions;
    uint32_t errors;
} jit_state_t;

static jit_state_t jit_state = {0};

/**
 * policy_jit_init()
 * Initialize JIT compiler
 */
int policy_jit_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY_JIT] JIT Compiler initialized\n");
    #endif
    
    jit_state.policy_count = 0;
    jit_state.compilations = 0;
    jit_state.executions = 0;
    jit_state.errors = 0;
    
    return 0;
}

/**
 * policy_jit_compile()
 * Compile policy rules to bytecode
 */
uint32_t policy_jit_compile(const char *policy_name, 
                           const char *policy_rules)
{
    if (jit_state.policy_count >= COMPILED_POLICIES_MAX) {
        jit_state.errors++;
        return 0;
    }
    
    compiled_policy_t *pol = &jit_state.policies[jit_state.policy_count];
    pol->id = jit_state.policy_count + 1;
    
    /* Copy policy name */
    int i = 0;
    while (policy_name[i] && i < 31) {
        pol->name[i] = policy_name[i];
        i++;
    }
    pol->name[i] = '\0';
    
    /* Simple bytecode generation from rule string */
    /* Parse rules: "allow file_read; deny network_send" */
    
    pol->instr_count = 0;
    int instr_idx = 0;
    
    /* Example: Generate default policy */
    if (policy_rules[0] == 'a') {  /* "allow" */
        pol->instructions[instr_idx].opcode = OP_ALLOW;
        pol->instructions[instr_idx].arg1 = 0;
        instr_idx++;
    } else if (policy_rules[0] == 'd') {  /* "deny" */
        pol->instructions[instr_idx].opcode = OP_DENY;
        pol->instructions[instr_idx].arg1 = 0;
        instr_idx++;
    } else {
        /* Parse more complex rules */
        pol->instructions[instr_idx].opcode = OP_LOAD_CAP;
        pol->instructions[instr_idx].arg1 = 1;  /* CAP_FILE_READ */
        instr_idx++;
        
        pol->instructions[instr_idx].opcode = OP_COMPARE_EQ;
        pol->instructions[instr_idx].arg1 = 1;
        instr_idx++;
        
        pol->instructions[instr_idx].opcode = OP_JUMP_IF_TRUE;
        pol->instructions[instr_idx].arg1 = 4;  /* Jump to allow */
        instr_idx++;
        
        pol->instructions[instr_idx].opcode = OP_DENY;
        pol->instructions[instr_idx].arg1 = 0;
        instr_idx++;
        
        pol->instructions[instr_idx].opcode = OP_RETURN;
        instr_idx++;
        
        pol->instructions[instr_idx].opcode = OP_ALLOW;
        pol->instructions[instr_idx].arg1 = 1;
        instr_idx++;
    }
    
    pol->instructions[instr_idx].opcode = OP_RETURN;
    instr_idx++;
    
    pol->instr_count = instr_idx;
    jit_state.policy_count++;
    jit_state.compilations++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[POLICY_JIT] Compiled policy: ");
    serial_puts(SERIAL_PORT_A, policy_name);
    serial_puts(SERIAL_PORT_A, " (");
    
    char instr_str[16];
    uint32_t temp = instr_idx;
    int len = 0;
    if (temp == 0) {
        instr_str[len++] = '0';
    } else {
        while (temp > 0) {
            instr_str[15 - len] = '0' + (temp % 10);
            temp /= 10;
            len++;
        }
    }
    if (len > 0) {
        serial_puts(SERIAL_PORT_A, &instr_str[15 - len + 1]);
    }
    
    serial_puts(SERIAL_PORT_A, " instructions)\n");
    #endif
    
    return pol->id;
}

/**
 * policy_jit_execute()
 * Execute compiled policy bytecode
 * 
 * Returns: 1=allow, 0=deny, -1=error
 */
int policy_jit_execute(uint32_t policy_id, uint32_t capability)
{
    if (policy_id == 0 || policy_id > jit_state.policy_count) {
        jit_state.errors++;
        return -1;
    }
    
    compiled_policy_t *pol = &jit_state.policies[policy_id - 1];
    
    /* Virtual machine state */
    uint32_t pc = 0;  /* Program counter */
    int condition_flag = 0;
    int result = 0;
    
    /* Execute bytecode */
    while (pc < pol->instr_count) {
        bytecode_instr_t *instr = &pol->instructions[pc];
        
        switch (instr->opcode) {
            case OP_NOP:
                break;
                
            case OP_LOAD_CAP:
                condition_flag = (capability == instr->arg1) ? 1 : 0;
                break;
                
            case OP_LOAD_CONST:
                condition_flag = instr->arg1;
                break;
                
            case OP_COMPARE_EQ:
                condition_flag = (capability == instr->arg1) ? 1 : 0;
                break;
                
            case OP_COMPARE_LT:
                condition_flag = (capability < instr->arg1) ? 1 : 0;
                break;
                
            case OP_COMPARE_GT:
                condition_flag = (capability > instr->arg1) ? 1 : 0;
                break;
                
            case OP_JUMP:
                pc = instr->arg1;
                continue;
                
            case OP_JUMP_IF_TRUE:
                if (condition_flag) {
                    pc = instr->arg1;
                    continue;
                }
                break;
                
            case OP_JUMP_IF_FALSE:
                if (!condition_flag) {
                    pc = instr->arg1;
                    continue;
                }
                break;
                
            case OP_ALLOW:
                result = 1;
                break;
                
            case OP_DENY:
                result = 0;
                break;
                
            case OP_AUDIT:
                /* Log event (instr->arg1 = event type) */
                break;
                
            case OP_RETURN:
                jit_state.executions++;
                return result;
                
            default:
                jit_state.errors++;
                return -1;
        }
        
        pc++;
    }
    
    jit_state.executions++;
    return result;
}

/**
 * policy_jit_get_bytecode()
 * Get bytecode for a compiled policy
 */
int policy_jit_get_bytecode(uint32_t policy_id, bytecode_instr_t *out_instrs,
                           uint32_t max_instrs, uint32_t *instr_count)
{
    if (policy_id == 0 || policy_id > jit_state.policy_count) {
        return -1;
    }
    
    compiled_policy_t *pol = &jit_state.policies[policy_id - 1];
    
    uint32_t copy_count = (pol->instr_count < max_instrs) ? 
        pol->instr_count : max_instrs;
    
    for (uint32_t i = 0; i < copy_count; i++) {
        out_instrs[i] = pol->instructions[i];
    }
    
    if (instr_count) *instr_count = copy_count;
    return 0;
}

/**
 * policy_jit_disassemble()
 * Display disassembly of policy bytecode
 */
void policy_jit_disassemble(uint32_t policy_id)
{
    #ifdef CONFIG_SERIAL_DRIVER
    if (policy_id == 0 || policy_id > jit_state.policy_count) {
        return;
    }
    
    compiled_policy_t *pol = &jit_state.policies[policy_id - 1];
    
    serial_puts(SERIAL_PORT_A, "\n[POLICY_JIT] Disassembly: ");
    serial_puts(SERIAL_PORT_A, pol->name);
    serial_puts(SERIAL_PORT_A, "\n");
    serial_puts(SERIAL_PORT_A, "========================\n");
    
    for (uint32_t i = 0; i < pol->instr_count; i++) {
        bytecode_instr_t *instr = &pol->instructions[i];
        
        const char *opcode_name = "UNKNOWN";
        switch (instr->opcode) {
            case OP_NOP: opcode_name = "NOP"; break;
            case OP_LOAD_CAP: opcode_name = "LOAD_CAP"; break;
            case OP_LOAD_CONST: opcode_name = "LOAD_CONST"; break;
            case OP_COMPARE_EQ: opcode_name = "CMP_EQ"; break;
            case OP_JUMP: opcode_name = "JUMP"; break;
            case OP_JUMP_IF_TRUE: opcode_name = "JUMP_TRUE"; break;
            case OP_JUMP_IF_FALSE: opcode_name = "JUMP_FALSE"; break;
            case OP_ALLOW: opcode_name = "ALLOW"; break;
            case OP_DENY: opcode_name = "DENY"; break;
            case OP_AUDIT: opcode_name = "AUDIT"; break;
            case OP_RETURN: opcode_name = "RETURN"; break;
            default: break;
        }
        
        serial_puts(SERIAL_PORT_A, "  ");
        serial_putchar(SERIAL_PORT_A, '0' + (i % 10));
        serial_puts(SERIAL_PORT_A, ": ");
        serial_puts(SERIAL_PORT_A, opcode_name);
        serial_puts(SERIAL_PORT_A, "\n");
    }
    #endif
}

/**
 * policy_jit_get_stats()
 * Get JIT compiler statistics
 */
void policy_jit_get_stats(uint32_t *compilations, uint32_t *executions, 
                         uint32_t *errors)
{
    if (compilations) *compilations = jit_state.compilations;
    if (executions) *executions = jit_state.executions;
    if (errors) *errors = jit_state.errors;
}
