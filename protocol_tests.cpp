#include "common.hpp"

#include <iostream>

using namespace GDot;

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);

    int failures = 0;

    const auto check =
        [&failures](bool condition, const char* name) {
            if (!condition) {
                std::cerr << "FAIL: " << name << '\n';
                ++failures;
            }
        };

    check(
        validOperation("system"),
        "system operation accepted"
    );

    check(
        validOperation("disk"),
        "disk operation accepted"
    );

    check(
        validOperation("processes"),
        "process operation accepted"
    );

    check(
        !validOperation("shell"),
        "arbitrary shell rejected"
    );

    check(
        !validOperation(""),
        "empty operation rejected"
    );

    const QString token = generateToken();

    check(
        token.size() == 64,
        "generated token length"
    );

    check(
        validToken(token),
        "generated token accepted"
    );

    check(
        !validToken("short"),
        "short token rejected"
    );

    check(
        !validToken(QString(32, ' ')),
        "whitespace token rejected"
    );

    check(
        !validToken(QString(129, 'a')),
        "oversized token rejected"
    );

    check(
        constantTimeEqual("abc", "abc"),
        "equal tokens accepted"
    );

    check(
        !constantTimeEqual("abc", "abd"),
        "different tokens rejected"
    );

    check(
        !constantTimeEqual("abc", "abcd"),
        "different lengths rejected"
    );

    const QByteArray encoded = encodeFrame({
        {"token", QString(64, 'a')},
        {"operation", "system"}
    });

    check(
        encoded.endsWith('\n'),
        "frame delimiter present"
    );

    check(
        encoded.count('\n') == 1,
        "single frame delimiter"
    );

    check(
        encoded.size() < RequestLimit,
        "frame size limit"
    );

    QJsonParseError parseError{};

    const QJsonDocument decoded =
        QJsonDocument::fromJson(
            encoded.trimmed(),
            &parseError
        );

    check(
        parseError.error == QJsonParseError::NoError,
        "frame JSON parses"
    );

    check(
        decoded.object()
            .value("operation")
            .toString()
            == "system",
        "operation round trip"
    );

    if (failures == 0) {
        std::cout << "All tests passed\n";
        return 0;
    }

    std::cout << failures << " test(s) failed\n";
    return 1;
}