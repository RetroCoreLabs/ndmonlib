# Integration Guide — Using ndmonlib in Your Emulator

> Step-by-step instructions to integrate ndmonlib MON call support into nd500x, nd100x, or other ND-series emulators.

## Overview

Integration involves:
1. Adding ndmonlib as a git submodule
2. Implementing architecture-specific callbacks
3. Wiring MON call detection in your CPU
4. Linking and building

**Time estimate**: 30-60 minutes

---

## Step 1: Add as Git Submodule

### For nd500x

```bash
cd ~/repos/nd500x
git submodule add ../ndmonlib external/ndmonlib
git submodule update --init
ls external/ndmonlib/  # Verify: should see include/, src/, metadata/, etc.
```

### For nd100x

```bash
cd ~/repos/nd100x
git submodule add ../ndmonlib external/ndmonlib
git submodule update --init
```

---

## Step 2: Implement Architecture Callbacks

Each architecture must implement callbacks for memory access, CPU registers, and optional features.

### Create Callback File

For ND-500, create `src/cpu/nd500_mon_callbacks.c`:

```c
/**
 * ND-500 specific MON call callbacks
 *
 * These implement the callback interface defined in <ndmon/mon_architecture.h>
 * Every MON call handler uses these callbacks to access machine state.
 */

#include <ndmon/mon.h>
#include "cpu.h"
#include "nd500_mmu.h"
#include "nd500_segment_alloc.h"

/* Memory access callbacks - use MMU for translation */

static uint32_t nd500_mon_read_word(void* cpu_ptr, uint32_t addr) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    // Translate virtual address via MMU
    uint32_t phys_addr = nd500_mmu_translate(cpu->mmu, addr);
    if (phys_addr == INVALID_ADDRESS) {
        return 0;  // Or handle error
    }
    // Read word (4 bytes on ND-500) from physical memory
    return machine_read_word(cpu->machine, phys_addr);
}

static void nd500_mon_write_word(void* cpu_ptr, uint32_t addr, uint32_t val) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    uint32_t phys_addr = nd500_mmu_translate(cpu->mmu, addr);
    if (phys_addr != INVALID_ADDRESS) {
        machine_write_word(cpu->machine, phys_addr, val);
    }
}

static uint8_t nd500_mon_read_byte(void* cpu_ptr, uint32_t addr) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    uint32_t phys_addr = nd500_mmu_translate(cpu->mmu, addr);
    if (phys_addr == INVALID_ADDRESS) {
        return 0;
    }
    return machine_read_byte(cpu->machine, phys_addr);
}

static void nd500_mon_write_byte(void* cpu_ptr, uint32_t addr, uint8_t val) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    uint32_t phys_addr = nd500_mmu_translate(cpu->mmu, addr);
    if (phys_addr != INVALID_ADDRESS) {
        machine_write_byte(cpu->machine, phys_addr, val);
    }
}

/* CPU register callbacks */

static void nd500_mon_set_k_flag(void* cpu_ptr, int value) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    // K flag is bit in ST1 register
    if (value) {
        cpu->st1 |= 0x0001;  // Set K flag
    } else {
        cpu->st1 &= ~0x0001;  // Clear K flag
    }
}

static void nd500_mon_set_error_code(void* cpu_ptr, int32_t code) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    // Error code goes in I1 (W1 register)
    cpu->i[0] = (uint32_t)code;
}

static void nd500_mon_set_i1(void* cpu_ptr, uint32_t value) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    cpu->i[0] = value;
}

static uint32_t nd500_mon_get_i1(void* cpu_ptr) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    return cpu->i[0];
}

/* Time callback */

static uint64_t nd500_mon_get_instruction_count(void* cpu_ptr) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    return cpu->instruction_count;
}

/* Segment allocation callbacks (ND-500 specific) */

static int nd500_mon_allocate_segment(void* cpu_ptr, void* machine_ptr,
                                      uint8_t domain, uint8_t seg_type,
                                      uint32_t size) {
    Nd500Cpu* cpu = (Nd500Cpu*)cpu_ptr;
    void* machine = machine_ptr;
    return nd500_segment_allocate(cpu, machine, domain, seg_type, size);
}

/* Initialize all callbacks for a MON context */

void nd500_mon_init_callbacks(MonContext* ctx, Nd500Cpu* cpu, void* machine) {
    ctx->cpu = cpu;
    ctx->machine = machine;

    /* Memory access */
    ctx->read_word = nd500_mon_read_word;
    ctx->write_word = nd500_mon_write_word;
    ctx->read_byte = nd500_mon_read_byte;
    ctx->write_byte = nd500_mon_write_byte;

    /* CPU registers */
    ctx->set_k_flag = nd500_mon_set_k_flag;
    ctx->set_error_code = nd500_mon_set_error_code;
    ctx->set_i1 = nd500_mon_set_i1;
    ctx->get_i1 = nd500_mon_get_i1;

    /* Time */
    ctx->get_instruction_count = nd500_mon_get_instruction_count;

    /* Segment allocation */
    ctx->allocate_segment = nd500_mon_allocate_segment;
}
```

---

## Step 3: Detect and Dispatch MON Calls

In your CPU instruction decoder (e.g., `src/cpu/nd500_indirect.c`):

```c
#include <ndmon/mon.h>

// In your instruction execution loop:

static MonResult execute_mon_call(Nd500Cpu* cpu, uint32_t mon_number) {
    MonContext ctx = {0};

    // Initialize callbacks for this MON call
    nd500_mon_init_callbacks(&ctx, cpu, cpu->machine);

    // Set MON parameters from CPU state
    ctx.mon_number = mon_number;
    ctx.arg_count = get_arg_count(mon_number);  // Per SINTRAN spec
    for (int i = 0; i < ctx.arg_count; i++) {
        ctx.arg_addresses[i] = cpu->i[i + 1];   // Args in I2-I5
    }

    // Dispatch to MON handler
    MonResult result = mon_dispatch(&ctx, mon_number, ctx.arg_addresses);

    // Handle control flow signals
    if (ctx.halt_requested) {
        cpu->stop_reason = STOP_MON_HALT;
        return MON_HALT;
    }
    if (ctx.break_requested) {
        cpu->stop_reason = STOP_MON_UNIMPLEMENTED;
        return MON_BREAK;
    }
    if (ctx.wait_requested) {
        // Rewind PC to CALLG instruction for retry
        cpu->pc -= 2;  // Depends on instruction size
        cpu->stop_reason = STOP_WAIT_INPUT;
        return MON_WAIT;
    }

    return result;
}

// In instruction decoder:

if (is_mon_call(instruction)) {
    uint32_t mon_number = extract_mon_number(instruction);
    MonResult result = execute_mon_call(cpu, mon_number);

    if (result != MON_SUCCESS) {
        // Handle error as needed
    }
}
```

---

## Step 4: Update CMakeLists.txt

### In nd500x/CMakeLists.txt

```cmake
# Add ndmonlib submodule
add_subdirectory(external/ndmonlib)

# Link mon library to all targets
link_libraries(mon)
```

### In nd500x/src/cpu/CMakeLists.txt

```cmake
add_library(nd500_cpu
    cpu.c
    nd500_indirect.c
    nd500_mon_callbacks.c        # NEW: ND-500 MON callbacks
    nd500_segment_alloc.c
    nd500_mmu.c
    # ... other source files
)

target_link_libraries(nd500_cpu
    mon                          # Link to ndmonlib
)

target_include_directories(nd500_cpu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/external/ndmonlib/include
)
```

---

## Step 5: Build and Test

### Build

```bash
cd ~/repos/nd500x
mkdir -p build && cd build
cmake ..
make

# Should compile without errors
echo "✓ Build successful"
```

### Quick Test

```bash
# Run a simple program that uses MON calls
./build/bin/nd500x --debug << 'EOF'
load examples/01-hello/hello.dom
run
EOF

# Should see:
# [MON] 2B (OUTBT) - Output: ...
# [MON] 3B (EXIT) - Process exited
```

---

## Step 6: Verify Integration

```bash
# Check that MON calls work
./build/bin/nd500x --debug << 'EOF'
load myprogram.dom
show mon status
run
EOF

# Expected output:
# MON Handler Status:
#   Validated: 180+
#   In Progress: 30+
#   Not Implemented: 20+
```

---

## Troubleshooting

### "mon.h: No such file or directory"

**Problem**: CMakeLists.txt not adding include directory

**Solution**:
```cmake
target_include_directories(YOUR_TARGET PUBLIC
    ${PROJECT_SOURCE_DIR}/external/ndmonlib/include
)
```

### "undefined reference to `mon_dispatch`"

**Problem**: Not linking to libmon library

**Solution**:
```cmake
target_link_libraries(YOUR_TARGET mon)
```

### "Unimplemented MON XXB"

**Problem**: Specific MON not yet implemented

**Solution**: See [docs/CARVING.md](CARVING.md) to implement it

---

## What's Next?

After successful integration:

1. **Run the test suite** — `ctest` to validate integration
2. **Use CARVING.md** — Identify + implement missing MON calls as needed
3. **Regenerate docs** — `python3 tools/generate_mon_calls.py` to track progress
4. **Share with nd100x** — Same process for ND-100 emulator

---

## Reference Files

| File | Purpose |
|------|---------|
| `include/ndmon/mon.h` | Main dispatcher API |
| `include/ndmon/mon_types.h` | MonContext, callback types |
| `include/ndmon/mon_errors.h` | SINTRAN error codes |
| `src/handlers/*.c` | 230+ MON handler implementations |
| `tools/generate_mon_calls.py` | Auto-generate documentation |
| `metadata/mon_registry.json` | Handler metadata + status |

---

For questions: See [docs/ARCHITECTURE.md](ARCHITECTURE.md) for system overview.
