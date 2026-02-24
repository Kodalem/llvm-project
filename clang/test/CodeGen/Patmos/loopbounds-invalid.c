// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF1 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK-MAL,CHECK1" %s
// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF2 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK-MAL,CHECK2" %s
// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF3 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK-MAL,CHECK3" %s
// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF4 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK-MAL,CHECK4" %s
// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF5 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK-MAL,CHECK5" %s
// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF6 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK-MAL,CHECK6" %s
// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF7 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK7" %s
// RUN: %clang --target=patmos %s -S -emit-llvm -D DEF8 2>&1 | \
// RUN: FileCheck --check-prefixes="CHECK8" %s
// END.
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test the error messages given for various incorrect uses of the loopbound pragma
//
///////////////////////////////////////////////////////////////////////////////////////////////////

int main(int x) { 
	int count = x;
	
	// CHECK-MAL: pragma loopbound is malformed; expecting '#pragma loopbound min NUM max NUM'
	#ifdef DEF1
	_Pragma( "loopbound min -1 max 10" )
	#endif
	// CHECK1: _Pragma( "loopbound min -1 max 10" )
	#ifdef DEF2
	_Pragma( "loopbound min 0 max -1" )
	#endif
	// CHECK2: _Pragma( "loopbound min 0 max -1" )
	#ifdef DEF3
	_Pragma( "loopbound" )
	#endif
	// CHECK3: _Pragma( "loopbound" )
	#ifdef DEF4
	_Pragma( "loopbound min 10" )
	#endif
	// CHECK4: _Pragma( "loopbound min 10" )
	#ifdef DEF5
	_Pragma( "loopbound max 10" )
	#endif
	// CHECK5: _Pragma( "loopbound max 10" )
	#ifdef DEF6
	_Pragma( "loopbound 10 11" )
	#endif
	// CHECK6: _Pragma( "loopbound 10 11" )
	#ifdef DEF7
	_Pragma( "loopbound min 5 max 4" )
	#endif
	// CHECK7: invalid values; expected min >= 0 && max >= 0 && min <= max
	// CHECK7: _Pragma( "loopbound min 5 max 4" )
	#ifndef DEF8
	while(x >0){
		x = x/2;
		count++;
	}
	#endif
	
	#ifdef DEF8
	_Pragma( "loopbound min 0 max 40" )
	#endif
	// CHECK8: invalid minimum loop bound; do-while loops must have mininum bound >= 1
	// CHECK8: _Pragma( "loopbound min 0 max 40" )
	#ifdef DEF8
	do {
		x = x/2;
		count++;
	} while(x >0);
	#endif
	
	return count;	
}
