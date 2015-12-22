#include "http_parser.h"
#include <cassert>
#include <algorithm>

bool http_is_whitespace(char c)
{
    return c == ' ' || c == '\t';
}

bool http_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

http_request_method parse_request_method(sub_string& text)
{
    if (text.empty())
        throw http_parsing_error("empty request-line");

    if (text.size() < 3)
        throw http_parsing_error("invalid request method");

    switch (text[0])
    {
    case 'O':
        if (!text.try_drop_prefix(sub_string::literal("OPTIONS")))
            throw http_parsing_error("invalid request method");
        return http_request_method::OPTIONS;
    case 'G':
        if (!text.try_drop_prefix(sub_string::literal("GET")))
            throw http_parsing_error("invalid request method");
        return http_request_method::GET;
    case 'H':
        if (!text.try_drop_prefix(sub_string::literal("HEAD")))
            throw http_parsing_error("invalid request method");
        return http_request_method::HEAD;
    case 'P':
        switch (text[1])
        {
        case 'U':
            if (!text.try_drop_prefix(sub_string::literal("PUT")))
                throw http_parsing_error("invalid request method");
            return http_request_method::PUT;
        case 'O':
            if (!text.try_drop_prefix(sub_string::literal("POST")))
                throw http_parsing_error("invalid request method");
            return http_request_method::POST;
        default:
            throw http_parsing_error("invalid request method");
        }
    case 'D':
        if (!text.try_drop_prefix(sub_string::literal("DELETE")))
            throw http_parsing_error("invalid request method");
        return http_request_method::DELETE;
    case 'T':
        if (!text.try_drop_prefix(sub_string::literal("TRACE")))
            throw http_parsing_error("invalid request method");
        return http_request_method::TRACE;
    case 'C':
        if (!text.try_drop_prefix(sub_string::literal("CONNECT")))
            throw http_parsing_error("invalid request method");
        return http_request_method::CONNECT;
    default:
        throw http_parsing_error("invalid request method");
    }
}

http_version parse_version(sub_string& text)
{
    // rfc2616 [3.1]
    // HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT

    // Note that the major and minor numbers MUST be treated as separate
    // integers and that each MAY be incremented higher than a single digit.
    // Thus, HTTP/2.4 is a lower version than HTTP/2.13, which in turn is
    // lower than HTTP/12.3. Leading zeros MUST be ignored by recipients and
    // MUST NOT be sent.

    if (!text.try_drop_prefix(sub_string::literal("HTTP/")))
        throw http_parsing_error("invalid protocol version");

    while (text.try_drop_prefix(sub_string::literal("0")));

    if (!text.try_drop_prefix(sub_string::literal("1.")))
        throw http_parsing_error("invalid protocol version");

    if (text.try_drop_prefix(sub_string::literal("0")))
    {
        while (text.try_drop_prefix(sub_string::literal("0")));
        
        if (text.try_drop_prefix(sub_string::literal("1")))
            return http_version::HTTP_11;

        return http_version::HTTP_10;
    }
    else if (text.try_drop_prefix(sub_string::literal("1")))
    {
        return http_version::HTTP_11;
    }
    else
        throw http_parsing_error("invalid protocol version");
}

http_request_line parse_request_line(sub_string& text)
{
    // rfc2616 [4.1]
    // In the interest of robustness, servers SHOULD ignore any empty line(s)
    // received where a Request-Line is expected. In other words, if the
    // server is reading the protocol stream at the beginning of a message and
    // receives a CRLF first, it should ignore the CRLF.

    while (text.try_drop_prefix(sub_string::literal("\r\n")));

    // rfc2616 [5.1]
    // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    http_request_line result;
    result.method = parse_request_method(text);

    if (!text.try_drop_prefix(sub_string::literal(" ")))
        throw http_parsing_error("invalid request line");

    auto i = std::find(text.begin(), text.end(), ' ');
    if (i == text.end())
        throw http_parsing_error("invalid request line");

    result.uri = sub_string{text.begin(), i};
    ++i;

    text.begin(i);

    result.version = parse_version(text);

    if (!text.try_drop_prefix(sub_string::literal("\r\n")))
        throw http_parsing_error("invalid request line");

    return result;
}

unsigned parse_status_code(sub_string& text)
{
    if (text.size() < 3)
        throw http_parsing_error("invalid status code");

    if (!http_is_digit(text[0])
     || !http_is_digit(text[1])
     || !http_is_digit(text[2]))
        throw http_parsing_error("invalid status code");

    unsigned result = (text[0] - '0') * 100
                    + (text[1] - '0') * 10
                    + (text[2] - '0');

    text.advance(3);

    return result;
}

http_status_line parse_status_line(sub_string& text)
{
    // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    http_status_line result;

    result.version = parse_version(text);

    if (!text.try_drop_prefix(sub_string::literal(" ")))
        throw http_parsing_error("invalid status line");

    result.status_code = parse_status_code(text);

    if (!text.try_drop_prefix(sub_string::literal(" ")))
        throw http_parsing_error("invalid status line");

    auto i = std::find(text.begin(), text.end(), '\r');
    if (i == text.end())
        throw http_parsing_error("invalid status line");

    result.reason_phrase = sub_string{text.begin(), i};

    text.begin(i);

    if (!text.try_drop_prefix(sub_string::literal("\r\n")))
        throw http_parsing_error("invalid status line");

    return result;
}

sub_string parse_token(sub_string& text)
{
    // token          = 1*<any CHAR except CTLs or separators>
    // separators     = "(" | ")" | "<" | ">" | "@"
    //                | "," | ";" | ":" | "\" | <">
    //                | "/" | "[" | "]" | "?" | "="
    //                | "{" | "}" | SP | HT
    // CTL            = <any US-ASCII control character
    //                  (octets 0 - 31) and DEL (127)>
    // HT             = <US-ASCII HT, horizontal-tab (9)>

    char const* start = text.begin();

    for (;;)
    {
        if (text.empty())
            break;
        char c = text[0];
        if (c <= ' '
         || c == '(' || c == ')' || c == '<' || c == '>' || c == '@'
         || c == ',' || c == ';' || c == ':' || c == '\\' || c == '"'
         || c == '/' || c == '[' || c == ']' || c == '?' || c == '='
         || c == '{' || c == '}' || c == '\x7f')
            break;

        text.advance(1);
    }

    if (start == text.begin())
        throw http_parsing_error("invalid token");

    return sub_string{start, text.begin()};
}

void skip_leading_whitespace(sub_string& text)
{
    for (;;)
    {
        if (text.empty())
            break;

        char c = text[0];
        if (!http_is_whitespace(c))
            break;

        text.advance(1);
    }
}

void parse_field_content(sub_string& text, std::string& target)
{
    char const* i = text.begin();
    char const* non_ws = i;
    for (;;)
    {
        if (i == text.end())
            break;

        char c = *i;
        if (c == '\r')
            break;

        ++i;

        if (!http_is_whitespace(c))
        {
            target.append(non_ws, i);
            non_ws = i;
        }
    }
    text.begin(i);
}

void parse_headers(sub_string& text, std::map<std::string, std::vector<std::string>>& headers)
{
        // message-header = field-name ":" [ field-value ]
    // field-name     = token
    // field-value    = *( field-content | LWS )
    // field-content  = <the OCTETs making up the field-value
    //                  and consisting of either *TEXT or combinations
    //                  of token, separators, and quoted-string>
    // LWS            = [CRLF] 1*( SP | HT )

    // The field-content does not include any leading or trailing LWS: linear
    // white space occurring before the first non-whitespace character of the
    // field-value or after the last non-whitespace character of the
    // field-value. Such leading or trailing LWS MAY be removed without
    // changing the semantics of the field value. Any LWS that occurs between
    // field-content MAY be replaced with a single SP before interpreting the
    // field value or forwarding the message downstream.

    for (;;)
    {
        if (text.try_drop_prefix(sub_string::literal("\r\n")))
            break;

        std::string field = parse_token(text).as_string();

        if (!text.try_drop_prefix(sub_string::literal(":")))
            throw http_parsing_error("invalid header");

        std::string value;

    parse_value:
        skip_leading_whitespace(text);

        parse_field_content(text, value);

        if (!text.try_drop_prefix(sub_string::literal("\r\n")))
            throw http_parsing_error("invalid header");

        if (!text.empty() && http_is_whitespace(text[0]))
        {
            while (!text.empty() && http_is_whitespace(text[0]))
                text.advance(1);

            value.append(1, ' ');
            goto parse_value;
        }

        headers[std::move(field)].emplace_back(std::move(value));
    }
}

http_request parse_request(sub_string& text)
{
    http_request result;

    result.request_line = parse_request_line(text);
    parse_headers(text, result.headers);

    return result;
}

http_response parse_response(sub_string& text)
{
    http_response result;

    result.status_line = parse_status_line(text);
    parse_headers(text, result.headers);

    return result;
}
