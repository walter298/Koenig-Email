**Summary**

This is a very simple C++ SMTP Client library, wrapped around libcurl. You construct ```Email``` objects, storing parameters such as the server name, sender email, and recipients as data members. Once the necessary parameters have been set, you call ```send``` to send the email. ```send``` parses all the members into a properly formatted email message, and then sends it using TLS encryption. You can resend an email and/or change its parameters. 

**Motivation**

I can't find a single other C++ library that sends email and is actually easy to use. Libcurl is written in C, and is a mess of macros and void pointers. VMime is very difficult to set up on Windows, and is not compatible with post C++14 versions. POCO is aggressively boilerplate-OOP style. I wanted a modern, simple, understandable library where you can send email as easily as you could in Python. 

**Requirements to Set Up**
1. Libcurl should be linked
2. A working C++20 Compiler and Standard Library

**Example**


```cpp
#include "smtp.h"

int main(void) {
    //globally initializes libcurl in constructor and quits libcurl in destructor. 
    koenig::Instance instance;

    koenig::Email email;
    email.serverURL = "smtp://smtp.gmail.com:587";
    email.sender = "ken@gmail.com";
    email.password = "my_app_password";
    email.certPath = "cacert.pem";
    email.recipients = { "robert@yahoo.com", "jim@gmail.com", "sarah@gmail.com" };
    email.ccRecipients = { "walter@gmail.com" };
    email.subject = "Example Email";
    email.body = "It seems you've been living two lives";
    email.embedHTML("html_ex.html");
    auto res = email.send(false); //pass true if you want debug information to be printed 

    if (!res) {
        std::cerr << res.what() << '\n';
    }

    return 0;
}
```
