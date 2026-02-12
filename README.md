# RISC-V Trace Control Interface — Minimal Software Model


# Trace Subsystem Controller

A spec-aligned modeling and abstraction layer for managing Trace Encoder (TE), Trace Funnel (TF), and Trace RamSink (TRS) hardware components.

## Overview

This project provides a driver-style API and a controller abstraction to manage the lifecycle of trace data—from hardware configuration and signal encoding to data retrieval.



### Features
* **Spec-aligned Modeling:** Precise register definitions including offsets and bitfields for TE, TF, and TRS.
* **Safe Register Access:** Built-in Read-Modify-Write (RMW) helpers to set, clear, and mask fields without corrupting adjacent bits.
* **Driver-style API:** Clean abstraction layer using only `ReadMemory()` and `WriteMemory()` primitives.
* **Validation:** Assertions integrated into the flow to validate register transitions.
* **Lightweight Simulation:** Capable of generating trace words and validating that the `Fetch()` logic operates correctly.

---

## Sequencing & API Reference

The controller sequences operations through four primary stages:

### 1. Configure
Sets up the hardware path. Components must be activated before specific feature bits are applied.

| Component | Register Field | Value | Requirement |
| :--- | :--- | :--- | :--- |
| **TraceEncoder** | `TR_TE_CONTROL[0]` | `1` | **Active**: Write 1 and read back to verify. |
| | `TR_TE_CONTROL[2]` | `1` | **InstTracing**: Must be set before Enable. |
| **TraceFunnel** | `TR_FUNNEL_CONTROL[0]`| `1` | **Active** |
| | `TR_FUNNEL_CONTROL[1]`| `1` | **Enable** |
| | `TR_FUNNEL_DIS_INPUT` | `0` | **Disable Input**: Set bits [15:0] to 0. |
| **TraceRamSink** | `TR_RAM_CONTROL[0]`   | `1` | **Active** |
| | `TR_RAM_CONTROL[1]`   | `1` | **Enable** |

### 2. Start
Activates the tracing stream.

* **TraceEncoder:** Set `TR_TE_CONTROL[1] = 1`.
    > **Note:** The `trTeEnable` bit **must** be set last, only after all other settings (e.g., `trTeInstTracing`, `trTeInstMode`) are confirmed.

### 3. Stop
Gracefully terminates the trace to prevent data loss.

* **TraceEncoder:** `TR_TE_CONTROL[1] = 0`. 
* **TraceFunnel:** `TR_FUNNEL_CONTROL[1] = 0`.
* **TraceRamSink:** `TR_RAM_CONTROL[1] = 0`.

### 4. Fetch
Retrieves stored data from the Sink.

* **Logic:** Continuous polling of pointers. While `!(sinkRamRP == sinkRamWP)`, read `TR_RAM_DATA` 4 bytes (1 word) at a time.
* *Note: This operation does not affect the state of the Encoder or Funnel.*

---

## Limitations & Scope

### Not Modeled
* **Downstream Congestion:** The behavior when the downstream stream is full is currently not modeled.
* **Overflow Logic:** Logic to set and handle the overflow bit is not implemented.

### Requirements
* A target environment supporting `ReadMemory` and `WriteMemory` hooks.

<!-- ![Alt Text](./doc/riscv_tci_block_diagram.drawio.png) -->

## Building and Running

### Prerequisites
* **CMake** (version 3.22.2 or higher)
* A C++17 compatible compiler (GCC, Clang, or MSVC)

### 🐧 Linux & macOS
```bash
# Generate build files
mkdir build
cmake -S . -B ./build/

# Compile the project
cmake --build build

# Run the executable
./build/tci

```

### VS Code
```bash
# Inside Powershell Terminal
# Generate build files 
cmake -S . -B .\build\

# Compile the project
MSBuild.exe .\build\tci.vcxproj

# Run the executable
.\build\Debug\tci.exe

```

### Expected Output - Debug Mode
#### Assertions are done after write so there is a extra READ operation by probe
```text

 Configuring Trace System... 
[PROBE WRITE]TraceRamSink + 0x000 (TR_RAM_CONTROL) <= 0x3
[PROBE READ]TraceRamSink + 0x000 (TR_RAM_CONTROL) => 0xb
[PROBE WRITE]TraceFunnel + 0x000 (TR_FUNNEL_CONTROL) <= 0x3
[PROBE READ]TraceFunnel + 0x000 (TR_FUNNEL_CONTROL) => 0x3
[PROBE WRITE]TraceFunnel + 0x008 (TR_FUNNEL_DIS_INPUT) <= 0x0
[PROBE READ]TraceFunnel + 0x008 (TR_FUNNEL_DIS_INPUT) => 0x0
[PROBE WRITE]TraceEncoder + 0x000 (TR_TE_CONTROL) <= 0x5000005
[PROBE READ]TraceEncoder + 0x000 (TR_TE_CONTROL) => 0x5000005

 Starting Trace System...
[PROBE READ]TraceEncoder + 0x000 (TR_TE_CONTROL) => 0x5000005
[PROBE WRITE]TraceEncoder + 0x000 (TR_TE_CONTROL) <= 0x5000007
[PROBE READ]TraceEncoder + 0x000 (TR_TE_CONTROL) => 0x5000007

 Stopping Trace System...
[PROBE READ]TraceEncoder + 0x000 (TR_TE_CONTROL) => 0x5000007
[PROBE WRITE]TraceEncoder + 0x000 (TR_TE_CONTROL) <= 0x5000005
[PROBE READ]TraceEncoder + 0x000 (TR_TE_CONTROL) => 0x5000005
[PROBE READ]TraceFunnel + 0x000 (TR_FUNNEL_CONTROL) => 0x3
[PROBE WRITE]TraceFunnel + 0x000 (TR_FUNNEL_CONTROL) <= 0x1
[PROBE READ]TraceFunnel + 0x000 (TR_FUNNEL_CONTROL) => 0x1
[PROBE READ]TraceRamSink + 0x000 (TR_RAM_CONTROL) => 0x3
[PROBE WRITE]TraceRamSink + 0x000 (TR_RAM_CONTROL) <= 0x1
[PROBE READ]TraceRamSink + 0x000 (TR_RAM_CONTROL) => 0x1

 Fetching trace data from TraceRamSink (size: 10 words)...
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x0
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0x3000
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x4
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0xdeadbeef
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x8
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0x3004
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0xc
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0xcafebabe
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x10
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0x3008
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x14
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0xaafe1000
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x18
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0x300c
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x1c
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x040 (TR_RAM_DATA) => 0xbafe1004
[PROBE READ]TraceRamSink + 0x028 (TR_RAM_RP_LOW) => 0x20
[PROBE READ]TraceRamSink + 0x020 (TR_RAM_WP_LOW) => 0x20

 Fetched trace data words: 3000 deadbeef 3004 cafebabe 3008 aafe1000 300c bafe1004 

 Main function finished execution

[Done] exited with code=0 in 0.724 seconds
```

<p align="center">
  <img src="./doc/riscv_tci_block_diagram.drawio.png" alt="Trace Controller Interface Block Diagram" width="900">
</p>