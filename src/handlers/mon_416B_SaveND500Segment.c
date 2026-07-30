/*
 * MON 416B (270 decimal): SaveND500Segment (WSEGN)
 *
 * "Writes all modified pages of a segment back to the disk."
 *  - Not allowed when the segment is fixed in memory.
 *
 * Parameters:
 *   [I] LogSegmentNo (INTEGER2): Logical segment number in the domain. If 0,
 *       every segment mapped in this process is flushed.
 *   [I] FirstPage   (INTEGER2): First logical page in the segment (advisory).
 *   [I] LastPage    (INTEGER2): Last  logical page in the segment (advisory).
 *
 * nd500x model (MON 412B FileAsSegment is the oracle for how a segment reaches
 * the disk here): a logical segment is backed by an OPEN host file, and ordinary
 * writes go straight through to that host file (write-through, same rationale as
 * 327B fn1 WRBIX). So "write modified pages back to disk" reduces to flushing the
 * host FILE handle of the file(s) connected as this segment - exactly the
 * `fflush(entry->host_file)` that 412B does before mapping. FirstPage/LastPage
 * are advisory: write-through means there is never a partially-dirty page to
 * bound, so we always flush the whole backing file.
 *
 * If no file is connected as the requested segment there is nothing dirty to
 * force (writes already went through), so we still report success rather than
 * failing the caller. This is what unblocks BM-FILERE-B02, which issues WSEGN
 * early to persist its working segment.
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_errors.h"
#include "mon_file_table.h"

MonResult mon_416B_SaveND500Segment(MonContext* ctx) {
    if (ctx->arg_count < 1) {
        mon_log(MON_LOG_WARN, "MON 416B WSEGN: missing LogSegmentNo");
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);   /* 157B */
        return MON_ERROR;
    }

    uint32_t log_segment_no = mon_read_param_word(ctx, 0);
    MON_LOG_IN_WORD(ctx, 0, "LogSegmentNo");
    if (ctx->arg_count > 1) MON_LOG_IN_WORD(ctx, 1, "FirstPage");
    if (ctx->arg_count > 2) MON_LOG_IN_WORD(ctx, 2, "LastPage");

    /* Flush every open file connected as this segment (LogSegmentNo==0 -> all).
     * Writes are already write-through, so this makes the host file bytes durable
     * the same way 412B does before mapping. */
    int flushed = 0;
    for (int fn = FILE_NUMBER_MIN; fn <= FILE_NUMBER_MAX; fn++) {
        OpenFileEntry* e = mon_file_table_get(fn);
        if (!e || !e->in_use || !e->mapped_as_segment) continue;
        if (log_segment_no != 0 && e->mapped_segment_no != log_segment_no) continue;
        if (e->host_file) {
            fflush(e->host_file);
            flushed++;
        }
    }

    mon_log(MON_LOG_DEBUG,
            "MON 416B WSEGN: LogSegmentNo=%o -> flushed %d mapped file(s) (write-through)",
            log_segment_no, flushed);

    mon_set_success(ctx);
    return MON_SUCCESS;
}
