     
int main(int input)
{	
	// We treat the first 2 decimals as the remainder,
	// and the rest as the max
	unsigned remaining = input%100;
	unsigned max = input/100;
	
	// We put inputs in r23/r24 without registering them as clobbers
	// to ensure the compiler doesn't mess with them before the call
	asm volatile (
		"mov	$r23 = %[max];"
		"mov	$r24 = %[remaining];"
		:
		: [max] "r" (max), [remaining] "r" (remaining)
		:
	);
	COMPENSATION_FN_ALIAS();
		
	return 0;
}