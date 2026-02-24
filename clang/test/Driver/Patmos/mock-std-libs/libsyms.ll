
@llvm.used = appending global [3 x i8*] [
	i8* bitcast (i32 (i32)* @patmos_mock_libc_function to i8*),
	i8* bitcast (i32 (i32)* @patmos_mock_libm_function to i8*),
	i8* bitcast (i32 (i32)* @patmos_mock_crt0_function to i8*)], section "llvm.metadata"

declare i32 @patmos_mock_libc_function(i32)
declare i32 @patmos_mock_libm_function(i32)
declare i32 @patmos_mock_crt0_function(i32)