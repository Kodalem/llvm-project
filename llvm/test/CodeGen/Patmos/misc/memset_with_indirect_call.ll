; RUN: llc %s -o %t
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that a 'llvm.memset' for large numbers can be followed by an indirect call 
; in the same block.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@indirect_call = global void ()* null

define i8 @main(i8 %x, i8(i8)* %indirect_call)  {
entry:
  %ptr = alloca i8, i32 2500, align 4
  call void @llvm.memset.p0i8.p0i8.i32(i8* %ptr, i8 %x, i32 2500, i1 false)
  %val = load i8, i8* %ptr
  %res = call i8 %indirect_call(i8 %val)
  ret i8 %res
}
declare void @llvm.memset.p0i8.p0i8.i32(i8*, i8, i32, i1)