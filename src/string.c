/*** cjson string ***/

struct cjson *
cjson_string_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_STRING, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    FILE *out = ecx_ccstreams_fmemopen(&node->value.string.bytes, &node->value.string.length, "w+");
    ec_with(out, (ec_unwind_f)ecx_fclose) {
      int64_t current = cjson_jestr_fgetu(stream);
      if (current == EOF) {
        cjsonx_parse_u(stream, current, "Expecting more data; Failed to find string to parse.");
      }
      else if (current != '"') {
        cjsonx_parse_u(stream, current, "Failed to find string to parse; Expecting '\"'.");
      }

      int peek = ecx_getc(stream);
      ecx_ungetc(peek, stream);
      current = cjson_jestr_fgetu(stream);

      for (;; peek = ecx_getc(stream), ecx_ungetc(peek, stream), current = cjson_jestr_fgetu(stream)) {
        if (current == EOF) {
          cjsonx_parse_u(stream, current, "Expecting more data; Failed to find end of string.");
        }
        else if (current == '"' && peek != '\\') {
          break;
        }
        cjson_u8_fputu(current, out);
      }
    }

    if (node->hook &&
        node->hook->valid) {
      node->hook->valid(node);
    }
  }

  return node;
}

void
cjson_string_fprint(FILE *stream, struct cjson *node)
{
  cjsonx_type(node, CJSON_STRING);

  FILE *in = ecx_ccstreams_fmemopen(&node->value.string.bytes, &node->value.string.length, "r");
  ec_with(in, (ec_unwind_f)ecx_fclose) {
    ecx_fprintf(stream, "\"");

    for (int64_t current = cjson_u8_fgetu(in); current != EOF; current = cjson_u8_fgetu(in)) {
      cjson_jestr_fputu(current, stream);
    }
    ecx_fprintf(stream, "\"");
  }
}
