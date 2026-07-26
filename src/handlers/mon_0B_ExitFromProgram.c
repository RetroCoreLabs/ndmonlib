/*
 * MON 0B [LEAVE/ExitFromProgram]
 *
 * Terminates the program. Returns to SINTRAN III. Batch jobs continues with the next command.
 *
 * - Background programs close all files not set permanently open. RT programs do not close any files.
 * - RT programs release all reserved devices.
 *
 * Implementation:
 *   Requests CPU halt to stop execution. In emulator context, this ends the program.
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_file_table.h"

MonResult mon_0B_ExitFromProgram(MonContext* ctx) {
    mon_log(MON_LOG_INFO, MON_ID_0B ": IN: (none)");

    /* "Background programs close all files not set permanently open" (see
     * file header) - this was documented but never implemented: without it,
     * any file the program built by writing through a connected segment
     * (mon_412B_FileAsSegment) is silently discarded, since writeback only
     * happens inside MON 43B CLOSE. RT programs are documented to keep their
     * files open across LEAVE; that distinction isn't tracked yet, so this
     * always closes-with-writeback (correct for the background/batch case
     * this emulator actually runs). */
    mon_file_table_close_all_for_exit(ctx);

    /* Request CPU halt - program is exiting */
    mon_request_halt(ctx, "Program exit (MON 0B LEAVE)");

    mon_log(MON_LOG_INFO, MON_ID_0B ": Program terminated");

    mon_set_success(ctx);
    return MON_SUCCESS;
}
