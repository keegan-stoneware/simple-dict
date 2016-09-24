#pragma once

#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>

typedef enum SimpleDictDataType {
  SimpleDictDataType_Raw,
  SimpleDictDataType_Bool,
  SimpleDictDataType_Int,
  SimpleDictDataType_String,

  SimpleDictDataTypeCount
} SimpleDictDataType;

typedef LinkedRoot SimpleDict;

SimpleDict *simple_dict_create(void);

void simple_dict_destroy(SimpleDict *dict);

bool simple_dict_contains(const SimpleDict *dict, const char *key);

void simple_dict_remove(SimpleDict *dict, const char *key);

void simple_dict_clear(SimpleDict *dict);

void simple_dict_update_data(SimpleDict *dict, const char *key, const void *data, size_t n);

void simple_dict_update_bool(SimpleDict *dict, const char *key, bool value);

void simple_dict_update_int(SimpleDict *dict, const char *key, int value);

void simple_dict_update_string(SimpleDict *dict, const char *key, const char *value);

bool simple_dict_get_data(const SimpleDict *dict, const char *key, void *value_out, size_t n);

bool simple_dict_get_bool(const SimpleDict *dict, const char *key, bool *value_out);

bool simple_dict_get_int(const SimpleDict *dict, const char *key, int *value_out);

bool simple_dict_get_string(const SimpleDict *dict, const char *key, char *value_out, size_t n);

typedef bool (*SimpleDictForEachCallback)(const char *key, SimpleDictDataType type,
                                          const void *data, size_t data_size, void *context);

void simple_dict_foreach(const SimpleDict *dict, SimpleDictForEachCallback callback, void *context);
