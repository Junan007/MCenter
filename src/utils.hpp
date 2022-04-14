#ifndef UTILS_HPP
#define UTILS_HPP

#include <stdio.h>

extern "C" {
    #include <plist/plist.h>
}

int is_key_wants(const char *key);

char *base64encode(const unsigned char *buf, size_t size);

void plist_node_print_to_stream(plist_t node, int* indent_level, FILE* stream);

#endif

