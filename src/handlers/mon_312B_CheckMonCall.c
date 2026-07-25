/*
 * MON 312B [MOINF/CheckMonCall]
 *
 * Some monitor calls are optional or only available in later versions of
 * SINTRAN III. This monitor call checks if a monitor call exists in your
 * particular SINTRAN III system.
 *
 * Parameters:
 *   [I] MonCallNumber (INTEGER): MON call number to check
 *   [O] MonCallEntry (INTEGER): Address of entry (0 if exists, non-zero if not)
 *
 * Note: Result is written to OUTPUT parameter, NOT W1.
 *
 * Reference: SINTRAN III Monitor Calls (ND-860228.2 EN)
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_errors.h"

/*
 * Real SINTRAN III VSX-500 L07 Monitor-Call TABle (MCTAB / 9MCTA @ 005620B),
 * carved byte-for-byte from segment 044-S3IDPIT (L-VSX-500 image, MCTAB[0] at
 * file byte offset 1824, 256 big-endian words). MOINF/312B returns MCTAB[N]:
 * the call's dispatch-entry address, or 0 if the call is not generated into this
 * system. 216 of 256 slots are populated; every populated slot lands on a named
 * L07 symbol (e.g. MCTAB[312B]=032600B MOINF, MCTAB[317B]=050701B UECOM,
 * MCTAB[321B]=065453B UEADM - all oracle-verified).
 *
 * This REPLACES the earlier fake (0xF8000000+num), which the L07 oracle
 * (mon-oracle-for-NC/312B-MOINF_317B-UECOM.md) explicitly flagged as wrong: MOINF
 * returns a 16-bit entry word or 0, never a 32-bit sentinel, and NC dereferences
 * the entry for some calls (e.g. 321B UEADM), so the true value is required.
 */
static const uint16_t MCTAB_L07[256] = {
    /* 000B */ 0x35BB, 0x2D7E, 0x2D80, 0x4960, 0x4915, 0x8411, 0x8413, 0x2D7A,
    /* 010B */ 0x2D7C, 0x41E7, 0x51B6, 0x4850, 0x4855, 0x870F, 0x4998, 0x4996,
    /* 020B */ 0x8733, 0x37DC, 0x376F, 0x37DE, 0x3771, 0x8782, 0x4A4A, 0x3F04,
    /* 030B */ 0x741E, 0x4A1F, 0x852B, 0x3EA6, 0x3ECB, 0x52FF, 0x49E3, 0x0000,
    /* 040B */ 0x6F7A, 0x881D, 0x861F, 0x86ED, 0x8A08, 0x049D, 0x04A5, 0x04BC,
    /* 050B */ 0x861C, 0x18B3, 0x4D5D, 0x409A, 0x8C33, 0x8C8A, 0x84F3, 0x84F5,
    /* 060B */ 0x310E, 0x7D60, 0x87F7, 0x37E0, 0x1DCC, 0x1DCE, 0x4883, 0x4899,
    /* 070B */ 0x51BB, 0x4E10, 0x4E12, 0x87C6, 0x87D0, 0x8805, 0x87EA, 0x87DD,
    /* 100B */ 0x3D90, 0x4138, 0x4157, 0x4124, 0x41A5, 0x3DD1, 0x4CFB, 0x4D38,
    /* 110B */ 0x3D9C, 0x4267, 0x421F, 0x41EE, 0x42C3, 0x6E58, 0x6EE1, 0x2D05,
    /* 120B */ 0x2D07, 0x47CF, 0x3E43, 0x3E6E, 0x3E3E, 0x3E67, 0x413F, 0x4189,
    /* 130B */ 0x412B, 0x3816, 0x6CED, 0x6D08, 0x35BB, 0x35A8, 0x7425, 0x742A,
    /* 140B */ 0x3E8B, 0x4811, 0x73FE, 0x5318, 0x2CEC, 0x0000, 0x9617, 0x9634,
    /* 150B */ 0x965C, 0x4316, 0x43E5, 0x966C, 0x9676, 0x0000, 0x96A1, 0x6FF4,
    /* 160B */ 0x7CE2, 0x9518, 0x95B9, 0x0000, 0x735F, 0x0000, 0x0000, 0x6F93,
    /* 170B */ 0x96D1, 0x972A, 0x9788, 0x97C4, 0x97F8, 0x0000, 0x0000, 0x0000,
    /* 200B */ 0x0000, 0x8576, 0x0000, 0x0000, 0x896C, 0x8A3D, 0x84B5, 0x850B,
    /* 210B */ 0x0000, 0x0000, 0x0000, 0x8A0A, 0x8AC1, 0x881F, 0x8908, 0x8B1A,
    /* 220B */ 0x8616, 0x8B72, 0x532B, 0x0000, 0x0000, 0x0000, 0x0000, 0x4E38,
    /* 230B */ 0x4E3A, 0x8B6D, 0x8C30, 0x8C2A, 0x8C2D, 0x8619, 0x86EB, 0x8B6A,
    /* 240B */ 0x8CC7, 0x8CFF, 0x8D72, 0x8DDC, 0x8E49, 0x8E4C, 0x8F01, 0x8F03,
    /* 250B */ 0x8DDA, 0x9028, 0x86E8, 0x8B70, 0x8555, 0x9850, 0x920D, 0x928A,
    /* 260B */ 0x4E50, 0x4E55, 0x6612, 0x8E44, 0x2CFD, 0x2D01, 0x2CE9, 0x41CB,
    /* 270B */ 0x8F27, 0x8F29, 0x913A, 0x920B, 0x9288, 0x8C23, 0x9110, 0x9112,
    /* 300B */ 0x9139, 0x913B, 0x90C2, 0x90F2, 0x87BD, 0x8891, 0x4DCE, 0x49E6,
    /* 310B */ 0x0000, 0x8E46, 0x3580, 0x9163, 0x9341, 0x6894, 0x2F58, 0x51C1,
    /* 320B */ 0x51D6, 0x6B2B, 0x4314, 0x7236, 0x0000, 0x533E, 0x8D35, 0x9373,
    /* 330B */ 0xA586, 0xB577, 0x8DCD, 0x91F8, 0x8EBB, 0x9036, 0x53E5, 0x6E23,
    /* 340B */ 0x3FE5, 0x6D33, 0xA75C, 0xAA85, 0x9D0F, 0x699E, 0x0000, 0x4E3A,
    /* 350B */ 0x4E85, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /* 360B */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /* 370B */ 0x0000, 0x0000, 0x0000, 0x4EDF, 0x8F1C, 0x8F1E, 0x34E0, 0x34E7,
};

MonResult mon_312B_CheckMonCall(MonContext* ctx) {
    /* Defensive check for argument count */
    if (ctx->arg_count < 1) {
        mon_log(MON_LOG_WARN, MON_ID_312B ": Missing parameters (need 1, got %u)",
                ctx->arg_count);
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);  /* 157B Missing parameter */
        return MON_ERROR;
    }

    /* Read MON call number to check */
    uint32_t mon_number = mon_read_param_word(ctx, 0);

    MON_LOG_IN_WORD(ctx, 0, "MonCallNumber");

    mon_log(MON_LOG_DEBUG, MON_ID_312B ": IN: MonCallNumber=%oB", mon_number);

    /* MOINF is a capability probe: it returns the call's real dispatch entry,
     * MCTAB[N], or 0 if the call is not generated into THIS SINTRAN system. We ARE
     * emulating an L07 VSX-500 system, so we report exactly what that system's
     * carved MCTAB holds - no fabricated value, no dependence on which handlers
     * this emulator happens to implement. 0 = not present; non-zero = present and
     * the 16-bit entry address (which NC dereferences for some calls, e.g. 321B).
     *
     * NOTE (ND-100 vs ND-500 form): the ND-100 MAC ABI signals presence via the
     * skip-return convention; the ND-500 CALLG two-parameter form NC uses writes
     * the entry into the second parameter (0 = not implemented). We follow the
     * ND-500 form and write MCTAB[N] into arg[1]. */
    uint32_t entry = (mon_number < 256) ? MCTAB_L07[mon_number] : 0;

    if (ctx->arg_count >= 2) {
        mon_write_param_word(ctx, 1, entry);
        MON_LOG_OUT_WORD(ctx, 1, "MonCallEntry");
    }

    if (entry != 0) {
        mon_log(MON_LOG_DEBUG, MON_ID_312B ": OUT: MON %oB present (MCTAB=0%oB)",
                mon_number, entry);
    } else {
        mon_log(MON_LOG_DEBUG, MON_ID_312B ": OUT: MON %oB not present (MCTAB=0)", mon_number);
    }

    mon_set_success(ctx);
    return MON_SUCCESS;
}
