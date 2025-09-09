#pragma once

/// fancy assert
void attest(bool test, const char * epilogue, ...)
  __attribute__ ((format (printf, 2, 3)));
