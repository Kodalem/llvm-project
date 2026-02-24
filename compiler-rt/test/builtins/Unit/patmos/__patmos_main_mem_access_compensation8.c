// RUN: INPUT=100; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN100"
// RUN: INPUT=101; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN101"

// RUN: INPUT=300; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN300"
// RUN: INPUT=301; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN301"
// RUN: INPUT=302; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN302"
// RUN: INPUT=303; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN3XX,IN303"

// RUN: INPUT=400; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN400"
// RUN: INPUT=401; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN401"
// RUN: INPUT=402; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN402"
// RUN: INPUT=403; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN403"
// RUN: INPUT=404; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN4XX,IN404"

// RUN: INPUT=500; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN500"
// RUN: INPUT=501; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN501"
// RUN: INPUT=502; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN502"
// RUN: INPUT=505; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN5XX,IN505"

// RUN: INPUT=1500; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1500"
// RUN: INPUT=1501; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1501"
// RUN: INPUT=1502; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1502"
// RUN: INPUT=1505; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1505"
// RUN: INPUT=1505; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1505"
// RUN: INPUT=1506; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1506"
// RUN: INPUT=1507; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1507"
// RUN: INPUT=1508; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1508"
// RUN: INPUT=1509; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1509"
// RUN: INPUT=1515; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN15XX,IN1515"

// RUN: INPUT=1600; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1600"
// RUN: INPUT=1601; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1601"
// RUN: INPUT=1602; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1602"
// RUN: INPUT=1605; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1605"
// RUN: INPUT=1605; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1605"
// RUN: INPUT=1606; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1606"
// RUN: INPUT=1607; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1607"
// RUN: INPUT=1608; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1608"
// RUN: INPUT=1609; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1609"
// RUN: INPUT=1615; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1615"
// RUN: INPUT=1616; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN16XX,IN1616"

// RUN: INPUT=1700; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1700"
// RUN: INPUT=1701; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1701"
// RUN: INPUT=1702; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1702"
// RUN: INPUT=1705; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1705"
// RUN: INPUT=1705; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1705"
// RUN: INPUT=1706; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1706"
// RUN: INPUT=1707; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1707"
// RUN: INPUT=1708; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1708"
// RUN: INPUT=1709; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1709"
// RUN: INPUT=1715; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1715"
// RUN: INPUT=1716; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1716"
// RUN: INPUT=1717; \
// RUN: %test-patmos-input-librt --print-stats __patmos_main_mem_access_compensation8 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN17XX,IN1717"

// END.

extern void __patmos_main_mem_access_compensation8();   
#define COMPENSATION_FN_ALIAS __patmos_main_mem_access_compensation8

#include "__patmos_main_mem_access_compensation_main.h"

// Ensure that regardless of how many accesses need to be compensated,
// the same number of instructions are executed (ensure is single-path)
// IN1XX: Operations: 41
// IN3XX: Operations: 41
// IN4XX: Operations: 41
// IN5XX: Operations: 41
// IN15XX: Operations: 54
// IN16XX: Operations: 67
// IN17XX: Operations: 67
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