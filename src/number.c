/*** cjson number ***/

static const char number_pattern[] = "^([-]?([0-9]|[1-9][0-9]*))([.][0-9]+)?([eE][+-]?[0-9]+)?$";
static regex_t number_regex_storage;
static regex_t *number_regex = NULL;

struct cjson *
cjson_number_fscan(FILE *stream, struct cjson *parent)
{
  struct cjson *node = cjson_malloc(CJSON_NUMBER, parent);
  ec_with_on_x(node, (ec_unwind_f)cjson_free) {
    char *number = NULL;
    ec_with_on_x(number, free) {
      FILE *out = ecx_ccstreams_fstropen(&number, "w+");
      ec_with(out, (ec_unwind_f)ecx_fclose) {
        /* Scan and buffer up the number. */
        int current = ecx_fgetc(stream);
        for (; current != EOF; errno = 0, current = ecx_fgetc(stream)) {
          if (current == ' '  ||
              current == '\n' ||
              current == '\r' ||
              current == '\t' ||
              current == ','  ||
              current == ']'  ||
              current == '}') {
            ecx_ungetc(current, stream);
            break;
          }
          ecx_fputc(current, out);
        }
      }

      if (strnlen(number, 1) == 0) {
        ec_throw_str_static(CJSONX_PARSE, "Failed to find number to parse.");
      }

      /* Verify that the format is valid. */
      int status = regexec(number_regex, number, 0, NULL, 0);
      if (status != 0) {
        ec_throw_strf(CJSONX_PARSE, "Failed to parse number; Format is invalid: '%s'.", number);
      }

      node->value.number = number;
    }
  }

  if (node->hook &&
      node->hook->valid) {
    node->hook->valid(node);
  }

  return node;
}

void
cjson_number_fprint(FILE *stream, struct cjson *node)
{
  if (node->type != CJSON_NUMBER) {
    ec_throw_strf(CJSONX_PARSE, "Invalid node type: 0x%2x. Requires CJSON_NUMBER.", node->type);
    return;
  }

  ecx_fprintf(stream, "%s", node->value.number);
}
