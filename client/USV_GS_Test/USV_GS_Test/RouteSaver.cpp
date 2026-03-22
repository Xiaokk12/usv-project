#include "RouteSaver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>
#include <QRegularExpression>
#include <QFileInfo>
#include <QUrl>
#include <QStringList>

#include <cmath>

namespace {

QString htmlEscape(const QString &value)
{
    QString s = value;
    s.replace('&', "&amp;");
    s.replace('<', "&lt;");
    s.replace('>', "&gt;");
    s.replace('"', "&quot;");
    s.replace('\'', "&#39;");
    return s;
}

void appendHtmlRow(QStringList &lines, const QStringList &cols, bool header)
{
    QString row = "<tr>";
    const QString tag = header ? "th" : "td";
    for (const auto &col : cols) {
        row += "<" + tag + ">" + htmlEscape(col) + "</" + tag + ">";
    }
    row += "</tr>";
    lines << row;
}

QString normalizeExportPath(const QString &filePath)
{
    QString normalized = filePath.trimmed();
    if (normalized.startsWith("file:")) {
        QUrl url(normalized);
        if (url.isLocalFile())
            normalized = url.toLocalFile();
    }
    return normalized;
}

bool extractPointLngLat(const QJsonValue &value, double &lng, double &lat)
{
    if (!value.isObject())
        return false;
    const QJsonObject obj = value.toObject();
    if (obj.contains("lng") && obj.contains("lat")) {
        lng = obj.value("lng").toDouble();
        lat = obj.value("lat").toDouble();
        return true;
    }
    if (obj.contains("lon") && obj.contains("lat")) {
        lng = obj.value("lon").toDouble();
        lat = obj.value("lat").toDouble();
        return true;
    }
    return false;
}

QString jsonIntString(const QJsonValue &value)
{
    if (value.isDouble())
        return QString::number(static_cast<qint64>(std::llround(value.toDouble())));
    if (value.isString())
        return value.toString();
    return {};
}

QString jsonDoubleString(const QJsonValue &value, int precision)
{
    if (value.isDouble())
        return QString::number(value.toDouble(), 'f', precision);
    if (value.isString())
        return value.toString();
    return {};
}

QString envValueString(const QJsonObject &sp, const QString &key, int precision)
{
    if (!sp.contains("env") || !sp.value("env").isObject())
        return {};
    const QJsonObject env = sp.value("env").toObject();
    if (!env.contains(key))
        return {};
    const QJsonValue val = env.value(key);
    if (val.isDouble())
        return QString::number(val.toDouble(), 'f', precision);
    if (val.isString())
        return val.toString();
    if (val.isObject()) {
        const QJsonObject obj = val.toObject();
        const QJsonValue v = obj.value("value");
        if (v.isDouble())
            return QString::number(v.toDouble(), 'f', precision);
        if (v.isString())
            return v.toString();
    }
    return {};
}

} // namespace

RouteSaver::RouteSaver(QObject *parent)
    : QObject(parent)
{
}

QString RouteSaver::routesDirPath() const
{
    // 用户可写目录，避免打包后只读
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty())
        basePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (basePath.isEmpty())
        basePath = QCoreApplication::applicationDirPath();

    QDir dir(basePath);
    if (!dir.exists() && !dir.mkpath(".")) {
        return basePath;
    }
    if (!dir.exists("routes") && !dir.mkdir("routes")) {
        return basePath;
    }
    if (!dir.cd("routes")) {
        return basePath;
    }
    return dir.absolutePath();
}

QString RouteSaver::tasksDirPath() const
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty())
        basePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (basePath.isEmpty())
        basePath = QCoreApplication::applicationDirPath();

    QDir dir(basePath);
    if (!dir.exists() && !dir.mkpath(".")) {
        return basePath;
    }
    if (!dir.exists("tasks") && !dir.mkdir("tasks")) {
        return basePath;
    }
    if (!dir.cd("tasks")) {
        return basePath;
    }
    return dir.absolutePath();
}

bool RouteSaver::saveRoute(const QString &jsonString)
{
    // 1. 解析 JSON 字符串
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull() || !doc.isArray()) {
        qWarning() << "RouteSaver: invalid JSON received!";
        return false;
    }

    QJsonArray points = doc.array();
    if (points.isEmpty()) {
        qWarning() << "RouteSaver: empty route, not saving.";
        return false;
    }

    // 2. routes 目录
    QDir dir(routesDirPath());

    // 3. 生成文件名
    const QString timestamp =
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString fileName = QString("route_%1.json").arg(timestamp);
    QFile file(dir.filePath(fileName));

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "RouteSaver: cannot open file" << file.fileName();
        return false;
    }

    // 4. 组装 JSON 内容
    QJsonObject root;
    root["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["points"] = points;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "RouteSaver: route saved to" << file.fileName();
    return true;
}

bool RouteSaver::saveTask(const QString &taskName, const QString &jsonString)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull()) {
        qWarning() << "RouteSaver: invalid task JSON received!";
        return false;
    }

    QJsonArray points;
    QJsonArray samplingPoints;
    QJsonObject samplingSettings;
    QString payloadTime;
    QString payloadLocation;
    QString payloadName;

    if (doc.isArray()) {
        points = doc.array();
    } else if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        if (obj.contains("points") && obj.value("points").isArray())
            points = obj.value("points").toArray();
        if (obj.contains("samplingPoints") && obj.value("samplingPoints").isArray())
            samplingPoints = obj.value("samplingPoints").toArray();
        else if (obj.contains("sampling_points") && obj.value("sampling_points").isArray())
            samplingPoints = obj.value("sampling_points").toArray();
        if (obj.contains("samplingSettings") && obj.value("samplingSettings").isObject())
            samplingSettings = obj.value("samplingSettings").toObject();
        else if (obj.contains("sampling_settings") && obj.value("sampling_settings").isObject())
            samplingSettings = obj.value("sampling_settings").toObject();
        if (obj.contains("time") && obj.value("time").isString())
            payloadTime = obj.value("time").toString();
        if (obj.contains("location") && obj.value("location").isString())
            payloadLocation = obj.value("location").toString();
        if (obj.contains("name") && obj.value("name").isString())
            payloadName = obj.value("name").toString();
    } else {
        qWarning() << "RouteSaver: task JSON neither array nor object.";
        return false;
    }

    if (points.isEmpty()) {
        qWarning() << "RouteSaver: empty task route, not saving.";
        return false;
    }

    double sumLng = 0.0;
    double sumLat = 0.0;
    int count = 0;
    for (const auto &v : points) {
        if (!v.isObject())
            continue;
        const QJsonObject obj = v.toObject();
        if (obj.contains("lng") && obj.contains("lat")) {
            sumLng += obj.value("lng").toDouble();
            sumLat += obj.value("lat").toDouble();
            ++count;
        }
    }
    if (count == 0) {
        qWarning() << "RouteSaver: task points missing lng/lat.";
        return false;
    }

    const double centerLng = sumLng / count;
    const double centerLat = sumLat / count;
    QString locationStr = QString("%1,%2")
                              .arg(centerLng, 0, 'f', 5)
                              .arg(centerLat, 0, 'f', 5);
    if (!payloadLocation.isEmpty())
        locationStr = payloadLocation;

    QString safeName = taskName.trimmed();
    if (safeName.isEmpty())
        safeName = payloadName.trimmed();
    if (safeName.isEmpty())
        safeName = "task";
    QString displayName = taskName.isEmpty() ? payloadName : taskName;
    if (displayName.trimmed().isEmpty())
        displayName = safeName;

    auto sanitize = [](QString s) {
        s.replace(QRegularExpression(R"([\\/\:\*\?\"<>\| ]+)"), "_");
        return s;
    };

    const QString fileSafeName = sanitize(safeName);
    const QString fileSafeLocation = sanitize(locationStr);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString fileName = QString("%1-%2-%3.json")
                                 .arg(fileSafeName, fileSafeLocation, timestamp);

    QDir dir(tasksDirPath());
    QFile file(dir.filePath(fileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "RouteSaver: cannot open task file" << file.fileName();
        return false;
    }

    QJsonObject root;
    root["name"] = displayName;
    root["location"] = locationStr;
    root["centroid"] = QJsonObject{
        { "lng", centerLng },
        { "lat", centerLat }
    };
    root["time"] = payloadTime.isEmpty()
                       ? QDateTime::currentDateTime().toString(Qt::ISODate)
                       : payloadTime;
    root["points"] = points;
    if (!samplingPoints.isEmpty())
        root["sampling_points"] = samplingPoints;
    if (!samplingSettings.isEmpty())
        root["sampling_settings"] = samplingSettings;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "RouteSaver: task saved to" << file.fileName();
    return true;
}

bool RouteSaver::exportTaskToExcel(const QString &jsonString, const QString &filePath)
{
    if (jsonString.trimmed().isEmpty()) {
        qWarning() << "RouteSaver: empty task JSON for export.";
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull()) {
        qWarning() << "RouteSaver: invalid task JSON for export.";
        return false;
    }

    QJsonObject root;
    if (doc.isObject()) {
        root = doc.object();
    } else if (doc.isArray()) {
        root.insert("points", doc.array());
    } else {
        qWarning() << "RouteSaver: task export JSON neither array nor object.";
        return false;
    }

    QJsonArray points = root.value("points").toArray();
    if (points.isEmpty()) {
        qWarning() << "RouteSaver: task export missing points.";
        return false;
    }

    QString name = root.value("name").toString();
    QString location = root.value("location").toString();
    QString time = root.value("time").toString();

    QJsonArray samplingPoints;
    if (root.contains("samplingPoints") && root.value("samplingPoints").isArray())
        samplingPoints = root.value("samplingPoints").toArray();
    else if (root.contains("sampling_points") && root.value("sampling_points").isArray())
        samplingPoints = root.value("sampling_points").toArray();

    QJsonObject samplingSettings;
    if (root.contains("samplingSettings") && root.value("samplingSettings").isObject())
        samplingSettings = root.value("samplingSettings").toObject();
    else if (root.contains("sampling_settings") && root.value("sampling_settings").isObject())
        samplingSettings = root.value("sampling_settings").toObject();

    double sumLng = 0.0;
    double sumLat = 0.0;
    int count = 0;
    for (const auto &v : points) {
        double lng = 0.0;
        double lat = 0.0;
        if (extractPointLngLat(v, lng, lat)) {
            sumLng += lng;
            sumLat += lat;
            ++count;
        }
    }
    if (count == 0) {
        qWarning() << "RouteSaver: task export points missing lng/lat.";
        return false;
    }

    const double centerLng = sumLng / count;
    const double centerLat = sumLat / count;

    if (location.trimmed().isEmpty()) {
        location = QString("%1,%2")
                       .arg(centerLng, 0, 'f', 5)
                       .arg(centerLat, 0, 'f', 5);
    }
    if (time.trimmed().isEmpty())
        time = QDateTime::currentDateTime().toString(Qt::ISODate);
    if (name.trimmed().isEmpty())
        name = "task";

    QString outputPath = normalizeExportPath(filePath);
    if (outputPath.isEmpty()) {
        qWarning() << "RouteSaver: empty export path.";
        return false;
    }
    QFileInfo info(outputPath);
    const QString suffix = info.suffix().toLower();
    if (suffix != "xls") {
        const QString base = info.completeBaseName().isEmpty() ? "task" : info.completeBaseName();
        outputPath = QDir(info.absolutePath()).filePath(base + ".xls");
        info = QFileInfo(outputPath);
    }

    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        qWarning() << "RouteSaver: cannot create export dir" << dir.absolutePath();
        return false;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "RouteSaver: cannot open export file" << file.fileName();
        return false;
    }

    QStringList lines;
    lines << "<html><head><meta charset=\"utf-8\">";
    lines << "<style>"
             "table{border-collapse:collapse;margin:6px 0 16px 0;}"
             "td,th{border:1px solid #000;padding:4px 6px;text-align:left;}"
             "</style></head><body>";

    lines << "<div><b>任务信息</b></div>";
    lines << "<table>";
    appendHtmlRow(lines, {
        "任务名称",
        "位置",
        "时间",
        "中心经度",
        "中心纬度",
        "航点数量",
        "采样点数量"
    }, true);
    appendHtmlRow(lines, {
        name,
        location,
        time,
        QString::number(centerLng, 'f', 6),
        QString::number(centerLat, 'f', 6),
        QString::number(points.size()),
        QString::number(samplingPoints.size())
    }, false);
    lines << "</table>";

    lines << "<div><b>采样设置</b></div>";
    lines << "<table>";
    appendHtmlRow(lines, {
        "每点瓶数",
        "每瓶容量(L)",
        "采样深度(m)"
    }, true);

    const QJsonValue bottlesValue = samplingSettings.contains("bottlesPerPoint")
        ? samplingSettings.value("bottlesPerPoint")
        : samplingSettings.value("bottles_per_point");
    const QJsonValue volumeValue = samplingSettings.contains("volumePerBottle")
        ? samplingSettings.value("volumePerBottle")
        : samplingSettings.value("volume_per_bottle");
    const QJsonValue depthValue = samplingSettings.contains("depthMeters")
        ? samplingSettings.value("depthMeters")
        : samplingSettings.value("depth_meters");

    appendHtmlRow(lines, {
        jsonIntString(bottlesValue),
        jsonDoubleString(volumeValue, 3),
        jsonDoubleString(depthValue, 3)
    }, false);
    lines << "</table>";

    lines << "<div><b>航线点</b></div>";
    lines << "<table>";
    appendHtmlRow(lines, { "序号", "经度", "纬度" }, true);
    for (int i = 0; i < points.size(); ++i) {
        double lng = 0.0;
        double lat = 0.0;
        if (!extractPointLngLat(points[i], lng, lat))
            continue;
        appendHtmlRow(lines, {
            QString::number(i + 1),
            QString::number(lng, 'f', 6),
            QString::number(lat, 'f', 6)
        }, false);
    }
    lines << "</table>";

    lines << "<div><b>采样点</b></div>";
    lines << "<table>";
    appendHtmlRow(lines, {
        "序号",
        "航点序号",
        "位点",
        "经度",
        "纬度",
        "瓶号",
        "容量(L)",
        "深度(m)",
        "时间",
        "温度(℃)",
        "盐碱度(‰)",
        "pH",
        "叶绿素(ug/L)"
    }, true);

    for (int i = 0; i < samplingPoints.size(); ++i) {
        if (!samplingPoints[i].isObject())
            continue;
        const QJsonObject sp = samplingPoints[i].toObject();

        int pointIndex = -1;
        if (sp.contains("pointIndex"))
            pointIndex = sp.value("pointIndex").toInt(-1);
        else if (sp.contains("index"))
            pointIndex = sp.value("index").toInt(-1) + 1;

        QString orderStr = jsonIntString(sp.value("order"));
        if (orderStr.isEmpty() && pointIndex > 0)
            orderStr = QString::number(pointIndex);

        double lng = 0.0;
        double lat = 0.0;
        bool hasLngLat = extractPointLngLat(sp, lng, lat);
        if (!hasLngLat && pointIndex > 0 && pointIndex <= points.size())
            hasLngLat = extractPointLngLat(points[pointIndex - 1], lng, lat);

        const QString pointIndexStr = pointIndex > 0
            ? QString::number(pointIndex)
            : jsonIntString(sp.value("pointIndex"));

        appendHtmlRow(lines, {
            orderStr,
            pointIndexStr,
            jsonIntString(sp.value("site")),
            hasLngLat ? QString::number(lng, 'f', 6) : QString(),
            hasLngLat ? QString::number(lat, 'f', 6) : QString(),
            jsonIntString(sp.value("bottle")),
            jsonDoubleString(sp.value("volume"), 3),
            jsonDoubleString(sp.value("depth"), 3),
            sp.value("time").toString(),
            envValueString(sp, "temperature", 1),
            envValueString(sp, "salinity", 1),
            envValueString(sp, "ph", 2),
            envValueString(sp, "chlorophyll", 1)
        }, false);
    }
    lines << "</table>";
    lines << "</body></html>";

    const QByteArray output = lines.join("\n").toUtf8();
    file.write("\xEF\xBB\xBF", 3);
    file.write(output);
    file.close();

    qDebug() << "RouteSaver: task exported to" << file.fileName();
    return true;
}

QStringList RouteSaver::listRoutes() const
{
    QDir dir(routesDirPath());
    if (!dir.exists())
        return {};

    QStringList filters;
    filters << "route_*.json";
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Time | QDir::Name);

    return dir.entryList(QDir::Files, QDir::Time | QDir::Name);
}

QStringList RouteSaver::listTasks() const
{
    QDir dir(tasksDirPath());
    if (!dir.exists())
        return {};

    QStringList filters;
    filters << "*.json";
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Time | QDir::Name);

    return dir.entryList(QDir::Files, QDir::Time | QDir::Name);
}

QString RouteSaver::loadRoute(const QString &fileName) const
{
    if (fileName.isEmpty())
        return {};

    QDir dir(routesDirPath());
    QFile file(dir.filePath(fileName));
    if (!file.exists()) {
        qWarning() << "RouteSaver: file not found" << file.fileName();
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "RouteSaver: cannot open" << file.fileName();
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QString RouteSaver::loadTask(const QString &fileName) const
{
    if (fileName.isEmpty())
        return {};

    QDir dir(tasksDirPath());
    QFile file(dir.filePath(fileName));
    if (!file.exists()) {
        qWarning() << "RouteSaver: task file not found" << file.fileName();
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "RouteSaver: cannot open task file" << file.fileName();
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool RouteSaver::deleteRoute(const QString &fileName) const
{
    if (fileName.isEmpty())
        return false;

    QDir dir(routesDirPath());
    QFile file(dir.filePath(fileName));
    if (!file.exists()) {
        qWarning() << "RouteSaver: file not found for delete" << file.fileName();
        return false;
    }
    if (!file.remove()) {
        qWarning() << "RouteSaver: failed to delete" << file.fileName();
        return false;
    }
    return true;
}

bool RouteSaver::deleteTask(const QString &fileName) const
{
    if (fileName.isEmpty())
        return false;

    QDir dir(tasksDirPath());
    QFile file(dir.filePath(fileName));
    if (!file.exists()) {
        qWarning() << "RouteSaver: task file not found for delete" << file.fileName();
        return false;
    }
    if (!file.remove()) {
        qWarning() << "RouteSaver: failed to delete task" << file.fileName();
        return false;
    }
    return true;
}
