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
    // 创建登录表单、验证码和密码显隐控件。
    setWindowTitle(QStringLiteral("智能监控系统登录界面"));
    resize(1000, 650);
    setMinimumSize(800, 550);

    // 背景图资源准备好后，从 Qt 资源系统加载
    backgroundImage.load(
        QStringLiteral(":/images/login_background.png"));

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(48, 32, 48, 32);
    mainLayout->setSpacing(0);

    mainLayout->addStretch(1);

    titleLabel = new QLabel(
        QStringLiteral("智能监控系统登录界面"), this);
    titleLabel->setObjectName(QStringLiteral("loginTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);

    mainLayout->addSpacing(28);

    // 表单区域
    formPanel = new QFrame(this);
    formPanel->setMaximumWidth(560);
    formPanel->setObjectName(QStringLiteral("loginFormPanel"));

    formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(28, 24, 28, 24);
    formLayout->setSpacing(20);

    fieldsLayout = new QFormLayout;
    fieldsLayout->setHorizontalSpacing(16);
    fieldsLayout->setVerticalSpacing(18);

    usernameEdit = new QLineEdit(formPanel);
    usernameEdit->setPlaceholderText(
        QStringLiteral("请输入用户名"));
    usernameEdit->setMinimumWidth(300);

    passwordEdit = new QLineEdit(formPanel);
    passwordEdit->setPlaceholderText(
        QStringLiteral("请输入密码"));
    passwordEdit->setEchoMode(QLineEdit::Password);

    passwordToggleButton = new QPushButton(formPanel);
    passwordToggleButton->setObjectName(
        QStringLiteral("passwordToggleButton"));
    passwordToggleButton->setIcon(
        QIcon(QStringLiteral(":/images/eye.png")));
    passwordToggleButton->setIconSize(QSize(22, 22));
    passwordToggleButton->setCheckable(true);
    passwordToggleButton->setToolTip(QStringLiteral("显示密码"));
    passwordToggleButton->setAccessibleName(
        QStringLiteral("显示密码"));

    passwordLayout = new QHBoxLayout;
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(8);
    passwordLayout->addWidget(passwordEdit, 1);
    passwordLayout->addWidget(passwordToggleButton);

    usernameErrorLabel = new QLabel(
        QStringLiteral("输入格式错误：用户名须为6-10位字母或数字"),
        formPanel);

    usernameErrorLabel->setObjectName(
        QStringLiteral("loginInputError"));
    usernameErrorLabel->hide();

    usernameFieldLayout = new QVBoxLayout;
    usernameFieldLayout->setContentsMargins(0, 0, 0, 0);
    usernameFieldLayout->setSpacing(4);
    usernameFieldLayout->addWidget(usernameEdit);
    usernameFieldLayout->addWidget(usernameErrorLabel);

    passwordErrorLabel = new QLabel(
        QStringLiteral("输入格式错误：密码须为6-8位字母或数字"),
        formPanel);

    passwordErrorLabel->setObjectName(
        QStringLiteral("loginInputError"));
    passwordErrorLabel->hide();

    passwordFieldLayout = new QVBoxLayout;
    passwordFieldLayout->setContentsMargins(0, 0, 0, 0);
    passwordFieldLayout->setSpacing(4);
    passwordFieldLayout->addLayout(passwordLayout);
    passwordFieldLayout->addWidget(passwordErrorLabel);

    captchaEdit = new QLineEdit(formPanel);
    captchaEdit->setPlaceholderText(
        QStringLiteral("请输入验证码"));
    captchaEdit->setMaxLength(4);

    // 现有 IdentifyingLabel 支持点击刷新验证码
    captchaLabel = new IdentifyingLabel(formPanel);

    QHBoxLayout *captchaRowLayout = new QHBoxLayout;
    captchaRowLayout->setSpacing(12);
    captchaRowLayout->addWidget(captchaEdit, 1);
    captchaRowLayout->addWidget(captchaLabel);

    fieldsLayout->addRow(
        QStringLiteral("用户名"), usernameFieldLayout);

    fieldsLayout->addRow(
        QStringLiteral("密码"), passwordFieldLayout);
    fieldsLayout->addRow(
        QStringLiteral("验证码"), captchaRowLayout);

    formLayout->addLayout(fieldsLayout);

    // 登录和取消按钮
    buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(20);
    buttonLayout->addStretch();

    loginButton = new QPushButton(
        QStringLiteral("登录"), formPanel);
    loginButton->setObjectName(QStringLiteral("loginButton"));
    cancelButton = new QPushButton(
        QStringLiteral("取消"), formPanel);

    loginButton->setMinimumWidth(110);
    cancelButton->setMinimumWidth(110);

    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    formLayout->addLayout(buttonLayout);

    mainLayout->addWidget(formPanel, 0, Qt::AlignHCenter);
    mainLayout->addStretch(1);

    initConnect();
    initQss();

}

void LoginWidget::resizeEvent(QResizeEvent *event)
{
    // 窗口尺寸变化时同步缩放背景图。
    QWidget::resizeEvent(event);

    if (!backgroundImage.isNull())
        updateBackground();
}

void LoginWidget::initQss()
{
    // 从Qt资源中加载登录页面样式。
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
    // 连接输入校验、登录取消及密码显隐交互。
    connect(usernameEdit, &QLineEdit::editingFinished,
            this, [this]() {
                validateUsername();
            });

    connect(passwordEdit, &QLineEdit::editingFinished,
            this, [this]() {
                validatePassword();
            });

    connect(loginButton, &QPushButton::clicked,
            this, &LoginWidget::onLoginClicked);

    connect(cancelButton, &QPushButton::clicked,
            this, [this]() {
                WinManager::getInstance()->mainWidget.show();
                this->hide();
    });

    connect(passwordToggleButton, &QPushButton::toggled,
            this, [this](bool visible) {
                passwordEdit->setEchoMode(
                    visible ? QLineEdit::Normal : QLineEdit::Password);

                const QString description = visible
                                                ? QStringLiteral("隐藏密码")
                                                : QStringLiteral("显示密码");

                passwordToggleButton->setToolTip(description);
                passwordToggleButton->setAccessibleName(description);
    });
}

void LoginWidget::updateBackground()
{
    // 将背景图片按窗口尺寸缩放后应用到窗口调色板。
    QPalette backgroundPalette = palette();

    backgroundPalette.setBrush(
        QPalette::Window,
        QBrush(backgroundImage.scaled(
            size(),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation)));

    setPalette(backgroundPalette);
}

bool LoginWidget::validateUsername()
{
    // 检查用户名非空且符合6至10位字母数字规则。
    const QString username =
        usernameEdit->text().trimmed();

    if (username.isEmpty()) {
        usernameErrorLabel->setText(
            QStringLiteral("用户名不能为空"));
        usernameErrorLabel->show();
        return false;
    }

    static const QRegularExpression usernamePattern(
        QStringLiteral("^[A-Za-z0-9]{6,10}$"));

    const bool valid =
        usernamePattern.match(username).hasMatch();

    if (!valid) {
        usernameErrorLabel->setText(
            QStringLiteral(
                "输入格式错误：用户名须为6-10位字母或数字"));
        usernameErrorLabel->show();
    } else {
        usernameErrorLabel->hide();
    }

    return valid;
}

bool LoginWidget::validatePassword()
{
    // 检查密码非空且符合6至8位字母数字规则。
    const QString password =
        passwordEdit->text();

    if (password.trimmed().isEmpty()) {
        passwordErrorLabel->setText(
            QStringLiteral("密码不能为空"));
        passwordErrorLabel->show();
        return false;
    }

    static const QRegularExpression passwordPattern(
        QStringLiteral("^[A-Za-z0-9]{6,8}$"));

    const bool valid =
        passwordPattern.match(password).hasMatch();

    if (!valid) {
        passwordErrorLabel->setText(
            QStringLiteral(
                "输入格式错误：密码须为6-8位字母或数字"));
        passwordErrorLabel->show();
    } else {
        passwordErrorLabel->hide();
    }

    return valid;
}

void LoginWidget::clearAllInputs()
{
    // 清空登录表单并恢复密码默认隐藏状态。
    usernameEdit->clear();
    passwordEdit->clear();
    captchaEdit->clear();

    usernameErrorLabel->hide();
    passwordErrorLabel->hide();

    // 清空后恢复密文输入状态。
    passwordToggleButton->setChecked(false);
}

void LoginWidget::onLoginClicked()
{
    // 依次验证验证码、输入格式和账户凭据，并处理登录结果。
    const QString attemptedUsername = usernameEdit->text().trimmed();

    // 验证码始终优先校验。
    const bool captchaCorrect =
        QString::compare(
            captchaEdit->text().trimmed(),
            captchaLabel->getCode(),
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

        captchaEdit->clear();
        captchaLabel->regenerate();
        captchaEdit->setFocus();
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
        usernameEdit->text().trimmed();

    // 认证服务接收明文仅用于本次校验，任何日志描述均不包含密码。
    const AdminUserService::LoginResult loginResult =
        adminUserService.login(
            username,
            passwordEdit->text());

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
        captchaLabel->regenerate();
        usernameEdit->setFocus();
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
        captchaLabel->regenerate();
        usernameEdit->setFocus();
        return;
    }

    logService.recordOperation(
        username, 0, QStringLiteral("login"),
        QStringLiteral("user"), 0, true,
        QStringLiteral("账号%1登录成功").arg(username));

    WinManager *manager = WinManager::getInstance();
    manager->mainWidget.setCurrentUser(username);

    clearAllInputs();
    captchaLabel->regenerate();

    manager->mainWidget.show();
    hide();
}

