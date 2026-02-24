// RUN: INPUT=100; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN100"
// RUN: INPUT=101; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN101"

// RUN: INPUT=300; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN300"
// RUN: INPUT=301; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN301"
// RUN: INPUT=302; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN302"
// RUN: INPUT=303; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN303"

// RUN: INPUT=400; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN400"
// RUN: INPUT=401; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN401"
// RUN: INPUT=402; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN402"
// RUN: INPUT=403; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN403"
// RUN: INPUT=404; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN404"

// RUN: INPUT=500; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN500"
// RUN: INPUT=501; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN501"
// RUN: INPUT=502; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN502"
// RUN: INPUT=505; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN505"
// END.

extern void __patmos_main_mem_access_compensation2_di();   
#define COMPENSATION_FN_ALIAS __patmos_main_mem_access_compensation2_di

#include "__patmos_main_mem_access_compensation_main.h"

// Ensure that regardless of how many accesses need to be compensated,
// the same number of instructions are executed (ensure is single-path)
// IN1XX: Operations: 13
// IN3XX: Operations: 20
// IN4XX: Operations: 27
// IN5XX: Operations: 27
// CHECK-LABEL: Instruction Cache Statistics:
// CHECK-LABEL: Data Cache Statistics:
// CHECK-LABEL: Stack Cache Statistics:
// CHECK-LABEL: Main Memory Statistics:

// Ensure the correct number of accesses go to main memory
// 1 accesses always comes from the call itself (load instructions)

// IN100: Requests : 1
// IN101: Requests : 2

// IN300: Requests : 1
// IN301: Requests : 2
// IN302: Requests : 3
// IN303: Requests : 4

// IN400: Requests : 1
// IN401: Requests : 2
// IN402: Requests : 3
// IN403: Requests : 4
// IN404: Requests : 5

// IN500: Requests : 1
// IN501: Requests : 2
// IN502: Requests : 3
// IN505: Requests : 6