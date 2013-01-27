/*** cjson root ***/

struct cjson *
cjson_root_fscan(FILE *stream, enum cjson_type valid, unsigned int continuous, struct cjson_hook *hook)
{
  struct cjson *node = NULL;
  if (hook != NULL &&
      hook->cjson_malloc != NULL) {
    node = hook->cjson_malloc(CJSON_ROOT, NULL);
  }
  else {
    node = cjson_malloc(CJSON_ROOT, NULL);
  }
  node->hook = hook;
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
    cjsonx_parse_c(stream, current, "Expecting to find a JSON type to parse.");

l_whitespace:
    goto l_loop;

l_array:
    ecx_ungetc(current, stream);
    if (valid & CJSON_ARRAY == 0) {
      ec_throw_str_static(CJSONX_PARSE, "Found an array, but it is not a valid type for a bare item.");
    }
    child = cjson_array_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_append(node, child);
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
      cjson_array_append(node, child);
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
      cjson_array_append(node, child);
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
      cjson_array_append(node, child);
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
      cjson_array_append(node, child);
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
      cjson_array_append(node, child);
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
    if (node->hook &&
        node->hook->valid) {
      node->hook->valid(node);
    }
  }

  return node;
}

void
cjson_root_fprint(FILE *stream, struct cjson *node)
{
  cjsonx_type(node, CJSON_ROOT);

  Word_t total = 0;
  Word_t index = 0;
  struct cjson **value = NULL;
  size_t count = depth(node);

  JLC(total, node->value.root.data, 0, -1);
  JLF(value, node->value.root.data, index);
  while (value != NULL) {
    cjson_fprint(stream, *value);

    if (index + 1 != total) {
      ecx_fprintf(stream, "\n");
    }

    JLN(value, node->value.root.data, index);
  }
}
