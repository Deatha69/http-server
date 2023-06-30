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
