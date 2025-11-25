#ifndef FISH_MOTION_H
#define FISH_MOTION_H

#include "servo_ctrl.h"

// Paramètres cinématiques de base pour les mouvements de la queue.
// Ces valeurs sont à adapter expérimentalement (cf. rapport de stage).
typedef struct {
    float forward_freq_hz;        // fréquence d’ondulation en ligne droite
    float forward_amp_deg;        // amplitude en ligne droite
    float forward_center_deg;     // offset neutre (souvent 0)

    float turn_freq_hz;           // fréquence pendant un virage (peut être identique)
    float turn_amp_deg;           // amplitude pendant un virage
    float turn_center_offset_deg; // décalage de centre appliqué à gauche/droite
} FishMotionParams;

typedef struct {
    ServoCtrl         *servo;
    FishMotionParams  params;
} FishMotion;

// Initialisation du module de mouvement.
// - servo : pointeur vers un ServoCtrl déjà initialisé (sc_init).
// - params : NULL -> valeurs par défaut internes.
void fm_init(FishMotion *fm, ServoCtrl *servo, const FishMotionParams *params);

// Mouvement continu : la queue ondule autour de l’axe neutre.
void fm_forward(FishMotion *fm);

// Virage à gauche : centre d’oscillation décalé d’un offset.
// Le signe réel à utiliser dépend de ta mécanique; à ajuster in situ.
void fm_turn_left(FishMotion *fm);

// Virage à droite : centre d’oscillation décalé dans l’autre sens.
void fm_turn_right(FishMotion *fm);

// Arrêt complet de l’ondulation.
void fm_stop(FishMotion *fm);

#endif // FISH_MOTION_H
