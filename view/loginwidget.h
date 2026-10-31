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

    QPixmap backgroundImage;

    AdminUserService adminUserService;
    LogService logService;

    QVBoxLayout *usernameFieldLayout = nullptr;
    QVBoxLayout *passwordFieldLayout = nullptr;
    QLabel *usernameErrorLabel = nullptr;
    QLabel *passwordErrorLabel = nullptr;

    QVBoxLayout *mainLayout = nullptr;
    QLabel *titleLabel = nullptr;

    QFrame *formPanel = nullptr;
    QVBoxLayout *formLayout = nullptr;
    QFormLayout *fieldsLayout = nullptr;

    QLineEdit *usernameEdit = nullptr;
    QLineEdit *passwordEdit = nullptr;
    QLineEdit *captchaEdit = nullptr;
    IdentifyingLabel *captchaLabel = nullptr;
    QHBoxLayout *passwordLayout = nullptr;
    QPushButton *passwordToggleButton = nullptr;

    QHBoxLayout *buttonLayout = nullptr;
    QPushButton *loginButton = nullptr;
    QPushButton *cancelButton = nullptr;

public slots:
    void onLoginClicked();
};

#endif // LOGINWIDGET_H
