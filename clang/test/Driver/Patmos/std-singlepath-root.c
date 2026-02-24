// RUN: %clang --target=patmos -O2 %s -o %t -mpatmos-enable-cet \
// RUN: -mpatmos-cet-functions="patmos_mock_libc_function_calling_function"
// RUN: llvm-objdump -d %t | FileCheck %s
// END.
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests that can give a single-path root that is part of the standard library and not the program.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

extern int patmos_mock_libc_function_calling_function(int x);
// CHECK-DAG: <main>:
int main() { 
	return patmos_mock_libc_function_calling_function(1);
}
// CHECK-DAG: <patmos_mock_libc_function_pseudo_sp_>: