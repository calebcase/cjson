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
#define IN "[]"
  char *buf = IN;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "r");
  struct cjson *node = cjson_array_fscan(stream, NULL);

  fail_unless(node != NULL);
  fail_unless(node->value.array.data == NULL);

  fclose(stream);
  cjson_free(node);
#undef IN
}
END_TEST

START_TEST(fprint)
{
#define IN "[]"
#define EXP "[]"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_array_fscan(in_stream, NULL);
  fclose(in_stream);

  cjson_array_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print array to stream. Got: %s Exp: %s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(gambit_leaf)
{
#define IN "[3.14, \"\", true, null]"
#define EXP "[\n" \
            "  3.14,\n" \
            "  \"\",\n" \
            "  true,\n" \
            "  null\n" \
            "]"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_array_fscan(in_stream, NULL);
  fclose(in_stream);

  cjson_array_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print array to stream. Got: %s Exp: %s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(gambit_nested_leaf)
{
#define IN "[[], [0], [[0, \"\"]], [[0, \"\", true, null], [false]]]"
#define EXP "[\n" \
            "  [],\n" \
            "  [\n" \
            "    0\n" \
            "  ],\n" \
            "  [\n" \
            "    [\n" \
            "      0,\n" \
            "      \"\"\n" \
            "    ]\n" \
            "  ],\n" \
            "  [\n" \
            "    [\n" \
            "      0,\n" \
            "      \"\",\n" \
            "      true,\n" \
            "      null\n" \
            "    ],\n" \
            "    [\n" \
            "      false\n" \
            "    ]\n" \
            "  ]\n" \
            "]"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_array_fscan(in_stream, NULL);
  fclose(in_stream);

  cjson_array_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print array to stream. Got: %s Exp: %s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(manipulate)
{
  char *a_str = "[]";
  FILE *a_stream = ecx_ccstreams_fstropen(&a_str, "r");
  struct cjson *a = cjson_array_fscan(a_stream, NULL);
  fclose(a_stream);

  fail_unless(cjson_array_length(a) == 0);

  char *tmp_str = NULL;
  FILE *tmp_stream = NULL;
  struct cjson *tmp = NULL;
  struct cjson *old = NULL;

  tmp_str = "true";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_boolean_fscan(tmp_stream, NULL);
  fclose(tmp_stream);

  cjson_array_append(a, tmp);
  fail_unless(cjson_array_length(a) == 1);
  fail_unless(cjson_array_get(a, 0) == tmp);

  tmp_str = "null";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_null_fscan(tmp_stream, NULL);
  fclose(tmp_stream);

  old = cjson_array_set(a, 0, tmp);
  fail_unless(cjson_array_length(a) == 1);
  fail_unless(cjson_array_get(a, 0) == tmp);
  fail_unless(old->type == CJSON_BOOLEAN);
  cjson_free(old);

  tmp_str = "1";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_number_fscan(tmp_stream, NULL);
  fclose(tmp_stream);

  cjson_array_append(a, tmp);
  fail_unless(cjson_array_length(a) == 2);
  fail_unless(cjson_array_get(a, 1) == tmp);

  tmp_str = "2";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_number_fscan(tmp_stream, NULL);
  fclose(tmp_stream);

  cjson_array_append(a, tmp);
  fail_unless(cjson_array_length(a) == 3);
  fail_unless(cjson_array_get(a, 2) == tmp);

  old = cjson_array_truncate(a, 1);
  fail_unless(cjson_array_length(a) == 1);
  fail_unless(cjson_array_length(old) == 2);
  fail_unless(cjson_array_get(a, 0)->type == CJSON_NULL);
  fail_unless(cjson_array_get(old, 0)->type == CJSON_NUMBER);
  fail_unless(strcmp(cjson_array_get(old, 0)->value.number, "1") == 0);
  fail_unless(cjson_array_get(old, 1)->type == CJSON_NUMBER);
  fail_unless(strcmp(cjson_array_get(old, 1)->value.number, "2") == 0);

  cjson_array_extend(a, old);
  fail_unless(cjson_array_length(a) == 3);
  fail_unless(cjson_array_get(a, 2) == tmp);
  old = cjson_array_truncate(a, 1);
  cjson_free(old);

  old = cjson_array_truncate(a, 0);
  fail_unless(cjson_array_length(a) == 0);
  fail_unless(cjson_array_length(old) == 1);
  fail_unless(cjson_array_get(old, 0)->type == CJSON_NULL);
  cjson_free(old);

  cjson_free(a);
}
END_TEST

static
Suite *
suite(void)
{
  Suite *suite = suite_create("array");

  TCase *tcase_fscan = tcase_create("fscan");
  tcase_add_test(tcase_fscan, fscan);
  suite_add_tcase(suite, tcase_fscan);

  TCase *tcase_fprint = tcase_create("fprint");
  tcase_add_test(tcase_fprint, fprint);
  suite_add_tcase(suite, tcase_fprint);

  TCase *tcase_gambit = tcase_create("gambit");
  tcase_add_test(tcase_gambit, gambit_leaf);
  tcase_add_test(tcase_gambit, gambit_nested_leaf);
  suite_add_tcase(suite, tcase_gambit);

  TCase *tcase_manipulate = tcase_create("manipulate");
  tcase_add_test(tcase_manipulate, manipulate);
  suite_add_tcase(suite, tcase_manipulate);

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
