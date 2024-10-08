// RUN: %clangxx %s -O1 -o %t -fexperimental-sanitize-metadata=covered,uar && %t | FileCheck %s
// RUN: %clangxx %s -O1 -o %t -fexperimental-sanitize-metadata=covered,uar -fsanitize=address,signed-integer-overflow,alignment && %t | FileCheck %s
// RUN: %clangxx %s -O1 -o %t -mcmodel=large -fexperimental-sanitize-metadata=covered,uar -fsanitize=address,signed-integer-overflow,alignment && %t | FileCheck %s

// CHECK-DAG: metadata add version 2

__attribute__((noinline, not_tail_called)) void escape(const volatile void *p) {
  [[maybe_unused]] static const volatile void *sink;
  sink = p;
}

__attribute__((noinline, not_tail_called)) void use(int x) {
  static volatile int sink;
  sink += x;
}

// CHECK-DAG: empty: features=0 stack_args=0
void empty() {}

// CHECK-DAG: simple: features=0 stack_args=0
int simple(int *data, int index) { return data[index + 1]; }

// CHECK-DAG: builtins: features=0 stack_args=0
int builtins() {
  int x = 0;
  __builtin_prefetch(&x);
  return x;
}

// CHECK-DAG: ellipsis: features=0 stack_args=0
void ellipsis(const char *fmt, ...) {
  int x;
  escape(&x);
}

// CHECK-DAG: non_empty_function: features=2 stack_args=0
void non_empty_function() {
  int x;
  escape(&x);
}

// CHECK-DAG: no_stack_args: features=2 stack_args=0
void no_stack_args(long a0, long a1, long a2, long a3, long a4, long a5) {
  int x;
  escape(&x);
}

// CHECK-DAG: stack_args: features=6 stack_args=16
void stack_args(long a0, long a1, long a2, long a3, long a4, long a5, long a6) {
  int x;
  escape(&x);
}

// CHECK-DAG: more_stack_args: features=6 stack_args=32
void more_stack_args(long a0, long a1, long a2, long a3, long a4, long a5,
                     long a6, long a7, long a8) {
  int x;
  escape(&x);
}

// CHECK-DAG: struct_stack_args: features=6 stack_args=144
struct large {
  char x[131];
};
void struct_stack_args(large a) {
  int x;
  escape(&x);
}

__attribute__((noinline)) int tail_called(int x) { return x; }

// CHECK-DAG: with_tail_call: features=2
int with_tail_call(int x) { [[clang::musttail]] return tail_called(x); }

__attribute__((noinline, noreturn)) int noreturn(int x) { __builtin_trap(); }

// CHECK-DAG: with_noreturn_tail_call: features=0
int with_noreturn_tail_call(int x) { return noreturn(x); }

// CHECK-DAG: local_array: features=0
void local_array(int x) {
  int data[10];
  use(data[x]);
}

// CHECK-DAG: local_alloca: features=0
void local_alloca(int size, int i, int j) {
  volatile int *p = static_cast<int *>(__builtin_alloca(size));
  p[i] = 0;
  use(p[j]);
}

// CHECK-DAG: escaping_alloca: features=2
void escaping_alloca(int size, int i) {
  volatile int *p = static_cast<int *>(__builtin_alloca(size));
  escape(&p[i]);
}

#define FUNCTIONS                                                              \
  FN(empty);                                                                   \
  FN(simple);                                                                  \
  FN(builtins);                                                                \
  FN(ellipsis);                                                                \
  FN(non_empty_function);                                                      \
  FN(no_stack_args);                                                           \
  FN(stack_args);                                                              \
  FN(more_stack_args);                                                         \
  FN(struct_stack_args);                                                       \
  FN(with_tail_call);                                                          \
  FN(with_noreturn_tail_call);                                                 \
  FN(local_array);                                                             \
  FN(local_alloca);                                                            \
  FN(escaping_alloca);                                                         \
  /**/

#include "common.h"
