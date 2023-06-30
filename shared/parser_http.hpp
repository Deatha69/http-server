#include <iostream>
#include <string>
#include <vector>
#include "headers.hpp"

class HttpRequest
{
  public:
    static void parseStartLine(const std::string& request, std::vector<std::string>& startLine);
    static void parseHeaders(const std::string& request, Headers& headers);
    static void parseData(const std::string& request, std::string& data);
  private:
    static size_t findLineEndPos(const std::string& request);
};


