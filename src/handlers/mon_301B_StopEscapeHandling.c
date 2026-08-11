/*
 * MON 301B (193 decimal): StopEscapeHandling (DUSEL)
 *
 * Disables user-defined escape handling. The ESCAPE key terminates the program
 * as normal again.
 *
 * Parameters (ND-860228-2 SINTRAN III Monitor Calls, p.490): none, beyond the
 * standard error code of appendix A. The MAC example is a bare "MON 301".
 *
 * Manual facts, as for MON 300B EUSEL: the body page says ASSEMBLY-500 "Not
 * available" and its footer reads "ND-100 | All users | Background programs",
 * so DUSEL is the ND-100 form. The ND-500 form is MON 405B USTBRK with the
 * on/off flag set to 0. "User-defined escape handling stops when a program
 * terminates", so this state is per-run.
 *
 * The stored handler ADDRESS survives this call - mon_set_user_escape_handler
 * only overwrites a stored handler with a non-zero one, so a later EUSEL or
 * USTBRK(on) without a fresh address behaves like SINTRAN's last handler.
 *
 * SCOPE: as for 300B and 405B, the state is RECORDED and the call succeeds;
 * the asynchronous ESCAPE->transfer is not delivered. Hence
 * MON_STATUS_IN_PROGRESS in the registry rather than VALIDATED.
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_terminal_state.h"

MonResult mon_301B_StopEscapeHandling(MonContext* ctx) {
    /* No input parameters. Device 1 == "own terminal", as 300B and 405B use. */
    mon_set_user_escape_handler(1, false, 0);

    mon_log(MON_LOG_DEBUG, MON_ID_301B ": user escape OFF");
    mon_set_success(ctx);
    return MON_SUCCESS;
}
