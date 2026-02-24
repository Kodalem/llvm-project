
define i32 @_start() {  
entry:
  call void asm sideeffect "
		li		$$r31	= 2064384
		mts		$$ss	= $0
		mts		$$st	= $0
		callnd 	$1
		mts 	$$srb = $$r0
		mts 	$$sro = $$r0
		ret
		nop
		nop
		nop
	", "r,r,~{r31},~{ss},~{st}"
	(i32 2097152, i32 ()* @pre_main)
  unreachable
}

declare i32 @main(i32)
@input = external global i32

define i32 @pre_main() {  
entry:
  %0 = ptrtoint i32* @input to i32
  %1 = call i32 @main(i32 %0)
  ret i32 %1
}
