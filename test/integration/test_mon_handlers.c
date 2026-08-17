/*
 * MON handler integration tests.
 *
 * Drives real handlers through a mock MonContext - a flat byte array for guest
 * memory and no CPU at all, since mon_set_error records the code in the context
 * itself. Every case here is a bug that actually shipped: a handler that reports
 * the WRONG ERROR CODE sends whoever is debugging it after the wrong thing, and
 * that costs more time than a handler that plainly does nothing.
 */

#include "mon.h"
#include "mon_config.h"
#include "mon_errors.h"
#include "mon_file_table.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures = 0;

static void check(int cond, const char* what) {
    printf("%s: %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) failures++;
}

/* ---- mock guest memory ------------------------------------------------- */

#define MOCK_MEM_SIZE 4096
static uint8_t mock_mem[MOCK_MEM_SIZE];

static uint32_t mock_read_word(void* cpu, uint32_t addr) {
    (void)cpu;
    if (addr + 3 >= MOCK_MEM_SIZE) return 0;
    return ((uint32_t)mock_mem[addr] << 24) | ((uint32_t)mock_mem[addr + 1] << 16) |
           ((uint32_t)mock_mem[addr + 2] << 8) | mock_mem[addr + 3];
}

static void mock_write_word(void* cpu, uint32_t addr, uint32_t val) {
    (void)cpu;
    if (addr + 3 >= MOCK_MEM_SIZE) return;
    mock_mem[addr]     = (uint8_t)(val >> 24);
    mock_mem[addr + 1] = (uint8_t)(val >> 16);
    mock_mem[addr + 2] = (uint8_t)(val >> 8);
    mock_mem[addr + 3] = (uint8_t)val;
}

static uint8_t mock_read_byte(void* cpu, uint32_t addr) {
    (void)cpu;
    return (addr < MOCK_MEM_SIZE) ? mock_mem[addr] : 0;
}

static void mock_write_byte(void* cpu, uint32_t addr, uint8_t val) {
    (void)cpu;
    if (addr < MOCK_MEM_SIZE) mock_mem[addr] = val;
}

/* Lay a SINTRAN string down at `addr`: the bytes then the 0x27 terminator. */
static void put_sintran_string(uint32_t addr, const char* s) {
    size_t i;
    for (i = 0; s[i] != '\0'; i++) mock_mem[addr + i] = (uint8_t)s[i];
    mock_mem[addr + i] = 0x27;   /* apostrophe terminator */
}

static void init_ctx(MonContext* ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->read_word   = mock_read_word;
    ctx->write_word  = mock_write_word;
    ctx->read_byte   = mock_read_byte;
    ctx->write_byte  = mock_write_byte;
}

/* Build the four-argument frame MON 50B OPEN expects and call it.
 *
 * The two string arguments are STRING DESCRIPTORS, not bare pointers: the
 * argument address holds [Length:4][Pointer:4] and the characters live at the
 * pointer, terminated by 0x27. Getting that wrong makes every open fail as
 * "no such file", which looks exactly like a broken handler - it is worth
 * spelling out here so the next person does not debug the wrong side.
 *
 * Arg words live at 0x100..0x10C, the descriptors at 0x180/0x190, the
 * characters at 0x200/0x280. */
static MonResult call_open(MonContext* ctx, int32_t file_no_in, uint32_t access,
                           const char* name, const char* type) {
    init_ctx(ctx);
    memset(mock_mem, 0, sizeof(mock_mem));

    mock_write_word(NULL, 0x100, (uint32_t)file_no_in);
    mock_write_word(NULL, 0x104, access);

    put_sintran_string(0x200, name);
    mock_write_word(NULL, 0x108, (uint32_t)strlen(name));   /* descriptor: length */
    mock_write_word(NULL, 0x10C, 0x200);                    /* descriptor: pointer */

    put_sintran_string(0x280, type);
    mock_write_word(NULL, 0x110, (uint32_t)strlen(type));
    mock_write_word(NULL, 0x114, 0x280);

    ctx->arg_count = 4;
    ctx->arg_addresses[0] = 0x100;
    ctx->arg_addresses[1] = 0x104;
    ctx->arg_addresses[2] = 0x108;   /* name descriptor */
    ctx->arg_addresses[3] = 0x110;   /* type descriptor */
    ctx->mon_number = 40;   /* 50B octal = 40 decimal */

    return mon_dispatch(ctx);
}

static char root[256];
static char userdir[512];

static void host_path_of(char* out, size_t n, const char* name) {
    snprintf(out, n, "%s/%s", userdir, name);
}

static void make_file(const char* name, size_t bytes) {
    char path[1024];
    FILE* f;
    size_t i;
    host_path_of(path, sizeof(path), name);
    f = fopen(path, "wb");
    if (!f) { printf("FAIL: cannot create %s\n", path); failures++; return; }
    for (i = 0; i < bytes; i++) fputc('x', f);
    fclose(f);
}

static long size_of(const char* name) {
    char path[1024];
    struct stat st;
    host_path_of(path, sizeof(path), name);
    if (stat(path, &st) != 0) return -1;
    return (long)st.st_size;
}

static void setup(void) {
    snprintf(root, sizeof(root), "/tmp/ndmon-handler-test-%d", (int)getpid());
    snprintf(userdir, sizeof(userdir), "%s/TESTUSER", root);
    mkdir(root, 0700);
    mkdir(userdir, 0700);
    mon_config_set_sintran_root(root);
    mon_config_set_current_user("TESTUSER");
    /* mon_init registers every handler in g_registry; without it mon_dispatch
     * finds no entry and reports the call unimplemented, whatever the handler
     * would have done. */
    mon_init();
    mon_file_table_init();
}

static void teardown(void) {
    char path[1024];
    static const char* leftovers[] = { "EXISTS.DATA", "MISSING.DATA", "NEWFILE.DATA", "OUTBYTE.DATA", NULL };
    int i;
    for (i = 0; leftovers[i]; i++) {
        host_path_of(path, sizeof(path), leftovers[i]);
        unlink(path);
    }
    rmdir(userdir);
    rmdir(root);
}

/* THE BUG: a quoted create over a file that already exists was reported to the
 * guest as 056B "No such file name". The file table detected it correctly and
 * returned -62 all along; mon_50B_OpenFile's switch simply had no case for -62,
 * so it fell to the default. CONVERT-DOMAIN's refusal to create its destination
 * then read as a name-resolution problem instead of "the destination is already
 * there", which is a completely different thing to go looking for. */
static void test_quoted_create_over_existing_reports_076B(void) {
    MonContext ctx;
    MonResult r;

    printf("== quoted create over an existing file ==\n");
    make_file("EXISTS.DATA", 100);

    r = call_open(&ctx, 0, ACCESS_SEQ_WRITE, "\"EXISTS\"", "DATA");

    check(r == MON_ERROR, "the call fails");
    check(ctx.error_flag == 1, "K flag is set");
    check(ctx.error_code == MON_ERR_FILE_ALREADY_EXISTS,
          "error is 076B File already exists, NOT 056B No such file name");
    check(size_of("EXISTS.DATA") == 100, "the existing file is untouched");
}

/* SINTRAN has no write-open-creates: an UNQUOTED name is lookup-only, so a
 * missing file is 056B for every access code. */
static void test_unquoted_missing_reports_056B(void) {
    MonContext ctx;
    MonResult r;

    printf("== unquoted open of a missing file ==\n");
    r = call_open(&ctx, 0, ACCESS_SEQ_WRITE, "MISSING", "DATA");

    check(r == MON_ERROR, "the call fails");
    check(ctx.error_code == MON_ERR_NO_SUCH_FILE_NAME, "error is 056B No such file name");
    check(size_of("MISSING.DATA") == -1, "nothing was created - SINTRAN has no write-open-creates");
}

/* A quoted name whose file is absent DOES create it, and the call succeeds with
 * a file number in the 64..127 range. This is the other half of the same
 * switch: proving the error path did not swallow the working case. */
static void test_quoted_create_of_absent_file_succeeds(void) {
    MonContext ctx;
    MonResult r;
    uint32_t returned;

    printf("== quoted create of an absent file ==\n");
    r = call_open(&ctx, 0, ACCESS_SEQ_WRITE, "\"NEWFILE\"", "DATA");

    check(r == MON_SUCCESS, "the call succeeds");
    check(ctx.error_flag == 0, "K flag is clear");
    returned = mock_read_word(NULL, 0x100);
    check(returned >= 64 && returned <= 127, "a file number in 64..127 is written back");
    check(size_of("NEWFILE.DATA") == 0, "the file now exists and is empty");

    if (r == MON_SUCCESS) mon_file_close((int)returned);
}

/* Drive MON 2B OUTBT: write one byte to an open file number. */
static MonResult call_outbt(MonContext* ctx, uint32_t file_no, uint8_t value) {
    init_ctx(ctx);
    mock_write_word(NULL, 0x300, file_no);
    mock_write_word(NULL, 0x304, value);
    ctx->arg_count = 2;
    ctx->arg_addresses[0] = 0x300;
    ctx->arg_addresses[1] = 0x304;
    ctx->mon_number = 2;    /* 2B */
    return mon_dispatch(ctx);
}

/* THE BUG THIS GUARDS: a sequential-write (access 0) open starts the session
 * length at zero and CLOSE truncates the file to it. That is correct SINTRAN
 * behaviour - but it is only safe if EVERY write path raises the length.
 * 2B OUTBT, 24B and 504B advanced the file position and did not, so turning the
 * truncation on cut their output back to nothing.
 *
 * This test goes through the real handler rather than the file-table helper,
 * because that is the only way it can fail if someone adds a write path and
 * forgets mon_file_note_write() - which is exactly how the bug arose. */
static void test_outbt_writes_survive_close_truncation(void) {
    MonContext ctx;
    uint32_t fno;
    int i;

    printf("== bytes written with 2B OUTBT survive the close-time truncation ==\n");

    /* Seed a longer file so a failure to track the length is visible as a
     * truncation to 0 rather than as "the file was already that size". */
    make_file("OUTBYTE.DATA", 500);

    if (call_open(&ctx, 0, ACCESS_SEQ_WRITE, "OUTBYTE", "DATA") != MON_SUCCESS) {
        check(0, "access-0 open of the seeded file succeeds");
        return;
    }
    fno = mock_read_word(NULL, 0x100);
    check(fno >= 64 && fno <= 127, "access-0 open of the seeded file succeeds");

    for (i = 0; i < 5; i++) {
        if (call_outbt(&ctx, fno, (uint8_t)('A' + i)) != MON_SUCCESS) {
            check(0, "each OUTBT succeeds");
            mon_file_close((int)fno);
            return;
        }
    }
    check(1, "each OUTBT succeeds");

    check(mon_file_close((int)fno) == 0, "close succeeds");
    check(size_of("OUTBYTE.DATA") == 5,
          "the file is exactly the 5 bytes written - not 0 (length untracked) and not 500 (no truncation)");
}

int main(void) {
    printf("MON handler integration tests\n\n");
    setup();

    test_quoted_create_over_existing_reports_076B();
    test_unquoted_missing_reports_056B();
    test_quoted_create_of_absent_file_succeeds();
    test_outbt_writes_survive_close_truncation();

    teardown();

    printf("\n%s\n", failures == 0 ? "All MON handler tests passed" : "FAILURES PRESENT");
    return failures == 0 ? 0 : 1;
}
