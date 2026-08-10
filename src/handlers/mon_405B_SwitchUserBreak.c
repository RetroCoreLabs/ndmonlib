/*
 * MON 405B (261 decimal): SwitchUserBreak (USTRK)
 *
 * Switches user-defined escape handling on and off. When it is ON, pressing the
 * ESCAPE key is meant to transfer control to a routine in the program (the "user
 * break" handler) instead of terminating the program. This is the combined form
 * of MON 300B EUSEL (on, with address) and MON 301B DUSEL (off); all three drive
 * the same per-terminal handler state.
 *
 * Parameters:
 *   [I] OnOffFlag (INTEGER): 1 = switch user escape handling ON, 0 = OFF.
 *   [I] Address   (INTEGER): ND-500 program address to continue at when ESCAPE is
 *                            pressed (only meaningful while ON).
 *
 * SCOPE: the handler on/off + address are RECORDED so the call succeeds and a
 * read-back agrees. The ASYNCHRONOUS ESCAPE->transfer is NOT delivered - a
 * headless run never presses ESCAPE, and there is no console->PC-jump path.
 * See mon_terminal_state (user_escape_*). Kept in sync with RetroCore
 * MON_405_USTRK.cs. Reference: ND-60158 Symbolic Debugger (USER-ESCAPE, p.82).
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_terminal_state.h"

MonResult mon_405B_SwitchUserBreak(MonContext* ctx) {
    uint32_t on_off  = (ctx->arg_count >= 1) ? mon_read_param_word(ctx, 0) : 0;
    uint32_t address = (ctx->arg_count >= 2) ? mon_read_param_word(ctx, 1) : 0;
    MON_LOG_IN_WORD(ctx, 0, "OnOffFlag");
    MON_LOG_IN_WORD(ctx, 1, "Address");

    /* USTRK carries no device number - it applies to the calling program own
     * terminal. Device 1 == "own terminal", as MON 71B/72B DESCF/EESCF use.
     * Record the handler; the async ESCAPE->transfer is not delivered. */
    mon_set_user_escape_handler(1, on_off != 0, address);

    mon_log(MON_LOG_DEBUG, MON_ID_405B ": user escape %s, handler 0%o",
            (on_off != 0) ? "ON" : "OFF", address);
    mon_set_success(ctx);
    return MON_SUCCESS;
}
