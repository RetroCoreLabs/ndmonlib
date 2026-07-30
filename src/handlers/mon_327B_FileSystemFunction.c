/*
 * MON 327B (215 decimal) - FSMTY (File System Multifunction), ND-100/ND-500.
 *
 * A multifunction call; the function is selected by the first argument (ND-500
 * CALLG) / the T-register (ND-100). Introduced J-version, extended J..M.
 * Sources: ND-860228-2-EN SINTRAN III Monitor Calls (327B FSMTY); the SINTRAN
 * release docs (ndfs-extensions.md section 3, function table 1..13B). See
 * Developer/MON/calls/327B_FileSystemFunction.yaml.
 *
 * Functions serviced here (the ones the ND-500 tools use, plus the cheap ones):
 *   1 WRBIX - write back index blocks: no-op on nd500x (writes go through the
 *             host file immediately), so it reports success.
 *   2 return block size of an open file (in bytes).
 *   3 get full file name of an open file (into a buffer descriptor).
 *   4 get file/device info == GTYPR (MON 45B); shares mon_gtypr_fill().
 * Other functions (5..13B) are directory/security operations not exercised by
 * these tools; they return benign success rather than trapping.
 */

#include "ndmon/mon.h"
#include "ndmon/mon_log.h"
#include "ndmon/mon_file_table.h"
#include "ndmon/mon_errors.h"

/* Shared GTYPR core, defined in mon_45B_GetTypeRing.c. */
extern void mon_gtypr_fill(MonContext* ctx, int unit_idx, int typing_idx,
                           int status_idx, int filenum_idx);

/* Copy a 0x27-terminated SINTRAN field (object_name / type) into a C string. */
static int copy_sintran_field(const char* src, int max, char* dst) {
    int i = 0;
    for (; i < max; i++) {
        char c = src[i];
        if (c == 0x27 /* ' */ || c == '\0' || (unsigned char)c < 0x20) break;
        dst[i] = c;
    }
    dst[i] = '\0';
    return i;
}

MonResult mon_327B_FileSystemFunction(MonContext* ctx) {
    if (ctx->arg_count < 1) {
        mon_log(MON_LOG_WARN, "MON 327B FSMTY: missing function code");
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);   /* 157B */
        return MON_ERROR;
    }

    uint32_t func = mon_read_param_word(ctx, 0);

    switch (func) {
    case 1:   /* WRBIX - flush index blocks. nd500x writes through -> nothing to do. */
        mon_log(MON_LOG_DEBUG, "MON 327B FSMTY fn 1 (WRBIX): no-op (write-through)");
        mon_set_success(ctx);
        return MON_SUCCESS;

    case 2: {  /* [1] open file no, [2] returned block size in bytes */
        uint32_t u = mon_read_param_word(ctx, 1);
        OpenFileEntry* e = mon_file_table_get((int)u);
        uint32_t bs = (e && e->in_use) ? e->block_size : 512u;
        if (ctx->arg_count > 2) mon_write_param_word(ctx, 2, bs);
        mon_log(MON_LOG_DEBUG, "MON 327B FSMTY fn 2 (block size): file=%u -> %u bytes", u, bs);
        mon_set_success(ctx);
        return MON_SUCCESS;
    }

    case 3: {  /* [1] open file no, [2] buffer descriptor to receive NAME:TYPE */
        uint32_t u = mon_read_param_word(ctx, 1);
        OpenFileEntry* e = mon_file_table_get((int)u);
        if (!e || !e->in_use) {
            mon_log(MON_LOG_WARN, "MON 327B FSMTY fn 3 (get name): bad file number %u", u);
            mon_set_error(ctx, MON_ERR_BAD_FILE_NUMBER);   /* 2B */
            return MON_ERROR;
        }
        char name[24];
        char ext[8];
        char full[40];
        copy_sintran_field(e->object_entry.object_name, 16, name);
        int tn = copy_sintran_field(e->object_entry.type, 4, ext);
        if (tn > 0)
            snprintf(full, sizeof full, "%s:%s", name, ext);
        else
            snprintf(full, sizeof full, "%s", name);
        if (ctx->arg_count > 2) mon_write_string(ctx, 2, full);
        mon_log(MON_LOG_DEBUG, "MON 327B FSMTY fn 3 (get name): file=%u -> '%s'", u, full);
        mon_set_success(ctx);
        return MON_SUCCESS;
    }

    case 4:   /* file/device info == GTYPR: [1] unit, [2] typing, [3] status, [4] sfn */
        mon_gtypr_fill(ctx, 1,
                       ctx->arg_count > 2 ? 2 : -1,
                       ctx->arg_count > 3 ? 3 : -1,
                       ctx->arg_count > 4 ? 4 : -1);
        mon_log(MON_LOG_DEBUG, "MON 327B FSMTY fn 4 (GTYPR): unit=%u", mon_read_param_word(ctx, 1));
        mon_set_success(ctx);
        return MON_SUCCESS;

    default:
        /* Directory / security / iteration functions (5..13B) - not exercised by
         * the ND-500 tools. Report success (benign) rather than trapping the DOM. */
        mon_log(MON_LOG_WARN, "MON 327B FSMTY: function %uB not implemented (benign success)", func);
        mon_set_success(ctx);
        return MON_SUCCESS;
    }
}
