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

START_TEST(fscan)
{
#define IN "\"ASCII\\u0000\\uD834\\uDD1E\\udBfF\\udFfd\\u0001\\u0002\""
#define EXP "ASCII\0\xF0\x9D\x84\x9E\xF4\x8F\xBF\xBD\x1\x2"
  char *buf = IN;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "r");
  struct cjson *node = cjson_string_fscan(stream, NULL);

  fail_unless(node != NULL);
  fail_unless(node->value.string.bytes != NULL);
  {
    const char fmt[] = "Failed to scan string from stream. Got: %zu Exp: %zu";
    fail_unless(node->value.string.length == sizeof(EXP) - 1, fmt, node->value.string.length, sizeof(EXP) - 1);
  }
  {
    const char fmt[] = "Failed to scan string from stream. Got: %s Exp: %s";
    fail_unless(memcmp(node->value.string.bytes, EXP, sizeof(EXP) - 1) == 0, fmt, node->value.string.bytes, EXP);
  }

  fclose(stream);
  cjson_free(node);
#undef EXP
#undef IN
}
END_TEST

START_TEST(fprint)
{
#define IN "\"ASCII\\u0000\\uD834\\uDD1E\\udBfF\\udFfd\\u0001\\u0002\""
#define EXP "\"ASCII\\u0000\xF0\x9D\x84\x9E\xF4\x8F\xBF\xBD\\u0001\\u0002\""
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_string_fscan(in_stream, NULL);
  fclose(in_stream);

  cjson_string_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print string to stream. Got: %s Exp: %s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

static
Suite *
suite(void)
{
  Suite *suite = suite_create("string");

  TCase *tcase_fscan = tcase_create("fscan");
  tcase_add_test(tcase_fscan, fscan);
  suite_add_tcase(suite, tcase_fscan);

  TCase *tcase_fprint = tcase_create("fprint");
  tcase_add_test(tcase_fprint, fprint);
  suite_add_tcase(suite, tcase_fprint);

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
