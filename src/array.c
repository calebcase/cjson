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
      cjson_array_lappend(node, child);
    }
    goto l_loop;

l_number:
    ecx_ungetc(current, stream);
    child = cjson_number_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_lappend(node, child);
    }
    goto l_loop;

l_object:
    ecx_ungetc(current, stream);
    child = cjson_object_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_lappend(node, child);
    }
    goto l_loop;

l_string:
    ecx_ungetc(current, stream);
    child = cjson_string_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_lappend(node, child);
    }
    goto l_loop;

l_boolean:
    ecx_ungetc(current, stream);
    child = cjson_boolean_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_lappend(node, child);
    }
    goto l_loop;

l_null:
    ecx_ungetc(current, stream);
    child = cjson_null_fscan(stream, node);
    ec_with_on_x(child, (ec_unwind_f)cjson_free) {
      cjson_array_lappend(node, child);
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

  return node;
}

void
cjson_array_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_ARRAY) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY.", node->type);
    return;
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

void
cjson_array_lappend(struct cjson *self, struct cjson *item)
{
  if (self->type != CJSON_ARRAY) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_ARRAY.", self->type);
    return;
  }

  struct cjson **value = NULL;
  Word_t index = 0;

  JLC(index, self->value.array.data, 0, -1);
  JLI(value, self->value.array.data, index);
  *value = item;
}
