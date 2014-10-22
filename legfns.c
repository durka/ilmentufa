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

YYS* _make_pair(char *s1, YYS *s2)
{
  TRACE_IN;

  assert(s2->type == VT_STR);
  YYS *ret = _make_seq();

  ret->seq->value = _make_str(s1, 0);
  ret->seq->next = malloc(sizeof(LLNode));
  ret->seq->next->value = s2;
  ret->seq->next->next = NULL;
  ret->len = ret->seq->value->len + ret->seq->next->value->len;

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
      _serialize(n->value, 1);
      if (n->next != NULL) strncat(out, ", ", 2);
    }
    strncat(out, "]", 1);
  }

  return out;
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

YYS* _zoi_assign_delim(YYS *word, char **delim)
{
  TRACE_IN;
  assert(word->type == VT_STR);

  char *comma = strrchr(word->str, ',');
  *delim = (comma == NULL) ? word->str : (comma+1);

  TRACE_OUT;
  return word;
}

bool _zoi_check_quote(char *quote, char *delim)
{
  TRACE_IN;

  int quote_len = strlen(quote);
  char *quote_no_commas = malloc(quote_len+1);
  memset(quote_no_commas, 0, quote_len+1);

  for (int i = 0, j = 0; i < quote_len; ++i)
  {
    if (quote[i] != ',') quote_no_commas[j++] = quote[i];
  }

  TRACE_OUT;
  return strcmp(quote_no_commas, delim) == 0;
}

bool _zoi_check_delim(char *word, char *delim)
{
  TRACE_IN;

  char *comma = strrchr(word, ',');
  char *new_delim = (comma == NULL) ? word : (comma+1);

  TRACE_OUT;
  return strcmp(new_delim, delim) == 0;
}

