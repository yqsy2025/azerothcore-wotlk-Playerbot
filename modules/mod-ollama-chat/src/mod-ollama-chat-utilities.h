#ifndef MOD_OLLAMA_CHAT_UTILS_H
#define MOD_OLLAMA_CHAT_UTILS_H

#include <string>
#include "Log.h"
#include <vector>
#include <sstream>
#include <utility>
#include <type_traits>
#include <iomanip>
#include <locale>

// Safe formatting utility for the Ollama Chat module.
// This will catch all fmt::format errors and log them.
struct NamedFormatArg
{
    std::string key;
    std::string value;
};

inline void ReplaceAllInPlace(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
        return;

    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
}

inline void ReplaceNamedInPlace(std::string& str, const std::string& key, const std::string& value)
{
    ReplaceAllInPlace(str, "{" + key + "}", value);

    const std::string prefix = "{" + key + ":";
    size_t pos = 0;
    while ((pos = str.find(prefix, pos)) != std::string::npos)
    {
        size_t end = str.find('}', pos + prefix.size());
        if (end == std::string::npos)
            break;

        str.replace(pos, end - pos + 1, value);
        pos += value.size();
    }
}

template <typename T>
inline std::string ToString(const T& value)
{
    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    ss << value;
    return ss.str();
}

inline std::string ToString(const std::string& value) { return value; }
inline std::string ToString(const char* value) { return value ? std::string(value) : std::string(); }

template <typename T>
inline NamedFormatArg Arg(const char* key, const T& value)
{
    return NamedFormatArg{key ? std::string(key) : std::string(), ToString(value)};
}

inline std::string FormatFixed(double value, int decimals)
{
    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(decimals) << value;
    return ss.str();
}

template<typename... Args>
inline std::string SafeFormat(const std::string& templ, Args&&... args)
{
    try
    {
        std::string result = templ;

        ReplaceAllInPlace(result, "{{", "\x01");
        ReplaceAllInPlace(result, "}}", "\x02");

        std::vector<std::string> positionalArgs;
        positionalArgs.reserve(sizeof...(Args));

        auto processArg = [&](auto&& arg)
        {
            using ArgT = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<ArgT, NamedFormatArg>)
            {
                ReplaceNamedInPlace(result, arg.key, arg.value);
            }
            else
            {
                positionalArgs.push_back(ToString(arg));
            }
        };

        (processArg(std::forward<Args>(args)), ...);

        for (const auto& value : positionalArgs)
        {
            size_t pos = result.find("{}");
            if (pos == std::string::npos)
            {
                LOG_ERROR("server.loading", "[Ollama Chat] Format error: too many positional args | Template: {}", templ);
                break;
            }
            result.replace(pos, 2, value);
        }

        ReplaceAllInPlace(result, "\x01", "{");
        ReplaceAllInPlace(result, "\x02", "}");

        return result;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("server.loading", "[Ollama Chat] Format error: {} | Template: {}", e.what(), templ);
        return "[Format Error]";
    }
}

inline std::vector<std::string> SplitString(const std::string& str, char delim)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim))
    {
        // Trim whitespace from token
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
            tokens.push_back(token.substr(start, end - start + 1));
    }
    return tokens;
}

// Sanitize a string to be valid UTF-8 by removing or replacing invalid bytes
inline std::string SanitizeUTF8(const std::string& str)
{
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); )
    {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        // Single-byte character (0xxxxxxx)
        if (c <= 0x7F)
        {
            result.push_back(str[i]);
            i++;
        }
        // Two-byte character (110xxxxx 10xxxxxx)
        else if ((c & 0xE0) == 0xC0)
        {
            if (i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80)
            {
                result.push_back(str[i]);
                result.push_back(str[i + 1]);
                i += 2;
            }
            else
            {
                // Invalid sequence, replace with space
                result.push_back(' ');
                i++;
            }
        }
        // Three-byte character (1110xxxx 10xxxxxx 10xxxxxx)
        else if ((c & 0xF0) == 0xE0)
        {
            if (i + 2 < str.size() &&
                (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(str[i + 2]) & 0xC0) == 0x80)
            {
                result.push_back(str[i]);
                result.push_back(str[i + 1]);
                result.push_back(str[i + 2]);
                i += 3;
            }
            else
            {
                // Invalid sequence, replace with space
                result.push_back(' ');
                i++;
            }
        }
        // Four-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        else if ((c & 0xF8) == 0xF0)
        {
            if (i + 3 < str.size() &&
                (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(str[i + 2]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(str[i + 3]) & 0xC0) == 0x80)
            {
                result.push_back(str[i]);
                result.push_back(str[i + 1]);
                result.push_back(str[i + 2]);
                result.push_back(str[i + 3]);
                i += 4;
            }
            else
            {
                // Invalid sequence, replace with space
                result.push_back(' ');
                i++;
            }
        }
        else
        {
            // Invalid UTF-8 start byte, replace with space
            result.push_back(' ');
            i++;
        }
    }
    
    return result;
}

#endif // MOD_OLLAMA_CHAT_UTILS_H
