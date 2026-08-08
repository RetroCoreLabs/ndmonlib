/*
 * MON 113B [CLOCK/GetCurrentTime]
 *
 * Gets the current system time and date.
 *
 * - The current system time is returned as basic time units, seconds, minutes,
 *   hours, day, month, and year.
 * - Basic time units = 50 per second (20ms each).
 *
 * Parameters:
 *   [O] TimeBuffer (ARRAY of 7 INTEGERs):
 *       [0] Basic time units since midnight
 *       [1] Seconds (0-59)
 *       [2] Minutes (0-59)
 *       [3] Hours (0-23)
 *       [4] Day (1-31)
 *       [5] Month (1-12)
 *       [6] Year, written as the full year (e.g. 1985). INFERRED, not proven -
 *           see the note at the write below before relying on or changing it
 *
 * Reference: SINTRAN III Monitor Calls (ND-860228.2 EN). It lists the seventh
 * word only as "%Year." and never states a width, so the manual settles
 * nothing here either way.
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_errors.h"
#include "mon_clock.h"

/* Basic time units per second */
#define BASIC_TIME_UNITS_PER_SEC 50

MonResult mon_113B_GetCurrentTime(MonContext* ctx) {
    /* Defensive check for argument count */
    if (ctx->arg_count < 1) {
        mon_log(MON_LOG_WARN, MON_ID_113B ": Missing parameters (need 1, got %u)",
                ctx->arg_count);
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);  /* 157B Missing parameter */
        return MON_ERROR;
    }

    /* Get output buffer address */
    uint32_t buffer_addr = ctx->arg_addresses[0];

    /* Get current time via the MON clock (pinned UTC instant in deterministic
     * mode, else real host localtime). */
    time_t now = mon_clock_now();
    struct tm tm_storage;
    struct tm* tm_now = mon_clock_breakdown(now, &tm_storage);

    if (!tm_now) {
        mon_log(MON_LOG_WARN, MON_ID_113B ": Failed to get local time");
        mon_set_error(ctx, MON_ERR_ILLEGAL_PARAMETER);  /* 174B Illegal parameter */
        return MON_ERROR;
    }

    /* Calculate basic time units since midnight
     * = hours * 3600 * 50 + minutes * 60 * 50 + seconds * 50
     */
    uint32_t basic_units = (uint32_t)(tm_now->tm_hour * 3600 + tm_now->tm_min * 60 + tm_now->tm_sec)
                         * BASIC_TIME_UNITS_PER_SEC;

    /* Write 7 words to buffer */
    /* [0] Basic time units since midnight */
    ctx->write_word(ctx->cpu, buffer_addr + 0, basic_units);

    /* [1] Seconds (0-59) */
    ctx->write_word(ctx->cpu, buffer_addr + 4, (uint32_t)tm_now->tm_sec);

    /* [2] Minutes (0-59) */
    ctx->write_word(ctx->cpu, buffer_addr + 8, (uint32_t)tm_now->tm_min);

    /* [3] Hours (0-23) */
    ctx->write_word(ctx->cpu, buffer_addr + 12, (uint32_t)tm_now->tm_hour);

    /* [4] Day (1-31) */
    ctx->write_word(ctx->cpu, buffer_addr + 16, (uint32_t)tm_now->tm_mday);

    /* [5] Month (1-12) - tm_mon is 0-11 */
    ctx->write_word(ctx->cpu, buffer_addr + 20, (uint32_t)(tm_now->tm_mon + 1));

    /* [6] Year, written as the full year (e.g. 1985).
     *
     * INFERRED, NOT PROVEN. Read the whole note before changing this either
     * way. This returned tm_year % 100 until 2026-08-08.
     *
     * What is settled:
     *   - This buffer is seven separate words, not the packed 32-bit ND date
     *     word used for file-system dates (that one crams year/month/day/
     *     hour/minute/second into one word, year as a 6-bit offset from 1950).
     *     The MON 113B manual page lists the seven words individually, and
     *     MP-P2-N500.NPL from line 1310 copies all seven resident clock words
     *     verbatim to the ND-500, widening each to 32 bits and converting
     *     nothing. So the packed format does not apply here.
     *
     * What is NOT settled - the width of this word:
     *   - The manual never states it. The MON 113B page gives the seventh word
     *     only as "%Year.", and MON 111B SetClock likewise says only "Year.".
     *   - The one piece of evidence either way is a single line of SINTRAN.
     *     RP-P2-ACCRT.NPL packs an accounting record right after the call:
     *         "TPARA+ACBASE"; *MON 2CLOC     % GET TIME OF DUMP    (line 184)
     *         D:=0; A:=YEAR-3554/\77; ...                          (line 186)
     *     TPARA is this same seven-word buffer (declared above it as BUNI,
     *     SEC, MINUT, HOUR, DAY, MNTH, YEAR). The radix there is octal - the
     *     mask 77 is 63, six contiguous bits, whereas 77 decimal would be the
     *     nonsense mask 1001101 - so 3554 is 1900. Subtracting 1900 from the
     *     returned value only makes sense if it is a full year: on 85 it goes
     *     negative. That is the whole argument.
     *   - Against it: that line uses a base of 1900, while the file-date
     *     format uses 1950, so the two are not one consistent convention.
     *   - Do not cite the MON YAML corpus. Its "last two digits (tm_year %
     *     100)" note quotes this C file, so it was written from the emulator.
     *
     * No observation distinguishes the two. For any 1980s date, a caller that
     * prefixes "19" to the low two digits prints the same thing whichever it
     * received. The ND LINKER does exactly that, which is why its banner still
     * reads 1926 with this fix in place.
     *
     * What would settle it: a capture from a real SINTRAN III system showing a
     * date rendered from this call, or the resident clock's own year cell read
     * out of a live system or a memory image.
     */
    ctx->write_word(ctx->cpu, buffer_addr + 24, (uint32_t)(tm_now->tm_year + 1900));

    mon_log(MON_LOG_DEBUG, MON_ID_113B ": IN: (none)");
    mon_log(MON_LOG_DEBUG, MON_ID_113B ": OUT: Time=%02d:%02d:%02d Date=%02d/%02d/%04d BasicUnits=%o",
            tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
            tm_now->tm_mday, tm_now->tm_mon + 1, (tm_now->tm_year + 1900),
            basic_units);

    mon_set_success(ctx);
    return MON_SUCCESS;
}
