#include "common.hpp"

using namespace GDot;

namespace {

class ConsoleWindow final : public QMainWindow {
public:
    ConsoleWindow()
    {
        setWindowTitle("G DOT | Administration Console");
        resize(1100, 850);
        setMinimumSize(900, 700);

        auto* root = new QWidget;
        auto* layout = new QVBoxLayout(root);

        layout->setContentsMargins(26, 24, 26, 24);
        layout->setSpacing(16);

        setCentralWidget(root);

        addHeader(
            layout,
            "G DOT / Administration Console",
            "Windows assessment · Verified identity · Structured results"
        );

        auto* endpointCard =
            createCard(layout, "Managed endpoint");

        auto* endpointForm = new QFormLayout;
        cardLayout(endpointCard)->addLayout(endpointForm);

        host = new QLineEdit("localhost");

        port = new QSpinBox;
        port->setRange(1024, 65535);
        port->setValue(DefaultPort);

        token = new QLineEdit;
        token->setEchoMode(QLineEdit::Password);
        token->setPlaceholderText(
            "Enter the token shown by Managed Computer"
        );

        endpointForm->addRow("Host / IP", host);
        endpointForm->addRow("TCP port", port);
        endpointForm->addRow("Session token", token);

        selectCertificateButton =
            new QPushButton("Select trusted certificate");

        certificateDetails =
            new QLabel("No certificate selected");

        certificateDetails->setObjectName("subtitle");
        certificateDetails->setWordWrap(true);
        certificateDetails->setTextInteractionFlags(
            Qt::TextSelectableByMouse
        );

        cardLayout(endpointCard)->addWidget(
            selectCertificateButton
        );
        cardLayout(endpointCard)->addWidget(
            certificateDetails
        );

        auto* assessmentCard =
            createCard(layout, "Assessment request");

        auto* assessmentRow = new QHBoxLayout;

        operation = new QComboBox;
        operation->addItems(operationLabels());

        requestButton =
            new QPushButton("Request assessment");

        cancelButton =
            new QPushButton("Cancel");

        cancelButton->setObjectName("danger");
        cancelButton->setEnabled(false);

        assessmentRow->addWidget(operation, 1);
        assessmentRow->addWidget(requestButton);
        assessmentRow->addWidget(cancelButton);

        cardLayout(assessmentCard)->addLayout(
            assessmentRow
        );

        state = new QLabel("Ready — not connected");
        state->setObjectName("status");
        state->setWordWrap(true);

        requestId = new QLabel("Request ID: none");
        requestId->setObjectName("subtitle");
        requestId->setTextInteractionFlags(
            Qt::TextSelectableByMouse
        );

        cardLayout(assessmentCard)->addWidget(state);
        cardLayout(assessmentCard)->addWidget(requestId);

        auto* resultCard =
            createCard(layout, "Assessment results");

        results = new QTreeWidget;
        results->setHeaderLabels({"Property", "Value"});
        results->setColumnWidth(0, 360);
        results->setSelectionMode(
            QAbstractItemView::ExtendedSelection
        );

        cardLayout(resultCard)->addWidget(results);

        auto* resultControls = new QHBoxLayout;

        expandButton = new QPushButton("Expand all");
        collapseButton = new QPushButton("Collapse all");
        exportButton = new QPushButton("Export JSON");

        resultControls->addWidget(expandButton);
        resultControls->addWidget(collapseButton);
        resultControls->addStretch();
        resultControls->addWidget(exportButton);

        cardLayout(resultCard)->addLayout(
            resultControls
        );

        layout->setStretch(layout->count() - 1, 1);

        connect(
            selectCertificateButton,
            &QPushButton::clicked,
            this,
            [this] {
                chooseCertificate();
            }
        );

        connect(
            requestButton,
            &QPushButton::clicked,
            this,
            [this] {
                beginRequest();
            }
        );

        connect(
            cancelButton,
            &QPushButton::clicked,
            this,
            [this] {
                cancelRequest();
            }
        );

        connect(
            expandButton,
            &QPushButton::clicked,
            results,
            &QTreeWidget::expandAll
        );

        connect(
            collapseButton,
            &QPushButton::clicked,
            results,
            &QTreeWidget::collapseAll
        );

        connect(
            exportButton,
            &QPushButton::clicked,
            this,
            [this] {
                exportResponse();
            }
        );

        statusBar()->showMessage(
            "Select the managed computer certificate before connecting."
        );
    }

    ~ConsoleWindow() override
    {
        cancelRequest();
    }

private:
    QLineEdit* host{};
    QSpinBox* port{};
    QLineEdit* token{};

    QComboBox* operation{};

    QPushButton* selectCertificateButton{};
    QPushButton* requestButton{};
    QPushButton* cancelButton{};
    QPushButton* expandButton{};
    QPushButton* collapseButton{};
    QPushButton* exportButton{};

    QLabel* certificateDetails{};
    QLabel* state{};
    QLabel* requestId{};

    QTreeWidget* results{};

    QSslCertificate trustedCertificate;
    QPointer<QSslSocket> currentSocket;

    QByteArray responseBuffer;
    QJsonObject latestResponse;
    bool completed = false;

    void setBusy(bool busy)
    {
        host->setEnabled(!busy);
        port->setEnabled(!busy);
        token->setEnabled(!busy);
        operation->setEnabled(!busy);

        selectCertificateButton->setEnabled(!busy);
        requestButton->setEnabled(!busy);
        cancelButton->setEnabled(busy);
    }

    void chooseCertificate()
    {
        const QString path =
            QFileDialog::getOpenFileName(
                this,
                "Select trusted server certificate",
                {},
                "PEM certificate (*.pem)"
            );

        if (path.isEmpty())
            return;

        QFile file(path);

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(
                this,
                "Certificate",
                "The certificate could not be opened."
            );
            return;
        }

        const QSslCertificate candidate(
            file.readAll(),
            QSsl::Pem
        );

        if (candidate.isNull()) {
            QMessageBox::warning(
                this,
                "Certificate",
                "The selected file is not a valid PEM certificate."
            );
            return;
        }

        trustedCertificate = candidate;

        const QString fingerprint =
            QString::fromLatin1(
                candidate.digest(
                    QCryptographicHash::Sha256
                ).toHex(':')
            );

        certificateDetails->setText(
            QFileInfo(path).fileName()
            + "\nSHA-256: "
            + fingerprint
        );

        certificateDetails->setToolTip(path);
    }

    void beginRequest()
    {
        if (host->text().trimmed().isEmpty()) {
            QMessageBox::warning(
                this,
                "Configuration",
                "Enter a host or IP address."
            );
            return;
        }

        if (!validToken(token->text())) {
            QMessageBox::warning(
                this,
                "Configuration",
                "The token must contain 32–128 printable ASCII "
                "characters without spaces."
            );
            return;
        }

        if (trustedCertificate.isNull()) {
            QMessageBox::warning(
                this,
                "Configuration",
                "Select the trusted server certificate first."
            );
            return;
        }

        completed = false;
        responseBuffer.clear();
        latestResponse = {};

        results->clear();
        requestId->setText("Request ID: pending");
        state->setText("Connecting securely…");

        setBusy(true);

        auto* socket = new QSslSocket(this);
        currentSocket = socket;

        socket->setReadBufferSize(ResponseLimit + 1);

        QSslConfiguration configuration =
            QSslConfiguration::defaultConfiguration();

        configuration.setProtocol(QSsl::TlsV1_2OrLater);
        configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
        configuration.setCaCertificates({
            trustedCertificate
        });

        socket->setSslConfiguration(configuration);

        auto* timeout = new QTimer(socket);
        timeout->setSingleShot(true);

        connect(
            timeout,
            &QTimer::timeout,
            this,
            [this] {
                finishRequest("Timed out — request cancelled");
            }
        );

        timeout->start(ConnectionTimeoutMs);

        connect(
            socket,
            &QSslSocket::encrypted,
            this,
            [this, socket] {
                const QByteArray expected =
                    trustedCertificate.digest(
                        QCryptographicHash::Sha256
                    );

                const QByteArray actual =
                    socket->peerCertificate().digest(
                        QCryptographicHash::Sha256
                    );

                if (actual != expected) {
                    finishRequest(
                        "Certificate identity mismatch"
                    );
                    return;
                }

                state->setText(
                    "TLS verified — waiting for local approval"
                );

                const QJsonObject request{
                    {"token", token->text()},
                    {"operation",
                        operationId(operation->currentIndex())}
                };

                socket->write(encodeFrame(request));
            }
        );

        connect(
            socket,
            &QSslSocket::sslErrors,
            this,
            [this](const QList<QSslError>& errors) {
                QStringList messages;

                for (const QSslError& error : errors)
                    messages.append(error.errorString());

                finishRequest(
                    "TLS rejected: "
                    + messages.join("; ")
                );
            }
        );

        connect(
            socket,
            &QSslSocket::errorOccurred,
            this,
            [this, socket](QAbstractSocket::SocketError error) {
                if (error ==
                    QAbstractSocket::RemoteHostClosedError) {
                    return;
                }

                finishRequest(
                    "Connection failed: "
                    + shortError(socket->errorString())
                );
            }
        );

        connect(
            socket,
            &QSslSocket::readyRead,
            this,
            [this, socket] {
                responseBuffer.append(socket->readAll());

                if (responseBuffer.size() > ResponseLimit) {
                    finishRequest(
                        "Response exceeded the size limit"
                    );
                    return;
                }

                const qsizetype newline =
                    responseBuffer.indexOf('\n');

                if (newline < 0)
                    return;

                if (newline != responseBuffer.size() - 1) {
                    finishRequest(
                        "Invalid response framing"
                    );
                    return;
                }

                QJsonParseError parseError{};

                const QJsonDocument document =
                    QJsonDocument::fromJson(
                        responseBuffer.left(newline),
                        &parseError
                    );

                if (parseError.error !=
                        QJsonParseError::NoError
                    || !document.isObject()
                    || !document.object()
                            .value("ok")
                            .isBool()) {
                    finishRequest(
                        "Invalid server response"
                    );
                    return;
                }

                completed = true;
                latestResponse = document.object();

                requestId->setText(
                    "Request ID: "
                    + latestResponse.value(
                        "request_id"
                    ).toString("not assigned")
                );

                if (!latestResponse.value("ok").toBool()) {
                    finishRequest(
                        "Denied / failed: "
                        + latestResponse.value(
                            "error"
                        ).toString("Unknown error")
                    );
                    return;
                }

                addValue(
                    nullptr,
                    "result",
                    latestResponse.value("result")
                );

                results->expandToDepth(1);

                finishRequest(
                    "Completed — approved on managed computer"
                );
            }
        );

        connect(
            socket,
            &QSslSocket::disconnected,
            this,
            [this] {
                if (!completed) {
                    finishRequest(
                        "Connection closed before a complete response"
                    );
                }
            }
        );

        socket->connectToHostEncrypted(
            host->text().trimmed(),
            static_cast<quint16>(port->value())
        );
    }

    void finishRequest(const QString& message)
    {
        state->setText(message);
        setBusy(false);

        if (currentSocket) {
            auto* socket = currentSocket.data();
            currentSocket.clear();

            socket->disconnect(this);
            socket->abort();
            socket->deleteLater();
        }
    }

    void cancelRequest()
    {
        if (currentSocket)
            finishRequest("Cancelled by administrator");
    }

    void addValue(
        QTreeWidgetItem* parent,
        const QString& name,
        const QJsonValue& value)
    {
        auto* item = parent
            ? new QTreeWidgetItem(parent)
            : new QTreeWidgetItem(results);

        item->setText(0, name);

        if (value.isObject()) {
            const QJsonObject object =
                value.toObject();

            item->setText(
                1,
                QString("%1 fields").arg(object.size())
            );

            for (auto iterator = object.constBegin();
                 iterator != object.constEnd();
                 ++iterator) {
                addValue(
                    item,
                    iterator.key(),
                    iterator.value()
                );
            }

            return;
        }

        if (value.isArray()) {
            const QJsonArray array =
                value.toArray();

            item->setText(
                1,
                QString("%1 entries").arg(array.size())
            );

            for (qsizetype index = 0;
                 index < array.size();
                 ++index) {
                addValue(
                    item,
                    QString::number(index),
                    array.at(index)
                );
            }

            return;
        }

        if (value.isBool()) {
            item->setText(
                1,
                value.toBool() ? "true" : "false"
            );
            return;
        }

        if (value.isDouble()) {
            item->setText(
                1,
                QString::number(
                    value.toDouble(),
                    'g',
                    16
                )
            );
            return;
        }

        if (value.isNull()) {
            item->setText(1, "null");
            return;
        }

        item->setText(1, value.toString());
    }

    void exportResponse()
    {
        if (latestResponse.isEmpty()) {
            QMessageBox::information(
                this,
                "Export",
                "There is no completed response to export."
            );
            return;
        }

        const QString path =
            QFileDialog::getSaveFileName(
                this,
                "Export assessment",
                "assessment.json",
                "JSON files (*.json)"
            );

        if (path.isEmpty())
            return;

        QSaveFile file(path);

        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(
                this,
                "Export",
                "The output file could not be opened."
            );
            return;
        }

        const QByteArray data =
            QJsonDocument(latestResponse)
                .toJson(QJsonDocument::Indented);

        if (file.write(data) != data.size()
            || !file.commit()) {
            QMessageBox::warning(
                this,
                "Export",
                "The assessment could not be saved."
            );
            return;
        }

        statusBar()->showMessage(
            "Assessment exported successfully",
            5000
        );
    }
};

} // namespace

QMainWindow* createConsoleWindow()
{
    return new ConsoleWindow;
}