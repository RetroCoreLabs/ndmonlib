/*
 * MON 600B [NDIX Front-End Call] — NDIX front-end (fecall) support
 *
 * Address models (verified from kernel machine/locore.c + if.h):
 *  - FE_INIT via _feinit_fecall: `go fe2` skips translation, so the packet
 *    pointers are RAW VIRTUAL BYTE addresses -> use the MMU callbacks directly.
 *  - all other calls via _fecall: pointers are ND-100 WORD addresses,
 *    ND-500 byte addr = (word << 1) - private -> use read/write_phys_word.
 *  - args are passed BY ADDRESS: value = read_word(arg_addresses[i]).
 */

#include "mon.h"
#include "mon_log.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* --- feinit response (init_rpk) field layout, byte-packed per if.h --------
 * completion@0(2) howto@2(4) rootdev@6(4) condev@10(2) spst@12(4) scont@16(4)
 * sndix@20(4) stext@24(4) sdata@28(4) sstack@32(4) sfree@36(4) sbuffer@40(4)
 * sphys@44(4) private@48(4) cputype@52(2) s3_vers@54(2) sharedseg@56(4)
 * contigno@60(2) pageno@62(2) booted@64(256) ; total 320. Big-endian.       */
#define INIT_RPK_SIZE 320

/* Physical memory model (D1: full 16 MB MPM5, ND-500 base 0, private=0).
 * Response addresses are ND-100 WORD addresses (kernel does htob = <<1 to bytes).
 * realmem = (htob(sphys)-htob(scont))/NBPG ; firstaddr = (htob(sfree)-private)/NBPG. */
#define MEM_BYTES        0x01000000u   /* 16 MB */
#define SFREE_BYTE       0x00100000u   /* first free phys page after kernel image (1 MB) */
#define W(x)             ((x) >> 1)     /* byte -> ND-100 word address */

#define KERNEL_TEXT_SEG   0x08000000u
#define KERNEL_DATA_SEG   0x10000000u
#define KERNEL_STACK_SEG  0xE8000000u
#define KERNEL_PST_SEG    0x30000000u
#define KERNEL_SHARED_SEG 0x30000000u
#define ND500_CPU_TYPE    5
#define SINTRAN_VERSION   0x0300

static void put16(uint8_t* b, int off, uint16_t v) {
    b[off] = (uint8_t)(v >> 8); b[off + 1] = (uint8_t)v;
}
static void put32(uint8_t* b, int off, uint32_t v) {
    b[off] = (uint8_t)(v >> 24); b[off + 1] = (uint8_t)(v >> 16);
    b[off + 2] = (uint8_t)(v >> 8); b[off + 3] = (uint8_t)v;
}

static MonResult handle_feinit_fecall(MonContext* ctx) {
    mon_log(MON_LOG_INFO, "MON 600B: FE_INIT (feinit)");

    if (!ctx->read_word || !ctx->write_byte) {
        mon_log(MON_LOG_INFO, "MON 600B: ERROR - missing callbacks");
        ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    /* feinit skips address translation: arg[2] deref = VIRTUAL byte addr of init_rpk */
    uint32_t resp_addr = ctx->read_word(ctx->cpu, ctx->arg_addresses[2]);

    uint8_t buf[INIT_RPK_SIZE];
    memset(buf, 0, sizeof(buf));
    put16(buf,  0, 0);                         /* completion = OK           */
    put32(buf,  2, 0);                         /* howto                     */
    put32(buf,  6, 0);                         /* rootdev = di0 (maj0 min0) */
    put16(buf, 10, 0);                         /* condev  = console         */
    put32(buf, 12, W(KERNEL_PST_SEG));         /* spst                      */
    put32(buf, 16, W(0));                      /* scont = phys base 0       */
    put32(buf, 20, 0);                         /* sndix                     */
    put32(buf, 24, W(KERNEL_TEXT_SEG));        /* stext                     */
    put32(buf, 28, W(KERNEL_DATA_SEG));        /* sdata                     */
    put32(buf, 32, W(KERNEL_STACK_SEG));       /* sstack                    */
    put32(buf, 36, W(SFREE_BYTE));             /* sfree                     */
    put32(buf, 40, W(SFREE_BYTE));             /* sbuffer                   */
    put32(buf, 44, W(MEM_BYTES));              /* sphys = 16 MB top         */
    put32(buf, 48, 0);                         /* private = 0 (no offset)   */
    put16(buf, 52, ND500_CPU_TYPE);            /* cputype                   */
    put16(buf, 54, SINTRAN_VERSION);           /* s3_vers                   */
    put32(buf, 56, W(KERNEL_SHARED_SEG));      /* sharedseg                 */
    put16(buf, 60, 0);                         /* contigno                  */
    put16(buf, 62, 0);                         /* pageno                    */
    memcpy(buf + 64, "(SYSTEM)VMUNIX", 14);    /* booted                    */

    for (int i = 0; i < INIT_RPK_SIZE; i++)
        ctx->write_byte(ctx->cpu, resp_addr + i, buf[i]);

    mon_log(MON_LOG_INFO,
            "MON 600B: feinit response -> 0x%08x (realmem=%u pages, 16MB, private=0)",
            resp_addr, (MEM_BYTES) / 2048);

    ctx->set_k_flag(ctx->cpu, 0);
    return MON_SUCCESS;
}

/* NDIX shared-segment console-output global (_cout, locore.s). fewcon() stores
 * the char as a 32-bit long before the FE_WCON call; BE low byte = _cout+3. */
#define NDIX_COUT_ADDR      0x30001048u
#define NDIX_COUT_CHAR_BYTE (NDIX_COUT_ADDR + 3u)

static MonResult handle_fe_wcon(MonContext* ctx) {
    uint8_t ch = ctx->read_byte(ctx->cpu, NDIX_COUT_CHAR_BYTE);
    putchar(ch);
    fflush(stdout);
    if (ctx->arg_count >= 3) {
        uint32_t resp = ctx->read_word(ctx->cpu, ctx->arg_addresses[2]);
        ctx->write_halfword(ctx->cpu, resp, 0);  /* completion = 0 (best effort) */
    }
    ctx->set_k_flag(ctx->cpu, 0);
    return MON_SUCCESS;
}

/* --- diagnostic decode tables (logging every fecall) ------------------- */
static const char* fe_name(unsigned code) {
    switch (code) {
    case 0x1: return "FE_INIT"; case 0x2: return "FE_IDEV";
    case 0x3: return "FE_OPEN"; case 0x4: return "FE_CLOS";
    case 0x5: return "FE_READ"; case 0x6: return "FE_RCON";
    case 0x7: return "FE_WRIT"; case 0x8: return "FE_WCON";
    case 0x9: return "FE_DCTL"; case 0xb: return "FE_EXIT";
    case 0xe: return "FE_ERRM"; default: return "FE_???";
    }
}
static const char* dev_name(unsigned d) {
    switch (d) {
    case 0: return "FAULT"; case 1: return "DISK"; case 2: return "TAPE";
    case 3: return "TERM_IN"; case 4: return "TERM_OUT"; case 5: return "CLOCK";
    case 6: return "GTRAP"; case 7: return "XMSG"; case 8: return "SIINTR";
    default: return "DEV?";
    }
}

static void log_fecall_detail(MonContext* ctx, unsigned dt, unsigned fc) {
    static int seq = 0;
    seq++;
    if (fc == 0x8) return;  /* skip the very chatty console writes */
    mon_log(MON_LOG_INFO, "FECALL #%d  dev=%u(%s) fe=0x%x(%s) argc=%d",
            seq, dt, dev_name(dt), fc, fe_name(fc), (int)ctx->arg_count);
    if ((int)ctx->arg_count >= 4 && ctx->read_word) {
        uint32_t rp = ctx->read_word(ctx->cpu, ctx->arg_addresses[2]);
        uint32_t cp = ctx->read_word(ctx->cpu, ctx->arg_addresses[3]);
        mon_log(MON_LOG_INFO, "   respptr=0x%08x cmdptr=0x%08x", rp, cp);
    }
}

static int feinit_completed = 0;

MonResult mon_600B_NDIX(MonContext* ctx) {
    if (!ctx || !ctx->cpu) {
        if (ctx) ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }
    if (ctx->arg_count < 3) {
        mon_log(MON_LOG_INFO, "MON 600B: ERROR - not enough arguments");
        ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    /* decode device + FE code (args by-address; value = deref) */
    unsigned dt = 0, fc = 0;
    if (ctx->read_word && ctx->arg_count >= 2) {
        dt = (ctx->read_word(ctx->cpu, ctx->arg_addresses[0]) >> 16) & 0xFFFF;
        fc =  ctx->read_word(ctx->cpu, ctx->arg_addresses[1]) & 0xFFFF;
    }
    log_fecall_detail(ctx, dt, fc);

    /* Dispatch. FE_INIT is the very first call; keep the ordinal guard as a
     * fallback in case decode is ever wrong. Console writes stay on the fast
     * path. Other calls are logged and left unimplemented for now (Phase C). */
    if (fc == 0x1 || !feinit_completed) {
        feinit_completed = 1;
        return handle_feinit_fecall(ctx);
    }
    if (fc == 0x8) {
        return handle_fe_wcon(ctx);
    }

    /* Unimplemented (FE_IDEV/OPEN/READ/... ) - leave completion as-is for now.
     * Return success so the guest keeps running and we can see the next call. */
    ctx->set_k_flag(ctx->cpu, 0);
    return MON_SUCCESS;
}
