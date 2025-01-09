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

#include "app.h"

#include "adc.h"
#include "advanced_keys.h"
#include "debug.h"
#include "hardware.h"
#include "hid.h"
#include "layer.h"
#include "layout.h"
#include "post_hid_report.h"
#include "switches.h"
#include "tusb.h"
#include "user_config.h"

void app_init(void) {
    hardware_init();
    user_config_init();
    switch_init();

    hid_init();
    layer_init();
    post_hid_report_init();
    advanced_key_init();
    layout_init();

    wait_for_switch_calibration();

    tusb_init(BOARD_TUD_RHPORT, NULL);
}

void app_task(void) {
    tud_task();

    // Perform user-defined ADC task
    adc_user_task();

    layout_task();
}
