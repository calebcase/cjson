/*** JSON Encoded String IO ***/

/* Write the unicode code point to a JSON encoded string stream. */
void
cjson_jestr_fputu(const uint32_t u, FILE *stream)
{
  static const char simple_escape[] = {
    [0 ... 127] = '\0',

    ['"']  = '"',
    ['\\'] = '\\',
    ['\b'] = 'b',
    ['\f'] = 'f',
    ['\n'] = 'n',
    ['\r'] = 'r',
    ['\t'] = 't',
  };

  if (u <= 0x7F) {
    if (simple_escape[u] != '\0') {
      ecx_fputc('\\', stream);
      ecx_fputc(simple_escape[u], stream);
      return;
    }
    else if (u <= 0x1F) {
      cjson_u16e_fputu(u, stream);
      return;
    }
  }

  cjson_u8_fputu(u, stream);
}

/* Read the unicode code point from a JSON encoded string stream. */
int64_t
cjson_jestr_fgetu(FILE *stream)
{
  char *message = NULL;

  static void *go_jestr[] = {
    [0 ... 255] = &&l_invalid,

    [32  ... 127] = &&l_char,

    [194 ... 244] = &&l_utf8,

    ['\\'] = &&l_escape,
  };

  static void *go_jestr_escape[] = {
    [0 ... 255] = &&l_invalid,

    ['"']  = &&l_escape_q,
    ['\\'] = &&l_escape_rs,
    ['/']  = &&l_escape_s,
    ['b']  = &&l_escape_b,
    ['f']  = &&l_escape_f,
    ['n']  = &&l_escape_n,
    ['r']  = &&l_escape_r,
    ['t']  = &&l_escape_t,
    ['u']  = &&l_u16e,
  };

  void **go = go_jestr;

  int current = ecx_fgetc(stream);
  for (; current != EOF; errno = 0, current = ecx_fgetc(stream)) {
    goto *go[current];
l_loop:;
  }

  if (current == EOF) {
    return EOF;
  }

l_invalid:
  {
    long location = ftell(stream);
    ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %x: %s", location, current, message);
    return 0;
  }

l_char:
  return current;

l_utf8:
  ecx_ungetc(current, stream);
  return cjson_u8_fgetu(stream);

l_escape:
  go = go_jestr_escape;
  goto l_loop;

l_escape_q:
  return '"';

l_escape_rs:
  return '\\';

l_escape_s:
  return '/';
  
l_escape_b:
  return '\b';

l_escape_f:
  return '\f';

l_escape_n:
  return '\n';

l_escape_r:
  return '\r';

l_escape_t:
  return '\t';

l_u16e:
  ecx_ungetc(current, stream);
  {
    int64_t val = cjson_u16e_fgetu(stream);
    if (val == EOF) {
      message = "Unexpected EOF.";
      goto l_invalid;
    }
    return val;
  }
}

char *
cjson_jestr_fscan(FILE *stream)
{
  char * jestr = NULL;
  ec_with_on_x(jestr, free) {
    FILE * out = ecx_ccstreams_fstropen(&jestr, "w+");
    ec_with(out, (ec_unwind_f)ecx_fclose) {
      int64_t current = cjson_jestr_fgetu(stream);
      if (current == EOF) {
        long location = ftell(stream);
        ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %" PRIx64 ": Expecting more data.", location, current);
        return NULL;
      }
      else if (current != '"') {
        long location = ftell(stream);
        ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %" PRIx64 ": Expecting '\"'.", location, current);
        return NULL;
      }

      int peek = ecx_getc(stream);
      ecx_ungetc(peek, stream);

      current = cjson_jestr_fgetu(stream);

      for (; current != EOF; peek = ecx_getc(stream), ecx_ungetc(peek, stream), current = cjson_jestr_fgetu(stream)) {
        if (current == EOF) {
          ec_throw_strf(CJSONX_PARSE, "Invalid character at %ld: %" PRIx64 ": Expecting more data.", ftell(stream), current);
        }
        if (current == '"' && peek != '\\') {
          break;
        }
        cjson_jestr_fputu(current, out);
      }
    }
  }

  return jestr;
}

void
cjson_jestr_fprint(FILE *stream, char *jestr)
{
  FILE *in = ecx_ccstreams_fstropen(&jestr, "r");
  ec_with(in, (ec_unwind_f)ecx_fclose) {
    int64_t current = cjson_jestr_fgetu(in);

    ecx_fprintf(stream, "\"");
    for (; current != EOF; current = cjson_jestr_fgetu(in)) {
      cjson_jestr_fputu(current, stream);
    }
    ecx_fprintf(stream, "\"");
  }
}
