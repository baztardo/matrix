# 4x4 Matrix Keypad Driver (Multi-Platform)

Fast, non-blocking matrix keypad driver with **simple** and **robust/bulletproof** versions.

## 🚀 Supported Platforms

- ✅ **Raspberry Pi Pico / Pico2** (RP2040 / RP2350)
- ✅ **STM32** (F0, F1, F3, F4, F7, G0, G4, H7, L0, L4, etc.)

## 📦 Two Versions Available

### 🟢 Simple Version (matrix.h/c)
**Best for:** Learning, prototyping, simple projects
- Polling-based scanning
- Easy to understand
- Minimal code
- [Quick Start Guide](#quick-start-pico)

### 🛡️ Robust Version (matrix_robust.h/c)
**Best for:** Production, safety-critical, power-sensitive
- **Hardware timer ISR** (precise timing)
- **Event queue** (never miss keypresses)
- **Ghost key detection** (prevents false triggers)
- **Stuck key detection** (identifies failures)
- **Power management** (sleep + wake on keypress)
- **Thread-safe** (RTOS compatible)
- **Statistics** (monitoring & diagnostics)
- 📖 [Full Documentation](ROBUST_FEATURES.md)

---

## Quick Links

| Document | Description |
|----------|-------------|
| **[ROBUST_FEATURES.md](ROBUST_FEATURES.md)** | Bulletproof version guide ⭐ |
| **[PLATFORMS.md](PLATFORMS.md)** | Pico vs STM32 comparison |
| **[FUNCTION_MODE.md](FUNCTION_MODE.md)** | F-key function mode |
| **[stm32/README_STM32.md](stm32/README_STM32.md)** | STM32 setup guide |
| **[stm32/PIN_SELECTION_GUIDE.md](stm32/PIN_SELECTION_GUIDE.md)** | STM32 pin selection |

---

## Quick Start (Pico)

### Simple Version
```bash
mkdir build && cd build
cmake .. && make
# Flash matrix_keypad.uf2 to Pico
```

See files: `matrix.h`, `matrix.c`, `main.c`

### Robust Version
Same build process, but include `matrix_robust.c` in CMakeLists.txt

See files: `matrix_robust.h`, `matrix_robust.c`, `main_robust_example.c`

---

## Features Comparison

| Feature | Simple | Robust |
|---------|--------|--------|
| Non-blocking | ✅ | ✅ |
| Hardware timer | ❌ | ✅ |
| Event queue | ❌ | ✅ (32 events) |
| Ghost detection | ❌ | ✅ |
| Stuck detection | ❌ | ✅ |
| Power mgmt | ❌ | ✅ |
| RTOS safe | ⚠️ | ✅ |
| Complexity | Low | Medium |

---

For detailed documentation, see [ROBUST_FEATURES.md](ROBUST_FEATURES.md)
