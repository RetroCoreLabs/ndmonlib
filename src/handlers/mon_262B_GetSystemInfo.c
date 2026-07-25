/*
 * MON 262B [CPUST/GetSystemInfo]
 *
 * Gets various system information in a 24-byte buffer.
 *
 * Parameters:
 *   [I] Number (INTEGER): System number to query (usually 0)
 *   [O] Buffer (24 bytes):
 *       Bytes 0:1   - System number (16 bits)
 *       Byte 2      - CPU type (2-5 for ND-100 variants, not used for ND-500)
 *       Byte 3      - Instruction set (0-3)
 *       Bytes 4:5   - Microprogram version
 *       Bytes 6:7   - System type (100, 500, 502, 5561, etc.)
 *       Byte 8      - Operating system (1=VSE, 2=VSE-500, 3=RTP, 4=VSX, 5=VSX-500)
 *       Byte 9      - OS version (ASCII A-Z)
 *       Bytes 10:11 - Reserved
 *       Bytes 12:13 - Patch level indicator
 *       Bytes 14:15 - Generation time (minutes)
 *       Bytes 16:17 - Generation time (hours)
 *       Bytes 18:19 - Generation time (day)
 *       Bytes 20:21 - Generation time (month)
 *       Bytes 22:23 - Generation time (year)
 *
 * Reference: SINTRAN III Monitor Calls (ND-860228.2 EN)
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_errors.h"

MonResult mon_262B_GetSystemInfo(MonContext* ctx) {
    /* Defensive check for argument count */
    if (ctx->arg_count < 2) {
        mon_log(MON_LOG_WARN, MON_ID_262B ": Missing parameters (need 2, got %u)",
                ctx->arg_count);
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);  /* 157B Missing parameter */
        return MON_ERROR;
    }

    /* Read system number (not really used, but log it) */
    uint32_t sys_num = mon_read_param_word(ctx, 0);
    uint32_t buffer_addr = ctx->arg_addresses[1];

    MON_LOG_IN_WORD(ctx, 0, "Number");

    mon_log(MON_LOG_DEBUG, MON_ID_262B ": IN: SysNum=%o", sys_num);

    /* Write 24-byte buffer */
    /* Bytes 0:1 - System number (16 bits) */
    ctx->write_byte(ctx->cpu, buffer_addr + 0, 0);
    ctx->write_byte(ctx->cpu, buffer_addr + 1, 1);  /* System 1 */

    /* Bytes 2:3 describe the FRONT-END ND-100-family CPU that runs SINTRAN III,
     * NOT the ND-500 coprocessor - the L07 oracle (mon-oracle-for-NC/262B-CPUST.md,
     * sec 5) is explicit that there is no ND-500 encoding for these two bytes and
     * that a real machine reports the front-end identity here. On an ND-5800 the
     * front end is an ND-110/CX-class CPU, so per the manual table:
     *   byte 2 = 4  (ND-110/CX or ND-120/CX, 48-bit FP)
     *   byte 3 = 3  (ND-110/CX / ND-120/CX, 16-PIT micro-segment administration)
     * (Previously 0/0, which is not a valid ND-100-family value.) */
    ctx->write_byte(ctx->cpu, buffer_addr + 2, 4);   /* CPU type: ND-110/CX 48-bit FP */
    ctx->write_byte(ctx->cpu, buffer_addr + 3, 3);   /* Instruction set: 16-PIT micro-seg admin */

    /* Bytes 4:5 - Microprogram version. Report the ND-5800 control-store version
     * 11930 (0x2E9A = MICRO-5800-B30), the same value the sysgen range-check and
     * the octobus VERSION path expect for an ND-5800. */
    ctx->write_byte(ctx->cpu, buffer_addr + 4, (11930 >> 8) & 0xFF);
    ctx->write_byte(ctx->cpu, buffer_addr + 5, 11930 & 0xFF);

    /* Bytes 6:7 - System type = the machine MODEL number. This machine is an
     * ND-5800, so report 5800 here (the field that carries 100/500/502/5561/...
     * per the manual). This is where "CPU type 5800" belongs - not byte 2, which
     * is the front-end CPU-type enum above. */
    ctx->write_byte(ctx->cpu, buffer_addr + 6, (5800 >> 8) & 0xFF);
    ctx->write_byte(ctx->cpu, buffer_addr + 7, 5800 & 0xFF);

    /* Byte 8 - Operating system (5 = SINTRAN III VSX-500) */
    ctx->write_byte(ctx->cpu, buffer_addr + 8, 5);

    /* Byte 9 - OS version (ASCII 'L' = 0x4C for SINTRAN L) */
    ctx->write_byte(ctx->cpu, buffer_addr + 9, 'L');

    /* Bytes 10:11 - Reserved */
    ctx->write_byte(ctx->cpu, buffer_addr + 10, 0);
    ctx->write_byte(ctx->cpu, buffer_addr + 11, 0);

    /* Bytes 12:13 - Patch level indicator */
    ctx->write_byte(ctx->cpu, buffer_addr + 12, 0);
    ctx->write_byte(ctx->cpu, buffer_addr + 13, 0);

    /* Bytes 14:15 - Generation time (minutes) */
    ctx->write_byte(ctx->cpu, buffer_addr + 14, 0);
    ctx->write_byte(ctx->cpu, buffer_addr + 15, 0);

    /* Bytes 16:17 - Generation time (hours) */
    ctx->write_byte(ctx->cpu, buffer_addr + 16, 0);
    ctx->write_byte(ctx->cpu, buffer_addr + 17, 0);

    /* Bytes 18:19 - Generation time (day) */
    ctx->write_byte(ctx->cpu, buffer_addr + 18, 0);
    ctx->write_byte(ctx->cpu, buffer_addr + 19, 1);

    /* Bytes 20:21 - Generation time (month) */
    ctx->write_byte(ctx->cpu, buffer_addr + 20, 0);
    ctx->write_byte(ctx->cpu, buffer_addr + 21, 1);

    /* Bytes 22:23 - Generation time (year) */
    ctx->write_byte(ctx->cpu, buffer_addr + 22, (1990 >> 8) & 0xFF);
    ctx->write_byte(ctx->cpu, buffer_addr + 23, 1990 & 0xFF);

    mon_log(MON_LOG_DEBUG, MON_ID_262B ": OUT: SysType=5800, CPU=ND-110/CX(4/3), MicVer=11930, OS=VSX-500, Version=L");

    mon_set_success(ctx);
    return MON_SUCCESS;
}
