#include <cassert>
#include <chrono>
#include <fstream>
#include <string>
#include <ranges>
#include <vector>

#include "curl/curl.h"

namespace koenig {
    struct Instance {
        Instance() noexcept;
        ~Instance() noexcept;
    };

    struct EmailResult {
    private:
        const CURLcode status;
    public:
        const bool success;
        
        EmailResult() noexcept;
        explicit EmailResult(CURLcode status) noexcept;

        explicit operator bool() const noexcept;
        std::string what() const noexcept;
    };

    class Email {
    public:
        CURL* m_curl = nullptr;

        size_t m_bytesRead = 0;
        const char* m_payload;

        std::string getPayload() noexcept;

        //is static because CURLOPT_READFUNCTION expects C-style function pointer - email param is "this"
        static size_t readPayload(char* ptr, size_t size, size_t nmemb, void* email);
    public: 
        Email() noexcept;
        ~Email() noexcept;
        Email(const Email&) = delete;

        std::string serverURL;
        std::string sender;
        std::string password;
        std::vector<std::string> recipients;
        std::vector<std::string> ccRecipients;
        std::string subject;
        std::string contentType = "text/plain; charset = \"UTF-8\"";
        std::string body;
        std::string certPath;

        void embedHTML(const std::string& htmlPath) noexcept;
        EmailResult send(bool printOutput = true) noexcept;
    };
};