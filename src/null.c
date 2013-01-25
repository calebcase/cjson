/*** cjson boolean ***/

struct cjson *
cjson_null_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_NULL, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    int current = ecx_fgetc(stream);
    if (current == 'n') {
      if ((current = ecx_fgetc(stream)) != 'u') { goto l_invalid; }
      if ((current = ecx_fgetc(stream)) != 'l') { goto l_invalid; }
      if ((current = ecx_fgetc(stream)) != 'l') { goto l_invalid; }
    }
    else if (current == EOF) {
      ec_throw_str_static(CJSONX_PARSE, "Failed to find null to parse.");
    }
    else {
l_invalid:
      {
        long location = ftell(stream);
        ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %x.", location, current);
      }
    }
  }

  return node;
}

void
cjson_null_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_NULL) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_NULL.", node->type);
    return;
  }

  ecx_fputs("null", stream);
}
