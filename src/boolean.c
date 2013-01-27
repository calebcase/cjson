/*** cjson boolean ***/

struct cjson *
cjson_boolean_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_BOOLEAN, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    int current = ecx_fgetc(stream);
    if (current == 't') {
      if ((current = ecx_fgetc(stream)) != 'r') { goto l_invalid; }
      if ((current = ecx_fgetc(stream)) != 'u') { goto l_invalid; }
      if ((current = ecx_fgetc(stream)) != 'e') { goto l_invalid; }
      node->value.boolean = 1;
    }
    else if (current == 'f') {
      if ((current = ecx_fgetc(stream)) != 'a') { goto l_invalid; }
      if ((current = ecx_fgetc(stream)) != 'l') { goto l_invalid; }
      if ((current = ecx_fgetc(stream)) != 's') { goto l_invalid; }
      if ((current = ecx_fgetc(stream)) != 'e') { goto l_invalid; }
      node->value.boolean = 0;
    }
    else if (current == EOF) {
      ec_throw_str_static(CJSONX_PARSE, "Failed to find boolean to parse.");
    }
    else {
l_invalid:
      {
        long location = ftell(stream);
        ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %x.", location, current);
      }
    }
  }

  if (node->hook &&
      node->hook->valid) {
    node->hook->valid(node);
  }

  return node;
}

void
cjson_boolean_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_BOOLEAN) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_BOOLEAN.", node->type);
    return;
  }

  if (node->value.boolean == 0) {
    ecx_fputs("false", stream);
  }
  else {
    ecx_fputs("true", stream);
  }
}
