#include "common.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include <functional>
#include <stdexcept>

using namespace GDot;

namespace {

QJsonObject collectSystemInventory()
{
    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);

    if (!GlobalMemoryStatusEx(&memory))
        throw std::runtime_error("GlobalMemoryStatusEx failed.");

    return {
        {"computer", QSysInfo::machineHostName()},
        {"operating_system", QSysInfo::prettyProductName()},
        {"kernel", QSysInfo::kernelVersion()},
        {"architecture", QSysInfo::currentCpuArchitecture()},
        {"logical_processors", QThread::idealThreadCount()},
        {"memory_total_bytes",
            static_cast<double>(memory.ullTotalPhys)},
        {"memory_available_bytes",
            static_cast<double>(memory.ullAvailPhys)},
        {"memory_load_percent",
            static_cast<int>(memory.dwMemoryLoad)},
        {"collected_at_utc",
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    };
}

QJsonObject collectDiskInventory()
{
    wchar_t windowsDirectory[MAX_PATH]{};

    const UINT length = GetWindowsDirectoryW(
        windowsDirectory,
        MAX_PATH
    );

    if (length == 0 || length >= MAX_PATH)
        throw std::runtime_error("Unable to locate the Windows directory.");

    const QString root =
        QDir(QString::fromWCharArray(windowsDirectory)).rootPath();

    ULARGE_INTEGER available{};
    ULARGE_INTEGER total{};
    ULARGE_INTEGER freeSpace{};

    const auto* rootPath =
        reinterpret_cast<LPCWSTR>(root.utf16());

    if (!GetDiskFreeSpaceExW(
            rootPath,
            &available,
            &total,
            &freeSpace)) {
        throw std::runtime_error("GetDiskFreeSpaceExW failed.");
    }

    return {
        {"drive", root},
        {"total_bytes", static_cast<double>(total.QuadPart)},
        {"free_bytes", static_cast<double>(freeSpace.QuadPart)},
        {"available_to_user_bytes",
            static_cast<double>(available.QuadPart)},
        {"used_bytes",
            static_cast<double>(total.QuadPart - freeSpace.QuadPart)}
    };
}

QJsonObject collectProcessInventory()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );

    if (snapshot == INVALID_HANDLE_VALUE)
        throw std::runtime_error("CreateToolhelp32Snapshot failed.");

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    QJsonArray processes;
    bool truncated = false;

    if (!Process32FirstW(snapshot, &entry)) {
        CloseHandle(snapshot);
        throw std::runtime_error("Process32FirstW failed.");
    }

    do {
        if (processes.size() >= 2000) {
            truncated = true;
            break;
        }

        processes.append(QJsonObject{
            {"image", QString::fromWCharArray(entry.szExeFile)},
            {"pid", static_cast<double>(entry.th32ProcessID)},
            {"parent_pid",
                static_cast<double>(entry.th32ParentProcessID)},
            {"threads",
                static_cast<double>(entry.cntThreads)}
        });
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);

    return {
        {"processes", processes},
        {"count", processes.size()},
        {"truncated", truncated},
        {"limit", 2000}
    };
}

QJsonObject executeOperation(const QString& operation)
{
    if (operation == "system")
        return collectSystemInventory();

    if (operation == "disk")
        return collectDiskInventory();

    if (operation == "processes")
        return collectProcessInventory();

    throw std::runtime_error("Unsupported operation.");
}

class Listener final : public QTcpServer {
public:
    std::function<void(qintptr)> connectionHandler;

protected:
    void incomingConnection(qintptr descriptor) override
    {
        if (connectionHandler) {
            connectionHandler(descriptor);
            return;
        }

        QTcpSocket socket;

        if (socket.setSocketDescriptor(descriptor))
            socket.abort();
    }
};

class ManagedWindow final : public QMainWindow {
public:
    ManagedWindow()
    {
        setWindowTitle("G DOT | Managed Computer");
        resize(900, 780);
        setMinimumSize(780, 650);

        auto* root = new QWidget;
        auto* layout = new QVBoxLayout(root);

        layout->setContentsMargins(26, 24, 26, 24);
        layout->setSpacing(16);

        setCentralWidget(root);

        addHeader(
            layout,
            "G DOT / Managed Computer",
            "Visible access · Encrypted connections · Local approval"
        );

        auto* connectionCard =
            createCard(layout, "Connection security");

        auto* connectionForm = new QFormLayout;
        cardLayout(connectionCard)->addLayout(connectionForm);

        bindAddress = new QLineEdit("127.0.0.1");

        port = new QSpinBox;
        port->setRange(1024, 65535);
        port->setValue(DefaultPort);

        token = new QLineEdit(generateToken());
        token->setEchoMode(QLineEdit::Password);

        connectionForm->addRow("Bind address", bindAddress);
        connectionForm->addRow("TCP port", port);
        connectionForm->addRow("Session token", token);

        auto* tokenControls = new QHBoxLayout;

        regenerateToken = new QPushButton("Generate new token");
        revealToken = new QCheckBox("Show token");

        tokenControls->addWidget(regenerateToken);
        tokenControls->addWidget(revealToken);
        tokenControls->addStretch();

        cardLayout(connectionCard)->addLayout(tokenControls);

        connect(
            regenerateToken,
            &QPushButton::clicked,
            this,
            [this] {
                token->setText(generateToken());
            }
        );

        connect(
            revealToken,
            &QCheckBox::toggled,
            this,
            [this](bool checked) {
                token->setEchoMode(
                    checked
                        ? QLineEdit::Normal
                        : QLineEdit::Password
                );
            }
        );

        auto* accessCard =
            createCard(layout, "Access control");

        state = new QLabel(
            "Stopped — remote access unavailable"
        );
        state->setObjectName("status");
        state->setWordWrap(true);

        cardLayout(accessCard)->addWidget(state);

        auto* explanation = new QLabel(
            "Every request displays an approval dialog on this computer. "
            "Stop Access immediately closes the listener and any active "
            "connection. Use only on a trusted LAN or VPN."
        );
        explanation->setWordWrap(true);
        explanation->setObjectName("subtitle");

        cardLayout(accessCard)->addWidget(explanation);

        auto* accessButtons = new QHBoxLayout;

        startButton = new QPushButton("Start Access");
        stopButton = new QPushButton("Stop Access");
        stopButton->setObjectName("danger");
        stopButton->setEnabled(false);

        accessButtons->addWidget(startButton);
        accessButtons->addWidget(stopButton);
        accessButtons->addStretch();

        cardLayout(accessCard)->addLayout(accessButtons);

        auto* activityCard =
            createCard(layout, "Security activity");

        activity = new QPlainTextEdit;
        activity->setReadOnly(true);
        activity->setMaximumBlockCount(1000);

        cardLayout(activityCard)->addWidget(activity);

        layout->setStretch(layout->count() - 1, 1);

        listener.connectionHandler =
            [this](qintptr descriptor) {
                acceptConnection(descriptor);
            };

        connect(
            startButton,
            &QPushButton::clicked,
            this,
            [this] {
                startAccess();
            }
        );

        connect(
            stopButton,
            &QPushButton::clicked,
            this,
            [this] {
                stopAccess();
            }
        );
    }

    ~ManagedWindow() override
    {
        stopAccess();
    }

private:
    Listener listener;

    QPointer<QSslSocket> activeSocket;

    QSslCertificate certificate;
    QSslKey privateKey;

    QLineEdit* bindAddress{};
    QSpinBox* port{};
    QLineEdit* token{};

    QPushButton* regenerateToken{};
    QPushButton* startButton{};
    QPushButton* stopButton{};

    QCheckBox* revealToken{};
    QLabel* state{};
    QPlainTextEdit* activity{};

    QByteArray sessionToken;
    QFile auditFile;

    void audit(
        const QString& event,
        const QJsonObject& fields = {})
    {
        QJsonObject record = fields;
        record["event"] = event;
        record["time_utc"] =
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        const QByteArray data = encodeFrame(record);

        activity->appendPlainText(
            QString::fromUtf8(data).trimmed()
        );

        if (auditFile.isOpen()) {
            if (auditFile.write(data) != data.size()
                || !auditFile.flush()) {
                state->setText(
                    "Audit error — stop access and inspect storage"
                );
            }
        }
    }

    void sendReply(
        QSslSocket* socket,
        const QJsonObject& object)
    {
        if (!socket)
            return;

        if (socket->state() ==
            QAbstractSocket::UnconnectedState) {
            return;
        }

        socket->write(encodeFrame(object));
        socket->disconnectFromHost();
    }

    void handleRequest(
        QSslSocket* socket,
        const QJsonObject& request)
    {
        const QString peer =
            socket->peerAddress().toString();

        if (request.size() != 2
            || !request.value("token").isString()
            || !request.value("operation").isString()) {
            sendReply(socket, {
                {"ok", false},
                {"error", "Invalid request format"}
            });
            return;
        }

        const QString operation =
            request.value("operation").toString();

        const QByteArray suppliedToken =
            request.value("token").toString().toUtf8();

        if (!validOperation(operation)) {
            sendReply(socket, {
                {"ok", false},
                {"error", "Unsupported operation"}
            });
            return;
        }

        if (!constantTimeEqual(suppliedToken, sessionToken)) {
            audit("authentication_denied", {
                {"peer", peer}
            });

            sendReply(socket, {
                {"ok", false},
                {"error", "Authentication failed"}
            });
            return;
        }

        const QString requestId =
            QUuid::createUuid().toString(QUuid::WithoutBraces);

        audit("approval_requested", {
            {"peer", peer},
            {"operation", operation},
            {"request_id", requestId}
        });

        state->setText(
            "Approval required — " + operation
        );

        auto* dialog = new QMessageBox(
            QMessageBox::Question,
            "G DOT approval required",
            QString(
                "Remote request received.\n\n"
                "Computer: %1\n"
                "Operation: %2\n"
                "Request ID: %3\n\n"
                "Allow this read-only assessment?"
            ).arg(peer, operation, requestId),
            QMessageBox::Yes | QMessageBox::No,
            this
        );

        dialog->setDefaultButton(QMessageBox::No);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        QPointer<QSslSocket> guardedSocket = socket;

        connect(
            socket,
            &QSslSocket::disconnected,
            dialog,
            [dialog] {
                dialog->reject();
            }
        );

        connect(
            dialog,
            &QDialog::finished,
            this,
            [
                this,
                guardedSocket,
                peer,
                operation,
                requestId
            ](int result) {
                if (!guardedSocket)
                    return;

                if (guardedSocket->state() ==
                    QAbstractSocket::UnconnectedState) {
                    return;
                }

                if (result != QMessageBox::Yes) {
                    audit("request_denied", {
                        {"peer", peer},
                        {"operation", operation},
                        {"request_id", requestId}
                    });

                    sendReply(guardedSocket, {
                        {"ok", false},
                        {"error", "Local approval denied"},
                        {"request_id", requestId}
                    });

                    return;
                }

                try {
                    const QJsonObject resultObject =
                        executeOperation(operation);

                    audit("operation_completed", {
                        {"peer", peer},
                        {"operation", operation},
                        {"request_id", requestId}
                    });

                    sendReply(guardedSocket, {
                        {"ok", true},
                        {"request_id", requestId},
                        {"result", resultObject}
                    });
                } catch (const std::exception& error) {
                    const QString errorText =
                        QString::fromUtf8(error.what());

                    audit("operation_failed", {
                        {"operation", operation},
                        {"request_id", requestId},
                        {"error", errorText}
                    });

                    sendReply(guardedSocket, {
                        {"ok", false},
                        {"request_id", requestId},
                        {"error", errorText}
                    });
                }
            }
        );

        dialog->open();
    }

    void acceptConnection(qintptr descriptor)
    {
        if (activeSocket) {
            QTcpSocket rejected;

            if (rejected.setSocketDescriptor(descriptor))
                rejected.abort();

            return;
        }

        auto* socket = new QSslSocket(this);

        if (!socket->setSocketDescriptor(descriptor)) {
            socket->deleteLater();
            return;
        }

        activeSocket = socket;
        socket->setReadBufferSize(RequestLimit + 1);

        QSslConfiguration configuration =
            QSslConfiguration::defaultConfiguration();

        configuration.setProtocol(QSsl::TlsV1_2OrLater);
        configuration.setLocalCertificate(certificate);
        configuration.setPrivateKey(privateKey);
        configuration.setPeerVerifyMode(QSslSocket::VerifyNone);

        socket->setSslConfiguration(configuration);

        auto* timeout = new QTimer(socket);
        timeout->setSingleShot(true);

        connect(
            timeout,
            &QTimer::timeout,
            this,
            [this, socket] {
                audit("connection_timeout");
                socket->abort();
            }
        );

        timeout->start(ConnectionTimeoutMs);

        auto buffer =
            std::make_shared<QByteArray>();

        auto received =
            std::make_shared<bool>(false);

        connect(
            socket,
            &QSslSocket::readyRead,
            this,
            [
                this,
                socket,
                buffer,
                received
            ] {
                if (*received) {
                    socket->readAll();
                    return;
                }

                buffer->append(socket->readAll());

                if (buffer->size() > RequestLimit) {
                    audit("request_too_large");
                    socket->abort();
                    return;
                }

                const qsizetype newline =
                    buffer->indexOf('\n');

                if (newline < 0)
                    return;

                *received = true;

                if (newline != buffer->size() - 1) {
                    sendReply(socket, {
                        {"ok", false},
                        {"error", "Invalid request framing"}
                    });
                    return;
                }

                QJsonParseError parseError{};

                const QJsonDocument document =
                    QJsonDocument::fromJson(
                        buffer->left(newline),
                        &parseError
                    );

                if (parseError.error !=
                        QJsonParseError::NoError
                    || !document.isObject()) {
                    sendReply(socket, {
                        {"ok", false},
                        {"error", "Invalid JSON"}
                    });
                    return;
                }

                handleRequest(socket, document.object());
            }
        );

        connect(
            socket,
            &QSslSocket::encrypted,
            this,
            [this, socket] {
                audit("tls_connected", {
                    {"peer", socket->peerAddress().toString()}
                });

                state->setText(
                    "TLS connected — authentication pending"
                );
            }
        );

        connect(
            socket,
            &QSslSocket::sslErrors,
            this,
            [this, socket](const QList<QSslError>& errors) {
                QStringList messages;

                for (const QSslError& error : errors)
                    messages.append(error.errorString());

                audit("tls_error", {
                    {"error", messages.join("; ")}
                });

                socket->abort();
            }
        );

        connect(
            socket,
            &QSslSocket::errorOccurred,
            this,
            [this, socket](QAbstractSocket::SocketError) {
                audit("socket_error", {
                    {"error", shortError(socket->errorString())}
                });
            }
        );

        connect(
            socket,
            &QSslSocket::disconnected,
            this,
            [this, socket] {
                if (activeSocket == socket)
                    activeSocket.clear();

                state->setText(
                    listener.isListening()
                        ? "Listening — approval required"
                        : "Stopped — remote access unavailable"
                );

                socket->deleteLater();
            }
        );

        socket->startServerEncryption();
    }

    void startAccess()
    {
        if (!validToken(token->text())) {
            QMessageBox::warning(
                this,
                "Invalid token",
                "The token must contain 32–128 printable ASCII "
                "characters without spaces."
            );
            return;
        }

        QHostAddress address;

        if (!address.setAddress(bindAddress->text().trimmed())) {
            QMessageBox::warning(
                this,
                "Invalid bind address",
                "Enter a numeric IPv4 or IPv6 address."
            );
            return;
        }

        const QString certificatePath =
            QFileDialog::getOpenFileName(
                this,
                "Select server certificate",
                {},
                "PEM certificate (*.pem)"
            );

        if (certificatePath.isEmpty())
            return;

        const QString keyPath =
            QFileDialog::getOpenFileName(
                this,
                "Select private key",
                {},
                "PEM private key (*.pem)"
            );

        if (keyPath.isEmpty())
            return;

        QFile certificateFile(certificatePath);
        QFile keyFile(keyPath);

        if (!certificateFile.open(QIODevice::ReadOnly)
            || !keyFile.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(
                this,
                "TLS files",
                "The selected TLS files could not be opened."
            );
            return;
        }

        certificate = QSslCertificate(
            certificateFile.readAll(),
            QSsl::Pem
        );

        privateKey = QSslKey(
            keyFile.readAll(),
            QSsl::Rsa,
            QSsl::Pem,
            QSsl::PrivateKey
        );

        const QDateTime now =
            QDateTime::currentDateTimeUtc();

        if (certificate.isNull()
            || privateKey.isNull()
            || now < certificate.effectiveDate()
            || now > certificate.expiryDate()) {
            QMessageBox::warning(
                this,
                "Invalid TLS identity",
                "The certificate or private key is invalid, "
                "or the certificate is expired."
            );
            return;
        }

        const QString logDirectory =
            QStandardPaths::writableLocation(
                QStandardPaths::AppLocalDataLocation
            );

        if (!QDir().mkpath(logDirectory)) {
            QMessageBox::warning(
                this,
                "Audit storage",
                "The audit directory could not be created."
            );
            return;
        }

        const QString filename =
            "audit-" +
            QDateTime::currentDateTimeUtc().toString(
                "yyyyMMdd-HHmmss"
            ) +
            "-" +
            QUuid::createUuid().toString(
                QUuid::WithoutBraces
            ) +
            ".jsonl";

        auditFile.setFileName(
            QDir(logDirectory).filePath(filename)
        );

        if (!auditFile.open(
                QIODevice::WriteOnly
                | QIODevice::NewOnly)) {
            QMessageBox::warning(
                this,
                "Audit storage",
                "The audit file could not be created."
            );
            return;
        }

        sessionToken = token->text().toUtf8();

        if (!listener.listen(
                address,
                static_cast<quint16>(port->value()))) {
            QMessageBox::warning(
                this,
                "Listener",
                listener.errorString()
            );

            auditFile.close();
            sessionToken.fill('\0');
            sessionToken.clear();
            return;
        }

        bindAddress->setEnabled(false);
        port->setEnabled(false);
        token->setEnabled(false);
        revealToken->setEnabled(false);
        regenerateToken->setEnabled(false);

        startButton->setEnabled(false);
        stopButton->setEnabled(true);

        state->setText(
            "Listening — local approval required"
        );

        audit("listener_started", {
            {"address", bindAddress->text()},
            {"port", port->value()}
        });

        statusBar()->showMessage(
            "Audit log: " + auditFile.fileName()
        );
    }

    void stopAccess()
    {
        listener.close();

        if (activeSocket)
            activeSocket->abort();

        if (auditFile.isOpen())
            audit("listener_stopped");

        auditFile.close();

        sessionToken.fill('\0');
        sessionToken.clear();

        bindAddress->setEnabled(true);
        port->setEnabled(true);
        token->setEnabled(true);
        revealToken->setEnabled(true);
        regenerateToken->setEnabled(true);

        startButton->setEnabled(true);
        stopButton->setEnabled(false);

        state->setText(
            "Stopped — remote access unavailable"
        );
    }
};

} // namespace

QMainWindow* createManagedWindow()
{
    return new ManagedWindow;
}