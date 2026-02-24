// RUN: %clang --target=patmos -c %s -mllvm --mpatmos-singlepath=main -o %t
// RUN: llvm-objdump -d %t | FileCheck %s
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests can pass flags on to LLC using '-mllvm'.
// We test that the flag works by ensuring no branches are present, which means singlepath code
// is used.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// RUN: %clang --target=patmos -c %s -mllvm --mpatmos-unknown-flag 2>&1 | FileCheck %s --check-prefix=ERROR
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests unkown "--mpatmos" flag is caught by LLC not clang
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// END.

volatile int x = 0;

// CHECK: <main>:
int main(int argc, char *argv[]) {
	if( argc == x) {
		return x;
	} else {
		return argc;
	}
}
// CHECK-NOT: br

// ERROR: llc: Unknown command line argument '--mpatmos-unknown-flag'