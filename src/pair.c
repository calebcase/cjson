/*** cjson string ***/

struct cjson *
cjson_pair_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_PAIR, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    int current = 0;
    char *key = NULL;
    size_t length = 0;
    struct cjson *value = NULL;

    static void *go_pair_key[] = {
      [0 ... 255] = &&l_invalid,

      ['\t'] = &&l_whitespace,
      [' ']  = &&l_whitespace,
      ['\r'] = &&l_whitespace,
      ['\n'] = &&l_whitespace,

      [':']  = &&l_value,
    };

    static void *go_pair_value[] = {
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

    void **go = go_pair_key;

    /* Read in the key. */
    key = cjson_jestr_fscan(stream);
    length = strlen(key);

    current = ecx_fgetc(stream);
    for (; current != EOF; errno = 0, current = ecx_fgetc(stream)) {
      goto *go[current];
l_loop:;
    }

    cjsonx_parse_c(stream, current, "Expecting more data; Incomplete pair (missing data for '%s').", key);

l_invalid:
    cjsonx_parse_c(stream, current, "Expecting to find a JSON type to parse for pair data.");

l_whitespace:
    goto l_loop;

l_value:
    go = go_pair_value;
    goto l_loop;

l_array:
    ecx_ungetc(current, stream);
    value = cjson_array_fscan(stream, node);
    goto l_pair_finish;

l_number:
    ecx_ungetc(current, stream);
    value = cjson_number_fscan(stream, node);
    goto l_pair_finish;

l_object:
    ecx_ungetc(current, stream);
    value = cjson_object_fscan(stream, node);
    goto l_pair_finish;

l_string:
    ecx_ungetc(current, stream);
    value = cjson_string_fscan(stream, node);
    goto l_pair_finish;

l_boolean:
    ecx_ungetc(current, stream);
    value = cjson_boolean_fscan(stream, node);
    goto l_pair_finish;

l_null:
    ecx_ungetc(current, stream);
    value = cjson_null_fscan(stream, node);
    goto l_pair_finish;

l_pair_finish:
    node->value.pair.key = key;
    node->value.pair.value = value;

    if (node->hook &&
        node->hook->valid) {
      node->hook->valid(node);
    }
  }

  return node;
}

void
cjson_pair_fprint(FILE *stream, struct cjson *node)
{
  cjsonx_type(node, CJSON_PAIR);

  cjson_jestr_fprint(stream, node->value.pair.key);
  ecx_fprintf(stream, ": ");
  cjson_fprint(stream, node->value.pair.value);
}
