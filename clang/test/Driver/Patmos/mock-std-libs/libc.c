///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Mock lib for libc.a
//
///////////////////////////////////////////////////////////////////////////////////////////////////

volatile int PATMOS_MOCK_LIBC_INT = 5;

int patmos_mock_libc_function(int x)  {
	return x + PATMOS_MOCK_LIBC_INT;
}

int patmos_mock_libc_libm_shared_function(int x)  {
	return x + PATMOS_MOCK_LIBC_INT;
}

// By having this function perform a call, we can recognize itoa
// as a single-path root by looking for the single-path version
// of the called function.
int patmos_mock_libc_function_calling_function(int x)  {
	return x + patmos_mock_libc_function(PATMOS_MOCK_LIBC_INT);
}