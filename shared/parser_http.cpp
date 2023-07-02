#include "parser_http.hpp"

void HttpRequest::parseStartLine(const std::string& request, std::vector<std::string>& startLine)
{
    size_t      lineEndPos   = findLineEndPos(request);
    std::string startLineStr = request.substr(0, lineEndPos);
    size_t      start        = 0, end;

    while ((end = startLineStr.find(' ', start)) != std::string::npos) {
        startLine.push_back(startLineStr.substr(start, end - start));
        start = end + 1;
    }
    startLine.push_back(startLineStr.substr(start));
}

void HttpRequest::parseHeaders(const std::string& request, Headers& headers)
{
    size_t lineEndPos    = findLineEndPos(request);
    size_t headersEndPos = request.find("\r\n\r\n", lineEndPos);
    if (headersEndPos == std::string::npos) {
        headersEndPos = request.find("\n\n", lineEndPos);
        if (headersEndPos == std::string::npos) {
            throw std::runtime_error("Invalid request");
        }
    }
    headers.from_string(request.substr(lineEndPos + 2, headersEndPos - lineEndPos - 2));
}

void HttpRequest::parseData(const std::string& request, std::string& data)
{
    size_t headersEndPos = request.find("\r\n\r\n");
    if (headersEndPos == std::string::npos) {
        headersEndPos = request.find("\n\n");
        if (headersEndPos == std::string::npos) {
            throw std::runtime_error("Invalid request");
        }
    }
    data = request.substr(headersEndPos + 4);
}

void HttpRequest::parseQuery(std::string& path, std::unordered_map<std::string, std::string>& values)
{
    values.clear();
    
    size_t      pos = 0;
    std::string token, key, value;

    if (path == "/") {
        key         = "index.php";
        value       = "content of index.php";
        values[key] = value;
        return;
    }

    size_t startPos = path.find('?');

    if (startPos != std::string::npos) {
        path = path.substr(startPos + 1);
    }

    // Parse the query string
    while ((pos = path.find('&')) != std::string::npos) {
        token = path.substr(0, pos);
        path.erase(0, pos + 1);

        // Split the token into key and value using '=' as delimiter
        size_t equalsPos = token.find('=');
        if (equalsPos != std::string::npos) {
            key   = token.substr(0, equalsPos);
            value = token.substr(equalsPos + 1);

            // Store key-value pair in the unordered map
            values[key] = value;
        }
    }

    // Store the last key-value pair
    size_t equalsPos = path.find('=');
    if (equalsPos != std::string::npos) {
        key   = path.substr(0, equalsPos);
        value = path.substr(equalsPos + 1);

        // Store key-value pair in the unordered map
        values[key] = value;
    }
}

size_t HttpRequest::findLineEndPos(const std::string& request)
{
    size_t lineEndPos = request.find("\r\n");
    if (lineEndPos == std::string::npos) {
        lineEndPos = request.find("\n");
        if (lineEndPos == std::string::npos) {
            throw std::runtime_error("Invalid request");
        }
    }
    return lineEndPos;
}
