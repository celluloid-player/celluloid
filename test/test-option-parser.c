#include <glib.h>

#include "celluloid-option-parser.h"

static void
test_option_with_no_value(void)
{
	const gchar *str = "--foo";
	gchar *key = NULL;
	gchar *val = NULL;

	str = parse_option(str, &key, &val);
	g_assert_true(str && !*str);
	g_assert_true(g_strcmp0(key, "foo") == 0);
	g_assert_true(!val);
	g_free(key);
	g_free(val);
}

static void
test_option_with_value(void)
{
	const gchar *str = "--foo=bar";
	gchar *key = NULL;
	gchar *val = NULL;

	str = parse_option(str, &key, &val);
	g_assert_true(str && !*str);
	g_assert_true(g_strcmp0(key, "foo") == 0);
	g_assert_true(g_strcmp0(val, "bar") == 0);
	g_free(key);
	g_free(val);
}

static void
test_option_with_hyphen(void)
{
	const gchar *str = "--foo-bar=baz";
	gchar *key = NULL;
	gchar *val = NULL;

	str = parse_option(str, &key, &val);
	g_assert_true(str && !*str);
	g_assert_true(g_strcmp0(key, "foo-bar") == 0);
	g_assert_true(g_strcmp0(val, "baz") == 0);
	g_free(key);
	g_free(val);
}

static void
test_option_with_underscore(void)
{
	const gchar *str = "--foo_bar=baz";
	gchar *key = NULL;
	gchar *val = NULL;

	str = parse_option(str, &key, &val);
	g_assert_true(str && !*str);
	g_assert_true(g_strcmp0(key, "foo_bar") == 0);
	g_assert_true(g_strcmp0(val, "baz") == 0);
	g_free(key);
	g_free(val);
}

static void
test_multiple_options(void)
{
	const gchar *str = "--foo --foo-bar=baz --bar --baz=foo";
	gchar *key = NULL;
	gchar *val = NULL;

	str = parse_option(str, &key, &val);
	g_assert_true(str);
	g_assert_true(g_strcmp0(key, "foo") == 0);
	g_assert_true(!val);
	g_clear_pointer(&key, g_free);
	g_clear_pointer(&val, g_free);

	str = parse_option(str, &key, &val);
	g_assert_true(str);
	g_assert_true(g_strcmp0(key, "foo-bar") == 0);
	g_assert_true(g_strcmp0(val, "baz") == 0);
	g_clear_pointer(&key, g_free);
	g_clear_pointer(&val, g_free);

	str = parse_option(str, &key, &val);
	g_assert_true(str);
	g_assert_true(g_strcmp0(key, "bar") == 0);
	g_assert_true(!val);
	g_clear_pointer(&key, g_free);
	g_clear_pointer(&val, g_free);

	str = parse_option(str, &key, &val);
	g_assert_true(str && !*str);
	g_assert_true(g_strcmp0(key, "baz") == 0);
	g_assert_true(g_strcmp0(val, "foo") == 0);
	g_clear_pointer(&key, g_free);
	g_clear_pointer(&val, g_free);
}

static void
test_option_with_no_prefix(void)
{
	const gchar *str = "foo=bar";
	gchar *key = NULL;
	gchar *val = NULL;

	str = parse_option(str, &key, &val);
	g_assert_true(str && !*str);
	g_assert_true(g_strcmp0(key, "foo") == 0);
	g_assert_true(g_strcmp0(val, "bar") == 0);
	g_free(key);
	g_free(val);
}

int
main(gint argc, gchar **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/test-option-with-no-value",
			test_option_with_no_value );
	g_test_add_func("/test-option-with-value",
			test_option_with_value );
	g_test_add_func("/test-option-with-hyphen",
			test_option_with_hyphen );
	g_test_add_func("/test-option-with-underscore",
			test_option_with_underscore );
	g_test_add_func("/test-multiple-options",
			test_multiple_options );
	g_test_add_func("/test-option-with-no-prefix",
			test_option_with_no_prefix );

	return g_test_run();
}
