# Carving Guide: Identifying and Implementing Missing MON Calls

> How to identify which MON calls your program needs and implement them from SINTRAN manual references.

## Quick Summary

When a program fails with "Unimplemented MON XXB":

1. **Identify** the MON number from logs → check `docs/MON_CALLS.md`
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
# List all unimplemented MON calls:
grep "NOT_IMPLEMENTED" docs/MON_CALLS.md | head -20

# Check specific MON:
python3 tools/analyze_mon_calls.py --mon 412B --full
```

---

## Step 2: Research the MON Call Specification

### Find in MON_CALLS.md

```bash
# Quick lookup by MON number
grep -A 30 "^### 412B" docs/MON_CALLS.md
```

Output shows status, parameters, SINTRAN manual references.

### Consult SINTRAN Manual

Use the reference from metadata to find the official specification in the SINTRAN III Reference Manual.

---

## Step 3: Analyze Existing Similar Handlers

### Find Related Handlers

```bash
# List file operation handlers
python3 tools/analyze_mon_calls.py --class "File System" --implemented
```

### Examine Working Handler

```bash
# Look at a validated handler as a pattern
cat src/handlers/mon_420B_FileAttributes.c
```

---

## Step 4: Implement the Handler

### Copy Template

```bash
cd ~/repos/ndmonlib
cp src/handlers/MON_TEMPLATE.c src/handlers/mon_412B_FileSystemControl.c
```

### Edit Implementation

Reference the SINTRAN manual and adapt the template:

```c
/**
 * 412B FSCNT — File System Control
 * Get count of files open in system
 */
#include <ndmon/mon.h>
#include "../mon_file_table.h"

MonResult mon_412B_FileSystemControl(MonContext* ctx) {
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
    
    MonResult result = mon_412B_FileSystemControl(&ctx);
    
    assert_equals(result, MON_SUCCESS);
    assert_equals(ctx.get_i1(ctx.cpu), 0);
}
EOF
```

---

## Step 6: Update Metadata & Regenerate Docs

### Edit metadata/mon_registry.json

Update the entry for MON 412B:
```json
{
  "mon_number": "412B",
  "status": "IN_PROGRESS",
  "source_file": "src/handlers/mon_412B_FileSystemControl.c",
  "test_cases": ["test_mon_412B_file_count_empty"],
  "last_tested": "2026-07-23"
}
```

### Regenerate Documentation

```bash
python3 tools/generate_mon_calls.py

# Verify update
grep -A 5 "412B" docs/MON_CALLS.md
```

---

## Step 7: Build & Validate

```bash
cd build
cmake ..
make
ctest -R "test_mon_412B"
git add ../src/handlers/mon_412B_FileSystemControl.c
git add ../test/integration/test_mon_file_io.c
git add ../metadata/mon_registry.json
git add ../docs/MON_CALLS.md
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

- [docs/MON_CALLS.md](../docs/MON_CALLS.md) — Complete MON listing
- [docs/HANDLER_DEVELOPMENT.md](../docs/HANDLER_DEVELOPMENT.md) — Handler patterns
- [src/handlers/MON_TEMPLATE.c](../src/handlers/MON_TEMPLATE.c) — Template to copy
- SINTRAN III Reference Manual (offline) — Authoritative spec
