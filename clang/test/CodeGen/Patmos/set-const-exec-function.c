// RUN: %clang_cc1 -triple=patmos %s -mpatmos-enable-cet -mpatmos-cet-functions="some_fn" -emit-llvm -o - | FileCheck %s
// END.
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test that can designate a function to be constant execution time using the -mpatmos-cet-functions
// command line argument
//
///////////////////////////////////////////////////////////////////////////////////////////////////


// CHECK: @some_fn() 
// CHECK-SAME: #[[FN_ATTR:[0-9]]]
int some_fn()  { return 0;}

// CHECK: attributes #[[FN_ATTR]] = {
// CHECK-SAME: noinline 
// CHECK-SAME: "sp-root"