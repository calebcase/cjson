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
      long location = ftell(stream);
      ec_throw_strf(CJSONX_PARSE, "Unable to find object to parse: %ld", location);
    }

    for (current = ecx_fgetc(stream); current != EOF; errno = 0, current = ecx_fgetc(stream)) {
      goto *go[current];
l_loop:;
    }

    {
      long location = ftell(stream);
      ec_throw_strf(CJSONX_PARSE, "Incomplete array: %ld", location);
    }

l_invalid:
    {
      long location = ftell(stream);
      ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %x '%c'.", location, current, current);
    }

l_whitespace:
    goto l_loop;

l_pair:
    ecx_ungetc(current, stream);
    pair = cjson_pair_fscan(stream, node);
    ec_with_on_x(pair, (ec_unwind_f)cjson_free) {
      cjson_object_insert(node, pair);
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
  }

  return node;
}

void
cjson_object_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_OBJECT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_OBJECT.", node->type);
    return;
  }

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

void
cjson_object_insert(struct cjson *self, struct cjson *item)
{
  if (self->type != CJSON_OBJECT) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_OBJECT.", self->type);
    return;
  }

  if (item->type != CJSON_PAIR) {
    ec_throw_str_static(CJSONX_PARSE, "Invalid type: Only pairs can be inserted into objects.");
  }

  size_t length = strlen(item->value.pair.key);
  if (length < self->value.object.key_length) {
    length = self->value.object.key_length;
  }

  uint8_t *key = ecx_malloc(length + 1);
  ec_with(key, free) {
    strcpy(key, item->value.pair.key);

    struct cjson **value = NULL;
    JSLI(value, self->value.object.data, key);
    *value = item;
    self->value.object.key_length = length;
  }
}
