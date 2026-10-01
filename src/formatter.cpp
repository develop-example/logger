#include "formatter.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace logger::detail
{

namespace
{

enum class TokenKind
{
    kLiteral,
    kDateTime,
    kLevel,
    kThread,
    kName,
    kContext,
    kFile,
    kLine,
    kFunction,
    kMessage,
};

struct Token
{
    TokenKind kind{TokenKind::kLiteral};
    std::string value;
};

std::tm localTime(std::time_t time)
{
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &time);
#else
    localtime_r(&time, &result);
#endif
    return result;
}

std::string dateTime(const LogRecord& record)
{
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(record.timestamp);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(record.timestamp - seconds).count();
    const std::time_t time = std::chrono::system_clock::to_time_t(record.timestamp);
    const std::tm calendar = localTime(time);

    std::ostringstream output;
    output << std::put_time(&calendar, "%Y-%m-%d %H:%M:%S") << '.'
           << std::setfill('0') << std::setw(3) << milliseconds;
    return output.str();
}

std::string escapedText(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case ']':
                escaped += "\\]";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }
    return escaped;
}

std::string contextText(const LogRecord& record)
{
    std::ostringstream output;
    bool first = true;
    for (const auto& field : record.context)
    {
        if (!first)
        {
            output << ' ';
        }
        output << '[' << field.key << '=' << escapedText(field.value) << ']';
        first = false;
    }
    return output.str();
}

TokenKind tokenKind(std::string_view name)
{
    if (name == "datetime") return TokenKind::kDateTime;
    if (name == "level") return TokenKind::kLevel;
    if (name == "thread") return TokenKind::kThread;
    if (name == "name") return TokenKind::kName;
    if (name == "context") return TokenKind::kContext;
    if (name == "file") return TokenKind::kFile;
    if (name == "line") return TokenKind::kLine;
    if (name == "function") return TokenKind::kFunction;
    if (name == "message") return TokenKind::kMessage;
    throw std::invalid_argument("unknown logger pattern token '%" + std::string(name) + "'");
}

std::vector<Token> parsePattern(const std::string& pattern)
{
    std::vector<Token> tokens;
    std::string literal;
    for (std::size_t index = 0; index < pattern.size(); ++index)
    {
        if (pattern[index] != '%')
        {
            literal.push_back(pattern[index]);
            continue;
        }

        if (!literal.empty())
        {
            tokens.push_back(Token{TokenKind::kLiteral, std::move(literal)});
            literal.clear();
        }
        if (index + 1 >= pattern.size())
        {
            throw std::invalid_argument("logger.pattern ends with an incomplete token");
        }
        if (pattern[index + 1] == '%')
        {
            tokens.push_back(Token{TokenKind::kLiteral, "%"});
            ++index;
            continue;
        }

        const std::size_t start = index + 1;
        std::size_t end = start;
        while (end < pattern.size() &&
               ((pattern[end] >= 'a' && pattern[end] <= 'z') ||
                (pattern[end] >= 'A' && pattern[end] <= 'Z')))
        {
            ++end;
        }
        if (end == start)
        {
            throw std::invalid_argument("logger.pattern contains an invalid token");
        }
        tokens.push_back(Token{tokenKind(std::string_view(pattern).substr(start, end - start)), {}});
        index = end - 1;
    }
    if (!literal.empty())
    {
        tokens.push_back(Token{TokenKind::kLiteral, std::move(literal)});
    }
    return tokens;
}

std::string tokenValue(TokenKind kind, const LogRecord& record)
{
    switch (kind)
    {
        case TokenKind::kDateTime:
            return dateTime(record);
        case TokenKind::kLevel:
            return toString(record.level);
        case TokenKind::kThread:
        {
            std::ostringstream output;
            output << record.thread_id;
            return output.str();
        }
        case TokenKind::kName:
            return record.name;
        case TokenKind::kContext:
            return contextText(record);
        case TokenKind::kFile:
            return record.file;
        case TokenKind::kLine:
            return std::to_string(record.line);
        case TokenKind::kFunction:
            return record.function;
        case TokenKind::kMessage:
            return record.message;
        case TokenKind::kLiteral:
            break;
    }
    return {};
}

class TextFormatter final : public ILogFormatter
{
public:
    explicit TextFormatter(const FormatterConfig& config)
        : use_pattern_(config.pattern_configured),
          tokens_(use_pattern_ ? parsePattern(config.pattern) : std::vector<Token>{})
    {
    }

    std::string format(const LogRecord& record) const override
    {
        if (use_pattern_)
        {
            std::ostringstream output;
            for (const auto& token : tokens_)
            {
                output << (token.kind == TokenKind::kLiteral ? token.value
                                                              : tokenValue(token.kind, record));
            }
            return output.str();
        }

        std::ostringstream output;
        output << dateTime(record) << " [" << toString(record.level) << "]"
               << " [tid=" << record.thread_id << ']';
        if (!record.name.empty())
        {
            output << " [" << record.name << ']';
        }
        const std::string context = contextText(record);
        if (!context.empty())
        {
            output << ' ' << context;
        }
        if (!record.file.empty())
        {
            output << " [" << record.file << ':' << record.line << ']';
        }
        if (!record.function.empty())
        {
            output << " [" << record.function << ']';
        }
        output << ' ' << record.message;
        return output.str();
    }

private:
    bool use_pattern_;
    std::vector<Token> tokens_;
};

std::string jsonEscape(const std::string& value)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string escaped;
    escaped.reserve(value.size() + 2);
    for (const unsigned char character : value)
    {
        switch (character)
        {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (character < 0x20)
                {
                    escaped += "\\u00";
                    escaped.push_back(kHex[(character >> 4) & 0x0f]);
                    escaped.push_back(kHex[character & 0x0f]);
                }
                else
                {
                    escaped.push_back(static_cast<char>(character));
                }
                break;
        }
    }
    return escaped;
}

void appendJsonString(std::ostringstream& output, const std::string& key, const std::string& value,
                      bool& first)
{
    if (!first)
    {
        output << ',';
    }
    output << '"' << key << "\":\"" << jsonEscape(value) << '"';
    first = false;
}

class JsonFormatter final : public ILogFormatter
{
public:
    std::string format(const LogRecord& record) const override
    {
        std::ostringstream output;
        output << '{';
        bool first = true;
        appendJsonString(output, "timestamp", dateTime(record), first);
        appendJsonString(output, "level", toString(record.level), first);
        std::ostringstream thread;
        thread << record.thread_id;
        appendJsonString(output, "thread_id", thread.str(), first);
        if (!record.name.empty()) appendJsonString(output, "logger", record.name, first);

        if (!record.context.empty())
        {
            if (!first) output << ',';
            output << "\"context\":{";
            bool first_context = true;
            for (const auto& field : record.context)
            {
                if (!first_context) output << ',';
                output << '"' << jsonEscape(field.key) << "\":\"" << jsonEscape(field.value) << '"';
                first_context = false;
            }
            output << '}';
            first = false;
        }

        if (!record.file.empty()) appendJsonString(output, "file", record.file, first);
        if (record.line != 0)
        {
            if (!first) output << ',';
            output << "\"line\":" << record.line;
            first = false;
        }
        if (!record.function.empty()) appendJsonString(output, "function", record.function, first);
        appendJsonString(output, "message", record.message, first);
        output << '}';
        return output.str();
    }
};

}  // namespace

std::unique_ptr<ILogFormatter> createFormatter(const FormatterConfig& config)
{
    switch (config.format)
    {
        case ELogFormat::kText:
            return std::make_unique<TextFormatter>(config);
        case ELogFormat::kJson:
            if (config.pattern_configured)
            {
                throw std::invalid_argument("logger.pattern cannot be used when logger.format=json");
            }
            return std::make_unique<JsonFormatter>();
    }
    throw std::invalid_argument("unknown logger format");
}

}  // namespace logger::detail
