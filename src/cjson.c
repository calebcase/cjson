#include <Judy.h>
#include <ccstreams/ecx_ccstreams.h>
#include <ec/ec.h>
#include <ecx_stdio.h>
#include <ecx_stdlib.h>
#include <errno.h>
#include <regex.h>
#include <string.h>
#include <type.h>

#include <cjson.h>

/*** cjson exceptions ***/

const char CJSONX_PARSE[] = "cjson:parse";
const char CJSONX_TYPE[] = "cjson:type";
const char CJSONX_INDEX[] = "cjson:index";
const char CJSONX_NOT_FOUND[] = "cjson:not found";

#define xstr(s) str(s)
#define str(s) #s

#define cjsonx_type(n,t) \
  if ((n)->type != (t)) { \
    ec_throw_strf(CJSONX_TYPE, "Invalid node type: 0x%2x. Requires " str(t) " 0x%2x.", (n)->type, t); \
  } \

#define cjsonx_type2(n,t1,t2) \
  if ((n)->type != (t1) && (n)->type != (t2)) { \
    ec_throw_strf(CJSONX_TYPE, "Invalid node type: 0x%2x. Requires " str(t1) " 0x%2x or " str(t2) " 0x%2x.", (n)->type, t1, t2); \
  } \

#define cjsonx_parse_c(s,c,m,...) \
  ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %x '%c': " m, ftell(s), (c), (c), ##__VA_ARGS__); \

#define cjsonx_parse_u(s,u,m,...) \
  ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %" PRIx64 ": " m, ftell(s), (u), ##__VA_ARGS__); \

/*** cjson creation ***/

void
cjson_init(struct cjson *node, enum cjson_type type, struct cjson *parent)
{
  node->type = type;
  node->parent = parent;

  if (parent != NULL && parent->hook != NULL) {
    node->hook = parent->hook;
  }
  else {
    node->hook = NULL;
  }

  switch (type) {
    case CJSON_ARRAY:
      node->value.array.data = NULL;
      break;
    case CJSON_BOOLEAN:
      node->value.boolean = 0;
      break;
    case CJSON_NULL:
      break;
    case CJSON_NUMBER:
      node->value.number = NULL;
      break;
    case CJSON_OBJECT:
      node->value.object.key_length = 0;
      node->value.object.data = NULL;
      break;
    case CJSON_PAIR:
      node->value.pair.key = NULL;
      node->value.pair.value = NULL;
      break;
    case CJSON_ROOT:
      node->value.root.data = NULL;
      break;
    case CJSON_STRING:
      node->value.string.length = 0;
      node->value.string.bytes = NULL;
      break;
  }
}

static
struct cjson *
cjson_malloc(enum cjson_type type, struct cjson *parent)
{
  struct cjson *node = NULL;

  if (parent != NULL &&
      parent->hook != NULL &&
      parent->hook->cjson_malloc != NULL) {
      node = parent->hook->cjson_malloc(type, parent);
  }
  else {
    node = ecx_malloc(sizeof(*node));
  }

  cjson_init(node, type, parent);

  return node;
}

void
cjson_free(struct cjson *node)
{
  if (node == NULL) {
    return;
  }

  switch (node->type) {
    case CJSON_ARRAY:
      {
        int status = 0;
        Word_t index = 0;
        struct cjson **value = NULL;

        JLF(value, node->value.array.data, index);
        while (value != NULL) {
          cjson_free(*value);
          JLN(value, node->value.array.data, index);
        }

        JLFA(status, node->value.array.data);
      }
      break;
    case CJSON_BOOLEAN:
      break;
    case CJSON_NULL:
      break;
    case CJSON_NUMBER:
      free(node->value.number);
      break;
    case CJSON_OBJECT:
      {
        int status = 0;
        uint8_t *key = ecx_malloc(node->value.object.key_length + 1);
        ec_with(key, free) {
          key[0] = '\0';
          struct cjson **value = NULL;

          JSLF(value, node->value.object.data, key);
          while (value != NULL) {
            cjson_free(*value);
            JSLN(value, node->value.object.data, key);
          }
          JSLFA(status, node->value.object.data);
        }
      }
      break;
    case CJSON_PAIR:
      free(node->value.pair.key);
      cjson_free(node->value.pair.value);
      break;
    case CJSON_ROOT:
      {
        int status = 0;
        Word_t index = 0;
        struct cjson **value = NULL;

        JLF(value, node->value.root.data, index);
        while (value != NULL) {
          cjson_free(*value);
          JLN(value, node->value.root.data, index);
        }

        JLFA(status, node->value.root.data);
      }
      break;
    case CJSON_STRING:
      free(node->value.string.bytes);
      break;
  }

  if (node->hook != NULL &&
      node->hook->cjson_free != NULL) {
      node->hook->cjson_free(node);
  }
  else {
    free(node);
  }
}

/*** Utilities ***/

static
size_t
depth(const struct cjson *node)
{
  size_t count = 0;
  for (; node->parent != NULL && node->parent->type != CJSON_ROOT; node = node->parent) {
    if (node->type != CJSON_PAIR) {
      count++;
    }
  }
  return count;
}

static
void
indent(FILE *stream, size_t count)
{
  for (size_t i = 0; i < count; i++) {
    ecx_fprintf(stream, "  ");
  }
}

/*** cjson generic data handlers. ***/

void
cjson_fprint(FILE *stream, struct cjson *node)
{
  switch(node->type) {
    case CJSON_ARRAY:
      cjson_array_fprint(stream, node);
      break;
    case CJSON_BOOLEAN:
      cjson_boolean_fprint(stream, node);
      break;
    case CJSON_NULL:
      cjson_null_fprint(stream, node);
      break;
    case CJSON_NUMBER:
      cjson_number_fprint(stream, node);
      break;
    case CJSON_OBJECT:
      cjson_object_fprint(stream, node);
      break;
    case CJSON_PAIR:
      cjson_pair_fprint(stream, node);
      break;
    case CJSON_ROOT:
      cjson_root_fprint(stream, node);
      break;
    case CJSON_STRING:
      cjson_string_fprint(stream, node);
      break;
  }
}

struct cjson *
cjson_get(struct cjson *node, const char *segments)
{
  struct cjson *found = node;
  const char *segment = segments;
  size_t length = strlen(segment);
  while (length != 0 && found != NULL) {
    char *normalized = cjson_jestr_normalize(segment);
    ec_with(normalized, free) {
      switch (found->type) {
        case CJSON_ARRAY:
        case CJSON_ROOT:
          {
            size_t index = 0;
            ecx_sscanf(normalized, "%zu", &index);
            found = cjson_array_get(found, index);
          }
          break;
        case CJSON_OBJECT:
          found = cjson_object_get(found, normalized);
          if (found == NULL) {
            break;
          }
          found = found->value.pair.value;
          break;
        default:
          cjsonx_type2(found, CJSON_ARRAY, CJSON_OBJECT);
      }

      segment = segment + length + 1;
      length = strlen(segment);
    }
  }

  return found;
}

void
cjson_segments_fprint(FILE *stream, struct cjson *node, struct cjson *child)
{
  if (node == NULL) {
    return;
  }

  cjson_segments_fprint(stream, node->parent, node);

  if (child != NULL) {
    switch (node->type) {
      case CJSON_ARRAY:
      case CJSON_ROOT:
        {
          struct cjson *found = NULL;
          size_t length = cjson_array_length(node);
          for (size_t i = 0; i < length; i++) {
            found = cjson_array_get(node, i);
            if (found == child) {
              ecx_fprintf(stream, "%zu", i);
              ecx_fputc('\0', stream);
              break;
            }
          }
          if (found == NULL) {
            ec_throw_strf(CJSONX_NOT_FOUND, "Child wasn't found in the array: %p", child);
          }
        }
        break;
      case CJSON_PAIR:
        ecx_fputs(node->value.pair.key, stream);
        ecx_fputc('\0', stream);
        break;
    }
  }
  else {
    switch (node->type) {
      case CJSON_PAIR:
        ecx_fputs(node->value.pair.key, stream);
        ecx_fputc('\0', stream);
        break;
    }
    if (node->parent == NULL) {
      ecx_fputc('\0', stream);
    }
    ecx_fputc('\0', stream);
  }
}

struct cjson_object_walk {
  int (*call)(void *data, struct cjson *node);
  void *data;
};

static
int
cjson_object_walk(struct cjson_object_walk *self, struct cjson *pair)
{
  return cjson_walk(pair, self->call, self->data);
}

int
cjson_walk(struct cjson *self, int (*call)(void *data, struct cjson *node), void *data)
{
  int status = 0;

  switch (self->type) {
    case CJSON_ARRAY:
    case CJSON_ROOT:
      {
        size_t length = cjson_array_length(self);
        for (size_t i = 0; i < length; i++) {
          status = cjson_walk(cjson_array_get(self, i), call, data);
          if (status != 0) {
            return status;
          }
        }
        status = call(data, self);
      }
      break;
    case CJSON_BOOLEAN:
      status = call(data, self);
      break;
    case CJSON_NULL:
      status = call(data, self);
      break;
    case CJSON_NUMBER:
      status = call(data, self);
      break;
    case CJSON_OBJECT:
      {
        struct cjson_object_walk cow = {
          .call = call,
          .data = data,
        };
        status = cjson_object_for_each(self, (cjson_object_call_f)cjson_object_walk, &cow);
        if (status != 0) {
          return status;
        }
        status = call(data, self);
      }
      break;
    case CJSON_PAIR:
      status = cjson_walk(self->value.pair.value, call, data);
      if (status != 0) {
        return status;
      }
      status = call(data, self);
      break;
    case CJSON_STRING:
      status = call(data, self);
      break;
  }

  return status;
}

/*** cjson data handlers. ***/

#include "array.c"
#include "boolean.c"
#include "null.c"
#include "number.c"
#include "object.c"
#include "pair.c"
#include "root.c"

#include "u8.c"
#include "u16e.c"
#include "jestr.c"
#include "string.c"

/*** cjson library initialization. ***/

static void __attribute__ ((constructor))
libcjson_init(void)
{
  int status = 0;

  /* Compile number regex. */
  if (number_regex == NULL) {
    status = regcomp(&number_regex_storage, number_pattern, REG_EXTENDED);
    if (status != 0) {
      ec_throw_str_static(ECX_EC, "Failed to compile number regex.");
    }

    number_regex = &number_regex_storage;
  }
}

static void __attribute__ ((destructor))
libcjson_fini(void)
{
  if (number_regex != NULL) {
    regfree(number_regex);
    number_regex = NULL;
  }
}
