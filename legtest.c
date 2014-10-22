#include "cheat.h"

#include <stdio.h>
#include "strmap.h"

#ifndef __LEGFNS_INCLUDED_YET__
#define __LEGFNS_INCLUDED_YET__
StrMap *g_fns;
#define TRACE_IN do { \
                   union { \
                     long val; \
                     char buf[sizeof(long) + 1]; \
                   } s; \
                   memset(&s, 0, sizeof s); \
                   if (sm_get(g_fns, __FUNCTION__, s.buf, sizeof s)) { \
                     s.val = s.val + 1; \
                   } else { \
                     s.val = 1; \
                   } \
                   sm_put(g_fns, __FUNCTION__, s.buf); \
                 } while (0)
#define TRACE_OUT
#include "legfns.c"
#endif // __LEGFNS_INCLUDED_YET__

CHEAT_TEST(setup,
  g_fns = sm_new(100);
)

CHEAT_DECLARE(
  char* _serialize(YYS *arg, bool cont)
  {
    static char out[100];
    if (!cont)
    {
      memset(out, 0, 100);
    }

    if (arg->type == VT_STR)
    {
      strncat(out, "(", 1);
      strncat(out, arg->str, arg->len);
      strncat(out, ")", 1);
    }
    else
    {
      strncat(out, "[", 1);
      for (LLNode *n = arg->seq; n != NULL; n = n->next)
      {
        _serialize(n->value, out);
        if (n->next != NULL) strncat(out, ", ", 2);
      }
      strncat(out, "]", 1);
    }

    return out;
  }

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
)

CHEAT_TEST(join,
  cheat_assert(0 == strcmp(_join(&test_seq)->str, "hello world"));
)
CHEAT_TEST(node,
  cheat_assert(0 == strcmp(_serialize(_node(&test_str, &test_seq), 0), "[(test), [(hello), ( ), (world)]]"));
)
CHEAT_TEST(node_nonempty__long,
  cheat_assert(0 == strcmp(_serialize(_node_nonempty(&test_str, &test_seq), 0), "[(test), [(hello), ( ), (world)]]"));
)
CHEAT_TEST(node_nonempty__short,
  cheat_assert(0 == strcmp(_serialize(_node_nonempty(&test_str, &test_short_seq), 0), "[(test), [(test)]]"));
)
CHEAT_TEST(node2,
  cheat_assert(0 == strcmp(_serialize(_node2(&test_str, &test_seq, &test_seq), 0), "[(test), [(hello), ( ), (world)], [(hello), ( ), (world)]]"));
)

CHEAT_DECLARE(
  void enumfunc(const char *key, const char *value, const void *obj) {
    union {
      long val;
      char buf[sizeof(long) + 1];
    } s;
    sm_get(g_fns, key, s.buf, sizeof s);
    printf("\t%s: %ld\n", key, s.val);
  }
)

CHEAT_TEST(teardown,
  sm_enum(g_fns, enumfunc, NULL);
)

