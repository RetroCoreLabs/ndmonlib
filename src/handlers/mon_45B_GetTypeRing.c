/*
 * MON 45B (37 decimal) - GTYPR (Get Type Ring), ND-500 monitor call.
 *
 * Source: ND-60.136.04A ND-500 Loader Monitor, MON-call table (45B GTYPR
 *   <unit> <typing> <status> <Sintran III open file number>); its purpose is to
 *   hand back the SINTRAN III file number that DVINST (MON 503B) needs, plus a
 *   TYPING word (flag bits + open file number) and a status word. Equivalent to
 *   FSMTY (MON 327B) function 4 (ND-60230-5-EN K-version, section 16.6.4:9475).
 *
 * IMPLEMENTATION NOTE (see Developer/MON/calls/45B_GetTypeRing.yaml):
 *   The carved L07 dispatch actually routes an ND-500 45B to the native DBRK
 *   worker (byte-verified: MCHANDEL falls through GO NORMMC -> MCTAB[45B]=BDBRK;
 *   N5MPA patch slot is the no-op default). But the FraTor ND-500 tools EMIT 45B
 *   with GTYPR's 4-arg signature and consume the returned file number, so nd500x
 *   services it as GTYPR (decision recorded 2026-07-30) to make those tools run.
 *
 *   On nd500x the ND-500 open-file number IS the SINTRAN file number (the
 *   file-table key), so the returned <Sintran III open file number> is the input
 *   <unit> (identity). The full TYPING flag-bit layout is not byte-proven, so we
 *   expose the open-file number in it as a best-effort value (marked below).
 */

#include "ndmon/mon.h"
#include "ndmon/mon_log.h"
#include "ndmon/mon_file_table.h"
#include "ndmon/mon_errors.h"

/*
 * Shared GTYPR core, also called by MON 327B FSMTY function 4 (which shifts the
 * argument indices by one for its leading function-code word). Pass -1 for any
 * output index the caller did not supply. Fills:
 *   typing  : flag bits + open-file number (best-effort: the open-file number)
 *   status  : bit 0 = open for write, bit 1 = terminal/TAD (ndfs-extensions 3.4)
 *   sfn     : SINTRAN III open file number (= input unit on nd500x, identity)
 */
void mon_gtypr_fill(MonContext* ctx, int unit_idx, int typing_idx,
                    int status_idx, int filenum_idx) {
    uint32_t unit = mon_read_param_word(ctx, unit_idx);
    OpenFileEntry* e = mon_file_table_get((int)unit);

    uint32_t typing = 0;
    uint32_t status = 0;
    uint32_t sfn = unit;   /* identity: file-table key == SINTRAN file number */

    if (e && e->in_use) {
        /* access_mode: 0=read, 1=write, 2=read/write. bit 0 = open for write. */
        if (e->access_mode >= 1) status |= 1u;
        /* bit 1 = spooling file or terminal/TAD (0 for a plain mass-storage file) */
        if (is_terminal(e->object_entry.device_number)) status |= 2u;
        typing = unit;     /* best-effort: TYPING carries the open-file number */
    } else if (is_terminal(unit)) {
        /* <unit> was a device number for a terminal/TAD, not an open file. */
        status |= 2u;
    }

    if (typing_idx  >= 0) mon_write_param_word(ctx, typing_idx,  typing);
    if (status_idx  >= 0) mon_write_param_word(ctx, status_idx,  status);
    if (filenum_idx >= 0) mon_write_param_word(ctx, filenum_idx, sfn);
}

MonResult mon_45B_GetTypeRing(MonContext* ctx) {
    if (ctx->arg_count < 1) {
        mon_log(MON_LOG_WARN, "MON 45B GTYPR: missing parameters (need >=1, got %u)",
                ctx->arg_count);
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);   /* 157B */
        return MON_ERROR;
    }

    mon_gtypr_fill(ctx, 0,
                   ctx->arg_count > 1 ? 1 : -1,
                   ctx->arg_count > 2 ? 2 : -1,
                   ctx->arg_count > 3 ? 3 : -1);

    mon_log(MON_LOG_DEBUG, "MON 45B GTYPR: unit=%u -> sintran-file-number=%u",
            mon_read_param_word(ctx, 0), mon_read_param_word(ctx, 0));
    mon_set_success(ctx);
    return MON_SUCCESS;
}
