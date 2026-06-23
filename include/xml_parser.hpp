// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <boost/parser/parser.hpp>

#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace xml
{

// A single name="value" attribute on an element.
struct Attribute
{
    std::string name;
    std::string value;

    bool operator==(const Attribute&) const = default;
};

// A node in the parsed XML tree.  Only elements, attributes and text are
// modeled; the prolog, processing instructions, comments and the DOCTYPE
// declaration are skipped while parsing.  This is intentionally a "basic"
// parser, not a conforming one: it does not handle entity references, CDATA
// sections, namespaces, or DTD-defined content.
struct Element
{
    std::string name;
    std::vector<Attribute> attributes;
    std::vector<Element> children;
    std::string text;

    bool operator==(const Element&) const = default;

    // Returns the value of the named attribute, or nullptr if absent (mirrors
    const std::string* attribute(std::string_view attrName) const
    {
        for (const Attribute& attr : attributes)
        {
            if (attr.name == attrName)
            {
                return &attr.value;
            }
        }
        return nullptr;
    }

    // A lazy view over the child elements whose tag name equals childName.
    // Replaces tinyxml2's FirstChildElement(name)/NextSiblingElement(name)
    // iteration.
    auto childrenNamed(std::string_view childName) const
    {
        return children | std::views::filter([childName](const Element& child) {
                   return child.name == childName;
               });
    }
};

namespace grammar
{
namespace bp = boost::parser;

// Semantic actions used to assemble the tree by hand.  Attribute auto-
// propagation can't express XML's context sensitivity (matching close tags),
// so every piece is placed explicitly into the rule's value.
inline constexpr auto setName = [](auto& ctx) {
    bp::_val(ctx).name = std::move(bp::_attr(ctx));
};
inline constexpr auto addAttribute = [](auto& ctx) {
    bp::_val(ctx).attributes.push_back(std::move(bp::_attr(ctx)));
};
inline constexpr auto addChild = [](auto& ctx) {
    bp::_val(ctx).children.push_back(std::move(bp::_attr(ctx)));
};
inline constexpr auto addText = [](auto& ctx) {
    bp::_val(ctx).text.append(bp::_attr(ctx));
};
// Enforces well-formedness: the close tag name must equal the open tag name.
// Setting _pass to false makes the surrounding rule fail to match.
inline constexpr auto matchClose = [](auto& ctx) {
    bp::_pass(ctx) = (bp::_attr(ctx) == bp::_val(ctx).name);
};

// Name ::= NameStartChar NameChar*  (a pragmatic subset of the XML spec).
// Note: boost::parser::char_("abc") matches the literal set {a,b,c}; it does
// not treat "a-z" as a range the way Spirit does, so ranges are spelled out
// with the two-argument char_('a', 'z') form.
inline const auto nameStart =
    bp::char_('a', 'z') | bp::char_('A', 'Z') | bp::char_("_:");
inline const auto nameChar = nameStart | bp::char_('0', '9') | bp::char_(".-");
inline const bp::rule<struct NameTag, std::string> name = "xml name";
inline const auto name_def = bp::lexeme[nameStart >> *nameChar];

// Attribute ::= Name '=' ('"' chars '"' | "'" chars "'").
inline const bp::rule<struct AttributeTag, Attribute> attribute = "attribute";
inline const auto quoted =
    ('"' >> *(bp::char_ - '"') >> '"') | ('\'' >> *(bp::char_ - '\'') >> '\'');
inline const auto attribute_def = name >> '=' >> quoted;

// Skippable, non-content constructs: comments, the prolog / processing
// instructions, and a DOCTYPE declaration (D-Bus introspection XML starts
// with one).  "p - lit(s)" matches a char only where s does not start.
inline const auto comment = bp::lit("<!--") >>
                            *(bp::char_ - bp::lit("-->")) >> bp::lit("-->");
inline const auto piOrDecl = bp::lit("<?") >>
                             *(bp::char_ - bp::lit("?>")) >> bp::lit("?>");
// A basic DOCTYPE without an internal subset: everything up to the first '>'.
inline const auto doctype = bp::lit("<!DOCTYPE") >> *(bp::char_ - '>') >> '>';
// omit[] discards the matched text so misc contributes no attribute.
inline const auto misc = bp::omit[comment | piOrDecl | doctype];

// Element ::= '<' Name Attribute* ('/>' | '>' Content '</' Name '>').
inline const bp::rule<struct ElementTag, Element> element = "element";
// childElement just forwards to element.  A rule that references *itself*
// directly yields a nope attribute (Boost.Parser breaks the type recursion
// that way), so reading _attr in addChild would fail.  Going through a
// distinct tag makes the recursion indirect, and the child's Element
// attribute propagates normally.
inline const bp::rule<struct ChildElementTag, Element> childElement = "element";
inline const auto childElement_def = element;

inline const auto text = bp::lexeme[+(bp::char_ - '<')];
inline const auto element_def =
    '<' >> name[setName] >> *(attribute[addAttribute]) >>
    (bp::lit("/>") | ('>' >> *(misc | childElement[addChild] | text[addText]) >>
                      bp::lit("</") >> name[matchClose] >> '>'));

BOOST_PARSER_DEFINE_RULES(name, attribute, element, childElement);

// Document ::= Misc* Element Misc*.  Kept as a plain parser expression (rather
// than a rule) so the single Element sub-attribute propagates through directly
// instead of being distributed across Element's members.
inline const auto document = *misc >> element >> *misc;
} // namespace grammar

// Parses a complete XML document into a tree of Elements.  Returns nullopt if
// the input is not well-formed (including mismatched close tags).  Surrounding
// whitespace, the prolog and comments are ignored.
inline std::optional<Element> parse(std::string_view input)
{
    // A default-constructed callback_error_handler has no callbacks, so a
    // failed parse quietly returns nullopt instead of writing to std::cerr.
    // with_error_handler binds the handler by reference, so it must outlive
    // the parse call below.
    boost::parser::callback_error_handler quiet;
    auto parser = boost::parser::with_error_handler(grammar::document, quiet);
    return boost::parser::parse(input, parser, boost::parser::ws);
}

} // namespace xml
