#ifndef ENVIRONMENTDATAPROVIDER_H
#define ENVIRONMENTDATAPROVIDER_H

#include <QObject>
#include <QVariantMap>

class EnvironmentDataProvider : public QObject
{
    Q_OBJECT
public:
    explicit EnvironmentDataProvider(QObject *parent = nullptr);

    // 生成随机环境参数（用于联调）
    Q_INVOKABLE QVariantMap generateRandom();
    // 规范化输入数据，补全缺失字段并保持统一结构
    Q_INVOKABLE QVariantMap normalize(const QVariantMap &data);

private:
    struct ParamMeta {
        QString key;
        QString label;
        QString unit;
        double min;
        double max;
        int decimals;
    };

    QList<ParamMeta> m_params;

    double randomValue(const ParamMeta &meta) const;
    QVariantMap normalizeItem(const QVariant &value, const ParamMeta &meta) const;
};

#endif // ENVIRONMENTDATAPROVIDER_H
