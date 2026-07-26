/*
 * MON 317B [UECOM/ExecuteCommand]
 *
 * Executes a SINTRAN III command. Specify the command name and the parameters
 * as a text string.
 *
 * - An error message is output if an error occurs. The program does not terminate.
 * - Some commands may destroy your program.
 * - Some commands have output, e.g. @LIST-FILES. This is displayed on the terminal.
 *
 * Parameters:
 *   [I] Command (STRING): SINTRAN III command to execute
 *
 * Note: This is a stub implementation that logs the command but doesn't execute it.
 * SINTRAN III command execution is not implemented.
 *
 * Reference: SINTRAN III Monitor Calls (ND-860228.2 EN)
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_errors.h"
#include <stdlib.h>

/* Frontend-registered nested-command runner (loads+runs a DOM re-entrantly).
 * NULL until the --monitor shell registers one; then UECOM actually executes
 * the named program (e.g. NC's CAT-CAT5-B06 code-generator back-end). */
static MonExecuteCommandFn g_exec_cmd = NULL;

void mon_set_execute_command(MonExecuteCommandFn fn) {
    g_exec_cmd = fn;
}

MonResult mon_317B_ExecuteCommand(MonContext* ctx) {
    /* Defensive check for argument count */
    if (ctx->arg_count < 1) {
        mon_log(MON_LOG_WARN, MON_ID_317B ": Missing parameters (need 1, got %u)",
                ctx->arg_count);
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);  /* 157B Missing parameter */
        return MON_ERROR;
    }

    /* TEMP probe: dump the raw command descriptor and the memory it points at,
     * to map NC's UECOM argument layout (the CAT-500 invocation). */
    if (getenv("ND500X_UECOM_DUMP")) {
        uint32_t da = ctx->arg_addresses[0];
        mon_log(MON_LOG_WARN, MON_ID_317B ": arg0 desc @0x%08X", da);
        for (int w = 0; w < 4; w++) {
            uint32_t v = ctx->read_word(ctx->cpu, da + (uint32_t)(w*4));
            mon_log(MON_LOG_WARN, "   desc[%d] @0x%08X = 0x%08X", w, da + (uint32_t)(w*4), v);
        }
        /* Dump 64 bytes at the descriptor address itself (in case it IS the text) */
        char raw[65];
        for (int i = 0; i < 64; i++) {
            uint8_t b = ctx->read_byte(ctx->cpu, da + (uint32_t)i);
            raw[i] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
        }
        raw[64] = 0;
        mon_log(MON_LOG_WARN, "   bytes@desc: \"%s\"", raw);
        /* Dump what each descriptor word points at (candidates for the command text) */
        for (int w = 0; w < 3; w++) {
            uint32_t p = ctx->read_word(ctx->cpu, da + (uint32_t)(w*4));
            if (!p) continue;
            for (int i = 0; i < 64; i++) {
                uint8_t b = ctx->read_byte(ctx->cpu, p + (uint32_t)i);
                raw[i] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
            }
            raw[64] = 0;
            mon_log(MON_LOG_WARN, "   *desc[%d]=0x%08X: \"%s\"", w, p, raw);
        }
    }

    /* Read command string. NC passes the command as a [Length:4][Pointer:4]
     * descriptor (arg0 = {maxlen, cmd_ptr, ...}; the command text lives at the
     * pointer and is 0x27-terminated), e.g. "NC-A", "CAT-CAT5-B". Try that
     * descriptor form first; fall back to a direct read for callers that pass
     * the string inline. */
    char command[256];
    if (mon_read_descriptor_string(ctx, 0, command, sizeof(command)) <= 0) {
        mon_read_sintran_string(ctx, 0, command, sizeof(command));
    }

    mon_log(MON_LOG_INFO, MON_ID_317B ": IN: Command='%s'", command);

    /* Nested subsystem invocation (e.g. NC's CAT-500 back-end "CAT-CAT5-B"):
     * if the frontend registered a runner, let it resolve+run the named program
     * re-entrantly (sharing this file table) so the back-end actually produces
     * its output. Contract: 0=ran ok, <0=not a known program (fall back to the
     * benign stub), >0=program ran but failed. */
    if (g_exec_cmd) {
        int r = g_exec_cmd(ctx->cpu, ctx->machine, command);
        if (r == 0) {
            mon_log(MON_LOG_INFO, MON_ID_317B ": nested '%s' completed", command);
            mon_set_success(ctx);
            return MON_SUCCESS;
        }
        if (r > 0) {
            mon_log(MON_LOG_WARN, MON_ID_317B ": nested '%s' failed (%d)", command, r);
            mon_set_error(ctx, MON_ERR_ILLEGAL_PARAMETER);  /* 174B */
            return MON_ERROR;
        }
        /* r < 0: not a known program - fall through to benign stub success. */
    }

    mon_set_success(ctx);
    return MON_SUCCESS;
}
