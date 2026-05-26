#pragma once
#include <string>

// 自包含 SHA256（不依赖外部库）
std::string sha256(const std::string& input);
