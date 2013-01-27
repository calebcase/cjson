/*** UTF-16 Escape IO ***/

/* Write the unicode code point to a UTF-8 encoded stream as a UTF-16 escape
 * sequence.
 */
void
cjson_u16e_fputu(const uint32_t u, FILE *stream)
{
  if ((u >= 0xD800 && u <= 0xDFFF) || /* Reserved for UTF-16 parsing. */
      (u >= 0xFFFE && u <= 0xFFFF)) {
    uint64_t u32 = u;
    cjsonx_parse_u(stream, u32, "Invalid unicode character.");
  }

  if (u <= 0xD7FF ||
      (u >= 0xE000 && u <= 0xFFFF)) {
    ecx_fprintf(stream, "\\u%04" PRIx16, u);
  }
  else if (u >= 0x1000 && u <= 0x10FFFF) {
    uint16_t uhex[2] = {0, 0};
    uint32_t utmp = u - 0x10000;
    uhex[0] = (utmp >> 10) + 0xD800;
    uhex[1] = (utmp & 0x03FF) + 0xDC00;
    ecx_fprintf(stream, "\\u%04" PRIx16 "\\u%04" PRIx16, uhex[0], uhex[1]);
  }
  else {
    uint64_t u32 = u;
    cjsonx_parse_u(stream, u32, "Invalid unicode character.");
  }
}

static
uint16_t
fget_uint16(FILE *stream)
{
  char *message = NULL;

  static void *go_u16[] = {
    [0 ... 255] = &&l_invalid,

    ['0' ... '9'] = &&l_nibble,
    ['a' ... 'f'] = &&l_nibble,
    ['A' ... 'F'] = &&l_nibble,
  };

  static const uint8_t lookup[] = {
    [0 ... 255] = '\0',

    ['0'] = 0x0,
    ['1'] = 0x1,
    ['2'] = 0x2,
    ['3'] = 0x3,
    ['4'] = 0x4,
    ['5'] = 0x5,
    ['6'] = 0x6,
    ['7'] = 0x7,
    ['8'] = 0x8,
    ['9'] = 0x9,
    ['a'] = 0xa,
    ['A'] = 0xa,
    ['b'] = 0xb,
    ['B'] = 0xb,
    ['c'] = 0xc,
    ['C'] = 0xc,
    ['d'] = 0xd,
    ['D'] = 0xd,
    ['e'] = 0xe,
    ['E'] = 0xe,
    ['f'] = 0xf,
    ['F'] = 0xf,
  };

  uint8_t bytes[4] = {0, 0, 0, 0};
  uint8_t count = 0;

  void **go = go_u16;

  int current = ecx_fgetc(stream);

  for (; current != EOF; errno = 0, current = ecx_fgetc(stream)) {
    goto *go[current];
l_loop:;
  }

  if (current == EOF) {
    cjsonx_parse_c(stream, current, "Expecting more data (4 bytes); Failed to find UTF-16 escape sequence.");
  }

l_invalid:
  cjsonx_parse_c(stream, current, "Expecting a hex character.");

l_nibble:
  bytes[count] = lookup[current];
  count++;
  if (count > 3) {
    uint16_t val = 0;
    val |= bytes[0] << 12;
    val |= bytes[1] << 8;
    val |= bytes[2] << 4;
    val |= bytes[3];
    return val;
  }
  goto l_loop;
}

/* Read the unicode code point from a UTF-8 encoded stream as a UTF-16 escape
 * sequence. The leading backslash is assumed to be consumed already so a
 * surrogate pair will look like: uD834\uDD1E.
 */
int64_t
cjson_u16e_fgetu(FILE *stream)
{
  char *message = NULL;

  uint16_t uhex[2] = {0, 0};
  uint32_t u = 0;

  int current = ecx_fgetc(stream);
  if (current == EOF) {
    return EOF;
  }

  if (current != 'u') {
    cjsonx_parse_c(stream, current, "Failed to find UTF-16 escape sequence; Expecting 'u'.");
  }

  /* Read the leading character. */
  uhex[0] = fget_uint16(stream);
  if (uhex[0] >= 0xd800 && uhex[0] <= 0xdfff) {
    if (uhex[0] <= 0xdbff) {
      u = (uhex[0] - 0xd800) << 10;

      /* Read the trailing character. */
      current = ecx_fgetc(stream);
      if (current != '\\') {
        cjsonx_parse_c(stream, current, "Failed to find trailing UTF-16 escape sequence; Expecting '\\'.");
      }

      current = ecx_fgetc(stream);
      if (current != 'u') {
        cjsonx_parse_c(stream, current, "Failed to find trailing UTF-16 escape sequence; Expecting 'u'.");
      }

      uhex[1] = fget_uint16(stream);
      if (uhex[1] >= 0xdc00 && uhex[1] <= 0xdfff) {
        u += uhex[1] - 0xdc00;
        u += 0x10000;
      }
      else {
        uint64_t u16 = uhex[1];
        cjsonx_parse_u(stream, u16, "Invalid trailing UTF-16 surrogate.");
      }
    }
    else {
      uint64_t u16 = uhex[0];
      cjsonx_parse_u(stream, u16, "Invalid leading UTF-16 surrogate.");
    }
  }
  else {
    u = uhex[0];
  }

  return u;
}
