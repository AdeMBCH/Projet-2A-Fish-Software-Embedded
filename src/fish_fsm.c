#include "fish_fsm.h"

#include "esp_log.h"

#define TAG "fish_sm"

// Raccourcis pour les combinaisons de masques obstacles
enum {
    OBST_NONE        = 0,
    OBST_LEFT        = FISH_OBSTACLE_LEFT,
    OBST_FRONT       = FISH_OBSTACLE_FRONT,
    OBST_RIGHT       = FISH_OBSTACLE_RIGHT,
    OBST_LEFT_FRONT  = FISH_OBSTACLE_LEFT  | FISH_OBSTACLE_FRONT,
    OBST_FRONT_RIGHT = FISH_OBSTACLE_FRONT | FISH_OBSTACLE_RIGHT,
    OBST_LEFT_RIGHT  = FISH_OBSTACLE_LEFT  | FISH_OBSTACLE_RIGHT,
    OBST_ALL         = FISH_OBSTACLE_LEFT  | FISH_OBSTACLE_FRONT | FISH_OBSTACLE_RIGHT
};

typedef void (*fish_action_fn)(FishCtrl *rc, uint8_t obst_mask);

typedef struct {
    FishState      next_state;
    fish_action_fn action;
} FishTransition;

// Prototypes internes pour les actions
static void action_idle      (FishCtrl *rc, uint8_t obst_mask);
static void action_forward   (FishCtrl *rc, uint8_t obst_mask);
static void action_turn_left (FishCtrl *rc, uint8_t obst_mask);
static void action_turn_right(FishCtrl *rc, uint8_t obst_mask);

// Table de transition [état courant][masque obstacles]
// Les actions sont des "entry actions" (on les rejoue à chaque sélection de transition).
static const FishTransition g_sm[FISH_STATE_COUNT][FISH_OBSTACLE_MASK_COUNT] = {
    // ------------------------ FISH_STATE_IDLE ------------------------
    [FISH_STATE_IDLE] = {
        [OBST_NONE]        = { FISH_STATE_FORWARD,    action_forward    },
        [OBST_LEFT]        = { FISH_STATE_FORWARD,    action_forward    },
        [OBST_FRONT]       = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_RIGHT]       = { FISH_STATE_FORWARD,    action_forward    },
        [OBST_LEFT_FRONT]  = { FISH_STATE_TURN_RIGHT, action_turn_right },
        [OBST_FRONT_RIGHT] = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_LEFT_RIGHT]  = { FISH_STATE_FORWARD,    action_forward    },
        [OBST_ALL]         = { FISH_STATE_TURN_LEFT,  action_turn_left  },
    },

    // --------------------- FISH_STATE_FORWARD ------------------------
    [FISH_STATE_FORWARD] = {
        [OBST_NONE]        = { FISH_STATE_FORWARD,    action_forward    },  // voie libre
        [OBST_LEFT]        = { FISH_STATE_FORWARD,    action_forward    },  // obstacle loin à gauche
        [OBST_FRONT]       = { FISH_STATE_TURN_LEFT,  action_turn_left  },  // obstacle devant -> on tente gauche
        [OBST_RIGHT]       = { FISH_STATE_FORWARD,    action_forward    },  // obstacle loin à droite
        [OBST_LEFT_FRONT]  = { FISH_STATE_TURN_RIGHT, action_turn_right },  // blocage gauche + devant -> virage droite
        [OBST_FRONT_RIGHT] = { FISH_STATE_TURN_LEFT,  action_turn_left  },  // blocage droite + devant -> virage gauche
        [OBST_LEFT_RIGHT]  = { FISH_STATE_FORWARD,    action_forward    },  // couloir sans obstacle frontal
        [OBST_ALL]         = { FISH_STATE_TURN_LEFT,  action_turn_left  },  // mur -> choix arbitraire gauche
    },

    // -------------------- FISH_STATE_TURN_LEFT -----------------------
    [FISH_STATE_TURN_LEFT] = {
        [OBST_NONE]        = { FISH_STATE_FORWARD,    action_forward    },
        [OBST_LEFT]        = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_FRONT]       = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_RIGHT]       = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_LEFT_FRONT]  = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_FRONT_RIGHT] = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_LEFT_RIGHT]  = { FISH_STATE_TURN_LEFT,  action_turn_left  },
        [OBST_ALL]         = { FISH_STATE_TURN_LEFT,  action_turn_left  },
    },

    // ------------------- FISH_STATE_TURN_RIGHT -----------------------
    [FISH_STATE_TURN_RIGHT] = {
        [OBST_NONE]        = { FISH_STATE_FORWARD,    action_forward    },
        [OBST_LEFT]        = { FISH_STATE_TURN_RIGHT, action_turn_right },
        [OBST_FRONT]       = { FISH_STATE_TURN_RIGHT, action_turn_right },
        [OBST_RIGHT]       = { FISH_STATE_TURN_RIGHT, action_turn_right },
        [OBST_LEFT_FRONT]  = { FISH_STATE_TURN_RIGHT, action_turn_right },
        [OBST_FRONT_RIGHT] = { FISH_STATE_TURN_RIGHT, action_turn_right },
        [OBST_LEFT_RIGHT]  = { FISH_STATE_TURN_RIGHT, action_turn_right },
        [OBST_ALL]         = { FISH_STATE_TURN_RIGHT, action_turn_right },
    }
};

// ---------------------------------------------------------
// Utilitaires internes
// ---------------------------------------------------------

// Construit le masque d’obstacle à partir des 3 capteurs ToF.
static uint8_t build_obstacle_mask(const FishCtrl *rc)
{
    (void)rc; // pas utilisé directement mais laissé pour extensions futures

    tof_sample_t s_left  = {0};
    tof_sample_t s_front = {0};
    tof_sample_t s_right = {0};

    // Convention: 0 = gauche, 1 = centre, 2 = droite
    tof_get_last_sample(0, &s_left);
    tof_get_last_sample(1, &s_front);
    tof_get_last_sample(2, &s_right);

    uint8_t mask = 0;

    if (s_left.valid  && s_left.distance_mm  < rc->obstacle_threshold_mm) {
        mask |= FISH_OBSTACLE_LEFT;
    }
    if (s_front.valid && s_front.distance_mm < rc->obstacle_threshold_mm) {
        mask |= FISH_OBSTACLE_FRONT;
    }
    if (s_right.valid && s_right.distance_mm < rc->obstacle_threshold_mm) {
        mask |= FISH_OBSTACLE_RIGHT;
    }

    return mask;
}

static void apply_transition(FishCtrl *rc, uint8_t obst_mask)
{
    if (rc == NULL || rc->state >= FISH_STATE_COUNT || obst_mask >= FISH_OBSTACLE_MASK_COUNT) {
        return;
    }

    const FishTransition *tr = &g_sm[rc->state][obst_mask];
    FishState next = tr->next_state;

    // Sécurité: si la table n’est pas correctement initialisée, on reste dans l’état courant.
    if (next >= FISH_STATE_COUNT) {
        next = rc->state;
    }

    if (next != rc->state) {
        ESP_LOGI(TAG, "Transition %d -> %d (mask=0x%02X)", rc->state, next, obst_mask);
        rc->state = next;
        rc->state_entry_tick = xTaskGetTickCount();
    }

    if (tr->action != NULL) {
        tr->action(rc, obst_mask);
    }
}

// ---------------------------------------------------------
// API publique
// ---------------------------------------------------------

void fish_ctrl_init(FishCtrl *rc, ServoCtrl *servo, const FishMotionParams *params)
{
    if (rc == NULL || servo == NULL) {
        return;
    }

    // Paramètres par défaut
    rc->obstacle_threshold_mm = 400;                    // 40 cm
    rc->period_ticks          = pdMS_TO_TICKS(50);      // 20 Hz
    rc->min_turn_ticks        = pdMS_TO_TICKS(700);     // ~0.7 s min de virage

    rc->state             = FISH_STATE_IDLE;
    rc->state_entry_tick  = xTaskGetTickCount();

    fm_init(&rc->motion, servo, params);
    fm_stop(&rc->motion); // on démarre à l’arrêt, l’OS décidera du mouvement
}

void fish_ctrl_task(void *arg)
{
    FishCtrl *rc = (FishCtrl *)arg;
    if (rc == NULL) {
        vTaskDelete(NULL);
        return;
    }

    // État initial : on part en avant
    rc->state = FISH_STATE_FORWARD;
    rc->state_entry_tick = xTaskGetTickCount();
    action_forward(rc, OBST_NONE);

    for (;;) {
        TickType_t now = xTaskGetTickCount();

        uint8_t obst_mask = build_obstacle_mask(rc);

        // Pendant un virage, on impose une durée minimale avant de refaire du
        // raisonnement sur les obstacles (sinon on oscille).
        if ((rc->state == FISH_STATE_TURN_LEFT  ||
             rc->state == FISH_STATE_TURN_RIGHT) &&
            (now - rc->state_entry_tick) < rc->min_turn_ticks) {

            vTaskDelay(rc->period_ticks);
            continue;
        }

        apply_transition(rc, obst_mask);

        vTaskDelay(rc->period_ticks);
    }
}

// ---------------------------------------------------------
// Actions d’état (entry actions)
// ---------------------------------------------------------

static void action_idle(FishCtrl *rc, uint8_t obst_mask)
{
    (void)obst_mask;
    ESP_LOGI(TAG, "Action: IDLE");
    fm_stop(&rc->motion);
}

static void action_forward(FishCtrl *rc, uint8_t obst_mask)
{
    (void)obst_mask;
    ESP_LOGI(TAG, "Action: FORWARD");
    fm_forward(&rc->motion);
}

static void action_turn_left(FishCtrl *rc, uint8_t obst_mask)
{
    (void)obst_mask;
    ESP_LOGI(TAG, "Action: TURN_LEFT");
    fm_turn_left(&rc->motion);
}

static void action_turn_right(FishCtrl *rc, uint8_t obst_mask)
{
    (void)obst_mask;
    ESP_LOGI(TAG, "Action: TURN_RIGHT");
    fm_turn_right(&rc->motion);
}
