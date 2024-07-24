#ifndef TRANSLATION_DATA_TYPE_HPP
#define TRANSLATION_DATA_TYPE_HPP

#include <cstdint>
#include <vector>

//! [request]
struct TranslationRequest
{
    // iox::cxx::string<1024> code;
    std::vector<uint64_t> data;
};
//! [request]

//! [response]
struct TranslationResponse
{
    std::vector<uint64_t> data;
};
//! [response]

#endif // TRANSLATION_DATA_TYPE_HPP
