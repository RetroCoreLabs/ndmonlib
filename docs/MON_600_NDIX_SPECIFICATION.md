> **MON 600 IS NOT IMPLEMENTED IN THIS LIBRARY (since 2026-08-08).**
>
> The NDIX front-end call (fecall) lives in nd500x - `src/cpu/nd500_fecall.c` -
> and is reached from `src/cpu/nd500_indirect.c`, which intercepts CALLG offset
> `0x180` before the MON registry is consulted. It needs full cpu/machine/DMA
> access, which the generic registry does not provide. The handler that used to
> sit in `src/handlers/mon_600B_NDIX.c` never ran and had already drifted from
> the live code, so it was deleted.
>
> Keep reading this document as the **worked example for writing a handler** -
> just do not re-add MON 600.

# MON 600 — NDIX LLM Support Specification

> Implementation guidelines for MON 600, a custom non-SINTRAN MON call for running NDIX (LLM system) on ND-500 CPU.

---

## Overview

MON 600 is **architecture-specific**, **platform-specific**, and **non-standard SINTRAN**:
- **CPU**: ND-500 only
- **Platform**: NDIX LLM system only
- **Compatibility**: NOT ND-100, NOT standard SINTRAN
- **Status**: NDIX-specific extension

This document provides **rules and structure** for implementing MON 600 in ndmonlib.

---

## Implementation Rules

### Rule 1: Architecture Compatibility Metadata

In `metadata/mon_registry.json`, MON 600 entry MUST have:

```json
{
  "mon_number": "600B",
  "decimal": 384,
  "name": "[YOUR_NAME]",
  "class": "NDIX",
  "description": "[YOUR_DESCRIPTION]",
  "status": "VALIDATED",
  "source_file": "src/handlers/mon_600B_NDIX.c",
  "nd100_compat": false,              // ← MUST be false (ND-500 only)
  "nd500_compat": true,               // ← MUST be true
  "parameter_count": [YOUR_COUNT],
  "test_cases": ["test_mon_600B_..."],
  "references": ["NDIX LLM Specification"],
  "notes": "NDIX-specific, non-standard SINTRAN extension"
}
```

### Rule 2: Handler File Naming & Location

```
Location: src/handlers/mon_600B_NDIX.c
Pattern:  mon_600B_[Description].c
```

### Rule 3: Handler Function Signature

```c
/**
 * 600B [NAME] — NDIX LLM Support
 *
 * [DESCRIPTION OF WHAT IT DOES]
 *
 * This is a non-standard SINTRAN extension for NDIX LLM runtime.
 * Only available on ND-500 CPU with NDIX-enabled SINTRAN.
 *
 * Parameters:
 *   [List each parameter with index, name, type, description]
 *
 * Output:
 *   [List output registers/parameters]
 *
 * Errors:
 *   [List error codes and conditions]
 *
 * References:
 *   NDIX LLM Specification [VERSION/SECTION]
 */

#include <ndmon/mon.h>
#include "../mon_file_table.h"
#include "../mon_log.h"

MonResult mon_600B_NDIX(MonContext* ctx) {
    // YOUR IMPLEMENTATION HERE
    return MON_SUCCESS;  // or MON_ERROR
}
```

### Rule 4: Parameter Access Pattern

**ALWAYS use callbacks, NEVER direct memory access:**

```c
// ✅ CORRECT: Use callbacks
uint32_t param = mon_read_param_word(ctx, 0);
ctx->read_byte(ctx->cpu, addr);
ctx->write_word(ctx->cpu, addr, value);

// ❌ WRONG: Direct access
MyCpu* cpu = (MyCpu*)ctx->cpu;
value = cpu->memory[addr];  // NEVER DO THIS
```

### Rule 5: Error Handling

```c
// Check parameter count
if (ctx->arg_count != EXPECTED_COUNT) {
    mon_set_error(ctx, SINTRAN_ERROR_WRONG_ARG_COUNT);
    ctx->set_k_flag(ctx->cpu, 1);
    return MON_ERROR;
}

// Check conditions
if (bad_condition) {
    mon_set_error(ctx, YOUR_ERROR_CODE);
    ctx->set_k_flag(ctx->cpu, 1);
    return MON_ERROR;
}

// Success
mon_set_error(ctx, SINTRAN_ERROR_NONE);
ctx->set_k_flag(ctx->cpu, 0);
return MON_SUCCESS;
```

### Rule 6: Logging

Use logging for debugging (not user output):

```c
#include "../mon_log.h"

mon_log_printf("MON 600B: [YOUR_MESSAGE]");
mon_log_printf("MON 600B: Input param=%d, addr=0x%08x", param, addr);
```

### Rule 7: Testing

Create test in `test/integration/test_mon_handlers.c`:

```c
void test_mon_600B_success(void) {
    // Setup initial state
    MonContext ctx = mock_cpu_context();
    ctx.arg_count = [YOUR_COUNT];
    ctx.arg_addresses[0] = [YOUR_SETUP];
    // ... setup more parameters

    // Call handler
    MonResult result = mon_600B_NDIX(&ctx);

    // Assert
    assert_equals(result, MON_SUCCESS);
    assert_equals(ctx.error_flag, 0);
    // ... verify output
}

void test_mon_600B_error_case(void) {
    // Test error conditions
    MonContext ctx = mock_cpu_context();
    // ... setup bad condition

    MonResult result = mon_600B_NDIX(&ctx);

    assert_equals(result, MON_ERROR);
    assert_equals(ctx.error_flag, 1);
}
```

### Rule 8: No Mixing with Standard SINTRAN

- **Never use MON 600 in standard SINTRAN programs**
- Only in NDIX-enabled SINTRAN runtime
- Emulator must detect this and warn appropriately

Add note in handler:

```c
    // This MON is NDIX-specific and not available in standard SINTRAN
    // Calling from non-NDIX program should be detected at runtime
```

---

## File Creation Checklist

Before submitting MON 600 implementation:

- [ ] **Handler file created**: `src/handlers/mon_600B_NDIX.c`
  - [ ] Follows function signature template
  - [ ] Uses callbacks (never direct access)
  - [ ] Proper error handling
  - [ ] Includes comprehensive documentation header
  
- [ ] **Metadata updated**: `metadata/mon_registry.json`
  - [ ] `nd100_compat: false`
  - [ ] `nd500_compat: true`
  - [ ] Status: VALIDATED
  - [ ] References NDIX spec
  - [ ] Marked as NDIX-specific in notes

- [ ] **Tests created**: `test/integration/test_mon_handlers.c`
  - [ ] Success case
  - [ ] Error cases (at least 2)
  - [ ] Edge cases (if applicable)
  - [ ] All tests pass

- [ ] **No hardcoding**: All CPU-specific access via callbacks

- [ ] **Documentation complete**: 
  - [ ] Function header explains what it does
  - [ ] Parameters documented
  - [ ] Return values documented
  - [ ] NDIX-specific nature clearly stated

---

## Template File

Use this exact template:

```c
/**
 * 600B [NAME] — NDIX LLM Support
 *
 * [WHAT THIS DOES - 2-3 sentences]
 *
 * This is a non-standard SINTRAN extension for the NDIX LLM system.
 * Only available on ND-500 CPU running NDIX-enabled SINTRAN.
 * Not compatible with standard SINTRAN or ND-100.
 *
 * Parameters:
 *   [index] [name] ([type]) — [description]
 *
 * Output:
 *   [register] = [meaning]
 *
 * Errors:
 *   [error_code] — [when]
 *
 * References:
 *   NDIX LLM Specification
 */

#include <ndmon/mon.h>
#include "../mon_log.h"

MonResult mon_600B_NDIX(MonContext* ctx) {
    // Validate parameter count
    if (ctx->arg_count != [EXPECTED]) {
        mon_set_error(ctx, SINTRAN_ERROR_WRONG_ARG_COUNT);
        ctx->set_k_flag(ctx->cpu, 1);
        return MON_ERROR;
    }

    // Read parameters via callbacks
    uint32_t param1 = mon_read_param_word(ctx, 0);
    // ... more parameters

    // Your implementation here
    
    // Set results
    ctx->set_i1(ctx->cpu, result);
    
    // Success
    mon_set_error(ctx, SINTRAN_ERROR_NONE);
    ctx->set_k_flag(ctx->cpu, 0);
    
    return MON_SUCCESS;
}
```

---

## After Implementation

Once MON 600 is complete:

1. **Notify**: Provide:
   - Handler file (`mon_600B_NDIX.c`)
   - Metadata JSON entry
   - Test cases
   - Brief description of what MON 600 does

2. **Integration**: I will:
   - Verify it follows rules above
   - Add to metadata/mon_registry.json
   - Run `tools/generate_mon_calls.py` to auto-generate docs
   - Commit to ndmonlib
   - Both emulators automatically get MON 600 support

3. **Documentation**: Will be auto-generated showing:
   - MON 600 as ND-500 only
   - NDIX-specific
   - Non-standard SINTRAN
   - All parameters and references

---

## Key Reminders

| ❌ DON'T | ✅ DO |
|----------|-------|
| Direct CPU/MMU access | Use callbacks (ctx->read_word, ctx->write_byte, etc.) |
| Hardcode ND-500 details | Implement via callbacks, architecture-agnostic |
| Skip error handling | Always validate, set K flag, return MON_ERROR |
| Forget documentation | Complete function header + metadata |
| Test only happy path | Test success, errors, edge cases |
| Mix with SINTRAN | Mark NDIX-specific, ND-500 only |

---

## Questions?

- **Parameter types unclear?** → Check `mon_types.h` for MonContext fields
- **How to call a callback?** → See `src/handlers/mon_1B_InByte.c` for examples
- **Error codes?** → See `include/ndmon/mon_errors.h`
- **Test patterns?** → See `test/integration/test_mon_handlers.c`

---

**When complete, MON 600 will be:**
- ✅ Available in ndmonlib
- ✅ Auto-documented in MON_CALLS.md
- ✅ Accessible from nd500x with NDIX
- ✅ Marked as non-standard SINTRAN
- ✅ ND-500 CPU only
- ✅ Fully tested

---

**Submit when ready! Structure:**
1. `mon_600B_NDIX.c` (handler implementation)
2. JSON metadata entry (for mon_registry.json)
3. Test cases (for test_mon_handlers.c)
4. Brief description (what MON 600 does)
