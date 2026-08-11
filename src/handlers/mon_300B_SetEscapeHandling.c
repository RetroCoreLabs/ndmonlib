/*
 * MON 300B (192 decimal): SetEscapeHandling (EUSEL)
 *
 * Enables user-defined escape handling. When the ESCAPE key is pressed,
 * execution continues at the specified address in the program.
 *
 * Parameters (ND-860228-2 SINTRAN III Monitor Calls, p.442):
 *   [I] EscapeHandler (INTEGER): contents of the start of the escape-handler
 *                                routine. The MAC example is "LDA PROG %Address
 *                                of user escape handler routine", so the value
 *                                passed is the handler's address.
 *   Standard error code, appendix A.
 *
 * Manual facts worth keeping, because they are not obvious from the number:
 *   - The manual's ND-100/ND-500 overview table (p.047) marks 300B for BOTH,
 *     but the body page is the specific statement and it says ASSEMBLY-500
 *     "Not available", gives a MAC example, and its footer reads
 *     "ND-100 | All users | Background programs". EUSEL/DUSEL are the ND-100
 *     form of this facility.
 *   - The ND-500 form is MON 405B USTBRK, whose own body page (p.500/501)
 *     carries a CALLG example and the footer "ND-500 | All users | All
 *     programs", and says MAC is "Not available". It takes the on/off flag and
 *     the address as two parameters, i.e. 405B is 300B and 301B combined.
 *   - "The normal escape handling is reset when the program aborts", so this
 *     state is per-run, not persistent.
 *
 * All three calls drive ONE per-terminal handler state - see
 * mon_terminal_state (user_escape_*) and mon_405B_SwitchUserBreak.c.
 *
 * SCOPE: the handler on/off + address are RECORDED so the call succeeds and a
 * read-back agrees. The ASYNCHRONOUS ESCAPE->transfer is NOT delivered - a
 * headless run never presses ESCAPE, and there is no console->PC-jump path.
 * That is why the registry entry is MON_STATUS_IN_PROGRESS and not VALIDATED.
 *
 * PARAMETER WIDTH, UNVERIFIED FOR ND-100: this reads a 32-bit word, as every
 * other handler in this library does. On the ND-100 an INTEGER parameter is 16
 * bits and MON 300B takes its argument in the A register rather than through a
 * CALLG parameter list, so an ND-100 caller may need a different read here.
 * MonContext carries no architecture flag today, so nothing is guessed - the
 * ND-500 path is what is exercised.
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_terminal_state.h"

MonResult mon_300B_SetEscapeHandling(MonContext* ctx) {
    uint32_t address = (ctx->arg_count >= 1) ? mon_read_param_word(ctx, 0) : 0;
    MON_LOG_IN_WORD(ctx, 0, "EscapeHandler");

    /* EUSEL carries no device number - it applies to the calling program's own
     * terminal. Device 1 == "own terminal", as MON 71B/72B DESCF/EESCF and MON
     * 405B USTBRK use. */
    mon_set_user_escape_handler(1, true, address);

    mon_log(MON_LOG_DEBUG, MON_ID_300B ": user escape ON, handler 0%o", address);
    mon_set_success(ctx);
    return MON_SUCCESS;
}
