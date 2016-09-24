#include <pebble.h>

#include <simple-dict/simple-dict.h>

static const char *prv_get_result_string(bool result) {
  return result ? "PASS" : "FAIL";
}

static AppLogLevel prv_get_result_log_level(bool result) {
  return result ? APP_LOG_LEVEL_INFO : APP_LOG_LEVEL_ERROR;
}

static bool prv_test_int(const SimpleDict *dict, const char *key, int expected) {
  int actual;
  const bool pass = (simple_dict_get_int(dict, key, &actual) && (actual == expected));
  const char *result = prv_get_result_string(pass);
  const AppLogLevel log_level = prv_get_result_log_level(pass);
  APP_LOG(log_level, "%s: %s = %d", result, key, actual);
  return pass;
}

static bool prv_test_bool(const SimpleDict *dict, const char *key, bool expected) {
  bool actual;
  const bool pass = (simple_dict_get_bool(dict, key, &actual) && (actual == expected));
  const char *result = prv_get_result_string(pass);
  const AppLogLevel log_level = prv_get_result_log_level(pass);
  APP_LOG(log_level, "%s: %s = %s", result, key, actual ? "true" : "false");
  return pass;
}

static bool prv_test_string(const SimpleDict *dict, const char *key, const char *expected) {
  char actual_buffer[128] = {0};
  const bool pass =
      (simple_dict_get_string(dict, key, actual_buffer, sizeof(actual_buffer)) &&
       (strncmp(actual_buffer, expected, sizeof(actual_buffer)) == 0));
  const char *result = prv_get_result_string(pass);
  const AppLogLevel log_level = prv_get_result_log_level(pass);
  APP_LOG(log_level, "%s: %s = %s", result, key, actual_buffer);
  return pass;
}

static bool prv_count_entries(const char *key, SimpleDictDataType type, const void *data,
                              size_t data_size, void *context) {
  size_t *count = context;
  if (count) {
    (*count)++;
  }
  return (count != NULL);
}

void test(void) {
  SimpleDict *dict = simple_dict_create();

  const char *key1 = "key1";
  const int key1_expected = 1337;
  simple_dict_update_int(dict, key1, key1_expected);
  if (!prv_test_int(dict, key1, key1_expected)) {
    return;
  }

  simple_dict_remove(dict, key1);
  const bool key1_removed = !simple_dict_contains(dict, key1);
  APP_LOG(prv_get_result_log_level(key1_removed), "%s: %s %s", prv_get_result_string(key1_removed),
          key1, key1_removed ? "removed" : "still exists");
  if (!key1_removed) {
    return;
  }

  const char *key2 = "key2";
  const bool key2_expected = true;
  simple_dict_update_bool(dict, key2, key2_expected);
  if (!prv_test_bool(dict, key2, key2_expected)) {
    return;
  }

  const char *key3 = "key3";
  const char *key3_expected = "boogie-woogie";
  simple_dict_update_string(dict, key3, key3_expected);
  if (!prv_test_string(dict, key3, key3_expected)) {
    return;
  }

  size_t count = 0;
  simple_dict_foreach(dict, prv_count_entries, &count);
  const bool count_correct = (count == 2);
  APP_LOG(prv_get_result_log_level(count_correct), "%s: count via foreach = %zu",
          prv_get_result_string(count_correct), count);
  if (!count_correct) {
    return;
  }

  simple_dict_destroy(dict);
}

int main(void) {
  test();
  app_event_loop();
}
