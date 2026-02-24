// RUN: %clang --target=patmos -O2 %s -o %t -mpatmos-enable-cet
// RUN: llvm-objdump -d %t | FileCheck %s
// END.
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test can enable constant execution time code using -mpatmos-enable-cet and specify singlepath
// through the attribute
//
///////////////////////////////////////////////////////////////////////////////////////////////////
volatile int _0 = 0;
__attribute__((singlepath))
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
// CHEC-LABEL: >:

int main() {
	return some_fn();
}

