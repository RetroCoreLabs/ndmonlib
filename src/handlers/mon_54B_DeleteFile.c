/*
 * MON 54B [MDLFI/DeleteFile]
 *
 * Deletes a file. The pages of the file are released.
 *
 * - You must have directory access to the file in order to delete it.
 * - RT programs can delete a file if user RT has directory access to it.
 * - Include a version number in the file name to delete specific versions.
 *
 * Parameters:
 *   [I] FileName (STRING): File name to delete
 *
 * Returns:
 *   K flag set on error, error code in W1:
 *     46 = File not found
 *     52 = Invalid parameter
 *
 * Reference: SINTRAN III Monitor Calls (ND-860228.2 EN)
 */

#include "mon.h"
#include "mon_log.h"
#include "mon_errors.h"
#include "mon_file_table.h"
#include <stdio.h>
#include <string.h>

MonResult mon_54B_DeleteFile(MonContext* ctx) {
    /* Defensive check for argument count */
    if (ctx->arg_count < 1) {
        mon_log(MON_LOG_WARN, MON_ID_54B ": Missing parameters (need 1, got %u)",
                ctx->arg_count);
        mon_set_error(ctx, MON_ERR_MISSING_PARAMETER);  /* 157B Missing parameter */
        return MON_ERROR;
    }

    /* Read filename string.
     * ND-500 passes filename args as DESCRIPTORS ([len, ptr]) - the same shape
     * MON 50B OPEN reads via mon_read_descriptor_string. The old raw-inline
     * mon_read_sintran_string read the descriptor words as characters and got an
     * empty string, so NC's scratch cleanup/finalization (its 54B deletes) all
     * failed silently. Read as a descriptor first; fall back to the raw reader
     * only if the descriptor is not usable. */
    char filename[65];
    int dn = mon_read_descriptor_string(ctx, 0, filename, 65);
    if (dn <= 0 || filename[0] == '\0') {
        mon_read_sintran_string(ctx, 0, filename, 65);
    }

    mon_log(MON_LOG_DEBUG, MON_ID_54B ": IN: FileName='%s'", filename);

    /* Validate filename */
    if (filename[0] == '\0') {
        mon_log(MON_LOG_WARN, MON_ID_54B ": Empty filename");
        mon_set_error(ctx, MON_ERR_NO_SUCH_FILE_NAME);  /* 056B No such file name */
        return MON_ERROR;
    }

    /* Build host path using shared utility */
    char host_path[256];
    mon_build_host_path(filename, host_path, sizeof(host_path));

    /* Check if file exists */
    FILE* test_fp = fopen(host_path, "rb");
    if (!test_fp) {
        mon_log(MON_LOG_WARN, MON_ID_54B ": File '%s' not found", host_path);
        mon_set_error(ctx, MON_ERR_NO_SUCH_FILE_NAME);  /* 056B No such file name */
        return MON_ERROR;
    }
    fclose(test_fp);

    /* Delete the file */
    if (remove(host_path) != 0) {
        mon_log(MON_LOG_WARN, MON_ID_54B ": Failed to delete file '%s'", host_path);
        mon_set_error(ctx, MON_ERR_TRANSFER_ERROR);  /* 141B Transfer error */
        return MON_ERROR;
    }

    mon_log(MON_LOG_DEBUG, MON_ID_54B ": OUT: Deleted file '%s'", host_path);

    mon_set_success(ctx);
    return MON_SUCCESS;
}
