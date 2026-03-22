#include "EnvironmentDataProvider.h"

#include <QRandomGenerator>
#include <cmath>

EnvironmentDataProvider::EnvironmentDataProvider(QObject *parent)
    : QObject(parent)
{
    m_params = {
        { "temperature", "温度", "℃", 10.0, 35.0, 1 },
        { "salinity", "盐碱度", "‰", 0.0, 35.0, 1 },
        { "ph", "pH", "", 6.5, 8.5, 2 },
        { "chlorophyll", "叶绿素", "ug/L", 0.1, 25.0, 1 }
    };
}

QVariantMap EnvironmentDataProvider::generateRandom()
{
    return normalize(QVariantMap());
}

QVariantMap EnvironmentDataProvider::normalize(const QVariantMap &data)
{
    QVariantMap out;
    for (const auto &meta : m_params) {
        out.insert(meta.key, normalizeItem(data.value(meta.key), meta));
    }
    return out;
}

double EnvironmentDataProvider::randomValue(const ParamMeta &meta) const
{
    const double span = meta.max - meta.min;
    const double raw = meta.min + QRandomGenerator::global()->generateDouble() * span;
    const double factor = std::pow(10.0, meta.decimals);
    return std::round(raw * factor) / factor;
}

QVariantMap EnvironmentDataProvider::normalizeItem(const QVariant &value, const ParamMeta &meta) const
{
    bool ok = false;
    double number = 0.0;
    QString unit = meta.unit;
    QString label = meta.label;
    int decimals = meta.decimals;

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        if (map.contains("value")) {
            number = map.value("value").toDouble(&ok);
        }
        if (map.contains("unit") && map.value("unit").canConvert<QString>()) {
            const QString customUnit = map.value("unit").toString();
            if (!customUnit.isEmpty())
                unit = customUnit;
        }
        if (map.contains("label") && map.value("label").canConvert<QString>()) {
            const QString customLabel = map.value("label").toString();
            if (!customLabel.isEmpty())
                label = customLabel;
        }
        if (map.contains("decimals") && map.value("decimals").canConvert<int>()) {
            decimals = map.value("decimals").toInt();
        }
    } else if (value.canConvert<double>()) {
        number = value.toDouble(&ok);
    }

    if (!ok) {
        number = randomValue(meta);
    }

    QVariantMap item;
    item.insert("value", number);
    item.insert("unit", unit);
    item.insert("label", label);
    item.insert("decimals", decimals);
    return item;
}
