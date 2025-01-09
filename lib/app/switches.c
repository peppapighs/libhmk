/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "switches.h"

#include <stdbool.h>

#include "timer.h"
#include "user_config.h"

const uint16_t sw_distance[] = {
    [SW_GEON_RAW_HE] = 68,
    [SW_GATERON_MAGNETIC_JADE] = 70,
    [SW_GEON_RAPTOR_HE] = 80,
};

_Static_assert(sizeof(sw_distance) / sizeof(sw_distance[0]) == SW_COUNT,
               "Invalid switch distance array size");

static bool is_calibrating;
static uint32_t calibration_start_time;

static switch_state_t switches[NUM_KEYS];

void switch_init(void) {
    const uint8_t sw_id = user_config_sw_id();
    const uint16_t min_adc_value = calibration_initial_min(sw_id);
    const uint16_t max_adc_value =
        SAFE_ADC_ADD(min_adc_value, calibration_delta(sw_id));

    for (uint32_t i = 0; i < NUM_KEYS; i++) {
        switches[i].adc_state.adc_value = max_adc_value;
        switches[i].adc_state.min_adc_value = min_adc_value;
        switches[i].adc_state.max_adc_value = max_adc_value;
        switches[i].distance = 0;
        switches[i].peek_distance = 0;
        switches[i].sw_dir = SW_DIR_NONE;
        switches[i].sw_state = 0;
    }

    is_calibrating = true;
    calibration_start_time = timer_read();
}

static bool is_switch_calibrating(void) {
    // Short-circuit if not calibrating
    if (!is_calibrating)
        return false;

    if (timer_elapsed(calibration_start_time) >= CALIBRATION_TIME) {
        is_calibrating = false;
        calibration_start_time = 0;
    }

    return is_calibrating;
}

void wait_for_switch_calibration(void) {
    while (is_switch_calibrating())
        ;
}

void switch_recalibrate(void) {
    // We simply re-initialize the switches to restart the calibration.
    switch_init();
    wait_for_switch_calibration();
}

void store_adc_value(uint8_t index, uint16_t adc_value) {
    if (index >= NUM_KEYS)
        return;

    adc_state_t *adc_state = &switches[index].adc_state;

    // Exponential smoothing
    // x[n] = (1 - alpha) * x[n-1] + alpha * x[n]
    adc_state->adc_value = ((uint32_t)adc_state->adc_value *
                                ((1 << SMOOTHING_FACTOR_RECIPROCAL_LOG2) - 1) +
                            adc_value) >>
                           SMOOTHING_FACTOR_RECIPROCAL_LOG2;

    // Calibrate the switch if needed
    if (is_switch_calibrating() &&
        adc_state->adc_value < adc_state->min_adc_value) {
        adc_state->min_adc_value = adc_state->adc_value;
        adc_state->max_adc_value = SAFE_ADC_ADD(
            adc_state->min_adc_value, calibration_delta(user_config_sw_id()));
    }
}

__attribute__((weak)) uint8_t adc_state_to_distance(
    const adc_state_t *adc_state, uint8_t sw_id, uint8_t sw_dist) {
    // curve[x] = 1024 * log10 (1 + 9x / 1024)
    // This curve is obtained by fitting the data from the Geon Raw HE switch.
    static const uint16_t curve[] = {
        0,    4,    8,    12,   15,   19,   23,   27,   30,   34,   37,   41,
        45,   48,   52,   55,   59,   62,   65,   69,   72,   75,   79,   82,
        85,   88,   92,   95,   98,   101,  104,  107,  110,  113,  116,  119,
        122,  125,  128,  131,  134,  137,  140,  143,  145,  148,  151,  154,
        157,  159,  162,  165,  167,  170,  173,  175,  178,  181,  183,  186,
        188,  191,  193,  196,  198,  201,  203,  206,  208,  211,  213,  216,
        218,  220,  223,  225,  228,  230,  232,  234,  237,  239,  241,  244,
        246,  248,  250,  253,  255,  257,  259,  261,  264,  266,  268,  270,
        272,  274,  276,  278,  280,  283,  285,  287,  289,  291,  293,  295,
        297,  299,  301,  303,  305,  307,  309,  311,  313,  315,  316,  318,
        320,  322,  324,  326,  328,  330,  332,  333,  335,  337,  339,  341,
        343,  344,  346,  348,  350,  351,  353,  355,  357,  359,  360,  362,
        364,  365,  367,  369,  371,  372,  374,  376,  377,  379,  381,  382,
        384,  386,  387,  389,  390,  392,  394,  395,  397,  399,  400,  402,
        403,  405,  406,  408,  410,  411,  413,  414,  416,  417,  419,  420,
        422,  423,  425,  426,  428,  429,  431,  432,  434,  435,  437,  438,
        440,  441,  443,  444,  445,  447,  448,  450,  451,  453,  454,  455,
        457,  458,  460,  461,  462,  464,  465,  466,  468,  469,  471,  472,
        473,  475,  476,  477,  479,  480,  481,  483,  484,  485,  487,  488,
        489,  490,  492,  493,  494,  496,  497,  498,  499,  501,  502,  503,
        504,  506,  507,  508,  509,  511,  512,  513,  514,  516,  517,  518,
        519,  521,  522,  523,  524,  525,  527,  528,  529,  530,  531,  533,
        534,  535,  536,  537,  538,  540,  541,  542,  543,  544,  545,  546,
        548,  549,  550,  551,  552,  553,  554,  556,  557,  558,  559,  560,
        561,  562,  563,  564,  565,  567,  568,  569,  570,  571,  572,  573,
        574,  575,  576,  577,  578,  580,  581,  582,  583,  584,  585,  586,
        587,  588,  589,  590,  591,  592,  593,  594,  595,  596,  597,  598,
        599,  600,  601,  602,  603,  604,  605,  606,  607,  608,  609,  610,
        611,  612,  613,  614,  615,  616,  617,  618,  619,  620,  621,  622,
        623,  624,  625,  626,  627,  628,  629,  630,  631,  632,  633,  633,
        634,  635,  636,  637,  638,  639,  640,  641,  642,  643,  644,  645,
        646,  646,  647,  648,  649,  650,  651,  652,  653,  654,  655,  655,
        656,  657,  658,  659,  660,  661,  662,  663,  663,  664,  665,  666,
        667,  668,  669,  670,  670,  671,  672,  673,  674,  675,  676,  676,
        677,  678,  679,  680,  681,  682,  682,  683,  684,  685,  686,  687,
        687,  688,  689,  690,  691,  692,  692,  693,  694,  695,  696,  696,
        697,  698,  699,  700,  701,  701,  702,  703,  704,  705,  705,  706,
        707,  708,  709,  709,  710,  711,  712,  713,  713,  714,  715,  716,
        716,  717,  718,  719,  720,  720,  721,  722,  723,  723,  724,  725,
        726,  726,  727,  728,  729,  730,  730,  731,  732,  733,  733,  734,
        735,  736,  736,  737,  738,  739,  739,  740,  741,  741,  742,  743,
        744,  744,  745,  746,  747,  747,  748,  749,  750,  750,  751,  752,
        752,  753,  754,  755,  755,  756,  757,  757,  758,  759,  760,  760,
        761,  762,  762,  763,  764,  764,  765,  766,  767,  767,  768,  769,
        769,  770,  771,  771,  772,  773,  773,  774,  775,  776,  776,  777,
        778,  778,  779,  780,  780,  781,  782,  782,  783,  784,  784,  785,
        786,  786,  787,  788,  788,  789,  790,  790,  791,  792,  792,  793,
        794,  794,  795,  796,  796,  797,  798,  798,  799,  799,  800,  801,
        801,  802,  803,  803,  804,  805,  805,  806,  807,  807,  808,  808,
        809,  810,  810,  811,  812,  812,  813,  814,  814,  815,  815,  816,
        817,  817,  818,  819,  819,  820,  820,  821,  822,  822,  823,  823,
        824,  825,  825,  826,  827,  827,  828,  828,  829,  830,  830,  831,
        831,  832,  833,  833,  834,  834,  835,  836,  836,  837,  837,  838,
        839,  839,  840,  840,  841,  841,  842,  843,  843,  844,  844,  845,
        846,  846,  847,  847,  848,  848,  849,  850,  850,  851,  851,  852,
        853,  853,  854,  854,  855,  855,  856,  857,  857,  858,  858,  859,
        859,  860,  861,  861,  862,  862,  863,  863,  864,  864,  865,  866,
        866,  867,  867,  868,  868,  869,  869,  870,  871,  871,  872,  872,
        873,  873,  874,  874,  875,  876,  876,  877,  877,  878,  878,  879,
        879,  880,  880,  881,  881,  882,  883,  883,  884,  884,  885,  885,
        886,  886,  887,  887,  888,  888,  889,  889,  890,  891,  891,  892,
        892,  893,  893,  894,  894,  895,  895,  896,  896,  897,  897,  898,
        898,  899,  899,  900,  900,  901,  901,  902,  903,  903,  904,  904,
        905,  905,  906,  906,  907,  907,  908,  908,  909,  909,  910,  910,
        911,  911,  912,  912,  913,  913,  914,  914,  915,  915,  916,  916,
        917,  917,  918,  918,  919,  919,  920,  920,  921,  921,  922,  922,
        923,  923,  924,  924,  925,  925,  926,  926,  926,  927,  927,  928,
        928,  929,  929,  930,  930,  931,  931,  932,  932,  933,  933,  934,
        934,  935,  935,  936,  936,  937,  937,  938,  938,  939,  939,  939,
        940,  940,  941,  941,  942,  942,  943,  943,  944,  944,  945,  945,
        946,  946,  946,  947,  947,  948,  948,  949,  949,  950,  950,  951,
        951,  952,  952,  952,  953,  953,  954,  954,  955,  955,  956,  956,
        957,  957,  958,  958,  958,  959,  959,  960,  960,  961,  961,  962,
        962,  962,  963,  963,  964,  964,  965,  965,  966,  966,  966,  967,
        967,  968,  968,  969,  969,  970,  970,  970,  971,  971,  972,  972,
        973,  973,  974,  974,  974,  975,  975,  976,  976,  977,  977,  977,
        978,  978,  979,  979,  980,  980,  981,  981,  981,  982,  982,  983,
        983,  984,  984,  984,  985,  985,  986,  986,  987,  987,  987,  988,
        988,  989,  989,  989,  990,  990,  991,  991,  992,  992,  992,  993,
        993,  994,  994,  995,  995,  995,  996,  996,  997,  997,  997,  998,
        998,  999,  999,  999,  1000, 1000, 1001, 1001, 1002, 1002, 1002, 1003,
        1003, 1004, 1004, 1004, 1005, 1005, 1006, 1006, 1006, 1007, 1007, 1008,
        1008, 1008, 1009, 1009, 1010, 1010, 1011, 1011, 1011, 1012, 1012, 1013,
        1013, 1013, 1014, 1014, 1015, 1015, 1015, 1016, 1016, 1017, 1017, 1017,
        1018, 1018, 1018, 1019, 1019, 1020, 1020, 1020, 1021, 1021, 1022, 1022,
        1022, 1023, 1023, 1024, 1024,
    };

    if (adc_state->adc_value <= adc_state->min_adc_value ||
        adc_state->min_adc_value >= adc_state->max_adc_value)
        return 0;

    if (adc_state->adc_value >= adc_state->max_adc_value)
        return sw_dist;

    // Scale the ADC value to the range [0, 1024]
    const uint32_t range = adc_state->max_adc_value - adc_state->min_adc_value;
    const uint32_t normalized =
        (((uint32_t)(adc_state->adc_value - adc_state->min_adc_value) << 10) +
         (range >> 1)) /
        range;

    // Scale the curve to the range [0, sw_dist]
    return ((uint32_t)curve[normalized] * sw_dist + 512) >> 10;
}

void matrix_scan(void) {
    const uint8_t sw_id = user_config_sw_id();
    const uint8_t sw_dist = sw_distance[sw_id];
    const uint8_t current_profile = user_config_current_profile();

    for (uint32_t i = 0; i < NUM_KEYS; i++) {
        switch_state_t *sw = &switches[i];
        const key_config_t *config = user_config_key_config(current_profile, i);

        // WARNING: Copy the current ADC value to a local variable to prevent
        // data race in case the interrupt updates the ADC value in the middle
        // of the calculation.
        const uint16_t adc_value = sw->adc_state.adc_value;

        // Update the maximum ADC value
        if (adc_value > sw->adc_state.max_adc_value)
            sw->adc_state.max_adc_value = adc_value;

        // Update the travel distance
        sw->distance = adc_state_to_distance(&sw->adc_state, sw_id, sw_dist);

        switch (config->mode) {
        case KEY_MODE_NORMAL:
            const key_mode_normal_t *nm = &config->nm;

            // If the switch reached the actuation distance
            sw->sw_state = (sw->distance >= nm->actuation_distance);
            break;

        case KEY_MODE_RAPID_TRIGGER:
            const key_mode_rapid_trigger_t *rt = &config->rt;
            const uint8_t reset_distance =
                rt->continuous ? 0 : nm->actuation_distance;

            switch (sw->sw_dir) {
            case SW_DIR_NONE:
                if (sw->distance > rt->actuation_distance) {
                    // Travel down past the actuation distance
                    sw->sw_dir = SW_DIR_DOWN;
                    sw->peek_distance = sw->distance;
                    sw->sw_state = 1;
                }
                break;

            case SW_DIR_DOWN:
                if (sw->distance <= reset_distance) {
                    // Travel up past the reset distance
                    sw->sw_dir = SW_DIR_NONE;
                    sw->peek_distance = sw->distance;
                    sw->sw_state = 0;
                } else if (sw->distance + rt->rt_up_distance <
                           sw->peek_distance) {
                    // Change direction to up
                    sw->sw_dir = SW_DIR_UP;
                    sw->peek_distance = sw->distance;
                    sw->sw_state = 0;
                } else if (sw->distance > sw->peek_distance)
                    // Travel further down
                    sw->peek_distance = sw->distance;
                break;

            case SW_DIR_UP:
                if (sw->distance <= reset_distance) {
                    // Travel up past the reset distance
                    sw->sw_dir = SW_DIR_NONE;
                    sw->peek_distance = sw->distance;
                    sw->sw_state = 0;
                } else if (sw->peek_distance + rt->rt_down_distance <
                           sw->distance) {
                    // Change direction to down
                    sw->sw_dir = SW_DIR_DOWN;
                    sw->peek_distance = sw->distance;
                    sw->sw_state = 1;
                } else if (sw->distance < sw->peek_distance)
                    // Travel further up
                    sw->peek_distance = sw->distance;
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
    }
}

uint16_t get_switch_adc_value(uint8_t index) {
    if (index >= NUM_KEYS)
        return 0;

    return switches[index].adc_state.adc_value;
}

uint8_t get_switch_distance(uint8_t index) {
    if (index >= NUM_KEYS)
        return 0;

    return switches[index].distance;
}

uint8_t get_switch_state(uint8_t index) {
    if (index >= NUM_KEYS)
        return 0;

    return switches[index].sw_state;
}
