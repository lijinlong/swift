// RUN: %sourcekitd-test -req=related-idents -pos=%(line + 1):10 %s -- %s | %FileCheck %s
func foo(x: Int) {
#if true
  print(x)
#else
  print(x)
#endif
}

// CHECK: START RANGES
// CHECK-NEXT: 2:10 - 1
// CHECK-NEXT: 4:9 - 1
// CHECK-NEXT: END RANGES
