class ManagedColor : QObject
{
%TypeHeaderCode
#include "ManagedColor.h"
%End

%ConvertToSubClassCode
  // Need to add sub class conversion, otherwise a ManagedColor passed to a Signal is provided as a QObject instead of
  // a ManagedColor object
  // (not sure why, but explicit conversion here fix the problem)
  ManagedColor* mc = qobject_cast<ManagedColor*>(sipCpp);
  sipType = 0;
  if (mc) {
      sipType = sipType_ManagedColor;
  }
%End

    ManagedColor(const ManagedColor & __0);
public:
    ManagedColor(const QString &colorModel, const QString &colorDepth, const QString &colorProfile, QObject *parent = 0);
    bool operator==(const ManagedColor &other) const;
    QColor colorForCanvas(Canvas *canvas) const;
    static ManagedColor *fromQColor(const QColor &qcolor, Canvas *canvas = 0);
    QString colorDepth() const;
    QString colorModel() const;
    QString colorProfile() const;
    bool setColorProfile(const QString &colorProfile);
    bool setColorSpace(const QString &colorModel, const QString &colorDepth, const QString &colorProfile);
    %If (Krita_Qt5)
    QVector<float> components() const;
    QVector<float> componentsOrdered() const;
    void setComponents(const QVector<float> &values);
    %End
    %If (!Krita_Qt5)
    QList<float> components() const;
    QList<float> componentsOrdered() const;
    void setComponents(const QList<float> &values);
    %End
    QString toXML() const;
    void fromXML(const QString &xml);
    QString toQString();

private:
};
