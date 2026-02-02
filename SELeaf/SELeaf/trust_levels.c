/**
 * kernel/SELeaf/trust_levels.c
 * 
 * SELeaf Dynamic Trust Levels
 * 
 * Provides:
 * - Adaptive trust scoring
 * - Behavior-based trust adjustment
 * - Trust elevation/demotion
 * - Reputation system
 */

#include <stdint.h>
#include <stddef.h>
#include "../../include/serial.h"

/* Trust level definitions */
typedef enum {
    TRUST_UNTRUSTED = 0,    /* Malware/untrusted */
    TRUST_LOW = 1,          /* Limited permissions */
    TRUST_MEDIUM = 2,       /* Standard permissions */
    TRUST_HIGH = 3,         /* Extended permissions */
    TRUST_SYSTEM = 4        /* Kernel/system level */
} trust_level_t;

/* Trust context for entity */
typedef struct {
    uint32_t entity_id;      /* Process/driver ID */
    trust_level_t level;
    uint32_t trust_score;    /* 0-1000 points */
    uint32_t violations;
    uint32_t good_actions;
    uint32_t last_update;
} trust_context_t;

/* Trust levels state */
#define ENTITIES_MAX            128

typedef struct {
    trust_context_t entities[ENTITIES_MAX];
    uint32_t entity_count;
    
    uint32_t trust_elevations;
    uint32_t trust_demotions;
    uint32_t recalculations;
} trust_state_t;

static trust_state_t trust_state = {0};

/**
 * trust_levels_init()
 * Initialize trust level system
 */
int trust_levels_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[TRUST] Dynamic trust levels initialized\n");
    #endif
    
    trust_state.entity_count = 0;
    trust_state.trust_elevations = 0;
    trust_state.trust_demotions = 0;
    trust_state.recalculations = 0;
    
    return 0;
}

/**
 * trust_register_entity()
 * Register entity with initial trust level
 */
uint32_t trust_register_entity(uint32_t entity_id, trust_level_t initial_level)
{
    if (trust_state.entity_count >= ENTITIES_MAX) {
        return 0;  /* Error */
    }
    
    trust_context_t *ctx = &trust_state.entities[trust_state.entity_count];
    ctx->entity_id = entity_id;
    ctx->level = initial_level;
    ctx->trust_score = initial_level * 200;  /* 0-1000 scale */
    ctx->violations = 0;
    ctx->good_actions = 0;
    ctx->last_update = 0;
    
    trust_state.entity_count++;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[TRUST] Entity ");
    
    char id_str[16];
    uint32_t temp = entity_id;
    int len = 0;
    if (temp == 0) {
        id_str[len++] = '0';
    } else {
        while (temp > 0) {
            id_str[15 - len] = '0' + (temp % 10);
            temp /= 10;
            len++;
        }
    }
    if (len > 0) {
        serial_puts(SERIAL_PORT_A, &id_str[15 - len + 1]);
    }
    
    serial_puts(SERIAL_PORT_A, " registered at level ");
    serial_putchar(SERIAL_PORT_A, '0' + initial_level);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 1;
}

/**
 * trust_get_level()
 * Get current trust level for entity
 */
trust_level_t trust_get_level(uint32_t entity_id)
{
    for (uint32_t i = 0; i < trust_state.entity_count; i++) {
        if (trust_state.entities[i].entity_id == entity_id) {
            return trust_state.entities[i].level;
        }
    }
    
    return TRUST_UNTRUSTED;  /* Unknown entity */
}

/**
 * trust_record_violation()
 * Record security violation and decrease trust
 */
int trust_record_violation(uint32_t entity_id, uint32_t severity)
{
    for (uint32_t i = 0; i < trust_state.entity_count; i++) {
        if (trust_state.entities[i].entity_id == entity_id) {
            trust_context_t *ctx = &trust_state.entities[i];
            
            ctx->violations++;
            
            /* Decrease trust score based on severity */
            uint32_t penalty = severity * 50;
            if (ctx->trust_score > penalty) {
                ctx->trust_score -= penalty;
            } else {
                ctx->trust_score = 0;
            }
            
            /* Recalculate trust level */
            trust_recalculate_level(entity_id);
            
            return 0;
        }
    }
    
    return -1;  /* Entity not found */
}

/**
 * trust_record_good_action()
 * Record positive action and increase trust
 */
int trust_record_good_action(uint32_t entity_id, uint32_t points)
{
    for (uint32_t i = 0; i < trust_state.entity_count; i++) {
        if (trust_state.entities[i].entity_id == entity_id) {
            trust_context_t *ctx = &trust_state.entities[i];
            
            ctx->good_actions++;
            
            /* Increase trust score (capped at 1000) */
            if (ctx->trust_score < 1000) {
                ctx->trust_score += points;
                if (ctx->trust_score > 1000) {
                    ctx->trust_score = 1000;
                }
            }
            
            /* Recalculate trust level */
            trust_recalculate_level(entity_id);
            
            return 0;
        }
    }
    
    return -1;
}

/**
 * trust_recalculate_level()
 * Recalculate trust level based on score
 */
int trust_recalculate_level(uint32_t entity_id)
{
    trust_level_t old_level = TRUST_UNTRUSTED;
    trust_level_t new_level = TRUST_UNTRUSTED;
    
    for (uint32_t i = 0; i < trust_state.entity_count; i++) {
        if (trust_state.entities[i].entity_id == entity_id) {
            trust_context_t *ctx = &trust_state.entities[i];
            
            old_level = ctx->level;
            
            /* Map score to trust level */
            if (ctx->trust_score < 200) {
                new_level = TRUST_UNTRUSTED;
            } else if (ctx->trust_score < 400) {
                new_level = TRUST_LOW;
            } else if (ctx->trust_score < 600) {
                new_level = TRUST_MEDIUM;
            } else if (ctx->trust_score < 900) {
                new_level = TRUST_HIGH;
            } else {
                new_level = TRUST_SYSTEM;
            }
            
            /* Detect level changes */
            if (new_level > old_level) {
                trust_state.trust_elevations++;
            } else if (new_level < old_level) {
                trust_state.trust_demotions++;
            }
            
            ctx->level = new_level;
            trust_state.recalculations++;
            
            return 0;
        }
    }
    
    return -1;
}

/**
 * trust_can_perform_action()
 * Check if entity can perform action at current trust level
 * 
 * Returns: 1 if allowed, 0 if denied
 */
int trust_can_perform_action(uint32_t entity_id, uint32_t action_level)
{
    trust_level_t current_level = trust_get_level(entity_id);
    
    /* Entity must be at or above required action level */
    return (current_level >= action_level) ? 1 : 0;
}

/**
 * trust_elevate()
 * Manually elevate trust level
 */
int trust_elevate(uint32_t entity_id, trust_level_t new_level)
{
    for (uint32_t i = 0; i < trust_state.entity_count; i++) {
        if (trust_state.entities[i].entity_id == entity_id) {
            trust_context_t *ctx = &trust_state.entities[i];
            
            if (new_level > ctx->level) {
                ctx->level = new_level;
                ctx->trust_score = new_level * 200;
                trust_state.trust_elevations++;
                
                #ifdef CONFIG_SERIAL_DRIVER
                serial_puts(SERIAL_PORT_A, "[TRUST] Elevated ");
                
                char id_str[16];
                uint32_t temp = entity_id;
                int len = 0;
                if (temp == 0) {
                    id_str[len++] = '0';
                } else {
                    while (temp > 0) {
                        id_str[15 - len] = '0' + (temp % 10);
                        temp /= 10;
                        len++;
                    }
                }
                if (len > 0) {
                    serial_puts(SERIAL_PORT_A, &id_str[15 - len + 1]);
                }
                
                serial_puts(SERIAL_PORT_A, " to level ");
                serial_putchar(SERIAL_PORT_A, '0' + new_level);
                serial_puts(SERIAL_PORT_A, "\n");
                #endif
            }
            
            return 0;
        }
    }
    
    return -1;
}

/**
 * trust_demote()
 * Manually demote trust level
 */
int trust_demote(uint32_t entity_id, trust_level_t new_level)
{
    for (uint32_t i = 0; i < trust_state.entity_count; i++) {
        if (trust_state.entities[i].entity_id == entity_id) {
            trust_context_t *ctx = &trust_state.entities[i];
            
            if (new_level < ctx->level) {
                ctx->level = new_level;
                ctx->trust_score = new_level * 200;
                trust_state.trust_demotions++;
            }
            
            return 0;
        }
    }
    
    return -1;
}

/**
 * trust_get_context()
 * Get full trust context for entity
 */
int trust_get_context(uint32_t entity_id, trust_context_t *out_context)
{
    for (uint32_t i = 0; i < trust_state.entity_count; i++) {
        if (trust_state.entities[i].entity_id == entity_id) {
            *out_context = trust_state.entities[i];
            return 0;
        }
    }
    
    return -1;
}

/**
 * trust_get_stats()
 * Get trust system statistics
 */
void trust_get_stats(uint32_t *elevations, uint32_t *demotions, 
                    uint32_t *recalculations)
{
    if (elevations) *elevations = trust_state.trust_elevations;
    if (demotions) *demotions = trust_state.trust_demotions;
    if (recalculations) *recalculations = trust_state.recalculations;
}
