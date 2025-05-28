#include "_unity/unity.h"
#include <stdbool.h>
#include "../src/args/args.h"

void test_parse_args_no_args(void) {
    char *argv[] = {"cleric"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void test_parse_args_too_many_args(void) {
    char *argv[] = {"cleric", "file.c", "extra"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void test_parse_args_valid_file(void) {
    char *argv[] = {"cleric", "input.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_EQUAL_STRING("input.c", parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void test_parse_args_lex_only_valid(void) {
    char *argv[] = {"cleric", "--lex", "prog.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_EQUAL_STRING("prog.c", parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_TRUE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void test_parse_args_lex_only_missing_file(void) {
    char *argv[] = {"cleric", "--lex"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void test_parse_args_invalid_option(void) {
    char *argv[] = {"cleric", "--invalid", "file.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void test_parse_args_parse_only_valid(void) {
    char *argv[] = {"cleric", "--parse", "prog.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_EQUAL_STRING("prog.c", parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_TRUE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void test_parse_args_parse_only_missing_file(void) {
    char *argv[] = {"cleric", "--parse"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

// --- Tests for --codegen --- 

void test_parse_args_codegen_only_valid(void) {
    char *argv[] = {"cleric", "--codegen", "asm_me.c"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_EQUAL_STRING("asm_me.c", parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_TRUE(codegen_only);
}

void test_parse_args_codegen_only_missing_file(void) {
    char *argv[] = {"cleric", "--codegen"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    bool lex_only = false;
    bool parse_only = false;
    bool validate_only = false;
    bool irgen_only = false;
    bool codegen_only = false;
    TEST_ASSERT_NULL(parse_args(argc, argv, &lex_only, &parse_only, &validate_only, &irgen_only, &codegen_only));
    TEST_ASSERT_FALSE(lex_only);
    TEST_ASSERT_FALSE(parse_only);
    TEST_ASSERT_FALSE(validate_only);
    TEST_ASSERT_FALSE(irgen_only);
    TEST_ASSERT_FALSE(codegen_only);
}

void run_main_args_tests(void) {
    RUN_TEST(test_parse_args_no_args);
    RUN_TEST(test_parse_args_too_many_args);
    RUN_TEST(test_parse_args_valid_file);
    RUN_TEST(test_parse_args_lex_only_valid);
    RUN_TEST(test_parse_args_lex_only_missing_file);
    RUN_TEST(test_parse_args_invalid_option);
    RUN_TEST(test_parse_args_parse_only_valid);
    RUN_TEST(test_parse_args_parse_only_missing_file);
    RUN_TEST(test_parse_args_codegen_only_valid);
    RUN_TEST(test_parse_args_codegen_only_missing_file);
}
