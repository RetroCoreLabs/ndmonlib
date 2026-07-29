/*
 * MON 52B (42 decimal): TerminalMode (TERMO)
 *
 * Selects various terminal functions (SINTRAN III Monitor Calls manual):
 *   Bit 0: Convert input to capital letters
 *   Bit 1: Delay after carriage return
 *   Bit 2: Stop output on full page
 *   Bit 3: Automatic logout if the line to the terminal is broken
 *
 * Parameters:
 *   [I] DeviceNumber (INTEGER): logical device number, 1 = own terminal
 *   [I] Mode (INTEGER): terminal mode bits (0-15)
 *
 * Emulator semantics: the host console has no page length, no CR delay and
 * no modem line, and uppercase conversion is not wanted, so none of the mode
 * bits change console behaviour. The mode is STORED per device (see
 * mon_terminal_state.c) so a later read-back sees what was set, and the call
 * returns success - LED calls TERMO(1, 0) during screen setup and only needs
 * the call to not error.
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_errors.h"
#include "mon_terminal_state.h"

MonResult mon_52B_TerminalMode(MonContext* ctx) {
    if (ctx->arg_count < 2) {
        mon_log(MON_LOG_WARN, MON_ID_52B ": Missing parameters (need 2, got %u)",
                ctx->arg_count);
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);  /* 157B Missing parameter */
        return MON_ERROR;
    }

    uint32_t device_no = mon_read_param_word(ctx, 0);
    uint32_t mode      = mon_read_param_word(ctx, 1);
    MON_LOG_IN_WORD(ctx, 0, "DeviceNumber");
    MON_LOG_IN_WORD(ctx, 1, "Mode");

    if (device_no == 0) device_no = 1;  /* own terminal */
    mon_set_terminal_mode(device_no, (int32_t)mode);

    mon_set_success(ctx);
    return MON_SUCCESS;
}
