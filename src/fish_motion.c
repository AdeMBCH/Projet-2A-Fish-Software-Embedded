#include "fish_motion.h"

#include <string.h>

// Valeurs par défaut raisonnables pour un petit robot-poisson.
// À ajuster suivant tes essais / rapport de stage.
static const FishMotionParams k_default_params = {
    .forward_freq_hz        = 2.0f,
    .forward_amp_deg        = 25.0f,
    .forward_center_deg     = 0.0f,
    .turn_freq_hz           = 2.0f,
    .turn_amp_deg           = 25.0f,
    .turn_center_offset_deg = 20.0f
};

void fm_init(FishMotion *fm, ServoCtrl *servo, const FishMotionParams *params)
{
    if (fm == NULL || servo == NULL) {
        return;
    }

    fm->servo = servo;

    if (params != NULL) {
        fm->params = *params;
    } else {
        fm->params = k_default_params;
    }

    // Profil neutre au démarrage
    sc_set_profile(fm->servo,
                   fm->params.forward_freq_hz,
                   fm->params.forward_amp_deg,
                   fm->params.forward_center_deg);
    sc_start(fm->servo);
}

void fm_forward(FishMotion *fm)
{
    if (fm == NULL || fm->servo == NULL) {
        return;
    }

    sc_set_profile(fm->servo,
                   fm->params.forward_freq_hz,
                   fm->params.forward_amp_deg,
                   fm->params.forward_center_deg);
    sc_start(fm->servo);
}

void fm_turn_left(FishMotion *fm)
{
    if (fm == NULL || fm->servo == NULL) {
        return;
    }

    // Convention: offset positif = virage à gauche
    float center = fm->params.forward_center_deg + fm->params.turn_center_offset_deg;

    sc_set_profile(fm->servo,
                   fm->params.turn_freq_hz,
                   fm->params.turn_amp_deg,
                   center);
    sc_start(fm->servo);
}

void fm_turn_right(FishMotion *fm)
{
    if (fm == NULL || fm->servo == NULL) {
        return;
    }

    // Offset négatif = virage à droite.
    float center = fm->params.forward_center_deg - fm->params.turn_center_offset_deg;

    sc_set_profile(fm->servo,
                   fm->params.turn_freq_hz,
                   fm->params.turn_amp_deg,
                   center);
    sc_start(fm->servo);
}

void fm_stop(FishMotion *fm)
{
    if (fm == NULL || fm->servo == NULL) {
        return;
    }

    sc_stop(fm->servo);
}
