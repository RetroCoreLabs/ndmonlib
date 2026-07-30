/*
 * MON 320B (208 decimal) - UELOG (User Environment login), ND-100/ND-500.
 *
 * Logs a user into a User Environment (sibling of UECOM 317B / UEADM 321B).
 * Source: ND-860228-2-EN SINTRAN III Monitor Calls line 2614 (name-only);
 * carve tools/.../re/mon-analysis/320B-UELogin/. See
 * Developer/MON/calls/320B_UELogin.yaml.
 *
 * BYTE-VERIFIED non-load-bearing in the FraTor ND-500 tools: both callers
 * (convert-dom-a03 @0802CB59, linker-b01 @B004F42E) execute `clrk` (clear the
 * error flag) immediately after the call and return, DISCARDING the result. So a
 * benign success stub satisfies them. The exact argument meaning is UNVERIFIED
 * (a single word, observed 0x24), so we deliberately do not act on it.
 */

#include "ndmon/mon.h"
#include "ndmon/mon_log.h"

MonResult mon_320B_UELogin(MonContext* ctx) {
    mon_log(MON_LOG_DEBUG,
            "MON 320B UELOG: benign success stub (result discarded by callers; arg=%u)",
            ctx->arg_count > 0 ? mon_read_param_word(ctx, 0) : 0);
    mon_set_success(ctx);
    return MON_SUCCESS;
}
