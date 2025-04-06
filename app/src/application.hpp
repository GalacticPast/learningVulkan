#pragma once
#include "defines.hpp"
#include <string>

struct application_config
{
    std::string application_name;
};

struct application_state
{
    application_config *config;
};
