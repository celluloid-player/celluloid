#include <glib.h>

#include "../src/celluloid-playlist-model.h"
#include "../src/celluloid-playlist-item.h"

#include <stdio.h>

#define TEST_DATA \
	{	{"Foo", "file:///foo.webm"}, \
		{"Bar", "file:///bar.webm"}, \
		{"Baz", "file:///baz.webm"}, \
		{NULL, NULL} }

struct ItemData
{
	const gchar *title;
	const gchar *uri;
};

static CelluloidPlaylistModel *
add_test_data(CelluloidPlaylistModel *model)
{
	const struct ItemData data[] = TEST_DATA;

	const guint initial_n_items =
		g_list_model_get_n_items(G_LIST_MODEL(model));

	for(guint i = 0; data[i].title; i++)
	{
		CelluloidPlaylistItem *item =
			celluloid_playlist_item_new_take
			(g_strdup(data[i].title), g_strdup(data[i].uri), FALSE);

		celluloid_playlist_model_append(model, item);

		const guint n_items =
			g_list_model_get_n_items(G_LIST_MODEL(model));
		g_assert_cmpuint(n_items, ==, initial_n_items + i + 1);
	}

	return model;
}

static void
test_append(void)
{

	const struct ItemData data[] = TEST_DATA;

	CelluloidPlaylistModel *model = celluloid_playlist_model_new();
	g_assert_true(g_list_model_get_n_items(G_LIST_MODEL(model)) == 0);

	add_test_data(model);

	const guint n_items = g_list_model_get_n_items(G_LIST_MODEL(model));

	for(guint i = 0; i < n_items; i++)
	{
		CelluloidPlaylistItem *item =
			g_list_model_get_item(G_LIST_MODEL(model), i);
		const gchar *title =
			celluloid_playlist_item_get_title(item);
		const gchar *uri =
			celluloid_playlist_item_get_uri(item);

		g_assert_cmpstr(title, ==, data[i].title);
		g_assert_cmpstr(uri, ==, data[i].uri);
	}

	g_object_unref(model);
}

static void
test_remove(void)
{
	CelluloidPlaylistModel *model = celluloid_playlist_model_new();
	CelluloidPlaylistItem *item = NULL;
	const gchar *title = NULL;
	const gchar *uri = NULL;
	gboolean is_current = TRUE;
	guint n_items = 0;

	add_test_data(model);

	celluloid_playlist_model_remove(model, 1);

	n_items = g_list_model_get_n_items(G_LIST_MODEL(model));
	g_assert_cmpuint(n_items, ==, 2);

	item = g_list_model_get_item(G_LIST_MODEL(model), 0);
	title = celluloid_playlist_item_get_title(item);
	uri = celluloid_playlist_item_get_uri(item);
	is_current = celluloid_playlist_item_get_is_current(item);
	g_assert_cmpstr(title, ==, "Foo");
	g_assert_cmpstr(uri, ==, "file:///foo.webm");
	g_assert_cmpint(is_current, ==, FALSE);

	item = g_list_model_get_item(G_LIST_MODEL(model), 1);
	title = celluloid_playlist_item_get_title(item);
	uri = celluloid_playlist_item_get_uri(item);
	is_current = celluloid_playlist_item_get_is_current(item);
	g_assert_cmpstr(title, ==, "Baz");
	g_assert_cmpstr(uri, ==, "file:///baz.webm");
	g_assert_cmpint(is_current, ==, FALSE);

	g_object_unref(model);
}

static void
test_clear(void)
{
	CelluloidPlaylistModel *model = celluloid_playlist_model_new();
	guint n_items = 0;

	add_test_data(model);
	n_items = g_list_model_get_n_items(G_LIST_MODEL(model));
	g_assert_cmpuint(n_items, ==, 3);

	celluloid_playlist_model_clear(model);
	n_items = g_list_model_get_n_items(G_LIST_MODEL(model));
	g_assert_cmpuint(n_items, ==, 0);

	// Check if the clear function works correctly if the model is already
	// empty.
	celluloid_playlist_model_clear(model);
	n_items = g_list_model_get_n_items(G_LIST_MODEL(model));
	g_assert_cmpuint(n_items, ==, 0);

	g_object_unref(model);
}

static void
test_set_current(void)
{
	CelluloidPlaylistModel *model = celluloid_playlist_model_new();
	gint current = -1;

	add_test_data(model);

	current = celluloid_playlist_model_get_current(model);
	g_assert_cmpint(current, <, 0);

	celluloid_playlist_model_set_current(model, 1);
	current = celluloid_playlist_model_get_current(model);
	g_assert_cmpint(current, ==, 1);

	celluloid_playlist_model_set_current(model, 10);
	current = celluloid_playlist_model_get_current(model);
	g_assert_cmpint(current, ==, 2);

	celluloid_playlist_model_set_current(model, -10);
	current = celluloid_playlist_model_get_current(model);
	g_assert_cmpint(current, <, 0);

	g_object_unref(model);
}

static void
handle_contents_changed(	CelluloidPlaylistModel *model,
				guint position,
				guint removed,
				guint added,
				gpointer data )
{
	guint *counter = data;

	*counter += 1;
}

static void
handle_items_changed(	CelluloidPlaylistModel *model,
			guint position,
			guint removed,
			guint added,
			gpointer data )
{
	guint *counter = data;

	*counter += 1;
}

static void
test_signals(void)
{
	CelluloidPlaylistModel *model = celluloid_playlist_model_new();
	guint contents_changed_counter = 0;
	guint items_changed_counter = 0;

	g_signal_connect(	model,
				"contents-changed",
				G_CALLBACK(handle_contents_changed),
				&contents_changed_counter );
	g_signal_connect(	model,
				"items-changed",
				G_CALLBACK(handle_items_changed),
				&items_changed_counter );

	add_test_data(model);
	g_assert_cmpuint(contents_changed_counter, ==, 3);
	g_assert_cmpuint(items_changed_counter, ==, 3);

	celluloid_playlist_model_set_current(model, 2);
	g_assert_cmpuint(contents_changed_counter, ==, 3);
	g_assert_cmpuint(items_changed_counter, ==, 4);

	celluloid_playlist_model_remove(model, 1);
	g_assert_cmpuint(contents_changed_counter, ==, 4);
	g_assert_cmpuint(items_changed_counter, ==, 5);

	g_object_unref(model);
}

int
main(gint argc, gchar **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/test-append", test_append);
	g_test_add_func("/test-remove", test_remove);
	g_test_add_func("/test-clear", test_clear);
	g_test_add_func("/test-set-current", test_set_current);
	g_test_add_func("/test-signals", test_signals);

	return g_test_run();
}
