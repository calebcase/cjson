/*** cjson root ***/

struct cjson *
cjson_root_fscan(FILE *stream, enum cjson_type valid, unsigned int continuous)
{
  struct cjson *node = cjson_malloc(CJSON_ROOT, NULL);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    int current = 0;
    struct cjson *child = NULL;

    static void *go_root[] = {
      [0 ... 255] = &&l_invalid,

      ['\t'] = &&l_whitespace,
      [' ']  = &&l_whitespace,
      ['\r'] = &&l_whitespace,
      ['\n'] = &&l_whitespace,

      ['['] = &&l_array,

      ['-']       = &&l_number,
      [48 ... 57] = &&l_number,

      ['{'] = &&l_object,

      ['"'] = &&l_string,

      ['t'] = &&l_boolean,
      ['f'] = &&l_boolean,
      ['n'] = &&l_null,
    };

    static void *go_root_next[] = {
      [0 ... 255] = &&l_invalid,

      ['\t'] = &&l_whitespace,
      [' ']  = &&l_whitespace,
      ['\r'] = &&l_root_next,
      ['\n'] = &&l_root_next,
    };

    void **go = go_root;

    current = ecx_fgetc(stream);
    for (; current != EOF; errno = 0, current = ecx_fgetc(stream)) {
      goto *go[current];
l_loop:;
    }

    goto l_root_finish;

l_invalid:
    {
      long location = ftell(stream);
      ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %x '%c'.", location, current, current);
    }

l_whitespace:
    goto l_loop;

l_array:
    ecx_ungetc(current, stream);
    if (valid & CJSON_ARRAY == 0) {
      ec_throw_str_static(CJSONX_PARSE, "Found an array, but it is not a valid type for a bare item.");
    }
    child = cjson_array_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_root_lappend(node, child);
    }
    go = go_root_next;
    goto l_loop;

l_number:
    ecx_ungetc(current, stream);
    if (valid & CJSON_NUMBER == 0) {
      ec_throw_str_static(CJSONX_PARSE, "Found a number, but it is not a valid type for a bare item.");
    }
    child = cjson_number_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_root_lappend(node, child);
    }
    go = go_root_next;
    goto l_loop;

l_object:
    ecx_ungetc(current, stream);
    if (valid & CJSON_OBJECT == 0) {
      ec_throw_str_static(CJSONX_PARSE, "Found an object, but it is not a valid type for a bare item.");
    }
    child = cjson_object_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_root_lappend(node, child);
    }
    go = go_root_next;
    goto l_loop;

l_string:
    ecx_ungetc(current, stream);
    if (valid & CJSON_STRING == 0) {
      ec_throw_str_static(CJSONX_PARSE, "Found a string, but it is not a valid type for a bare item.");
    }
    child = cjson_string_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_root_lappend(node, child);
    }
    go = go_root_next;
    goto l_loop;

l_boolean:
    ecx_ungetc(current, stream);
    if (valid & CJSON_BOOLEAN == 0) {
      ec_throw_str_static(CJSONX_PARSE, "Found a boolean, but it is not a valid type for a bare item.");
    }
    child = cjson_boolean_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_root_lappend(node, child);
    }
    go = go_root_next;
    goto l_loop;

l_null:
    ecx_ungetc(current, stream);
    if (valid & CJSON_NULL == 0) {
      ec_throw_str_static(CJSONX_PARSE, "Found a null, but it is not a valid type for a bare item.");
    }
    child = cjson_null_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_root_lappend(node, child);
    }
    go = go_root_next;
    goto l_loop;

l_root_next:
    if (continuous != 0) {
      go = go_root;
      goto l_loop;
    }
    goto l_root_finish;

l_root_finish:
    go = go_root;
  }

  return node;
}

void
cjson_root_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ROOT.", node->type);
    return;
  }

  Word_t total = 0;
  Word_t index = 0;
  struct cjson **value = NULL;
  size_t count = depth(node);

  JLC(total, node->value.root.data, 0, -1);
  JLF(value, node->value.array.data, index);
  while (value != NULL) {
    cjson_fprint(stream, *value);

    if (index + 1 != total) {
      ecx_fprintf(stream, "\n");
    }

    JLN(value, node->value.array.data, index);
  }
}

void
cjson_root_lappend(struct cjson *self, struct cjson *item)
{
  if (self->type != CJSON_ROOT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ROOT.", self->type);
    return;
  }

  struct cjson **value = NULL;
  Word_t index = 0;

  JLC(index, self->value.root.data, 0, -1);
  JLI(value, self->value.root.data, index);
  *value = item;
}
