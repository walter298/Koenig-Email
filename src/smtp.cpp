#include "smtp.h"

koenig::Instance::Instance() noexcept {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

koenig::Instance::~Instance() noexcept {
    curl_global_cleanup();
}

koenig::EmailResult::EmailResult() noexcept
    : status{ CURLE_OK }, success{ true } {}

koenig::EmailResult::EmailResult(CURLcode status) noexcept
    : status{ status }, success{ false } {}

std::string koenig::EmailResult::what() const noexcept {
    return curl_easy_strerror(status);
}

koenig::EmailResult::operator bool() const noexcept {
    return success;
}

std::string koenig::Email::getPayload() noexcept {
    //create message-ID header
    auto time = std::chrono::system_clock::now();
    auto ID = std::to_string(time.time_since_epoch().count()) + sender;

    std::string msg; //the raw email message

    //add up the size of all string members and reserve it in msg
    constexpr size_t headerNamesSize = 36; //space taken up by "To: ", "Subject: ", etc
    auto totalEmailSize = headerNamesSize + ID.size();
    totalEmailSize += sender.size();
    for (const auto& recipient : recipients) {
        totalEmailSize += recipient.size();
    }
    for (const auto& recipient : ccRecipients) {
        totalEmailSize += recipient.size();
    }
    totalEmailSize += subject.size();
    totalEmailSize += body.size();
    msg.reserve(totalEmailSize);

    auto writeRecipientNames = [&msg](const auto& recipients) {
        for (const auto& rcpt : recipients) {
            msg.append("<" + rcpt + "> ");
        }
        msg.append("\r\n");
    };

    //create the message
    msg.append("To: ");
    writeRecipientNames(recipients);
    msg.append("From: <" + sender + ">\r\n");
    msg.append("Cc: ");
    writeRecipientNames(ccRecipients);
    msg.append("Message ID: <" + ID + ">\r\n");
    msg.append("Subject: " + subject + "\r\n");
    msg.append("Content-Type: " + contentType + "\r\n");
    msg.append("\r\n");
    msg.append(body);

    return msg;
}

size_t koenig::Email::readPayload(char* ptr, size_t size, size_t nmemb, void* p) {
    auto email = static_cast<Email*>(p);

    const char* data;
    size_t room = size * nmemb;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
        return size_t{ 0 };
    }

    data = &email->m_payload[email->m_bytesRead];

    if (data) {
        size_t len = strlen(data);
        if (room < len) {
            len = room;
        }
    
        memcpy(ptr, data, len);
        email->m_bytesRead += len;

        return len;
    }

    return size_t{ 0 };
}

koenig::Email::Email() noexcept {
    m_curl = curl_easy_init();
}

koenig::Email::~Email() noexcept {
    curl_easy_cleanup(m_curl);
}

void koenig::Email::embedHTML(const std::string& HTMLPath) noexcept {
    std::ifstream file{ HTMLPath };
    assert(file.is_open());

    //put all the HTML into the body of the message
    std::string line;
    while (std::getline(file, line)) {
        body.append(std::move(line) + "\r\n");
        line.clear();
    }
    //specifiy that we are sending html in our email
    contentType = "text/html; charset=\"UTF-8\"\r\n";
}

koenig::EmailResult koenig::Email::send(bool printOutput) noexcept {
    //sanity checks
    assert(!sender.empty());
    assert(!password.empty());
    assert(!certPath.empty());
    assert(!recipients.empty());

    CURLcode res = CURLE_OK;
    curl_slist* curlRecipients = nullptr;

    auto payload = getPayload();
    m_payload = payload.c_str();

    if (m_curl) {
        //authenticate user information. For Gmail you need to supply an "app password" - NOT your normal password
        curl_easy_setopt(m_curl, CURLOPT_USERNAME, sender.c_str());
        curl_easy_setopt(m_curl, CURLOPT_PASSWORD, password.c_str());

        //specify email server and encryption
        curl_easy_setopt(m_curl, CURLOPT_URL, serverURL.c_str());
        curl_easy_setopt(m_curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        curl_easy_setopt(m_curl, CURLOPT_CAINFO, certPath.c_str());
        curl_easy_setopt(m_curl, CURLOPT_MAIL_FROM, sender.c_str());

        //add strings from recipients and ccRecipients to curlRecipients
        auto appendToCurlRecipients = [&curlRecipients](const auto& recipients) {
            for (const auto& recipient : recipients) {
                curlRecipients = curl_slist_append(curlRecipients, recipient.c_str());
            }
        };
        appendToCurlRecipients(recipients);
        appendToCurlRecipients(ccRecipients);
        curl_easy_setopt(m_curl, CURLOPT_MAIL_RCPT, curlRecipients);

        //read the payload data
        curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, readPayload);
        curl_easy_setopt(m_curl, CURLOPT_READDATA, this);
        curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
        
        if (printOutput) {
            curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
        }
       
        //send the email!
        res = curl_easy_perform(m_curl);

        curl_slist_free_all(curlRecipients);
        curl_easy_reset(m_curl);
    }
    return res == CURLE_OK ? EmailResult{} : EmailResult{ res };
}
