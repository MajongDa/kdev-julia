#include "projectconfigpage.h"
#include "ui_projectconfig.h"
#include "kdevjuliaversion.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QMutexLocker>

namespace Julia {

ProjectConfigPage::ProjectConfigPage(KDevelop::IPlugin* self, const KDevelop::ProjectConfigOptions& options, QWidget* parent)
    : KDevelop::ConfigPage(self, nullptr, parent)
    , m_ui(new Ui_ProjectConfig)
{
    m_configGroup = options.project->projectConfiguration()->group(QStringLiteral("Juliasupport"));
    m_ui->setupUi(this);
    m_project = options.project;
    // So apply button activates
    connect(m_ui->JuliaInterpreter, &QLineEdit::textChanged, this, &ProjectConfigPage::changed);
    connect(m_ui->GraphicsFileDir, &QLineEdit::textChanged, this, &ProjectConfigPage::changed);
    connect(m_ui->SocketHost, &QLineEdit::textChanged, this, &ProjectConfigPage::changed);
    connect(m_ui->SocketPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &ProjectConfigPage::changed);
    connect(m_ui->GraphicsMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProjectConfigPage::changed);
}

void Julia::ProjectConfigPage::apply()
{
    m_configGroup.writeEntry("interpreter", m_ui->JuliaInterpreter->text());
    m_configGroup.writeEntry("graphicsFileDir", m_ui->GraphicsFileDir->text());
    m_configGroup.writeEntry("socketHost", m_ui->SocketHost->text());
    m_configGroup.writeEntry("socketPort", m_ui->SocketPort->value());
    m_configGroup.writeEntry("graphicsMethod", m_ui->GraphicsMethod->currentIndex());
    // remove cached paths, so they are updated on the next parse job run
    // QMutexLocker lock(&Helper::cacheMutex);
    // Helper::cachedSearchPaths.remove(m_project);
}

void Julia::ProjectConfigPage::defaults()
{
    // Use default Julia executable as default
    m_ui->JuliaInterpreter->setText(QStringLiteral(JULIA_EXECUTABLE));
    m_ui->GraphicsFileDir->setText(QStringLiteral("/tmp/graphics"));
    m_ui->SocketHost->setText(QStringLiteral("localhost"));
    m_ui->SocketPort->setValue(8080);
    m_ui->GraphicsMethod->setCurrentIndex(0); // File-based by default
}

void Julia::ProjectConfigPage::reset()
{
    QString interpreter = m_configGroup.readEntry("interpreter");
    if (interpreter.isEmpty()) {
        // Use default Julia executable if none configured
        interpreter = QStringLiteral(JULIA_EXECUTABLE);
    }
    m_ui->JuliaInterpreter->setText(interpreter);
    m_ui->GraphicsFileDir->setText(m_configGroup.readEntry("graphicsFileDir", QStringLiteral("/tmp/graphics")));
    m_ui->SocketHost->setText(m_configGroup.readEntry("socketHost", QStringLiteral("localhost")));
    m_ui->SocketPort->setValue(m_configGroup.readEntry("socketPort", 8080));
    m_ui->GraphicsMethod->setCurrentIndex(m_configGroup.readEntry("graphicsMethod", 0));
}

QString Julia::ProjectConfigPage::name() const
{
    return i18n("Julia Settings");
}

}


