#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QResizeEvent>

#include "identifyinglabel.h"
#include "../service/adminuserservice.h"
#include "../service/logservice.h"

class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *parent = nullptr);
signals:
protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    void initQss();
    void initConnect();
    void updateBackground();

    bool validateUsername();
    bool validatePassword();
    void clearAllInputs();

    QPixmap m_backgroundImage;

    AdminUserService m_adminUserService;
    LogService logService;

    QVBoxLayout *m_usernameFieldLayout = nullptr;
    QVBoxLayout *m_passwordFieldLayout = nullptr;
    QLabel *m_usernameErrorLabel = nullptr;
    QLabel *m_passwordErrorLabel = nullptr;

    QVBoxLayout *m_mainLayout = nullptr;
    QLabel *m_titleLabel = nullptr;

    QFrame *m_formPanel = nullptr;
    QVBoxLayout *m_formLayout = nullptr;
    QFormLayout *m_fieldsLayout = nullptr;

    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLineEdit *m_captchaEdit = nullptr;
    IdentifyingLabel *m_captchaLabel = nullptr;
    QHBoxLayout *m_passwordLayout = nullptr;
    QPushButton *m_passwordToggleButton = nullptr;

    QHBoxLayout *m_buttonLayout = nullptr;
    QPushButton *m_loginButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

public slots:
    void onLoginClicked();
};

#endif // LOGINWIDGET_H
