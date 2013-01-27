/*** cjson array ***/

struct cjson *
cjson_object_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_OBJECT, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    int current = 0;
    struct cjson *pair = NULL;
    int continued = 0;

    static void *go_object[] = {
      [0 ... 255] = &&l_invalid,

      ['\t'] = &&l_whitespace,
      [' ']  = &&l_whitespace,
      ['\r'] = &&l_whitespace,
      ['\n'] = &&l_whitespace,

      ['"'] = &&l_pair,

      [','] = &&l_object_continue,
      ['}'] = &&l_object_finish,
    };

    void **go = go_object;

    current = ecx_fgetc(stream);
    if (current != '{') {
      cjsonx_parse_c(stream, current, "Unable to find object to parse; Expecting '{'.");
    }

    for (current = ecx_fgetc(stream); current != EOF; errno = 0, current = ecx_fgetc(stream)) {
      goto *go[current];
l_loop:;
    }

    cjsonx_parse_c(stream, current, "Expecting more data; Incomplete object.");

l_invalid:
    cjsonx_parse_c(stream, current, "Expecting to find a JSON key/value pair to parse.");

l_whitespace:
    goto l_loop;

l_pair:
    ecx_ungetc(current, stream);
    pair = cjson_pair_fscan(stream, node);
    ec_with_on_x(pair, (ec_unwind_f)cjson_free) {
      struct cjson *old = cjson_object_set(node, pair);
      if (old != NULL) {
        ec_throw_strf(CJSONX_PARSE, "Invalid duplicate key at %ld: \"%s\".", ftell(stream), old->value.pair.key);
      }
    }
    goto l_loop;

l_object_continue:
    if (pair == NULL) {
      goto l_invalid;
    }
    else {
      pair = NULL;
    }
    continued = 1;
    goto l_loop;

l_object_finish:
    if (continued && pair == NULL) {
      goto l_invalid;
    }

    if (node->hook &&
        node->hook->valid) {
      node->hook->valid(node);
    }
  }

  return node;
}

void
cjson_object_fprint(FILE *stream, struct cjson *node)
{
  cjsonx_type(node, CJSON_OBJECT);

  struct cjson **value = NULL;
  struct cjson **next = NULL;
  size_t count = depth(node);

  uint8_t *key = ecx_malloc(node->value.object.key_length + 1);
  ec_with(key, free) {
    key[0] = '\0';

    ecx_fprintf(stream, "{");

    JSLF(value, node->value.object.data, key);

    if (value != NULL) {
      ecx_fprintf(stream, "\n");
    }

    while (value != NULL) {
      indent(stream, count + 1);
      cjson_fprint(stream, *value);

      JSLN(next, node->value.object.data, key);
      if (next != NULL) {
        ecx_fprintf(stream, ",");
        ecx_fprintf(stream, "\n");
        value = next;
      }
      else {
        ecx_fprintf(stream, "\n");
        indent(stream, count);
        value = NULL;
      }
    }

    ecx_fprintf(stream, "}");
  }
}

static
int
object_count_pairs(size_t *count, struct cjson *pair)
{
  (*count)++;
  return 0;
}

size_t
cjson_object_count(struct cjson *self)
{
  size_t count = 0;
  cjson_object_for_each(self, (cjson_object_call_f)object_count_pairs, &count);
  return count;
}

struct cjson *
cjson_object_get(struct cjson *self, char *key)
{
  cjsonx_type(self, CJSON_OBJECT);

  struct cjson **value = NULL;
  JSLG(value, self->value.object.data, key);
  if (value == NULL) {
    return NULL;
  }
  else {
    return *value;
  }
}

static
int
object_longest_key(size_t *longest, struct cjson *pair)
{
  size_t length = strlen(pair->value.pair.key);
  if (length > *longest) {
    *longest = length;
  }

  return 0;
}

struct object_unset {
  struct cjson *self;
  struct cjson *pair;
  struct cjson *previous;
  struct cjson *parent;
  size_t length;
};

static
void
object_unset(struct object_unset *u)
{
  struct cjson **value = NULL;
  if (u->previous != NULL) {
    JSLI(value, u->self->value.object.data, u->pair->value.pair.key);
    *value = u->previous;
    u->previous->parent = u->self;
  }
  else {
    int status = 0;
    JSLD(status, u->self->value.object.data, u->pair->value.pair.key);
  }
  u->pair->parent = u->parent;
  u->self->value.object.key_length = u->length;
}

struct cjson *
cjson_object_set(struct cjson *self, struct cjson *pair)
{
  cjsonx_type(self, CJSON_OBJECT);
  cjsonx_type(pair, CJSON_PAIR);

  struct cjson **value = NULL;
  struct cjson *previous = cjson_object_get(self, pair->value.pair.key);

  struct object_unset u = {
    .self = self,
    .pair = pair,
    .previous = previous,
    .parent = pair->parent,
    .length = self->value.object.key_length,
  };
  ec_with_on_x(&u, (ec_unwind_f)object_unset) {
    JSLI(value, self->value.object.data, pair->value.pair.key);
    *value = pair;
    pair->parent = self;
    if (previous != NULL) {
      previous->parent = NULL;
    }

    size_t length = strlen(pair->value.pair.key);
    if (length >= self->value.object.key_length) {
      self->value.object.key_length = length;
    }
    else {
      if (previous != NULL) {
        size_t previous_length = strlen(previous->value.pair.key);
        if (previous_length == self->value.object.key_length) {
          size_t longest = 0;
          cjson_object_for_each(self, (cjson_object_call_f)object_longest_key, &longest);
          self->value.object.key_length = longest;
        }
      }
    }

    if (self->hook &&
        self->hook->valid) {
      self->hook->valid(self);
    }
  }

  return previous;
}

struct object_unremove {
  struct cjson *self;
  struct cjson *previous;
  size_t length;
};

static
void
object_unremove(struct object_unremove *u)
{
  struct cjson **value = NULL;
  JSLG(value, u->self->value.object.data, u->previous->value.pair.key);
  if (value == NULL) {
    JSLI(value, u->self->value.object.data, u->previous->value.pair.key);
    *value = u->previous;
    u->previous->parent = u->self;
    u->self->value.object.key_length = u->length;
  }
}

struct cjson *
cjson_object_remove(struct cjson *self, struct cjson *pair)
{
  cjsonx_type(self, CJSON_OBJECT);
  cjsonx_type(pair, CJSON_PAIR);

  struct cjson **value = NULL;
  JSLG(value, self->value.object.data, pair->value.pair.key);
  if (value == NULL) {
    ec_throw_strf(CJSONX_NOT_FOUND, "Key provided was not found in object: \"%s\".", pair->value.pair.key);
  }

  struct object_unremove u = {
    .self = self,
    .previous = *value,
    .length = self->value.object.key_length,
  };
  ec_with_on_x(&u, (ec_unwind_f)object_unremove) {
    int status = 0;
    JSLD(status, self->value.object.data, pair->value.pair.key);

    pair->parent = NULL;

    size_t length = strlen(pair->value.pair.key);
    if (length == self->value.object.key_length) {
      size_t longest = 0;
      cjson_object_for_each(self, (cjson_object_call_f)object_longest_key, &longest);
      self->value.object.key_length = longest;
    }

    if (self->hook &&
        self->hook->valid) {
      self->hook->valid(self);
    }
  }

  return pair;
}

int
cjson_object_for_each(struct cjson *self, int (*call)(void *data, struct cjson *pair), void *data) {
  cjsonx_type(self, CJSON_OBJECT);

  int status = 0;
  char *key = ecx_malloc(self->value.object.key_length + 1);
  key[0] = '\0';
  ec_with(key, free) {
    struct cjson **value = NULL;
    JSLF(value, self->value.object.data, key);

    while (value != NULL) {
      status = call(data, *value);
      if (status != 0) {
        break;
      }

      JSLN(value, self->value.object.data, key);
    }
  }

  return status;
}
