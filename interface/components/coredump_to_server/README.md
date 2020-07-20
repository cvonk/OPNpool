# ESP32 - Core dump from flash to server

[![LICENSE](https://img.shields.io/github/license/jvonk/pact)](LICENSE)

## Goal

[The ESP-IDF API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/core_dump.html) provides support to generate core dumps on unrecoverable software errors. This useful technique allows post-mortem analysis of software state at the moment of failure.

This component is intended for deployed devices where you don't have access to the UART. It reads a core dump from flash and can forward them to e.g. a network server.

## Example

An example can be found in the [`test`](test) directory.

## Usage

The software relies on the master ESP-IDF SDK version v4.3-dev-472-gcf056a7d0 and accompanying tools.

Clone this component in your project's `components` directory, or use it as a git submodule.

In your project's `sdkconfig.defaults`, specify:
  - coredumping to flash in ELF format, and
  - a custom `partition.csv` that includes a `coredump` partition.

To call the component, declare callback functions to be used by `coredump_to_server()`.  These may be as simple as `printf()` statements, or more advanced where the core dump is forwarded to server on the network.

A simple example of the callback functions:

```c
static esp_err_t
_coredump_to_server_begin_cb(void * priv)
{
    ets_printf("================= CORE DUMP START =================\r\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_end_cb(void * priv)
{
    ets_printf("================= CORE DUMP END ===================\r\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_write_cb(void * priv, char const * const str)
{
    ets_printf("%s\r\n", str);
    return ESP_OK;
}
```

At the beginning of the `app_main`, but after `nvs_flash_init()`, use

```c
coredump_to_server_config_t coredump_cfg = {
    .start = _coredump_to_server_begin_cb,
    .end = _coredump_to_server_end_cb,
    .write = _coredump_to_server_write_cb,
};
coredump_to_server(&coredump_cfg);
```

In the simple case of printing the coredump to the UART, the `idf.py monitor` will catch the coredump and dispatch it to `espcoredump.py` for analysis.  An exmample of the output:

```
I (0) cpu_start: Starting scheduler on APP CPU.
Core dump started (further output muted)
Received  14 kB...
Core dump finished!
espcoredump.py v0.4-dev
===============================================================
==================== ESP32 CORE DUMP START ====================

Crashed task handle: 0x3ffb7170, name: 'main', GDB name: 'process 1073443184'

================== CURRENT THREAD REGISTERS ===================
exccause       0x1d (StoreProhibitedCause)
excvaddr       0x0
epc1           0x0
epc2           0x0
epc3           0x0
epc4           0x0
epc5           0x0
epc6           0x0
epc7           0x0
eps2           0x0
eps3           0x0
eps4           0x0
eps5           0x0
eps6           0x0
eps7           0x400e7e37
pc             0x400848b6       0x400848b6 <panic_abort+18>
lbeg           0x4000c349       1073791817
lend           0x4000c36b       1073791851
lcount         0x0      0
sar            0x10     16
ps             0x60d20  396576
threadptr      <unavailable>
br             <unavailable>
scompare1      <unavailable>
acclo          <unavailable>
acchi          <unavailable>
m0             <unavailable>
m1             <unavailable>
m2             <unavailable>
m3             <unavailable>
expstate       <unavailable>
f64r_lo        <unavailable>
f64r_hi        <unavailable>
f64s           <unavailable>
fcr            <unavailable>
fsr            <unavailable>
a0             0x80084fa4       -2146938972
a1             0x3ffb6fb0       1073442736
a2             0x3ffb6ffb       1073442811
a3             0x3ffb7028       1073442856
a4             0xa      10
a5             0x0      0
a6             0x3ffb6d60       1073442144
a7             0xff000000       -16777216
a8             0x0      0
a9             0x1      1
a10            0x3ffb6ff9       1073442809
a11            0x3ffb6ff9       1073442809
a12            0x0      0
a13            0x3ffae988       1073408392
a14            0x0      0
a15            0x0      0

==================== CURRENT THREAD STACK =====================
#0  0x400848b6 in panic_abort (details=0x3ffb6ffb \"abort() was called at PC 0x400d961f on core 0\") at C:/Users/coert/espressif/esp-idf-master/components/esp_system/panic.c:360
#1  0x40084fa4 in esp_system_abort (details=0x3ffb6ffb \"abort() was called at PC 0x400d961f on core 0\") at C:/Users/coert/espressif/esp-idf-master/components/esp_system/system_api.c:105
Backtrace stopped: previous frame identical to this frame (corrupt stack?)

======================== THREADS INFO =========================
  Id   Target Id         Frame
* 1    process 1073443184 0x400848b6 in panic_abort (details=0x3ffb6ffb \"abort() was called at PC 0x400d961f on core
0\") at C:/Users/coert/espressif/esp-idf-master/components/esp_system/panic.c:360
  2    process 1073446968 0x400e810e in esp_pm_impl_waiti () at C:/Users/coert/espressif/esp-idf-master/components/esp32/pm_esp32.c:484
  3    process 1073445076 0x400e810e in esp_pm_impl_waiti () at C:/Users/coert/espressif/esp-idf-master/components/esp32/pm_esp32.c:484
  4    process 1073449616 0x4008433c in esp_crosscore_int_send_yield (core_id=0) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115
  5    process 1073437244 0x4008433c in esp_crosscore_int_send_yield (core_id=1) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115
  6    process 1073435864 0x4008433c in esp_crosscore_int_send_yield (core_id=0) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115
  7    process 1073412788 0x4008433c in esp_crosscore_int_send_yield (core_id=0) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115

==================== THREAD 1 (TCB: 0x3ffb7170, name: 'main') =====================
#0  0x400848b6 in panic_abort (details=0x3ffb6ffb \"abort() was called at PC 0x400d961f on core 0\") at C:/Users/coert/espressif/esp-idf-master/components/esp_system/panic.c:360
#1  0x40084fa4 in esp_system_abort (details=0x3ffb6ffb \"abort() was called at PC 0x400d961f on core 0\") at C:/Users/coert/espressif/esp-idf-master/components/esp_system/system_api.c:105
Backtrace stopped: previous frame identical to this frame (corrupt stack?)

==================== THREAD 2 (TCB: 0x3ffb8038, name: 'IDLE1') =====================
#0  0x400e810e in esp_pm_impl_waiti () at C:/Users/coert/espressif/esp-idf-master/components/esp32/pm_esp32.c:484
#1  0x400d5756 in esp_vApplicationIdleHook () at C:/Users/coert/espressif/esp-idf-master/components/esp_common/src/freertos_hooks.c:63
#2  0x40085a7d in prvIdleTask (pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/tasks.c:3385
#3  0x40084fac in vPortTaskWrapper (pxCode=0x40085a74 <prvIdleTask>, pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/xtensa/port.c:169

==================== THREAD 3 (TCB: 0x3ffb78d4, name: 'IDLE0') =====================
#0  0x400e810e in esp_pm_impl_waiti () at C:/Users/coert/espressif/esp-idf-master/components/esp32/pm_esp32.c:484
#1  0x400d5756 in esp_vApplicationIdleHook () at C:/Users/coert/espressif/esp-idf-master/components/esp_common/src/freertos_hooks.c:63
#2  0x40085a7d in prvIdleTask (pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/tasks.c:3385
#3  0x40084fac in vPortTaskWrapper (pxCode=0x40085a74 <prvIdleTask>, pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/xtensa/port.c:169

==================== THREAD 4 (TCB: 0x3ffb8a90, name: 'Tmr Svc') =====================
#0  0x4008433c in esp_crosscore_int_send_yield (core_id=0) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115
#1  0x4008745a in prvProcessTimerOrBlockTask (xNextExpireTime=<optimized out>, xListWasEmpty=<optimized out>) at C:/Users/coert/espressif/esp-idf-master/components/soc/src/esp32/include/hal/cpu_ll.h:39
#2  0x40087547 in prvTimerTask (pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/timers.c:544
#3  0x40084fac in vPortTaskWrapper (pxCode=0x40087538 <prvTimerTask>, pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/xtensa/port.c:169

==================== THREAD 5 (TCB: 0x3ffb5a3c, name: 'ipc1') =====================
#0  0x4008433c in esp_crosscore_int_send_yield (core_id=1) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115
#1  0x40087e1c in xQueueGenericReceive (xQueue=0x3ffaff74, pvBuffer=0x0, xTicksToWait=<optimized out>, xJustPeeking=0) at C:/Users/coert/espressif/esp-idf-master/components/soc/src/esp32/include/hal/cpu_ll.h:39
#2  0x4008311b in ipc_task (arg=0x1) at C:/Users/coert/espressif/esp-idf-master/components/esp_ipc/ipc.c:51
#3  0x40084fac in vPortTaskWrapper (pxCode=0x400830e8 <ipc_task>, pvParameters=0x1) at C:/Users/coert/espressif/esp-idf-master/components/freertos/xtensa/port.c:169

==================== THREAD 6 (TCB: 0x3ffb54d8, name: 'ipc0') =====================
#0  0x4008433c in esp_crosscore_int_send_yield (core_id=0) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115
#1  0x40087e1c in xQueueGenericReceive (xQueue=0x3ffafe78, pvBuffer=0x0, xTicksToWait=<optimized out>, xJustPeeking=0) at C:/Users/coert/espressif/esp-idf-master/components/soc/src/esp32/include/hal/cpu_ll.h:39
#2  0x4008311b in ipc_task (arg=0x0) at C:/Users/coert/espressif/esp-idf-master/components/esp_ipc/ipc.c:51
#3  0x40084fac in vPortTaskWrapper (pxCode=0x400830e8 <ipc_task>, pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/xtensa/port.c:169

==================== THREAD 7 (TCB: 0x3ffafab4, name: 'esp_timer') =====================
#0  0x4008433c in esp_crosscore_int_send_yield (core_id=0) at C:/Users/coert/espressif/esp-idf-master/components/esp32/crosscore_int.c:115
#1  0x40087e1c in xQueueGenericReceive (xQueue=0x3ffaea5c, pvBuffer=0x0, xTicksToWait=<optimized out>, xJustPeeking=0) at C:/Users/coert/espressif/esp-idf-master/components/soc/src/esp32/include/hal/cpu_ll.h:39
#2  0x400d58df in timer_task (arg=0x0) at C:/Users/coert/espressif/esp-idf-master/components/esp_timer/src/esp_timer.c:329
#3  0x40084fac in vPortTaskWrapper (pxCode=0x400d58cc <timer_task>, pvParameters=0x0) at C:/Users/coert/espressif/esp-idf-master/components/freertos/xtensa/port.c:169


======================= ALL MEMORY REGIONS ========================
Name   Address   Size   Attrs
.rtc.text 0x400c0000 0x0 RW
.rtc.dummy 0x3ff80000 0x0 RW
.rtc.force_fast 0x3ff80000 0x0 RW
.rtc_noinit 0x50000000 0x0 RW
.rtc.force_slow 0x50000000 0x0 RW
.iram0.vectors 0x40080000 0x404 R XA
.iram0.text 0x40080404 0xacd8 R XA
.dram0.data 0x3ffb0000 0x3a2c RW A
.noinit 0x3ffb3a2c 0x0 RW
.flash.rodata 0x3f400020 0x7394 RW A
.flash.text 0x400d0020 0x18b4c R XA
.iram0.data 0x4008b0dc 0x0 RW
.iram0.bss 0x4008b0dc 0x0 RW
.dram0.heap_start 0x3ffb50b8 0x0 RW
.coredump.tasks.data 0x3ffb7170 0x15c RW
.coredump.tasks.data 0x3ffb6ef0 0x278 RW
.coredump.tasks.data 0x3ffb8038 0x15c RW
.coredump.tasks.data 0x3ffb7e90 0x1a0 RW
.coredump.tasks.data 0x3ffb78d4 0x15c RW
.coredump.tasks.data 0x3ffb7720 0x1ac RW
.coredump.tasks.data 0x3ffb8a90 0x15c RW
.coredump.tasks.data 0x3ffb88c0 0x1c8 RW
.coredump.tasks.data 0x3ffb5a3c 0x15c RW
.coredump.tasks.data 0x3ffb5870 0x1c4 RW
.coredump.tasks.data 0x3ffb54d8 0x15c RW
.coredump.tasks.data 0x3ffb5310 0x1c0 RW
.coredump.tasks.data 0x3ffafab4 0x15c RW
.coredump.tasks.data 0x3ffaf8e0 0x1cc RW

===================== ESP32 CORE DUMP END =====================
===============================================================
Done!
I (3414) coredump_to_server_test: waiting for BOOT/RESET button ..
```

## Feedback

We love to hear from you.  Please use the usual Github mechanisms to contact me.

## LICENSE

Copyright 2020 Coert Vonk

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
