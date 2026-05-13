#include"BootstrapPaths.h"

#include<QCoreApplication>
#include<QDir>
#include<QFileInfo>

#include<stdexcept>
#include<string>

namespace {
    QString requireExistingFile(const QString& path, const char* description) {
        const QString cleanPath = QDir::cleanPath(path);
        if (!QFileInfo::exists(cleanPath)) {
            throw std::runtime_error(std::string(description) + " not found at " + cleanPath.toStdString());
        }
        return cleanPath;
    }

    QString executableDatabaseDirectory() {
        return QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/database"));
    }
}

namespace BootstrapPaths {
    QString schemaAssetPath() {
        return requireExistingFile(
            executableDatabaseDirectory() + QStringLiteral("/schema.sql"),
            "SQLite schema asset");
    }

    QString seedAssetPath() {
        return requireExistingFile(
            executableDatabaseDirectory() + QStringLiteral("/seed.sql"),
            "SQLite seed asset");
    }

}
