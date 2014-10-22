#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef TRACE_IN
#   define TRACE_IN
#   define TRACE_OUT
#endif

struct LLNode;
struct YYS;

typedef struct LLNode
{
  struct YYS *value;
  struct LLNode *next;
} LLNode;

typedef struct YYS
{
  enum { VT_STR, VT_SEQ } type;
  union
  {
    char *str;
    LLNode *seq;
  };
  int len;
} YYS;

YYS* _make_str(char *s, int len)
{
  TRACE_IN;

  YYS *ret = malloc(sizeof(YYS));
  ret->type = VT_STR;

  if (s != NULL)
  {
    ret->str = s;
    if (len != 0)
    {
      ret->len = len;
    }
    else
    {
      ret->len = strlen(ret->str);
    }
  }
  else
  {
    ret->str = NULL;
    ret->len = 0;
  }
  
  TRACE_OUT;
  return ret;
}

YYS* _make_seq()
{
  TRACE_IN;

  YYS *ret = malloc(sizeof(YYS));
  ret->type = VT_SEQ;
  ret->seq = malloc(sizeof(LLNode));
  ret->seq->value = NULL;
  ret->seq->next = NULL;

  TRACE_OUT;
  return ret;
}

YYS* _join(YYS *arg)
{
  TRACE_IN;

  YYS *ret = _make_str(NULL, 0);

  if (arg == NULL)
  {
    TRACE_OUT;
    return ret;
  }
  else if (arg->type == VT_STR)
  {
    ret->str = arg->str;
    ret->len = arg->len;
  }
  else
  {
    ret->len = arg->len;
    ret->str = malloc(ret->len+1);
    ret->str[0] = 0;
    for (LLNode *n = arg->seq; n != NULL; n = n->next)
    {
      if (n->value->type == VT_STR)
      {
        strncat(ret->str, n->value->str, n->value->len);
      }
      else
      {
        strncat(ret->str, _join(n->value)->str, n->value->len);
      }
    }
  }

  TRACE_OUT;
  return ret;
}

void _push(YYS *pushee, YYS *pushed)
{
  TRACE_IN;
  assert(pushee->type == VT_SEQ);

  pushee->len += pushed->len;

  LLNode *n = pushee->seq;
  while (n->next != NULL) n = n->next;
  if (n->value == NULL)
  {
    n->value = pushed;
  }
  else
  {
    n->next = malloc(sizeof(LLNode));
    n->next->value = pushed;
    n->next->next = NULL;
  }

  TRACE_OUT;
}

void _concat(YYS *concattee, YYS *concatted)
{
  TRACE_IN;
  assert(concattee->type == VT_SEQ);
  assert(concatted->type == VT_SEQ);

  concattee->len += concatted->len;

  LLNode *n = concattee->seq;
  while (n->next != NULL) n = n->next;
  if (n->value == NULL)
  {
    concattee->seq = concatted->seq;
  }
  else
  {
    n->next = concatted->seq;
  }

  TRACE_OUT;
}

YYS* _node_int(YYS *label, YYS *arg)
{
  TRACE_IN;

  if (arg != NULL && arg->type == VT_STR)
  {
    TRACE_OUT;
    return arg;
  }

  YYS *ret = _make_seq();
  if (label != NULL) _push(ret, label);
  if (arg != NULL)
  {
    for (LLNode *n = arg->seq; n != NULL; n = n->next)
    {
      if (n->value->len != 0)
        _push(ret, _node_int(NULL, n->value));
    }
  }

  TRACE_OUT;
  return ret;
}

YYS* _node(YYS *label, YYS *arg)
{
  TRACE_IN;

  YYS *ret = _make_seq();

  if (label != NULL) _push(ret, label);
  if (arg != NULL)
  {
    if (arg->type == VT_SEQ && arg->seq->value->type == VT_STR && arg->seq->value->len > 0)
    {
      _push(ret, arg);
      TRACE_OUT;
      return ret;
    }
  }
  TRACE_OUT;
  return _node_int(label, arg);
} 

YYS* _node2(YYS *label, YYS *arg1, YYS *arg2)
{
  TRACE_IN;

  YYS *ret = _make_seq();

  _push(ret, label);
  _concat(ret, _node(arg1, NULL));
  _concat(ret, _node(arg2, NULL));

  TRACE_OUT;
  return ret;
}

YYS* _node_nonempty(YYS *label, YYS *arg)
{
  TRACE_IN;
  assert(label->type == VT_STR);

  YYS *yy = _node(label, arg);

  TRACE_OUT;
  return (yy->type == VT_SEQ
       && yy->seq != NULL
       && yy->seq->next == NULL
       && yy->seq->value->type == VT_STR
       && strcmp(yy->seq->value->str, label->str) == 0)
    ? NULL
    : yy;
}

// === ZOI functions === //

char *_g_zoi_delim = "";

YYS* _zoi_assign_delim(YYS *word)
{
  TRACE_IN;
  assert(word->type == VT_STR);

  _g_zoi_delim = strrchr(word->str, ',')+1;

  TRACE_OUT;
  return word;
}

bool _zoi_check_quote(YYS *word)
{
  TRACE_IN;
  assert(word->type == VT_STR);

  char *p;
  while ((p = strchr(word->str, ',')) != NULL)
  {
    memmove(p, p+1, word->len + word->str - p);
    word->len--;
  }

  TRACE_OUT;
  return strcmp(word->str, _g_zoi_delim) == 0;
}

bool _zoi_check_delim(YYS *word)
{
  TRACE_IN;
  assert(word->type == VT_STR);

  char *s = strrchr(word->str, ',')+1;

  TRACE_OUT;
  return strcmp(word->str, _g_zoi_delim) == 0;
}

