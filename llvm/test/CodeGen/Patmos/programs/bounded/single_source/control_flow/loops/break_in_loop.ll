; RUN: EXEC_ARGS="0=10 1=10 6=10 7=17 9=17 13=17"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Test conditionally breaking out of a loop.
;
; The following is the equivalent C code:
;
; volatile int _7 = 7;
;
; int main(int x){
; 	#pragma loopbound min 0 max 5
; 	while(x > 0){
; 		if(x == 7){
; 			break;
; 		}
; 		x--;
; 	}
; 	x += 10;
; 	return x;
; }
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////
@_7 = global i32 7
define i32 @main(i32 %x) {
entry:
  br label %while.cond

while.cond:
  %x.addr.0 = phi i32 [ %x, %entry ], [ %dec, %if.end ]
  %cmp = icmp sgt i32 %x.addr.0, 0
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %cmp, label %while.body, label %while.end

while.body:
  %0 = load volatile i32, i32* @_7
  %cmp1 = icmp eq i32 %x.addr.0, %0
  br i1 %cmp1, label %while.end, label %if.end

if.end:
  %dec = add nsw i32 %x.addr.0, -1
  br label %while.cond

while.end:
  %add = add nsw i32 %x.addr.0, 10
  ret i32 %add
}

declare void @llvm.loop.bound(i32, i32)
