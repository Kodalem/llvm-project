; RUN: llc %s -o %t
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that a 'llvm.memcpy' for large numbers can be followed by an indirect call 
; in the same block.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@indirect_call = global void ()* null

define i8 @main(i8 %x, i8(i8)* %indirect_call)  {
entry:
  %ptr.1 = alloca i8, i32 2500, align 4
  %ptr.2 = alloca i8, i32 2500, align 4
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %ptr.1, i8* %ptr.2, i32 2500, i1 false)
  %val = load i8, i8* %ptr.1
  %res = call i8 %indirect_call(i8 %val)
  ret i8 %res
}
declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i1)