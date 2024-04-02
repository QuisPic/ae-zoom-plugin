#include <string>

/* Macros for reading a file to a string at compile time */
#define STRINGIFY(...) #__VA_ARGS__
#define STR(...) STRINGIFY(__VA_ARGS__)

extern std::string zoom_increment_js;
extern std::string zoom_set_to_js;
extern std::string pass_key_bind_js;
