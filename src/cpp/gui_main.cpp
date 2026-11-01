#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>

#include "gui/ApiController.h"
#include "gui/GuiBootstrap.h"
#include "httplib/httplib.h"

#ifndef TIVIDY_WEB_DIR
#define TIVIDY_WEB_DIR "web"
#endif

int main(int argc, char** argv) {
    int port = 8080;
    std::string webDirectory;

    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0) {
            port = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--web") == 0) {
            webDirectory = argv[++i];
        }
    }

    GuiEngine engine;
    ApiController api(engine.facade);
    std::mutex engineMutex;

    httplib::Server server;
    const char* webDirectories[] = {
        webDirectory.c_str(),
        "web",
        TIVIDY_WEB_DIR,
    };

    bool mounted = false;
    for (const char* directory : webDirectories) {
        if (*directory != '\0' && server.set_mount_point("/", directory)) {
            mounted = true;
            break;
        }
    }

    if (!mounted) {
        std::cerr << "Could not find the 'web' folder. Run from the project folder "
                     "or pass --web <path>.\n";
        return 1;
    }

    httplib::Server::Handler forwardRequest =
        [&](const httplib::Request& request, httplib::Response& response) {
            std::lock_guard<std::mutex> lock(engineMutex);
            ApiResponse result =
                api.handle(request.method, request.path, request.body);

            response.status = result.status;
            response.set_content(result.body.dump(), "application/json");
        };

    server.Get(R"(/api/.*)", forwardRequest);
    server.Post(R"(/api/.*)", forwardRequest);

    std::cout << "TIVIDY board running at http://localhost:" << port
              << "  (Ctrl+C to stop)" << std::endl;

    if (!server.listen("127.0.0.1", port)) {
        std::cerr << "Could not listen on port " << port
                  << " (in use?). Try --port 8081\n";
        return 1;
    }

    return 0;
}
