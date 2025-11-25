#ifndef FISH_SM_H
#define FISH_SM_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tofm.h"
#include "fish_motion.h"

// Encodage binaire de la situation obstacle, sur 3 bits.
#define FISH_OBSTACLE_LEFT   (1u << 0)
#define FISH_OBSTACLE_FRONT  (1u << 1)
#define FISH_OBSTACLE_RIGHT  (1u << 2)

#define FISH_OBSTACLE_MASK_COUNT 8u   // 2^3 combinaisons

typedef enum {
    FISH_STATE_IDLE = 0,
    FISH_STATE_FORWARD,
    FISH_STATE_TURN_LEFT,
    FISH_STATE_TURN_RIGHT,
    FISH_STATE_COUNT
} FishState;

// Contexte global de la machine à états (mode automatique simple).
typedef struct {
    FishState    state;
    FishMotion   motion;

    uint16_t      obstacle_threshold_mm; // seuil détection (min distance)
    TickType_t    min_turn_ticks;        // durée minimale d’un virage
    TickType_t    period_ticks;          // période d’exécution de la tâche
    TickType_t    state_entry_tick;      // horodatage d’entrée dans l’état
} FishCtrl;

// Initialisation de la structure de contrôle et du module de mouvement.
// - servo doit pointer vers un ServoCtrl déjà initialisé (sc_init).
// - params peut être NULL pour utiliser des valeurs par défaut.
void fish_ctrl_init(FishCtrl *rc, ServoCtrl *servo, const FishMotionParams *params);

// Tâche FreeRTOS : boucle principale de la machine à états.
// Arg attendu : pointeur vers une instance de FishCtrl.
void fish_ctrl_task(void *arg);

#endif // FISH_SM_H
