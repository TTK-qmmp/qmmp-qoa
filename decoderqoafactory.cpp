#include "decoderqoafactory.h"
#include "decoder_qoa.h"
#include "qoahelper.h"

#include <QMessageBox>

bool DecoderQOAFactory::canDecode(QIODevice *input) const
{
    const QFile * const file = qobject_cast<QFile*>(input);
    if(!file)
    {
        return false;
    }

    QOAHelper helper(file->fileName());
    return helper.initialize();
}

DecoderProperties DecoderQOAFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("QOA Plugin");
    properties.shortName = "qoa";
    properties.filters << "*.qoa";
    properties.description = "Quite OK Audio File";
    properties.protocols << "file";
    properties.noInput = true;
    properties.hasAbout = true;
    return properties;
}

Decoder *DecoderQOAFactory::create(const QString &path, QIODevice *input)
{
    Q_UNUSED(input);
    return new DecoderQOA(path);
}

QList<TrackInfo*> DecoderQOAFactory::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *)
{
    TrackInfo *info = new TrackInfo(path);
    if(parts == TrackInfo::Parts())
    {
        return QList<TrackInfo*>() << info;
    }

    QOAHelper helper(path);
    if(!helper.initialize())
    {
        delete info;
        return QList<TrackInfo*>();
    }

    if(parts & TrackInfo::Properties)
    {
        info->setValue(Qmmp::BITRATE, helper.bitrate());
        info->setValue(Qmmp::SAMPLERATE, helper.sampleRate());
        info->setValue(Qmmp::CHANNELS, helper.channels());
        info->setValue(Qmmp::BITS_PER_SAMPLE, helper.depth());
        info->setValue(Qmmp::FORMAT_NAME, "Quite OK Audio");
        info->setDuration(helper.totalTime());
    }
    return QList<TrackInfo*>() << info;
}

MetaDataModel* DecoderQOAFactory::createMetaDataModel(const QString &path, bool readOnly)
{
    Q_UNUSED(path);
    Q_UNUSED(readOnly);
    return nullptr;
}

void DecoderQOAFactory::showSettings(QWidget *parent)
{
    Q_UNUSED(parent);
}

void DecoderQOAFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About QOA Reader Plugin"),
                       tr("Qmmp QOA Reader Plugin") + "\n" +
                       tr("Written by: Greedysky <greedysky@163.com>"));
}

QString DecoderQOAFactory::translation() const
{
    return QString();
}
