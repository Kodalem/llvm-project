// RUN: %build-patmos-input-librt --defsym input=100
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN100"
// RUN: %build-patmos-input-librt --defsym input=101
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN101"

// RUN: %build-patmos-input-librt --defsym input=300
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN300"
// RUN: %build-patmos-input-librt --defsym input=301
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN301"
// RUN: %build-patmos-input-librt --defsym input=302
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN302"
// RUN: %build-patmos-input-librt --defsym input=303
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN303"

// RUN: %build-patmos-input-librt --defsym input=400
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN400"
// RUN: %build-patmos-input-librt --defsym input=401
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN401"
// RUN: %build-patmos-input-librt --defsym input=402
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN402"
// RUN: %build-patmos-input-librt --defsym input=403
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN403"
// RUN: %build-patmos-input-librt --defsym input=404
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN404"

// RUN: %build-patmos-input-librt --defsym input=500
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN500"
// RUN: %build-patmos-input-librt --defsym input=501
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN501"
// RUN: %build-patmos-input-librt --defsym input=502
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN502"
// RUN: %build-patmos-input-librt --defsym input=505
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN505"

// RUN: %build-patmos-input-librt --defsym input=1500
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1500"
// RUN: %build-patmos-input-librt --defsym input=1501
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1501"
// RUN: %build-patmos-input-librt --defsym input=1502
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1502"
// RUN: %build-patmos-input-librt --defsym input=1505
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1505"
// RUN: %build-patmos-input-librt --defsym input=1505
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1505"
// RUN: %build-patmos-input-librt --defsym input=1506
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1506"
// RUN: %build-patmos-input-librt --defsym input=1507
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1507"
// RUN: %build-patmos-input-librt --defsym input=1508
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1508"
// RUN: %build-patmos-input-librt --defsym input=1509
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1509"
// RUN: %build-patmos-input-librt --defsym input=1515
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1515"

// RUN: %build-patmos-input-librt --defsym input=1600
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1600"
// RUN: %build-patmos-input-librt --defsym input=1601
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1601"
// RUN: %build-patmos-input-librt --defsym input=1602
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1602"
// RUN: %build-patmos-input-librt --defsym input=1605
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1605"
// RUN: %build-patmos-input-librt --defsym input=1605
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1605"
// RUN: %build-patmos-input-librt --defsym input=1606
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1606"
// RUN: %build-patmos-input-librt --defsym input=1607
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1607"
// RUN: %build-patmos-input-librt --defsym input=1608
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1608"
// RUN: %build-patmos-input-librt --defsym input=1609
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1609"
// RUN: %build-patmos-input-librt --defsym input=1615
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1615"
// RUN: %build-patmos-input-librt --defsym input=1616
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1616"

// RUN: %build-patmos-input-librt --defsym input=1700
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1700"
// RUN: %build-patmos-input-librt --defsym input=1701
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1701"
// RUN: %build-patmos-input-librt --defsym input=1702
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1702"
// RUN: %build-patmos-input-librt --defsym input=1705
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1705"
// RUN: %build-patmos-input-librt --defsym input=1705
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1705"
// RUN: %build-patmos-input-librt --defsym input=1706
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1706"
// RUN: %build-patmos-input-librt --defsym input=1707
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1707"
// RUN: %build-patmos-input-librt --defsym input=1708
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1708"
// RUN: %build-patmos-input-librt --defsym input=1709
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1709"
// RUN: %build-patmos-input-librt --defsym input=1715
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1715"
// RUN: %build-patmos-input-librt --defsym input=1716
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1716"
// RUN: %build-patmos-input-librt --defsym input=1717
// RUN: %exec-patmos --print-stats __patmos_main_mem_access_compensation8_di 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1717"

// END.

extern void __patmos_main_mem_access_compensation8_di();   
#define COMPENSATION_FN_ALIAS __patmos_main_mem_access_compensation8_di

#include "__patmos_main_mem_access_compensation_main.h"

// Ensure that regardless of how many accesses need to be compensated,
// the same number of instructions are executed (ensure is single-path)
// IN1XX: Operations: 48
// IN3XX: Operations: 48
// IN4XX: Operations: 48
// IN5XX: Operations: 48
// IN15XX: Operations: 61
// IN16XX: Operations: 74
// IN17XX: Operations: 74
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

// IN1500: Requests : 1
// IN1501: Requests : 2
// IN1502: Requests : 3
// IN1505: Requests : 6
// IN1506: Requests : 7
// IN1507: Requests : 8
// IN1508: Requests : 9
// IN1509: Requests : 10
// IN1515: Requests : 16

// IN1600: Requests : 1
// IN1601: Requests : 2
// IN1602: Requests : 3
// IN1605: Requests : 6
// IN1606: Requests : 7
// IN1607: Requests : 8
// IN1608: Requests : 9
// IN1609: Requests : 10
// IN1615: Requests : 16
// IN1616: Requests : 17

// IN1700: Requests : 1
// IN1701: Requests : 2
// IN1702: Requests : 3
// IN1705: Requests : 6
// IN1706: Requests : 7
// IN1707: Requests : 8
// IN1708: Requests : 9
// IN1709: Requests : 10
// IN1715: Requests : 16
// IN1716: Requests : 17
// IN1717: Requests : 18