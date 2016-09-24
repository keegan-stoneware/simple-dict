#include "simple-dict.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct SimpleDictNode {
  char *key;
  SimpleDictDataType type;
  size_t length;
  void *data;
} SimpleDictNode;

SimpleDict *simple_dict_create(void) {
  return linked_list_create_root();
}

static void prv_node_destroy(SimpleDictNode *node) {
  if (node) {
    free(node->key);
    free(node->data);
    free(node);
  }
}

static bool prv_linked_list_foreach_free_data_callback(void *data, void *context) {
  SimpleDictNode *node = data;
  prv_node_destroy(node);
  return true;
}

static void prv_destroy_and_clear_nodes_in_dict(SimpleDict *dict) {
  if (!dict) {
    return;
  }

  linked_list_foreach(dict, prv_linked_list_foreach_free_data_callback, NULL);
  linked_list_clear(dict);
}

void simple_dict_destroy(SimpleDict *dict) {
  prv_destroy_and_clear_nodes_in_dict(dict);
  free(dict);
}

typedef struct LinkedListForeachFindNodeCallbackData {
  const char *key;
  SimpleDictNode *node_found;
  size_t index;
} LinkedListForeachFindNodeCallbackData;

static bool prv_linked_list_foreach_find_node_callback(void *data, void *context) {
  LinkedListForeachFindNodeCallbackData *callback_data = context;
  if (!callback_data || !callback_data->key) {
    return false;
  }

  SimpleDictNode *node = data;
  if (!node || !node->key) {
    callback_data->index++;
    return true;
  }

  const bool found_node = (strcmp(callback_data->key, node->key) == 0);
  if (found_node) {
    callback_data->node_found= node;
  }

  callback_data->index++;
  return !found_node;
}

static bool prv_find_node_and_index_in_dict(const SimpleDict *dict, const char *key,
                                            SimpleDictNode **node_out, size_t *index_out) {
  if (!dict) {
    return false;
  }

  LinkedListForeachFindNodeCallbackData callback_data = (LinkedListForeachFindNodeCallbackData) {
    .key = key,
  };
  linked_list_foreach((SimpleDict *)dict, prv_linked_list_foreach_find_node_callback,
                      &callback_data);
  if (callback_data.node_found) {
    // Subtract one for incrementing on the last node
    callback_data.index--;
    if (node_out) {
      *node_out = callback_data.node_found;
    }
    if (index_out) {
      *index_out = callback_data.index;
    }
  }
  return (callback_data.node_found != NULL);
}

static bool prv_find_node_in_dict(const SimpleDict *dict, const char *key,
                                  SimpleDictNode **node_out) {
  return prv_find_node_and_index_in_dict(dict, key, node_out, NULL /* index */);
}

bool simple_dict_contains(const SimpleDict *dict, const char *key) {
  return prv_find_node_and_index_in_dict(dict, key, NULL /* node */, NULL /* index */);
}

void simple_dict_remove(SimpleDict *dict, const char *key) {
  SimpleDictNode *node;
  size_t index;
  if (prv_find_node_and_index_in_dict(dict, key, &node, &index)) {
    prv_node_destroy(node);
    linked_list_remove(dict, (uint16_t)index);
  }
}

void simple_dict_clear(SimpleDict *dict) {
  prv_destroy_and_clear_nodes_in_dict(dict);
}

static void prv_update_data(SimpleDict *dict, const char *key, const void *data,
                            SimpleDictDataType type, size_t n) {
  if (!dict || !key) {
    return;
  }

  if (!data || (n == 0)) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Can't set NULL/zero-length data for key %s, use simple_dict_remove() to delete key "
              "instead", key);
    return;
  }

  void *data_copy = malloc(n);
  if (!data_copy) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to malloc %zu bytes for data of key %s", n, key);
    return;
  }

  memcpy(data_copy, data, n);

  SimpleDictNode *node;
  bool key_already_present_in_dict = prv_find_node_and_index_in_dict(dict, key, &node,
                                                                     NULL /* index */);
  if (key_already_present_in_dict) {
    free(node->data);
  } else {
    const size_t key_length = strlen(key);
    const size_t key_size = key_length + 1;
    char *key_copy = malloc(key_size);
    if (!key_copy) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to malloc %zu bytes for copy of key %s", key_size, key);
      free(data_copy);
      return;
    }
    strcpy(key_copy, key);

    node = malloc(sizeof(*node));
    if (!node) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to malloc %zu bytes for key %s", n, key);
      free(key_copy);
      free(data_copy);
      return;
    }
    node->key = key_copy;
  }

  node->type = type;
  node->data = data_copy;
  node->length = n;

  if (!key_already_present_in_dict) {
    // Prepend is faster then append
    linked_list_prepend(dict, node);
  }
}

void simple_dict_update_data(SimpleDict *dict, const char *key, const void *data, size_t n) {
  prv_update_data(dict, key, data, SimpleDictDataType_Raw, n);
}

void simple_dict_update_bool(SimpleDict *dict, const char *key, bool value) {
  prv_update_data(dict, key, &value, SimpleDictDataType_Bool, sizeof(value));
}

void simple_dict_update_int(SimpleDict *dict, const char *key, int value) {
  prv_update_data(dict, key, &value, SimpleDictDataType_Int, sizeof(value));
}

void simple_dict_update_string(SimpleDict *dict, const char *key, const char *value) {
  if (!dict || !key) {
    return;
  }

  if (!value) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Can't set NULL/zero-length string for key %s, use simple_dict_remove() to delete key "
              "instead", key);
    return;
  }

  const size_t value_length = strlen(value);
  const size_t value_size = value_length + 1;
  prv_update_data(dict, key, value, SimpleDictDataType_String, value_size);
}

static bool prv_get_data_and_size_read(const SimpleDict *dict, const char *key,
                                       SimpleDictDataType expected_type, void *value_out, size_t n,
                                       size_t *size_read_out) {
  SimpleDictNode *node;
  if (!prv_find_node_in_dict(dict, key, &node)) {
    return false;
  }

  if (node->type != expected_type) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Value for key %s does not match type requested", key);
  }

  const size_t size_to_copy = MIN(node->length, n);

  if (value_out) {
    memcpy(value_out, node->data, size_to_copy);
  }

  if (size_read_out) {
    *size_read_out = size_to_copy;
  }

  return true;
}

static bool prv_get_data(const SimpleDict *dict, const char *key, SimpleDictDataType expected_type,
                         void *value_out, size_t n) {
  return prv_get_data_and_size_read(dict, key, expected_type, value_out, n, NULL /* size_read */);
}

bool simple_dict_get_data(const SimpleDict *dict, const char *key, void *value_out, size_t n) {
  return prv_get_data(dict, key, SimpleDictDataType_Raw, value_out, n);
}

bool simple_dict_get_bool(const SimpleDict *dict, const char *key, bool *value_out) {
  return prv_get_data(dict, key, SimpleDictDataType_Bool, value_out, sizeof(*value_out));
}

bool simple_dict_get_int(const SimpleDict *dict, const char *key, int *value_out) {
  return prv_get_data(dict, key, SimpleDictDataType_Int, value_out, sizeof(*value_out));
}

bool simple_dict_get_string(const SimpleDict *dict, const char *key, char *value_out, size_t n) {
  size_t size_read;
  const bool string_read = prv_get_data_and_size_read(dict, key, SimpleDictDataType_String,
                                                      value_out, n, &size_read);
  if (string_read) {
    value_out[size_read - 1] = '\0';
  }

  return string_read;
}

typedef struct LinkedListForeachForeachCallbackData {
  SimpleDictForEachCallback callback;
  void *context;
} LinkedListForeachForeachCallbackData;

static bool prv_linked_list_foreach_foreach_callback(void *data, void *context) {
  LinkedListForeachForeachCallbackData *callback_data = context;
  if (!callback_data || !callback_data->callback) {
    return false;
  }

  SimpleDictNode *node = data;
  if (!node) {
    return true;
  }

  return callback_data->callback(node->key, node->type, node->data, node->length,
                                 callback_data->context);
}

void simple_dict_foreach(const SimpleDict *dict, SimpleDictForEachCallback callback,
                         void *context) {
  if (!dict) {
    return;
  }

  LinkedListForeachForeachCallbackData callback_data = (LinkedListForeachForeachCallbackData) {
    .callback = callback,
    .context = context,
  };
  linked_list_foreach((SimpleDict *)dict, prv_linked_list_foreach_foreach_callback, &callback_data);
}
