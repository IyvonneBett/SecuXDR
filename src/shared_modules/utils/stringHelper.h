/*
 * Wazuh shared modules utils
 * Copyright (C) 2015, Wazuh Inc.
 * June 11, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _STRING_HELPER_H
#define _STRING_HELPER_H

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <regex>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

namespace Utils
{
    static void ISO8859ToUTF8(std::string& data)
    {
        // Convert from ISO-8859-1 to UTF-8
        std::string strOut;
        // 0xc0 is 11000000 in binary, used to mask the first 2 bits of the character of a 2-byte sequence
        constexpr auto UTF8_2BYTE_SEQ{ 0xc0 };
        // 6 is the number of bits to shift the character to the right
        constexpr auto UTF8_2BYTE_SEQ_VALUE_LEN{ 6 };
        // 0x80 is 10000000 in binary, is the first code point of a 2-byte sequence
        constexpr auto UTF8_2BYTE_FIRST_CODE_VALUE{ 0x80 };
        // 0x3f is 00111111 in binary, used to mask the last 6 bits of the character of a 2-byte sequence
        constexpr auto UTF8_2BYTE_MASK{ 0x3f };

        for (auto it = data.begin(); it != data.end(); ++it)
        {
            const uint8_t ch = *it;

            // ASCII character
            if (ch < UTF8_2BYTE_FIRST_CODE_VALUE)
            {
                strOut.push_back(ch);
            }
            // Extended ASCII
            else
            {
                // 2-byte sequence
                // 110xxxxx
                strOut.push_back(UTF8_2BYTE_SEQ | ch >> UTF8_2BYTE_SEQ_VALUE_LEN);
                // 10xxxxxx
                strOut.push_back(UTF8_2BYTE_FIRST_CODE_VALUE | (ch & UTF8_2BYTE_MASK));
            }
        }

        data = strOut;
    }

    static bool replaceAll(std::string& data,
                           const std::string& toSearch,
                           const std::string& toReplace)
    {
        auto pos { data.find(toSearch) };
        const auto ret{ std::string::npos != pos };

        while (std::string::npos != pos)
        {
            data.replace(pos, toSearch.size(), toReplace);
            pos = data.find(toSearch, pos);
        }

        return ret;
    }

    static bool replaceFirst(std::string& data,
                             const std::string& toSearch,
                             const std::string& toReplace)
    {
        auto pos { data.find(toSearch) };
        auto ret { false };

        if (std::string::npos != pos)
        {
            data.replace(pos, toSearch.size(), toReplace);
            ret = true;
        }

        return ret;
    }

    static std::vector<std::string> split(const std::string& str,
                                          const char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream{ str };

        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    static std::string splitIndex(const std::string& str,
                                  const char delimiter,
                                  const size_t index)
    {
        std::string retVal;
        const auto& splitResult { split(str, delimiter) };

        if (index < splitResult.size())
        {
            retVal = splitResult.at(index);
        }
        else
        {
            throw std::runtime_error("Invalid index to get values.");
        }

        return retVal;
    }

    static std::vector<std::string> splitNullTerminatedStrings(const char* buffer)
    {
        constexpr auto NULL_TERMINATED_DELIMITER {'\0'};
        std::vector<std::string> ret;

        while (buffer[0] != NULL_TERMINATED_DELIMITER)
        {
            const std::string token(buffer);

            if (!token.empty())
            {
                ret.push_back(token);
            }

            buffer += token.size() + 1;
        }

        return ret;
    }

    static std::string asciiToHex(const std::vector<unsigned char>& asciiData)
    {
        std::string ret;
        std::stringstream ss;

        for (const auto& val : asciiData)
        {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(val);
        }

        if (ss.good())
        {
            ret = ss.str();
        }
        // LCOV_EXCL_START
        else
        {
            const auto size{asciiData.size() * 2};
            const auto buffer{std::make_unique<char[]>(size + 1)};
            char* output{buffer.get()};

            for (const auto& value : asciiData)
            {
                snprintf(output, 3, "%02x", value);
                output += 2;
            }

            ret = std::string{buffer.get(), size};
        }

        // LCOV_EXCL_STOP
        return ret;
    }

    static std::string leftTrim(const std::string& str, const std::string& args = " ")
    {
        const auto pos{ str.find_first_not_of(args) };

        if (pos != std::string::npos)
        {
            return str.substr(pos);
        }

        return str;
    }

    static std::string rightTrim(const std::string& str, const std::string& args = " ")
    {
        const auto pos{ str.find_last_not_of(args) };

        if (pos != std::string::npos)
        {
            return str.substr(0, pos + 1);
        }

        return str;
    }

    static std::string trim(const std::string& str, const std::string& args = " ")
    {
        return leftTrim(rightTrim(str, args), args);
    }

    static std::string toUpperCase(const std::string& str)
    {
        std::string temp{ str };
        std::transform(std::begin(temp),
                       std::end(temp),
                       std::begin(temp),
                       [](std::string::value_type character)
        {
            return std::toupper(character);
        });
        return temp;
    }

    static bool startsWith(const std::string& str, const std::string& start)
    {
        if (!str.empty() && str.length() >= start.length())
        {
            return str.compare(0, start.length(), start) == 0;
        }

        return false;
    }

    static bool endsWith(const std::string& str, const std::string& ending)
    {
        if (!str.empty() && str.length() >= ending.length())
        {
            const auto endLength{ ending.length()};
            const auto token{ str.substr(str.length() - endLength, endLength) };
            return token == ending;
        }

        return false;
    }

    static std::string substrOnFirstOccurrence(const std::string& str, const std::string& args = " ")
    {
        const auto pos{ str.find(args) };

        if (pos != std::string::npos)
        {
            return str.substr(0, pos);
        }

        return str;
    }

    static std::pair<std::string, std::string> splitKeyValueNonEscapedDelimiter(const std::string& str,
                                                                                const char delimiter,
                                                                                const char escapeChar)
    {
        std::pair<std::string, std::string> retVal { std::make_pair(str, "") };
        const auto findText { std::string{escapeChar} + std::string{delimiter} };
        auto found { str.find_first_of(findText) };
        constexpr auto DELIMITER_ESCAPE_LENGTH { 2 };

        while (std::string::npos != found)
        {
            if (str.at(found) == delimiter)
            {
                retVal = std::make_pair(str.substr(0, found), str.substr(found + 1));
                break;
            }

            found = str.find_first_of(findText, found + DELIMITER_ESCAPE_LENGTH);
        }

        return retVal;
    }

    static bool findRegexInString(const std::string& in,
                                  std::string& match,
                                  const std::regex& pattern,
                                  const size_t matchIndex = 0,
                                  const std::string& start = "")
    {
        bool ret{false};

        if (start.empty() || startsWith(in, start))
        {
            std::smatch sm;
            ret = std::regex_search(in, sm, pattern);

            if (ret && sm.size() >= matchIndex)
            {
                match = sm[matchIndex];
            }
        }

        return ret;
    }
}

#pragma GCC diagnostic pop

#endif // _STRING_HELPER_H
