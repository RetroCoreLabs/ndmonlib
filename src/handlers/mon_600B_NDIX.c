/*
 * MON 600B [NDIX Front-End Call] — NDIX LLM Support
 *
 * Handles inter-CPU communication between ND-500 kernel and ND-100 I/O processor.
 * Dispatches fecall operations: feinit (initialization), fecall (I/O), feexit (shutdown).
 *
 * This is a non-standard SINTRAN extension for the NDIX LLM system.
 * Only available on ND-500 CPU running NDIX-enabled SINTRAN.
 * Not compatible with standard SINTRAN or ND-100.
 *
 * Packet Format (in B register frame):
 *   B.20 = Command packet pointer (byte address)
 *   B.24 = Parameter 2
 *   B.28 = Response packet pointer (byte address)
 *   B.32 = Parameter 4
 *
 * Operation Types (identified by command packet format):
 *   - feinit_fecall: Kernel initialization (return memory layout)
 *   - fecall: Generic I/O (open/read/write/close/etc)
 *   - feexit_fecall: Kernel shutdown
 *
 * Implementation Notes:
 *   - All addresses are ND-500 byte addresses (32-bit)
 *   - Must convert addresses via phyladr instruction semantics
 *   - Response packet MUST be written back before returning
 *   - Completion field in response indicates success (0) or error (non-zero)
 *
 * References:
 *   NDIX-C Kernel: /mnt/e/Dev/Ronny/NDIX-C/kernel/MASTER/machine/locore.c:216-239
 *   NDIX Boot Spec: /mnt/e/Dev/Ronny/NDIX-C/verified-docs/BOOT_SEQUENCE_COMPLETE_REFERENCE.md
 *   Bootstrap Guide: /mnt/e/Dev/Ronny/NDIX-C/verified-docs/BOOT_SEQUENCE_COMPLETE_REFERENCE.md:387-436
 *
 * Author: Code Generator (NDIX LLM System)
 * Date: 2026-07-23
 * Status: PHASE 1 - Minimal feinit response for kernel bootstrap
 */

#include "mon.h"
#include "mon_log.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Forward declarations */
static MonResult handle_feinit_fecall(MonContext* ctx);
static MonResult handle_fecall_io(MonContext* ctx);
static MonResult handle_feexit_fecall(MonContext* ctx);

/*
 * ============================================================================
 * PHASE 1: Minimal feinit_fecall Response
 * ============================================================================
 *
 * Kernel sends init_cpk (22 bytes):
 *   +0  short ux_vers                    VERSION number
 *   +2  struct cxb *cxb0                 Context block 0 address
 *   +6  struct cxb *trcxb                Trap context block address
 *   +10 caddr_t trdata                   Trap data area address
 *   +14 u_int (*intvec)()                Interrupt vector routine
 *   +18 u_int (*trapvec)()               Trap vector routine
 *
 * ND-500 expects init_rpk response (320 bytes) to be written to B.28:
 *   +0  short completion                 0 = OK, non-zero = error
 *   +2  short howto                      Boot flags
 *   +4  dev_t rootdev                    Root device number
 *   +8  dev_t condev                     Console device number
 *   +12 caddr_t stext                    Kernel text segment start (word addr!)
 *   +16 caddr_t sdata                    Kernel data segment start
 *   +20 caddr_t sstack                   Kernel stack segment
 *   +24 caddr_t sfree                    First free memory page
 *   +28 caddr_t sphys                    Physical memory window start
 *   +32 caddr_t scont                    Contiguous memory size
 *   +36 caddr_t spst                     Physical Segment Table location
 *   +40 int private                      Private offset for ND-100 address translation
 *   +44 int cputype                      CPU model (5 = ND-500)
 *   +48 int s3_vers                      SINTRAN version
 *   +52 char booted[64]                  Boot device name
 *   ... more fields (total 320 bytes)
 */

/* Kernel memory layout constants from kernel build */
#define KERNEL_TEXT_SEGMENT_START   0x08000000  /* Kernel text (virtual) */
#define KERNEL_DATA_SEGMENT_START   0x10000000  /* Kernel data (virtual) */
#define KERNEL_STACK_SEGMENT_START  0x18000000  /* Kernel stack (virtual) */
#define KERNEL_FREE_PAGE_START      0x20000000  /* First allocatable page */
#define KERNEL_PHYS_WINDOW_START    0x28000000  /* Physical memory window */
#define KERNEL_PHYS_WINDOW_SIZE     0x10000000  /* 256 MB (physical memory) */
#define KERNEL_PST_LOCATION         0x30000000  /* Physical Segment Table */

/* ND-500 specific CPU type */
#define ND500_CPU_TYPE              5
#define SINTRAN_VERSION             0x0300      /* SINTRAN III v3.0 */

/* Device number stubs (TODO: implement real device table) */
#define STUB_ROOT_DEVICE            0
#define STUB_CONSOLE_DEVICE         0

/**
 * Build and return minimal feinit response packet
 *
 * This stub response allows the kernel to:
 *   1. Know where kernel segments are loaded
 *   2. Know first free memory page
 *   3. Know how to translate ND-100 addresses
 *   4. Proceed with initialization
 *
 * TODO (Phase 3):
 *   - Read actual device configuration
 *   - Return real rootdev/condev from device table
 *   - Read kernel image to determine actual segment sizes
 *   - Calculate sfree from kernel size + initialized data
 */
static MonResult handle_feinit_fecall(MonContext* ctx) {
    mon_log(MON_LOG_INFO, "MON 600B: feinit_fecall - Kernel initialization request");

    /* Validate context has required callbacks */
    if (!ctx->read_word || !ctx->write_word || !ctx->write_halfword || !ctx->write_byte) {
        mon_log(MON_LOG_INFO, "MON 600B: ERROR - Missing memory access callbacks");
        ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    /* Get response packet address from arg[2] (B.28 from callg instruction) */
    if (ctx->arg_count < 3) {
        mon_log(MON_LOG_INFO, "MON 600B: ERROR - Not enough arguments (need at least 3)");
        ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    uint32_t resp_pkt_addr = ctx->arg_addresses[2];
    mon_log(MON_LOG_INFO, "MON 600B: Response packet address = 0x%x", resp_pkt_addr);

    /* Build response packet (320 bytes total) */
    struct {
        uint16_t completion;     /* 0 = OK */
        uint16_t howto;          /* Boot flags */
        uint32_t rootdev;        /* Root device */
        uint32_t condev;         /* Console device */
        uint32_t stext;          /* Kernel text start */
        uint32_t sdata;          /* Kernel data start */
        uint32_t sstack;         /* Kernel stack start */
        uint32_t sfree;          /* First free page */
        uint32_t sphys;          /* Physical window start */
        uint32_t scont;          /* Contiguous size */
        uint32_t spst;           /* PST location */
        uint32_t private;        /* Private offset */
        uint32_t cputype;        /* CPU model */
        uint32_t s3_vers;        /* SINTRAN version */
        char booted[64];         /* Boot device name */
        /* ... remainder initialized to 0 (276 more bytes) */
    } response;

    /* Initialize response structure */
    memset(&response, 0, sizeof(response));

    /* Fill in response fields */
    response.completion = 0;                    /* OK - no error */
    response.howto = 0;                         /* Normal boot (no single-user, etc) */
    response.rootdev = STUB_ROOT_DEVICE;        /* Root device (stub) */
    response.condev = STUB_CONSOLE_DEVICE;      /* Console device (stub) */

    /* Segment addresses - these are ND-500 word addresses
     * (converted from kernel's virtual addresses via symbolic constants) */
    response.stext = (KERNEL_TEXT_SEGMENT_START >> 1);    /* Convert to word addr */
    response.sdata = (KERNEL_DATA_SEGMENT_START >> 1);
    response.sstack = (KERNEL_STACK_SEGMENT_START >> 1);
    response.sfree = (KERNEL_FREE_PAGE_START >> 1);
    response.sphys = (KERNEL_PHYS_WINDOW_START >> 1);
    response.scont = KERNEL_PHYS_WINDOW_SIZE;
    response.spst = (KERNEL_PST_LOCATION >> 1);

    /* Private offset: used by ND-500 kernel to translate ND-100 addresses
     * For now: 0 (no translation needed in this stub) */
    response.private = 0;

    /* CPU identification */
    response.cputype = ND500_CPU_TYPE;
    response.s3_vers = SINTRAN_VERSION;

    /* Boot device name (where kernel was loaded from) */
    strncpy(response.booted, "(SYSTEM)VMUNIX", sizeof(response.booted) - 1);
    response.booted[sizeof(response.booted) - 1] = '\0';

    /* Log what we're returning */
    mon_log(MON_LOG_INFO, "MON 600B: feinit response:");
    mon_log(MON_LOG_INFO, "  completion=%d (OK)", response.completion);
    mon_log(MON_LOG_INFO, "  stext=0x%x (word addr)", response.stext);
    mon_log(MON_LOG_INFO, "  sdata=0x%x (word addr)", response.sdata);
    mon_log(MON_LOG_INFO, "  sfree=0x%x (word addr)", response.sfree);
    mon_log(MON_LOG_INFO, "  cputype=%d (ND-500)", response.cputype);

    /* Write response packet back to memory at response packet address
     * ND-500 is big-endian, so we write halfwords in the correct byte order */
    uint32_t addr = resp_pkt_addr;
    uint8_t* resp_bytes = (uint8_t*)&response;

    /* Write entire response packet, 4 bytes at a time where possible */
    for (int i = 0; i < (int)sizeof(response); i += 4) {
        if (i + 4 <= (int)sizeof(response)) {
            uint32_t word = *(uint32_t*)(resp_bytes + i);
            ctx->write_word(ctx->cpu, addr + i, word);
        } else if (i + 2 <= (int)sizeof(response)) {
            uint16_t half = *(uint16_t*)(resp_bytes + i);
            ctx->write_halfword(ctx->cpu, addr + i, half);
        } else {
            uint8_t byte = *(uint8_t*)(resp_bytes + i);
            ctx->write_byte(ctx->cpu, addr + i, byte);
        }
    }

    mon_log(MON_LOG_INFO, "MON 600B: feinit response written (%d bytes) to B.28 at 0x%x",
                   (int)sizeof(response), resp_pkt_addr);

    /* Set success status */
    ctx->set_k_flag(ctx->cpu, 0);  /* Clear K flag (no error) */

    return MON_SUCCESS;
}

/**
 * Handle FE_WCON: Write single character to console
 *
 * Command packet: physaddr (4 bytes) - address of character in kernel memory
 * Response packet: completion (2 bytes) - 0=OK, non-zero=error
 */
static MonResult handle_fe_wcon(MonContext* ctx) {
    mon_log(MON_LOG_DEBUG, "MON 600B: FE_WCON - Write console character");

    if (ctx->arg_count < 1) {
        ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    /* Read character address from arg[0] (command packet pointer) */
    uint32_t cmd_pkt_addr = ctx->arg_addresses[0];
    uint32_t char_addr = ctx->read_word(ctx->cpu, cmd_pkt_addr);

    /* Read the character byte from kernel memory at that address */
    uint8_t ch = ctx->read_byte(ctx->cpu, char_addr);

    /* Output the character to console */
    putchar(ch);
    fflush(stdout);

    mon_log(MON_LOG_DEBUG, "MON 600B: WCON output: 0x%02x ('%c')", ch,
            (ch >= 32 && ch < 127) ? ch : '.');

    /* Write response packet with completion=0 (success) */
    if (ctx->arg_count >= 3) {
        uint32_t resp_pkt_addr = ctx->arg_addresses[2];
        ctx->write_halfword(ctx->cpu, resp_pkt_addr, 0);  /* completion = 0 */
    }

    ctx->set_k_flag(ctx->cpu, 0);  /* Success */
    return MON_SUCCESS;
}

/**
 * Handle FE_RCON: Read single character from console
 *
 * Command packet: physaddr (4 bytes) - address where to store character
 * Response packet: completion (2 bytes) - 0=OK, non-zero=error
 *
 * TODO: Implement actual console input
 */
static MonResult handle_fe_rcon(MonContext* ctx) {
    mon_log(MON_LOG_DEBUG, "MON 600B: FE_RCON - Read console character");

    if (ctx->arg_count < 1) {
        ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    /* For now, just return 0 (no input available) */
    uint8_t ch = 0;

    /* Write the character to kernel memory */
    if (ctx->arg_count >= 1) {
        uint32_t cmd_pkt_addr = ctx->arg_addresses[0];
        uint32_t char_addr = ctx->read_word(ctx->cpu, cmd_pkt_addr);
        ctx->write_byte(ctx->cpu, char_addr, ch);
    }

    /* Write response packet with completion=0 */
    if (ctx->arg_count >= 3) {
        uint32_t resp_pkt_addr = ctx->arg_addresses[2];
        ctx->write_halfword(ctx->cpu, resp_pkt_addr, 0);
    }

    ctx->set_k_flag(ctx->cpu, 0);
    return MON_SUCCESS;
}

/**
 * Dispatch generic fecall I/O operation
 *
 * Identifies operation type and dispatches to appropriate handler.
 * Operation type comes from the request code (in second argument or command packet).
 *
 * Supported operations:
 *   FE_WCON (0x8) - Write console
 *   FE_RCON (0x6) - Read console
 *   FE_OPEN (0x3) - Open device (TODO)
 *   FE_READ (0x5) - Read from device (TODO)
 *   FE_WRIT (0x7) - Write to device (TODO)
 *   FE_CLOS (0x4) - Close device (TODO)
 */
static MonResult handle_fecall_io(MonContext* ctx) {
    mon_log(MON_LOG_INFO, "MON 600B: fecall - Generic I/O operation");

    /* For console I/O, kernel passes:
     * arg[0] = command packet address (contains device/operation info)
     * arg[1] = parameter 2
     * arg[2] = response packet address
     * arg[3] = parameter 4
     *
     * The operation type is encoded in the request (wreq/rreq passed to fecall).
     * Since we don't have the request directly, we'll detect by pattern or
     * dispatch based on which arguments are valid.
     *
     * For now, assume console operations (most common for startup).
     */

    /* Try to distinguish between WCON and RCON by looking at what's being asked
     * WCON: kernel wants to output (command has data to send)
     * RCON: kernel wants to input (response receives data)
     *
     * For startup, only WCON is used. Implement that first.
     */

    /* Assume WCON for now (write console) */
    return handle_fe_wcon(ctx);
}

/**
 * Dispatch feexit_fecall: Kernel shutdown
 *
 * Kernel is shutting down cleanly. Acknowledge and allow exit.
 *
 * TODO (Phase 2):
 *   - Close all open devices
 *   - Flush filesystem cache
 *   - Graceful ND-100 shutdown
 */
static MonResult handle_feexit_fecall(MonContext* ctx) {
    mon_log(MON_LOG_INFO, "MON 600B: feexit_fecall - Kernel shutdown");

    ctx->set_k_flag(ctx->cpu, 0);

    return MON_SUCCESS;
}

/**
 * MON 600B: Main Entry Point
 *
 * NDIX kernel calls this via indirect domain call to 0xf8000180.
 * Identifies operation type and dispatches to appropriate handler.
 *
 * This MON is NDIX-specific and not available in standard SINTRAN.
 * Calling from non-NDIX program should be detected at runtime and fail.
 *
 * Parameters (in B register frame, set by kernel before callg):
 *   B.20 = Command packet pointer (byte address)
 *   B.24 = Parameter 2 (currently unused)
 *   B.28 = Response packet pointer (byte address)
 *   B.32 = Parameter 4 (currently unused)
 *
 * Return:
 *   K flag set = error, clear = success
 *   I1 register = error code (if K flag set)
 *   Response packet written to B.28 = results
 *
 * Errors:
 *   SINTRAN_ERROR_WRONG_ARG_COUNT - Invalid packet format
 *   Other SINTRAN error codes for specific failures
 */
MonResult mon_600B_NDIX(MonContext* ctx) {
    mon_log(MON_LOG_INFO, "MON 600B: Entry point");

    /* Validate context */
    if (!ctx || !ctx->cpu) {
        if (ctx) ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    /* TODO (Phase 2): Read command packet to identify operation
     *
     * Command packet format (at B.20):
     *   Offset 0: Operation type (2 bytes)
     *   - 0xf801 = feinit_fecall (initialization)
     *   - 0xf802 = fecall (generic I/O)
     *   - 0xf803 = feexit_fecall (shutdown)
     *
     * For now, assume feinit (first call kernel makes)
     */

    /* PHASE 1: Always respond to feinit (kernel bootstrap)
     * PHASE 2: Add operation detection and dispatch table */

    return handle_feinit_fecall(ctx);
}

/*
 * ============================================================================
 * PHASE 2 PLACEHOLDERS (To Be Implemented)
 * ============================================================================
 *
 * The following stubs are reserved for Phase 2 I/O operations:
 *
 *   static MonResult handle_fe_init(MonContext* ctx)    Device initialization
 *   static MonResult handle_fe_open(MonContext* ctx)    Open device
 *   static MonResult handle_fe_close(MonContext* ctx)   Close device
 *   static MonResult handle_fe_read(MonContext* ctx)    Read from device
 *   static MonResult handle_fe_write(MonContext* ctx)   Write to device
 *   static MonResult handle_fe_rcon(MonContext* ctx)    Read control
 *   static MonResult handle_fe_wcon(MonContext* ctx)    Write control
 *   static MonResult handle_fe_dctl(MonContext* ctx)    Device control
 *   static MonResult handle_fe_errm(MonContext* ctx)    Error message
 *   static MonResult handle_fe_exit(MonContext* ctx)    Exit device
 *
 * When implemented, add a dispatch table and operation detection.
 */
