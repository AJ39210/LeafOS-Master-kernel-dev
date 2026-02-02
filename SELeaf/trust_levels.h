/**
 * include/SELeaf/trust_levels.h
 */

#ifndef SELEAF_TRUST_LEVELS_H
#define SELEAF_TRUST_LEVELS_H

#include <stdint.h>

typedef enum {
    TRUST_UNTRUSTED = 0,
    TRUST_LOW = 1,
    TRUST_MEDIUM = 2,
    TRUST_HIGH = 3,
    TRUST_SYSTEM = 4
} trust_level_t;

typedef struct {
    uint32_t entity_id;
    trust_level_t level;
    uint32_t trust_score;
    uint32_t violations;
    uint32_t good_actions;
    uint32_t last_update;
} trust_context_t;

int trust_levels_init(void);
uint32_t trust_register_entity(uint32_t entity_id, trust_level_t initial_level);
trust_level_t trust_get_level(uint32_t entity_id);
int trust_record_violation(uint32_t entity_id, uint32_t severity);
int trust_record_good_action(uint32_t entity_id, uint32_t points);
int trust_recalculate_level(uint32_t entity_id);
int trust_can_perform_action(uint32_t entity_id, uint32_t action_level);
int trust_elevate(uint32_t entity_id, trust_level_t new_level);
int trust_demote(uint32_t entity_id, trust_level_t new_level);
int trust_get_context(uint32_t entity_id, trust_context_t *out_context);
void trust_get_stats(uint32_t *elevations, uint32_t *demotions, uint32_t *recalculations);

#endif
