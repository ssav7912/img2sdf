//
// Created by Soren on 1/04/2024.
//

#include "jumpflooderror.h"

jumpflood_error::jumpflood_error(HRESULT hr, const std::string &message) :
std::runtime_error(message + std::format("HRESULT: {:x}\n", hr)){}
