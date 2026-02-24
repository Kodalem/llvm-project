/* ===---- patmos_main_mem_access_compensation.c - Access Compensation -----===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * Function that can compensate for the variability in main memory accesses.
 *
 * Are are given in r23 (max) and r24 (remaining).
 *
 * "max" is the maximum number of extra enabled accesses a function can need
 * "remaining" is the number of enabled accesses a given call to the function
 * is missing.
 * 
 * __patmos_main_mem_access_compensation performs "remaining" number of enabled
 * loads to main memory.
 *
 * __patmos_main_mem_access_compensation has a custom calling convention:
 * It can only clobber r23, r24, p1, and p2. All other register must remain the same.
 *
 * ===----------------------------------------------------------------------===
 */

#include "../int_lib.h"

void __patmos_main_mem_access_compensation() __attribute__((alias("__patmos_main_mem_access_compensation4")));
void __patmos_main_mem_access_compensation_di() __attribute__((alias("__patmos_main_mem_access_compensation4_di")));

void
__patmos_main_mem_access_compensation1() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation1()
{
    __asm__ volatile (
		"__patmos_main_mem_access_compensation_loop:;"
		"cmpneq $p1 = $r23, 1;"
		"cmpneq $p2 = $r24, 0;"
		"lwm ($p2) $r0 = [$r0];"
		"br ($p1) __patmos_main_mem_access_compensation_loop; "
		"sub ($p2) $r24 = $r24, 1;"
		"sub ($p1) $r23 = $r23, 1;"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}

void
__patmos_main_mem_access_compensation2() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation2()
{
    __asm__ volatile (
		"__patmos_main_mem_access_compensation2_loop:;"
		"cmpult $p2 = $r23, 2;" // We want r3 >= 2, so negate p2 use
		"cmpult $p1 = $r24, 2;" // We want r4 >= 2, so negate p1 use
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"br (!$p2) __patmos_main_mem_access_compensation2_loop; "
		"sub (!$p1) $r24 = $r24, 2;"
		"sub (!$p2) $r23 = $r23, 2;"
		"cmpult $p1 = $r0, $r24;"
		"lwm ($p1) $r0 = [$r0];"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}

void
__patmos_main_mem_access_compensation4() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation4()
{
    __asm__ volatile (
		"__patmos_main_mem_access_compensation4_loop:;"
		"cmpult $p2 = $r23, 4;" // We want r3 >= 4, so negate p2 use
		"cmpult $p1 = $r24, 4;" // We want r4 >= 4, so negate p1 use
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"br (!$p2) __patmos_main_mem_access_compensation4_loop; "
		"sub (!$p1) $r24 = $r24, 4;"
		"sub $r23 = $r23, 4;"
		"cmpult $p1 = $r0, $r24;"
		"sub ($p1) $r24 =  $r24, 1;"
		"lwm ($p1) $r0 = [$r0];"
		"cmpult $p1 = $r0, $r24;"
		"sub ($p1) $r24 =  $r24, 1;"
		"lwm ($p1) $r0 = [$r0];"
		"cmpult $p1 = $r0, $r24;"
		"lwm ($p1) $r0 = [$r0];"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}

void
__patmos_main_mem_access_compensation8() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation8()
{
    __asm__ volatile (
		"brnd __patmos_main_mem_access_compensation8_cond; "
		"__patmos_main_mem_access_compensation8_loop:;"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"__patmos_main_mem_access_compensation8_cond:;"
		"cmpult $p2 = $r23, 8;"
		"cmpult $p1 = $r24, 8;"
		"br (!$p2) __patmos_main_mem_access_compensation8_loop; "
		"sub (!$p2) $r23 = $r23, 8;"
		"sub (!$p1) $r24 = $r24, 8;"
		// We might have decremented r3 to less than r4 (which is <8). 
		// Therefore reset to max possible for r4
		"li  $r23 = 7;" 
		"__patmos_main_mem_access_compensation82_loop:;"
		"cmpult $p2 = $r23, 2;" // We want r3 >= 2, so negate p2 use
		"cmpult $p1 = $r24, 2;" // We want r4 >= 2, so negate p1 use
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"br (!$p2) __patmos_main_mem_access_compensation82_loop; "
		"sub (!$p1) $r24 = $r24, 2;"
		"sub (!$p2) $r23 = $r23, 2;"
		"cmpult $p1 = $r0, $r24;"
		"lwm ($p1) $r0 = [$r0];"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}

void
__patmos_main_mem_access_compensation1_di() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation1_di()
{ 
    __asm__ volatile (
		"__patmos_main_mem_access_compensation1_di_loop:;"
		"{ cmpneq $p1 = $r23, 1; cmpneq $p2 = $r24, 0 }"
		"br ($p1) __patmos_main_mem_access_compensation1_di_loop; "
		"lwm ($p2) $r0 = [$r0];"
		"{ sub ($p2) $r24 = $r24, 1; sub ($p1) $r23 = $r23, 1 }"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}

void
__patmos_main_mem_access_compensation2_di() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation2_di()
{
    __asm__ volatile (
		"__patmos_main_mem_access_compensation2_di_loop:;"
		"{ cmpult $p2 = $r23, 2; cmpult $p1 = $r24, 2 }" 	// We want r3 >= 2, so negate p2 use
															// We want r4 >= 2, so negate p1 use
		"br (!$p2) __patmos_main_mem_access_compensation2_di_loop; "
		"{ lwm (!$p1) $r0 = [$r0]; sub (!$p1) $r24 = $r24, 2 }"
		"{ lwm (!$p1) $r0 = [$r0]; sub (!$p2) $r23 = $r23, 2 }"
		"cmpult $p1 = $r0, $r24;"
		"lwm ($p1) $r0 = [$r0];"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}

void
__patmos_main_mem_access_compensation4_di() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation4_di()
{
    __asm__ volatile (
		"__patmos_main_mem_access_compensation4_di_loop:;"
		"{ cmpult $p2 = $r23, 4; cmpult $p1 = $r24, 4 };"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"br (!$p2) __patmos_main_mem_access_compensation4_di_loop; "
		"{ lwm (!$p1) $r0 = [$r0]; sub (!$p1) $r24 = $r24, 4 };"
		"{ lwm (!$p1) $r0 = [$r0]; sub $r23 = $r23, 4 };"
		
		"{ cmpult $p1 = $r24, 3; cmpult $p2 = $r24, 2 }"
		"{ lwm (!$p1) $r0 = [$r0]; cmpult $p1 = $r24, 1 };"
		"lwm (!$p2) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}

void
__patmos_main_mem_access_compensation8_di() __attribute__((naked,noinline));
void
__patmos_main_mem_access_compensation8_di()
{
    __asm__ volatile (
		"{ brnd __patmos_main_mem_access_compensation8_di_cond; pset $p1}"
		"__patmos_main_mem_access_compensation8_di_loop:;"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"lwm (!$p1) $r0 = [$r0];"
		"__patmos_main_mem_access_compensation8_di_cond:;"
		"{ lwm (!$p1) $r0 = [$r0]; cmpult $p2 = $r23, 8 }"
		"{ lwm (!$p1) $r0 = [$r0]; cmpult $p1 = $r24, 8 }"
		"br (!$p2) __patmos_main_mem_access_compensation8_di_loop; "
		"{ lwm (!$p1) $r0 = [$r0]; sub (!$p2) $r23 = $r23, 8 }"
		"{ lwm (!$p1) $r0 = [$r0]; sub (!$p1) $r24 = $r24, 8 }"
		// We might have decremented r3 to less than r4 (which is <8). 
		// Therefore reset to max possible for r4
		"li  $r23 = 7;" 
		"__patmos_main_mem_access_compensation8_di2_loop:;"
		"{ cmpult $p2 = $r23, 2; cmpult $p1 = $r24, 2 }"	// We want r3 >= 2, so negate p2 use
															// We want r4 >= 2, so negate p1 use
		"br (!$p2) __patmos_main_mem_access_compensation8_di2_loop; "
		"{ lwm (!$p1) $r0 = [$r0]; sub (!$p1) $r24 = $r24, 2 }"
		"{ lwm (!$p1) $r0 = [$r0]; sub (!$p2) $r23 = $r23, 2 }"
		"cmpult $p1 = $r0, $r24;"
		"lwm ($p1) $r0 = [$r0];"
		"retnd;"
		:
		: 
		: "r23", "r24"
	);
}