// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "utest.h"

#include <ctype.h>

int sut_sscanf(const char *s, char *fmt, ...);

const char *test_str = ") R 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 "
                       "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n";

UTEST(sscanf, example_scanf) {
  char state;
  int num2 = 0;
  int rc = sut_sscanf(test_str, ") %c%n", &state, &num2);
  ASSERT_EQ(state, 'R');
  ASSERT_EQ(num2, 3);
  ASSERT_EQ(rc, 1);
}

// POSIX-compatible scanf/sscanf tests for common conversions.

UTEST(sscanf, integers_and_bases) {
  int d, i, o, x;
  char s[16];
  int rc = sut_sscanf("  -42 0x10 020 0XFF hello", "%d %i %o %x %s", &d, &i, &o,
                      &x, s);
  ASSERT_EQ(rc, 5);
  ASSERT_EQ(d, -42);
  ASSERT_EQ(i, 0x10); // %i accepts 0x prefix
  ASSERT_EQ(o, 020);  // octal literal -> decimal 16
  ASSERT_EQ(x, 0xFF);
  ASSERT_STREQ(s, "hello");
}

UTEST(sscanf, field_widths) {
  int a = 0;
  int b = 0;
  int rc = sut_sscanf("12345", "%3d%2d", &a, &b);
  ASSERT_EQ(rc, 2);
  ASSERT_EQ(a, 123);
  ASSERT_EQ(b, 45);
}

UTEST(sscanf, scanset_and_string) {
  char alpha[16] = {0};
  int num = 0;
  char tail[16] = {0};
  int rc = sut_sscanf("abc123def", "%[a-z]%d%s", alpha, &num, tail);
  ASSERT_EQ(rc, 3);
  ASSERT_STREQ(alpha, "abc");
  ASSERT_EQ(num, 123);
  ASSERT_STREQ(tail, "def");
}

// New test: negated scanset (%[^...]) should read up to the first char in the
// set.
UTEST(sscanf, negated_scanset) {
  char non_alpha[16] = {0};
  char alpha[16] = {0};
  int rc = sut_sscanf("123abc", "%[^a-z]%[a-z]", non_alpha, alpha);
  ASSERT_EQ(rc, 2);
  ASSERT_STREQ(non_alpha, "123");
  ASSERT_STREQ(alpha, "abc");

  // If the first character is in the scanset, the negated scanset conversion
  // fails. Expect no assignments (returns 0).
  non_alpha[0] = '\0';
  alpha[0] = '\0';
  rc = sut_sscanf("abc123", "%[^a-z]%[a-z]", non_alpha, alpha);
  ASSERT_EQ(rc, 0);
  ASSERT_STREQ(non_alpha, "");
  ASSERT_STREQ(alpha, "");
}

UTEST(sscanf, char_and_whitespace_behavior) {
  char c1 = 0;
  char c2 = 0;
  int rc = sut_sscanf(" A", "%c%c", &c1, &c2); // first %c reads whitespace
  ASSERT_EQ(rc, 2);
  ASSERT_EQ(c1, ' ');
  ASSERT_EQ(c2, 'A');

  char s[4] = {0};
  int n = -1;
  rc = sut_sscanf("  abcdef", "%3s%n", s, &n); // %n not counted in return
  ASSERT_EQ(rc, 1);
  ASSERT_STREQ(s, "abc");
  ASSERT_EQ(n, 5);
}

UTEST(sscanf, suppressed_assignment) {
  int a = 0;
  int b = 0;
  int rc = sut_sscanf("123 skip 456", "%d %*s %d", &a, &b);
  ASSERT_EQ(rc, 2); // suppressed conversion not counted
  ASSERT_EQ(a, 123);
  ASSERT_EQ(b, 456);
}

UTEST(sscanf, literal_matching_and_failure) {
  int a = 0;
  int rc = sut_sscanf("X123", "Y%d", &a); // literal mismatch -> no conversion
  ASSERT_EQ(rc, 0);

  rc = sut_sscanf("Y123", "Y%d", &a);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(a, 123);
}

UTEST(sscanf, partial_input_and_eof) {
  int v1 = 0;
  int v2 = 0;
  int rc = sut_sscanf("10", "%d %d", &v1, &v2); // second missing
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v1, 10);
}

UTEST(sscanf, sp_and_pc_variants_minimal) {
  // basic sanity: %s stops at whitespace, %c always reads one char
  char buf[8] = {0};
  char c = 0;
  int rc = sut_sscanf("hi!", "%3s%c", buf, &c);
  ASSERT_EQ(rc, 1);
  ASSERT_STREQ(buf, "hi!"); // %3s will read up to 3 chars; input shorter
  ASSERT_EQ(c, 0); // no more input -> c unchanged by conversion (implementation
                   // dependent)
  // Note: Some implementations won't touch c if conversion didn't occur; we
  // keep a conservative check above.
}

UTEST(sscanf, percent_i_basic) {
  int v;
  int rc;

  rc = sut_sscanf("42", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 42);

  rc = sut_sscanf("   -15", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, -15);

  rc = sut_sscanf("+0x10", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 0x10);

  rc = sut_sscanf("0X1A", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 0x1A);

  rc = sut_sscanf("077", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 077); // octal -> decimal 63

  rc = sut_sscanf("0", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 0);
}

UTEST(sscanf, percent_i_prefix_and_stop) {
  int v;
  int rc;

  // stops at non-hex digit, parses "0x1"
  rc = sut_sscanf("0x1g", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 1);

  // leading 0 => octal, stops before invalid '9', parses "012" -> 10
  rc = sut_sscanf("0129", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 012); // decimal 10

  // "0x" with no digits -> conversion fails
  rc = sut_sscanf("0x", "%i", &v);
  ASSERT_EQ(rc, 0);
}

UTEST(sscanf, percent_i_field_width_and_suppression) {
  int a = 0;
  int b = 0;
  int rc;

  rc = sut_sscanf("12345", "%3i%2i", &a, &b);
  ASSERT_EQ(rc, 2);
  ASSERT_EQ(a, 123);
  ASSERT_EQ(b, 45);

  // suppression: %*s doesn't count toward return value
  rc = sut_sscanf("0x10 extra", "%i %*s", &a);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(a, 0x10);
}

UTEST(sscanf, percent_i_signed_hex_and_zero) {
  int v;
  int rc;

  rc = sut_sscanf("-0x10", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, -0x10);

  rc = sut_sscanf("+077", "%i", &v);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(v, 077); // octal with explicit plus
}

// More scanset examples: numbers with commas, and single letters separated by
// commas.

UTEST(sscanf, scanset_numbers_positive) {
  char nums[32] = {0};
  int rc = sut_sscanf("1,234,567", "%[0-45,]", nums);
  ASSERT_EQ(rc, 1);
  ASSERT_STREQ(nums, "1,234,5");
}

UTEST(sscanf, scanset_many_dashes_and_commas) {
  char nums[32] = {0};
  int rc = sut_sscanf("1,234,567", "%[0-4,5-6,]", nums);
  ASSERT_EQ(rc, 1);
  ASSERT_STREQ(nums, "1,234,56");
}

UTEST(sscanf, scanset_numbers_split_by_commas_negated) {
  char a[8] = {0}, b[8] = {0}, c[8] = {0};
  int rc = sut_sscanf("1,234,567", "%[^,],%[^,],%[^,]", a, b, c);
  ASSERT_EQ(rc, 3);
  ASSERT_STREQ(a, "1");
  ASSERT_STREQ(b, "234");
  ASSERT_STREQ(c, "567");

  // If input starts with the separator, the negated scanset fails (no
  // assignment).
  a[0] = b[0] = c[0] = '\0';
  rc = sut_sscanf(",a,b", "%[^,],%[^,],%[^,]", a, b, c);
  ASSERT_EQ(rc, 0);
  ASSERT_STREQ(a, "");
}

UTEST(sscanf, scanset_single_letters_commas_positive_and_negated) {
  char l1[4] = {0}, l2[4] = {0}, l3[4] = {0};
  int rc = sut_sscanf("a,b,c", "%[a-z],%[a-z],%[a-z]", l1, l2, l3);
  ASSERT_EQ(rc, 3);
  ASSERT_STREQ(l1, "a");
  ASSERT_STREQ(l2, "b");
  ASSERT_STREQ(l3, "c");

  // Same parsing using negated scanset to read up to commas.
  l1[0] = l2[0] = l3[0] = '\0';
  rc = sut_sscanf("a,b,c", "%[^,],%[^,],%[^,]", l1, l2, l3);
  ASSERT_EQ(rc, 3);
  ASSERT_STREQ(l1, "a");
  ASSERT_STREQ(l2, "b");
  ASSERT_STREQ(l3, "c");
}
