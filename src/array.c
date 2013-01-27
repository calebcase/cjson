/*** cjson array ***/

struct cjson *
cjson_array_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_ARRAY, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    struct cjson *child = NULL;
    int current = 0;
    int continued = 0;

    static void *go_array[] = {
      [0 ... 255] = &&l_invalid,

      ['\t'] = &&l_whitespace,
      [' ']  = &&l_whitespace,
      ['\r'] = &&l_whitespace,
      ['\n'] = &&l_whitespace,

      ['['] = &&l_array,

      ['-']         = &&l_number,
      ['0' ... '9'] = &&l_number,

      ['{'] = &&l_object,

      ['"'] = &&l_string,

      [','] = &&l_array_continue,
      [']'] = &&l_array_finish,

      ['t'] = &&l_boolean,
      ['f'] = &&l_boolean,
      ['n'] = &&l_null,
    };

    void **go = go_array;

    current = ecx_fgetc(stream);
    if (current != '[') {
      long location = ftell(stream);
      ec_throw_strf(CJSONX_PARSE, "Unable to find array to parse: %ld", location);
    }

    for (current = ecx_fgetc(stream); current != EOF; errno = 0, current = ecx_fgetc(stream)) {
      goto *go[current];
l_loop:;
    }

    ec_throw_strf(CJSONX_PARSE, "Incomplete array: %ld", ftell(stream));

l_invalid:
    ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %x '%c'.", ftell(stream), current, current);

l_whitespace:
    goto l_loop;

l_array:
    ecx_ungetc(current, stream);
    child = cjson_array_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_append(node, child);
    }
    goto l_loop;

l_number:
    ecx_ungetc(current, stream);
    child = cjson_number_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_append(node, child);
    }
    goto l_loop;

l_object:
    ecx_ungetc(current, stream);
    child = cjson_object_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_append(node, child);
    }
    goto l_loop;

l_string:
    ecx_ungetc(current, stream);
    child = cjson_string_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_append(node, child);
    }
    goto l_loop;

l_boolean:
    ecx_ungetc(current, stream);
    child = cjson_boolean_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_append(node, child);
    }
    goto l_loop;

l_null:
    ecx_ungetc(current, stream);
    child = cjson_null_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_append(node, child);
    }
    goto l_loop;

l_array_continue:
    if (child == NULL) {
      goto l_invalid;
    }
    else {
      child = NULL;
    }
    continued = 1;
    goto l_loop;

l_array_finish:
    if (continued && child == NULL) {
      goto l_invalid;
    }
  }

  if (node->hook &&
      node->hook->valid) {
    node->hook->valid(node);
  }

  return node;
}

void
cjson_array_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_ARRAY) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY.", node->type);
  }

  Word_t total = 0;
  Word_t index = 0;
  struct cjson **value = NULL;
  size_t count = depth(node);

  ecx_fprintf(stream, "[");

  JLC(total, node->value.array.data, 0, -1);
  if (total > 0) {
    ecx_fprintf(stream, "\n");

    JLF(value, node->value.array.data, index);
    while (value != NULL) {
      indent(stream, count + 1);
      cjson_fprint(stream, (*value));

      if (index + 1 != total) {
        ecx_fprintf(stream, ",");
      }
      ecx_fprintf(stream, "\n");

      JLN(value, node->value.array.data, index);
    }

    indent(stream, count);
  }

  ecx_fprintf(stream, "]");
}

size_t
cjson_array_length(struct cjson *self)
{
  if (self->type != CJSON_ARRAY &&
      self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY or CJSON_ROOT.", self->type);
  }

  size_t total = 0;
  JLC(total, self->value.array.data, 0, -1);
  return total;
}

struct cjson *
cjson_array_get(struct cjson *self, size_t index)
{
  if (self->type != CJSON_ARRAY &&
      self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY or CJSON_ROOT.", self->type);
  }

  struct cjson **value = NULL;
  JLG(value, self->value.array.data, index);
  if (value == NULL) {
    ec_throw_strf(CJSONX_INDEX, "Invalid index (out of bounds): %zu (Array length: %zu).", index, cjson_array_length(self));
  }

  return *value;
}

struct array_unset {
  struct cjson *self;
  size_t index;
  struct cjson *item;
  struct cjson *previous;
  struct cjson *parent;
};

static
void
array_unset(struct array_unset *u)
{
  struct cjson **value = NULL;
  JLI(value, u->self->value.array.data, u->index);
  *value = u->previous;
  u->item->parent = u->item->parent;
  u->previous->parent = u->self;
}

struct cjson *
cjson_array_set(struct cjson *self, size_t index, struct cjson *item)
{
  if (self->type != CJSON_ARRAY &&
      self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY or CJSON_ROOT.", self->type);
  }

  struct cjson **value = NULL;
  struct cjson *previous = cjson_array_get(self, index);

  struct array_unset u = {
    .self = self,
    .index = index,
    .item = item,
    .previous = previous,
    .parent = item->parent,
  };
  ec_with_on_x(&u, (ec_unwind_f)array_unset) {
    JLI(value, self->value.array.data, index);
    if (value == NULL) {
      ec_throw_strf(CJSONX_INDEX, "Invalid index: %zu. Requires 0 <= index < %zu.", index, cjson_array_length(self));
    }
    *value = item;
    item->parent = self;
    previous->parent = NULL;

    if (self->hook &&
        self->hook->valid) {
      self->hook->valid(self);
    }
  }

  return previous;
}

struct array_untruncate {
  struct cjson *self;
  struct cjson *node;
};

static
void
array_untruncate(struct array_untruncate *u)
{
  int status = 0;
  struct cjson **source_value = NULL;
  struct cjson **target_value = NULL;
  size_t source_index = 0;
  size_t target_index = SIZE_MAX;

  JLF(source_value, u->node->value.array.data, source_index);
  JLL(target_value, u->self->value.array.data, target_index);
  if (target_value == NULL) {
    target_index = 0;
  }
  else {
    JLN(target_value, u->self->value.array.data, target_index);
  }
  while (source_value != NULL) {
    JLI(target_value, u->self->value.array.data, target_index);
    *target_value = *source_value;
    JLD(status, u->node->value.array.data, source_index);

    JLN(source_value, u->node->value.array.data, source_index);
    JLN(target_value, u->self->value.array.data, target_index);
  }
}

struct cjson *
cjson_array_truncate(struct cjson *self, size_t length)
{
  if (self->type != CJSON_ARRAY &&
      self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY or CJSON_ROOT.", self->type);
  }

  size_t current_length = cjson_array_length(self);
  if (length >= current_length) {
    ec_throw_strf(CJSONX_INDEX, "Invalid index (out of bounds): %zu (Array length: %zu).", length, current_length);
  }

  struct cjson *node = cjson_malloc(CJSON_ARRAY, self);
  node->parent = NULL;
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    struct array_untruncate u = {
      .self = self,
      .node = node,
    };
    ec_with_on_x(&u, (ec_unwind_f)array_untruncate) {
      int status = 0;
      struct cjson **value = NULL;

      JLF(value, self->value.array.data, length);
      while (value != NULL) {
        cjson_array_append(node, *value);
        JLD(status, self->value.array.data, length);
        JLN(value, self->value.array.data, length);
      }

      if (self->hook &&
          self->hook->valid) {
        self->hook->valid(self);
      }
    }
  }

  return node;
}

struct array_unappend {
  struct cjson *self;
  struct cjson *item;
  struct cjson *parent;
};

static
void
array_unappend(struct array_unappend *u)
{
  int status = 0;
  struct cjson **value = NULL;
  size_t index = 0;

  JLC(index, u->self->value.array.data, 0, -1);
  index--;
  JLD(status, u->self->value.array.data, index);
  u->item->parent = u->parent;
}

void
cjson_array_append(struct cjson *self, struct cjson *item)
{
  if (self->type != CJSON_ARRAY &&
      self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY or CJSON_ROOT.", self->type);
  }

  struct array_unappend u = {
    .self = self,
    .item = item,
    .parent = item->parent,
  };
  ec_with_on_x(&u, (ec_unwind_f)array_unappend) {
    struct cjson **value = NULL;
    size_t index = 0;

    JLC(index, self->value.array.data, 0, -1);
    JLI(value, self->value.array.data, index);
    *value = item;
    item->parent = self;

    if (self->hook &&
        self->hook->valid) {
      self->hook->valid(self);
    }
  }
}

struct array_unextend {
  struct cjson *self;
  struct cjson *item;
  size_t length;
};

static
void
array_unextend(struct array_unextend *u) {
  int status = 0;
  size_t index = u->length;
  struct cjson **value = NULL;

  JLF(value, u->self->value.array.data, index);
  while (value != NULL) {
    struct cjson **revalue = NULL;
    JLI(revalue, u->item->value.array.data, index);
    JLD(status, u->self->value.array.data, index);
    JLN(value, u->self->value.array.data, index);
  }
}

void
cjson_array_extend(struct cjson *self, struct cjson *item)
{
  if (self->type != CJSON_ARRAY &&
      self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY or CJSON_ROOT.", self->type);
  }

  if (item->type != CJSON_ARRAY &&
      self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid item type: 0x%2x. Requires CJSON_ARRAY or CJSON_ROOT.", item->type);
  }

  int status = 0;
  size_t index = 0;
  struct cjson **value = NULL;

  struct array_unextend u = {
    .self = self,
    .item = item,
    .length = cjson_array_length(self),
  };
  ec_with_on_x(&u, (ec_unwind_f)array_unextend) {
    JLF(value, item->value.array.data, index);
    while (value != NULL) {
      cjson_array_append(self, *value);
      JLD(status, item->value.array.data, index);
      JLN(value, item->value.array.data, index);
    }

    if (self->hook &&
        self->hook->valid) {
      self->hook->valid(self);
    }
  }
}
