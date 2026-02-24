// RUN: %clang --target=patmos -O2 %s -o %t -mpatmos-enable-cet -mpatmos-cet-functions="some_fn"
// RUN: llvm-objdump -d %t | FileCheck %s
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test that can designate a function as constant execution using -mpatmos-cet-functions
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// RUN: %clang --target=patmos %s -o %t -mpatmos-enable-cet -mpatmos-cet-functions="some_fn" -mllvm --mpatmos-enable-cet=opposite
// RUN: llvm-objdump -d %t | FileCheck %s
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test that can specify which memory access compensation algorithm to use manually
// through llc's mpatmos-enable-cet option
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// RUN: %clang --target=patmos -c %s -mpatmos-enable-cet -mpatmos-cet-functions="some_fn" \
// RUN: -mllvm --mpatmos-singlepath-scheduler-config=some_fn
// RUN: llvm-objdump -d %t | FileCheck %s
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests can pass a flag to llc starting with "--patmos-singlepath..." while also using "-mpatmos-enable-cet".
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// END.

volatile int _0 = 0;

int some_fn()  {
	if(_0) {
		return _0 + 12;
	} else {
		return 3;
	}
}

// Ensure the function is there
// CHECK-LABEL: <some_fn>:

// Ensure it is singlepath (no branches)
// CHECK-NOT: {{([0-9]|[a-f])+}} br

// Stop checking for branching at next function
// CHECK-LABEL: >:

int main() {
	return some_fn();
}
