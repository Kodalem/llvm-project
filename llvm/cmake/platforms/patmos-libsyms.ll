
@llvm.used = appending global [44 x i8*] [
	i8* bitcast (i8* (i8*, i8*, i32)* @memcpy to i8*), 
	i8* bitcast (i8* (i8*, i32, i32)* @memset to i8*), 
	; The following are all the floating point or division operations.
	; They are needed here to ensure single-path code
	; knows to convert them to single-path, as calls to them
	; will only be visible after instruction selection.
	i8* bitcast (i32 (i32, i32)* @__divsi3 to i8*), 
	i8* bitcast (i32 (i32, i32)* @__udivsi3 to i8*), 
	i8* bitcast (i32 (i32, i32)* @__modsi3 to i8*), 
	i8* bitcast (i32 (i32, i32)* @__umodsi3 to i8*),
	i8* bitcast (i64 (i64, i32)* @__ashldi3 to i8*),
	i8* bitcast (i64 (i64, i32)* @__lshrdi3 to i8*),
	i8* bitcast (double (double, double)* @__adddf3 to i8*), 
	i8* bitcast (float (float, float)* @__addsf3 to i8*), 
	i8* bitcast (i32 (double, double)* @__ledf2 to i8*), 
	i8* bitcast (i32 (double, double)* @__gedf2 to i8*), 
	i8* bitcast (i32 (double, double)* @__unorddf2 to i8*), 
	i8* bitcast (i32 (double, double)* @__eqdf2 to i8*), 
	i8* bitcast (i32 (double, double)* @__ltdf2 to i8*), 
	i8* bitcast (i32 (double, double)* @__nedf2 to i8*), 
	i8* bitcast (i32 (double, double)* @__gtdf2 to i8*), 
	i8* bitcast (i32 (float, float)* @__lesf2 to i8*), 
	i8* bitcast (i32 (float, float)* @__gesf2 to i8*), 
	i8* bitcast (i32 (float, float)* @__unordsf2 to i8*), 
	i8* bitcast (double (double, double)* @__divdf3 to i8*), 
	i8* bitcast (float (float, float)* @__divsf3 to i8*), 
	i8* bitcast (double (float)* @__extendsfdf2 to i8*), 
	i8* bitcast (i64 (double)* @__fixdfdi to i8*), 
	i8* bitcast (i32 (double)* @__fixdfsi to i8*), 
	i8* bitcast (i64 (float)* @__fixsfdi to i8*), 
	i8* bitcast (i32 (float)* @__fixsfsi to i8*), 
	i8* bitcast (i64 (double)* @__fixunsdfdi to i8*), 
	i8* bitcast (i32 (double)* @__fixunsdfsi to i8*), 
	i8* bitcast (i64 (float)* @__fixunssfdi to i8*), 
	i8* bitcast (i32 (float)* @__fixunssfsi to i8*), 
	i8* bitcast (double (i64)* @__floatdidf to i8*), 
	i8* bitcast (float (i64)* @__floatdisf to i8*), 
	i8* bitcast (double (i32)* @__floatsidf to i8*), 
	i8* bitcast (float (i32)* @__floatsisf to i8*), 
	i8* bitcast (double (i64)* @__floatundidf to i8*), 
	i8* bitcast (float (i64)* @__floatundisf to i8*), 
	i8* bitcast (double (i32)* @__floatunsidf to i8*), 
	i8* bitcast (float (i32)* @__floatunsisf to i8*), 
	i8* bitcast (double (double, double)* @__muldf3 to i8*), 
	i8* bitcast (float (float, float)* @__mulsf3 to i8*), 
	i8* bitcast (double (double, double)* @__subdf3 to i8*), 
	i8* bitcast (float (float, float)* @__subsf3 to i8*), 
	i8* bitcast (float (double)* @__truncdfsf2 to i8*) ], section "llvm.metadata"

declare i8* @memset(i8*, i32, i32)
declare i8* @memcpy(i8*, i8*, i32)
declare i32 @__divsi3(i32, i32)
declare i32 @__udivsi3(i32, i32)
declare i32 @__modsi3(i32, i32)
declare i32 @__umodsi3(i32, i32)
declare i64 @__ashldi3(i64, i32)
declare i64 @__lshrdi3(i64, i32)
declare double @__adddf3(double, double)
declare float @__addsf3(float, float)
declare i32 @__ledf2(double, double)
declare i32 @__gedf2(double, double)
declare i32 @__unorddf2(double, double)
declare i32 @__eqdf2(double, double)
declare i32 @__ltdf2(double, double)
declare i32 @__nedf2(double, double)
declare i32 @__gtdf2(double, double)
declare i32 @__lesf2(float, float)
declare i32 @__gesf2(float, float)
declare i32 @__unordsf2(float, float)
declare double @__divdf3(double, double)
declare float @__divsf3(float, float)
declare double @__extendsfdf2(float)
declare i64 @__fixdfdi(double)
declare i32 @__fixdfsi(double)
declare i64 @__fixsfdi(float)
declare i32 @__fixsfsi(float)
declare i64 @__fixunsdfdi(double)
declare i32 @__fixunsdfsi(double)
declare i64 @__fixunssfdi(float)
declare i32 @__fixunssfsi(float)
declare double @__floatdidf(i64)
declare float @__floatdisf(i64)
declare double @__floatsidf(i32)
declare float @__floatsisf(i32)
declare double @__floatundidf(i64)
declare float @__floatundisf(i64)
declare double @__floatunsidf(i32)
declare float @__floatunsisf(i32)
declare double @__muldf3(double, double)
declare float @__mulsf3(float, float)
declare double @__subdf3(double, double)
declare float @__subsf3(float, float)
declare float @__truncdfsf2(double)