class Shape : QObject
{
%TypeHeaderCode
#include "Shape.h"
%End

%ConvertToSubClassCode
  // Need to add sub class conversion, otherwise a Shape passed to a Signal is provided as a QObject instead of
  // a Shape object
  // (not sure why, but explicit conversion here fix the problem)
  Shape* mc = qobject_cast<Shape*>(sipCpp);
  sipType = 0;
  if (mc) {
      sipType = sipType_Shape;
  }
%End

    Shape(const Shape & __0);
    virtual ~Shape();
    bool operator==(const Shape &other) const;
    bool operator!=(const Shape &other) const;

public Q_SLOTS:
    QString type() const;
    QString name() const;
    void setName(const QString &name);
    int zIndex() const;
    void setZIndex(int zindex);
    bool visible() const;
    void setVisible(bool visible);
    bool selectable() const;
    void setSelectable(bool selectable);
    bool geometryProtected() const;
    void setGeometryProtected(bool protect);
    QRectF boundingBox() const;
    QPointF position() const;
    void setPosition(QPointF point);
    QTransform transformation() const;
    void setTransformation(QTransform matrix);
    QTransform absoluteTransformation() const;
    bool remove();
    void update();
    void updateAbsolute(QRectF box);
    QString toSvg(bool prependStyles = false, bool stripTextMode = true);
    void select();
    void deselect();
    bool isSelected();
    Shape * parentShape() const /Factory/;
Q_SIGNALS:
private:
};
