#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include "kanaco.h"

void test_is_1byte()
{
    char *s1 = "abc";
    CU_ASSERT_TRUE(is_1byte(s1, strlen(s1)));
    char *s2 = "¼½¾";
    CU_ASSERT_FALSE(is_1byte(s2, strlen(s2)));
    char *s3 = "あいう";
    CU_ASSERT_FALSE(is_1byte(s3, strlen(s3)));
    char *s4 = "🌀🌁🌂";
    CU_ASSERT_FALSE(is_1byte(s4, strlen(s4)));
}

void test_is_2bytes()
{
    char *s1 = "abc";
    CU_ASSERT_FALSE(is_2bytes(s1, strlen(s1)));
    char *s2 = "¼½¾";
    CU_ASSERT_TRUE(is_2bytes(s2, strlen(s2)));
    char *s3 = "あいう";
    CU_ASSERT_FALSE(is_2bytes(s3, strlen(s3)));
    char *s4 = "🌀🌁🌂";
    CU_ASSERT_FALSE(is_2bytes(s4, strlen(s4)));
}

void test_is_3bytes()
{
    char *s1 = "abc";
    CU_ASSERT_FALSE(is_3bytes(s1, strlen(s1)));
    char *s2 = "¼½¾";
    CU_ASSERT_FALSE(is_3bytes(s2, strlen(s2)));
    char *s3 = "あいう";
    CU_ASSERT_TRUE(is_3bytes(s3, strlen(s3)));
    char *s4 = "🌀🌁🌂";
    CU_ASSERT_FALSE(is_3bytes(s4, strlen(s4)));
}

void test_is_4bytes()
{
    char *s1 = "abc";
    CU_ASSERT_FALSE(is_4bytes(s1, strlen(s1)));
    char *s2 = "¼½¾";
    CU_ASSERT_FALSE(is_4bytes(s2, strlen(s2)));
    char *s3 = "あいう";
    CU_ASSERT_FALSE(is_4bytes(s3, strlen(s3)));
    char *s4 = "🌀🌁🌂";
    CU_ASSERT_TRUE(is_4bytes(s4, strlen(s4)));
}

void test_is_voiced()
{
    char *s1 = "ｶﾞｷﾞｸﾞｹﾞｺﾞｻﾞｼﾞｽﾞｾﾞｿﾞﾀﾞﾁﾞﾂﾞﾃﾞﾄﾞﾊﾞﾋﾞﾌﾞﾍﾞﾎﾞｳﾞ";
    int len1 = (int)(strlen(s1) / 6);
    for (int i = 0; i < len1; i++)
    {
        CU_ASSERT_TRUE(is_voiced(s1 + i * 6, strlen(s1) - i * 6));
    }
    char *s2 = "ｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾊﾋﾌﾍﾎｳ";
    int len2 = (int)(strlen(s2) / 3);
    for (int i = 0; i < len2; i++)
    {
        CU_ASSERT_FALSE(is_voiced(s2 + i * 3, strlen(s2) - i * 3));
    }
}

void test_is_semi_voiced()
{
    char *s1 = "ﾊﾟﾋﾟﾌﾟﾍﾟﾎﾟ";
    int len1 = (int)(strlen(s1) / 6);
    for (int i = 0; i < len1; i++)
    {
        CU_ASSERT_TRUE(is_semi_voiced(s1 + i * 6, strlen(s1) - i * 6));
    }
    char *s2 = "ﾊﾋﾌﾍﾎ";
    int len2 = (int)(strlen(s2) / 3);
    for (int i = 0; i < len2; i++)
    {
        CU_ASSERT_FALSE(is_semi_voiced(s2 + i * 3, strlen(s2) - i * 3));
    }
}

void test_lower_r()
{
    character cases[4] = {
        {"Ａ", strlen("Ａ"), CNV_LOWER_R, "", 0},
        {"Ｚ", strlen("Ｚ"), CNV_LOWER_R, "", 0},
        {"ａ", strlen("ａ"), CNV_LOWER_R, "", 0},
        {"ｚ", strlen("ｚ"), CNV_LOWER_R, "", 0},
    };
    char *expected = "AZaz";

    for (int i = 0; i < 4; i++)
    {
        lower_r(&cases[i]);
        // printf("[%d] %s(%d) -> %s(%d): %c\n", i, (cases + i)->val, (cases + i)->len, (cases + i)->cval, (cases + i)->clen, expected[i]);
        CU_ASSERT_STRING_EQUAL(&cases[i].cval, expected[i]);
        CU_ASSERT_EQUAL(cases[i].clen, 1);
        printf("\n");
    }
}

void test_upper_r()
{
}

void test_lower_n()
{
}

void test_upper_n()
{
}

void test_lower_a()
{
}

void test_upper_a()
{
}

void test_lower_s()
{
}

void test_upper_s()
{
}

void test_lower_k()
{
}

void test_upper_k()
{
}

void test_lower_h()
{
}

void test_upper_h()
{
}

void test_lower_c()
{
}

void test_upper_c()
{
}

void test_asis()
{
}

void test_create_filters()
{
}

void test_init_character()
{
    character c;
    init_character(&c);
    CU_ASSERT_EQUAL(c.val[0], 0x00);
    CU_ASSERT_EQUAL(c.len, 0);
    CU_ASSERT_EQUAL(c.conv, CNV_ASIS);
    CU_ASSERT_EQUAL(c.cval[0], 0x00);
    CU_ASSERT_EQUAL(c.clen, 0);
}

void test_conv()
{
}

void test_extract()
{
}

int main(int argc, char **argv)
{
    // Initialize
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    CU_pSuite suite = CU_add_suite("MathTest", NULL, NULL);
    if (!suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_add_test(suite, "test_is_1byte", test_is_1byte);
    CU_add_test(suite, "test_is_2bytes", test_is_2bytes);
    CU_add_test(suite, "test_is_3bytes", test_is_3bytes);
    CU_add_test(suite, "test_is_4bytes", test_is_4bytes);
    CU_add_test(suite, "test_is_voiced", test_is_voiced);
    CU_add_test(suite, "test_is_semi_voiced", test_is_semi_voiced);
    CU_add_test(suite, "test_lower_r", test_lower_r);
    CU_add_test(suite, "test_upper_r", test_upper_r);
    CU_add_test(suite, "test_lower_n", test_lower_n);
    CU_add_test(suite, "test_upper_n", test_upper_n);
    CU_add_test(suite, "test_lower_a", test_lower_a);
    CU_add_test(suite, "test_upper_a", test_upper_a);
    CU_add_test(suite, "test_lower_s", test_lower_s);
    CU_add_test(suite, "test_upper_s", test_upper_s);
    CU_add_test(suite, "test_lower_k", test_lower_k);
    CU_add_test(suite, "test_upper_k", test_upper_k);
    CU_add_test(suite, "test_lower_h", test_lower_h);
    CU_add_test(suite, "test_upper_h", test_upper_h);
    CU_add_test(suite, "test_lower_c", test_lower_c);
    CU_add_test(suite, "test_upper_c", test_upper_c);
    CU_add_test(suite, "test_asis", test_asis);
    CU_add_test(suite, "test_create_filters", test_create_filters);
    CU_add_test(suite, "test_init_character", test_init_character);
    CU_add_test(suite, "test_conv", test_conv);
    CU_add_test(suite, "test_extract", test_extract);

    // Execute test (BasicMode)
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();
    return 0;
}
