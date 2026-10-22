#ifndef IDENTIFYINGLABEL_H
#define IDENTIFYINGLABEL_H
#include <QLabel>
#include <QList>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <random>

class IdentifyingLabel: public QLabel
{
    Q_OBJECT
public:
    explicit IdentifyingLabel(QWidget *parent = nullptr);
    QString getCode() const;
    void regenerate();
signals:
    void refreshed();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_code;
    QList<QColor> m_colors;
    int randomInt();
};

#endif // IDENTIFYINGLABEL_H
