#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <sentry.h>

namespace {
std::mutex sentry_mutex;
}

std::string basename(const std::string &path) {
    return std::filesystem::path(path).filename().string();
}

void sentry_send_error(const std::string &error_type,
                       const std::string &message,
                       const std::string &transaction) {
    sentry_set_level(SENTRY_LEVEL_ERROR);

    // Set the class/function here
    sentry_set_transaction(transaction.c_str());

    sentry_value_t event = sentry_value_new_event();
    sentry_value_t exception = sentry_value_new_object();
    sentry_value_set_by_key(exception, "type",
                            sentry_value_new_string(error_type.c_str()));
    sentry_value_set_by_key(exception, "value",
                            sentry_value_new_string(message.c_str()));
    sentry_value_t exceptions = sentry_value_new_list();
    sentry_value_append(exceptions, exception);
    sentry_value_t values = sentry_value_new_object();
    sentry_value_set_by_key(values, "values", exceptions);
    sentry_value_set_by_key(event, "exception", values);
    sentry_event_value_add_stacktrace(event, NULL, 0);
    sentry_capture_event(event);
}

void sentry_capture_error(const std::string &type, const std::string &message,
                          const char *file, int line, const char *function) {
    const std::string file_name = basename(file);
    const std::string transaction =
        file_name + ":" + std::string(function) + ":" + std::to_string(line);

    std::cerr << "error: " << transaction << ": " << message << std::endl;

    sentry_send_error(type, message, transaction);
}

void sentry_set_handler_context(const std::string &request_id,
                                const std::string &request_method) {
    // Set the sentry context for a request, this needs to be done close to
    // sending the error because the context is global and threads can overwrite
    // one another
    sentry_value_t context = sentry_value_new_object();
    sentry_value_set_by_key(context, "type",
                            sentry_value_new_string("request"));
    sentry_value_set_by_key(context, "requestId",
                            sentry_value_new_string(request_id.c_str()));
    sentry_value_set_by_key(context, "method",
                            sentry_value_new_string(request_method.c_str()));
    std::stringstream thread_id;
    thread_id << "0x" << std::hex << std::this_thread::get_id();
    sentry_value_set_by_key(context, "thread",
                            sentry_value_new_string(thread_id.str().c_str()));
    sentry_set_context("request", context);
}

void sentry_capture_error_with_context(const std::string &type,
                                       const std::string &message,
                                       const char *file, int line,
                                       const char *function,
                                       const std::string &request_id,
                                       const std::string &request_method) {
    sentry_mutex.lock();
    try {
        if (!request_id.empty() || !request_method.empty())
            sentry_set_handler_context(request_id, request_method);

        sentry_capture_error(type, message, file, line, function);
    } catch (const std::exception &e) {
        sentry_mutex.unlock();
        throw e;
    }

    sentry_mutex.unlock();
}

void sentry_capture_server_exception(const std::runtime_error &exception,
                                     const char *file, int line,
                                     const char *function) {
    const std::string message =
        std::string("Server exception thrown: ") + exception.what();
    std::string error_type = "ServerError";

    const sentry_level_e level = SENTRY_LEVEL_ERROR;
    sentry_capture_error_with_context(error_type, message, file, line, function,
                                      "0", "none");
}
#define CAPTURE_SERVER_EXCEPTION(exception)                                    \
    sentry_capture_server_exception(exception, __FILE__, __LINE__, __func__)

int main() {
    // Configure sentry
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options,
                           "https://"
                           "fe6a7828e8e04b21bd5e2b8a87c2f860@o4505505890238464."
                           "ingest.sentry.io/4505505894170624");

    sentry_options_set_environment(options, "dev");
    sentry_options_set_release(options, "lbcpp@2.3.14");
    sentry_options_set_debug(options, 1);
    sentry_init(options);

    try {
        // Throw an instance of std::runtime_error
        throw std::runtime_error("This is an error!");

    } catch (const std::runtime_error &e) {
        // Catch and handle the error
        CAPTURE_SERVER_EXCEPTION(e);
        std::cerr << "Caught an exception: " << e.what() << std::endl;
    }

    sentry_capture_event(sentry_value_new_message_event(
        /*   level */ SENTRY_LEVEL_INFO,
        /*  logger */ "custom",
        /* message */ "It works!"));

    sentry_close();
}
