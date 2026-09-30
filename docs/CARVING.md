# Carving Guide: Identifying and Implementing Missing MON Calls

> How to identify which MON calls your program needs and implement them from SINTRAN manual references.

## Quick Summary

When a program fails with "Unimplemented MON XXB":

1. **Identify** the MON number from logs → check `docs/mon-implementation-status.md`
2. **Research** SINTRAN manual sections linked in the docs
3. **Analyze** existing similar handlers for implementation patterns
4. **Implement** by copying template and adding semantics
5. **Test** with variants (success, error, edge cases)
6. **Validate** against SINTRAN specification

---

## Step 1: Identify Unimplemented MON Calls

### From Program Execution

```bash
# Terminal output shows:
# ERROR: MON 412B not implemented (call #1 from PC=0x12345678)

# In nd500x debugger:
> load linker.dom
> run
[MON] 412B — unimplemented at PC=0x12345678
[MON] Error code set: CALL_NOT_ALLOWED
[HALT] Execution stopped
```

### From Automated Analysis

```bash
# List all unimplemented MON calls (the "Stub" section):
sed -n '/^## Stub/,$p' docs/mon-implementation-status.md | head -30

# Check specific MON:
grep '`412B`' docs/mon-implementation-status.md
```

---

## Step 2: Research the MON Call Specification

### Find the registration

```bash
# Name, description, parameters and status as registered
grep -n -A 12 '"412B"' src/core/mon_registry.c
```

### Consult SINTRAN Manual

Use the reference from metadata to find the official specification in the SINTRAN III Reference Manual.

---

## Step 3: Analyze Existing Similar Handlers

### Find Related Handlers

```bash
# Calls the manual groups under 2.9 File System Operations
python3 -c "import json; g=json.load(open('metadata/mon_function_groups.json'))['groups']; print([x['calls'] for x in g if x['section']=='2.9'][0])"
```

### Examine Working Handler

```bash
# Look at a validated handler as a pattern
cat src/handlers/mon_50B_OpenFile.c
```

---

## Step 4: Implement the Handler

### Open the Stub

Every registered call already has a stub file in `src/handlers/`, named after
its handler function (for 412B: `src/handlers/mon_412B_FileAsSegment.c`).
Replace the stub body, and remove the `AUTO-GENERATED STUB` header line.

### Edit Implementation

Reference the SINTRAN manual and write the handler:

```c
/**
 * 412B FSCNT — File System Control
 * Get count of files open in system
 */
#include <ndmon/mon.h>
#include "../mon_file_table.h"

MonResult mon_412B_FileAsSegment(MonContext* ctx) {
    if (ctx->arg_count != 0) {
        mon_set_error(ctx, SINTRAN_ERROR_WRONG_ARG_COUNT);
        return MON_ERROR;
    }

    int file_count = mon_file_table_count_open();
    ctx->set_i1(ctx->cpu, (uint32_t)file_count);
    mon_set_error(ctx, SINTRAN_ERROR_NONE);
    
    return MON_SUCCESS;
}
```

---

## Step 5: Add Test Cases

### Add Test

```bash
# Edit test/integration/test_mon_file_io.c
cat >> test/integration/test_mon_file_io.c << 'EOF'

void test_mon_412B_file_count_empty(void) {
    mon_file_table_clear();
    MonContext ctx = mock_cpu_context();
    ctx.arg_count = 0;
    
    MonResult result = mon_412B_FileAsSegment(&ctx);
    
    assert_equals(result, MON_SUCCESS);
    assert_equals(ctx.get_i1(ctx.cpu), 0);
}
EOF
```

---

## Step 6: Update Status & Regenerate Docs

### Set the status in src/core/mon_registry.c

Change the status in the call's `mon_register_ex(...)` block, for example
`MON_STATUS_NOT_IMPLEMENTED` to `MON_STATUS_IN_PROGRESS` or
`MON_STATUS_VALIDATED`. The dispatcher never calls a handler registered
`MON_STATUS_NOT_IMPLEMENTED`.

### Regenerate Documentation

```bash
python3 tools/generate_mon_status.py

# Verify update
grep '`412B`' docs/mon-implementation-status.md
```

---

## Step 7: Build & Validate

```bash
cd build
cmake ..
make
ctest -R mon_handlers -V      # handler tests live in test/integration/test_mon_handlers.c
python3 ../tools/generate_mon_status.py   # refresh status docs and README tables
git add ../src/handlers/mon_412B_FileAsSegment.c
git add ../src/core/mon_registry.c
git add ../test/integration/test_mon_handlers.c
git add ../docs/mon-implementation-status.md ../metadata/mon_status.json ../README.md
git commit -m "implement: MON 412B FSCNT (file system control)"
```

---

## Common Implementation Patterns

### Reading Parameters

```c
uint32_t filename_addr = ctx->arg_addresses[0];
uint32_t param_value = mon_read_param_word(ctx, 0);
```

### Accessing Memory (via callbacks)

```c
uint32_t word = ctx->read_word(ctx->cpu, addr);
ctx->write_byte(ctx->cpu, addr, byte_value);
```

### Reading SINTRAN Strings (terminated by 0x27)

```c
char buffer[256];
for (int i = 0; i < sizeof(buffer); i++) {
    uint8_t byte = ctx->read_byte(ctx->cpu, filename_addr + i);
    buffer[i] = byte;
    if (byte == 0x27)  // SINTRAN string terminator
        break;
}
```

### Writing Results

```c
ctx->set_i1(ctx->cpu, result_value);           // Result in I1 register
mon_write_param_word(ctx, 0, result_value);    // Result in parameter
```

### Error Handling

```c
#include "mon_errors.h"

if (bad_condition) {
    mon_set_error(ctx, SINTRAN_ERROR_FILE_NOT_FOUND);
    ctx->set_k_flag(ctx->cpu, 1);
    return MON_ERROR;
}
```

---

## Testing Checklist

Before marking as VALIDATED:

- [ ] Happy path test passes
- [ ] Edge cases covered
- [ ] Error cases tested
- [ ] Parameter validation correct
- [ ] Error codes set appropriately
- [ ] No memory access violations
- [ ] Documentation updated

---

## References

- [mon-implementation-status.md](mon-implementation-status.md) — Complete MON listing with status
- SINTRAN III Reference Manual (offline) — Authoritative spec
