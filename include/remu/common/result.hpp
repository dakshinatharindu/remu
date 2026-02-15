#pragma once

#include <string>
#include <utility>
#include <variant>

namespace remu::common {

//
// Generic Result<T>
//

template <typename T>
class Result {
   public:
    // Factory: success
    static Result ok(T value) { return Result(std::move(value)); }

    // Factory: error
    static Result err(std::string error_message) {
        return Result(std::move(error_message));
    }

    // Check success
    bool has_value() const { return std::holds_alternative<T>(storage_); }

    explicit operator bool() const { return has_value(); }

    // Access value (caller must check first)
    T& value() { return std::get<T>(storage_); }

    const T& value() const { return std::get<T>(storage_); }

    // Access error
    const std::string& error() const { return std::get<std::string>(storage_); }

   private:
    // Success constructor
    explicit Result(T value) : storage_(std::move(value)) {}

    // Error constructor
    explicit Result(std::string error_message)
        : storage_(std::move(error_message)) {}

   private:
    std::variant<T, std::string> storage_;
};

//
// Specialization for Result<void>
//

template <>
class Result<void> {
   public:
    static Result ok() { return Result(true, ""); }

    static Result err(std::string error_message) {
        return Result(false, std::move(error_message));
    }

    bool has_value() const { return ok_; }

    explicit operator bool() const { return ok_; }

    const std::string& error() const { return error_; }

   private:
    Result(bool ok, std::string error_message)
        : ok_(ok), error_(std::move(error_message)) {}

   private:
    bool ok_;
    std::string error_;
};

}  // namespace remu::common
