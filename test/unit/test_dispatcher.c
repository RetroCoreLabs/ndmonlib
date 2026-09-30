/*
 * Dispatcher tests - src/core/mon_dispatch.c.
 *
 * Covers what mon_dispatch() does with each registered status, the
 * unimplemented / in-progress behaviour settings, and the registry lookups.
 * Expected values are taken from mon_dispatch.c and from the registrations in
 * src/core/mon_registry.c.
 *
 * The fake handlers are registered at MON 600B (384 decimal). mon_registry.c
 * deliberately leaves 600B unregistered - nd500x serves it itself - so the
 * test does not replace a real handler.
 */

#include "unit/test_utils.h"
#include "mon_config.h"
#include <stdio.h>
#include <string.h>

#define TEST_MON    0600   /* 384: not registered by mon_registry.c */
#define UNUSED_MON  0601   /* 385: not registered either */

static int handler_calls = 0;

static MonResult fake_handler(MonContext* ctx) {
    handler_calls++;
    mon_set_success(ctx);
    return MON_SUCCESS;
}

static int k_flag_seen = -1;
static void spy_set_k_flag(void* cpu, int value) { (void)cpu; k_flag_seen = value; }

static void fresh(MonContext* ctx, uint32_t mon_number) {
    mock_init_ctx(ctx);
    ctx->set_k_flag = spy_set_k_flag;
    ctx->mon_number = mon_number;
    handler_calls = 0;
    k_flag_seen = -1;
}

static void reg(MonHandler handler, MonImplStatus status) {
    mon_register(TEST_MON, "600B", "TEST", "TestCall", "dispatcher test", handler, status, 0);
}

/* ---- registry lookups (real registrations from mon_registry.c) ---------- */

static void test_registry_lookups(void) {
    const char* name = mon_get_name(0);
    const char* octal = mon_get_octal(0);
    check(name && strcmp(name, "LEAVE") == 0, "MON 0B is registered with short name LEAVE");
    check(octal && strcmp(octal, "0B") == 0, "MON 0B octal string is \"0B\"");
    check(mon_get_status(0) == MON_STATUS_VALIDATED, "MON 0B is registered VALIDATED");
    check(mon_is_implemented(0) && mon_is_validated(0), "MON 0B counts as implemented and validated");

    check(mon_get_status(07) == MON_STATUS_NOT_IMPLEMENTED, "MON 7B ReadBlock is registered NOT_IMPLEMENTED");
    check(!mon_is_implemented(07), "MON 7B does not count as implemented");

    check(mon_get_entry(TEST_MON) == NULL, "MON 600B is not registered");
    check(mon_get_name(TEST_MON) == NULL, "name of an unregistered call is NULL");
    check(mon_get_status(TEST_MON) == MON_STATUS_NOT_IMPLEMENTED,
          "status of an unregistered call is NOT_IMPLEMENTED");
    check(mon_get_entry(512) == NULL && mon_get_name(512) == NULL,
          "numbers past the 512-entry registry return NULL");
}

/* ---- default behaviour settings ----------------------------------------- */

static void test_defaults(void) {
    check(mon_get_unimpl_behavior() == MON_UNIMPL_BREAK, "default for unimplemented calls is BREAK");
    check(mon_get_inprogress_behavior() == MON_UNIMPL_CONTINUE, "default for in-progress calls is CONTINUE");
}

/* ---- dispatch by status -------------------------------------------------- */

static void test_not_implemented(void) {
    MonContext ctx;
    reg(fake_handler, MON_STATUS_NOT_IMPLEMENTED);

    mon_set_unimpl_behavior(MON_UNIMPL_CONTINUE);
    fresh(&ctx, TEST_MON);
    check(mon_dispatch(&ctx) == MON_ERROR, "NOT_IMPLEMENTED: dispatch returns MON_ERROR");
    check(handler_calls == 0, "NOT_IMPLEMENTED: the handler is NOT called");
    check(k_flag_seen == 1, "NOT_IMPLEMENTED: K flag is set");
    check(!ctx.break_requested && !ctx.halt_requested, "NOT_IMPLEMENTED + CONTINUE: no break, no halt");

    mon_set_unimpl_behavior(MON_UNIMPL_BREAK);
    fresh(&ctx, TEST_MON);
    mon_dispatch(&ctx);
    check(ctx.break_requested && !ctx.halt_requested, "NOT_IMPLEMENTED + BREAK: break requested");

    mon_set_unimpl_behavior(MON_UNIMPL_HALT);
    fresh(&ctx, TEST_MON);
    mon_dispatch(&ctx);
    check(ctx.halt_requested && !ctx.break_requested, "NOT_IMPLEMENTED + HALT: halt requested");

    mon_set_unimpl_behavior(MON_UNIMPL_CONTINUE);
}

static void test_in_progress(void) {
    MonContext ctx;
    reg(fake_handler, MON_STATUS_IN_PROGRESS);

    mon_set_inprogress_behavior(MON_UNIMPL_CONTINUE);
    fresh(&ctx, TEST_MON);
    check(mon_dispatch(&ctx) == MON_SUCCESS, "IN_PROGRESS: the handler's result is returned");
    check(handler_calls == 1, "IN_PROGRESS: the handler is called once");
    check(!ctx.break_requested && !ctx.halt_requested, "IN_PROGRESS + CONTINUE: no break, no halt");

    mon_set_inprogress_behavior(MON_UNIMPL_BREAK);
    fresh(&ctx, TEST_MON);
    mon_dispatch(&ctx);
    check(handler_calls == 1 && ctx.break_requested, "IN_PROGRESS + BREAK: handler still runs, break requested");

    mon_set_inprogress_behavior(MON_UNIMPL_HALT);
    fresh(&ctx, TEST_MON);
    mon_dispatch(&ctx);
    check(handler_calls == 1 && ctx.halt_requested, "IN_PROGRESS + HALT: handler still runs, halt requested");

    mon_set_inprogress_behavior(MON_UNIMPL_CONTINUE);
}

static void test_validated(void) {
    MonContext ctx;
    reg(fake_handler, MON_STATUS_VALIDATED);

    mon_set_inprogress_behavior(MON_UNIMPL_HALT);   /* must not apply to VALIDATED */
    fresh(&ctx, TEST_MON);
    check(mon_dispatch(&ctx) == MON_SUCCESS, "VALIDATED: the handler's result is returned");
    check(handler_calls == 1, "VALIDATED: the handler is called once");
    check(k_flag_seen == 0, "VALIDATED: the handler's mon_set_success clears K");
    check(!ctx.break_requested && !ctx.halt_requested, "VALIDATED: no break, no halt");
    mon_set_inprogress_behavior(MON_UNIMPL_CONTINUE);
}

static void test_missing_handler_and_bad_numbers(void) {
    MonContext ctx;
    reg(NULL, MON_STATUS_VALIDATED);
    fresh(&ctx, TEST_MON);
    check(mon_dispatch(&ctx) == MON_ERROR && ctx.break_requested,
          "registered with no handler function: MON_ERROR and break requested");

    mon_set_unimpl_behavior(MON_UNIMPL_CONTINUE);
    fresh(&ctx, UNUSED_MON);
    check(mon_dispatch(&ctx) == MON_ERROR && k_flag_seen == 1,
          "unregistered number: MON_ERROR and K flag set");

    fresh(&ctx, 512);
    check(mon_dispatch(&ctx) == MON_ERROR, "number past the registry: MON_ERROR");

    check(mon_dispatch(NULL) == MON_ERROR, "NULL context: MON_ERROR");
}

int main(void) {
    printf("Dispatcher tests\n\n");

    /* mon_init() opens a default scratch file on disk unless this is off. */
    mon_config_set_auto_scratch_64(0);
    test_defaults();          /* before mon_init: the defaults are static initialisers */
    mon_init();

    test_registry_lookups();
    test_not_implemented();
    test_in_progress();
    test_validated();
    test_missing_handler_and_bad_numbers();

    mon_shutdown();
    return test_finish("dispatcher");
}
