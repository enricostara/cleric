#include "unity/unity.h"
#include <stdbool.h>
#include "../src/args/args.h"

void test_parse_args_no_args(void) {
    char *argv[] = {"cleric"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only));
    TEST_ASSERT_FALSE(lex_only);
}

void test_parse_args_too_many_args(void) {
    char *argv[] = {"cleric", "file.c", "extra"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only));
    TEST_ASSERT_FALSE(lex_only);
}

void test_parse_args_valid_file(void) {
    char *argv[] = {"cleric", "input.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    TEST_ASSERT_EQUAL_STRING("input.c", parse_args(argc, argv, &lex_only));
    TEST_ASSERT_FALSE(lex_only);
}

void test_parse_args_lex_only_valid(void) {
    char *argv[] = {"cleric", "--lex", "prog.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    TEST_ASSERT_EQUAL_STRING("prog.c", parse_args(argc, argv, &lex_only));
    TEST_ASSERT_TRUE(lex_only);
}

void test_parse_args_lex_only_missing_file(void) {
    char *argv[] = {"cleric", "--lex"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only));
    TEST_ASSERT_FALSE(lex_only); // Should reset lex_only
}

void test_parse_args_invalid_option(void) {
    char *argv[] = {"cleric", "--invalid", "file.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only));
    TEST_ASSERT_FALSE(lex_only);
}

void run_main_args_tests(void) {
    RUN_TEST(test_parse_args_no_args);
    RUN_TEST(test_parse_args_too_many_args);
    RUN_TEST(test_parse_args_valid_file);
    RUN_TEST(test_parse_args_lex_only_valid);
    RUN_TEST(test_parse_args_lex_only_missing_file);
    RUN_TEST(test_parse_args_invalid_option);
}
