#include <stdio.h>

int g_trace_indent = 0, g_trace_i;
#define TRACE_IN g_trace_i = ++g_trace_indent; while (--g_trace_i > 0) printf("    "); printf("%s\n", __FUNCTION__);
#define TRACE_OUT --g_trace_indent
#include "legfns.c"

int main()
{
  YYS test_seq = {.type = VT_SEQ,
                  .seq = &(LLNode){.value = &(YYS){.type = VT_STR,
                                                   .str = "hello",
                                                   .len = 5},
                                   .next = &(LLNode){.value = &(YYS){.type = VT_STR,
                                                                     .str = " ",
                                                                     .len = 1},
                                                     .next = &(LLNode){.value = &(YYS){.type = VT_STR,
                                                                                       .str = "world",
                                                                                       .len = 5},
                                                                       .next = NULL}}},
                  .len = 5+1+5};
  YYS test_short_seq = {.type = VT_SEQ,
                        .seq = &(LLNode){.value = &(YYS){.type = VT_STR,
                                                         .str = "test",
                                                         .len = 4},
                                         .next = NULL},
                        .len = 4};
  YYS test_str = {.type = VT_STR,
                  .str = "test",
                  .len = 4};

  printf("join: %s\n", _join(&test_seq)->str);
  printf("node: %s\n", _join(_node(&test_str, &test_seq))->str);
  printf("node_nonempty long: %s\n", _join(_node_nonempty(&test_str, &test_seq))->str);
  printf("node_nonempty short: %s\n", _join(_node_nonempty(&test_str, &test_short_seq))->str);
  printf("node2: %s\n", _join(_node2(&test_str, &test_seq, &test_seq))->str);

  return 0;
}

// gcc -g legtest.c && ./a.out | egrep -v '^ *_' && ./a.out | grep '^ *_' | tr -d ' ' | sort | uniq -c

