#include "common.hpp"

using namespace GDot;

QMainWindow* createManagedWindow();
QMainWindow* createConsoleWindow();

namespace {

void paintLogo(
    QPainter& painter,
    const QRectF& bounds)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal side =
        qMin(bounds.width(), bounds.height());

    painter.translate(
        bounds.center().x() - side / 2.0,
        bounds.center().y() - side / 2.0
    );

    painter.scale(side / 100.0, side / 100.0);

    QLinearGradient background(
        0,
        0,
        100,
        100
    );

    background.setColorAt(0, QColor("#2a405f"));
    background.setColorAt(1, QColor("#101827"));

    painter.setPen(
        QPen(QColor("#405675"), 1.5)
    );
    painter.setBrush(background);

    painter.drawRoundedRect(
        QRectF(2, 2, 96, 96),
        23,
        23
    );

    QPainterPath g;
    g.moveTo(70, 31);
    g.cubicTo(60, 19, 35, 21, 27, 37);
    g.cubicTo(16, 59, 28, 78, 49, 78);
    g.cubicTo(63, 78, 73, 69, 74, 56);
    g.lineTo(53, 56);

    QPen letterPen(
        QColor("#f0f6ff"),
        8
    );

    letterPen.setCapStyle(Qt::RoundCap);
    letterPen.setJoinStyle(Qt::RoundJoin);

    painter.setPen(letterPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(g);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#5eead4"));
    painter.drawEllipse(
        QPointF(78, 78),
        8,
        8
    );

    painter.restore();
}

QIcon createBrandIcon()
{
    QPixmap pixmap(256, 256);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    paintLogo(
        painter,
        QRectF(0, 0, 256, 256)
    );

    painter.end();

    return QIcon(pixmap);
}

class LogoWidget final : public QWidget {
public:
    explicit LogoWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(104, 104);
        setAccessibleName("G DOT logo");
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        paintLogo(painter, rect());
    }
};

class FeatureCard final : public QFrame {
public:
    FeatureCard(
        const QString& title,
        const QString& description,
        QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setObjectName("feature");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(7);

        auto* heading = new QLabel(title);
        heading->setObjectName("featureTitle");

        auto* text = new QLabel(description);
        text->setObjectName("featureText");
        text->setWordWrap(true);

        layout->addWidget(heading);
        layout->addWidget(text);
    }
};

class Launcher final : public QMainWindow {
public:
    Launcher()
    {
        setWindowTitle("G DOT | Windows Administration");
        setWindowIcon(createBrandIcon());
        resize(1000, 780);
        setMinimumSize(850, 680);

        auto* root = new QWidget;
        auto* layout = new QVBoxLayout(root);

        layout->setContentsMargins(32, 30, 32, 26);
        layout->setSpacing(20);

        setCentralWidget(root);

        auto* masthead = new QHBoxLayout;
        masthead->setSpacing(22);

        masthead->addWidget(new LogoWidget);

        auto* brandLayout = new QVBoxLayout;
        brandLayout->setSpacing(6);

        auto* brand = new QLabel("G DOT");
        brand->setObjectName("brand");

        auto* eyebrow = new QLabel(
            "WINDOWS ADMINISTRATION & ASSESSMENT"
        );
        eyebrow->setObjectName("eyebrow");

        auto* slogan = new QLabel(
            "Encrypted. Visible. Approved."
        );
        slogan->setObjectName("subtitle");

        brandLayout->addWidget(brand);
        brandLayout->addWidget(eyebrow);
        brandLayout->addWidget(slogan);
        brandLayout->addStretch();

        masthead->addLayout(brandLayout, 1);
        layout->addLayout(masthead);

        auto* assurance = new QLabel(
            "READ-ONLY ASSESSMENT  •  LOCAL APPROVAL  •  "
            "TLS IDENTITY VERIFICATION"
        );
        assurance->setObjectName("assurance");
        assurance->setWordWrap(true);

        layout->addWidget(assurance);

        auto* workspaceCard =
            createCard(layout, "Choose your workspace");

        auto* explanation = new QLabel(
            "Run Managed Computer on the Windows endpoint. "
            "Run Administration Console on the administrator computer. "
            "Both modes may run together for a local demonstration."
        );
        explanation->setObjectName("subtitle");
        explanation->setWordWrap(true);

        cardLayout(workspaceCard)->addWidget(
            explanation
        );

        auto* buttons = new QHBoxLayout;
        buttons->setSpacing(14);

        auto* managedButton = new QPushButton(
            "MANAGED COMPUTER\n"
            "Approve incoming assessments"
        );
        managedButton->setMinimumHeight(88);

        auto* consoleButton = new QPushButton(
            "ADMINISTRATION CONSOLE\n"
            "Request and inspect assessments"
        );
        consoleButton->setObjectName("secondary");
        consoleButton->setMinimumHeight(88);

        buttons->addWidget(managedButton);
        buttons->addWidget(consoleButton);

        cardLayout(workspaceCard)->addLayout(buttons);

        connect(
            managedButton,
            &QPushButton::clicked,
            this,
            [this] {
                openManaged();
            }
        );

        connect(
            consoleButton,
            &QPushButton::clicked,
            this,
            [this] {
                openConsole();
            }
        );

        auto* features = new QGridLayout;
        features->setHorizontalSpacing(14);
        features->setVerticalSpacing(14);

        features->addWidget(
            new FeatureCard(
                "SYSTEM INVENTORY",
                "Windows version, kernel, architecture, CPU "
                "count, physical memory and memory load."
            ),
            0,
            0
        );

        features->addWidget(
            new FeatureCard(
                "PROCESS ASSESSMENT",
                "Native Windows process enumeration with process "
                "IDs, parent IDs and thread counts."
            ),
            0,
            1
        );

        features->addWidget(
            new FeatureCard(
                "VERIFIED CONNECTIONS",
                "TLS encryption, certificate identity validation "
                "and a randomly generated session token."
            ),
            1,
            0
        );

        features->addWidget(
            new FeatureCard(
                "ACCOUNTABLE ACCESS",
                "Approval for every request, visible stop control, "
                "local audit records and JSON export."
            ),
            1,
            1
        );

        layout->addLayout(features);
        layout->addStretch();

        auto* footer = new QLabel(
            "Keep the managed interface visible. Use a trusted LAN "
            "or VPN. Closing this launcher closes its workspaces."
        );
        footer->setObjectName("subtitle");
        footer->setWordWrap(true);

        layout->addWidget(footer);

        statusBar()->showMessage(
            "G DOT 1.0 | No background service | No automatic startup"
        );
    }

protected:
    void closeEvent(QCloseEvent* event) override
    {
        if (managedWindow)
            managedWindow->close();

        if (consoleWindow)
            consoleWindow->close();

        event->accept();
    }

private:
    QPointer<QMainWindow> managedWindow;
    QPointer<QMainWindow> consoleWindow;

    void openManaged()
    {
        if (!managedWindow) {
            managedWindow = createManagedWindow();
            managedWindow->setAttribute(
                Qt::WA_DeleteOnClose
            );

            connect(
                managedWindow,
                &QObject::destroyed,
                this,
                [this] {
                    managedWindow.clear();
                }
            );
        }

        managedWindow->show();
        managedWindow->raise();
        managedWindow->activateWindow();
    }

    void openConsole()
    {
        if (!consoleWindow) {
            consoleWindow = createConsoleWindow();
            consoleWindow->setAttribute(
                Qt::WA_DeleteOnClose
            );

            connect(
                consoleWindow,
                &QObject::destroyed,
                this,
                [this] {
                    consoleWindow.clear();
                }
            );
        }

        consoleWindow->show();
        consoleWindow->raise();
        consoleWindow->activateWindow();
    }
};

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    application.setOrganizationName("GDotProject");
    application.setApplicationName("G-DOT");
    application.setApplicationVersion("1.0.0");
    application.setWindowIcon(createBrandIcon());

    applyTheme(application);

    if (!QSslSocket::supportsSsl()) {
        QMessageBox::critical(
            nullptr,
            "G DOT | TLS unavailable",
            "A usable Qt TLS backend was not found. "
            "Deploy the Qt TLS plugins before running G DOT."
        );

        return 1;
    }

    Launcher launcher;
    launcher.show();

    return application.exec();
}