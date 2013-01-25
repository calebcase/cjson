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
#define IN "{}"
  char *buf = IN;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "r");
  struct cjson *node = cjson_object_fscan(stream, NULL);

  fail_unless(node != NULL);
  fail_unless(node->type == CJSON_OBJECT);
  fail_unless(node->value.object.key_length == 0);
  fail_unless(node->value.object.data == NULL);

  fclose(stream);
  cjson_free(node);
#undef IN
}
END_TEST

START_TEST(fprint)
{
#define IN "{}"
#define EXP "{}"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_object_fscan(in_stream, NULL);
  fclose(in_stream);

  cjson_object_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print object to stream. Got: %s Exp: %s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(gambit_leaf)
{
#define IN "{\"number\": 3.14, \"string\": \"\", \"boolean\": true, \"null\": null}"
#define EXP "{\n" \
            "  \"boolean\": true,\n" \
            "  \"null\": null,\n" \
            "  \"number\": 3.14,\n" \
            "  \"string\": \"\"\n" \
            "}"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_object_fscan(in_stream, NULL);
  fclose(in_stream);

  cjson_object_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print object to stream.\nGot:\n%s\nExp:\n%s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(gambit_nested_leaf)
{
#define IN "{\"empty\": {}, \"one\": {\"number\": 3.14}, \"down\": {\"number\": 3.14, \"down\": {\"string\": \"\", \"down\": {\"boolean\": true, \"down\": {\"null\": null}}}}}"
#define EXP "{\n" \
            "  \"down\": {\n" \
            "    \"down\": {\n" \
            "      \"down\": {\n" \
            "        \"boolean\": true,\n" \
            "        \"down\": {\n" \
            "          \"null\": null\n" \
            "        }\n" \
            "      },\n" \
            "      \"string\": \"\"\n" \
            "    },\n" \
            "    \"number\": 3.14\n" \
            "  },\n" \
            "  \"empty\": {},\n" \
            "  \"one\": {\n" \
            "    \"number\": 3.14\n" \
            "  }\n" \
            "}"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_object_fscan(in_stream, NULL);
  fclose(in_stream);

  cjson_object_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print object to stream.\nGot:\n%s\nExp:\n%s";
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
  Suite *suite = suite_create("object");

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
