#include "loginwidget.h"
#include "../util/winmanager.h"
#include <QPalette>
#include <QBrush>
#include <QSizePolicy>
#include <QIcon>
#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget{parent}
{
    setWindowTitle(QStringLiteral("智能监控系统登录界面"));
    resize(1000, 650);
    setMinimumSize(800, 550);

    // 背景图资源准备好后，从 Qt 资源系统加载
    m_backgroundImage.load(
        QStringLiteral(":/images/login_background.png"));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(48, 32, 48, 32);
    m_mainLayout->setSpacing(0);

    m_mainLayout->addStretch(1);

    m_titleLabel = new QLabel(
        QStringLiteral("智能监控系统登录界面"), this);
    m_titleLabel->setObjectName(QStringLiteral("loginTitle"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel, 0, Qt::AlignHCenter);

    m_mainLayout->addSpacing(28);

    // 表单区域
    m_formPanel = new QFrame(this);
    m_formPanel->setMaximumWidth(560);
    m_formPanel->setObjectName(QStringLiteral("loginFormPanel"));

    m_formLayout = new QVBoxLayout(m_formPanel);
    m_formLayout->setContentsMargins(28, 24, 28, 24);
    m_formLayout->setSpacing(20);

    m_fieldsLayout = new QFormLayout;
    m_fieldsLayout->setHorizontalSpacing(16);
    m_fieldsLayout->setVerticalSpacing(18);

    m_usernameEdit = new QLineEdit(m_formPanel);
    m_usernameEdit->setPlaceholderText(
        QStringLiteral("请输入用户名"));
    m_usernameEdit->setMinimumWidth(300);

    m_passwordEdit = new QLineEdit(m_formPanel);
    m_passwordEdit->setPlaceholderText(
        QStringLiteral("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_passwordToggleButton = new QPushButton(m_formPanel);
    m_passwordToggleButton->setObjectName(
        QStringLiteral("passwordToggleButton"));
    m_passwordToggleButton->setIcon(
        QIcon(QStringLiteral(":/images/eye.png")));
    m_passwordToggleButton->setIconSize(QSize(22, 22));
    m_passwordToggleButton->setCheckable(true);
    m_passwordToggleButton->setToolTip(QStringLiteral("显示密码"));
    m_passwordToggleButton->setAccessibleName(
        QStringLiteral("显示密码"));

    m_passwordLayout = new QHBoxLayout;
    m_passwordLayout->setContentsMargins(0, 0, 0, 0);
    m_passwordLayout->setSpacing(8);
    m_passwordLayout->addWidget(m_passwordEdit, 1);
    m_passwordLayout->addWidget(m_passwordToggleButton);

    m_usernameErrorLabel = new QLabel(
        QStringLiteral("输入格式错误：用户名须为6-10位字母或数字"),
        m_formPanel);

    m_usernameErrorLabel->setObjectName(
        QStringLiteral("loginInputError"));
    m_usernameErrorLabel->hide();

    m_usernameFieldLayout = new QVBoxLayout;
    m_usernameFieldLayout->setContentsMargins(0, 0, 0, 0);
    m_usernameFieldLayout->setSpacing(4);
    m_usernameFieldLayout->addWidget(m_usernameEdit);
    m_usernameFieldLayout->addWidget(m_usernameErrorLabel);

    m_passwordErrorLabel = new QLabel(
        QStringLiteral("输入格式错误：密码须为6-8位字母或数字"),
        m_formPanel);

    m_passwordErrorLabel->setObjectName(
        QStringLiteral("loginInputError"));
    m_passwordErrorLabel->hide();

    m_passwordFieldLayout = new QVBoxLayout;
    m_passwordFieldLayout->setContentsMargins(0, 0, 0, 0);
    m_passwordFieldLayout->setSpacing(4);
    m_passwordFieldLayout->addLayout(m_passwordLayout);
    m_passwordFieldLayout->addWidget(m_passwordErrorLabel);

    m_captchaEdit = new QLineEdit(m_formPanel);
    m_captchaEdit->setPlaceholderText(
        QStringLiteral("请输入验证码"));
    m_captchaEdit->setMaxLength(4);

    // 现有 IdentifyingLabel 支持点击刷新验证码
    m_captchaLabel = new IdentifyingLabel(m_formPanel);

    QHBoxLayout *captchaRowLayout = new QHBoxLayout;
    captchaRowLayout->setSpacing(12);
    captchaRowLayout->addWidget(m_captchaEdit, 1);
    captchaRowLayout->addWidget(m_captchaLabel);

    m_fieldsLayout->addRow(
        QStringLiteral("用户名"), m_usernameFieldLayout);

    m_fieldsLayout->addRow(
        QStringLiteral("密码"), m_passwordFieldLayout);
    m_fieldsLayout->addRow(
        QStringLiteral("验证码"), captchaRowLayout);

    m_formLayout->addLayout(m_fieldsLayout);

    // 登录和取消按钮
    m_buttonLayout = new QHBoxLayout;
    m_buttonLayout->setSpacing(20);
    m_buttonLayout->addStretch();

    m_loginButton = new QPushButton(
        QStringLiteral("登录"), m_formPanel);
    m_loginButton->setObjectName(QStringLiteral("loginButton"));
    m_cancelButton = new QPushButton(
        QStringLiteral("取消"), m_formPanel);

    m_loginButton->setMinimumWidth(110);
    m_cancelButton->setMinimumWidth(110);

    m_buttonLayout->addWidget(m_loginButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addStretch();

    m_formLayout->addLayout(m_buttonLayout);

    m_mainLayout->addWidget(m_formPanel, 0, Qt::AlignHCenter);
    m_mainLayout->addStretch(1);

    initConnect();
    initQss();

}

void LoginWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!m_backgroundImage.isNull())
        updateBackground();
}

void LoginWidget::initQss()
{
    QFile file(QStringLiteral(":/styles/loginwidget.qss"));

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(file.readAll()));
    } else {
        qWarning() << "登录页面样式加载失败："
                   << file.errorString();
    }
}

void LoginWidget::initConnect()
{
    connect(m_usernameEdit, &QLineEdit::editingFinished,
            this, [this]() {
                validateUsername();
            });

    connect(m_passwordEdit, &QLineEdit::editingFinished,
            this, [this]() {
                validatePassword();
            });

    connect(m_loginButton, &QPushButton::clicked,
            this, &LoginWidget::onLoginClicked);

    connect(m_cancelButton, &QPushButton::clicked,
            this, [this]() {
                WinManager::getInstance()->mainWidget.show();
                this->hide();
    });

    connect(m_passwordToggleButton, &QPushButton::toggled,
            this, [this](bool visible) {
                m_passwordEdit->setEchoMode(
                    visible ? QLineEdit::Normal : QLineEdit::Password);

                const QString description = visible
                                                ? QStringLiteral("隐藏密码")
                                                : QStringLiteral("显示密码");

                m_passwordToggleButton->setToolTip(description);
                m_passwordToggleButton->setAccessibleName(description);
    });
}

void LoginWidget::updateBackground()
{
    QPalette backgroundPalette = palette();

    backgroundPalette.setBrush(
        QPalette::Window,
        QBrush(m_backgroundImage.scaled(
            size(),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation)));

    setPalette(backgroundPalette);
}

bool LoginWidget::validateUsername()
{
    const QString username =
        m_usernameEdit->text().trimmed();

    if (username.isEmpty()) {
        m_usernameErrorLabel->setText(
            QStringLiteral("用户名不能为空"));
        m_usernameErrorLabel->show();
        return false;
    }

    static const QRegularExpression usernamePattern(
        QStringLiteral("^[A-Za-z0-9]{6,10}$"));

    const bool valid =
        usernamePattern.match(username).hasMatch();

    if (!valid) {
        m_usernameErrorLabel->setText(
            QStringLiteral(
                "输入格式错误：用户名须为6-10位字母或数字"));
        m_usernameErrorLabel->show();
    } else {
        m_usernameErrorLabel->hide();
    }

    return valid;
}

bool LoginWidget::validatePassword()
{
    const QString password =
        m_passwordEdit->text();

    if (password.trimmed().isEmpty()) {
        m_passwordErrorLabel->setText(
            QStringLiteral("密码不能为空"));
        m_passwordErrorLabel->show();
        return false;
    }

    static const QRegularExpression passwordPattern(
        QStringLiteral("^[A-Za-z0-9]{6,8}$"));

    const bool valid =
        passwordPattern.match(password).hasMatch();

    if (!valid) {
        m_passwordErrorLabel->setText(
            QStringLiteral(
                "输入格式错误：密码须为6-8位字母或数字"));
        m_passwordErrorLabel->show();
    } else {
        m_passwordErrorLabel->hide();
    }

    return valid;
}

void LoginWidget::clearAllInputs()
{
    m_usernameEdit->clear();
    m_passwordEdit->clear();
    m_captchaEdit->clear();

    m_usernameErrorLabel->hide();
    m_passwordErrorLabel->hide();

    // 清空后恢复密文输入状态。
    m_passwordToggleButton->setChecked(false);
}

void LoginWidget::onLoginClicked()
{
    const QString attemptedUsername = m_usernameEdit->text().trimmed();

    // 验证码始终优先校验。
    const bool captchaCorrect =
        QString::compare(
            m_captchaEdit->text().trimmed(),
            m_captchaLabel->getCode(),
            Qt::CaseInsensitive) == 0;

    if (!captchaCorrect) {
        logService.recordOperation(
            attemptedUsername, 0, QStringLiteral("login"),
            QStringLiteral("user"), 0, false,
            QStringLiteral("账号%1登录失败：验证码错误")
                .arg(attemptedUsername.isEmpty()
                         ? QStringLiteral("未填写") : attemptedUsername));

        QMessageBox::warning(
            this,
            QStringLiteral("登录失败"),
            QStringLiteral("验证码错误"));

        m_captchaEdit->clear();
        m_captchaLabel->regenerate();
        m_captchaEdit->setFocus();
        return;
    }

    const bool usernameValid = validateUsername();
    const bool passwordValid = validatePassword();

    if (!usernameValid || !passwordValid) {
        logService.recordOperation(
            attemptedUsername, 0, QStringLiteral("login"),
            QStringLiteral("user"), 0, false,
            QStringLiteral("账号%1登录失败：用户名或密码格式错误")
                .arg(attemptedUsername.isEmpty()
                         ? QStringLiteral("未填写") : attemptedUsername));
        return;
    }

    const QString username =
        m_usernameEdit->text().trimmed();

    const AdminUserService::LoginResult loginResult =
        m_adminUserService.login(
            username,
            m_passwordEdit->text());

    if (loginResult ==
        AdminUserService::LoginResult::DatabaseError) {
        logService.recordOperation(
            username, 0, QStringLiteral("login"),
            QStringLiteral("user"), 0, false,
            QStringLiteral("账号%1登录失败：数据库连接或查询失败").arg(username));
        QMessageBox::warning(
            this,
            QStringLiteral("登录失败"),
            QStringLiteral("数据库连接或查询失败"));
        return;
    }

    if (loginResult ==
        AdminUserService::LoginResult::AccountLocked) {
        logService.recordOperation(
            username, 0, QStringLiteral("login"),
            QStringLiteral("user"), 0, false,
            QStringLiteral("账号%1登录失败：账号处于锁定状态").arg(username));
        QMessageBox::warning(
            this,
            QStringLiteral("登录失败"),
            QStringLiteral(
                "当前账号已锁定，请30秒后再尝试登录"));

        clearAllInputs();
        m_captchaLabel->regenerate();
        m_usernameEdit->setFocus();
        return;
    }

    if (loginResult ==
        AdminUserService::LoginResult::InvalidCredentials) {
        logService.recordOperation(
            username, 0, QStringLiteral("login"),
            QStringLiteral("user"), 0, false,
            QStringLiteral("账号%1登录失败：用户名或密码错误").arg(username));
        QMessageBox::warning(
            this,
            QStringLiteral("登录失败"),
            QStringLiteral("用户名或密码错误"));

        clearAllInputs();
        m_captchaLabel->regenerate();
        m_usernameEdit->setFocus();
        return;
    }

    logService.recordOperation(
        username, 0, QStringLiteral("login"),
        QStringLiteral("user"), 0, true,
        QStringLiteral("账号%1登录成功").arg(username));

    WinManager *manager = WinManager::getInstance();
    manager->mainWidget.setCurrentUser(username);

    clearAllInputs();
    m_captchaLabel->regenerate();

    manager->mainWidget.show();
    hide();
}

