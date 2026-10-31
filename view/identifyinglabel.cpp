#include "identifyinglabel.h"

IdentifyingLabel::IdentifyingLabel(QWidget *parent) : QLabel(parent)
{
    // 初始化可点击刷新的验证码控件并生成首个验证码。
    setFixedSize(100, 42);
    setCursor(Qt::PointingHandCursor);
    setToolTip("点击刷新");
    colors << QColor("#2c3e50") << QColor("#8e44ad") << QColor("#16a085") << QColor("#c0392b");
    regenerate();
}

QString IdentifyingLabel::getCode() const
{
    // 返回当前绘制的验证码文本。
    return code;
}

void IdentifyingLabel::regenerate()
{
    // 生成新的四位验证码并请求界面重绘。
    const QString chars = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    code.clear();
    for(int i=0; i<4; ++i) {
        int index = randomInt() % chars.length();
        code.append(chars.at(index));
    }
    update();
    emit refreshed();
}

void IdentifyingLabel::mousePressEvent(QMouseEvent *event)
{
    // 点击验证码区域时重新生成验证码。
    if (event->button() == Qt::LeftButton) regenerate();
}

void IdentifyingLabel::paintEvent(QPaintEvent *event)
{
    // 绘制验证码字符及随机干扰点、干扰线。
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#f9f9f9"));
    painter.setPen(QColor("#e0e0e0"));
    painter.drawRect(rect().adjusted(0,0,-1,-1));

    for(int i=0; i<30; ++i) {
        painter.setPen(colors[randomInt() % colors.size()]);
        painter.drawPoint(randomInt() % width(), randomInt() % height());
    }
    // 简单画点干扰线
    for(int i=0; i<4; ++i) {
        painter.setPen(QPen(colors[randomInt() % colors.size()], 1));
        painter.drawLine(randomInt() % width(), randomInt() % height(),
                         randomInt() % width(), randomInt() % height());
    }

    QFont font("Georgia", 18, QFont::Bold);
    painter.setFont(font);
    int charWidth = width() / 4;
    for(int i=0; i<4; ++i) {
        painter.setPen(colors[randomInt() % colors.size()]);
        painter.save();
        int x = i * charWidth + charWidth/2 - 6;
        int y = height() / 2 + 6;
        painter.translate(x, y);
        painter.rotate(-15 + randomInt() % 30);
        painter.drawText(0, 0, QString(code.at(i)));
        painter.restore();
    }
}

int IdentifyingLabel::randomInt()
{
    // 返回验证码绘制使用的随机整数。
    static std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(0,RAND_MAX);
    return dist(engine);
}
