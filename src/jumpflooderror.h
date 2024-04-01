//
// Created by Soren on 1/04/2024.
//

#ifndef IMG2SDF_JUMPFLOODERROR_H
#define IMG2SDF_JUMPFLOODERROR_H

#include <stdexcept>
#include <format>
#include <winerror.h>

struct jumpflood_error : public std::runtime_error
{
    explicit jumpflood_error(HRESULT hr, const std::string& message = "");
};

#endif //IMG2SDF_JUMPFLOODERROR_H
