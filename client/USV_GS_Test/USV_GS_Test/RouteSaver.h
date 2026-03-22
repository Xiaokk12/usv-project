#ifndef ROUTESAVER_H
#define ROUTESAVER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class RouteSaver : public QObject
{
    Q_OBJECT

public:
    explicit RouteSaver(QObject *parent = nullptr);

    // JS: RouteSaver.saveRoute(JSON.stringify(routeData));
    Q_INVOKABLE bool saveRoute(const QString &jsonPoints);
    Q_INVOKABLE bool saveTask(const QString &taskName, const QString &jsonPoints);
    // Export task payload as CSV (Excel-readable).
    Q_INVOKABLE bool exportTaskToExcel(const QString &jsonString, const QString &filePath);

    // 列出 routes 目录下的所有航线文件名
    Q_INVOKABLE QStringList listRoutes() const;
    // 列出 tasks 目录下的所有任务文件名
    Q_INVOKABLE QStringList listTasks() const;

    // 读取指定航线文件内容（返回 JSON 字符串），失败返回空串
    Q_INVOKABLE QString loadRoute(const QString &fileName) const;
    // 读取任务文件
    Q_INVOKABLE QString loadTask(const QString &fileName) const;

    // 删除指定航线文件，返回是否成功
    Q_INVOKABLE bool deleteRoute(const QString &fileName) const;
    // 删除任务文件
    Q_INVOKABLE bool deleteTask(const QString &fileName) const;

private:
    QString routesDirPath() const;
    QString tasksDirPath() const;
};

#endif // ROUTESAVER_H
