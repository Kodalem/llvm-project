// RUN: %build-patmos-input-librt --defsym input=100
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN100"
// RUN: %build-patmos-input-librt --defsym input=101
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN101"

// RUN: %build-patmos-input-librt --defsym input=300
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN300"
// RUN: %build-patmos-input-librt --defsym input=301
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN301"
// RUN: %build-patmos-input-librt --defsym input=302
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN302"
// RUN: %build-patmos-input-librt --defsym input=303
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN303"

// RUN: %build-patmos-input-librt --defsym input=400
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN400"
// RUN: %build-patmos-input-librt --defsym input=401
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN401"
// RUN: %build-patmos-input-librt --defsym input=402
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN402"
// RUN: %build-patmos-input-librt --defsym input=403
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN403"
// RUN: %build-patmos-input-librt --defsym input=404
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN404"

// RUN: %build-patmos-input-librt --defsym input=500
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN500"
// RUN: %build-patmos-input-librt --defsym input=501
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN501"
// RUN: %build-patmos-input-librt --defsym input=502
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN502"
// RUN: %build-patmos-input-librt --defsym input=505
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation2_di 2>&1 | \
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