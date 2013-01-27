/*** cjson boolean ***/

struct cjson *
cjson_null_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_NULL, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    int current = ecx_fgetc(stream);
    if (current == 'n') {
      if ((current = ecx_fgetc(stream)) != 'u') { cjsonx_parse_c(stream, current, "Parsing 'null': Expecting 'u'.") };
      if ((current = ecx_fgetc(stream)) != 'l') { cjsonx_parse_c(stream, current, "Parsing 'null': Expecting 'l'.") };
      if ((current = ecx_fgetc(stream)) != 'l') { cjsonx_parse_c(stream, current, "Parsing 'null': Expecting 'l'.") };
    }
    else if (current == EOF) {
      ec_throw_str_static(CJSONX_PARSE, "Expecting more data; Failed to find null to parse.");
    }
    else {
l_invalid:
      cjsonx_parse_c(stream, current, "Expecting 'n' to begin parsing 'null'.");
    }

    if (node->hook &&
        node->hook->valid) {
      node->hook->valid(node);
    }
  }

  return node;
}

void
cjson_null_fprint(FILE *stream, struct cjson *node)
{
  cjsonx_type(node, CJSON_NULL);

  ecx_fputs("null", stream);
}
