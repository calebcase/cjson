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
        ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %" PRIx64 ": Expecting more data.", ftell(stream), current);
      }
      else if (current != '"') {
        ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %" PRIx64 ": Expecting '\"'.", ftell(stream), current);
      }

      int peek = ecx_getc(stream);
      ecx_ungetc(peek, stream);
      current = cjson_jestr_fgetu(stream);

      for (;; peek = ecx_getc(stream), ecx_ungetc(peek, stream), current = cjson_jestr_fgetu(stream)) {
        if (current == EOF) {
          ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %" PRIx64 ": Expecting more data.", ftell(stream), current);
        }
        else if (current == '"' && peek != '\\') {
          break;
        }
        cjson_u8_fputu(current, out);
      }
    }
  }

  return node;
}

void
cjson_string_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_STRING) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_STRING.", node->type);
    return;
  }

  FILE *in = ecx_ccstreams_fmemopen(&node->value.string.bytes, &node->value.string.length, "r");
  ec_with(in, (ec_unwind_f)ecx_fclose) {
    ecx_fprintf(stream, "\"");

    for (int64_t current = cjson_u8_fgetu(in); current != EOF; current = cjson_u8_fgetu(in)) {
      cjson_jestr_fputu(current, stream);
    }
    ecx_fprintf(stream, "\"");
  }
}
