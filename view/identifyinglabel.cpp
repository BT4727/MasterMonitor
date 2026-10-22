#include "identifyinglabel.h"

IdentifyingLabel::IdentifyingLabel(QWidget *parent) : QLabel(parent)
{
    setFixedSize(100, 42);
    setCursor(Qt::PointingHandCursor);
    setToolTip("点击刷新");
    m_colors << QColor("#2c3e50") << QColor("#8e44ad") << QColor("#16a085") << QColor("#c0392b");
    regenerate();
}

QString IdentifyingLabel::getCode() const
{
    return m_code;
}

void IdentifyingLabel::regenerate()
{
    const QString chars = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    m_code.clear();
    for(int i=0; i<4; ++i) {
        int index = randomInt() % chars.length();
        m_code.append(chars.at(index));
    }
    update();
    emit refreshed();
}

void IdentifyingLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) regenerate();
}

void IdentifyingLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#f9f9f9"));
    painter.setPen(QColor("#e0e0e0"));
    painter.drawRect(rect().adjusted(0,0,-1,-1));

    for(int i=0; i<30; ++i) {
        painter.setPen(m_colors[randomInt() % m_colors.size()]);
        painter.drawPoint(randomInt() % width(), randomInt() % height());
    }
    // 简单画点干扰线
    for(int i=0; i<4; ++i) {
        painter.setPen(QPen(m_colors[randomInt() % m_colors.size()], 1));
        painter.drawLine(randomInt() % width(), randomInt() % height(),
                         randomInt() % width(), randomInt() % height());
    }

    QFont font("Georgia", 18, QFont::Bold);
    painter.setFont(font);
    int charWidth = width() / 4;
    for(int i=0; i<4; ++i) {
        painter.setPen(m_colors[randomInt() % m_colors.size()]);
        painter.save();
        int x = i * charWidth + charWidth/2 - 6;
        int y = height() / 2 + 6;
        painter.translate(x, y);
        painter.rotate(-15 + randomInt() % 30);
        painter.drawText(0, 0, QString(m_code.at(i)));
        painter.restore();
    }
}

int IdentifyingLabel::randomInt()
{
    static std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(0,RAND_MAX);
    return dist(engine);
}
