/*
 * File table tests.
 *
 * Covers the sequential-write (access code 0) session length, which is the
 * emulator's stand-in for SINTRAN's in-core datafield word 21B: such an open
 * resets it, every write raises it, and CLOSE writes it back by truncating the
 * host file. Carved from the SINTRAN L file system segment - SOFT@066123B sets
 * it to -1 for access 0 alone at 066374B, FCL2@070132B stores it into object
 * entry word 62B at 071311B-071314B. Write-up: NDInsight
 * SINTRAN/ND500/nd-500-mon/CARVE-ANSWER-FOUR-OPEN-QUESTIONS-2026-08-17.md, Q4.
 *
 * The point of testing this here is that no vendor program available opens with
 * access code 0 - a traced linker run uses 1, 2 and 3 only - so a guest run
 * cannot exercise it.
 */

#include "unit/test_utils.h"
#include "mon_file_table.h"
#include "mon_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char root[256];
static char userdir[512];

/* Absolute path of a file inside the temporary SINTRAN user directory. */
static void host_path_of(char* out, size_t out_size, const char* name) {
    snprintf(out, out_size, "%s/%s", userdir, name);
}

static long file_size_of(const char* name) {
    char path[1024];
    struct stat st;
    host_path_of(path, sizeof(path), name);
    if (stat(path, &st) != 0) return -1;
    return (long)st.st_size;
}

static void write_seed_file(const char* name, size_t bytes) {
    char path[1024];
    host_path_of(path, sizeof(path), name);
    FILE* f = fopen(path, "wb");
    if (!f) {
        printf("FAIL: could not create seed file %s\n", path);
        test_failures++;
        return;
    }
    for (size_t i = 0; i < bytes; i++) fputc('A' + (int)(i % 26), f);
    fclose(f);
}

/* Write `bytes` bytes straight to the host handle the file table opened, the
 * way a write handler does, then tell the table how far the file now reaches. */
static void write_through(int file_number, size_t bytes) {
    OpenFileEntry* entry = mon_file_table_get(file_number);
    if (!entry || !entry->host_file) {
        printf("FAIL: file %d has no host handle\n", file_number);
        test_failures++;
        return;
    }
    for (size_t i = 0; i < bytes; i++) fputc('z', entry->host_file);
    entry->current_position += (uint32_t)bytes;
    mon_file_note_write(entry);
}

static void setup(void) {
    snprintf(root, sizeof(root), "/tmp/ndmon-file-table-test-%d", (int)getpid());
    snprintf(userdir, sizeof(userdir), "%s/TESTUSER", root);
    mkdir(root, 0700);
    mkdir(userdir, 0700);
    mon_config_set_sintran_root(root);
    mon_config_set_current_user("TESTUSER");
    mon_file_table_init();
}

static void teardown(void) {
    char path[1024];
    static const char* leftovers[] = {
        "SHORTER.DATA", "NOWRITE.DATA", "READONLY.DATA", "SEGMAP.DATA", NULL
    };
    for (int i = 0; leftovers[i]; i++) {
        host_path_of(path, sizeof(path), leftovers[i]);
        unlink(path);
    }
    rmdir(userdir);
    rmdir(root);
}

/* Writing FEWER bytes than the file held must leave the file that much shorter
 * - the old tail is not SINTRAN's to keep. */
static void test_seq_write_shortens(void) {
    write_seed_file("SHORTER.DATA", 5000);
    check(file_size_of("SHORTER.DATA") == 5000, "seed file is 5000 bytes");

    int fno = mon_file_open("SHORTER", "DATA", ACCESS_SEQ_WRITE);
    check(fno >= 64, "access-0 open of an existing file succeeds");
    if (fno < 64) return;

    check(file_size_of("SHORTER.DATA") == 5000,
          "open alone does NOT shorten the file (r+b, not wb)");

    write_through(fno, 100);
    check(mon_file_close(fno) == 0, "close succeeds");
    check(file_size_of("SHORTER.DATA") == 100,
          "close truncated the file to the 100 bytes written");
}

/* Opening for sequential write and writing nothing empties the file. This is
 * the case that surprises people, so it is asserted explicitly. */
static void test_seq_write_nothing_empties(void) {
    write_seed_file("NOWRITE.DATA", 4096);
    int fno = mon_file_open("NOWRITE", "DATA", ACCESS_SEQ_WRITE);
    check(fno >= 64, "second access-0 open succeeds");
    if (fno < 64) return;
    check(mon_file_close(fno) == 0, "close after writing nothing succeeds");
    check(file_size_of("NOWRITE.DATA") == 0,
          "a write-open that wrote nothing leaves an empty file");
}

/* Every other access code leaves the stored length alone. */
static void test_read_open_never_truncates(void) {
    write_seed_file("READONLY.DATA", 777);
    int fno = mon_file_open("READONLY", "DATA", ACCESS_SEQ_READ);
    check(fno >= 64, "access-1 open succeeds");
    if (fno < 64) return;
    check(mon_file_close(fno) == 0, "close of a read-open succeeds");
    check(file_size_of("READONLY.DATA") == 777,
          "a read-open leaves the file length untouched");
}

/* A segment-mapped file's bytes arrive through the segment writeback, which
 * never advances current_position - so the session length says nothing about
 * the real length and must not be applied. Getting this wrong would truncate a
 * whole linked domain to zero. */
static void test_segment_mapped_is_left_alone(void) {
    write_seed_file("SEGMAP.DATA", 2048);
    int fno = mon_file_open("SEGMAP", "DATA", ACCESS_SEQ_WRITE);
    check(fno >= 64, "access-0 open of the segment file succeeds");
    if (fno < 64) return;

    OpenFileEntry* entry = mon_file_table_get(fno);
    entry->mapped_as_segment = true;
    entry->mapped_segment_no = 11;

    check(mon_file_close(fno) == 0, "close of a segment-mapped file succeeds");
    check(file_size_of("SEGMAP.DATA") == 2048,
          "a segment-mapped file is NOT truncated by the session length");
}

/* The ESCAPE poll peeks stdin on every run-loop tick. On the stdio console a
 * peek has to READ from the OS, so a byte that is not an escape is parked in an
 * internal one-byte pushback and left "for the program". A program that never
 * reads input then exits with that byte still parked, and a host that reads its
 * next command line straight from stdin starts one byte late - which is how
 * @CPU-STAT followed by USER came to execute SER.
 *
 * mon_console_take_pushback() is what lets the host reclaim it. The test drives
 * the real thing: a pipe on stdin, a real peek through the poll, then the
 * reclaim.
 */
static void test_console_pushback_is_reclaimable(void) {
    int fds[2];
    int saved_stdin;
    int c;

    printf("== the ESCAPE poll's peeked byte can be reclaimed ==\n");

    if (pipe(fds) != 0) { printf("FAIL: pipe() failed\n"); test_failures++; return; }
    if (write(fds[1], "F", 1) != 1) { printf("FAIL: write to pipe failed\n"); test_failures++; return; }

    saved_stdin = dup(STDIN_FILENO);
    dup2(fds[0], STDIN_FILENO);

    mon_install_stdio_console();

    /* 'F' is not an escape, so the poll reports "no break" and leaves the byte -
     * parked in the pushback, invisible to anyone reading stdin directly. */
    check(mon_console_poll_user_break() == 0, "a plain byte is not reported as a user break");

    c = mon_console_take_pushback();
    check(c == 'F', "the peeked byte is handed back, so the host can prepend it to its line");

    check(mon_console_take_pushback() == -1, "a second reclaim returns -1: the byte is handed over once");

    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(fds[0]);
    close(fds[1]);
}

int main(void) {
    printf("File table tests\n");
    setup();

    test_seq_write_shortens();
    test_seq_write_nothing_empties();
    test_read_open_never_truncates();
    test_segment_mapped_is_left_alone();
    test_console_pushback_is_reclaimable();

    teardown();

    printf("%s\n", test_failures == 0 ? "All file table tests passed" : "FAILURES PRESENT");
    return test_failures == 0 ? 0 : 1;
}
