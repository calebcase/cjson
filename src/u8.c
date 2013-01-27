/*** UTF-8 IO ***/

/* Write the unicode code point to a UTF-8 encoded stream. */
void
cjson_u8_fputu(const uint32_t u, FILE *stream)
{
  uint8_t bytes[] = {0, 0, 0, 0};

  if ((u >= 0xD800 && u <= 0xDFFF) || /* Reserved for UTF-16 parsing. */
      (u >= 0xFFFE && u <= 0xFFFF)) {
    uint64_t u32 = u;
    cjsonx_parse_u(stream, u32, "Invalid unicode character.");
  }

  if (u <= 0x7F) {
    bytes[0] = u;
    ecx_fwrite(bytes, 1, 1, stream);
  }
  else if (u <= 0x7FF) {
    bytes[0] = 0xC0 | ((u >> 6) & 0x1F);
    bytes[1] = 0x80 | (u & 0x3F);
    ecx_fwrite(bytes, 1, 2, stream);
  }
  else if (u <= 0xFFFF) {
    bytes[0] = 0xE0 | ((u >> 12) & 0xF);
    bytes[1] = 0x80 | ((u >> 6) & 0x3F);
    bytes[2] = 0x80 | (u & 0x3F);
    ecx_fwrite(bytes, 1, 3, stream);
  }
  else if (u <= 0x10FFFF) {
    bytes[0] = 0xF0 | ((u >> 18) & 0x7);
    bytes[1] = 0x80 | ((u >> 12) & 0x3F);
    bytes[2] = 0x80 | ((u >> 6) & 0x3F);
    bytes[3] = 0x80 | (u & 0x3F);
    ecx_fwrite(bytes, 1, 4, stream);
  }
  else {
    uint64_t u32 = u;
    cjsonx_parse_u(stream, u32, "Invalid unicode character.");
  }
}

static
int
u8_overlong(uint8_t bytes[4]) {
  switch (bytes[0]) {
    case 0xC0:
    case 0xC1:
      return 1;
      break;
    case 0xE0:
    case 0xF0:
    case 0xF8:
    case 0xFC:
      if ((bytes[0] & bytes[1]) == 0x80) {
        return 1;
      }
      break;
  }

  return 0;
}

/* Read the unicode code point from a UTF-8 encoded stream. */
int64_t
cjson_u8_fgetu(FILE *stream)
{
  char *message = NULL;

  uint32_t u = 0;
  uint8_t bytes[4] = {0, 0, 0, 0};

  static void *go_u8[] = {
    [0   ... 255] = &&l_invalid,

    [0   ... 127] = &&l_utf8_1,
    [194 ... 223] = &&l_utf8_2,
    [224 ... 239] = &&l_utf8_3,
    [240 ... 244] = &&l_utf8_4,
  };

  void **go = go_u8;

  int current = ecx_fgetc(stream);
  if (current != EOF) {
    goto *go[current];
  }
  else {
    return EOF;
  }

l_invalid:
  cjsonx_parse_c(stream, current, "Expecting a UTF-8 character.");

l_utf8_1:
  return current;

l_utf8_2:
  bytes[0] = current;
  bytes[1] = ecx_fgetc(stream);

  u  = (bytes[0] & 0x1F) << 6;
  u |= (bytes[1] & 0x3F);

  if (u8_overlong(bytes)) {
    int64_t u32 = u;
    cjsonx_parse_u(stream, u32, "Overlong 2-byte UTF-8 encoding.");
  }

  return u;

l_utf8_3:
  bytes[0] = current;
  bytes[1] = ecx_fgetc(stream);
  bytes[2] = ecx_fgetc(stream);

  u  = (bytes[0] & 0x0F) << 12;
  u |= (bytes[1] & 0x3F) << 6;
  u |= (bytes[2] & 0x3F);

  if (u8_overlong(bytes)) {
    int64_t u32 = u;
    cjsonx_parse_u(stream, u32, "Overlong 3-byte UTF-8 encoding.");
  }

  return u;

l_utf8_4:
  bytes[0] = current;
  bytes[1] = ecx_fgetc(stream);
  bytes[2] = ecx_fgetc(stream);
  bytes[3] = ecx_fgetc(stream);

  u  = (bytes[0] & 0x07) << 18;
  u |= (bytes[1] & 0x3F) << 12;
  u |= (bytes[2] & 0x3F) << 6;
  u |= (bytes[3] & 0x3F);

  if (u8_overlong(bytes)) {
    int64_t u32 = u;
    cjsonx_parse_u(stream, u32, "Overlong 4-byte UTF-8 encoding.");
  }

  return u;
}
