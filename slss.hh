#pragma once

#include <cstdbool>
#include <string>

/// fancy assert
void attest(bool test, const char * epilogue, ...)
  __attribute__ ((format (printf, 2, 3)));

bool endswith(std::string const & str, std::string const & end);
