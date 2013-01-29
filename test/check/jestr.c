/* Copyright 2013 Caleb Case
 *
 * This file is part of the CJSON Library.
 *
 * The CJSON Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * The CJSON Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the CJSON Library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ccstreams/ecx_ccstreams.h>
#include <check.h>
#include <ecx_stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <cjson.h>

START_TEST(ascii_fgetu)
{
#define BUF "Hello"
  char *buf = BUF;
  size_t len = sizeof(BUF) - 1;
  FILE *stream = ecx_ccstreams_fmemopen(&buf, &len, "r");
  uint32_t exp[] = {'H', 'e', 'l', 'l', 'o'};
  size_t i = 0;

  for (int64_t u = cjson_jestr_fgetu(stream); u != EOF; i++, u = cjson_jestr_fgetu(stream)) {
    const char fmt[] = "Failed to read ASCII from stream. Got: U+%" PRIx64 " Exp: U+%" PRIx32;
    fail_unless(u == exp[i], fmt, u, exp[i]);
  }

  fclose(stream);
#undef BUF
}
END_TEST

START_TEST(ascii_fputu)
{
  uint32_t in[] = {'H', 'e', 'l', 'l', 'o'};
  char *buf = NULL;
  size_t len = 0;
  FILE *stream = ecx_ccstreams_fmemopen(&buf, &len, "w+");
#define EXP "Hello"
  char *exp = EXP;

  for (size_t i = 0; i < sizeof(in) / sizeof(in[0]); i++) {
    cjson_jestr_fputu(in[i], stream);
  }

  fclose(stream);

  const char fmt[] = "Failed to write ASCII to stream. Got: %s Exp: %s";
  fail_unless(len == sizeof(EXP) - 1, fmt, buf, exp);
  fail_unless(memcmp(buf, exp, sizeof(EXP) - 1) == 0, fmt, buf, exp);

  free(buf);
#undef EXP
#undef BUF
}
END_TEST

START_TEST(simple_escape_fgetu)
{
#define BUF "\\\\\\\"\\/\\b\\f\\n\\r\\t"
  char *buf = BUF;
  size_t len = sizeof(BUF) - 1;
  FILE *stream = ecx_ccstreams_fmemopen((char **)&buf, &len, "r");
  uint32_t exp[] = {'\\', '"', '/', '\b', '\f', '\n', '\r', '\t'};
  size_t i = 0;

  for (int64_t u = cjson_jestr_fgetu(stream); u != EOF; i++, u = cjson_jestr_fgetu(stream)) {
    const char fmt[] = "Failed to read simple escape from stream. Got: U+%" PRIx64 " Exp: U+%" PRIx32;
    fail_unless(u == exp[i], fmt, u, exp[i]);
  }

  fclose(stream);
#undef BUF
}
END_TEST

START_TEST(simple_escape_fputu)
{
  uint32_t in[] = {'\\', '"', '\b', '\f', '\n', '\r', '\t'};
  char *buf = NULL;
  size_t len = 0;
  FILE *stream = ecx_ccstreams_fmemopen(&buf, &len, "w+");
#define EXP "\\\\\\\"\\b\\f\\n\\r\\t"
  char *exp = EXP;

  for (size_t i = 0; i < sizeof(in) / sizeof(in[0]); i++) {
    cjson_jestr_fputu(in[i], stream);
  }

  fclose(stream);

  const char fmt[] = "Failed to write simple escape to stream. Got: %s Exp: %s";
  fail_unless(len == sizeof(EXP) - 1, fmt, buf, exp);
  fail_unless(memcmp(buf, exp, sizeof(EXP) - 1) == 0, fmt, buf, exp);

  free(buf);
#undef EXP
#undef BUF
}
END_TEST

START_TEST(u16e_escape_fgetu)
{
#define BUF "\\u0000\\uD834\\uDD1E\\udBfF\\udFfd"
  char *buf = BUF;
  size_t len = sizeof(BUF) - 1;
  FILE *stream = ecx_ccstreams_fmemopen(&buf, &len, "r");
  uint32_t exp[] = {'\0', 0x1D11E, 0x10FFFD};
  size_t i = 0;

  for (int64_t u = cjson_jestr_fgetu(stream); u != EOF; i++, u = cjson_jestr_fgetu(stream)) {
    const char fmt[] = "Failed to read unicode escape from stream. Got: U+%" PRIx64 " Exp: U+%" PRIx32;
    fail_unless(u == exp[i], fmt, u, exp[i]);
  }

  fclose(stream);
#undef BUF
}
END_TEST

START_TEST(u16e_escape_fputu)
{
  uint32_t in[] = {'\0', 0x1D11E, 0x10FFFD, 0x1, 0x2};
  char *buf = NULL;
  size_t len = 0;
  FILE *stream = ecx_ccstreams_fmemopen(&buf, &len, "w+");
#define EXP "\\u0000\xF0\x9D\x84\x9E\xF4\x8F\xBF\xBD\\u0001\\u0002"
  char *exp = EXP;

  for (size_t i = 0; i < sizeof(in) / sizeof(in[0]); i++) {
    cjson_jestr_fputu(in[i], stream);
  }

  fclose(stream);

  const char fmt[] = "Failed to write unicode escape to stream. Got: %s Exp: %s";
  fail_unless(len == sizeof(EXP) - 1, fmt, buf, exp);
  fail_unless(memcmp(buf, exp, sizeof(EXP) - 1) == 0, fmt, buf, exp);

  free(buf);
#undef EXP
#undef BUF
}
END_TEST

START_TEST(fscan)
{
#define IN "\"ASCII\\u0000\\uD834\\uDD1E\\udBfF\\udFfd\\u0001\\u0002\""
#define EXP "ASCII\\u0000\xF0\x9D\x84\x9E\xF4\x8F\xBF\xBD\\u0001\\u0002"
  char *buf = IN;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "r");
  char *jestr = cjson_jestr_fscan(stream);

  fail_unless(jestr != NULL);
  const char fmt[] = "Failed to scan jestr from stream. Got: %s Exp: %s";
  fail_unless(strcmp(jestr, EXP) == 0, fmt, jestr, EXP);

  free(jestr);
  fclose(stream);
#undef EXP
#undef IN
}
END_TEST

START_TEST(fprint)
{
#define IN "ASCII\\u0000\\uD834\\uDD1E\\udBfF\\udFfd\\u0001\\u0002"
#define EXP "\"ASCII\\u0000\xF0\x9D\x84\x9E\xF4\x8F\xBF\xBD\\u0001\\u0002\""
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");
  char *jestr = IN;

  cjson_jestr_fprint(stream, jestr);
  fail_unless(jestr != NULL);

  fclose(stream);

  const char fmt[] = "Failed to print jestr to stream. Got: %s Exp: %s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(normalize)
{
  char *normalized = NULL;

  normalized = cjson_jestr_normalize("asdf");
  fail_unless(normalized != NULL);
  fail_unless(strcmp(normalized, "asdf") == 0);
  free(normalized);
}
END_TEST

static
Suite *
suite(void)
{
  Suite *suite = suite_create("jestr");

  TCase *tcase_ascii = tcase_create("ascii");
  tcase_add_test(tcase_ascii, ascii_fgetu);
  tcase_add_test(tcase_ascii, ascii_fputu);
  suite_add_tcase(suite, tcase_ascii);

  TCase *tcase_simple_escape = tcase_create("simple_escape");
  tcase_add_test(tcase_simple_escape, simple_escape_fgetu);
  tcase_add_test(tcase_simple_escape, simple_escape_fputu);
  suite_add_tcase(suite, tcase_simple_escape);

  TCase *tcase_u16e_escape = tcase_create("u16e_escape");
  tcase_add_test(tcase_u16e_escape, u16e_escape_fgetu);
  tcase_add_test(tcase_u16e_escape, u16e_escape_fputu);
  suite_add_tcase(suite, tcase_u16e_escape);

  TCase *tcase_fscan = tcase_create("fscan");
  tcase_add_test(tcase_fscan, fscan);
  suite_add_tcase(suite, tcase_fscan);

  TCase *tcase_fprint = tcase_create("fprint");
  tcase_add_test(tcase_fprint, fprint);
  suite_add_tcase(suite, tcase_fprint);

  TCase *tcase_normalize = tcase_create("normalize");
  tcase_add_test(tcase_normalize, normalize);
  suite_add_tcase(suite, tcase_normalize);

  return suite;
}

int
main(void)
{
  int failed = 0;

  SRunner *srunner = srunner_create(suite());

  srunner_run_all(srunner, CK_NORMAL);
  failed = srunner_ntests_failed(srunner);

  srunner_free(srunner);

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
