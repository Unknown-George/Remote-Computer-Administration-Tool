#pragma once

#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <QtWidgets>

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>

namespace GDot {

inline constexpr int DefaultPort = 8443;
inline constexpr int RequestLimit = 8192;
inline constexpr int ResponseLimit = 4 * 1024 * 1024;
inline constexpr int ConnectionTimeoutMs = 90'000;

inline QStringList operationLabels()
{
    return {
        "System inventory",
        "Disk usage",
        "Process inventory"
    };
}

inline QString operationId(int index)
{
    static const QStringList ids{
        "system",
        "disk",
        "processes"
    };

    if (index < 0 || index >= ids.size())
        return {};

    return ids.at(index);
}

inline bool validOperation(const QString& operation)
{
    return operation == "system"
        || operation == "disk"
        || operation == "processes";
}

inline bool validToken(const QString& token)
{
    if (token.size() < 32 || token.size() > 128)
        return false;

    for (const QChar character : token) {
        const ushort code = character.unicode();

        if (code < 33 || code > 126)
            return false;
    }

    return true;
}

inline bool constantTimeEqual(
    const QByteArray& first,
    const QByteArray& second)
{
    if (first.size() != second.size())
        return false;

    unsigned int difference = 0;

    for (qsizetype index = 0; index < first.size(); ++index) {
        difference |=
            static_cast<unsigned char>(first.at(index))
            ^
            static_cast<unsigned char>(second.at(index));
    }

    return difference == 0;
}

inline QByteArray encodeFrame(const QJsonObject& object)
{
    return QJsonDocument(object)
        .toJson(QJsonDocument::Compact)
        + '\n';
}

inline QString generateToken()
{
    QByteArray bytes(32, Qt::Uninitialized);

    QRandomGenerator* generator = QRandomGenerator::system();

    for (qsizetype index = 0; index < bytes.size(); ++index)
        bytes[index] = static_cast<char>(generator->generate() & 0xff);

    return QString::fromLatin1(bytes.toHex());
}

inline QString shortError(const QString& error)
{
    return error.left(400);
}

inline void applyTheme(QApplication& application)
{
    application.setStyle("Fusion");
    application.setFont(QFont("Segoe UI", 10));

    application.setStyleSheet(R"(
        QWidget {
            background: #0e1522;
            color: #edf4ff;
        }

        QMainWindow {
            background: #0e1522;
        }

        QFrame#card {
            background: #172235;
            border: 1px solid #2a3a53;
            border-radius: 12px;
        }

        QFrame#feature {
            background: #142033;
            border: 1px solid #2b3b54;
            border-radius: 10px;
        }

        QLabel {
            background: transparent;
        }

        QLabel#title {
            font-size: 27px;
            font-weight: 700;
            color: #f4f8ff;
        }

        QLabel#subtitle {
            color: #9aacc5;
        }

        QLabel#status {
            color: #6ee7b7;
            font-weight: 700;
        }

        QLabel#brand {
            font-size: 43px;
            font-weight: 800;
            color: #f4f8ff;
        }

        QLabel#eyebrow {
            color: #9aacc5;
            font-size: 11px;
            font-weight: 700;
        }

        QLabel#assurance {
            background: #11302f;
            border: 1px solid #24534e;
            border-radius: 8px;
            color: #7ff2dc;
            font-size: 11px;
            font-weight: 700;
            padding: 12px;
        }

        QLabel#featureTitle {
            color: #7fe4d5;
            font-size: 11px;
            font-weight: 700;
        }

        QLabel#featureText {
            color: #aebdd1;
        }

        QLineEdit,
        QComboBox,
        QSpinBox,
        QPlainTextEdit,
        QTreeWidget {
            background: #0b121e;
            border: 1px solid #344761;
            border-radius: 7px;
            padding: 8px;
            selection-background-color: #2563eb;
        }

        QLineEdit:focus,
        QComboBox:focus,
        QSpinBox:focus,
        QPlainTextEdit:focus,
        QTreeWidget:focus {
            border: 1px solid #4f8cff;
        }

        QPushButton {
            background: #167f78;
            border: none;
            border-radius: 7px;
            color: #f1fffc;
            font-weight: 700;
            padding: 10px 16px;
        }

        QPushButton:hover {
            background: #1c9b90;
        }

        QPushButton#secondary {
            background: #28436d;
        }

        QPushButton#secondary:hover {
            background: #365888;
        }

        QPushButton#danger {
            background: #ae354b;
        }

        QPushButton#danger:hover {
            background: #cc435b;
        }

        QPushButton:disabled {
            background: #27364d;
            color: #7589a4;
        }

        QHeaderView::section {
            background: #22324b;
            border: none;
            color: #c5d3e5;
            padding: 9px;
        }

        QTreeWidget::item {
            padding: 5px;
        }

        QStatusBar {
            color: #96a9c2;
        }

        QCheckBox {
            spacing: 7px;
        }
    )");
}

inline QFrame* createCard(
    QVBoxLayout* parent,
    const QString& title)
{
    auto* frame = new QFrame;
    frame->setObjectName("card");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto* heading = new QLabel(title);
    QFont font = heading->font();
    font.setBold(true);
    font.setPointSize(12);
    heading->setFont(font);

    layout->addWidget(heading);
    parent->addWidget(frame);

    return frame;
}

inline QVBoxLayout* cardLayout(QFrame* frame)
{
    return qobject_cast<QVBoxLayout*>(frame->layout());
}

inline void addHeader(
    QVBoxLayout* layout,
    const QString& title,
    const QString& subtitle)
{
    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("title");
    layout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(subtitle);
    subtitleLabel->setObjectName("subtitle");
    subtitleLabel->setWordWrap(true);
    layout->addWidget(subtitleLabel);
}

} // namespace GDot