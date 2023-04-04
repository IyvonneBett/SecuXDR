/*
 * Wazuh shared modules utils
 * Copyright (C) 2015, Wazuh Inc.
 * July 14, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "stringHelper_test.h"
#include "stringHelper.h"
#include <sstream>

void StringUtilsTest::SetUp() {};

void StringUtilsTest::TearDown() {};

TEST_F(StringUtilsTest, CheckReplacement)
{
    std::string string_base { "hello_world" };
    const auto retVal { Utils::replaceAll(string_base, "hello_", "bye_") };
    EXPECT_EQ(string_base, "bye_world");
    EXPECT_TRUE(retVal);
}

TEST_F(StringUtilsTest, CheckNotReplacement)
{
    std::string string_base {"hello_world" };
    const auto retVal { Utils::replaceAll(string_base, "nothing_", "bye_") };
    EXPECT_EQ(string_base, "hello_world");
    EXPECT_FALSE(retVal);
}

TEST_F(StringUtilsTest, SplitEmptyString)
{
    auto splitTextVector { Utils::split("", '.') };
    EXPECT_EQ(0ull, splitTextVector.size());
}

TEST_F(StringUtilsTest, SplitDelimiterNoCoincidence)
{
    const auto splitTextVector { Utils::split("hello_world", '.') };
    EXPECT_EQ(1ull, splitTextVector.size());
}

TEST_F(StringUtilsTest, SplitDelimiterCoincidence)
{
    const auto splitTextVector { Utils::split("hello.world", '.') };
    EXPECT_EQ(2ull, splitTextVector.size());
    EXPECT_EQ(splitTextVector[0], "hello");
    EXPECT_EQ(splitTextVector[1], "world");
}

TEST_F(StringUtilsTest, SplitIndex)
{
    const auto splitTextVector { Utils::splitIndex("hello.world", '.', 0) };
    EXPECT_EQ(5ull, splitTextVector.size());
    EXPECT_EQ(splitTextVector, "hello");
}

TEST_F(StringUtilsTest, SplitIndexRuntimeError)
{
    EXPECT_THROW(Utils::splitIndex("hello.world", '.', 2), std::runtime_error);
}

TEST_F(StringUtilsTest, AsciiToHexString)
{
    const std::vector<unsigned char> data{0x2d, 0x53, 0x3b, 0x9d, 0x9f, 0x0f, 0x06, 0xef, 0x4e, 0x3c, 0x23, 0xfd, 0x49, 0x6c, 0xfe, 0xb2, 0x78, 0x0e, 0xda, 0x7f};
    const std::string expected { "2d533b9d9f0f06ef4e3c23fd496cfeb2780eda7f" };
    const auto result {Utils::asciiToHex(data)};
    EXPECT_EQ(expected, result);
}

TEST_F(StringUtilsTest, CheckFirstReplacement)
{
    std::string string_base { "bye_bye" };
    const auto retVal { Utils::replaceFirst(string_base, "bye", "hello") };
    EXPECT_EQ(string_base, "hello_bye");
    EXPECT_TRUE(retVal);
}

TEST_F(StringUtilsTest, CheckNotFirstReplacement)
{
    std::string string_base {"hello_world" };
    const auto retVal { Utils::replaceFirst(string_base, "nothing_", "bye_") };
    EXPECT_EQ(string_base, "hello_world");
    EXPECT_FALSE(retVal);
}

TEST_F(StringUtilsTest, RightTrim)
{
    EXPECT_EQ("Hello", Utils::rightTrim("Hello"));
    EXPECT_EQ("Hello", Utils::rightTrim("Hello "));
    EXPECT_EQ("Hello", Utils::rightTrim("Hello  "));
    EXPECT_EQ("Hello", Utils::rightTrim("Hello            "));
    EXPECT_EQ(" Hello", Utils::rightTrim(" Hello"));
    EXPECT_EQ("\tHello", Utils::rightTrim("\tHello\t", "\t"));
    EXPECT_EQ(" \t\nHello", Utils::rightTrim(" \t\nHello \t\n ", "\t\n "));
    EXPECT_EQ(" \t\nHello \t\n", Utils::rightTrim(" \t\nHello \t\n "));
    EXPECT_EQ("", Utils::rightTrim(""));
}

TEST_F(StringUtilsTest, LeftTrim)
{
    EXPECT_EQ("Hello", Utils::leftTrim("Hello"));
    EXPECT_EQ("Hello", Utils::leftTrim(" Hello"));
    EXPECT_EQ("Hello", Utils::leftTrim(" Hello"));
    EXPECT_EQ("Hello", Utils::leftTrim("          Hello"));
    EXPECT_EQ("Hello\t ", Utils::leftTrim(" \tHello\t ", " \t"));
    EXPECT_EQ("Hello\t\n ", Utils::leftTrim(" \t\nHello\t\n ", " \t\n"));
    EXPECT_EQ("\t\nHello\t\n ", Utils::leftTrim(" \t\nHello\t\n "));
    EXPECT_EQ("", Utils::leftTrim(""));
}

TEST_F(StringUtilsTest, Trim)
{
    EXPECT_EQ("Hello", Utils::trim("Hello"));
    EXPECT_EQ("Hello", Utils::trim(" Hello "));
    EXPECT_EQ("Hello", Utils::trim(" Hello "));
    EXPECT_EQ("Hello", Utils::trim("          Hello      "));
    EXPECT_EQ("Hello", Utils::trim(" \tHello\t ", " \t"));
    EXPECT_EQ("Hello", Utils::trim(" \t\nHello\t\n ", " \t\n"));
}

TEST_F(StringUtilsTest, ToUpper)
{
    EXPECT_EQ("", Utils::toUpperCase(""));
    EXPECT_EQ("HELLO WORLD", Utils::toUpperCase("HeLlO WoRlD"));
    EXPECT_EQ("123", Utils::toUpperCase("123"));
}

TEST_F(StringUtilsTest, StartsWith)
{
    const std::string start{"Package_"};
    const std::string item1{"Package_6_for_KB4565554~31bf3856ad364e35~amd64~~18362.957.1.3"};
    const std::string item2{"Package_5_for_KB4569073~31bf3856ad364e35~amd64~~18362.1012.1.1"};
    const std::string item3{"Microsoft-Windows-IIS-WebServer-AddOn-Package~31bf3856ad364e35~amd64~~10.0.18362.815"};
    const std::string item4{"Microsoft-Windows-HyperV-OptionalFeature-VirtualMachinePlatform-Package_31bf3856ad364e35~amd64~~10.0.18362.1139.mum"};
    EXPECT_TRUE(Utils::startsWith(start, start));
    EXPECT_TRUE(Utils::startsWith(item1, start));
    EXPECT_TRUE(Utils::startsWith(item2, start));
    EXPECT_FALSE(Utils::startsWith("", start));
    EXPECT_FALSE(Utils::startsWith(item3, start));
    EXPECT_FALSE(Utils::startsWith(item4, start));
}

TEST_F(StringUtilsTest, EndsWith)
{
    const std::string end{"_package"};
    const std::string item1{"KB4565554~31bf3856ad364e35~amd64~~18362.957.1.3_package"};
    const std::string item2{"KB4569073~31bf3856ad364e35~amd64~~18362.1012.1.1_package"};
    const std::string item3{"Microsoft-Windows-IIS-WebServer-AddOn-Package~31bf3856ad364e35~amd64~~10.0.18362.815"};
    const std::string item4{"Microsoft-Windows-HyperV-OptionalFeature-VirtualMachinePlatform-Package_31bf3856ad364e35~amd64~~10.0.18362.1139.mum"};
    EXPECT_TRUE(Utils::endsWith(end, end));
    EXPECT_TRUE(Utils::endsWith(item1, end));
    EXPECT_TRUE(Utils::endsWith(item2, end));
    EXPECT_FALSE(Utils::endsWith("", end));
    EXPECT_FALSE(Utils::endsWith(item3, end));
    EXPECT_FALSE(Utils::endsWith(item4, end));
}

TEST_F(StringUtilsTest, SplitDelimiterNullTerminated)
{
    const char buffer[] {'h', 'e', 'l', 'l', 'o', '\0', 'w', 'o', 'r', 'l', 'd', '\0', '\0'};
    const auto tokens{Utils::splitNullTerminatedStrings(buffer)};
    EXPECT_EQ(2ull, tokens.size());
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST_F(StringUtilsTest, CheckMultiReplacement)
{
    std::string string_base { "hello         world" };
    const auto retVal { Utils::replaceAll(string_base, "  ", " ") };
    EXPECT_EQ(string_base, "hello world");
    EXPECT_TRUE(retVal);
}

TEST_F(StringUtilsTest, substrOnFirstOccurrenceCorrect)
{
    EXPECT_EQ(Utils::substrOnFirstOccurrence("hello         world", "         "), "hello");
}

TEST_F(StringUtilsTest, substrOnFirstOccurrenceCorrectEmpty)
{
    EXPECT_EQ(Utils::substrOnFirstOccurrence("", " "), "");
}

TEST_F(StringUtilsTest, substrOnFirstOccurrenceNoOccurrences)
{
    EXPECT_EQ(Utils::substrOnFirstOccurrence("hello         world", "bye"), "hello         world");
}

TEST_F(StringUtilsTest, substrOnFirstOccurrenceCorrectEndText)
{
    EXPECT_EQ(Utils::substrOnFirstOccurrence("hello         world", "world"), "hello         ");
}

TEST_F(StringUtilsTest, substrOnFirstOccurrenceCorrectFirstText)
{
    EXPECT_EQ(Utils::substrOnFirstOccurrence("hello         world", "hello"), "");
}

TEST_F(StringUtilsTest, substrOnFirstOccurrenceCorrectEscapeCharacter)
{
    EXPECT_EQ(Utils::substrOnFirstOccurrence("hello\nworld", "\n"), "hello");
}

TEST_F(StringUtilsTest, substrOnFirstOccurrenceCorrectEscapeCharacterEmptyResult)
{
    EXPECT_EQ(Utils::substrOnFirstOccurrence("\n", "\n"), "");
}

TEST_F(StringUtilsTest, splitKeyValueNonEscapedSimple)
{
    std::string stringBase { "hello:world" };
    const auto retVal { Utils::splitKeyValueNonEscapedDelimiter(stringBase, ':', '\\') };
    EXPECT_EQ(retVal.first, "hello");
    EXPECT_EQ(retVal.second, "world");
}

TEST_F(StringUtilsTest, splitKeyValueNonEscapedSimpleEnd)
{
    std::string stringBase { "hello:" };
    const auto retVal { Utils::splitKeyValueNonEscapedDelimiter(stringBase, ':', '\\') };
    EXPECT_EQ(retVal.first, "hello");
    EXPECT_EQ(retVal.second, "");
}

TEST_F(StringUtilsTest, splitKeyValueNonEscapedSimpleDoubleDelimiterEnd)
{
    std::string stringBase { "hello:world:" };
    const auto retVal { Utils::splitKeyValueNonEscapedDelimiter(stringBase, ':', '\\') };
    EXPECT_EQ(retVal.first, "hello");
    EXPECT_EQ(retVal.second, "world:");
}

TEST_F(StringUtilsTest, splitKeyValueNonEscapedSimpleDoubleEnd)
{
    std::string stringBase { "hello::" };
    const auto retVal { Utils::splitKeyValueNonEscapedDelimiter(stringBase, ':', '\\') };
    EXPECT_EQ(retVal.first, "hello");
    EXPECT_EQ(retVal.second, ":");
}

TEST_F(StringUtilsTest, splitKeyValueNonEscapedSimpleEmptyDoubleEnd)
{
    std::string stringBase { "::" };
    const auto retVal { Utils::splitKeyValueNonEscapedDelimiter(stringBase, ':', '\\') };
    EXPECT_EQ(retVal.first, "");
    EXPECT_EQ(retVal.second, ":");
}

TEST_F(StringUtilsTest, splitKeyValueNonEscapedComplex)
{
    std::string stringBase { "he\\:llo:world" };
    const auto retVal { Utils::splitKeyValueNonEscapedDelimiter(stringBase, ':', '\\') };
    EXPECT_EQ(retVal.first, "he\\:llo");
    EXPECT_EQ(retVal.second, "world");
}

TEST_F(StringUtilsTest, splitKeyValueNonEscapedComplexEnd)
{
    std::string stringBase { "he\\:llo:" };
    const auto retVal { Utils::splitKeyValueNonEscapedDelimiter(stringBase, ':', '\\') };
    EXPECT_EQ(retVal.first, "he\\:llo");
    EXPECT_EQ(retVal.second, "");
}

TEST_F(StringUtilsTest, findRegexInStringNotStartWith)
{
    std::string matchedValue;
    const auto valueToCheck{"PREFIX Some random content"};
    const auto regex{std::regex(R"(PREFIX Some random content)")};
    EXPECT_FALSE(Utils::findRegexInString(valueToCheck, matchedValue, regex, 0, "OTHERPREFIX"));
    EXPECT_TRUE(matchedValue.empty());
}

TEST_F(StringUtilsTest, findRegexInStringStartWith)
{
    std::string matchedValue;
    const auto valueToCheck{"PREFIX Some random content"};
    const auto regex{std::regex(R"(PREFIX Some random content)")};
    EXPECT_TRUE(Utils::findRegexInString(valueToCheck, matchedValue, regex, 0, "PREFIX"));
    EXPECT_EQ(matchedValue, valueToCheck);
}

TEST_F(StringUtilsTest, findRegexInStringMatchingRegexWithoutGroup)
{
    std::string matchedValue;
    const auto valueToCheck{"This string should not be extracted"};
    const auto regex{std::regex(R"(^This string should not be extracted$)")};
    EXPECT_TRUE(Utils::findRegexInString(valueToCheck, matchedValue, regex));
    EXPECT_EQ(matchedValue, valueToCheck);
}

TEST_F(StringUtilsTest, findRegexInStringNoExtractingFirstGroup)
{
    std::string matchedValue;
    const auto valueToCheck{"This string should be extracted"};
    const auto regex{std::regex(R"(^This (\S+) should be (\S+)$)")};
    EXPECT_TRUE(Utils::findRegexInString(valueToCheck, matchedValue, regex));
    EXPECT_EQ(matchedValue, valueToCheck);
}

TEST_F(StringUtilsTest, findRegexInStringExtractingFirstGroup)
{
    std::string matchedValue;
    const auto valueToCheck{"This string should be extracted"};
    const auto regex{std::regex(R"(^This (\S+) should be (\S+)$)")};
    EXPECT_TRUE(Utils::findRegexInString(valueToCheck, matchedValue, regex, 1));
    EXPECT_EQ(matchedValue, "string");
}

TEST_F(StringUtilsTest, findRegexInStringExtractingSecondGroup)
{
    std::string matchedValue;
    const auto valueToCheck{"This string should be extracted"};
    const auto regex{std::regex(R"(^This (\S+) should be (\S+)$)")};
    EXPECT_TRUE(Utils::findRegexInString(valueToCheck, matchedValue, regex, 2));
    EXPECT_EQ(matchedValue, "extracted");
}

TEST_F(StringUtilsTest, convertToUTF8NoChanges)
{
    std::string noUnicodeString{"This is a test"};
    Utils::ISO8859ToUTF8(noUnicodeString);
    EXPECT_EQ("This is a test", noUnicodeString);
}

TEST_F(StringUtilsTest, rawUnicodeToUTF8)
{
    std::stringstream fileContent;
    // Set buffer in ISO8859-1
    fileContent <<
                R"(CLASSES=none)"
                R"(BASEDIR=/opt/csw)"
                R"(INSTDATE=Jan 09 2023 14:35)"
                R"(PKGSAV=/var/sadm/pkg/CSWschilybase/save)"
                R"(PKGINST=CSWschilybase)"
                R"(PSTAMP=joerg@unstable9x-20130619141117)"
                R"(EMAIL=joerg@opencsw.org)"
                R"(HOTLINE=http://www.opencsw.org/bugtrack/)"
                R"(VENDOR=http://cdrecord.berlios.de/old/private/  packaged for CSW by J)" <<
                "\xF6" <<
                R"(rg Schilling)"
                R"(CATEGORY=application)"
                R"(NAME=schilybase - A collection of common files from J. Schilling)"
                R"(PKG=CSWschilybase)"
                R"(VERSION=1.01,REV=2013.06.19)"
                R"(ARCH=i386)"
                R"(OAMBASE=/usr/sadm/sysadm)"
                R"(PATH=/sbin:/usr/sbin:/usr/bin:/usr/sadm/install/bin)"
                R"(TZ=localtime)"
                R"(LANG=C)"
                R"(LC_ALL=)"
                R"(LC_MONETARY=)"
                R"(LC_MESSAGES=)"
                R"(LC_COLLATE=)"
                R"(LC_TIME=)"
                R"(LC_NUMERIC=)"
                R"(LC_CTYPE=)";

    std::string content;

    while (fileContent.good())
    {
        std::string line;
        std::getline(fileContent, line);
        // Convert 'line' to UTF-8
        Utils::ISO8859ToUTF8(line);

        content += line;
    }

    EXPECT_EQ("CLASSES=none"
              "BASEDIR=/opt/csw"
              "INSTDATE=Jan 09 2023 14:35"
              "PKGSAV=/var/sadm/pkg/CSWschilybase/save"
              "PKGINST=CSWschilybase"
              "PSTAMP=joerg@unstable9x-20130619141117"
              "EMAIL=joerg@opencsw.org"
              "HOTLINE=http://www.opencsw.org/bugtrack/VENDOR=http://cdrecord.berlios.de/old/private/  packaged for CSW"
              " by J\xC3\xB6rg Schilling"
              "CATEGORY=applicationNAME=schilybase - A collection of common files from J. SchillingPKG=CSWschilybase"
              "VERSION=1.01,REV=2013.06.19"
              "ARCH=i386"
              "OAMBASE=/usr/sadm/sysadm"
              "PATH=/sbin:/usr/sbin:/usr/bin:/usr/sadm/install/bin"
              "TZ=localtime"
              "LANG=C"
              "LC_ALL="
              "LC_MONETARY="
              "LC_MESSAGES="
              "LC_COLLATE="
              "LC_TIME="
              "LC_NUMERIC="
              "LC_CTYPE=",
              content);
}
