#pragma once

#include <cstdint>
#include <string>

void CreateParity(const uint8_t numData,
                  const uint8_t numParity,
                  const std::string & stub);

void RecoverData(const std::string & stub);
