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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* --- feinit response (init_rpk) field layout, byte-packed per if.h --------
 * completion@0(2) howto@2(4) rootdev@6(4) condev@10(2) spst@12(4) scont@16(4)
 * sndix@20(4) stext@24(4) sdata@28(4) sstack@32(4) sfree@36(4) sbuffer@40(4)
 * sphys@44(4) private@48(4) cputype@52(2) s3_vers@54(2) sharedseg@56(4)
 * contigno@60(2) pageno@62(2) booted@64(256) ; total 320. Big-endian.       */
#define INIT_RPK_SIZE 320

/* Physical memory model (D1: full 16 MB MPM5).
 * PRIVATE_BASE = the ND-100 physical byte address of ND-500 physical 0 (D2).
 * It MUST be non-zero: _dton (locore.c:1476) treats private==0 as "ND-100
 * layout unknown" and returns 0 for every dton(), which zeroes disk DMA
 * physaddr. scont/sphys/sfree/sbuffer are reported ND-100-ABSOLUTE (base+off)
 * so the kernel's (x - private) formulas (firstaddr machdep.c:205, mem.c:70)
 * recover the ND-500-relative value; every memory total is a difference, so
 * the base cancels. Packet/DMA inversion: ND-500 phys byte = (word<<1) - private
 * (confirmed by trap.c:1232). Response seg/phys fields are ND-100 WORD addresses
 * (kernel htob = <<1); `private` itself is stored in BYTES (machdep.c:827, no htob). */
#define MEM_BYTES        0x01000000u   /* 16 MB */
#define SFREE_BYTE       0x00100000u   /* first free ND-500-relative phys byte (1 MB) */
#define PRIVATE_BASE     0x00010000u   /* ND-100 byte base of ND-500 phys 0 (non-zero) */
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
    put32(buf, 16, W(PRIVATE_BASE));                 /* scont = ND-100 base       */
    put32(buf, 20, 0);                               /* sndix                     */
    put32(buf, 24, W(KERNEL_TEXT_SEG));              /* stext (virtual KVA)       */
    put32(buf, 28, W(KERNEL_DATA_SEG));              /* sdata (virtual KVA)       */
    put32(buf, 32, W(KERNEL_STACK_SEG));             /* sstack (virtual KVA)      */
    put32(buf, 36, W(PRIVATE_BASE + SFREE_BYTE));    /* sfree  (ND-100 absolute)  */
    put32(buf, 40, W(PRIVATE_BASE + SFREE_BYTE));    /* sbuffer(ND-100 absolute)  */
    put32(buf, 44, W(PRIVATE_BASE + MEM_BYTES));     /* sphys  (ND-100 absolute)  */
    put32(buf, 48, PRIVATE_BASE);                    /* private (BYTES, non-zero) */
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

/* ---------------------------------------------------------------------------
 * Disk fecalls (FE_OPEN / FE_READ), via _fecall (locore.c:227).
 *
 * _fecall converts each packet pointer to an ND-100 WORD address:
 *   b.28 = (phyladr(*resp) + _private) / 2 ;  b.32 = (phyladr(*cmd) + _private)/2
 * So the ND-500 physical byte address of a packet = (word<<1) - _private.
 * feinit reports _private = 0 (see handle_feinit_fecall), so phys = word<<1.
 * All packet fields are byte-packed big-endian (ND-500 PCC does not pad).
 * ------------------------------------------------------------------------- */
#define FE_PRIVATE PRIVATE_BASE   /* must match the `private` reported by feinit above */

/* physical byte address of the packet whose ND-100 word pointer is arg[idx] */
static uint32_t fe_pkt_phys(MonContext* ctx, int idx) {
    uint32_t word = ctx->read_word(ctx->cpu, ctx->arg_addresses[idx]);
    return (word << 1) - FE_PRIVATE;
}
static void     pput16(MonContext* c, uint32_t p, uint16_t v) {
    c->write_phys_byte(c->machine, p,   (uint8_t)(v >> 8));
    c->write_phys_byte(c->machine, p+1, (uint8_t)v);
}
static void     pput32(MonContext* c, uint32_t p, uint32_t v) {
    c->write_phys_byte(c->machine, p,   (uint8_t)(v >> 24));
    c->write_phys_byte(c->machine, p+1, (uint8_t)(v >> 16));
    c->write_phys_byte(c->machine, p+2, (uint8_t)(v >> 8));
    c->write_phys_byte(c->machine, p+3, (uint8_t)v);
}
static uint16_t pget16(MonContext* c, uint32_t p) {
    return (uint16_t)((c->read_phys_byte(c->machine, p) << 8)
                    |  c->read_phys_byte(c->machine, p+1));
}
static uint32_t pget32(MonContext* c, uint32_t p) {
    return ((uint32_t)c->read_phys_byte(c->machine, p)   << 24)
         | ((uint32_t)c->read_phys_byte(c->machine, p+1) << 16)
         | ((uint32_t)c->read_phys_byte(c->machine, p+2) << 8)
         |  (uint32_t)c->read_phys_byte(c->machine, p+3);
}

/* Backing root-disk image (optional). Path from NDIX_DISK_IMAGE. Addressed in
 * DEV_BSIZE = 1024-byte sectors (param.h:144; devaddr from di.c is in these
 * units since DEV_BSIZE/ssize = 1024/1024 = 1). */
#define NDIX_SECSIZE 1024u
static FILE* ndix_disk_img(void) {
    static FILE* fp = NULL;
    static int   tried = 0;
    if (!tried) {
        tried = 1;
        const char* path = getenv("NDIX_DISK_IMAGE");
        if (path && path[0]) {
            fp = fopen(path, "rb+");
            if (!fp) fp = fopen(path, "rb");
            if (fp) mon_log(MON_LOG_INFO, "MON 600B: root disk image = %s", path);
            else    mon_log(MON_LOG_WARN, "MON 600B: cannot open NDIX_DISK_IMAGE=%s", path);
        } else {
            mon_log(MON_LOG_WARN, "MON 600B: NDIX_DISK_IMAGE unset - disk reads serve zeros");
        }
    }
    return fp;
}

/* FE_IDEV (initialise device): diattach() (di.c:120) requires completion=0 and
 * subdevc>0 before it will set up ditab[]/format[] for the sub-devices; without
 * it diopen()/diread() run on an uninitialised sub_device (garbage packets).
 * Response variant is by device type (if.h):
 *   generic/disk: idev_rpk_xxxx { short completion@0; short subdevc@2; }
 *   term:         idev_rpk_term { short completion@0; long locdevm@2; short remdevc@6; }
 *   clock:        idev_rpk_clock{ short completion@0; struct s3_date now@2; }
 * DISK reports 1 sub-device (root di0). Other devices report completion=0 with
 * their variant left zeroed until they are needed. */
static MonResult handle_fe_idev(MonContext* ctx, unsigned dev_type) {
    uint32_t resp_phys = fe_pkt_phys(ctx, 2);
    pput16(ctx, resp_phys + 0, 0);            /* completion = OK */
    if (dev_type == 1 /* DISK */)
        pput16(ctx, resp_phys + 2, 1);        /* subdevc = 1 (di0) */
    mon_log(MON_LOG_INFO, "MON 600B: FE_IDEV dev=%u -> completion=0%s",
            dev_type, dev_type == 1 ? " subdevc=1" : "");
    ctx->set_k_flag(ctx->cpu, 0);
    return MON_SUCCESS;
}

/* FE_OPEN (disk): report a valid disk-type code so diopen() (di.c:166)
 * succeeds. devsiz is an INDEX into dist[] (disizes.c): 1 = dio70, logica
 * partitions (ptype 0, no BSD_PART bit). frmsiz/secsiz mirror dist[1]. */
static MonResult handle_fe_open(MonContext* ctx) {
    uint32_t cmd_phys  = fe_pkt_phys(ctx, 3);
    uint32_t resp_phys = fe_pkt_phys(ctx, 2);
    uint16_t format = pget16(ctx, cmd_phys + 0);   /* open_cpk_disk.format */

    /* open_rpk_disk: completion@0(2) devsiz@2(4) frmsiz@6(4) secsiz@10(2) */
    pput16(ctx, resp_phys +  0, 0);      /* completion = OK       */
    pput32(ctx, resp_phys +  2, 1);      /* devsiz = dist[1] dio70 */
    pput32(ctx, resp_phys +  6, 69530);  /* frmsiz = dist[1].dsize */
    pput16(ctx, resp_phys + 10, 1024);   /* secsiz = 1024          */

    mon_log(MON_LOG_INFO,
        "MON 600B: FE_OPEN(disk) cmd@0x%08x format=%u -> devsiz=1 frmsiz=69530 secsiz=1024",
        cmd_phys, format);
    ctx->set_k_flag(ctx->cpu, 0);
    return MON_SUCCESS;
}

/* --- async I/O completion: post an interrupt to the ND-500 ---------------
 * The real ND-100 signals async completion by queuing an int_descr on the
 * shared IPL record's ip_next; the ND-500 kernel's splx() (locore.c:1413)
 * polls ip_next on every ipl change and, since the idle loop spins on spl0()
 * (swtch.c:183 `while(_ffs(whichqs)==-1) spl0();`), picks it up almost
 * immediately: splx4 -> intvec -> dispatch (trap.c:602) -> (*drvtab[gen-1]
 * .fr_intr)(sub) = diintr(sub) (di.c:494) -> biodone(). diintr reads the
 * completion from ditab[sub].sd_apkt, which FE_READ already filled.
 *
 * We write into the shared segment via the virtual (MMU) accessors, big-endian.
 * int_descr (icb.h:16, byte-packed 22 bytes) lives in the iplrec slack at
 * 0x30001010 (iplrec is 0x30001000..~+10, clockrec starts 0x30001040).
 * ip_next is an ND-100 word address: intvec converts byte = word<<1 - shseg +
 * sharebase, with shseg=0x30000800, sharebase=0x30000000, so
 *   word = (byte + 0x800) >> 1. */
#define IPLREC_VADDR      0x30001000u
#define INT_DESCR_VADDR   0x30001010u
#define IPL_DK            4u          /* icb.h:81 */
#define GEN_DISK          1u          /* drvtab[0] = diintr; dispatch uses drvtab[gen-1] */

static void post_disk_interrupt(MonContext* ctx, uint16_t sub, uint32_t resp_word) {
    uint32_t d = INT_DESCR_VADDR;
    ctx->write_word    (ctx->cpu, d +  0, 0xFFFFFFFFu); /* id_next  = -1 (end)   */
    ctx->write_halfword(ctx->cpu, d +  4, IPL_DK);      /* id_ipl                */
    ctx->write_halfword(ctx->cpu, d +  6, 0);           /* id_s3add              */
    ctx->write_halfword(ctx->cpu, d +  8, 0);           /* id_s3dev              */
    ctx->write_halfword(ctx->cpu, d + 10, GEN_DISK);    /* id_gen_dev = DISK     */
    ctx->write_halfword(ctx->cpu, d + 12, sub);         /* id_sub_dev            */
    ctx->write_halfword(ctx->cpu, d + 14, 0);           /* id_flag               */
    ctx->write_halfword(ctx->cpu, d + 16, 0x5);         /* id_s3func = FE_READ   */
    ctx->write_word    (ctx->cpu, d + 18, resp_word);   /* id_resp_pkt           */

    /* link it: iplrec.ip_next = ND-100 word address of the int_descr */
    uint32_t ip_next_word = (INT_DESCR_VADDR + 0x800u) >> 1;  /* = 0x18000C08 */
    ctx->write_word(ctx->cpu, IPLREC_VADDR + 0, ip_next_word);
    mon_log(MON_LOG_INFO,
        "MON 600B: posted disk interrupt (sub=%u ipl=%u ip_next=0x%08x)",
        sub, IPL_DK, ip_next_word);
}

/* FE_READ (disk): DMA `nbytes` from the backing image at devaddr*1024 into the
 * guest physical address given by the ND-100-word physaddr in the command. */
static MonResult handle_fe_read(MonContext* ctx) {
    uint32_t cmd_phys  = fe_pkt_phys(ctx, 3);
    uint32_t resp_phys = fe_pkt_phys(ctx, 2);
    /* read_cpk_disk: nbytes@0(4) physaddr@4(4, ND-100 word) devaddr@8(4) */
    uint32_t nbytes   = pget32(ctx, cmd_phys + 0);
    uint32_t physw    = pget32(ctx, cmd_phys + 4);
    uint32_t devaddr  = pget32(ctx, cmd_phys + 8);
    uint32_t dma_phys = (physw << 1) - FE_PRIVATE;

    FILE*    fp  = ndix_disk_img();
    uint32_t got = 0;
    if (fp && nbytes <= (16u << 20)) {
        uint8_t* buf = (uint8_t*)malloc(nbytes ? nbytes : 1);
        if (buf) {
            if (fseek(fp, (long)((uint64_t)devaddr * NDIX_SECSIZE), SEEK_SET) == 0)
                got = (uint32_t)fread(buf, 1, nbytes, fp);
            for (uint32_t i = 0; i < nbytes; i++)
                ctx->write_phys_byte(ctx->machine, dma_phys + i, i < got ? buf[i] : 0);
            free(buf);
        }
    } else {
        for (uint32_t i = 0; i < nbytes; i++)
            ctx->write_phys_byte(ctx->machine, dma_phys + i, 0);
    }

    /* read_rpk_xxxx: completion@0(2) status@2(2) nbytes@4(4) */
    pput16(ctx, resp_phys + 0, 0);        /* completion = OK   */
    pput16(ctx, resp_phys + 2, 0);        /* status            */
    pput32(ctx, resp_phys + 4, nbytes);   /* bytes transferred */

    mon_log(MON_LOG_INFO,
        "MON 600B: FE_READ(disk) devaddr=%u(off=0x%llx) nbytes=%u -> dma_phys=0x%08x got=%u",
        devaddr, (unsigned long long)((uint64_t)devaddr * NDIX_SECSIZE),
        nbytes, dma_phys, got);

    /* async completion: post the I/O interrupt so diintr()->biodone() wakes the
     * blocked reader. sub-device = low 16 bits of the device arg (DISK<<16|sub);
     * resp pkt word address = the arg[2] value. */
    uint16_t sub = (uint16_t)(ctx->read_word(ctx->cpu, ctx->arg_addresses[0]) & 0xFFFF);
    uint32_t resp_word = ctx->read_word(ctx->cpu, ctx->arg_addresses[2]);
    post_disk_interrupt(ctx, sub, resp_word);

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
    if (fc == 0x2) {
        return handle_fe_idev(ctx, dt);
    }
    if (fc == 0x3) {
        return handle_fe_open(ctx);
    }
    if (fc == 0x5) {
        return handle_fe_read(ctx);
    }

    /* Unimplemented (FE_IDEV/CLOS/... ) - leave completion as-is for now.
     * Return success so the guest keeps running and we can see the next call. */
    ctx->set_k_flag(ctx->cpu, 0);
    return MON_SUCCESS;
}
