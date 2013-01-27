/*** cjson boolean ***/

struct cjson *
cjson_boolean_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_BOOLEAN, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    int current = ecx_fgetc(stream);
    if (current == 't') {
      if ((current = ecx_fgetc(stream)) != 'r') { cjsonx_parse_c(stream, current, "Parsing 'true': Expecting 'r'.") };
      if ((current = ecx_fgetc(stream)) != 'u') { cjsonx_parse_c(stream, current, "Parsing 'true': Expecting 'u'.") };
      if ((current = ecx_fgetc(stream)) != 'e') { cjsonx_parse_c(stream, current, "Parsing 'true': Expecting 'e'.") };
      node->value.boolean = 1;
    }
    else if (current == 'f') {
      if ((current = ecx_fgetc(stream)) != 'a') { cjsonx_parse_c(stream, current, "Parsing 'false': Expecting 'a'.") };
      if ((current = ecx_fgetc(stream)) != 'l') { cjsonx_parse_c(stream, current, "Parsing 'false': Expecting 'l'.") };
      if ((current = ecx_fgetc(stream)) != 's') { cjsonx_parse_c(stream, current, "Parsing 'false': Expecting 's'.") };
      if ((current = ecx_fgetc(stream)) != 'e') { cjsonx_parse_c(stream, current, "Parsing 'false': Expecting 'e'.") };
      node->value.boolean = 0;
    }
    else if (current == EOF) {
      ec_throw_str_static(CJSONX_PARSE, "Expecting more data; Failed to find boolean to parse.");
    }
    else {
l_invalid:
      cjsonx_parse_c(stream, current, "Expecting either 't' or 'f' to begin parsing 'true' or 'false'.");
    }

    if (node->hook &&
        node->hook->valid) {
      node->hook->valid(node);
    }
  }

  return node;
}

void
cjson_boolean_fprint(FILE *stream, struct cjson *node)
{
  cjsonx_type(node, CJSON_BOOLEAN);

  if (node->value.boolean == 0) {
    ecx_fputs("false", stream);
  }
  else {
    ecx_fputs("true", stream);
  }
}
