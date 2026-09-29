#ifndef INTELNET_ERROR_H
#define INTELNET_ERROR_H

#include <string>

namespace IntelNet {

enum class ErrorCode {
    Success = 0,
    InitFailed = -1,
    NotInitialized = -2,
    InvalidParameter = -3,
    ModelLoadFailed = -4,
    InferenceFailed = -5,
    TtsFailed = -6,
    Unknown = -999
};

struct Result {
    ErrorCode code;
    std::string message;

    bool isSuccess() const { return code == ErrorCode::Success; }
    bool isError() const { return code != ErrorCode::Success; }

    static Result success() {
        return Result{ErrorCode::Success, ""};
    }

    static Result error(ErrorCode code, const std::string& msg) {
        return Result{code, msg};
    }
};

// Convert FFI return codes to Result
inline Result fromFfiCode(int code) {
    if (code == 0) return Result::success();

    ErrorCode errorCode;
    std::string message;

    switch (code) {
        case -1:
            errorCode = ErrorCode::InitFailed;
            message = "Initialization failed";
            break;
        case -2:
            errorCode = ErrorCode::NotInitialized;
            message = "Not initialized";
            break;
        default:
            errorCode = ErrorCode::Unknown;
            message = "Unknown error: " + std::to_string(code);
    }

    return Result{errorCode, message};
}

} // namespace IntelNet

#endif // INTELNET_ERROR_H
